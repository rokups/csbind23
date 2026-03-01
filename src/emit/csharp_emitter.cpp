#include "csbind23/emit/csharp_emitter.hpp"

#include "csbind23/cabi/converter.hpp"
#include "csbind23/text_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <limits>
#include <unordered_map>
#include <unordered_set>
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

bool is_any_of(std::string_view value, std::initializer_list<std::string_view> candidates)
{
    for (const auto candidate : candidates)
    {
        if (value == candidate)
        {
            return true;
        }
    }
    return false;
}

bool is_csharp_enum_integral_underlying(std::string_view type_name)
{
    return is_any_of(type_name, {"byte", "sbyte", "short", "ushort", "int", "uint", "long", "ulong"});
}

bool is_csharp_builtin_value_type(std::string_view type_name)
{
    return is_any_of(type_name,
        {"bool", "byte", "sbyte", "short", "ushort", "int", "uint", "long", "ulong", "nint", "nuint",
            "float", "double", "char"});
}

bool is_pinvoke_pointer_sized_integral(std::string_view type_name)
{
    return is_any_of(type_name, {"nint", "nuint"});
}

bool is_unsigned_long_family(std::string_view type_name)
{
    return is_any_of(type_name, {"unsigned long", "unsigned long long"});
}

bool is_signed_long_family(std::string_view type_name)
{
    return is_any_of(type_name, {"long", "long long"});
}

bool is_cabi_void(std::string_view c_abi_name)
{
    return c_abi_name == "void";
}

bool is_pinvoke_int_ptr(std::string_view pinvoke_name)
{
    return pinvoke_name == "System.IntPtr";
}

bool is_cpp_std_string(std::string_view cpp_name)
{
    return cpp_name == "std::string";
}

std::optional<std::string_view> pinvoke_integral_type_from_cabi(std::string_view c_abi_name)
{
    using csbind23::cabi::Converter;

    if (c_abi_name == Converter<signed char>::c_abi_type_name())
    {
        return Converter<signed char>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned char>::c_abi_type_name() || c_abi_name == Converter<char>::c_abi_type_name())
    {
        return Converter<unsigned char>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<short>::c_abi_type_name())
    {
        return Converter<short>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned short>::c_abi_type_name())
    {
        return Converter<unsigned short>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<int>::c_abi_type_name())
    {
        return Converter<int>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned int>::c_abi_type_name())
    {
        return Converter<unsigned int>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<long>::c_abi_type_name())
    {
        return Converter<long>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned long>::c_abi_type_name())
    {
        return Converter<unsigned long>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<long long>::c_abi_type_name())
    {
        return Converter<long long>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned long long>::c_abi_type_name())
    {
        return Converter<unsigned long long>::pinvoke_type_name();
    }

    return std::nullopt;
}

