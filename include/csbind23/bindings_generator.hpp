#pragma once

#include "csbind23/ir.hpp"
#include "csbind23/type_utils.hpp"

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace csbind23
{

class ModuleBuilder;
class ClassBuilder;
class EnumBuilder;

struct WithDefaults
{
    std::size_t trailing_default_argument_count;
};

struct Virtual
{
};

struct CppSymbol
{
    std::string_view value;
};

struct CppName
{
    std::string_view value;
};

struct Flags
{
};

struct Attribute
{
    std::string_view value;
};

class BindingsGenerator
{
public:
    std::string name() const;

    ModuleBuilder module(std::string_view name);

    const std::vector<ModuleDecl>& modules() const;

    std::vector<std::filesystem::path> generate_cabi(const std::filesystem::path& output_root) const;
    std::vector<std::filesystem::path> generate_csharp(const std::filesystem::path& output_root) const;

    template <typename Type> TypeRef make_bound_param_type_ref() const
    {
        TypeRef type_ref = detail::make_param_type_ref<Type>();
        apply_managed_converter<Type>(type_ref);
        return type_ref;
    }

    template <typename Type> TypeRef make_bound_return_type_ref() const
    {
        TypeRef type_ref = detail::make_return_type_ref<Type>();
        apply_managed_converter<Type>(type_ref);
        return type_ref;
    }

    template <typename Type> TypeRef make_bound_type_ref() const
    {
        return make_bound_param_type_ref<Type>();
    }

private:
    template <typename Type> static void assign_managed_converter(TypeRef& type_ref)
    {
        type_ref.managed_type_name = std::string(cabi::detail::managed_type_name_for<Type>());
        type_ref.managed_to_pinvoke_expression =
            std::string(cabi::detail::managed_to_pinvoke_expression_for<Type>());
        type_ref.managed_from_pinvoke_expression =
            std::string(cabi::detail::managed_from_pinvoke_expression_for<Type>());
        type_ref.managed_finalize_to_pinvoke_statement =
            std::string(cabi::detail::managed_finalize_to_pinvoke_statement_for<Type>());
        type_ref.managed_finalize_from_pinvoke_statement =
            std::string(cabi::detail::managed_finalize_from_pinvoke_statement_for<Type>());
    }

    template <typename Type> void apply_managed_converter(TypeRef& type_ref) const
    {
        assign_managed_converter<Type>(type_ref);
        if (type_ref.has_managed_converter())
        {
            return;
        }

        using NoRef = std::remove_reference_t<Type>;
        using Bare = std::remove_cv_t<NoRef>;
        if constexpr (!std::is_same_v<Bare, Type>)
        {
            assign_managed_converter<Bare>(type_ref);
            if (type_ref.has_managed_converter())
            {
                return;
            }
        }

        using Base = std::remove_cv_t<std::remove_pointer_t<Bare>>;
        if constexpr (!std::is_same_v<Base, Bare>)
        {
            assign_managed_converter<Base>(type_ref);
        }

        using EnumBase = std::remove_cv_t<std::remove_pointer_t<NoRef>>;
        if constexpr (std::is_enum_v<EnumBase>)
        {
            if (type_ref.managed_type_name.empty())
            {
                type_ref.managed_type_name = detail::unqualified_type_name<EnumBase>();
            }
            type_ref.pinvoke_name = type_ref.managed_type_name;
        }
    }

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

    ModuleBuilder& csharp_api_class(std::string_view class_name)
    {
        module_decl_->csharp_api_class = std::string(class_name);
        return *this;
    }

    ModuleBuilder& cabi_include(std::string_view include_path)
    {
        module_decl_->cabi_includes.push_back(std::string(include_path));
        return *this;
    }

    template <auto Function> ModuleBuilder& def()
    {
        return def<Function>(detail::function_export_name<Function>());
    }

    template <auto Function, typename FirstOption, typename... RestOptions>
        requires(!std::is_convertible_v<std::decay_t<FirstOption>, std::string_view>)
    ModuleBuilder& def(FirstOption&& first_option, RestOptions&&... rest_options)
    {
        return def<Function>(detail::function_export_name<Function>(), std::forward<FirstOption>(first_option),
            std::forward<RestOptions>(rest_options)...);
    }

    template <auto Function, typename... Options>
    ModuleBuilder& def(std::string_view name, Options&&... options)
    {
        const auto def_options = make_function_def_options(std::forward<Options>(options)...);
        const std::string cpp_symbol = def_options.cpp_symbol.empty()
            ? std::string(detail::function_symbol_name<Function>())
            : std::string(def_options.cpp_symbol);
        return def_impl(name, Function, def_options.return_ownership, def_options.trailing_default_argument_count,
            cpp_symbol, def_options.csharp_attributes);
    }

    template <typename ReturnType, typename... Args, typename... Options>
    ModuleBuilder& def(std::string_view name, ReturnType (*function_ptr)(Args...), Options&&... options)
    {
        const auto def_options = make_function_def_options(std::forward<Options>(options)...);
        return def_impl(name, function_ptr, def_options.return_ownership, def_options.trailing_default_argument_count,
            def_options.cpp_symbol, def_options.csharp_attributes);
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def_impl(std::string_view name, ReturnType (*function_ptr)(Args...), Ownership return_ownership,
        std::size_t trailing_default_argument_count, std::string_view cpp_symbol,
        std::vector<std::string> csharp_attributes = {})
    {
        (void)function_ptr;

        FunctionDecl function_decl;
        function_decl.name = std::string(name);
        function_decl.cpp_symbol = cpp_symbol.empty() ? std::string(name) : std::string(cpp_symbol);
        function_decl.return_type = owner_->make_bound_return_type_ref<ReturnType>();
        function_decl.return_ownership = return_ownership;
        function_decl.trailing_default_argument_count =
            trailing_default_argument_count > sizeof...(Args) ? sizeof...(Args) : trailing_default_argument_count;
        function_decl.csharp_attributes = std::move(csharp_attributes);

        function_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((function_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>()})),
            ...);

        module_decl_->functions.push_back(std::move(function_decl));
        return *this;
    }

    template <typename ClassType, typename... BaseTypes>
    ClassBuilder class_();

    template <typename ClassType, typename... BaseTypes, typename... Options>
    ClassBuilder class_(std::string_view name, Options&&... options);

    template <typename EnumType>
    EnumBuilder enum_();

    template <typename EnumType, typename... Options>
    EnumBuilder enum_(std::string_view name, Options&&... options);

private:
    struct FunctionDefOptions
    {
        Ownership return_ownership = Ownership::Auto;
        std::size_t trailing_default_argument_count = 0;
        std::string_view cpp_symbol = {};
        std::vector<std::string> csharp_attributes;
    };

    struct ClassOptions
    {
        std::string_view cpp_name = {};
        std::vector<std::string> csharp_attributes;
    };

    struct EnumOptions
    {
        std::string_view cpp_name = {};
        bool is_flags = false;
        std::vector<std::string> csharp_attributes;
    };

    template <typename Option> struct dependent_false : std::false_type
    {
    };

    template <typename... Options> static FunctionDefOptions make_function_def_options(Options&&... options)
    {
        FunctionDefOptions parsed_options;
        (apply_function_def_option(parsed_options, std::forward<Options>(options)), ...);
        return parsed_options;
    }

    static void apply_function_def_option(FunctionDefOptions& options, Ownership return_ownership)
    {
        options.return_ownership = return_ownership;
    }

    static void apply_function_def_option(FunctionDefOptions& options, WithDefaults with_defaults)
    {
        options.trailing_default_argument_count = with_defaults.trailing_default_argument_count;
    }

    static void apply_function_def_option(FunctionDefOptions& options, CppSymbol cpp_symbol)
    {
        options.cpp_symbol = cpp_symbol.value;
    }

    static void apply_function_def_option(FunctionDefOptions& options, Attribute attribute)
    {
        options.csharp_attributes.push_back(std::string(attribute.value));
    }

    template <typename Option> static void apply_function_def_option(FunctionDefOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ModuleBuilder::def option. Supported tags: Ownership, WithDefaults, CppSymbol, Attribute.");
    }

    template <typename... Options> static ClassOptions make_class_options(Options&&... options)
    {
        ClassOptions parsed_options;
        (apply_class_option(parsed_options, std::forward<Options>(options)), ...);
        return parsed_options;
    }

    static void apply_class_option(ClassOptions& options, CppName cpp_name)
    {
        options.cpp_name = cpp_name.value;
    }

    static void apply_class_option(ClassOptions& options, Attribute attribute)
    {
        options.csharp_attributes.push_back(std::string(attribute.value));
    }

    template <typename Option> static void apply_class_option(ClassOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ModuleBuilder::class_ option. Supported tags: CppName, Attribute.");
    }

    template <typename... Options> static EnumOptions make_enum_options(Options&&... options)
    {
        EnumOptions parsed_options;
        (apply_enum_option(parsed_options, std::forward<Options>(options)), ...);
        return parsed_options;
    }

    static void apply_enum_option(EnumOptions& options, CppName cpp_name)
    {
        options.cpp_name = cpp_name.value;
    }

    static void apply_enum_option(EnumOptions& options, Flags)
    {
        options.is_flags = true;
    }

    static void apply_enum_option(EnumOptions& options, Attribute attribute)
    {
        options.csharp_attributes.push_back(std::string(attribute.value));
    }

    template <typename Option> static void apply_enum_option(EnumOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ModuleBuilder::enum_ option. Supported tags: CppName, Flags, Attribute.");
    }

    BindingsGenerator* owner_;
    ModuleDecl* module_decl_;
};

