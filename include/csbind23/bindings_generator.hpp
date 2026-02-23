#pragma once

#include "csbind23/ir.hpp"
#include "csbind23/type_utils.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace csbind23
{

class ModuleBuilder;
class ClassBuilder;

class BindingsGenerator
{
public:
    std::string name() const;

    ModuleBuilder module(std::string_view name);

    const std::vector<ModuleDecl>& modules() const;

    std::vector<std::filesystem::path> generate_cabi(const std::filesystem::path& output_root) const;
    std::vector<std::filesystem::path> generate_csharp(const std::filesystem::path& output_root) const;

private:
    ModuleDecl& upsert_module(std::string_view name);

    std::vector<ModuleDecl> modules_;
};

class ModuleBuilder
{
public:
    ModuleBuilder(BindingsGenerator& owner, ModuleDecl& module_decl);

    ModuleBuilder& pinvoke_library(std::string_view library_name)
    {
        module_decl_->pinvoke_library = std::string(library_name);
        return *this;
    }

    ModuleBuilder& cabi_include(std::string_view include_path)
    {
        module_decl_->cabi_includes.push_back(std::string(include_path));
        return *this;
    }

    template <auto Function> ModuleBuilder& def(Ownership return_ownership = Ownership::Auto)
    {
        return def_from_constant(detail::function_export_name<Function>(), Function, return_ownership,
            detail::function_symbol_name<Function>());
    }

    template <auto Function> ModuleBuilder& def(std::string_view name, Ownership return_ownership = Ownership::Auto)
    {
        return def_from_constant(name, Function, return_ownership, detail::function_symbol_name<Function>());
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def(std::string_view name, ReturnType (*function_ptr)(Args...))
    {
        return def_impl(name, function_ptr, Ownership::Auto, {});
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def(std::string_view name, ReturnType (*function_ptr)(Args...), std::string_view cpp_symbol)
    {
        return def_impl(name, function_ptr, Ownership::Auto, cpp_symbol);
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def(std::string_view name, ReturnType (*function_ptr)(Args...), Ownership return_ownership)
    {
        return def_impl(name, function_ptr, return_ownership, {});
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def(std::string_view name, ReturnType (*function_ptr)(Args...), Ownership return_ownership,
        std::string_view cpp_symbol)
    {
        return def_impl(name, function_ptr, return_ownership, cpp_symbol);
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def_impl(std::string_view name, ReturnType (*function_ptr)(Args...), Ownership return_ownership,
        std::string_view cpp_symbol)
    {
        (void)function_ptr;

        FunctionDecl function_decl;
        function_decl.name = std::string(name);
        function_decl.cpp_symbol = cpp_symbol.empty() ? std::string(name) : std::string(cpp_symbol);
        function_decl.return_type = detail::make_type_ref<ReturnType>();
        function_decl.return_ownership = return_ownership;

        function_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((function_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), detail::make_type_ref<Args>()})),
            ...);

        module_decl_->functions.push_back(std::move(function_decl));
        return *this;
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def_from_constant(std::string_view name, ReturnType (*function_ptr)(Args...),
        Ownership return_ownership, std::string_view cpp_symbol)
    {
        return def_impl(name, function_ptr, return_ownership, cpp_symbol);
    }

    template <typename ClassType>
    ClassBuilder class_(std::string_view name = detail::unqualified_type_name<ClassType>(),
        std::string_view cpp_name = detail::qualified_type_name<ClassType>());

private:
    BindingsGenerator* owner_;
    ModuleDecl* module_decl_;
};

class ClassBuilder
{
public:
    explicit ClassBuilder(ClassDecl& class_decl);

    template <auto MethodPtr> ClassBuilder& def(Ownership return_ownership = Ownership::Auto)
    {
        return def(detail::function_export_name<MethodPtr>(), MethodPtr, return_ownership);
    }

    template <auto MethodPtr> ClassBuilder& def(std::string_view name, Ownership return_ownership = Ownership::Auto)
    {
        return def(name, MethodPtr, return_ownership);
    }

    template <typename... Args> ClassBuilder& ctor(Ownership ownership = Ownership::Auto)
    {
        FunctionDecl ctor_decl;
        ctor_decl.name = "__ctor";
        ctor_decl.cpp_symbol = class_decl_->cpp_name;
        ctor_decl.return_type = detail::make_type_ref<void*>();
        ctor_decl.return_ownership = ownership;
        ctor_decl.is_constructor = true;
        ctor_decl.class_name = class_decl_->cpp_name;

        ctor_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((ctor_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), detail::make_type_ref<Args>()})),
            ...);

        class_decl_->methods.push_back(std::move(ctor_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    ClassBuilder& def(std::string_view name, ReturnType (ClassType::*method_ptr)(Args...),
        Ownership return_ownership = Ownership::Auto)
    {
        (void)method_ptr;
        add_method<ClassType, ReturnType, Args...>(name, false, return_ownership);
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    ClassBuilder& def(std::string_view name, ReturnType (ClassType::*method_ptr)(Args...) const,
        Ownership return_ownership = Ownership::Auto)
    {
        (void)method_ptr;
        add_method<ClassType, ReturnType, Args...>(name, true, return_ownership);
        return *this;
    }

private:
    template <typename ClassType, typename ReturnType, typename... Args>
    void add_method(std::string_view name, bool is_const, Ownership return_ownership)
    {
        FunctionDecl method_decl;
        method_decl.name = std::string(name);
        method_decl.cpp_symbol = std::string(name);
        method_decl.return_type = detail::make_type_ref<ReturnType>();
        method_decl.return_ownership = return_ownership;
        method_decl.is_method = true;
        method_decl.is_const = is_const;
        method_decl.class_name = class_decl_->cpp_name;

        method_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((method_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), detail::make_type_ref<Args>()})),
            ...);

        class_decl_->methods.push_back(std::move(method_decl));
    }

    ClassDecl* class_decl_;
};

template <typename ClassType> ClassBuilder ModuleBuilder::class_(std::string_view name, std::string_view cpp_name)
{
    ClassDecl class_decl;
    class_decl.name = std::string(name);
    class_decl.cpp_name = std::string(cpp_name);
    module_decl_->classes.push_back(std::move(class_decl));
    return ClassBuilder(module_decl_->classes.back());
}

} // namespace csbind23