std::string normalize_enum_underlying_integral(
    std::string_view pinvoke_name, std::string_view cpp_name, std::string_view c_abi_name)
{
    if (!is_pinvoke_pointer_sized_integral(pinvoke_name))
    {
        return std::string(pinvoke_name);
    }

    if (is_unsigned_long_family(cpp_name) || is_unsigned_long_family(c_abi_name))
    {
        return "ulong";
    }

    if (is_signed_long_family(cpp_name) || is_signed_long_family(c_abi_name))
    {
        return "long";
    }

    return "long";
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

std::string exported_symbol_name(const FunctionDecl& function_decl)
{
    return function_decl.exported_name.empty() ? function_decl.name : function_decl.exported_name;
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

    if (parameter.type.is_pointer && !parameter.type.is_const && !is_pinvoke_int_ptr(parameter.type.pinvoke_name))
    {
        return true;
    }

    if (parameter.type.has_managed_converter() && parameter.type.is_pointer && !parameter.type.is_const
        && is_cpp_std_string(parameter.type.cpp_name))
    {
        return true;
    }

    return false;
}

bool parameter_is_direct_ref(const ParameterDecl& parameter)
{
    return parameter_is_ref(parameter) && parameter.type.managed_to_pinvoke_expression.empty();
}

bool parameter_is_direct_out(const ParameterDecl& parameter)
{
    return parameter.is_output && parameter_is_direct_ref(parameter);
}

std::string parameter_direct_byref_keyword(const ParameterDecl& parameter)
{
    if (!parameter_is_direct_ref(parameter))
    {
        return {};
    }

    return parameter_is_direct_out(parameter) ? "out " : "ref ";
}

std::string parameter_signature_byref_keyword(const ParameterDecl& parameter)
{
    if (!parameter_is_ref(parameter))
    {
        return {};
    }

    return parameter_is_direct_out(parameter) ? "out " : "ref ";
}

std::string parameter_call_argument(const ParameterDecl& parameter)
{
    return parameter_direct_byref_keyword(parameter) + parameter.name;
}

std::string parameter_call_argument(const ParameterDecl& parameter, std::string_view argument_expression)
{
    return parameter_direct_byref_keyword(parameter) + std::string(argument_expression);
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

const ClassDecl* primary_base_class(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    if (!class_decl.base_classes.empty())
    {
        return find_class_by_cpp_name(module_decl, class_decl.base_classes.front().cpp_name);
    }

    if (!class_decl.base_cpp_name.empty())
    {
        return find_class_by_cpp_name(module_decl, class_decl.base_cpp_name);
    }

    return nullptr;
}

std::vector<const ClassDecl*> secondary_base_classes(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    std::vector<const ClassDecl*> bases;
    if (!class_decl.base_classes.empty())
    {
        for (std::size_t index = 1; index < class_decl.base_classes.size(); ++index)
        {
            const auto* base_class = find_class_by_cpp_name(module_decl, class_decl.base_classes[index].cpp_name);
            if (base_class != nullptr)
            {
                bases.push_back(base_class);
            }
        }
    }
    return bases;
}

bool is_wrapper_visible_method(const FunctionDecl& method_decl)
{
    return !method_decl.is_constructor && method_decl.is_method && !method_decl.is_property_getter
        && !method_decl.is_property_setter;
}

std::string method_signature_key(const FunctionDecl& method_decl)
{
    std::string key = method_decl.name;
    key += method_decl.is_const ? "|const|" : "|mutable|";
    key += method_decl.return_type.pinvoke_name;
    key += "|";
    for (const auto& parameter : method_decl.parameters)
    {
        key += parameter.type.pinvoke_name;
        key += parameter.type.is_const ? "#c" : "#m";
        key += parameter.type.is_pointer ? "#p" : "#v";
        key += parameter.type.is_reference ? "#r" : "#v";
        key += ";";
    }
    return key;
}

struct EmittedMethod
{
    FunctionDecl method;
    bool inherited_from_secondary_base = false;
};

struct GenericFunctionGroup
{
    std::string name;
    std::vector<const FunctionDecl*> instantiations;
};

struct GenericClassGroup
{
    std::string name;
    std::vector<const ClassDecl*> instantiations;
};

std::vector<GenericFunctionGroup> collect_generic_function_groups(const std::vector<FunctionDecl>& functions)
{
    std::vector<GenericFunctionGroup> groups;
    std::unordered_map<std::string, std::size_t> by_name;
    for (const auto& function_decl : functions)
    {
        if (!function_decl.is_generic_instantiation || function_decl.generic_group_name.empty())
        {
            continue;
        }

        const std::string& name = function_decl.generic_group_name;
        auto it = by_name.find(name);
        if (it == by_name.end())
        {
            by_name.emplace(name, groups.size());
            groups.push_back(GenericFunctionGroup{name, {&function_decl}});
        }
        else
        {
            groups[it->second].instantiations.push_back(&function_decl);
        }
    }
    return groups;
}

std::vector<GenericFunctionGroup> collect_generic_method_groups(const std::vector<EmittedMethod>& methods)
{
    std::vector<GenericFunctionGroup> groups;
    std::unordered_map<std::string, std::size_t> by_name;
    for (const auto& emitted_method : methods)
    {
        const auto& method_decl = emitted_method.method;
        if (!method_decl.is_generic_instantiation || method_decl.generic_group_name.empty())
        {
            continue;
        }

        const std::string& name = method_decl.generic_group_name;
        auto it = by_name.find(name);
        if (it == by_name.end())
        {
            by_name.emplace(name, groups.size());
            groups.push_back(GenericFunctionGroup{name, {&method_decl}});
        }
        else
        {
            groups[it->second].instantiations.push_back(&method_decl);
        }
    }
    return groups;
}

bool class_has_owned_ctor(const ClassDecl& class_decl);

std::vector<GenericClassGroup> collect_generic_class_groups(const std::vector<ClassDecl>& classes)
{
    std::vector<GenericClassGroup> groups;
    std::unordered_map<std::string, std::size_t> by_name;
    for (const auto& class_decl : classes)
    {
        if (!class_decl.is_generic_instantiation || class_decl.generic_group_name.empty())
        {
            continue;
        }

        const std::string& name = class_decl.generic_group_name;
        auto it = by_name.find(name);
        if (it == by_name.end())
        {
            by_name.emplace(name, groups.size());
            groups.push_back(GenericClassGroup{name, {&class_decl}});
        }
        else
        {
            groups[it->second].instantiations.push_back(&class_decl);
        }
    }
    return groups;
}

std::vector<EmittedMethod> collect_emitted_methods(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    std::vector<EmittedMethod> emitted_methods;
    emitted_methods.reserve(class_decl.methods.size());

    std::unordered_set<std::string> signatures;
    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor || !method_decl.is_method)
        {
            continue;
        }

        if (is_wrapper_visible_method(method_decl))
        {
            signatures.insert(method_signature_key(method_decl));
        }
        emitted_methods.push_back(EmittedMethod{method_decl, false});
    }

    for (const auto* secondary_base : secondary_base_classes(module_decl, class_decl))
    {
        for (const auto& base_method : secondary_base->methods)
        {
            if (!is_wrapper_visible_method(base_method))
            {
                continue;
            }

            const std::string signature = method_signature_key(base_method);
            if (signatures.contains(signature))
            {
                continue;
            }

            signatures.insert(signature);
            FunctionDecl inherited_method = base_method;
            inherited_method.class_name = secondary_base->cpp_name;
            emitted_methods.push_back(EmittedMethod{std::move(inherited_method), true});
        }
    }

    return emitted_methods;
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
    if (is_cabi_void(function_decl.return_type.c_abi_name))
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

std::string default_variant_condition(std::size_t omitted, const std::vector<std::string>& has_value_expressions)
{
    const std::size_t optional_count = has_value_expressions.size();
    std::string condition;
    for (std::size_t index = 0; index < optional_count; ++index)
    {
        const bool should_be_present = index < (optional_count - omitted);
        if (!condition.empty())
        {
            condition += " && ";
        }
        if (should_be_present)
        {
            condition += has_value_expressions[index];
        }
        else
        {
            condition += "!(" + has_value_expressions[index] + ")";
        }
    }

    return condition.empty() ? "true" : condition;
}

void append_default_variant_if_chain(TextWriter& output, std::string_view indent, const std::string& module_name,
    const std::string& native_name, const std::vector<std::string>& call_arguments, std::size_t defaults,
    const std::vector<std::string>& has_value_expressions, std::string_view assign_target, bool emit_return)
{
    for (std::size_t omitted = 0; omitted <= defaults; ++omitted)
    {
        const std::size_t count = call_arguments.size() - omitted;
        std::vector<std::string> prefix_args(call_arguments.begin(), call_arguments.begin() + count);
        const std::string call_arguments_rendered = join_arguments(prefix_args);
        const std::string variant_name =
            omitted == 0 ? native_name : native_name + std::format("__default_{}", omitted);
        const std::string variant_call = std::format("{}Native.{}({})", module_name, variant_name, call_arguments_rendered);
        const std::string condition = default_variant_condition(omitted, has_value_expressions);

        if (omitted == 0)
        {
            output.append_line_format("{}if ({})", indent, condition);
        }
        else
        {
            output.append_line_format("{}else if ({})", indent, condition);
        }
        output.append_line_format("{}{{", indent);

        if (emit_return)
        {
            output.append_line_format("{}    return {} ;", indent, variant_call);
        }
        else if (!assign_target.empty())
        {
            output.append_line_format("{}    {} = {} ;", indent, assign_target, variant_call);
        }
        else
        {
            output.append_line_format("{}    {} ;", indent, variant_call);
        }

        output.append_line_format("{}}}", indent);
    }

    output.append_line_format("{}else", indent);
    output.append_line_format("{}{{", indent);
    output.append_line_format(
        "{}    throw new System.ArgumentException(\"Optional arguments must be omitted from right to left.\");", indent);
    output.append_line_format("{}}}", indent);
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
    if (is_csharp_enum_integral_underlying(type_name))
    {
        return type_name;
    }

    const auto& cpp_name = enum_decl.underlying_type.cpp_name;
    const auto& c_abi_name = enum_decl.underlying_type.c_abi_name;

    if (is_pinvoke_pointer_sized_integral(type_name))
    {
        return normalize_enum_underlying_integral(type_name, cpp_name, c_abi_name);
    }

    if (const auto mapped = pinvoke_integral_type_from_cabi(c_abi_name); mapped.has_value())
    {
        return normalize_enum_underlying_integral(*mapped, cpp_name, c_abi_name);
    }

    return "int";
}

std::string normalize_csharp_attribute(std::string_view attribute)
{
    const std::size_t first = attribute.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
    {
        return {};
    }
    const std::size_t last = attribute.find_last_not_of(" \t\r\n");
    const std::string_view trimmed = attribute.substr(first, last - first + 1);
    if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']')
    {
        return std::string(trimmed);
    }
    return std::format("[{}]", trimmed);
}

