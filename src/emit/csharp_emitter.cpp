#include "csbind23/emit/csharp_emitter.hpp"

#include "csbind23/text_writer.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>

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

std::string wrapper_return_type(const FunctionDecl& function_decl)
{
    if (function_decl.return_type.c_abi_name == "void")
    {
        return "void";
    }
    return function_decl.return_type.pinvoke_name;
}

std::string parameter_list_without_self(const FunctionDecl& function_decl)
{
    std::string rendered;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        rendered += std::format("{} {}", parameter.type.pinvoke_name, parameter.name);
        if (index + 1 < function_decl.parameters.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

std::string argument_list_without_self(const FunctionDecl& function_decl, std::string_view prefix = {})
{
    std::string rendered;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        rendered += prefix;
        rendered += function_decl.parameters[index].name;
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
    const std::string call_arguments = argument_list_without_self(method_decl);
    const std::string native_name = std::format("{}_{}_{}", module_name, class_decl.name, method_decl.name);
    const std::string return_type = wrapper_return_type(method_decl);

    output.append_format("    public {} {}(", return_type, method_decl.name);
    output.append(parameter_list);
    output.append_line(")");
    output.append_line("    {");

    if (return_type == "void")
    {
        if (call_arguments.empty())
        {
            output.append_line_format("        {}Native.{}(_handle);", module_name, native_name);
        }
        else
        {
            output.append_line_format("        {}Native.{}(_handle, {});", module_name, native_name, call_arguments);
        }
        output.append_line("    }");
        output.append_line();
        return;
    }

    if (call_arguments.empty())
    {
        output.append_line_format("        return {}Native.{}(_handle);", module_name, native_name);
    }
    else
    {
        output.append_line_format("        return {}Native.{}(_handle, {});", module_name, native_name, call_arguments);
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
        const std::string call_arguments = argument_list_without_self(method_decl);
        const std::string create_name = std::format("{}_{}_create", module_name, class_decl.name);
        const bool owns_handle = infer_ownership(method_decl) == Ownership::Owned;

        output.append_format("    public {}(", class_decl.name);
        output.append(parameter_list);
        output.append_line(")");
        output.append_line("    {");
        if (call_arguments.empty())
        {
            output.append_line_format("        _handle = {}Native.{}();", module_name, create_name);
        }
        else
        {
            output.append_line_format("        _handle = {}Native.{}({});", module_name, create_name, call_arguments);
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
        const std::string args = argument_list_without_self(function_decl);
        const std::string return_type = wrapper_return_type(function_decl);
        const std::string native_name = module_decl.name + "_" + function_decl.name;

        generated.append_format("    public static {} {}(", return_type, function_decl.name);
        generated.append(params);
        generated.append_line(")");
        generated.append_line("    {");
        if (return_type == "void")
        {
            generated.append_line_format("        {}Native.{}({});", module_decl.name, native_name, args);
        }
        else
        {
            generated.append_line_format("        return {}Native.{}({});", module_decl.name, native_name, args);
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
