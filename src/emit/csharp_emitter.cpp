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

    if (return_type.managed_converter.has_value())
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

void append_native_connect_signature(TextWriter& output, std::string_view pinvoke_library,
    const std::string& exported_name, const std::vector<const FunctionDecl*>& virtual_methods)
{
    output.append_line_format(
        "    [System.Runtime.InteropServices.DllImport(\"{}\", CallingConvention = "
        "System.Runtime.InteropServices.CallingConvention.Cdecl)]",
        pinvoke_library);
    output.append_format("    internal static extern void {}(System.IntPtr self", exported_name);
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        output.append_format(", System.IntPtr callback{}", index);
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

void append_wrapper_method(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl,
    const FunctionDecl& method_decl, bool is_virtual, std::size_t virtual_index)
{
    const std::string parameter_list = parameter_list_without_self(method_decl);
    const std::string native_name = std::format("{}_{}_{}", module_name, class_decl.name, method_decl.name);
    const std::string base_native_name = native_name + "__base";
    const std::string return_type = wrapper_return_type(module_decl, method_decl);

    if (is_virtual)
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
    const std::string base_native_call = call_arguments_rendered.empty()
        ? std::format("{}Native.{}(_handle)", module_name, base_native_name)
        : std::format("{}Native.{}(_handle, {})", module_name, base_native_name, call_arguments_rendered);

    const std::string dispatch_native_call = is_virtual
        ? std::format("(__csbind23_dispatchDepth{} > 0 ? {} : {})", virtual_index, base_native_call, native_call)
        : native_call;

    const bool has_return_converter = method_decl.return_type.managed_converter.has_value()
        && !method_decl.return_type.managed_converter->from_pinvoke_expression.empty();
    const bool has_return_finalize = method_decl.return_type.managed_converter.has_value()
        && !method_decl.return_type.managed_converter->finalize_from_pinvoke_statement.empty();
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
                output.append_line_format("            {};", finalize_statement);
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
                output.append_line_format("            {};", finalize_statement);
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
                dispatch_native_call);
            output.append_line_format("        {} {} = {};", return_type, managed_result_name, converted_expression);
            output.append_line_format("        return {};", managed_result_name);
        }
        else
        {
            output.append_line_format("        {} {} = default!;", method_decl.return_type.pinvoke_name, native_result_name);
            output.append_line_format("        {} {} = default!;", return_type, managed_result_name);
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {} = {};", native_result_name, dispatch_native_call);
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

void append_virtual_director_support(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods)
{
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        output.append_line("    [System.ThreadStatic]");
        output.append_line_format("    private static int __csbind23_dispatchDepth{};", index);
    }
    output.append_line();

    output.append_line("    private static readonly object __csbind23_registryLock = new object();");
    output.append_line_format(
        "    private static readonly System.Collections.Generic.Dictionary<System.IntPtr, "
        "System.WeakReference<{}>> __csbind23_registry = new();",
        class_decl.name);
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
            output.append_format(", {} {}", parameter.type.pinvoke_name, parameter.name);
        }
        output.append_line(");");
        output.append_line_format(
            "    private {} __csbind23_callbackDelegate{};", callback_delegate_name(method_decl, index), index);
        output.append_line();
    }

    output.append_line("    private void __csbind23_ConnectDirector()");
    output.append_line("    {");
    output.append_line("        if (_handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");

    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        output.append_line_format(
            "        __csbind23_callbackDelegate{} = new {}({});",
            index,
            callback_delegate_name(method_decl, index),
            callback_method_name(method_decl, index));
    }

    output.append_format("        {}Native.{}_{}_connect_director(_handle", module_name, module_name, class_decl.name);
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        output.append_format(", System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(__csbind23_callbackDelegate{})", index);
    }
    output.append_line(");");
    output.append_line("    }");
    output.append_line();

    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        const std::string method_name = callback_method_name(method_decl, index);

        output.append_format("    private static {} {}(System.IntPtr self", method_decl.return_type.pinvoke_name, method_name);
        for (const auto& parameter : method_decl.parameters)
        {
            output.append_format(", {} {}", parameter.type.pinvoke_name, parameter.name);
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

        std::vector<std::string> managed_args;
        managed_args.reserve(method_decl.parameters.size());
        for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
        {
            const auto& parameter = method_decl.parameters[p];
            const auto& converter = parameter.type.managed_converter;
            if (converter.has_value() && !converter->from_pinvoke_expression.empty())
            {
                const std::string managed_name = std::format("__csbind23_arg{}_managed", p);
                output.append_line_format(
                    "        {} {} = {};",
                    wrapper_type_name(parameter.type),
                    managed_name,
                    render_inline_template(
                        converter->from_pinvoke_expression, managed_name, parameter.name, parameter.name));
                managed_args.push_back(managed_name);
            }
            else
            {
                managed_args.push_back(parameter.name);
            }
        }

        const std::string invoke_args = join_arguments(managed_args);

        output.append_line("        try");
        output.append_line("        {");
        output.append_line_format("            __csbind23_dispatchDepth{}++;", index);

        if (method_decl.return_type.c_abi_name == "void")
        {
            output.append_line_format("            instance.{}({});", method_decl.name, invoke_args);
        }
        else
        {
            output.append_line_format(
                "            {} __csbind23_result = instance.{}({});",
                wrapper_return_type(module_decl, method_decl),
                method_decl.name,
                invoke_args);

            const auto& ret_converter = method_decl.return_type.managed_converter;
            if (ret_converter.has_value() && !ret_converter->to_pinvoke_expression.empty())
            {
                output.append_line_format(
                    "            return {};",
                    render_inline_template(ret_converter->to_pinvoke_expression, "__csbind23_result",
                        "__csbind23_result", "__csbind23_result"));
            }
            else
            {
                output.append_line("            return __csbind23_result;");
            }
        }

        output.append_line("        }");
        output.append_line("        finally");
        output.append_line("        {");
        output.append_line_format("            __csbind23_dispatchDepth{}--;", index);
        output.append_line("        }");

        if (method_decl.return_type.c_abi_name != "void")
        {
            output.append_line("        return default!;");
        }

        output.append_line("    }");
        output.append_line();
    }
}

