#pragma once

#include <optional>
#include <string>
#include <vector>

namespace csbind23
{

enum class Ownership
{
    Auto,
    Borrowed,
    Owned,
};

struct ManagedInlineConverter
{
    std::string managed_type_name;
    std::string to_pinvoke_expression;
    std::string from_pinvoke_expression;
    std::string finalize_to_pinvoke_statement;
    std::string finalize_from_pinvoke_statement;
};

struct TypeRef
{
    std::string cpp_name;
    std::string c_abi_name;
    std::string pinvoke_name;
    std::optional<ManagedInlineConverter> managed_converter;
    bool is_const = false;
    bool is_pointer = false;
    bool is_reference = false;
};

struct ParameterDecl
{
    std::string name;
    TypeRef type;
};

struct FunctionDecl
{
    std::string name;
    std::string cpp_symbol;
    TypeRef return_type;
    std::vector<ParameterDecl> parameters;
    Ownership return_ownership = Ownership::Auto;

    bool is_constructor = false;
    bool is_method = false;
    bool is_const = false;
    std::string class_name;
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
    std::string name;
    std::string cpp_name;
    std::vector<FunctionDecl> methods;
};

struct ModuleDecl
{
    std::string name;
    std::string pinvoke_library;
    std::vector<std::string> cabi_includes;
    std::vector<FunctionDecl> functions;
    std::vector<ClassDecl> classes;
};

} // namespace csbind23