class ClassBuilder
{
public:
    ClassBuilder(BindingsGenerator& owner, ClassDecl& class_decl);

    ClassBuilder& enable_virtual_overrides(bool enabled = true)
    {
        class_decl_->enable_virtual_overrides = enabled;
        return *this;
    }

    template <auto MethodPtr> ClassBuilder& def()
    {
        return def<MethodPtr>(detail::function_export_name<MethodPtr>());
    }

    template <auto MethodPtr, typename FirstOption, typename... RestOptions>
        requires(!std::is_convertible_v<std::decay_t<FirstOption>, std::string_view>)
    ClassBuilder& def(FirstOption&& first_option, RestOptions&&... rest_options)
    {
        return def<MethodPtr>(detail::function_export_name<MethodPtr>(), std::forward<FirstOption>(first_option),
            std::forward<RestOptions>(rest_options)...);
    }

    template <auto MethodPtr, typename... Options>
    ClassBuilder& def(std::string_view name, Options&&... options)
    {
        return def(name, MethodPtr, std::forward<Options>(options)...);
    }

    template <typename... Args> ClassBuilder& ctor(Ownership ownership = Ownership::Auto)
    {
        FunctionDecl ctor_decl;
        ctor_decl.name = "__ctor";
        ctor_decl.cpp_symbol = class_decl_->cpp_name;
        ctor_decl.return_type = owner_->make_bound_return_type_ref<void*>();
        ctor_decl.return_ownership = ownership;
        ctor_decl.is_constructor = true;
        ctor_decl.class_name = class_decl_->cpp_name;

        ctor_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((ctor_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>()})),
            ...);

        class_decl_->methods.push_back(std::move(ctor_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename... Args, typename... Options>
    ClassBuilder& def(std::string_view name, ReturnType (ClassType::*method_ptr)(Args...), Options&&... options)
    {
        (void)method_ptr;
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        if (def_options.allow_override)
        {
            class_decl_->enable_virtual_overrides = true;
        }
        add_method<ClassType, ReturnType, Args...>(name, false, def_options.return_ownership,
            def_options.trailing_default_argument_count, def_options.allow_override, false, false,
            def_options.cpp_symbol, def_options.csharp_attributes);
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename... Args, typename... Options>
    ClassBuilder& def(
        std::string_view name, ReturnType (ClassType::*method_ptr)(Args...) const, Options&&... options)
    {
        (void)method_ptr;
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        if (def_options.allow_override)
        {
            class_decl_->enable_virtual_overrides = true;
        }
        add_method<ClassType, ReturnType, Args...>(name, true, def_options.return_ownership,
            def_options.trailing_default_argument_count, def_options.allow_override, false, false,
            def_options.cpp_symbol, def_options.csharp_attributes);
        return *this;
    }

    template <auto GetterPtr> ClassBuilder& property(std::string_view name)
    {
        return property(name, GetterPtr, detail::function_symbol_name<GetterPtr>());
    }

    template <auto GetterPtr, auto SetterPtr> ClassBuilder& property(std::string_view name)
    {
        return property(name, GetterPtr, SetterPtr, detail::function_symbol_name<GetterPtr>(),
            detail::function_symbol_name<SetterPtr>());
    }

    template <typename ClassType, typename ReturnType>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)(),
        std::string_view getter_cpp_symbol)
    {
        (void)getter_ptr;
        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>();
        property_decl.has_getter = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);

        add_method<ClassType, ReturnType>(property_decl.getter_name, false, Ownership::Auto, 0, false,
            true, false, getter_cpp_symbol);

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)() const,
        std::string_view getter_cpp_symbol)
    {
        (void)getter_ptr;
        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>();
        property_decl.has_getter = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);

        add_method<ClassType, ReturnType>(property_decl.getter_name, true, Ownership::Auto, 0, false,
            true, false, getter_cpp_symbol);

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)())
    {
        (void)getter_ptr;
        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>();
        property_decl.has_getter = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);

        add_method<ClassType, ReturnType>(property_decl.getter_name, false, Ownership::Auto, 0, false,
            true, false, std::string(name));

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)() const)
    {
        (void)getter_ptr;
        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>();
        property_decl.has_getter = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);

        add_method<ClassType, ReturnType>(property_decl.getter_name, true, Ownership::Auto, 0, false,
            true, false, std::string(name));

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename SetterArg>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)() const,
        void (ClassType::*setter_ptr)(SetterArg))
    {
        (void)getter_ptr;
        (void)setter_ptr;

        return property(name, getter_ptr, setter_ptr, std::string(name), std::string(name));
    }

    template <typename ClassType, typename ReturnType, typename SetterArg>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)(),
        void (ClassType::*setter_ptr)(SetterArg))
    {
        (void)getter_ptr;
        (void)setter_ptr;

        return property(name, getter_ptr, setter_ptr, std::string(name), std::string(name));
    }

    template <typename ClassType, typename ReturnType, typename SetterArg>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)() const,
        void (ClassType::*setter_ptr)(SetterArg), std::string_view getter_cpp_symbol,
        std::string_view setter_cpp_symbol)
    {
        (void)getter_ptr;
        (void)setter_ptr;

        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>();
        property_decl.has_getter = true;
        property_decl.has_setter = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);
        property_decl.setter_name = std::string("__csbind23_propset_") + std::string(name);

        add_method<ClassType, ReturnType>(property_decl.getter_name, true, Ownership::Auto, 0, false,
            true, false, getter_cpp_symbol);
        add_method<ClassType, void, SetterArg>(property_decl.setter_name, false, Ownership::Auto, 0, false,
            false, true, setter_cpp_symbol);

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename SetterArg>
    ClassBuilder& property(std::string_view name, ReturnType (ClassType::*getter_ptr)(),
        void (ClassType::*setter_ptr)(SetterArg), std::string_view getter_cpp_symbol,
        std::string_view setter_cpp_symbol)
    {
        (void)getter_ptr;
        (void)setter_ptr;

        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>();
        property_decl.has_getter = true;
        property_decl.has_setter = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);
        property_decl.setter_name = std::string("__csbind23_propset_") + std::string(name);

        add_method<ClassType, ReturnType>(property_decl.getter_name, false, Ownership::Auto, 0, false,
            true, false, getter_cpp_symbol);
        add_method<ClassType, void, SetterArg>(property_decl.setter_name, false, Ownership::Auto, 0, false,
            false, true, setter_cpp_symbol);

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
    }