void append_wrapper_class(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl)
{
    const bool emits_destroy = class_has_owned_ctor(class_decl);
    const auto virtual_methods = collect_virtual_methods(class_decl);
    const bool has_virtual_support = !virtual_methods.empty();

    output.append_line_format(
        "public {} class {} : System.IDisposable",
        has_virtual_support ? "partial" : "sealed",
        class_decl.name);
    output.append_line("{");
    output.append_line("    private System.IntPtr _handle;");
    output.append_line("    private bool _ownsHandle;");
    output.append_line();

    output.append_line_format("    internal {}(System.IntPtr handle, bool ownsHandle)", class_decl.name);
    output.append_line("    {");
    output.append_line("        _handle = handle;");
    output.append_line("        _ownsHandle = ownsHandle;");
    if (has_virtual_support)
    {
        output.append_line("        __csbind23_RegisterInstance(_handle, this);");
        output.append_line("        __csbind23_ConnectDirector();");
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
        if (has_virtual_support)
        {
            output.append_line("        __csbind23_RegisterInstance(_handle, this);");
            output.append_line("        __csbind23_ConnectDirector();");
        }
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

        append_wrapper_method(output, module_decl, module_name, class_decl, method_decl, is_virtual, virtual_index);
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
        type_name_decl.return_type.c_abi_name = "const char*";
        type_name_decl.return_type.pinvoke_name = "System.IntPtr";
        append_native_signature(generated, type_name_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + class_decl.name + "_static_type_name", false);

        FunctionDecl dynamic_type_name_decl;
        dynamic_type_name_decl.return_type.c_abi_name = "const char*";
        dynamic_type_name_decl.return_type.pinvoke_name = "System.IntPtr";
        append_native_signature(generated, dynamic_type_name_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + class_decl.name + "_dynamic_type_name", true);

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

    generated.append_line_format("internal static class {}Runtime", module_decl.name);
    generated.append_line("{");
    generated.append_line("    private static readonly System.Collections.Generic.Dictionary<string, System.Func<System.IntPtr, bool, object>> __csbind23_typeRegistry =");
    generated.append_line("        new System.Collections.Generic.Dictionary<string, System.Func<System.IntPtr, bool, object>>()");
    generated.append_line("        {");
    for (const auto& class_decl : module_decl.classes)
    {
        generated.append_line_format(
            "            [{}_NativeTypeName_{}()] = (handle, ownsHandle) => new {}(handle, ownsHandle),",
            module_decl.name,
            class_decl.name,
            class_decl.name);
    }
    generated.append_line("        };");
    generated.append_line();

    for (const auto& class_decl : module_decl.classes)
    {
        generated.append_line_format(
            "    private static string {}_NativeTypeName_{}() => "
            "System.Runtime.InteropServices.Marshal.PtrToStringAnsi({}Native.{}_{}_static_type_name()) ?? string.Empty;",
            module_decl.name,
            class_decl.name,
            module_decl.name,
            module_decl.name,
            class_decl.name);
    }
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
            "        var dynamicType = System.Runtime.InteropServices.Marshal.PtrToStringAnsi({}Native.{}_{}_dynamic_type_name(handle));",
            module_decl.name,
            module_decl.name,
            class_decl.name);
        generated.append_line("        if (!string.IsNullOrEmpty(dynamicType) && __csbind23_typeRegistry.TryGetValue(dynamicType, out var factory))");
        generated.append_line("        {");
        generated.append_line("            return factory(handle, ownsHandle);");
        generated.append_line("        }");
        generated.append_line();
        generated.append_line_format("        return new {}(handle, ownsHandle);", class_decl.name);
        generated.append_line("    }");
        generated.append_line();
    }

    generated.append_line("}");
    generated.append_line();

    generated.append_line_format("public static class {}Api", module_decl.name);
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
                    generated.append_line_format("            {};", finalize_statement);
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
                        generated.append_line_format("            {};", finalize_statement);
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
