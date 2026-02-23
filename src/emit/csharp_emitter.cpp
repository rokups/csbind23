#include "csbind23/emit/csharp_emitter.hpp"

#include "csbind23/text_writer.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace csbind23::emit
{
namespace
{

std::filesystem::path write_csharp_file(
    const std::filesystem::path& output_root, const std::string& filename, const std::string& content)
{
    const auto output_path = output_root / filename;
    std::ofstream output(output_path, std::ios::binary);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return output_path;
}
std::string pinvoke_return_type(const FunctionDecl& function_decl)
{
    if (function_decl.is_constructor)
    {
        return "System.IntPtr";
    }
    return function_decl.return_type.pinvoke_name;
}

std::string wrapper_type_name(const TypeRef& type_ref)
{
    if (type_ref.managed_converter.has_value() && !type_ref.managed_converter->managed_type_name.empty())
    {
        return type_ref.managed_converter->managed_type_name;
    }
    return type_ref.pinvoke_name;
}

std::string wrapper_return_type(const FunctionDecl& function_decl)
{
    if (function_decl.return_type.c_abi_name == "void")
    {
        return "void";
    }
    return wrapper_type_name(function_decl.return_type);
}

std::string replace_all(std::string value, std::string_view from, std::string_view to)
{
    std::size_t start = 0;
    while (true)
    {
        const std::size_t index = value.find(from, start);
        if (index == std::string::npos)
        {
            break;
        }
        value.replace(index, from.size(), to);
        start = index + to.size();
    }
    return value;
}

std::string render_inline_template(
    std::string templ, std::string_view managed_name, std::string_view pinvoke_name, std::string_view value_name)
{
    templ = replace_all(std::move(templ), "{managed}", managed_name);
    templ = replace_all(std::move(templ), "{pinvoke}", pinvoke_name);
    templ = replace_all(std::move(templ), "{value}", value_name);
    return templ;
}

std::string join_arguments(const std::vector<std::string>& arguments)
{
    std::string rendered;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        rendered += arguments[index];
        if (index + 1 < arguments.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

std::string parameter_list_without_self(const FunctionDecl& function_decl)
{
    std::string rendered;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        rendered += std::format("{} {}", wrapper_type_name(parameter.type), parameter.name);
        if (index + 1 < function_decl.parameters.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

void append_native_signature(TextWriter& output, const FunctionDecl& function_decl, std::string_view pinvoke_library,
    const std::string& exported_name, bool include_self)
{
    output.append_line_format(
        "    [System.Runtime.InteropServices.DllImport(\"{}\", CallingConvention = "
        "System.Runtime.InteropServices.CallingConvention.Cdecl)]",
        pinvoke_library);
    output.append_format("    internal static extern {} {}(", pinvoke_return_type(function_decl), exported_name);

    bool needs_separator = false;
    if (include_self)
    {
        output.append("System.IntPtr self");
        needs_separator = true;
    }

    for (const auto& parameter : function_decl.parameters)
    {
        if (needs_separator)
        {
            output.append(", ");
        }
        output.append_format("{} {}", parameter.type.pinvoke_name, parameter.name);
        needs_separator = true;
    }

    output.append_line(");");
    output.append_line();
}

bool class_has_owned_ctor(const ClassDecl& class_decl)
{
    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor && infer_ownership(method_decl) == Ownership::Owned)
        {
            return true;
        }
    }
    return false;
}

void append_wrapper_method(
    TextWriter& output, const std::string& module_name, const ClassDecl& class_decl, const FunctionDecl& method_decl)
{
    const std::string parameter_list = parameter_list_without_self(method_decl);
    const std::string native_name = std::format("{}_{}_{}", module_name, class_decl.name, method_decl.name);
    const std::string return_type = wrapper_return_type(method_decl);

    output.append_format("    public {} {}(", return_type, method_decl.name);
    output.append(parameter_list);
    output.append_line(")");
    output.append_line("    {");

    std::vector<std::string> call_arguments;
    std::vector<std::string> finalize_statements;
    call_arguments.reserve(method_decl.parameters.size());
    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        const auto& parameter = method_decl.parameters[index];
        const auto& converter = parameter.type.managed_converter;
        if (converter.has_value() && !converter->to_pinvoke_expression.empty())
        {
            const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
            output.append_line_format(
                "        {} {} = {};",
                parameter.type.pinvoke_name,
                pinvoke_name,
                render_inline_template(converter->to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name));
            call_arguments.push_back(pinvoke_name);

            if (!converter->finalize_to_pinvoke_statement.empty())
            {
                finalize_statements.push_back(render_inline_template(
                    converter->finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name));
            }
            continue;
        }

        call_arguments.push_back(parameter.name);
    }
    const std::string call_arguments_rendered = join_arguments(call_arguments);
    const std::string native_call = call_arguments_rendered.empty()
        ? std::format("{}Native.{}(_handle)", module_name, native_name)
        : std::format("{}Native.{}(_handle, {})", module_name, native_name, call_arguments_rendered);

    const bool has_return_converter = method_decl.return_type.managed_converter.has_value()
        && !method_decl.return_type.managed_converter->from_pinvoke_expression.empty();
    const bool has_return_finalize = method_decl.return_type.managed_converter.has_value()
        && !method_decl.return_type.managed_converter->finalize_from_pinvoke_statement.empty();
    const bool needs_finally = !finalize_statements.empty() || has_return_finalize;

    if (return_type == "void")
    {
        if (!needs_finally)
        {
            output.append_line_format("        {}();", native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {}();", native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                output.append_line_format("            {};", finalize_statement);
            }
            output.append_line("        }");
        }
        output.append_line("    }");
        output.append_line();
        return;
    }

    if (!has_return_converter)
    {
        if (!needs_finally)
        {
            output.append_line_format("        return {};", native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            return {};", native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                output.append_line_format("            {};", finalize_statement);
            }
            output.append_line("        }");
        }
    }
    else
    {
        const auto& converter = *method_decl.return_type.managed_converter;
        const std::string native_result_name = "__csbind23_result_pinvoke";
        const std::string managed_result_name = "__csbind23_result_managed";
        const std::string converted_expression =
            render_inline_template(converter.from_pinvoke_expression, managed_result_name, native_result_name,
                native_result_name);

        if (!needs_finally)
        {
            output.append_line_format("        {} {} = {};", method_decl.return_type.pinvoke_name, native_result_name,
                native_call);
            output.append_line_format("        {} {} = {};", return_type, managed_result_name, converted_expression);
            output.append_line_format("        return {};", managed_result_name);
        }
        else
        {
            output.append_line_format("        {} {} = default!;", method_decl.return_type.pinvoke_name, native_result_name);
            output.append_line_format("        {} {} = default!;", return_type, managed_result_name);
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {} = {};", native_result_name, native_call);
            output.append_line_format("            {} = {};", managed_result_name, converted_expression);
            output.append_line_format("            return {};", managed_result_name);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                output.append_line_format("            {};", finalize_statement);
            }
            if (has_return_finalize)
            {
                output.append_line_format(
                    "            {};",
                    render_inline_template(
                        converter.finalize_from_pinvoke_statement, managed_result_name, native_result_name,
                        native_result_name));
            }
            output.append_line("        }");
        }
    }

    output.append_line("    }");
    output.append_line();
}

void append_wrapper_class(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl)
{
    const bool emits_destroy = class_has_owned_ctor(class_decl);

    output.append_line_format("public sealed class {} : System.IDisposable", class_decl.name);
    output.append_line("{");
    output.append_line("    private System.IntPtr _handle;");
    output.append_line("    private bool _ownsHandle;");
    output.append_line();

    output.append_line_format("    internal {}(System.IntPtr handle, bool ownsHandle)", class_decl.name);
    output.append_line("    {");
    output.append_line("        _handle = handle;");
    output.append_line("        _ownsHandle = ownsHandle;");
    output.append_line("    }");
    output.append_line();

    for (const auto& method_decl : class_decl.methods)
    {
        if (!method_decl.is_constructor)
        {
            continue;
        }

        const std::string parameter_list = parameter_list_without_self(method_decl);
        const std::string create_name = std::format("{}_{}_create", module_name, class_decl.name);
        const bool owns_handle = infer_ownership(method_decl) == Ownership::Owned;

        output.append_format("    public {}(", class_decl.name);
        output.append(parameter_list);
        output.append_line(")");
        output.append_line("    {");

        std::vector<std::string> converted_arguments;
        std::vector<std::string> finalize_statements;
        converted_arguments.reserve(method_decl.parameters.size());
        for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
        {
            const auto& parameter = method_decl.parameters[index];
            const auto& converter = parameter.type.managed_converter;
            if (converter.has_value() && !converter->to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                output.append_line_format(
                    "        {} {} = {};",
                    parameter.type.pinvoke_name,
                    pinvoke_name,
                    render_inline_template(
                        converter->to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name));
                converted_arguments.push_back(pinvoke_name);
                if (!converter->finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        converter->finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name));
                }
                continue;
            }

            converted_arguments.push_back(parameter.name);
        }

        const std::string converted_arguments_rendered = join_arguments(converted_arguments);
        const std::string native_call = converted_arguments_rendered.empty()
            ? std::format("{}Native.{}()", module_name, create_name)
            : std::format("{}Native.{}({})", module_name, create_name, converted_arguments_rendered);

        if (finalize_statements.empty())
        {
            output.append_line_format("        _handle = {};", native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            _handle = {};", native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                output.append_line_format("            {};", finalize_statement);
            }
            output.append_line("        }");
        }

        output.append_line_format("        _ownsHandle = {};", owns_handle ? "true" : "false");
        output.append_line("    }");
        output.append_line();
    }

    output.append_line("    public void Dispose()");
    output.append_line("    {");
    output.append_line("        ReleaseUnmanaged();");
    output.append_line("        System.GC.SuppressFinalize(this);");
    output.append_line("    }");
    output.append_line();

    output.append_line_format("    ~{}()", class_decl.name);
    output.append_line("    {");
    output.append_line("        ReleaseUnmanaged();");
    output.append_line("    }");
    output.append_line();

    output.append_line("    private void ReleaseUnmanaged()");
    output.append_line("    {");
    output.append_line("        if (_handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");
    output.append_line();
    output.append_line("        if (_ownsHandle)");
    output.append_line("        {");
    if (emits_destroy)
    {
        output.append_line_format(
            "            {}Native.{}_{}_destroy(_handle);", module_name, module_name, class_decl.name);
    }
    output.append_line("        }");
    output.append_line();
    output.append_line("        _handle = System.IntPtr.Zero;");
    output.append_line("        _ownsHandle = false;");
    output.append_line("    }");
    output.append_line();

    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor)
        {
            continue;
        }
        append_wrapper_method(output, module_name, class_decl, method_decl);
    }

    output.append_line("}");
    output.append_line();
}

} // namespace