void append_csharp_attributes(
    TextWriter& output, std::string_view indent, const std::vector<std::string>& csharp_attributes)
{
    for (const auto& attribute : csharp_attributes)
    {
        const std::string rendered = normalize_csharp_attribute(attribute);
        if (!rendered.empty())
        {
            output.append_line_format("{}{}", indent, rendered);
        }
    }
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
        rendered += std::format("{}{} {}", parameter_signature_byref_keyword(parameter), wrapper_type_name(parameter.type),
            parameter.name);
        if (index + 1 < function_decl.parameters.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

bool csharp_type_is_value_type(const ModuleDecl& module_decl, std::string_view type_name)
{
    if (is_csharp_builtin_value_type(type_name))
    {
        return true;
    }

    for (const auto& enum_decl : module_decl.enums)
    {
        if (enum_decl.name == type_name)
        {
            return true;
        }
    }

    return false;
}

bool parameter_can_be_nullable_optional(const ParameterDecl& parameter)
{
    return !parameter_is_ref(parameter) && parameter.type.managed_to_pinvoke_expression.empty()
        && parameter.type.managed_finalize_to_pinvoke_statement.empty();
}

std::size_t free_function_default_count(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    (void)module_decl;
    const std::size_t declared = function_decl.trailing_default_argument_count > function_decl.parameters.size()
        ? function_decl.parameters.size()
        : function_decl.trailing_default_argument_count;

    std::size_t effective = 0;
    for (std::size_t offset = 0; offset < declared; ++offset)
    {
        const auto& parameter = function_decl.parameters[function_decl.parameters.size() - 1 - offset];
        if (!parameter_can_be_nullable_optional(parameter))
        {
            break;
        }
        ++effective;
    }

    return effective;
}

bool is_nullable_optional_parameter(const ModuleDecl& module_decl, const FunctionDecl& function_decl, std::size_t index)
{
    const std::size_t defaults = free_function_default_count(module_decl, function_decl);
    if (defaults == 0 || index >= function_decl.parameters.size())
    {
        return false;
    }

    return index >= (function_decl.parameters.size() - defaults);
}

std::string parameter_type_for_public_signature(
    const ModuleDecl& module_decl, const FunctionDecl& function_decl, const ParameterDecl& parameter, std::size_t index)
{
    std::string type_name = wrapper_type_name(parameter.type);
    if (!is_nullable_optional_parameter(module_decl, function_decl, index))
    {
        return type_name;
    }

    type_name += "?";
    return type_name;
}

std::string free_function_parameter_list(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    std::string rendered;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        rendered += std::format("{}{} {}", parameter_signature_byref_keyword(parameter),
            parameter_type_for_public_signature(module_decl, function_decl, parameter, index), parameter.name);
        if (index + 1 < function_decl.parameters.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

std::string nullable_parameter_unwrap_expression(
    const ModuleDecl& module_decl, const FunctionDecl& function_decl, const ParameterDecl& parameter, std::size_t index)
{
    if (!is_nullable_optional_parameter(module_decl, function_decl, index))
    {
        return parameter.name;
    }

    const std::string wrapper_type = wrapper_type_name(parameter.type);
    if (csharp_type_is_value_type(module_decl, wrapper_type))
    {
        return parameter.name + ".Value";
    }

    return parameter.name + "!";
}

std::string optional_parameter_has_value_expression(
    const ModuleDecl& module_decl, const FunctionDecl& function_decl, const ParameterDecl& parameter, std::size_t index)
{
    if (!is_nullable_optional_parameter(module_decl, function_decl, index))
    {
        return "true";
    }

    const std::string wrapper_type = wrapper_type_name(parameter.type);
    if (csharp_type_is_value_type(module_decl, wrapper_type))
    {
        return parameter.name + ".HasValue";
    }

    return parameter.name + " != null";
}

std::vector<const FunctionDecl*> collect_virtual_methods(
    const ClassDecl& class_decl, const std::vector<EmittedMethod>& emitted_methods)
{
    std::vector<const FunctionDecl*> methods;
    for (const auto& emitted_method : emitted_methods)
    {
        const auto& method_decl = emitted_method.method;
        if (method_decl.is_constructor || !method_decl.is_method || !method_decl.allow_override)
        {
            continue;
        }

        if (!class_decl.enable_virtual_overrides && !emitted_method.inherited_from_secondary_base)
        {
            continue;
        }

        methods.push_back(&emitted_method.method);
    }

    return methods;
}

struct GenericDispatchShape
{
    bool supported = false;
    std::vector<int> generic_parameter_slot_ids;
    int generic_return_slot_id = -1;
    std::size_t generic_arity = 0;
    std::string unsupported_reason;
};

std::string concrete_type_for_slot(
    const FunctionDecl& instantiation, const GenericDispatchShape& shape, int slot_id);

std::string csharp_string_literal_escape(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size() + 16);
    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

std::string generic_expected_type_tuples(const GenericFunctionGroup& group, const GenericDispatchShape& shape)
{
    if (shape.generic_arity == 0)
    {
        return {};
    }

    std::vector<std::string> tuples;
    tuples.reserve(group.instantiations.size());
    for (const auto* instantiation : group.instantiations)
    {
        std::string tuple = "(";
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            if (slot > 0)
            {
                tuple += ", ";
            }
            tuple += concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
        }
        tuple += ")";
        tuples.push_back(std::move(tuple));
    }

    std::sort(tuples.begin(), tuples.end());
    tuples.erase(std::unique(tuples.begin(), tuples.end()), tuples.end());
    return join_arguments(tuples);
}

std::string generic_dispatch_not_supported_message(const GenericFunctionGroup& group, const GenericDispatchShape& shape)
{
    std::string message = std::format(
        "No generic mapping for {} with provided generic type arguments.",
        group.name);

    const std::string expected_tuples = generic_expected_type_tuples(group, shape);
    if (!expected_tuples.empty())
    {
        message += std::format(" Expected type tuples: {}.", expected_tuples);
    }

    if (!shape.supported && !shape.unsupported_reason.empty())
    {
        message += std::format(" Dispatch shape is not supported: {}.", shape.unsupported_reason);
    }

    return message;
}

bool same_wrapper_type_shape(const TypeRef& lhs, const TypeRef& rhs)
{
    return wrapper_type_name(lhs) == wrapper_type_name(rhs)
        && lhs.is_const == rhs.is_const
        && lhs.is_pointer == rhs.is_pointer
        && lhs.is_reference == rhs.is_reference;
}

std::string generic_type_parameter_name(std::size_t index, std::size_t generic_arity)
{
    if (generic_arity == 1)
    {
        return "T";
    }

    return std::format("T{}", index);
}

std::string generic_type_parameter_list(std::size_t generic_arity)
{
    std::string rendered;
    for (std::size_t index = 0; index < generic_arity; ++index)
    {
        if (index > 0)
        {
            rendered += ", ";
        }
        rendered += generic_type_parameter_name(index, generic_arity);
    }
    return rendered;
}

std::string concrete_type_for_slot(
    const FunctionDecl& instantiation, const GenericDispatchShape& shape, int slot_id);

std::string concrete_type_for_slot(
    const FunctionDecl& instantiation, const GenericDispatchShape& shape, int slot_id)
{
    for (std::size_t index = 0; index < instantiation.parameters.size(); ++index)
    {
        if (shape.generic_parameter_slot_ids[index] == slot_id)
        {
            return wrapper_type_name(instantiation.parameters[index].type);
        }
    }

    if (shape.generic_return_slot_id == slot_id)
    {
        return wrapper_type_name(instantiation.return_type);
    }

    return {};
}

GenericDispatchShape analyze_generic_dispatch_shape(const GenericFunctionGroup& group)
{
    GenericDispatchShape shape;
    if (group.instantiations.empty())
    {
        return shape;
    }

    const FunctionDecl& first = *group.instantiations.front();
    const std::size_t parameter_count = first.parameters.size();
    shape.generic_parameter_slot_ids.assign(parameter_count, -1);

    for (const auto* instantiation : group.instantiations)
    {
        if (instantiation->parameters.size() != parameter_count)
        {
            return GenericDispatchShape{};
        }
    }

    std::vector<bool> varying_parameters(parameter_count, false);
    bool has_unsupported_varying_shape = false;

    for (std::size_t index = 0; index < parameter_count; ++index)
    {
        bool all_same = true;
        const auto& first_parameter = first.parameters[index];
        for (const auto* instantiation : group.instantiations)
        {
            const auto& parameter = instantiation->parameters[index];
            if (!same_wrapper_type_shape(parameter.type, first_parameter.type))
            {
                all_same = false;
            }
        }

        if (!all_same)
        {
            for (const auto* instantiation : group.instantiations)
            {
                const auto& parameter = instantiation->parameters[index];
                if (parameter.type.has_managed_converter())
                {
                    has_unsupported_varying_shape = true;
                    shape.unsupported_reason = "varying generic slots with managed converters";
                }
            }

            varying_parameters[index] = true;
        }
    }

    bool varying_return = false;
    bool return_all_same = true;
    for (const auto* instantiation : group.instantiations)
    {
        const auto& return_type = instantiation->return_type;
        if (!same_wrapper_type_shape(return_type, first.return_type))
        {
            return_all_same = false;
        }
    }

    if (!return_all_same)
    {
        for (const auto* instantiation : group.instantiations)
        {
            const auto& return_type = instantiation->return_type;
            if (return_type.is_pointer || return_type.is_reference || return_type.has_managed_converter())
            {
                has_unsupported_varying_shape = true;
                shape.unsupported_reason = "varying generic return slots that are pointer/reference/converter-backed";
            }
        }

        varying_return = true;
    }

    bool saw_generic_slot = varying_return;
    for (const bool varying_parameter : varying_parameters)
    {
        saw_generic_slot = saw_generic_slot || varying_parameter;
    }

    if (!saw_generic_slot)
    {
        return GenericDispatchShape{};
    }

    std::unordered_map<std::string, int> slot_groups;
    auto assign_slot_group = [&](std::size_t index, bool is_return_slot) {
        std::string key;
        for (const auto* instantiation : group.instantiations)
        {
            const std::string slot_type = is_return_slot
                ? wrapper_type_name(instantiation->return_type)
                : wrapper_type_name(instantiation->parameters[index].type);
            key += slot_type;
            key += "|";
        }

        auto it = slot_groups.find(key);
        if (it != slot_groups.end())
        {
            return it->second;
        }

        const int group_id = static_cast<int>(slot_groups.size());
        slot_groups.emplace(std::move(key), group_id);
        return group_id;
    };

    for (std::size_t index = 0; index < parameter_count; ++index)
    {
        if (!varying_parameters[index])
        {
            continue;
        }

        shape.generic_parameter_slot_ids[index] = assign_slot_group(index, false);
    }

    if (varying_return)
    {
        shape.generic_return_slot_id = assign_slot_group(0, true);
    }

    shape.generic_arity = slot_groups.size();
    if (shape.generic_arity == 0)
    {
        return GenericDispatchShape{};
    }

    for (const auto* instantiation : group.instantiations)
    {
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            const std::string concrete_type =
                concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
            if (concrete_type.empty())
            {
                return GenericDispatchShape{};
            }
        }
    }

    shape.supported = saw_generic_slot && !has_unsupported_varying_shape;
    return shape;
}

void append_free_generic_dispatch_wrapper(TextWriter& output, const ModuleDecl& module_decl, const GenericFunctionGroup& group)
{
    const auto shape = analyze_generic_dispatch_shape(group);
    if (shape.generic_arity == 0)
    {
        return;
    }

    const FunctionDecl& first = *group.instantiations.front();
    const std::string return_type = shape.generic_return_slot_id >= 0
        ? generic_type_parameter_name(static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity)
        : wrapper_return_type(module_decl, first);
    const std::string generic_parameter_list = generic_type_parameter_list(shape.generic_arity);

    output.append_format("    public static {} {}<{}>(", return_type, group.name, generic_parameter_list);
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = shape.generic_parameter_slot_ids[index];
        const std::string parameter_type = slot_id >= 0
            ? generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    const std::string diagnostics_message =
        csharp_string_literal_escape(generic_dispatch_not_supported_message(group, shape));
    if (!shape.supported)
    {
        output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
        output.append_line("    }");
        output.append_line();
        return;
    }

    for (const auto* instantiation : group.instantiations)
    {
        std::string condition;
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            if (!condition.empty())
            {
                condition += " && ";
            }

            const std::string generic_name = generic_type_parameter_name(slot, shape.generic_arity);
            const std::string concrete_name =
                concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
            condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
        }

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = shape.generic_parameter_slot_ids[index];
            if (slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);
                    call_arguments.push_back(typed_name);
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call = std::format("{}({})", instantiation->name, join_arguments(call_arguments));
        if (return_type == "void")
        {
            output.append_line_format("            {} ;", call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line("            return;");
        }
        else if (shape.generic_return_slot_id >= 0)
        {
            const std::string concrete_name = wrapper_type_name(instantiation->return_type);
            const std::string generic_name = generic_type_parameter_name(
                static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity);
            output.append_line_format("            {} __csbind23_typed_result = {} ;", concrete_name, call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format(
                "            return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                concrete_name,
                generic_name);
        }
        else
        {
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format("            return {};", call);
        }
        output.append_line("        }");
    }

    output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
    output.append_line("    }");
    output.append_line();
}

void append_method_generic_dispatch_wrapper(TextWriter& output, const ModuleDecl& module_decl, const GenericFunctionGroup& group)
{
    const auto shape = analyze_generic_dispatch_shape(group);
    if (shape.generic_arity == 0)
    {
        return;
    }

    const FunctionDecl& first = *group.instantiations.front();
    const std::string return_type = shape.generic_return_slot_id >= 0
        ? generic_type_parameter_name(static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity)
        : wrapper_return_type(module_decl, first);
    const std::string generic_parameter_list = generic_type_parameter_list(shape.generic_arity);

    output.append_format("    public {} {}<{}>(", return_type, group.name, generic_parameter_list);
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = shape.generic_parameter_slot_ids[index];
        const std::string parameter_type = slot_id >= 0
            ? generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    const std::string diagnostics_message =
        csharp_string_literal_escape(generic_dispatch_not_supported_message(group, shape));
    if (!shape.supported)
    {
        output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
        output.append_line("    }");
        output.append_line();
        return;
    }

    for (const auto* instantiation : group.instantiations)
    {
        std::string condition;
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            if (!condition.empty())
            {
                condition += " && ";
            }

            const std::string generic_name = generic_type_parameter_name(slot, shape.generic_arity);
            const std::string concrete_name =
                concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
            condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
        }

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = shape.generic_parameter_slot_ids[index];
            if (slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);
                    call_arguments.push_back(typed_name);
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call = std::format("{}({})", instantiation->name, join_arguments(call_arguments));
        if (return_type == "void")
        {
            output.append_line_format("            {} ;", call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line("            return;");
        }
        else if (shape.generic_return_slot_id >= 0)
        {
            const std::string concrete_name = wrapper_type_name(instantiation->return_type);
            const std::string generic_name = generic_type_parameter_name(
                static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity);
            output.append_line_format("            {} __csbind23_typed_result = {} ;", concrete_name, call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format(
                "            return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                concrete_name,
                generic_name);
        }
        else
        {
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format("            return {};", call);
        }
        output.append_line("        }");
    }

    output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
    output.append_line("    }");
    output.append_line();
}

std::string generic_class_member_shape_key(const FunctionDecl& method_decl)
{
    std::string key = method_decl.name;
    key += method_decl.is_const ? "|const|" : "|mutable|";
    key += std::format("{}|", method_decl.parameters.size());
    for (const auto& parameter : method_decl.parameters)
    {
        key += parameter.is_output ? "#out" : "#in";
        key += parameter.type.is_reference ? "#ref" : "#val";
        key += parameter.type.is_pointer ? "#ptr" : "#scalar";
        key += parameter.type.is_const ? "#const" : "#mutable";
        key += ";";
    }
    return key;
}

std::vector<const FunctionDecl*> collect_declared_constructors(const ClassDecl& class_decl)
{
    std::vector<const FunctionDecl*> constructors;
    constructors.reserve(class_decl.methods.size());
    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor)
        {
            constructors.push_back(&method_decl);
        }
    }
    return constructors;
}

std::vector<GenericFunctionGroup> collect_generic_class_ctor_groups(const GenericClassGroup& class_group)
{
    std::vector<GenericFunctionGroup> groups;
    if (class_group.instantiations.empty())
    {
        return groups;
    }

    const auto first_ctors = collect_declared_constructors(*class_group.instantiations.front());
    for (const auto* first_ctor : first_ctors)
    {
        const std::string key = generic_class_member_shape_key(*first_ctor);
        GenericFunctionGroup ctor_group;
        ctor_group.name = class_group.name;
        ctor_group.instantiations.push_back(first_ctor);

        bool complete = true;
        for (std::size_t class_index = 1; class_index < class_group.instantiations.size(); ++class_index)
        {
            const auto* class_decl = class_group.instantiations[class_index];
            const FunctionDecl* matched_ctor = nullptr;
            for (const auto& candidate : class_decl->methods)
            {
                if (!candidate.is_constructor)
                {
                    continue;
                }
                if (generic_class_member_shape_key(candidate) == key)
                {
                    matched_ctor = &candidate;
                    break;
                }
            }

            if (matched_ctor == nullptr)
            {
                complete = false;
                break;
            }

            ctor_group.instantiations.push_back(matched_ctor);
        }

        if (complete)
        {
            groups.push_back(std::move(ctor_group));
        }
    }

    return groups;
}

std::vector<GenericFunctionGroup> collect_generic_class_method_groups(const GenericClassGroup& class_group)
{
    std::vector<GenericFunctionGroup> groups;
    if (class_group.instantiations.empty())
    {
        return groups;
    }

    const auto* first_class = class_group.instantiations.front();
    for (const auto& first_method : first_class->methods)
    {
        if (!is_wrapper_visible_method(first_method))
        {
            continue;
        }

        const std::string key = generic_class_member_shape_key(first_method);
        GenericFunctionGroup method_group;
        method_group.name = first_method.name;
        method_group.instantiations.push_back(&first_method);

        bool complete = true;
        for (std::size_t class_index = 1; class_index < class_group.instantiations.size(); ++class_index)
        {
            const auto* class_decl = class_group.instantiations[class_index];
            const FunctionDecl* matched_method = nullptr;
            for (const auto& candidate : class_decl->methods)
            {
                if (!is_wrapper_visible_method(candidate))
                {
                    continue;
                }

                if (generic_class_member_shape_key(candidate) == key)
                {
                    matched_method = &candidate;
                    break;
                }
            }

            if (matched_method == nullptr)
            {
                complete = false;
                break;
            }

            method_group.instantiations.push_back(matched_method);
        }

        if (complete)
        {
            groups.push_back(std::move(method_group));
        }
    }

    return groups;
}

std::string generic_class_type_parameter_name(std::size_t slot, std::size_t class_generic_arity)
{
    return generic_type_parameter_name(slot, class_generic_arity);
}

void append_generic_class_constructor(TextWriter& output, const GenericClassGroup& class_group,
    const ModuleDecl& module_decl, const GenericFunctionGroup& ctor_group, std::size_t class_generic_arity,
    const std::vector<std::vector<std::string>>& class_slot_types)
{
    const auto shape = analyze_generic_dispatch_shape(ctor_group);
    const bool use_shape_for_parameters = shape.supported;
    const FunctionDecl& first = *ctor_group.instantiations.front();
    output.append_format("    public {}(", class_group.name);
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
        const std::string parameter_type = (use_shape_for_parameters && slot_id >= 0)
            ? generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    for (std::size_t inst_index = 0; inst_index < ctor_group.instantiations.size(); ++inst_index)
    {
        const auto* instantiation = ctor_group.instantiations[inst_index];
        const auto* class_decl = class_group.instantiations[inst_index];

        std::string condition;
        for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
        {
            const std::string generic_name = generic_class_type_parameter_name(slot, class_generic_arity);
            const std::string concrete_name = class_slot_types[inst_index][slot];
            if (concrete_name.empty())
            {
                continue;
            }

            if (!condition.empty())
            {
                condition += " && ";
            }
            condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
        }

        if (condition.empty())
        {
            condition = "true";
        }

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
            if (use_shape_for_parameters && slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);
                    call_arguments.push_back(typed_name);
                }
            }
            else if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                const std::string converted = render_inline_template(
                    parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                    module_decl.name);
                append_embedded_assignment(
                    output,
                    "            ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    converted);
                call_arguments.push_back(pinvoke_name);

                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call_arguments_rendered = join_arguments(call_arguments);
        const std::string native_call = call_arguments_rendered.empty()
            ? std::format("{}Native.{}_{}_create()", module_decl.name, module_decl.name, class_decl->name)
            : std::format("{}Native.{}_{}_create({})", module_decl.name, module_decl.name, class_decl->name,
                call_arguments_rendered);

        if (finalize_statements.empty())
        {
            output.append_line_format("            _handle = {} ;", native_call);
        }
        else
        {
            output.append_line("            try");
            output.append_line("            {");
            output.append_line_format("                _handle = {} ;", native_call);
            output.append_line("            }");
            output.append_line("            finally");
            output.append_line("            {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "                ", finalize_statement);
            }
            output.append_line("            }");
        }

        output.append_line_format("            _ownsHandle = {} ;", class_has_owned_ctor(*class_decl) ? "true" : "false");
        output.append_line("            return;");
        output.append_line("        }");
    }

    output.append_line_format(
        "        throw new System.NotSupportedException(\"No generic mapping for {} with provided generic type arguments.\");",
        class_group.name);
    output.append_line("    }");
    output.append_line();
}

std::vector<std::vector<std::string>> collect_generic_class_slot_types(
    const GenericClassGroup& class_group, const std::vector<GenericFunctionGroup>& ctor_groups,
    const std::vector<GenericFunctionGroup>& method_groups,
    std::size_t class_generic_arity)
{
    std::vector<std::vector<std::string>> slot_types(
        class_group.instantiations.size(), std::vector<std::string>(class_generic_arity));

    for (const auto& method_group : method_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(method_group);
        if (!shape.supported)
        {
            continue;
        }

        for (std::size_t inst_index = 0; inst_index < method_group.instantiations.size(); ++inst_index)
        {
            const auto* instantiation = method_group.instantiations[inst_index];
            for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
            {
                const std::string concrete = concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
                if (!concrete.empty() && slot_types[inst_index][slot].empty())
                {
                    slot_types[inst_index][slot] = concrete;
                }
            }
        }
    }

    for (const auto& ctor_group : ctor_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(ctor_group);
        if (!shape.supported)
        {
            continue;
        }

        for (std::size_t inst_index = 0; inst_index < ctor_group.instantiations.size(); ++inst_index)
        {
            const auto* instantiation = ctor_group.instantiations[inst_index];
            for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
            {
                const std::string concrete = concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
                if (!concrete.empty() && slot_types[inst_index][slot].empty())
                {
                    slot_types[inst_index][slot] = concrete;
                }
            }
        }
    }

    return slot_types;
}

void append_generic_class_method(TextWriter& output, const ModuleDecl& module_decl, const GenericClassGroup& class_group,
    const GenericFunctionGroup& method_group, std::size_t class_generic_arity,
    const std::vector<std::vector<std::string>>& class_slot_types)
{
    const auto shape = analyze_generic_dispatch_shape(method_group);
    const bool use_shape_for_parameters = shape.supported;
    const bool use_shape_for_return = shape.supported && shape.generic_return_slot_id >= 0;

    const FunctionDecl& first = *method_group.instantiations.front();
    const std::string return_type = use_shape_for_return
        ? generic_class_type_parameter_name(static_cast<std::size_t>(shape.generic_return_slot_id), class_generic_arity)
        : wrapper_return_type(module_decl, first);

    output.append_format("    public {} {}(", return_type, method_group.name);
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
        const std::string parameter_type = (use_shape_for_parameters && slot_id >= 0)
            ? generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    const std::string diagnostics_message = csharp_string_literal_escape(std::format(
        "No generic mapping for {}.{} with provided generic type arguments.",
        class_group.name,
        method_group.name));

    for (std::size_t inst_index = 0; inst_index < method_group.instantiations.size(); ++inst_index)
    {
        const auto* instantiation = method_group.instantiations[inst_index];
        std::string condition;
        for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
        {
            const std::string generic_name = generic_class_type_parameter_name(slot, class_generic_arity);
            const std::string concrete_name = class_slot_types[inst_index][slot];
            if (concrete_name.empty())
            {
                continue;
            }

            if (!condition.empty())
            {
                condition += " && ";
            }
            condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
        }

        if (condition.empty())
        {
            condition = "true";
        }

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
            if (use_shape_for_parameters && slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);
                    call_arguments.push_back(typed_name);
                }
            }
            else if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                const std::string converted = render_inline_template(
                    parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                    module_decl.name);
                append_embedded_assignment(
                    output,
                    "            ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    converted);
                call_arguments.push_back(pinvoke_name);

                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call_arguments_rendered = join_arguments(call_arguments);
        const std::string call = call_arguments_rendered.empty()
            ? std::format("{}Native.{}_{}_{}(_handle)", module_decl.name, module_decl.name,
                class_group.instantiations[inst_index]->name, exported_symbol_name(*instantiation))
            : std::format("{}Native.{}_{}_{}(_handle, {})", module_decl.name, module_decl.name,
                class_group.instantiations[inst_index]->name, exported_symbol_name(*instantiation),
                call_arguments_rendered);

        const bool has_return_converter = !instantiation->return_type.managed_from_pinvoke_expression.empty();
        const bool has_return_finalize = !instantiation->return_type.managed_finalize_from_pinvoke_statement.empty();
        const bool needs_finally = !finalize_statements.empty() || has_return_finalize;
        const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, *instantiation);

        if (return_type == "void")
        {
            if (!needs_finally)
            {
                output.append_line_format("            {} ;", call);
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("            {}", post_assignment);
                }
                output.append_line("            return;");
            }
            else
            {
                output.append_line("            try");
                output.append_line("            {");
                output.append_line_format("                {} ;", call);
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("                {}", post_assignment);
                }
                output.append_line("                return;");
                output.append_line("            }");
                output.append_line("            finally");
                output.append_line("            {");
                for (const auto& finalize_statement : finalize_statements)
                {
                    append_embedded_statement(output, "                ", finalize_statement);
                }
                output.append_line("            }");
            }
        }
        else if (use_shape_for_return)
        {
            const std::string concrete_name = wrapper_type_name(instantiation->return_type);
            const std::string generic_name = generic_class_type_parameter_name(
                static_cast<std::size_t>(shape.generic_return_slot_id), class_generic_arity);
            if (!needs_finally)
            {
                output.append_line_format("            {} __csbind23_typed_result = {} ;", concrete_name, call);
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("            {}", post_assignment);
                }
                output.append_line_format(
                    "            return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                    concrete_name,
                    generic_name);
            }
            else
            {
                output.append_line_format("            {} __csbind23_typed_result = default!;", concrete_name);
                output.append_line("            try");
                output.append_line("            {");
                output.append_line_format("                __csbind23_typed_result = {} ;", call);
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("                {}", post_assignment);
                }
                output.append_line_format(
                    "                return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                    concrete_name,
                    generic_name);
                output.append_line("            }");
                output.append_line("            finally");
                output.append_line("            {");
                for (const auto& finalize_statement : finalize_statements)
                {
                    append_embedded_statement(output, "                ", finalize_statement);
                }
                output.append_line("            }");
            }
        }
        else
        {
            if (polymorphic_return_class != nullptr)
            {
                if (!needs_finally)
                {
                    output.append_line_format("            System.IntPtr __csbind23_result_ptr = {} ;", call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("            {}", post_assignment);
                    }
                    output.append_line_format("            return {}Runtime.WrapPolymorphic_{}(__csbind23_result_ptr, false);",
                        module_decl.name,
                        polymorphic_return_class->name);
                }
                else
                {
                    output.append_line("            try");
                    output.append_line("            {");
                    output.append_line_format("                System.IntPtr __csbind23_result_ptr = {} ;", call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("                {}", post_assignment);
                    }
                    output.append_line_format("                return {}Runtime.WrapPolymorphic_{}(__csbind23_result_ptr, false);",
                        module_decl.name,
                        polymorphic_return_class->name);
                    output.append_line("            }");
                    output.append_line("            finally");
                    output.append_line("            {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(output, "                ", finalize_statement);
                    }
                    output.append_line("            }");
                }
            }
            else if (!has_return_converter)
            {
                if (!needs_finally)
                {
                    output.append_line_format("            {} __csbind23_result = {} ;", return_type, call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("            {}", post_assignment);
                    }
                    output.append_line("            return __csbind23_result ;");
                }
                else
                {
                    output.append_line_format("            {} __csbind23_result = default!;", return_type);
                    output.append_line("            try");
                    output.append_line("            {");
                    output.append_line_format("                __csbind23_result = {} ;", call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("                {}", post_assignment);
                    }
                    output.append_line("                return __csbind23_result ;");
                    output.append_line("            }");
                    output.append_line("            finally");
                    output.append_line("            {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(output, "                ", finalize_statement);
                    }
                    output.append_line("            }");
                }
            }
            else
            {
                const std::string native_result_name = "__csbind23_result_pinvoke";
                const std::string managed_result_name = "__csbind23_result_managed";
                const std::string converted_expression = render_inline_template(
                    instantiation->return_type.managed_from_pinvoke_expression, managed_result_name, native_result_name,
                    native_result_name, module_decl.name);

                if (!needs_finally)
                {
                    output.append_line_format("            {} {} = {} ;", instantiation->return_type.pinvoke_name,
                        native_result_name, call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("            {}", post_assignment);
                    }
                    append_embedded_assignment(
                        output,
                        "            ",
                        std::format("{} {} = ", return_type, managed_result_name),
                        converted_expression);
                    output.append_line_format("            return {} ;", managed_result_name);
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = default!;", instantiation->return_type.pinvoke_name, native_result_name);
                    output.append_line_format("            {} {} = default!;", return_type, managed_result_name);
                    output.append_line("            try");
                    output.append_line("            {");
                    output.append_line_format("                {} = {} ;", native_result_name, call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("                {}", post_assignment);
                    }
                    append_embedded_assignment(
                        output,
                        "                ",
                        std::format("{} = ", managed_result_name),
                        converted_expression);
                    output.append_line_format("                return {} ;", managed_result_name);
                    output.append_line("            }");
                    output.append_line("            finally");
                    output.append_line("            {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(output, "                ", finalize_statement);
                    }
                    if (has_return_finalize)
                    {
                        append_embedded_statement(output, "                ", render_inline_template(
                            instantiation->return_type.managed_finalize_from_pinvoke_statement, managed_result_name,
                            native_result_name, native_result_name, module_decl.name));
                    }
                    output.append_line("            }");
                }
            }
        }
        output.append_line("        }");
    }

    output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
    output.append_line("    }");
    output.append_line();
}

