#include "csbind23/emit/csharp_emitter.hpp"

#include "csbind23/text_writer.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <cstdint>
#include <stdexcept>
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

bool module_uses_pinvoke_type(const ModuleDecl& module_decl, std::string_view pinvoke_type)
{
    auto type_matches = [pinvoke_type](const TypeRef& type_ref) {
        return type_ref.pinvoke_name == pinvoke_type;
    };

    for (const auto& function_decl : module_decl.functions)
    {
        if (type_matches(function_decl.return_type))
        {
            return true;
        }
        for (const auto& parameter : function_decl.parameters)
        {
            if (type_matches(parameter.type))
            {
                return true;
            }
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        for (const auto& method_decl : class_decl.methods)
        {
            if (type_matches(method_decl.return_type))
            {
                return true;
            }
            for (const auto& parameter : method_decl.parameters)
            {
                if (type_matches(parameter.type))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void emit_shared_pinvoke_types_if_needed(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root, std::vector<std::filesystem::path>& generated_files)
{
    if (!module_uses_pinvoke_type(module_decl, "CsBind23StringView"))
    {
        return;
    }

    const auto shared_path = output_root / "csbind23.types.g.cs";
    if (std::filesystem::exists(shared_path))
    {
        return;
    }

    TextWriter shared(256);
    shared.append_line("namespace CsBind23.Generated;");
    shared.append_line();
    shared.append_line("[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]");
    shared.append_line("public readonly struct CsBind23StringView");
    shared.append_line("{");
    shared.append_line("    public readonly System.IntPtr str;");
    shared.append_line("    public readonly nuint length;");
    shared.append_line();
    shared.append_line("    public CsBind23StringView(System.IntPtr str, nuint length)");
    shared.append_line("    {");
    shared.append_line("        this.str = str;");
    shared.append_line("        this.length = length;");
    shared.append_line("    }");
    shared.append_line("}");
    shared.append_line();
    shared.append_line("public static class CsBind23StringViewExtensions");
    shared.append_line("{");
    shared.append_line("    public static string ToManaged(CsBind23StringView view)");
    shared.append_line("    {");
    shared.append_line("        if (view.str == System.IntPtr.Zero || view.length == 0)");
    shared.append_line("        {");
    shared.append_line("            return string.Empty;");
    shared.append_line("        }");
    shared.append_line();
    shared.append_line("        int length = checked((int)view.length);");
    shared.append_line("        byte[] bytes = new byte[length];");
    shared.append_line("        System.Runtime.InteropServices.Marshal.Copy(view.str, bytes, 0, length);");
    shared.append_line("        return System.Text.Encoding.UTF8.GetString(bytes);");
    shared.append_line("    }");
    shared.append_line("}");

    generated_files.push_back(write_csharp_file(output_root, "csbind23.types.g.cs", shared.str()));
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
    if (type_ref.has_managed_converter() && !type_ref.managed_type_name.empty())
    {
        return type_ref.managed_type_name;
    }
    return type_ref.pinvoke_name;
}

bool parameter_is_ref(const ParameterDecl& parameter)
{
    if (parameter.type.is_reference && !parameter.type.is_const)
    {
        return true;
    }

    if (parameter.type.is_pointer && !parameter.type.is_const && parameter.type.pinvoke_name != "System.IntPtr")
    {
        return true;
    }

    if (parameter.type.has_managed_converter() && parameter.type.is_pointer && !parameter.type.is_const
        && parameter.type.cpp_name == "std::string")
    {
        return true;
    }

    return false;
}

bool parameter_is_direct_ref(const ParameterDecl& parameter)
{
    return parameter_is_ref(parameter) && parameter.type.managed_to_pinvoke_expression.empty();
}

std::string parameter_call_argument(const ParameterDecl& parameter)
{
    if (parameter_is_direct_ref(parameter))
    {
        return "ref " + parameter.name;
    }
    return parameter.name;
}

std::string reflection_parameter_type_expression(const ParameterDecl& parameter)
{
    std::string type_expression = std::format("typeof({})", wrapper_type_name(parameter.type));
    if (parameter_is_ref(parameter))
    {
        type_expression += ".MakeByRefType()";
    }
    return type_expression;
}

const ClassDecl* find_class_by_cpp_name(const ModuleDecl& module_decl, std::string_view cpp_name)
{
    for (const auto& class_decl : module_decl.classes)
    {
        if (class_decl.cpp_name == cpp_name)
        {
            return &class_decl;
        }
    }
    return nullptr;
}

const ClassDecl* find_pointer_class_return(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    const auto& return_type = function_decl.return_type;
    if (!return_type.is_pointer && !return_type.is_reference)
    {
        return nullptr;
    }

    if (return_type.has_managed_converter())
    {
        return nullptr;
    }

    return find_class_by_cpp_name(module_decl, return_type.cpp_name);
}

std::string wrapper_return_type(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    if (function_decl.return_type.c_abi_name == "void")
    {
        return "void";
    }

    if (find_pointer_class_return(module_decl, function_decl) != nullptr)
    {
        return "object";
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
    std::string templ, std::string_view managed_name, std::string_view pinvoke_name, std::string_view value_name,
    std::string_view module_name)
{
    templ = replace_all(std::move(templ), "{managed}", managed_name);
    templ = replace_all(std::move(templ), "{pinvoke}", pinvoke_name);
    templ = replace_all(std::move(templ), "{value}", value_name);
    templ = replace_all(std::move(templ), "{module}", module_name);
    return templ;
}

std::vector<std::string> split_trimmed_lines(std::string_view text)
{
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size())
    {
        std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos)
        {
            end = text.size();
        }

        std::string_view line = text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }

        const std::size_t first_non_ws = line.find_first_not_of(" \t");
        if (first_non_ws != std::string_view::npos)
        {
            lines.emplace_back(line.substr(first_non_ws));
        }

        if (end == text.size())
        {
            break;
        }
        start = end + 1;
    }

    return lines;
}

void append_embedded_statement(TextWriter& output, std::string_view indent, std::string_view statement)
{
    const auto lines = split_trimmed_lines(statement);
    for (const auto& line : lines)
    {
        if (!line.empty() && line.back() == ';')
        {
            output.append_line_format("{}{}", indent, line);
        }
        else
        {
            output.append_line_format("{}{};", indent, line);
        }
    }
}

void append_embedded_assignment(
    TextWriter& output, std::string_view indent, std::string_view assignment_prefix, std::string_view expression)
{
    const auto lines = split_trimmed_lines(expression);
    if (lines.empty())
    {
        output.append_line_format("{}{};", indent, assignment_prefix);
        return;
    }

    if (lines.size() == 1)
    {
        output.append_line_format("{}{}{};", indent, assignment_prefix, lines[0]);
        return;
    }

    output.append_line_format("{}{}{}", indent, assignment_prefix, lines[0]);
    for (std::size_t index = 1; index + 1 < lines.size(); ++index)
    {
        output.append_line_format("{}{}", indent, lines[index]);
    }
    output.append_line_format("{}{};", indent, lines.back());
}

void append_embedded_return(TextWriter& output, std::string_view indent, std::string_view expression)
{
    const auto lines = split_trimmed_lines(expression);
    if (lines.empty())
    {
        output.append_line_format("{}return;", indent);
        return;
    }

    if (lines.size() == 1)
    {
        output.append_line_format("{}return {};", indent, lines[0]);
        return;
    }

    output.append_line_format("{}return {}", indent, lines[0]);
    for (std::size_t index = 1; index + 1 < lines.size(); ++index)
    {
        output.append_line_format("{}{}", indent, lines[index]);
    }
    output.append_line_format("{}{};", indent, lines.back());
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

std::string csharp_api_class_name(const ModuleDecl& module_decl)
{
    if (!module_decl.csharp_api_class.empty())
    {
        return module_decl.csharp_api_class;
    }
    return module_decl.name + "Api";
}

std::string csharp_enum_underlying_type(const EnumDecl& enum_decl)
{
    const auto& type_name = enum_decl.underlying_type.pinvoke_name;
    if (type_name == "byte" || type_name == "sbyte" || type_name == "short" || type_name == "ushort"
        || type_name == "int" || type_name == "uint" || type_name == "long" || type_name == "ulong")
    {
        return type_name;
    }

    return "int";
}

std::string csharp_enum_value_literal(const EnumDecl& enum_decl, const EnumValueDecl& value_decl)
{
    const std::string underlying = csharp_enum_underlying_type(enum_decl);
    if (value_decl.is_signed)
    {
        return std::format("unchecked(({}){}L)", underlying, static_cast<std::int64_t>(value_decl.value));
    }

    return std::format("unchecked(({}){}UL)", underlying, value_decl.value);
}

std::string parameter_list_without_self(const FunctionDecl& function_decl)
{
    std::string rendered;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        const bool is_ref = parameter_is_ref(parameter);
        rendered += std::format("{}{} {}", is_ref ? "ref " : "", wrapper_type_name(parameter.type), parameter.name);
        if (index + 1 < function_decl.parameters.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

std::vector<const FunctionDecl*> collect_virtual_methods(const ClassDecl& class_decl)
{
    std::vector<const FunctionDecl*> methods;
    if (!class_decl.enable_virtual_overrides)
    {
        return methods;
    }

    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor || !method_decl.is_method || !method_decl.allow_override)
        {
            continue;
        }
        methods.push_back(&method_decl);
    }

    return methods;
}

std::string callback_delegate_name(const FunctionDecl& method_decl, std::size_t index)
{
    return std::format("__csbind23_CallbackDelegate{}_{}", index, method_decl.virtual_slot_name);
}

std::string callback_method_name(const FunctionDecl& method_decl, std::size_t index)
{
    return std::format("__csbind23_CallbackMethod{}_{}", index, method_decl.virtual_slot_name);
}

std::size_t virtual_mask_chunk(std::size_t virtual_index)
{
    return virtual_index / 64;
}

std::size_t virtual_mask_bit(std::size_t virtual_index)
{
    return virtual_index % 64;
}

std::string virtual_mask_field_name(std::size_t chunk_index)
{
    return std::format("__csbind23_derivedOverrideMask{}", chunk_index);
}

std::string virtual_mask_bit_literal(std::size_t virtual_index)
{
    return std::format("(1UL << {})", virtual_mask_bit(virtual_index));
}

std::string virtual_mask_test_expression(std::string_view instance_prefix, std::size_t virtual_index)
{
    return std::format(
        "(({}{} & {}) != 0)",
        instance_prefix,
        virtual_mask_field_name(virtual_mask_chunk(virtual_index)),
        virtual_mask_bit_literal(virtual_index));
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
        output.append_format("{}{} {}", parameter_is_direct_ref(parameter) ? "ref " : "", parameter.type.pinvoke_name,
            parameter.name);
        needs_separator = true;
    }

    output.append_line(");");
    output.append_line();
}

void append_native_connect_signature(TextWriter& output, std::string_view pinvoke_library,
    const std::string& exported_name, const std::vector<const FunctionDecl*>& virtual_methods)
{
    (void)virtual_methods;
    output.append_line_format(
        "    [System.Runtime.InteropServices.DllImport(\"{}\", CallingConvention = "
        "System.Runtime.InteropServices.CallingConvention.Cdecl)]",
        pinvoke_library);
    output.append_line_format(
        "    internal static extern void {}(System.IntPtr self, System.IntPtr callbacks, nuint callbackCount);",
        exported_name);
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

void append_wrapper_method(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl,
    const FunctionDecl& method_decl, bool is_virtual, bool is_override, std::size_t virtual_index)
{
    const std::string parameter_list = parameter_list_without_self(method_decl);
    const std::string native_name = std::format("{}_{}_{}", module_name, class_decl.name, method_decl.name);
    const std::string base_native_name = native_name + "__base";
    const std::string return_type = wrapper_return_type(module_decl, method_decl);

    if (method_decl.is_property_getter || method_decl.is_property_setter)
    {
        output.append_format("    private {} {}(", return_type, method_decl.name);
    }
    else if (is_override)
    {
        output.append_format("    public override {} {}(", return_type, method_decl.name);
    }
    else if (is_virtual)
    {
        output.append_format("    public virtual {} {}(", return_type, method_decl.name);
    }
    else
    {
        output.append_format("    public {} {}(", return_type, method_decl.name);
    }
    output.append(parameter_list);
    output.append_line(")");
    output.append_line("    {");

    std::vector<std::string> call_arguments;
    std::vector<std::string> finalize_statements;
    call_arguments.reserve(method_decl.parameters.size());
    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        const auto& parameter = method_decl.parameters[index];
        if (!parameter.type.managed_to_pinvoke_expression.empty())
        {
            const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
            const std::string converted = render_inline_template(
                parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                module_name);
            append_embedded_assignment(
                output, "        ", std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name), converted);
            call_arguments.push_back(pinvoke_name);

            if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
            {
                finalize_statements.push_back(render_inline_template(
                    parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                    module_name));
            }
            continue;
        }

        call_arguments.push_back(parameter_call_argument(parameter));
    }

    const std::string call_arguments_rendered = join_arguments(call_arguments);
    const std::string native_call = call_arguments_rendered.empty()
        ? std::format("{}Native.{}(_handle)", module_name, native_name)
        : std::format("{}Native.{}(_handle, {})", module_name, native_name, call_arguments_rendered);
    const std::string base_native_call = call_arguments_rendered.empty()
        ? std::format("{}Native.{}(_handle)", module_name, base_native_name)
        : std::format("{}Native.{}(_handle, {})", module_name, base_native_name, call_arguments_rendered);

    const std::string dispatch_native_call = is_virtual
        ? std::format("({} ? {} : {})", virtual_mask_test_expression("", virtual_index), base_native_call, native_call)
        : native_call;

    const bool has_return_converter = !method_decl.return_type.managed_from_pinvoke_expression.empty();
    const bool has_return_finalize = !method_decl.return_type.managed_finalize_from_pinvoke_statement.empty();
    const bool needs_finally = !finalize_statements.empty() || has_return_finalize;
    const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, method_decl);

    if (return_type == "void")
    {
        if (!needs_finally)
        {
            output.append_line_format("        {};", dispatch_native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {};", dispatch_native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }
        output.append_line("    }");
        output.append_line();
        return;
    }

    if (polymorphic_return_class != nullptr)
    {
        const std::string native_result_name = "__csbind23_result_ptr";
        const std::string wrapped_result = std::format(
            "{}Runtime.WrapPolymorphic_{}({}, false)", module_name, polymorphic_return_class->name, native_result_name);

        if (!needs_finally)
        {
            output.append_line_format("        System.IntPtr {} = {};", native_result_name, dispatch_native_call);
            output.append_line_format("        return {};", wrapped_result);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            System.IntPtr {} = {};", native_result_name, dispatch_native_call);
            output.append_line_format("            return {};", wrapped_result);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }
    }
    else if (!has_return_converter)
    {
        if (!needs_finally)
        {
            output.append_line_format("        return {};", dispatch_native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            return {};", dispatch_native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }
    }
    else
    {
        const std::string native_result_name = "__csbind23_result_pinvoke";
        const std::string managed_result_name = "__csbind23_result_managed";
        const std::string converted_expression =
            render_inline_template(method_decl.return_type.managed_from_pinvoke_expression, managed_result_name, native_result_name,
                native_result_name, module_name);

        if (!needs_finally)
        {
            output.append_line_format("        {} {} = {};", method_decl.return_type.pinvoke_name, native_result_name,
                dispatch_native_call);
            append_embedded_assignment(
                output, "        ", std::format("{} {} = ", return_type, managed_result_name), converted_expression);
            output.append_line_format("        return {};", managed_result_name);
        }
        else
        {
            output.append_line_format("        {} {} = default!;", method_decl.return_type.pinvoke_name, native_result_name);
            output.append_line_format("        {} {} = default!;", return_type, managed_result_name);
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {} = {};", native_result_name, dispatch_native_call);
            append_embedded_assignment(
                output, "            ", std::format("{} = ", managed_result_name), converted_expression);
            output.append_line_format("            return {};", managed_result_name);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            if (has_return_finalize)
            {
                append_embedded_statement(output, "            ", render_inline_template(
                    method_decl.return_type.managed_finalize_from_pinvoke_statement, managed_result_name,
                    native_result_name, native_result_name, module_name));
            }
            output.append_line("        }");
        }
    }

    output.append_line("    }");
    output.append_line();
}

void append_virtual_director_support(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods)
{
    output.append_line("    private static readonly object __csbind23_registryLock = new object();");
    output.append_line_format(
        "    private static readonly System.Collections.Generic.Dictionary<System.IntPtr, "
        "System.WeakReference<{}>> __csbind23_registry = new();",
        class_decl.name);
    output.append_line();

    const std::size_t mask_field_count = (virtual_methods.size() + 63) / 64;
    for (std::size_t chunk = 0; chunk < mask_field_count; ++chunk)
    {
        output.append_line_format("    private ulong {};", virtual_mask_field_name(chunk));
    }
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        if (method_decl.parameters.empty())
        {
            output.append_line_format("    private static readonly System.Type[] __csbind23_methodTypes{} = System.Type.EmptyTypes;", index);
        }
        else
        {
            std::string type_list;
            for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
            {
                const auto& parameter = method_decl.parameters[p];
                if (!type_list.empty())
                {
                    type_list += ", ";
                }
                type_list += reflection_parameter_type_expression(parameter);
            }
            output.append_line_format(
                "    private static readonly System.Type[] __csbind23_methodTypes{} = new System.Type[] {{ {} }};",
                index,
                type_list);
        }
    }
    output.append_line();

    output.append_line("    private static ulong __csbind23_SwigDerivedClassHasMethod(");
    output.append_line("        object instance,");
    output.append_line("        string methodName,");
    output.append_line("        System.Type classType,");
    output.append_line("        System.Type[] methodTypes,");
    output.append_line("        ulong derivedFlag)");
    output.append_line("    {");
    output.append_line("        var methodInfo = instance.GetType().GetMethod(");
    output.append_line("            methodName,");
    output.append_line("            System.Reflection.BindingFlags.Public |");
    output.append_line("            System.Reflection.BindingFlags.NonPublic |");
    output.append_line("            System.Reflection.BindingFlags.Instance,");
    output.append_line("            null,");
    output.append_line("            methodTypes,");
    output.append_line("            null);");
    output.append_line("        if (methodInfo == null)");
    output.append_line("        {");
    output.append_line("            return 0UL;");
    output.append_line("        }");
    output.append_line();
    output.append_line("        bool hasDerivedMethod = methodInfo.IsVirtual");
    output.append_line("            && !methodInfo.IsAbstract");
    output.append_line("            && (methodInfo.DeclaringType!.IsSubclassOf(classType) || methodInfo.DeclaringType == classType)");
    output.append_line("            && methodInfo.DeclaringType != methodInfo.GetBaseDefinition().DeclaringType;");
    output.append_line("        return hasDerivedMethod ? derivedFlag : 0UL;");
    output.append_line("    }");
    output.append_line();

    output.append_line("    private void __csbind23_InitializeDerivedOverrideFlags()");
    output.append_line("    {");
    for (std::size_t chunk = 0; chunk < mask_field_count; ++chunk)
    {
        output.append_line_format("        {} = 0UL;", virtual_mask_field_name(chunk));
    }
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        const std::size_t chunk = virtual_mask_chunk(index);
        output.append_line_format(
            "        {} |= __csbind23_SwigDerivedClassHasMethod(this, \"{}\", typeof({}), __csbind23_methodTypes{}, {});",
            virtual_mask_field_name(chunk),
            method_decl.name,
            class_decl.name,
            index,
            virtual_mask_bit_literal(index));
    }
    output.append_line("    }");
    output.append_line();

    output.append_line_format(
        "    private static void __csbind23_RegisterInstance(System.IntPtr handle, {} instance)",
        class_decl.name);
    output.append_line("    {");
    output.append_line("        if (handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");
    output.append_line("        lock (__csbind23_registryLock)");
    output.append_line("        {");
    output.append_line_format(
        "            __csbind23_registry[handle] = new System.WeakReference<{}>(instance);",
        class_decl.name);
    output.append_line("        }");
    output.append_line("    }");
    output.append_line();

    output.append_line("    private static void __csbind23_UnregisterInstance(System.IntPtr handle)");
    output.append_line("    {");
    output.append_line("        if (handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");
    output.append_line("        lock (__csbind23_registryLock)");
    output.append_line("        {");
    output.append_line("            __csbind23_registry.Remove(handle);");
    output.append_line("        }");
    output.append_line("    }");
    output.append_line();

    output.append_line_format(
        "    private static bool __csbind23_TryGetInstance(System.IntPtr handle, out {} instance)",
        class_decl.name);
    output.append_line("    {");
    output.append_line("        lock (__csbind23_registryLock)");
    output.append_line("        {");
    output.append_line("            if (__csbind23_registry.TryGetValue(handle, out var weak) && weak.TryGetTarget(out instance))");
    output.append_line("            {");
    output.append_line("                return true;");
    output.append_line("            }");
    output.append_line("        }");
    output.append_line("        instance = null!;");
    output.append_line("        return false;");
    output.append_line("    }");
    output.append_line();

    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        output.append_line("    [System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.Cdecl)]");
        output.append_format("    private delegate {} {}(System.IntPtr self", method_decl.return_type.pinvoke_name,
            callback_delegate_name(method_decl, index));
        for (const auto& parameter : method_decl.parameters)
        {
            output.append_format(", {}{} {}", parameter_is_direct_ref(parameter) ? "ref " : "",
                parameter.type.pinvoke_name, parameter.name);
        }
        output.append_line(");");
        output.append_line_format(
            "    private static readonly {} __csbind23_staticCallbackDelegate{} = {};",
            callback_delegate_name(method_decl, index),
            index,
            callback_method_name(method_decl, index));
        output.append_line_format(
            "    private static readonly System.IntPtr __csbind23_staticCallbackPtr{} = System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(__csbind23_staticCallbackDelegate{});",
            index,
            index);
        output.append_line();
    }

    output.append_line("    private void __csbind23_ConnectDirector()");
    output.append_line("    {");
    output.append_line("        if (_handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");

    output.append_line_format("        System.Span<System.IntPtr> __csbind23_callbacks = stackalloc System.IntPtr[{}];", virtual_methods.size());
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        output.append_line_format("        if ({})", virtual_mask_test_expression("", index));
        output.append_line("        {");
        output.append_line_format("            __csbind23_callbacks[{}] = __csbind23_staticCallbackPtr{};", index, index);
        output.append_line("        }");
    }
    output.append_line("        unsafe");
    output.append_line("        {");
    output.append_line("            fixed (System.IntPtr* __csbind23_callbacksPtr = __csbind23_callbacks)");
    output.append_line("            {");
    output.append_line_format(
        "                {}Native.{}_{}_connect_director(_handle, (System.IntPtr)__csbind23_callbacksPtr, (nuint){});",
        module_name,
        module_name,
        class_decl.name,
        virtual_methods.size());
    output.append_line("            }");
    output.append_line("        }");
    output.append_line("    }");
    output.append_line();

    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        const std::string method_name = callback_method_name(method_decl, index);

        output.append_format("    private static {} {}(System.IntPtr self", method_decl.return_type.pinvoke_name, method_name);
        for (const auto& parameter : method_decl.parameters)
        {
            output.append_format(", {}{} {}", parameter_is_direct_ref(parameter) ? "ref " : "",
                parameter.type.pinvoke_name, parameter.name);
        }
        output.append_line(")");
        output.append_line("    {");
        output.append_line_format("        if (!__csbind23_TryGetInstance(self, out var instance))");
        output.append_line("        {");
        if (method_decl.return_type.c_abi_name != "void")
        {
            output.append_line("            return default!;");
        }
        else
        {
            output.append_line("            return;");
        }
        output.append_line("        }");

        std::vector<std::string> base_call_arguments;
        base_call_arguments.reserve(method_decl.parameters.size() + 1);
        base_call_arguments.push_back("self");
        for (const auto& parameter : method_decl.parameters)
        {
            base_call_arguments.push_back(parameter_call_argument(parameter));
        }
        const std::string base_call_arguments_rendered = join_arguments(base_call_arguments);
        const std::string base_native_call = std::format(
            "{}Native.{}_{}__base({})",
            module_name,
            module_name,
            class_decl.name + "_" + method_decl.name,
            base_call_arguments_rendered);

        output.append_line_format("        if (!{})", virtual_mask_test_expression("instance.", index));
        output.append_line("        {");
        if (method_decl.return_type.c_abi_name == "void")
        {
            output.append_line_format("            {} ;", base_native_call);
            output.append_line("            return;");
        }
        else
        {
            output.append_line_format("            return {};", base_native_call);
        }
        output.append_line("        }");

        std::vector<std::string> managed_args;
        managed_args.reserve(method_decl.parameters.size());
        for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
        {
            const auto& parameter = method_decl.parameters[p];
            if (!parameter.type.managed_from_pinvoke_expression.empty())
            {
                const std::string managed_name = std::format("__csbind23_arg{}_managed", p);
                append_embedded_assignment(
                    output,
                    "        ",
                    std::format("{} {} = ", wrapper_type_name(parameter.type), managed_name),
                    render_inline_template(
                        parameter.type.managed_from_pinvoke_expression, managed_name, parameter.name, parameter.name,
                        module_name));
                managed_args.push_back(managed_name);
            }
            else
            {
                if (parameter_is_ref(parameter))
                {
                    managed_args.push_back("ref " + parameter.name);
                }
                else
                {
                    managed_args.push_back(parameter.name);
                }
            }
        }

        const std::string invoke_args = join_arguments(managed_args);

        if (method_decl.return_type.c_abi_name == "void")
        {
            output.append_line_format("        instance.{}({});", method_decl.name, invoke_args);
            output.append_line("        return;");
        }
        else
        {
            output.append_line_format(
                "        {} __csbind23_result = instance.{}({});",
                wrapper_return_type(module_decl, method_decl),
                method_decl.name,
                invoke_args);

            if (!method_decl.return_type.managed_to_pinvoke_expression.empty())
            {
                append_embedded_return(output, "        ", render_inline_template(
                    method_decl.return_type.managed_to_pinvoke_expression, "__csbind23_result", "__csbind23_result",
                    "__csbind23_result", module_name));
            }
            else
            {
                output.append_line("        return __csbind23_result;");
            }
        }

        output.append_line("    }");
        output.append_line();
    }
}