std::vector<std::filesystem::path> emit_csharp_module(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root)
{
    std::filesystem::create_directories(output_root);

    std::vector<std::filesystem::path> generated_files;

    TextWriter generated(2048);

    generated.append_line("namespace CsBind23.Generated;");
    generated.append_line();
    generated.append_line_format("internal static class {}Native", module_decl.name);
    generated.append_line("{");

    for (const auto& function_decl : module_decl.functions)
    {
        append_native_signature(generated, function_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + function_decl.name, false);
    }

    for (const auto& class_decl : module_decl.classes)
    {
        if (class_has_owned_ctor(class_decl))
        {
            FunctionDecl destroy_decl;
            destroy_decl.return_type.c_abi_name = "void";
            destroy_decl.return_type.pinvoke_name = "void";
            append_native_signature(generated, destroy_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_destroy", true);
        }

        for (const auto& method_decl : class_decl.methods)
        {
            if (method_decl.is_constructor)
            {
                append_native_signature(
                    generated, method_decl, module_decl.pinvoke_library,
                    module_decl.name + "_" + class_decl.name + "_create", false);
            }
            else
            {
                append_native_signature(generated, method_decl, module_decl.pinvoke_library,
                    module_decl.name + "_" + class_decl.name + "_" + method_decl.name, true);
            }
        }
    }

    generated.append_line("}");
    generated.append_line();

    generated.append_line_format("public static class {}Api", module_decl.name);
    generated.append_line("{");
    for (const auto& function_decl : module_decl.functions)
    {
        const std::string params = parameter_list_without_self(function_decl);
        const std::string return_type = wrapper_return_type(function_decl);
        const std::string native_name = module_decl.name + "_" + function_decl.name;

        generated.append_format("    public static {} {}(", return_type, function_decl.name);
        generated.append(params);
        generated.append_line(")");
        generated.append_line("    {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        call_arguments.reserve(function_decl.parameters.size());
        for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
        {
            const auto& parameter = function_decl.parameters[index];
            const auto& converter = parameter.type.managed_converter;
            if (converter.has_value() && !converter->to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                generated.append_line_format(
                    "        {} {} = {};",
                    parameter.type.pinvoke_name,
                    pinvoke_name,
                    render_inline_template(converter->to_pinvoke_expression, parameter.name, pinvoke_name,
                        parameter.name));
                call_arguments.push_back(pinvoke_name);
                if (!converter->finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        converter->finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name));
                }
                continue;
            }

            call_arguments.push_back(parameter.name);
        }

        const std::string call_arguments_rendered = join_arguments(call_arguments);
        const std::string native_call = std::format("{}Native.{}({})", module_decl.name, native_name, call_arguments_rendered);
        const bool has_return_converter = function_decl.return_type.managed_converter.has_value()
            && !function_decl.return_type.managed_converter->from_pinvoke_expression.empty();
        const bool has_return_finalize = function_decl.return_type.managed_converter.has_value()
            && !function_decl.return_type.managed_converter->finalize_from_pinvoke_statement.empty();
        const bool needs_finally = !finalize_statements.empty() || has_return_finalize;

        if (return_type == "void")
        {
            if (!needs_finally)
            {
                generated.append_line_format("        {};", native_call);
            }
            else
            {
                generated.append_line("        try");
                generated.append_line("        {");
                generated.append_line_format("            {};", native_call);
                generated.append_line("        }");
                generated.append_line("        finally");
                generated.append_line("        {");
                for (const auto& finalize_statement : finalize_statements)
                {
                    generated.append_line_format("            {};", finalize_statement);
                }
                generated.append_line("        }");
            }
        }
        else
        {
            if (!has_return_converter)
            {
                if (!needs_finally)
                {
                    generated.append_line_format("        return {};", native_call);
                }
                else
                {
                    generated.append_line("        try");
                    generated.append_line("        {");
                    generated.append_line_format("            return {};", native_call);
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        generated.append_line_format("            {};", finalize_statement);
                    }
                    generated.append_line("        }");
                }
            }
            else
            {
                const auto& converter = *function_decl.return_type.managed_converter;
                const std::string native_result_name = "__csbind23_result_pinvoke";
                const std::string managed_result_name = "__csbind23_result_managed";
                const std::string converted_expression =
                    render_inline_template(converter.from_pinvoke_expression, managed_result_name, native_result_name,
                        native_result_name);

                if (!needs_finally)
                {
                    generated.append_line_format(
                        "        {} {} = {};", function_decl.return_type.pinvoke_name, native_result_name, native_call);
                    generated.append_line_format(
                        "        {} {} = {};", return_type, managed_result_name, converted_expression);
                    generated.append_line_format("        return {};", managed_result_name);
                }
                else
                {
                    generated.append_line_format(
                        "        {} {} = default!;", function_decl.return_type.pinvoke_name, native_result_name);
                    generated.append_line_format("        {} {} = default!;", return_type, managed_result_name);
                    generated.append_line("        try");
                    generated.append_line("        {");
                    generated.append_line_format("            {} = {};", native_result_name, native_call);
                    generated.append_line_format("            {} = {};", managed_result_name, converted_expression);
                    generated.append_line_format("            return {};", managed_result_name);
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        generated.append_line_format("            {};", finalize_statement);
                    }
                    if (has_return_finalize)
                    {
                        generated.append_line_format(
                            "            {};",
                            render_inline_template(
                                converter.finalize_from_pinvoke_statement, managed_result_name, native_result_name,
                                native_result_name));
                    }
                    generated.append_line("        }");
                }
            }
        }
        generated.append_line("    }");
        generated.append_line();
    }
    generated.append_line("}");
    generated.append_line();

    const auto& written = generated.str();
    generated_files.push_back(write_csharp_file(output_root, module_decl.name + ".g.cs", written));

    for (const auto& class_decl : module_decl.classes)
    {
        TextWriter class_file(1024);
        class_file.append_line("namespace CsBind23.Generated;");
        class_file.append_line();
        append_wrapper_class(class_file, module_decl.name, class_decl);

        const auto& class_written = class_file.str();
        generated_files.push_back(
            write_csharp_file(output_root, module_decl.name + "." + class_decl.name + ".g.cs", class_written));
    }

    return generated_files;
}

} // namespace csbind23::emit