void append_generic_class_wrapper(TextWriter& output, const ModuleDecl& module_decl, const GenericClassGroup& class_group)
{
    if (class_group.instantiations.empty())
    {
        return;
    }

    const auto ctor_groups = collect_generic_class_ctor_groups(class_group);
    const auto method_groups = collect_generic_class_method_groups(class_group);

    std::size_t class_generic_arity = 0;
    for (const auto& ctor_group : ctor_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(ctor_group);
        if (shape.supported)
        {
            class_generic_arity = std::max(class_generic_arity, shape.generic_arity);
        }
    }
    for (const auto& method_group : method_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(method_group);
        if (shape.supported)
        {
            class_generic_arity = std::max(class_generic_arity, shape.generic_arity);
        }
    }

    if (class_generic_arity == 0)
    {
        return;
    }

    const auto class_slot_types =
        collect_generic_class_slot_types(class_group, ctor_groups, method_groups, class_generic_arity);

    output.append_line_format("public sealed class {}<{}> : System.IDisposable", class_group.name,
        generic_type_parameter_list(class_generic_arity));
    output.append_line("{");
    output.append_line("    private System.IntPtr _handle;");
    output.append_line("    private bool _ownsHandle;");
    output.append_line();

    for (const auto& ctor_group : ctor_groups)
    {
        append_generic_class_constructor(output, class_group, module_decl, ctor_group, class_generic_arity, class_slot_types);
    }

    for (const auto& method_group : method_groups)
    {
        append_generic_class_method(output, module_decl, class_group, method_group, class_generic_arity, class_slot_types);
    }

    output.append_line("    ~" + class_group.name + "()");
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
    for (std::size_t inst_index = 0; inst_index < class_group.instantiations.size(); ++inst_index)
    {
        const auto* class_decl = class_group.instantiations[inst_index];
        std::string condition;
        for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
        {
            const std::string concrete_name = class_slot_types[inst_index][slot];
            if (concrete_name.empty())
            {
                continue;
            }

            if (!condition.empty())
            {
                condition += " && ";
            }
            condition += std::format("typeof({}) == typeof({})", generic_class_type_parameter_name(slot, class_generic_arity), concrete_name);
        }

        if (condition.empty())
        {
            condition = "true";
        }

        if (inst_index == 0)
        {
            output.append_line_format("            if ({})", condition);
        }
        else
        {
            output.append_line_format("            else if ({})", condition);
        }
        output.append_line("            {");
        if (class_has_owned_ctor(*class_decl))
        {
            output.append_line_format("                {}Native.{}_{}_destroy(_handle);", module_decl.name, module_decl.name, class_decl->name);
        }
        output.append_line("            }");
    }
    output.append_line("        }");
    output.append_line();
    output.append_line("        _handle = System.IntPtr.Zero;");
    output.append_line("        _ownsHandle = false;");
    output.append_line("    }");
    output.append_line();

    output.append_line("    public void Dispose()");
    output.append_line("    {");
    output.append_line("        ReleaseUnmanaged();");
    output.append_line("        System.GC.SuppressFinalize(this);");
    output.append_line("    }");
    output.append_line("}");
    output.append_line();
}

