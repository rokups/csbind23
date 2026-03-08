#pragma once

#include <string>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace csbind23
{

struct CallableSignatureDecl;

inline constexpr std::string_view stl_module_name()
{
    return "csbind23_stl";
}

inline constexpr std::string_view default_stl_csharp_namespace()
{
    return "Std";
}

inline std::string make_safe_csharp_namespace_segment(std::string_view text)
{
    std::string result;
    result.reserve(text.size() + 8);

    for (char ch : text)
    {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')
        {
            result.push_back(ch);
        }
        else
        {
            result.push_back('_');
        }
    }

    if (result.empty())
    {
        result = "module";
    }

    if (result.front() >= '0' && result.front() <= '9')
    {
        result.insert(result.begin(), '_');
    }

    if (result == "string" || result == "namespace" || result == "class" || result == "public"
        || result == "internal" || result == "private")
    {
        result += "_module";
    }

    return result;
}

inline std::string replace_all(std::string value, std::string_view from, std::string_view to)
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

enum class CSharpNameKind
{
    Class,
    Function,
    Method,
    Property,
    MemberVar,
};

enum class Ownership
{
    Auto,
    Borrowed,
    Owned,
};

struct TypeRef
{
    std::string cpp_name;
    std::string cpp_full_type;
    std::string c_abi_name;
    std::string pinvoke_name;
    std::string managed_type_name;
    std::string managed_array_element_type_name;
    std::string managed_to_pinvoke_expression;
    std::string managed_from_pinvoke_expression;
    std::string managed_finalize_to_pinvoke_statement;
    std::string managed_finalize_from_pinvoke_statement;
    std::string callable_id;
    std::size_t managed_array_fixed_extent = std::numeric_limits<std::size_t>::max();
    bool is_const = false;
    bool is_pointer = false;
    bool is_reference = false;
    bool is_function_pointer = false;
    bool is_std_function = false;
    std::shared_ptr<CallableSignatureDecl> callable_signature;

    bool has_fixed_managed_array_extent() const
    {
        return managed_array_fixed_extent != std::numeric_limits<std::size_t>::max();
    }

    bool has_managed_converter() const
    {
        return callable_signature != nullptr || !managed_type_name.empty() || !managed_to_pinvoke_expression.empty()
            || !managed_from_pinvoke_expression.empty() || !managed_finalize_to_pinvoke_statement.empty()
            || !managed_finalize_from_pinvoke_statement.empty();
    }
};

struct CallableSignatureDecl
{
    TypeRef return_type;
    std::vector<TypeRef> parameter_types;
};

struct ParameterDecl
{
    std::string name;
    TypeRef type;
    bool is_output = false;
    bool is_c_array = false;
    std::size_t c_array_size_param_index = std::numeric_limits<std::size_t>::max();
};

struct FunctionDecl
{
    std::string name;
    std::string exported_name;
    std::string cpp_symbol;
    TypeRef return_type;
    std::vector<ParameterDecl> parameters;
    std::size_t trailing_default_argument_count = 0;
    Ownership return_ownership = Ownership::Auto;
    bool is_generic_instantiation = false;
    std::string generic_group_name;
    bool pinvoke_only = false;
    bool csharp_private = false;
    bool is_extension_method = false;

    bool is_constructor = false;
    bool is_method = false;
    bool is_const = false;
    bool is_virtual = false;
    bool is_pure_virtual = false;
    bool is_final = false;
    bool allow_override = false;
    bool is_property_getter = false;
    bool is_property_setter = false;
    bool is_field_accessor = false;
    std::string class_name;
    std::string virtual_slot_name;
    std::vector<std::string> csharp_attributes;
    std::string csharp_comment;
};

struct PropertyDecl
{
    std::string name;
    std::string getter_name;
    std::string setter_name;
    TypeRef type;
    bool has_getter = false;
    bool has_setter = false;
    bool is_field_projection = false;
    std::string csharp_comment;
};

inline Ownership infer_ownership(const FunctionDecl& function_decl)
{
    if (function_decl.return_ownership != Ownership::Auto)
    {
        return function_decl.return_ownership;
    }

    if (function_decl.is_constructor)
    {
        return Ownership::Owned;
    }

    if (function_decl.return_type.is_pointer || function_decl.return_type.is_reference)
    {
        return Ownership::Borrowed;
    }

    if (function_decl.return_type.c_abi_name == "void*" || function_decl.return_type.c_abi_name == "const void*")
    {
        return Ownership::Borrowed;
    }

    if (function_decl.return_type.cpp_name == "void" && !function_decl.return_type.is_pointer
        && !function_decl.return_type.is_reference)
    {
        return Ownership::Borrowed;
    }

    return Ownership::Owned;
}

struct ClassDecl
{
    struct BaseClassDecl
    {
        std::string name;
        std::string cpp_name;
    };

    std::string name;
    std::string cpp_name;
    std::string base_name;
    std::string base_cpp_name;
    std::vector<BaseClassDecl> base_classes;
    bool enable_virtual_overrides = false;
    bool is_generic_instantiation = false;
    std::string generic_group_name;
    std::string csharp_namespace;
    std::string instance_cache_type;
    std::vector<std::string> csharp_attributes;
    std::string csharp_comment;
    std::vector<std::string> csharp_interfaces;
    std::vector<std::string> csharp_member_snippets;
    std::vector<FunctionDecl> methods;
    std::vector<PropertyDecl> properties;
};

struct EnumValueDecl
{
    std::string name;
    std::uint64_t value = 0;
    bool is_signed = true;
};

struct EnumDecl
{
    std::string name;
    std::string cpp_name;
    TypeRef underlying_type;
    bool is_flags = false;
    std::vector<std::string> csharp_attributes;
    std::vector<EnumValueDecl> values;
};

struct ModuleDecl
{
    std::string name;
    std::string csharp_api_class;
    std::string csharp_namespace;
    std::function<std::string(CSharpNameKind, std::string_view)> csharp_name_formatter;
    std::string pinvoke_library;
    std::string instance_cache_type = "DefaultInstanceCache<T>";
    std::vector<std::string> imported_modules;
    std::vector<std::string> cabi_includes;
    std::vector<FunctionDecl> functions;
    std::vector<ClassDecl> classes;
    std::vector<EnumDecl> enums;
};

} // namespace csbind23
