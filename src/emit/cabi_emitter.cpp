#include "csbind23/emit/cabi_emitter.hpp"

#include "csbind23/text_writer.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>

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

std::string render_converted_argument_name(std::size_t index)
{
    return std::format("__csbind23_arg{}_cpp", index);
}

std::string render_callback_argument_name(std::size_t index)
{
    return std::format("__csbind23_arg{}_c", index);
}

void append_converted_arguments(TextWriter& output, const std::vector<ParameterDecl>& parameters)
{
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const auto& parameter = parameters[index];
        const auto converted_name = render_converted_argument_name(index);
        const auto rendered_cpp_type = render_cpp_type(parameter.type);

        output.append_line_format(
            "    decltype(auto) {} = csbind23::cabi::Converter<{}>::from_c_abi({});",
            converted_name,
            rendered_cpp_type,
            parameter.name);
    }
}

std::string render_call_arguments(const std::vector<ParameterDecl>& parameters)
{
    std::string arguments;
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        arguments += render_converted_argument_name(index);
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
    append_converted_arguments(output, function_decl.parameters);
    const std::string call_arguments = render_call_arguments(function_decl.parameters);
    if (function_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("    {}({});", function_decl.cpp_symbol, call_arguments);
        return;
    }

    output.append_line_format("    decltype(auto) result = {}({});", function_decl.cpp_symbol, call_arguments);
    output.append_line_format(
        "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(function_decl.return_type));
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

std::string callback_typedef_name(
    const std::string& module_name, const std::string& class_name, const FunctionDecl& method_decl)
{
    return std::format("{}_{}_Callback_{}", module_name, class_name, method_decl.virtual_slot_name);
}

void append_virtual_callback_typedef(
    TextWriter& output, const std::string& module_name, const ClassDecl& class_decl, const FunctionDecl& method_decl)
{
    const std::string typedef_name = callback_typedef_name(module_name, class_decl.name, method_decl);
    const char* self_type = method_decl.is_const ? "const void*" : "void*";

    output.append_format("using {} = {} (*)({} self", typedef_name, method_decl.return_type.c_abi_name, self_type);
    for (const auto& parameter : method_decl.parameters)
    {
        output.append_format(", {} {}", parameter.type.c_abi_name, parameter.name);
    }
    output.append_line(");");
}

void append_virtual_callback_table(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods)
{
    output.append_line_format("struct {}_{}_CallbackTable", module_name, class_decl.name);
    output.append_line("{");
    for (const auto* method_decl : virtual_methods)
    {
        const std::string typedef_name = callback_typedef_name(module_name, class_decl.name, *method_decl);
        output.append_line_format("    {} {} = nullptr;", typedef_name, method_decl->virtual_slot_name);
    }
    output.append_line("};");
    output.append_line();
}

void append_director_method_override(TextWriter& output, const ClassDecl& class_decl, const std::string& module_name,
    const FunctionDecl& method_decl)
{
    output.append_format("    {} {}(", render_cpp_type(method_decl.return_type), method_decl.cpp_symbol);

    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        const auto& parameter = method_decl.parameters[index];
        output.append_format("{} {}", render_cpp_type(parameter.type), parameter.name);
        if (index + 1 < method_decl.parameters.size())
        {
            output.append(", ");
        }
    }

    output.append(method_decl.is_const ? ") const override\n" : ") override\n");
    output.append_line("    {");

    output.append_line_format("        if (callbacks_.{} != nullptr)", method_decl.virtual_slot_name);
    output.append_line("        {");

    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        const auto& parameter = method_decl.parameters[index];
        output.append_line_format(
            "            {} {} = csbind23::cabi::Converter<{}>::to_c_abi({});",
            parameter.type.c_abi_name,
            render_callback_argument_name(index),
            render_cpp_type(parameter.type),
            parameter.name);
    }

    std::string callback_args = method_decl.is_const ? "static_cast<const void*>(this)" : "static_cast<void*>(this)";
    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        callback_args += ", ";
        callback_args += render_callback_argument_name(index);
    }

    if (method_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("            callbacks_.{}({});", method_decl.virtual_slot_name, callback_args);
        output.append_line("            return;");
    }
    else
    {
        output.append_line_format(
            "            auto __csbind23_callback_result = callbacks_.{}({});",
            method_decl.virtual_slot_name,
            callback_args);
        output.append_line_format(
            "            return csbind23::cabi::Converter<{}>::from_c_abi(__csbind23_callback_result);",
            render_cpp_type(method_decl.return_type));
    }

    output.append_line("        }");

    std::string base_call_args;
    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        base_call_args += method_decl.parameters[index].name;
        if (index + 1 < method_decl.parameters.size())
        {
            base_call_args += ", ";
        }
    }

    if (method_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("        {}::{}({});", class_decl.cpp_name, method_decl.cpp_symbol, base_call_args);
        output.append_line("    }");
        output.append_line();
        return;
    }

    output.append_line_format("        return {}::{}({});", class_decl.cpp_name, method_decl.cpp_symbol, base_call_args);
    output.append_line("    }");
    output.append_line();
}

