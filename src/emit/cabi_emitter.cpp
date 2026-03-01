#include "csbind23/emit/cabi_emitter.hpp"

#include "cabi_emitter_utils.hpp"

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

void append_function_signature(
    TextWriter& output, const FunctionDecl& function_decl, const std::string& exported_name, std::size_t parameter_count)
{
    output.append_format("extern \"C\" {} {}(", function_decl.return_type.c_abi_name, exported_name);

    const auto parameters = leading_parameters(function_decl, parameter_count);
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const auto& parameter = parameters[index];
        output.append_format("{} {}", parameter.type.c_abi_name, parameter.name);
        if (index + 1 < parameters.size())
        {
            output.append(", ");
        }
    }

    output.append(")");
}

void append_method_signature(
    TextWriter& output, const FunctionDecl& function_decl, const std::string& exported_name, std::size_t parameter_count)
{
    const char* self_type = function_decl.is_const ? "const void*" : "void*";
    output.append_format("extern \"C\" {} {}({} self", function_decl.return_type.c_abi_name, exported_name, self_type);

    const auto parameters = leading_parameters(function_decl, parameter_count);
    for (const auto& parameter : parameters)
    {
        output.append_format(", {} {}", parameter.type.c_abi_name, parameter.name);
    }

    output.append(")");
}