private:
    struct MethodDefOptions
    {
        Ownership return_ownership = Ownership::Auto;
        std::size_t trailing_default_argument_count = 0;
        bool allow_override = false;
        std::string_view cpp_symbol = {};
        std::vector<std::string> csharp_attributes;
    };

    template <typename Option> struct dependent_false : std::false_type
    {
    };

    template <typename... Options> static MethodDefOptions make_method_def_options(Options&&... options)
    {
        MethodDefOptions parsed_options;
        (apply_method_def_option(parsed_options, std::forward<Options>(options)), ...);
        return parsed_options;
    }

    static void apply_method_def_option(MethodDefOptions& options, Ownership return_ownership)
    {
        options.return_ownership = return_ownership;
    }

    static void apply_method_def_option(MethodDefOptions& options, WithDefaults with_defaults)
    {
        options.trailing_default_argument_count = with_defaults.trailing_default_argument_count;
    }

    static void apply_method_def_option(MethodDefOptions& options, Virtual)
    {
        options.allow_override = true;
    }

    static void apply_method_def_option(MethodDefOptions& options, CppSymbol cpp_symbol)
    {
        options.cpp_symbol = cpp_symbol.value;
    }

    static void apply_method_def_option(MethodDefOptions& options, Attribute attribute)
    {
        options.csharp_attributes.push_back(std::string(attribute.value));
    }

    template <typename Option> static void apply_method_def_option(MethodDefOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ClassBuilder::def option. Supported tags: Ownership, WithDefaults, Virtual, CppSymbol, Attribute.");
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    void add_method(std::string_view name, bool is_const, Ownership return_ownership,
        std::size_t trailing_default_argument_count = 0, bool allow_override = false,
        bool is_property_getter = false, bool is_property_setter = false, std::string_view cpp_symbol = {},
        std::vector<std::string> csharp_attributes = {})
    {
        FunctionDecl method_decl;
        method_decl.name = std::string(name);
        method_decl.cpp_symbol = cpp_symbol.empty() ? std::string(name) : std::string(cpp_symbol);
        method_decl.return_type = owner_->make_bound_return_type_ref<ReturnType>();
        method_decl.return_ownership = return_ownership;
        method_decl.trailing_default_argument_count = trailing_default_argument_count;
        method_decl.is_method = true;
        method_decl.is_const = is_const;
        method_decl.is_virtual = allow_override;
        method_decl.allow_override = allow_override;
        method_decl.is_property_getter = is_property_getter;
        method_decl.is_property_setter = is_property_setter;
        method_decl.class_name = class_decl_->cpp_name;
        method_decl.virtual_slot_name = std::string(name);
        method_decl.csharp_attributes = std::move(csharp_attributes);

        method_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((method_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>()})),
            ...);

        class_decl_->methods.push_back(std::move(method_decl));
    }

    BindingsGenerator* owner_;
    ClassDecl* class_decl_;
};