void append_director_class(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods)
{
    output.append_line_format("class {}_{}_Director : public {}", module_name, class_decl.name, class_decl.cpp_name);
    output.append_line("{");
    output.append_line("public:");

    for (const auto& method_decl : class_decl.methods)
    {
        if (!method_decl.is_constructor)
        {
            continue;
        }

        output.append_format("    explicit {}_{}_Director(", module_name, class_decl.name);
        for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
        {
            const auto& parameter = method_decl.parameters[index];
            output.append_format("{} {}", render_cpp_type(parameter.type), parameter.name);
            if (index + 1 < method_decl.parameters.size())
            {
                output.append(", ");
            }
        }
        output.append_format(")\n        : {}(", class_decl.cpp_name);
        for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
        {
            output.append_format("{}", method_decl.parameters[index].name);
            if (index + 1 < method_decl.parameters.size())
            {
                output.append(", ");
            }
        }
        output.append_line(") {}\n");
    }

    output.append_line_format("    void set_callbacks(const {}_{}_CallbackTable& callbacks)", module_name, class_decl.name);
    output.append_line("    {");
    output.append_line("        callbacks_ = callbacks;");
    output.append_line("    }");
    output.append_line();

    for (const auto* method_decl : virtual_methods)
    {
        append_director_method_override(output, class_decl, module_name, *method_decl);
    }

    output.append_line("private:");
    output.append_line_format("    {}_{}_CallbackTable callbacks_{{}};", module_name, class_decl.name);
    output.append_line("};");
    output.append_line();
}

void append_constructor_body(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl,
    const FunctionDecl& ctor_decl, bool use_director)
{
    append_converted_arguments(output, ctor_decl.parameters);
    const std::string call_arguments = render_call_arguments(ctor_decl.parameters);
    const Ownership ownership = infer_ownership(ctor_decl);

    if (ownership == Ownership::Borrowed)
    {
        output.append_line("    // Borrowed constructor ownership requested; instance is still heap-allocated for now.");
    }

    if (use_director)
    {
        output.append_line_format("    auto* instance = new {}_{}_Director({});", module_name, class_decl.name, call_arguments);
    }
    else
    {
        output.append_line_format("    auto* instance = new {}({});", class_decl.cpp_name, call_arguments);
    }
    output.append_line_format("    return csbind23::cabi::Converter<{}*>::to_c_abi(instance);", class_decl.cpp_name);
}

void append_method_body(
    TextWriter& output, const ClassDecl& class_decl, const FunctionDecl& method_decl, bool explicit_base_call)
{
    if (method_decl.is_const)
    {
        output.append_line_format(
            "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<const {}*>::from_c_abi(self);",
            class_decl.cpp_name);
        output.append_line("    auto* instance = __csbind23_self_cpp;");
    }
    else
    {
        output.append_line_format(
            "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<{}*>::from_c_abi(self);",
            class_decl.cpp_name);
        output.append_line("    auto* instance = __csbind23_self_cpp;");
    }

    append_converted_arguments(output, method_decl.parameters);

    const std::string call_arguments = render_call_arguments(method_decl.parameters);
    const std::string method_expr = explicit_base_call
        ? std::format("{}::{}", class_decl.cpp_name, method_decl.cpp_symbol)
        : method_decl.cpp_symbol;

    if (method_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("    instance->{}({});", method_expr, call_arguments);
        return;
    }

    output.append_line_format("    decltype(auto) result = instance->{}({});", method_expr, call_arguments);
    output.append_line_format(
        "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(method_decl.return_type));
}

void append_connect_signature(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods, const std::string& exported_name)
{
    output.append_format("extern \"C\" void {}(void* self", exported_name);
    for (const auto* method_decl : virtual_methods)
    {
        output.append_format(", {} {}", callback_typedef_name(module_name, class_decl.name, *method_decl),
            method_decl->virtual_slot_name);
    }
    output.append(")");
}

void append_connect_body(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods)
{
    output.append_line_format(
        "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<{}*>::from_c_abi(self);",
        class_decl.cpp_name);
    output.append_line("    auto* instance = __csbind23_self_cpp;");
    output.append_line_format("    auto* director = dynamic_cast<{}_{}_Director*>(instance);", module_name, class_decl.name);
    output.append_line("    if (director == nullptr)");
    output.append_line("    {");
    output.append_line("        return;");
    output.append_line("    }");
    output.append_line();
    output.append_line_format("    {}_{}_CallbackTable callbacks{{}};", module_name, class_decl.name);
    for (const auto* method_decl : virtual_methods)
    {
        output.append_line_format("    callbacks.{} = {};", method_decl->virtual_slot_name, method_decl->virtual_slot_name);
    }
    output.append_line("    director->set_callbacks(callbacks);");
}

void append_disconnect_body(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl)
{
    output.append_line_format(
        "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<{}*>::from_c_abi(self);",
        class_decl.cpp_name);
    output.append_line("    auto* instance = __csbind23_self_cpp;");
    output.append_line_format("    auto* director = dynamic_cast<{}_{}_Director*>(instance);", module_name, class_decl.name);
    output.append_line("    if (director == nullptr)");
    output.append_line("    {");
    output.append_line("        return;");
    output.append_line("    }");
    output.append_line_format("    director->set_callbacks({}_{}_CallbackTable{{}});", module_name, class_decl.name);
}

} // namespace