void append_free_function_body(TextWriter& output, const FunctionDecl& function_decl, std::size_t parameter_count)
{
    const auto parameters = leading_parameters(function_decl, parameter_count);
    append_converted_arguments(output, parameters);
    const std::string call_arguments = render_call_arguments(parameters);
    if (function_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("    {}({});", function_decl.cpp_symbol, call_arguments);
        return;
    }

    output.append_line_format("    decltype(auto) result = {}({});", function_decl.cpp_symbol, call_arguments);
    output.append_line_format(
        "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(function_decl.return_type));
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
    (void)module_name;
    const std::string owner_class_name = method_decl.class_name.empty() ? class_decl.cpp_name : method_decl.class_name;

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

    if (method_decl.is_const)
    {
        output.append_line_format("        auto* owner_instance = static_cast<const {}*>(this);", owner_class_name);
    }
    else
    {
        output.append_line_format("        auto* owner_instance = static_cast<{}*>(this);", owner_class_name);
    }

    if (method_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("        owner_instance->{}::{}({});", owner_class_name, method_decl.cpp_symbol, base_call_args);
        output.append_line("    }");
        output.append_line();
        return;
    }

    output.append_line_format("        return owner_instance->{}::{}({});", owner_class_name, method_decl.cpp_symbol, base_call_args);
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
    const FunctionDecl& ctor_decl, bool use_director, std::size_t parameter_count)
{
    const auto parameters = leading_parameters(ctor_decl, parameter_count);
    append_converted_arguments(output, parameters);
    const std::string call_arguments = render_call_arguments(parameters);
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
    TextWriter& output, const ClassDecl& class_decl, const FunctionDecl& method_decl, bool explicit_base_call,
    std::size_t parameter_count)
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

    const std::string owner_class_name = method_decl.class_name.empty() ? class_decl.cpp_name : method_decl.class_name;
    if (method_decl.is_const)
    {
        output.append_line_format("    auto* owner_instance = static_cast<const {}*>(instance);", owner_class_name);
    }
    else
    {
        output.append_line_format("    auto* owner_instance = static_cast<{}*>(instance);", owner_class_name);
    }

    const auto parameters = leading_parameters(method_decl, parameter_count);
    append_converted_arguments(output, parameters);

    if (method_decl.is_field_accessor)
    {
        const std::string field_expression = std::format("owner_instance->{}", method_decl.cpp_symbol);
        if (method_decl.is_property_getter)
        {
            output.append_line_format("    decltype(auto) result = ({});", field_expression);
            output.append_line_format(
                "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(method_decl.return_type));
            return;
        }

        if (method_decl.is_property_setter)
        {
            if (!parameters.empty())
            {
                output.append_line_format("    {} = {};", field_expression, render_converted_argument_name(0));
            }
            return;
        }
    }

    const std::string call_arguments = render_call_arguments(parameters);
    const std::string method_expr = explicit_base_call
        ? std::format("{}::{}", owner_class_name, method_decl.cpp_symbol)
        : method_decl.cpp_symbol;

    if (method_decl.return_type.c_abi_name == "void")
    {
        output.append_line_format("    owner_instance->{}({});", method_expr, call_arguments);
        return;
    }

    output.append_line_format("    decltype(auto) result = owner_instance->{}({});", method_expr, call_arguments);
    output.append_line_format(
        "    return csbind23::cabi::Converter<{}>::to_c_abi(result);", render_cpp_type(method_decl.return_type));
}

void append_connect_signature(TextWriter& output, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods, const std::string& exported_name)
{
    (void)module_name;
    (void)class_decl;
    (void)virtual_methods;
    output.append_format("extern \"C\" void {}(void* self, const void* const* callbacks, std::size_t callback_count)", exported_name);
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
    output.append_line_format("    {}_{}_CallbackTable callback_table{{}};", module_name, class_decl.name);
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto* method_decl = virtual_methods[index];
        output.append_line_format("    if (callbacks != nullptr && callback_count > {})", index);
        output.append_line("    {");
        output.append_line_format(
            "        callback_table.{} = reinterpret_cast<{}>(callbacks[{}]);",
            method_decl->virtual_slot_name,
            callback_typedef_name(module_name, class_decl.name, *method_decl),
            index);
        output.append_line("    }");
    }
    output.append_line("    director->set_callbacks(callback_table);");
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
    generated.append_line("#include <cstddef>");
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
        const std::string exported_symbol = exported_symbol_name(function_decl);
        append_function_signature(
            generated, function_decl, module_decl.name + "_" + exported_symbol, function_decl.parameters.size());
        generated.append_line(" {");
        append_free_function_body(generated, function_decl, function_decl.parameters.size());
        generated.append_line("}");
        generated.append_line();

        const std::size_t defaults = normalized_trailing_default_count(function_decl);
        for (std::size_t omitted = 1; omitted <= defaults; ++omitted)
        {
            const std::size_t parameter_count = function_decl.parameters.size() - omitted;
            const std::string exported_name =
                module_decl.name + "_" + exported_symbol + std::format("__default_{}", omitted);

            append_function_signature(generated, function_decl, exported_name, parameter_count);
            generated.append_line(" {");
            append_free_function_body(generated, function_decl, parameter_count);
            generated.append_line("}");
            generated.append_line();
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        const auto emitted_methods = collect_emitted_methods(module_decl, class_decl);
        const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);
        const bool has_virtual_director = !virtual_methods.empty();

        std::size_t class_type_id = 0;
        for (std::size_t i = 0; i < module_decl.classes.size(); ++i)
        {
            if (module_decl.classes[i].cpp_name == class_decl.cpp_name)
            {
                class_type_id = i;
                break;
            }
        }

        generated.append_line_format(
            "extern \"C\" int {}_{}_static_type_id() {{", module_decl.name, class_decl.name);
        generated.append_line_format("    return {};", class_type_id);
        generated.append_line("}");
        generated.append_line();

        generated.append_line_format(
            "extern \"C\" int {}_{}_dynamic_type_id(const void* self) {{",
            module_decl.name,
            class_decl.name);
        generated.append_line_format(
            "    decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<const {}*>::from_c_abi(self);",
            class_decl.cpp_name);
        generated.append_line("    if (__csbind23_self_cpp == nullptr)");
        generated.append_line("    {");
        generated.append_line("        return -1;");
        generated.append_line("    }");
        generated.append_line("    const auto& __csbind23_dynamic_type = typeid(*__csbind23_self_cpp);");
        for (std::size_t i = 0; i < module_decl.classes.size(); ++i)
        {
            generated.append_line_format("    if (__csbind23_dynamic_type == typeid({}))", module_decl.classes[i].cpp_name);
            generated.append_line("    {");
            generated.append_line_format("        return {};", i);
            generated.append_line("    }");
        }
        generated.append_line_format("    return {};", class_type_id);
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
            if (method_decl.is_constructor)
            {
                const std::string exported_name = module_decl.name + "_" + class_decl.name + "_create";
                append_function_signature(generated, method_decl, exported_name, method_decl.parameters.size());
                generated.append_line(" {");
                append_constructor_body(
                    generated, module_decl.name, class_decl, method_decl, has_virtual_director, method_decl.parameters.size());
                generated.append_line("}");
                generated.append_line();

                const std::size_t defaults = normalized_trailing_default_count(method_decl);
                for (std::size_t omitted = 1; omitted <= defaults; ++omitted)
                {
                    const std::size_t parameter_count = method_decl.parameters.size() - omitted;
                    const std::string default_exported_name = exported_name + std::format("__default_{}", omitted);
                    append_function_signature(generated, method_decl, default_exported_name, parameter_count);
                    generated.append_line(" {");
                    append_constructor_body(
                        generated, module_decl.name, class_decl, method_decl, has_virtual_director, parameter_count);
                    generated.append_line("}");
                    generated.append_line();
                }
            }
        }

        for (const auto& emitted_method : emitted_methods)
        {
            const auto& method_decl = emitted_method.method;
            const std::string exported_name =
                module_decl.name + "_" + class_decl.name + "_" + exported_symbol_name(method_decl);

            append_method_signature(generated, method_decl, exported_name, method_decl.parameters.size());
            generated.append_line(" {");
            append_method_body(generated, class_decl, method_decl, false, method_decl.parameters.size());
            generated.append_line("}");
            generated.append_line();

            const std::size_t defaults = normalized_trailing_default_count(method_decl);
            for (std::size_t omitted = 1; omitted <= defaults; ++omitted)
            {
                const std::size_t parameter_count = method_decl.parameters.size() - omitted;
                const std::string default_exported_name = exported_name + std::format("__default_{}", omitted);
                append_method_signature(generated, method_decl, default_exported_name, parameter_count);
                generated.append_line(" {");
                append_method_body(generated, class_decl, method_decl, false, parameter_count);
                generated.append_line("}");
                generated.append_line();
            }

            if (has_virtual_director && method_decl.allow_override)
            {
                append_method_signature(generated, method_decl, exported_name + "__base", method_decl.parameters.size());
                generated.append_line(" {");
                append_method_body(generated, class_decl, method_decl, true, method_decl.parameters.size());
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