class EnumBuilder
{
public:
    EnumBuilder(BindingsGenerator& owner, EnumDecl& enum_decl)
        : owner_(&owner)
        , enum_decl_(&enum_decl)
    {
    }

    template <typename EnumType> EnumBuilder& value(std::string_view name, EnumType enum_value)
    {
        static_assert(std::is_enum_v<EnumType>, "EnumBuilder::value expects an enum value");

        using Underlying = std::underlying_type_t<EnumType>;
        EnumValueDecl value_decl;
        value_decl.name = std::string(name);
        value_decl.is_signed = std::is_signed_v<Underlying>;
        value_decl.value = static_cast<std::uint64_t>(static_cast<Underlying>(enum_value));
        enum_decl_->values.push_back(std::move(value_decl));
        return *this;
    }

private:
    BindingsGenerator* owner_;
    EnumDecl* enum_decl_;
};

template <typename ClassType, typename... BaseTypes>
ClassBuilder ModuleBuilder::class_()
{
    return class_<ClassType, BaseTypes...>(
        detail::unqualified_type_name<ClassType>(), CppName{detail::qualified_type_name<ClassType>()});
}

template <typename ClassType, typename... BaseTypes, typename... Options>
ClassBuilder ModuleBuilder::class_(std::string_view name, Options&&... options)
{
    const auto class_options = make_class_options(std::forward<Options>(options)...);

    ClassDecl class_decl;
    class_decl.name = std::string(name);
    class_decl.cpp_name = class_options.cpp_name.empty() ? detail::qualified_type_name<ClassType>()
                                                          : std::string(class_options.cpp_name);
    class_decl.csharp_attributes = class_options.csharp_attributes;
    auto add_base = [&class_decl]<typename BaseType>() {
        class_decl.base_classes.push_back(
            ClassDecl::BaseClassDecl{detail::unqualified_type_name<BaseType>(), detail::qualified_type_name<BaseType>()});
    };
    (add_base.template operator()<BaseTypes>(), ...);

    if (!class_decl.base_classes.empty())
    {
        class_decl.base_name = class_decl.base_classes.front().name;
        class_decl.base_cpp_name = class_decl.base_classes.front().cpp_name;
    }
    module_decl_->classes.push_back(std::move(class_decl));
    return ClassBuilder(*owner_, module_decl_->classes.back());
}

template <typename EnumType>
EnumBuilder ModuleBuilder::enum_()
{
    return enum_<EnumType>(detail::unqualified_type_name<EnumType>(), CppName{detail::qualified_type_name<EnumType>()});
}

template <typename EnumType, typename... Options>
EnumBuilder ModuleBuilder::enum_(std::string_view name, Options&&... options)
{
    static_assert(std::is_enum_v<EnumType>, "ModuleBuilder::enum_ expects an enum type");

    const auto enum_options = make_enum_options(std::forward<Options>(options)...);

    using Underlying = std::underlying_type_t<EnumType>;
    EnumDecl enum_decl;
    enum_decl.name = std::string(name);
    enum_decl.cpp_name = enum_options.cpp_name.empty() ? detail::qualified_type_name<EnumType>()
                                                        : std::string(enum_options.cpp_name);
    enum_decl.underlying_type = owner_->make_bound_type_ref<Underlying>();
    enum_decl.is_flags = enum_options.is_flags;
    enum_decl.csharp_attributes = enum_options.csharp_attributes;
    module_decl_->enums.push_back(std::move(enum_decl));
    return EnumBuilder(*owner_, module_decl_->enums.back());
}

} // namespace csbind23