std::vector<std::filesystem::path> emit_cabi_module(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root)
{
    std::filesystem::create_directories(output_root);

    const auto source_path = output_root / (module_decl.name + "_cabi.cpp");
    TextWriter generated(4096);

    generated.append_line("#include <csbind23/cabi/converter.hpp>");
    generated.append_line("#include <typeinfo>");
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
        const auto virtual_methods = collect_virtual_methods(class_decl);
        const bool has_virtual_director = !virtual_methods.empty();

        generated.append_line_format(
            "extern \"C\" const char* {}_{}_static_type_name() {{", module_decl.name, class_decl.name);
        generated.append_line_format("    return typeid({}).name();", class_decl.cpp_name);
        generated.append_line("}");
        generated.append_line();

        generated.append_line_format(
            "extern \"C\" const char* {}_{}_dynamic_type_name(const void* self) {{",
            module_decl.name,
            class_decl.name);
        generated.append_line_format(
            "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<const {}*>::from_c_abi(self);",
            class_decl.cpp_name);
        generated.append_line("    if (__csbind23_self_cpp == nullptr)");
        generated.append_line("    {");
        generated.append_line("        return nullptr;");
        generated.append_line("    }");
        generated.append_line("    return typeid(*__csbind23_self_cpp).name();");
        generated.append_line("}");
        generated.append_line();

        if (has_virtual_director)
        {
            for (const auto* method_decl : virtual_methods)
            {
                append_virtual_callback_typedef(generated, module_decl.name, class_decl, *method_decl);
            }
            generated.append_line();
            append_virtual_callback_table(generated, module_decl.name, class_decl, virtual_methods);
            append_director_class(generated, module_decl.name, class_decl, virtual_methods);

            append_connect_signature(
                generated,
                module_decl.name,
                class_decl,
                virtual_methods,
                module_decl.name + "_" + class_decl.name + "_connect_director");
            generated.append_line(" {");
            append_connect_body(generated, module_decl.name, class_decl, virtual_methods);
            generated.append_line("}");
            generated.append_line();

            generated.append_line_format(
                "extern \"C\" void {}_{}_disconnect_director(void* self) {{",
                module_decl.name,
                class_decl.name);
            append_disconnect_body(generated, module_decl.name, class_decl);
            generated.append_line("}");
            generated.append_line();
        }

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
                "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<{}*>::from_c_abi(self);",
                class_decl.cpp_name);
            generated.append_line_format("    delete __csbind23_self_cpp;");
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
                append_constructor_body(generated, module_decl.name, class_decl, method_decl, has_virtual_director);
            }
            else
            {
                append_method_body(generated, class_decl, method_decl, false);
            }
            generated.append_line("}");
            generated.append_line();

            if (has_virtual_director && method_decl.allow_override && !method_decl.is_constructor)
            {
                append_method_signature(generated, method_decl, exported_name + "__base");
                generated.append_line(" {");
                append_method_body(generated, class_decl, method_decl, true);
                generated.append_line("}");
                generated.append_line();
            }
        }
    }

    generated.append_line_format("}} // namespace generated::{}", module_decl.name);

    std::ofstream output(source_path, std::ios::binary);
    const auto& written = generated.str();
    output.write(written.data(), static_cast<std::streamsize>(written.size()));

    return {source_path};
}

} // namespace csbind23::emit
