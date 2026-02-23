#include "csbind23/emit/cabi_emitter.hpp"

#include "csbind23/text_writer.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace csbind23::emit
{
namespace
{

std::string render_cpp_type(const TypeRef& type_ref)
{
    std::string rendered;
    if (type_ref.is_const)
    {
        rendered += "const ";
    }
    rendered += type_ref.cpp_name;
    if (type_ref.is_pointer)
    {
        rendered += "*";
    }
    if (type_ref.is_reference)
    {
        rendered += "&";
    }
    return rendered;
}

std::string render_argument_expr(const ParameterDecl& parameter)
{
    return std::format(
        "csbind23::cabi::Converter<{}>::from_c_abi({})", render_cpp_type(parameter.type), parameter.name);
}

std::string render_call_arguments(const std::vector<ParameterDecl>& parameters)
{
    std::string arguments;
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        arguments += render_argument_expr(parameters[index]);
        if (index + 1 < parameters.size())
        {
            arguments += ", ";
        }
    }
    return arguments;
}

void append_function_signature(TextWriter& output, const FunctionDecl& function_decl, const std::string& exported_name)
{
    output.append_format("extern \"C\" {} {}(", function_decl.return_type.c_abi_name, exported_name);

    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        output.append_format("{} {}", parameter.type.c_abi_name, parameter.name);
        if (index + 1 < function_decl.parameters.size())
        {
            output.append(", ");
        }
    }

    output.append(")");
}

void append_method_signature(TextWriter& output, const FunctionDecl& function_decl, const std::string& exported_name)
{
    const char* self_type = function_decl.is_const ? "const void*" : "void*";
    output.append_format("extern \"C\" {} {}({} self", function_decl.return_type.c_abi_name, exported_name, self_type);

    for (const auto& parameter : function_decl.parameters)
    {
        output.append_format(", {} {}", parameter.type.c_abi_name, parameter.name);
    }

    output.append(")");
}

void append_free_function_body(TextWriter& output, const FunctionDecl& function_decl)
{
    const std::string call_arguments = render_call_arguments(function_decl.parameters);
    if (function_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("    {}({});", function_decl.cpp_symbol, call_arguments);
        return;
    }

    output.append_line_format("    auto result = {}({});", function_decl.cpp_symbol, call_arguments);
    output.append_line_format(
        "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(function_decl.return_type));
}

void append_constructor_body(TextWriter& output, const ClassDecl& class_decl, const FunctionDecl& ctor_decl)
{
    const std::string call_arguments = render_call_arguments(ctor_decl.parameters);
    const Ownership ownership = infer_ownership(ctor_decl);

    if (ownership == Ownership::Borrowed)
    {
        output.append_line(
            "    // Borrowed constructor ownership requested; instance is still heap-allocated for now.");
    }

    output.append_line_format("    auto* instance = new {}({});", class_decl.cpp_name, call_arguments);
    output.append_line_format("    return csbind23::cabi::Converter<{}*>::to_c_abi(instance);", class_decl.cpp_name);
}

void append_method_body(TextWriter& output, const ClassDecl& class_decl, const FunctionDecl& method_decl)
{
    if (method_decl.is_const)
    {
        output.append_line_format(
            "    auto* instance = csbind23::cabi::Converter<const {}*>::from_c_abi(self);", class_decl.cpp_name);
    }
    else
    {
        output.append_line_format(
            "    auto* instance = csbind23::cabi::Converter<{}*>::from_c_abi(self);", class_decl.cpp_name);
    }

    const std::string call_arguments = render_call_arguments(method_decl.parameters);
    if (method_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("    instance->{}({});", method_decl.cpp_symbol, call_arguments);
        return;
    }

    output.append_line_format("    auto result = instance->{}({});", method_decl.cpp_symbol, call_arguments);
    output.append_line_format(
        "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(method_decl.return_type));
}

} // namespace

std::vector<std::filesystem::path> emit_cabi_module(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root)
{
    std::filesystem::create_directories(output_root);

    const auto source_path = output_root / (module_decl.name + "_cabi.cpp");
    TextWriter generated(1024);

    generated.append_line("#include <csbind23/cabi/converter.hpp>");
    for (const auto& include_directive : module_decl.cabi_includes)
    {
        generated.append_line_format("#include {}", include_directive);
    }
    generated.append_line();
    generated.append_line_format("namespace generated::{} {{", module_decl.name);
    generated.append_line();

    for (const auto& function_decl : module_decl.functions)
    {
        append_function_signature(generated, function_decl, module_decl.name + "_" + function_decl.name);
        generated.append_line(" {");
        append_free_function_body(generated, function_decl);
        generated.append_line("}");
        generated.append_line();
    }

    for (const auto& class_decl : module_decl.classes)
    {
        bool emits_owned_instance = false;
        for (const auto& method_decl : class_decl.methods)
        {
            if (method_decl.is_constructor && infer_ownership(method_decl) == Ownership::Owned)
            {
                emits_owned_instance = true;
                break;
            }
        }

        if (emits_owned_instance)
        {
            generated.append_line_format(
                "extern \"C\" void {}_{}_destroy(void* self) {{", module_decl.name, class_decl.name);
            generated.append_line_format(
                "    delete csbind23::cabi::Converter<{}*>::from_c_abi(self);", class_decl.cpp_name);
            generated.append_line("}");
            generated.append_line();
        }

        for (const auto& method_decl : class_decl.methods)
        {
            const std::string exported_name = method_decl.is_constructor
                ? module_decl.name + "_" + class_decl.name + "_create"
                : module_decl.name + "_" + class_decl.name + "_" + method_decl.name;

            if (method_decl.is_constructor)
            {
                append_function_signature(generated, method_decl, exported_name);
            }
            else
            {
                append_method_signature(generated, method_decl, exported_name);
            }

            generated.append_line(" {");
            if (method_decl.is_constructor)
            {
                append_constructor_body(generated, class_decl, method_decl);
            }
            else
            {
                append_method_body(generated, class_decl, method_decl);
            }
            generated.append_line("}");
            generated.append_line();
        }
    }

    generated.append_line_format("}} // namespace generated::{}", module_decl.name);

    std::ofstream output(source_path, std::ios::binary);
    const auto& written = generated.str();
    output.write(written.data(), static_cast<std::streamsize>(written.size()));

    return {source_path};
}

} // namespace csbind23::emit