std::string interface_name_for_class(const ClassDecl& class_decl)
{
    return "I" + class_decl.name;
}

bool is_primary_base_for_any_class(const ModuleDecl& module_decl, const ClassDecl& candidate)
{
    for (const auto& class_decl : module_decl.classes)
    {
        if (class_decl.cpp_name == candidate.cpp_name)
        {
            continue;
        }

        const auto* primary_base = primary_base_class(module_decl, class_decl);
        if (primary_base != nullptr && primary_base->cpp_name == candidate.cpp_name)
        {
            return true;
        }
    }

    return false;
}

void append_interface_declaration(TextWriter& output, const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    output.append_line_format("public interface {}", interface_name_for_class(class_decl));
    output.append_line("{");
    for (const auto& method_decl : class_decl.methods)
    {
        if (!is_wrapper_visible_method(method_decl))
        {
            continue;
        }

        output.append_line_format(
            "    {} {}({});",
            wrapper_return_type(module_decl, method_decl),
            method_decl.name,
            parameter_list_without_self(method_decl));
    }
    output.append_line("}");
    output.append_line();
}

bool module_has_virtual_callbacks(const ModuleDecl& module_decl)
{
    for (const auto& class_decl : module_decl.classes)
    {
        const auto emitted_methods = collect_emitted_methods(module_decl, class_decl);
        const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);
        if (!virtual_methods.empty())
        {
            return true;
        }
    }

    return false;
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
    const std::string& exported_name, bool include_self,
    std::size_t parameter_count = std::numeric_limits<std::size_t>::max())
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

    const std::size_t count = parameter_count > function_decl.parameters.size()
        ? function_decl.parameters.size()
        : parameter_count;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        if (needs_separator)
        {
            output.append(", ");
        }
        output.append_format("{}{} {}", parameter_direct_byref_keyword(parameter), parameter.type.pinvoke_name,
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
    const std::string native_name =
        std::format("{}_{}_{}", module_name, class_decl.name, exported_symbol_name(method_decl));
    const std::string base_native_name = native_name + "__base";
    const std::string return_type = wrapper_return_type(module_decl, method_decl);

    append_csharp_attributes(output, "    ", method_decl.csharp_attributes);

    if (method_decl.is_generic_instantiation)
    {
        output.append_format("    private {} {}(", return_type, method_decl.name);
    }
    else if (method_decl.is_property_getter || method_decl.is_property_setter)
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
            "        {} |= {}.__csbind23_DerivedClassHasMethod(this, \"{}\", typeof({}), __csbind23_methodTypes{}, {});",
            virtual_mask_field_name(chunk),
            csharp_api_class_name(module_decl),
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
            output.append_format(", {}{} {}", parameter_direct_byref_keyword(parameter), parameter.type.pinvoke_name,
                parameter.name);
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
            output.append_format(", {}{} {}", parameter_direct_byref_keyword(parameter), parameter.type.pinvoke_name,
                parameter.name);
        }
        output.append_line(")");
        output.append_line("    {");
        output.append_line_format("        if (!__csbind23_TryGetInstance(self, out var instance))");
        output.append_line("        {");
        if (!is_cabi_void(method_decl.return_type.c_abi_name))
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
        if (is_cabi_void(method_decl.return_type.c_abi_name))
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
                    managed_args.push_back(parameter_signature_byref_keyword(parameter) + parameter.name);
                }
                else
                {
                    managed_args.push_back(parameter.name);
                }
            }
        }

        const std::string invoke_args = join_arguments(managed_args);

        if (is_cabi_void(method_decl.return_type.c_abi_name))
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
    const ClassDecl* base_class = primary_base_class(module_decl, class_decl);
    const bool has_base_class = base_class != nullptr;
    const bool emits_destroy = class_has_owned_ctor(class_decl);
    const auto emitted_methods = collect_emitted_methods(module_decl, class_decl);
    const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);
    const bool has_virtual_support = !virtual_methods.empty();

    std::vector<std::string> base_types;
    if (has_base_class)
    {
        base_types.push_back(base_class->name);
    }
    else
    {
        base_types.push_back("System.IDisposable");
    }
    for (const auto* secondary_base : secondary_base_classes(module_decl, class_decl))
    {
        base_types.push_back(interface_name_for_class(*secondary_base));
    }

    std::string base_clause;
    for (std::size_t index = 0; index < base_types.size(); ++index)
    {
        if (index > 0)
        {
            base_clause += ", ";
        }
        base_clause += base_types[index];
    }

    const std::string class_declaration_kind = has_virtual_support
        ? "partial class"
        : (is_primary_base_for_any_class(module_decl, class_decl) ? "class" : "sealed class");
    const std::string class_visibility = class_decl.is_generic_instantiation ? "internal" : "public";

    append_csharp_attributes(output, "", class_decl.csharp_attributes);
    output.append_line_format("{} {} {} : {}", class_visibility, class_declaration_kind, class_decl.name, base_clause);
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

    for (std::size_t index = 0; index < emitted_methods.size(); ++index)
    {
        const auto& method_decl = emitted_methods[index].method;
        if (method_decl.is_constructor)
        {
            continue;
        }

        std::size_t virtual_index = 0;
        bool is_virtual = false;
        for (std::size_t i = 0; i < virtual_methods.size(); ++i)
        {
            if (virtual_methods[i] == &emitted_methods[index].method)
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

    const auto generic_method_groups = collect_generic_method_groups(emitted_methods);
    for (const auto& group : generic_method_groups)
    {
        append_method_generic_dispatch_wrapper(output, module_decl, group);
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

    std::unordered_set<std::string> emitted_interfaces;
    for (const auto& class_decl : module_decl.classes)
    {
        for (const auto* secondary_base : secondary_base_classes(module_decl, class_decl))
        {
            if (!emitted_interfaces.insert(secondary_base->cpp_name).second)
            {
                continue;
            }

            TextWriter interface_file(512);
            interface_file.append_line("namespace CsBind23.Generated;");
            interface_file.append_line();
            append_interface_declaration(interface_file, module_decl, *secondary_base);
            generated_files.push_back(write_csharp_file(
                output_root,
                module_decl.name + "." + interface_name_for_class(*secondary_base) + ".g.cs",
                interface_file.str()));
        }
    }

    TextWriter generated(3072);

    generated.append_line("namespace CsBind23.Generated;");
    generated.append_line();
    generated.append_line_format("internal static class {}Native", module_decl.name);
    generated.append_line("{");

    for (const auto& function_decl : module_decl.functions)
    {
        append_native_signature(generated, function_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + exported_symbol_name(function_decl), false);

        const std::size_t defaults = free_function_default_count(module_decl, function_decl);
        for (std::size_t omitted = 1; omitted <= defaults; ++omitted)
        {
            append_native_signature(
                generated,
                function_decl,
                module_decl.pinvoke_library,
                module_decl.name + "_" + exported_symbol_name(function_decl) + std::format("__default_{}", omitted),
                false,
                function_decl.parameters.size() - omitted);
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        const auto emitted_methods = collect_emitted_methods(module_decl, class_decl);
        const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);

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
        }

        for (const auto& emitted_method : emitted_methods)
        {
            const auto& method_decl = emitted_method.method;
            append_native_signature(generated, method_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_" + exported_symbol_name(method_decl), true);

            if (!virtual_methods.empty() && method_decl.allow_override)
            {
                append_native_signature(generated, method_decl, module_decl.pinvoke_library,
                    module_decl.name + "_" + class_decl.name + "_" + exported_symbol_name(method_decl) + "__base", true);
            }
        }
    }

    generated.append_line("}");
    generated.append_line();

    for (const auto& enum_decl : module_decl.enums)
    {
        if (enum_decl.is_flags)
        {
            generated.append_line("[System.Flags]");
        }
        append_csharp_attributes(generated, "", enum_decl.csharp_attributes);
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
        if (class_decl.is_generic_instantiation)
        {
            generated.append_line("            (handle, ownsHandle) => handle,");
        }
        else
        {
            generated.append_line_format("            (handle, ownsHandle) => new {}(handle, ownsHandle),", class_decl.name);
        }
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
        if (class_decl.is_generic_instantiation)
        {
            generated.append_line("        return handle;");
        }
        else
        {
            generated.append_line_format("        return new {}(handle, ownsHandle);", class_decl.name);
        }
        generated.append_line("    }");
        generated.append_line();
    }

    generated.append_line("}");
    generated.append_line();

    generated.append_line_format("public static class {}", csharp_api_class_name(module_decl));
    generated.append_line("{");
    if (module_has_virtual_callbacks(module_decl))
    {
        generated.append_line("    internal static ulong __csbind23_DerivedClassHasMethod(");
        generated.append_line("        object instance,");
        generated.append_line("        string methodName,");
        generated.append_line("        System.Type classType,");
        generated.append_line("        System.Type[] methodTypes,");
        generated.append_line("        ulong derivedFlag)");
        generated.append_line("    {");
        generated.append_line("        var methodInfo = instance.GetType().GetMethod(");
        generated.append_line("            methodName,");
        generated.append_line("            System.Reflection.BindingFlags.Public |");
        generated.append_line("            System.Reflection.BindingFlags.NonPublic |");
        generated.append_line("            System.Reflection.BindingFlags.Instance,");
        generated.append_line("            null,");
        generated.append_line("            methodTypes,");
        generated.append_line("            null);");
        generated.append_line("        if (methodInfo == null)");
        generated.append_line("        {");
        generated.append_line("            return 0UL;");
        generated.append_line("        }");
        generated.append_line();
        generated.append_line("        bool hasDerivedMethod = methodInfo.IsVirtual");
        generated.append_line("            && !methodInfo.IsAbstract");
        generated.append_line("            && (methodInfo.DeclaringType!.IsSubclassOf(classType) || methodInfo.DeclaringType == classType)");
        generated.append_line("            && methodInfo.DeclaringType != methodInfo.GetBaseDefinition().DeclaringType;");
        generated.append_line("        return hasDerivedMethod ? derivedFlag : 0UL;");
        generated.append_line("    }");
        generated.append_line();
    }

    for (const auto& function_decl : module_decl.functions)
    {
        append_csharp_attributes(generated, "    ", function_decl.csharp_attributes);
        const std::string params = free_function_parameter_list(module_decl, function_decl);
        const std::string return_type = wrapper_return_type(module_decl, function_decl);
        const std::string native_name = module_decl.name + "_" + exported_symbol_name(function_decl);
        const std::size_t defaults = free_function_default_count(module_decl, function_decl);
        const std::size_t optional_start = function_decl.parameters.size() - defaults;
        const std::string method_visibility = function_decl.is_generic_instantiation ? "private" : "public";

        generated.append_format("    {} static {} {}(", method_visibility, return_type, function_decl.name);
        generated.append(params);
        generated.append_line(")");
        generated.append_line("    {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        call_arguments.reserve(function_decl.parameters.size());
        for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
        {
            const auto& parameter = function_decl.parameters[index];
            const std::string managed_argument_expr =
                nullable_parameter_unwrap_expression(module_decl, function_decl, parameter, index);
            if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                append_embedded_assignment(
                    generated,
                    "        ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    render_inline_template(parameter.type.managed_to_pinvoke_expression, managed_argument_expr, pinvoke_name,
                        managed_argument_expr, module_decl.name));
                call_arguments.push_back(pinvoke_name);
                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, managed_argument_expr, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
                continue;
            }

            call_arguments.push_back(parameter_call_argument(parameter, managed_argument_expr));
        }

        std::string native_call;
        std::vector<std::string> default_variant_has_value_expressions;
        if (defaults == 0)
        {
            const std::string call_arguments_rendered = join_arguments(call_arguments);
            native_call = std::format("{}Native.{}({})", module_decl.name, native_name, call_arguments_rendered);
        }
        else
        {
            default_variant_has_value_expressions.reserve(defaults);
            for (std::size_t index = optional_start; index < function_decl.parameters.size(); ++index)
            {
                default_variant_has_value_expressions.push_back(optional_parameter_has_value_expression(
                    module_decl, function_decl, function_decl.parameters[index], index));
            }
        }
        const bool has_return_converter = !function_decl.return_type.managed_from_pinvoke_expression.empty();
        const bool has_return_finalize = !function_decl.return_type.managed_finalize_from_pinvoke_statement.empty();
        const bool needs_finally = !finalize_statements.empty() || has_return_finalize;
        const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, function_decl);

        if (return_type == "void")
        {
            if (!needs_finally)
            {
                if (defaults == 0)
                {
                    generated.append_line_format("        {};", native_call);
                }
                else
                {
                    append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                        call_arguments, defaults, default_variant_has_value_expressions, "", false);
                }
            }
            else
            {
                generated.append_line("        try");
                generated.append_line("        {");
                if (defaults == 0)
                {
                    generated.append_line_format("            {};", native_call);
                }
                else
                {
                    append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                        call_arguments, defaults, default_variant_has_value_expressions, "", false);
                }
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
                    if (defaults == 0)
                    {
                        generated.append_line_format("        System.IntPtr {} = {};", native_result_name, native_call);
                    }
                    else
                    {
                        generated.append_line_format("        System.IntPtr {} = default!;", native_result_name);
                        append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
                    generated.append_line_format("        return {};", wrapped_result);
                }
                else
                {
                    generated.append_line("        try");
                    generated.append_line("        {");
                    if (defaults == 0)
                    {
                        generated.append_line_format("            System.IntPtr {} = {};", native_result_name, native_call);
                    }
                    else
                    {
                        generated.append_line_format("            System.IntPtr {} = default!;", native_result_name);
                        append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
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
                    if (defaults == 0)
                    {
                        generated.append_line_format("        return {};", native_call);
                    }
                    else
                    {
                        append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, "", true);
                    }
                }
                else
                {
                    generated.append_line("        try");
                    generated.append_line("        {");
                    if (defaults == 0)
                    {
                        generated.append_line_format("            return {};", native_call);
                    }
                    else
                    {
                        append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, "", true);
                    }
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
                    if (defaults == 0)
                    {
                        generated.append_line_format(
                            "        {} {} = {};", function_decl.return_type.pinvoke_name, native_result_name, native_call);
                    }
                    else
                    {
                        generated.append_line_format(
                            "        {} {} = default!;", function_decl.return_type.pinvoke_name, native_result_name);
                        append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
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
                    if (defaults == 0)
                    {
                        generated.append_line_format("            {} = {};", native_result_name, native_call);
                    }
                    else
                    {
                        append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
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

    const auto generic_function_groups = collect_generic_function_groups(module_decl.functions);
    for (const auto& group : generic_function_groups)
    {
        append_free_generic_dispatch_wrapper(generated, module_decl, group);
    }
    generated.append_line("}");
    generated.append_line();

    const auto& written = generated.str();
    generated_files.push_back(write_csharp_file(output_root, module_decl.name + ".g.cs", written));

    for (const auto& class_decl : module_decl.classes)
    {
        if (class_decl.is_generic_instantiation)
        {
            continue;
        }

        TextWriter class_file(2048);
        class_file.append_line("namespace CsBind23.Generated;");
        class_file.append_line();
        append_wrapper_class(class_file, module_decl, module_decl.name, class_decl);

        const auto& class_written = class_file.str();
        generated_files.push_back(
            write_csharp_file(output_root, module_decl.name + "." + class_decl.name + ".g.cs", class_written));
    }

    const auto generic_class_groups = collect_generic_class_groups(module_decl.classes);
    for (const auto& generic_class_group : generic_class_groups)
    {
        TextWriter generic_class_file(2048);
        generic_class_file.append_line("namespace CsBind23.Generated;");
        generic_class_file.append_line();
        append_generic_class_wrapper(generic_class_file, module_decl, generic_class_group);

        const auto& generic_class_written = generic_class_file.str();
        generated_files.push_back(write_csharp_file(
            output_root,
            module_decl.name + "." + generic_class_group.name + ".g.cs",
            generic_class_written));
    }

    return generated_files;
}

} // namespace csbind23::emit