void append_wrapper_class(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl)
{
    const ClassDecl* base_class = class_decl.base_cpp_name.empty() ? nullptr : find_class_by_cpp_name(module_decl, class_decl.base_cpp_name);
    const bool has_base_class = base_class != nullptr;
    const bool emits_destroy = class_has_owned_ctor(class_decl);
    const auto virtual_methods = collect_virtual_methods(class_decl);
    const bool has_virtual_support = !virtual_methods.empty();
    const std::string base_clause = has_base_class ? base_class->name : "System.IDisposable";

    output.append_line_format(
        "public {} class {} : {}",
        has_virtual_support ? "partial" : "sealed",
        class_decl.name,
        base_clause);
    output.append_line("{");
    if (!has_base_class)
    {
        output.append_line("    protected System.IntPtr _handle;");
        output.append_line("    protected bool _ownsHandle;");
    }
    output.append_line();

    output.append_format("    internal {}(System.IntPtr handle, bool ownsHandle)", class_decl.name);
    if (has_base_class)
    {
        output.append(" : base(handle, ownsHandle)");
    }
    output.append_line("");
    output.append_line("    {");
    if (!has_base_class)
    {
        output.append_line("        _handle = handle;");
        output.append_line("        _ownsHandle = ownsHandle;");
        if (has_virtual_support)
        {
            output.append_line("        __csbind23_InitializeDerivedOverrideFlags();");
            output.append_line("        __csbind23_RegisterInstance(_handle, this);");
            output.append_line("        __csbind23_ConnectDirector();");
        }
    }
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
        output.append(")");
        if (has_base_class)
        {
            output.append(" : base(System.IntPtr.Zero, false)");
        }
        output.append_line("");
        output.append_line("    {");

        std::vector<std::string> converted_arguments;
        std::vector<std::string> finalize_statements;
        converted_arguments.reserve(method_decl.parameters.size());
        for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
        {
            const auto& parameter = method_decl.parameters[index];
            if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                append_embedded_assignment(
                    output,
                    "        ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    render_inline_template(
                        parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                        module_name));
                converted_arguments.push_back(pinvoke_name);
                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_name));
                }
                continue;
            }

            converted_arguments.push_back(parameter_call_argument(parameter));
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
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }

        output.append_line_format("        _ownsHandle = {};", owns_handle ? "true" : "false");
        if (has_virtual_support)
        {
            output.append_line("        __csbind23_InitializeDerivedOverrideFlags();");
            output.append_line("        __csbind23_RegisterInstance(_handle, this);");
            output.append_line("        __csbind23_ConnectDirector();");
        }
        output.append_line("    }");
        output.append_line();
    }

    if (!has_base_class)
    {
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

        if (has_virtual_support)
        {
            output.append_line("        var __csbind23_oldHandle = _handle;");
            output.append_line_format("        {}Native.{}_{}_disconnect_director(_handle);", module_name, module_name, class_decl.name);
            output.append_line("        __csbind23_UnregisterInstance(__csbind23_oldHandle);");
            output.append_line();
        }

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
    }

    if (has_virtual_support)
    {
        append_virtual_director_support(output, module_decl, module_name, class_decl, virtual_methods);
    }

    for (std::size_t index = 0; index < class_decl.methods.size(); ++index)
    {
        const auto& method_decl = class_decl.methods[index];
        if (method_decl.is_constructor)
        {
            continue;
        }

        std::size_t virtual_index = 0;
        bool is_virtual = false;
        for (std::size_t i = 0; i < virtual_methods.size(); ++i)
        {
            if (virtual_methods[i] == &method_decl)
            {
                is_virtual = true;
                virtual_index = i;
                break;
            }
        }

        bool is_override = false;
        if (has_base_class)
        {
            for (const auto& base_method : base_class->methods)
            {
                if (base_method.is_constructor || base_method.name != method_decl.name)
                {
                    continue;
                }

                if (base_method.parameters.size() != method_decl.parameters.size())
                {
                    continue;
                }

                bool same_signature = true;
                for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
                {
                    if (base_method.parameters[p].type.pinvoke_name != method_decl.parameters[p].type.pinvoke_name)
                    {
                        same_signature = false;
                        break;
                    }
                }

                if (same_signature)
                {
                    is_override = true;
                    break;
                }
            }
        }

        append_wrapper_method(output, module_decl, module_name, class_decl, method_decl, is_virtual, is_override,
            virtual_index);
    }

    for (const auto& property_decl : class_decl.properties)
    {
        const FunctionDecl* getter_decl = nullptr;
        for (const auto& method_decl : class_decl.methods)
        {
            if (method_decl.name == property_decl.getter_name)
            {
                getter_decl = &method_decl;
                break;
            }
        }

        std::string property_type = getter_decl != nullptr
            ? wrapper_return_type(module_decl, *getter_decl)
            : wrapper_type_name(property_decl.type);

        output.append_line_format("    public {} {}", property_type, property_decl.name);
        output.append_line("    {");
        if (property_decl.has_getter)
        {
            output.append_line_format("        get => {}();", property_decl.getter_name);
        }
        if (property_decl.has_setter)
        {
            output.append_line_format("        set => {}(value);", property_decl.setter_name);
        }
        output.append_line("    }");
        output.append_line();
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
    emit_shared_pinvoke_types_if_needed(module_decl, output_root, generated_files);

    TextWriter generated(3072);

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
        const auto virtual_methods = collect_virtual_methods(class_decl);

        FunctionDecl type_name_decl;
        type_name_decl.return_type.c_abi_name = "int";
        type_name_decl.return_type.pinvoke_name = "int";
        append_native_signature(generated, type_name_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + class_decl.name + "_static_type_id", false);

        FunctionDecl dynamic_type_name_decl;
        dynamic_type_name_decl.return_type.c_abi_name = "int";
        dynamic_type_name_decl.return_type.pinvoke_name = "int";
        append_native_signature(generated, dynamic_type_name_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + class_decl.name + "_dynamic_type_id", true);

        if (class_has_owned_ctor(class_decl))
        {
            FunctionDecl destroy_decl;
            destroy_decl.return_type.c_abi_name = "void";
            destroy_decl.return_type.pinvoke_name = "void";
            append_native_signature(generated, destroy_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_destroy", true);
        }

        if (!virtual_methods.empty())
        {
            append_native_connect_signature(generated, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_connect_director", virtual_methods);

            FunctionDecl disconnect_decl;
            disconnect_decl.return_type.c_abi_name = "void";
            disconnect_decl.return_type.pinvoke_name = "void";
            append_native_signature(generated, disconnect_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_disconnect_director", true);
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

                if (!virtual_methods.empty() && method_decl.allow_override)
                {
                    append_native_signature(generated, method_decl, module_decl.pinvoke_library,
                        module_decl.name + "_" + class_decl.name + "_" + method_decl.name + "__base", true);
                }
            }
        }
    }

    generated.append_line("}");
    generated.append_line();

    for (const auto& enum_decl : module_decl.enums)
    {
        generated.append_line_format("public enum {} : {}", enum_decl.name, csharp_enum_underlying_type(enum_decl));
        generated.append_line("{");
        for (const auto& enum_value : enum_decl.values)
        {
            generated.append_line_format("    {} = {},", enum_value.name, csharp_enum_value_literal(enum_decl, enum_value));
        }
        generated.append_line("}");
        generated.append_line();
    }

    generated.append_line_format("internal static class {}Runtime", module_decl.name);
    generated.append_line("{");
    generated.append_line("    private static readonly System.Func<System.IntPtr, bool, object>[] __csbind23_typeFactories =");
    generated.append_line("        new System.Func<System.IntPtr, bool, object>[]");
    generated.append_line("        {");
    for (const auto& class_decl : module_decl.classes)
    {
        generated.append_line_format("            (handle, ownsHandle) => new {}(handle, ownsHandle),", class_decl.name);
    }
    generated.append_line("        };");
    generated.append_line();

    for (const auto& class_decl : module_decl.classes)
    {
        generated.append_line_format(
            "    internal static object WrapPolymorphic_{}(System.IntPtr handle, bool ownsHandle)", class_decl.name);
        generated.append_line("    {");
        generated.append_line("        if (handle == System.IntPtr.Zero)");
        generated.append_line("        {");
        generated.append_line("            return null!;");
        generated.append_line("        }");
        generated.append_line();
        generated.append_line_format(
            "        int dynamicTypeId = {}Native.{}_{}_dynamic_type_id(handle);",
            module_decl.name,
            module_decl.name,
            class_decl.name);
        generated.append_line("        if (dynamicTypeId >= 0 && dynamicTypeId < __csbind23_typeFactories.Length)");
        generated.append_line("        {");
        generated.append_line("            return __csbind23_typeFactories[dynamicTypeId](handle, ownsHandle);");
        generated.append_line("        }");
        generated.append_line();
        generated.append_line_format("        return new {}(handle, ownsHandle);", class_decl.name);
        generated.append_line("    }");
        generated.append_line();
    }

    generated.append_line("}");
    generated.append_line();

    generated.append_line_format("public static class {}", csharp_api_class_name(module_decl));
    generated.append_line("{");
    for (const auto& function_decl : module_decl.functions)
    {
        const std::string params = parameter_list_without_self(function_decl);
        const std::string return_type = wrapper_return_type(module_decl, function_decl);
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
            if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                append_embedded_assignment(
                    generated,
                    "        ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    render_inline_template(parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name,
                        parameter.name, module_decl.name));
                call_arguments.push_back(pinvoke_name);
                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
                continue;
            }

            call_arguments.push_back(parameter_call_argument(parameter));
        }

        const std::string call_arguments_rendered = join_arguments(call_arguments);
        const std::string native_call = std::format("{}Native.{}({})", module_decl.name, native_name, call_arguments_rendered);
        const bool has_return_converter = !function_decl.return_type.managed_from_pinvoke_expression.empty();
        const bool has_return_finalize = !function_decl.return_type.managed_finalize_from_pinvoke_statement.empty();
        const bool needs_finally = !finalize_statements.empty() || has_return_finalize;
        const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, function_decl);

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
                    append_embedded_statement(generated, "            ", finalize_statement);
                }
                generated.append_line("        }");
            }
        }
        else
        {
            if (polymorphic_return_class != nullptr)
            {
                const std::string native_result_name = "__csbind23_result_ptr";
                const std::string wrapped_result = std::format(
                    "{}Runtime.WrapPolymorphic_{}({}, false)",
                    module_decl.name,
                    polymorphic_return_class->name,
                    native_result_name);

                if (!needs_finally)
                {
                    generated.append_line_format("        System.IntPtr {} = {};", native_result_name, native_call);
                    generated.append_line_format("        return {};", wrapped_result);
                }
                else
                {
                    generated.append_line("        try");
                    generated.append_line("        {");
                    generated.append_line_format("            System.IntPtr {} = {};", native_result_name, native_call);
                    generated.append_line_format("            return {};", wrapped_result);
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(generated, "            ", finalize_statement);
                    }
                    generated.append_line("        }");
                }
            }
            else if (!has_return_converter)
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
                        append_embedded_statement(generated, "            ", finalize_statement);
                    }
                    generated.append_line("        }");
                }
            }
            else
            {
                const std::string native_result_name = "__csbind23_result_pinvoke";
                const std::string managed_result_name = "__csbind23_result_managed";
                const std::string converted_expression =
                    render_inline_template(function_decl.return_type.managed_from_pinvoke_expression, managed_result_name, native_result_name,
                        native_result_name, module_decl.name);

                if (!needs_finally)
                {
                    generated.append_line_format(
                        "        {} {} = {};", function_decl.return_type.pinvoke_name, native_result_name, native_call);
                    append_embedded_assignment(
                        generated,
                        "        ",
                        std::format("{} {} = ", return_type, managed_result_name),
                        converted_expression);
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
                    append_embedded_assignment(
                        generated,
                        "            ",
                        std::format("{} = ", managed_result_name),
                        converted_expression);
                    generated.append_line_format("            return {};", managed_result_name);
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(generated, "            ", finalize_statement);
                    }
                    if (has_return_finalize)
                    {
                        append_embedded_statement(generated, "            ", render_inline_template(
                            function_decl.return_type.managed_finalize_from_pinvoke_statement, managed_result_name,
                            native_result_name, native_result_name, module_decl.name));
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
        TextWriter class_file(2048);
        class_file.append_line("namespace CsBind23.Generated;");
        class_file.append_line();
        append_wrapper_class(class_file, module_decl, module_decl.name, class_decl);

        const auto& class_written = class_file.str();
        generated_files.push_back(
            write_csharp_file(output_root, module_decl.name + "." + class_decl.name + ".g.cs", class_written));
    }

    return generated_files;
}

} // namespace csbind23::emit
