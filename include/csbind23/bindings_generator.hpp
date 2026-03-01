#pragma once

#include "csbind23/ir.hpp"
#include "csbind23/type_utils.hpp"

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <format>
#include <initializer_list>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace csbind23
{

class ModuleBuilder;
class ClassBuilder;
template <typename... ClassTypes> class GenericClassBuilder;
class EnumBuilder;

struct WithDefaults
{
    std::size_t trailing_default_argument_count;
};

struct Virtual
{
};

struct PInvoke
{
};

struct CppSymbol
{
    std::string_view value;
};

struct CppSymbols
{
    std::vector<std::string> values;

    CppSymbols() = default;

    CppSymbols(std::initializer_list<std::string_view> symbol_values)
    {
        values.reserve(symbol_values.size());
        for (const auto symbol : symbol_values)
        {
            values.push_back(std::string(symbol));
        }
    }
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

struct Comment
{
    std::string_view value;
};

struct Arg
{
    std::size_t idx = 0;
    std::string_view name = {};
    bool output = false;
};

namespace detail
{

template <auto First, auto... Rest, typename Callback>
void for_each_nontype_indexed(Callback&& callback)
{
    std::size_t index = 0;
    callback.template operator()<First>(index++);
    (callback.template operator()<Rest>(index++), ...);
}

inline std::string_view generic_cpp_symbol_for(const std::vector<std::string>& cpp_symbols, std::size_t index)
{
    if (index >= cpp_symbols.size())
    {
        return {};
    }

    return cpp_symbols[index];
}

inline std::string generic_instantiation_suffix(std::size_t index)
{
    return std::format("__csbind23_g{}", index);
}

inline void mark_generic_instantiation(FunctionDecl& function_decl, std::string_view group_name)
{
    function_decl.is_generic_instantiation = true;
    function_decl.generic_group_name = std::string(group_name);
}

inline void mark_generic_instantiation(ClassDecl& class_decl, std::string_view group_name)
{
    class_decl.is_generic_instantiation = true;
    class_decl.generic_group_name = std::string(group_name);
}

inline void apply_arg_options(std::vector<ParameterDecl>& parameters, const std::vector<Arg>& arg_options)
{
    for (const auto& arg_option : arg_options)
    {
        if (arg_option.idx >= parameters.size())
        {
            continue;
        }

        auto& parameter = parameters[arg_option.idx];
        if (!arg_option.name.empty())
        {
            parameter.name = std::string(arg_option.name);
        }
        parameter.is_output = arg_option.output;
    }
}

} // namespace detail

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

    ModuleBuilder& csharp_namespace(std::string_view namespace_name)
    {
        module_decl_->csharp_namespace = std::string(namespace_name);
        return *this;
    }

    ModuleBuilder& csharp_name_formatter(std::function<std::string(CSharpNameKind, std::string_view)> formatter)
    {
        module_decl_->csharp_name_formatter = std::move(formatter);
        return *this;
    }

    ModuleBuilder& cabi_include(std::string_view include_path)
    {
        module_decl_->cabi_includes.push_back(std::string(include_path));
        return *this;
    }

    ModuleBuilder& set_instance_cache_type(std::string_view instance_cache_type)
    {
        module_decl_->instance_cache_type = std::string(instance_cache_type);
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
            cpp_symbol, def_options.csharp_attributes, def_options.csharp_comment, def_options.arg_options,
            {}, def_options.pinvoke_only);
    }

    template <typename ReturnType, typename... Args, typename... Options>
    ModuleBuilder& def(std::string_view name, ReturnType (*function_ptr)(Args...), Options&&... options)
    {
        const auto def_options = make_function_def_options(std::forward<Options>(options)...);
        return def_impl(name, function_ptr, def_options.return_ownership, def_options.trailing_default_argument_count,
            def_options.cpp_symbol, def_options.csharp_attributes, def_options.csharp_comment, def_options.arg_options,
            {}, def_options.pinvoke_only);
    }

    template <auto FirstFunction, auto... RestFunctions> ModuleBuilder& def_generic()
    {
        return def_generic<FirstFunction, RestFunctions...>(detail::function_export_name<FirstFunction>());
    }

    template <auto FirstFunction, auto... RestFunctions, typename... Options>
    ModuleBuilder& def_generic(std::string_view name, Options&&... options)
    {
        const auto def_options = make_function_def_options(std::forward<Options>(options)...);
        detail::for_each_nontype_indexed<FirstFunction, RestFunctions...>([&]<auto Function>(std::size_t index) {
            add_generic_instantiation<Function>(
                name, def_options, detail::generic_cpp_symbol_for(def_options.cpp_symbols, index));
        });
        return *this;
    }

    template <typename ReturnType, typename... Args>
    ModuleBuilder& def_impl(std::string_view name, ReturnType (*function_ptr)(Args...), Ownership return_ownership,
        std::size_t trailing_default_argument_count, std::string_view cpp_symbol,
        std::vector<std::string> csharp_attributes = {}, std::string csharp_comment = {},
        std::vector<Arg> arg_options = {},
        std::string_view exported_name = {}, bool pinvoke_only = false)
    {
        (void)function_ptr;

        FunctionDecl function_decl;
        function_decl.name = std::string(name);
        function_decl.exported_name = exported_name.empty() ? std::string(name) : std::string(exported_name);
        function_decl.cpp_symbol = cpp_symbol.empty() ? std::string(name) : std::string(cpp_symbol);
        function_decl.return_type = owner_->make_bound_return_type_ref<ReturnType>();
        function_decl.return_ownership = return_ownership;
        function_decl.trailing_default_argument_count =
            trailing_default_argument_count > sizeof...(Args) ? sizeof...(Args) : trailing_default_argument_count;
        function_decl.pinvoke_only = pinvoke_only;
        function_decl.csharp_attributes = std::move(csharp_attributes);
        function_decl.csharp_comment = std::move(csharp_comment);

        function_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((function_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>()})),
            ...);

        detail::apply_arg_options(function_decl.parameters, arg_options);

        module_decl_->functions.push_back(std::move(function_decl));
        return *this;
    }

    template <typename ClassType, typename... BaseTypes>
    ClassBuilder class_();

    template <typename ClassType, typename... BaseTypes, typename... Options>
    ClassBuilder class_(std::string_view name, Options&&... options);

    template <typename FirstClass, typename... RestClasses>
    GenericClassBuilder<FirstClass, RestClasses...> class_generic();

    template <typename FirstClass, typename... RestClasses>
    GenericClassBuilder<FirstClass, RestClasses...> class_generic(std::string_view name);

    template <typename EnumType>
    EnumBuilder enum_();

    template <typename EnumType, typename... Options>
    EnumBuilder enum_(std::string_view name, Options&&... options);

private:
    struct FunctionDefOptions;

    template <auto Function>
    ModuleBuilder& add_generic_instantiation(
        std::string_view name, const FunctionDefOptions& def_options, std::string_view cpp_symbol_override = {})
    {
        const std::string cpp_symbol = !cpp_symbol_override.empty()
            ? std::string(cpp_symbol_override)
            : (def_options.cpp_symbol.empty()
            ? std::string(detail::function_symbol_name<Function>())
            : std::string(def_options.cpp_symbol));

        const std::string group_name(name);
        const std::string unique_suffix = detail::generic_instantiation_suffix(module_decl_->functions.size());
        const std::string managed_name = group_name + unique_suffix;
        const std::string export_name = group_name + unique_suffix;

        def_impl(managed_name, Function, def_options.return_ownership,
            def_options.trailing_default_argument_count, cpp_symbol, def_options.csharp_attributes,
            def_options.csharp_comment, def_options.arg_options, export_name, def_options.pinvoke_only);

        FunctionDecl& inserted = module_decl_->functions.back();
        detail::mark_generic_instantiation(inserted, group_name);
        return *this;
    }

    struct FunctionDefOptions
    {
        Ownership return_ownership = Ownership::Auto;
        std::size_t trailing_default_argument_count = 0;
        bool pinvoke_only = false;
        std::string_view cpp_symbol = {};
        std::vector<std::string> cpp_symbols;
        std::vector<std::string> csharp_attributes;
        std::string csharp_comment;
        std::vector<Arg> arg_options;
    };

    struct ClassOptions
    {
        std::string_view cpp_name = {};
        std::vector<std::string> csharp_attributes;
        std::string csharp_comment;
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

    static void apply_function_def_option(FunctionDefOptions& options, PInvoke)
    {
        options.pinvoke_only = true;
    }

    static void apply_function_def_option(FunctionDefOptions& options, CppSymbol cpp_symbol)
    {
        options.cpp_symbol = cpp_symbol.value;
    }

    static void apply_function_def_option(FunctionDefOptions& options, CppSymbols cpp_symbols)
    {
        options.cpp_symbols = std::move(cpp_symbols.values);
    }

    static void apply_function_def_option(FunctionDefOptions& options, Attribute attribute)
    {
        options.csharp_attributes.push_back(std::string(attribute.value));
    }

    static void apply_function_def_option(FunctionDefOptions& options, Comment comment)
    {
        options.csharp_comment = std::string(comment.value);
    }

    static void apply_function_def_option(FunctionDefOptions& options, Arg arg_option)
    {
        options.arg_options.push_back(arg_option);
    }

    template <typename Option> static void apply_function_def_option(FunctionDefOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ModuleBuilder::def option. Supported tags: Ownership, WithDefaults, PInvoke, CppSymbol, CppSymbols, Attribute, Comment, Arg.");
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

    static void apply_class_option(ClassOptions& options, Comment comment)
    {
        options.csharp_comment = std::string(comment.value);
    }

    template <typename Option> static void apply_class_option(ClassOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ModuleBuilder::class_ option. Supported tags: CppName, Attribute, Comment.");
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

    ClassBuilder& comment(std::string_view comment_text)
    {
        class_decl_->csharp_comment = std::string(comment_text);
        return *this;
    }

    ClassBuilder& property_comment(std::string_view property_name, std::string_view comment_text)
    {
        for (auto& property_decl : class_decl_->properties)
        {
            if (property_decl.name == property_name)
            {
                property_decl.csharp_comment = std::string(comment_text);
                break;
            }
        }
        return *this;
    }

    ClassBuilder& set_instance_cache_type(std::string_view instance_cache_type)
    {
        class_decl_->instance_cache_type = std::string(instance_cache_type);
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
            def_options.cpp_symbol, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only);
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
            def_options.cpp_symbol, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only);
        return *this;
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs> ClassBuilder& def_generic()
    {
        return def_generic<FirstMethodPtr, RestMethodPtrs...>(detail::function_export_name<FirstMethodPtr>());
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs, typename... Options>
    ClassBuilder& def_generic(std::string_view name, Options&&... options)
    {
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        detail::for_each_nontype_indexed<FirstMethodPtr, RestMethodPtrs...>([&]<auto MethodPtr>(std::size_t index) {
            add_generic_method_instantiation_auto<MethodPtr>(
                name, def_options, detail::generic_cpp_symbol_for(def_options.cpp_symbols, index));
        });
        return *this;
    }

    template <auto GetterPtr> ClassBuilder& property(std::string_view name)
    {
        return property(name, GetterPtr, detail::function_symbol_name<GetterPtr>());
    }

    template <auto FieldPtr> ClassBuilder& field()
    {
        return field<FieldPtr>(detail::unqualified_name(detail::function_symbol_name<FieldPtr>()));
    }

    template <auto FieldPtr> ClassBuilder& field(std::string_view name)
    {
        using FieldTraits = detail::FieldPointerTraits<decltype(FieldPtr)>;
        using ClassType = typename FieldTraits::class_type;
        using FieldType = typename FieldTraits::field_type;
        using BareFieldType = std::remove_const_t<FieldType>;

        PropertyDecl property_decl;
        property_decl.name = std::string(name);
        property_decl.type = owner_->make_bound_type_ref<BareFieldType>();
        property_decl.has_getter = true;
        property_decl.is_field_projection = true;
        property_decl.getter_name = std::string("__csbind23_propget_") + std::string(name);

        const std::string field_symbol = detail::unqualified_name(detail::function_symbol_name<FieldPtr>());

        add_method<ClassType, BareFieldType>(property_decl.getter_name, true, Ownership::Auto, 0, false,
            true, false, field_symbol, {}, {}, {}, true);

        if constexpr (!std::is_const_v<FieldType>)
        {
            property_decl.has_setter = true;
            property_decl.setter_name = std::string("__csbind23_propset_") + std::string(name);
            add_method<ClassType, void, BareFieldType>(property_decl.setter_name, false, Ownership::Auto, 0,
                false, false, true, field_symbol, {}, {}, {}, true);
        }

        class_decl_->properties.push_back(std::move(property_decl));
        return *this;
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
        bool pinvoke_only = false;
        std::string_view cpp_symbol = {};
        std::vector<std::string> cpp_symbols;
        std::vector<std::string> csharp_attributes;
        std::string csharp_comment;
        std::vector<Arg> arg_options;
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

    static void apply_method_def_option(MethodDefOptions& options, PInvoke)
    {
        options.pinvoke_only = true;
    }

    static void apply_method_def_option(MethodDefOptions& options, CppSymbol cpp_symbol)
    {
        options.cpp_symbol = cpp_symbol.value;
    }

    static void apply_method_def_option(MethodDefOptions& options, CppSymbols cpp_symbols)
    {
        options.cpp_symbols = std::move(cpp_symbols.values);
    }

    static void apply_method_def_option(MethodDefOptions& options, Attribute attribute)
    {
        options.csharp_attributes.push_back(std::string(attribute.value));
    }

    static void apply_method_def_option(MethodDefOptions& options, Comment comment)
    {
        options.csharp_comment = std::string(comment.value);
    }

    static void apply_method_def_option(MethodDefOptions& options, Arg arg_option)
    {
        options.arg_options.push_back(arg_option);
    }

    template <typename Option> static void apply_method_def_option(MethodDefOptions&, Option&&)
    {
        static_assert(dependent_false<std::decay_t<Option>>::value,
            "Unsupported ClassBuilder::def option. Supported tags: Ownership, WithDefaults, Virtual, PInvoke, CppSymbol, CppSymbols, Attribute, Comment, Arg.");
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    void add_method(std::string_view name, bool is_const, Ownership return_ownership,
        std::size_t trailing_default_argument_count = 0, bool allow_override = false,
        bool is_property_getter = false, bool is_property_setter = false, std::string_view cpp_symbol = {},
        std::vector<std::string> csharp_attributes = {}, std::string csharp_comment = {},
        std::vector<Arg> arg_options = {},
        bool is_field_accessor = false, bool pinvoke_only = false)
    {
        FunctionDecl method_decl;
        method_decl.name = std::string(name);
        method_decl.exported_name = std::string(name);
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
        method_decl.is_field_accessor = is_field_accessor;
        method_decl.pinvoke_only = pinvoke_only;
        method_decl.class_name = class_decl_->cpp_name;
        method_decl.virtual_slot_name = std::string(name);
        method_decl.csharp_attributes = std::move(csharp_attributes);
        method_decl.csharp_comment = std::move(csharp_comment);

        method_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((method_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>()})),
            ...);

        detail::apply_arg_options(method_decl.parameters, arg_options);

        class_decl_->methods.push_back(std::move(method_decl));
    }

    template <auto MethodPtr>
    void add_generic_method_instantiation_auto(
        std::string_view name, const MethodDefOptions& def_options, std::string_view cpp_symbol_override = {})
    {
        const std::string cpp_symbol = !cpp_symbol_override.empty()
            ? std::string(cpp_symbol_override)
            : (def_options.cpp_symbol.empty()
            ? infer_generic_method_cpp_symbol(std::string(detail::function_symbol_name<MethodPtr>()), MethodPtr)
            : std::string(def_options.cpp_symbol));

        add_generic_method_instantiation(name, MethodPtr, def_options, cpp_symbol);
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    static std::string infer_generic_method_cpp_symbol(
        std::string base_symbol, ReturnType (ClassType::*)(Args...))
    {
        if (base_symbol.empty() || base_symbol.find('<') != std::string::npos)
        {
            return base_symbol;
        }

        std::vector<std::string> template_args;
        if constexpr (sizeof...(Args) == 0)
        {
            template_args.push_back(detail::cpp_type_name<ReturnType>());
        }
        else
        {
            const std::string return_type_name = detail::cpp_type_name<ReturnType>();
            const std::string first_arg_type_name = detail::cpp_type_name<std::tuple_element_t<0, std::tuple<Args...>>>();
            if (return_type_name != first_arg_type_name)
            {
                template_args.push_back(return_type_name);
            }

            (template_args.push_back(detail::cpp_type_name<Args>()), ...);
        }

        if (template_args.empty())
        {
            return base_symbol;
        }

        std::string rendered = std::move(base_symbol);
        rendered += "<";
        for (std::size_t index = 0; index < template_args.size(); ++index)
        {
            rendered += template_args[index];
            if (index + 1 < template_args.size())
            {
                rendered += ", ";
            }
        }
        rendered += ">";
        return rendered;
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    static std::string infer_generic_method_cpp_symbol(
        std::string base_symbol, ReturnType (ClassType::*)(Args...) const)
    {
        if (base_symbol.empty() || base_symbol.find('<') != std::string::npos)
        {
            return base_symbol;
        }

        std::vector<std::string> template_args;
        if constexpr (sizeof...(Args) == 0)
        {
            template_args.push_back(detail::cpp_type_name<ReturnType>());
        }
        else
        {
            const std::string return_type_name = detail::cpp_type_name<ReturnType>();
            const std::string first_arg_type_name = detail::cpp_type_name<std::tuple_element_t<0, std::tuple<Args...>>>();
            if (return_type_name != first_arg_type_name)
            {
                template_args.push_back(return_type_name);
            }

            (template_args.push_back(detail::cpp_type_name<Args>()), ...);
        }

        if (template_args.empty())
        {
            return base_symbol;
        }

        std::string rendered = std::move(base_symbol);
        rendered += "<";
        for (std::size_t index = 0; index < template_args.size(); ++index)
        {
            rendered += template_args[index];
            if (index + 1 < template_args.size())
            {
                rendered += ", ";
            }
        }
        rendered += ">";
        return rendered;
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    void add_generic_method_instantiation(std::string_view name, ReturnType (ClassType::*method_ptr)(Args...),
        const MethodDefOptions& def_options, std::string_view cpp_symbol_override = {})
    {
        (void)method_ptr;
        if (def_options.allow_override)
        {
            class_decl_->enable_virtual_overrides = true;
        }

        const std::string group_name(name);
        const std::string unique_suffix = detail::generic_instantiation_suffix(class_decl_->methods.size());
        const std::string managed_name = group_name + unique_suffix;

        add_method<ClassType, ReturnType, Args...>(managed_name, false, def_options.return_ownership,
            def_options.trailing_default_argument_count, def_options.allow_override, false, false,
            cpp_symbol_override, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only);

        FunctionDecl& inserted = class_decl_->methods.back();
        detail::mark_generic_instantiation(inserted, group_name);
        inserted.exported_name = inserted.name;
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    void add_generic_method_instantiation(std::string_view name, ReturnType (ClassType::*method_ptr)(Args...) const,
        const MethodDefOptions& def_options, std::string_view cpp_symbol_override = {})
    {
        (void)method_ptr;
        if (def_options.allow_override)
        {
            class_decl_->enable_virtual_overrides = true;
        }

        const std::string group_name(name);
        const std::string unique_suffix = detail::generic_instantiation_suffix(class_decl_->methods.size());
        const std::string managed_name = group_name + unique_suffix;

        add_method<ClassType, ReturnType, Args...>(managed_name, true, def_options.return_ownership,
            def_options.trailing_default_argument_count, def_options.allow_override, false, false,
            cpp_symbol_override, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only);

        FunctionDecl& inserted = class_decl_->methods.back();
        detail::mark_generic_instantiation(inserted, group_name);
        inserted.exported_name = inserted.name;
    }

    BindingsGenerator* owner_;
    ClassDecl* class_decl_;
};

template <typename... ClassTypes>
class GenericClassBuilder
{
public:
    GenericClassBuilder(
        BindingsGenerator& owner, ModuleDecl& module_decl, std::vector<std::size_t> class_indices, std::string group_name)
        : owner_(&owner)
        , module_decl_(&module_decl)
        , class_indices_(std::move(class_indices))
        , group_name_(std::move(group_name))
    {
    }

    template <typename... Args> GenericClassBuilder& ctor(Ownership ownership = Ownership::Auto)
    {
        for (const auto class_index : class_indices_)
        {
            ClassBuilder(*owner_, module_decl_->classes[class_index]).template ctor<Args...>(ownership);
        }
        return *this;
    }

    GenericClassBuilder& set_instance_cache_type(std::string_view instance_cache_type)
    {
        for (const auto class_index : class_indices_)
        {
            module_decl_->classes[class_index].instance_cache_type = std::string(instance_cache_type);
        }
        return *this;
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs> GenericClassBuilder& def()
    {
        return def<FirstMethodPtr, RestMethodPtrs...>(detail::function_export_name<FirstMethodPtr>());
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs, typename... Options>
    GenericClassBuilder& def(std::string_view name, Options&&... options)
    {
        constexpr std::size_t method_count = 1 + sizeof...(RestMethodPtrs);
        constexpr std::size_t class_count = sizeof...(ClassTypes);
        static_assert(method_count == class_count,
            "GenericClassBuilder::def requires one method pointer per class instantiation.");

        auto options_tuple = std::make_tuple(std::forward<Options>(options)...);
        detail::for_each_nontype_indexed<FirstMethodPtr, RestMethodPtrs...>([&]<auto MethodPtr>(std::size_t index) {
            ClassBuilder class_builder(*owner_, module_decl_->classes[class_indices_[index]]);
            std::apply(
                [&]<typename... OptionTypes>(const OptionTypes&... unpacked_options) {
                    class_builder.template def<MethodPtr>(name, unpacked_options...);
                },
                options_tuple);
        });

        return *this;
    }

private:
    BindingsGenerator* owner_;
    ModuleDecl* module_decl_;
    std::vector<std::size_t> class_indices_;
    std::string group_name_;
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
    class_decl.csharp_comment = class_options.csharp_comment;
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

template <typename FirstClass, typename... RestClasses>
GenericClassBuilder<FirstClass, RestClasses...> ModuleBuilder::class_generic()
{
    return class_generic<FirstClass, RestClasses...>(detail::unqualified_type_name<FirstClass>());
}

template <typename FirstClass, typename... RestClasses>
GenericClassBuilder<FirstClass, RestClasses...> ModuleBuilder::class_generic(std::string_view name)
{
    const std::string group_name(name);
    std::vector<std::size_t> instantiation_indices;
    instantiation_indices.reserve(1 + sizeof...(RestClasses));

    auto add_instantiation = [this, &group_name, &instantiation_indices]<typename ClassType>() {
        const std::string unique_suffix = detail::generic_instantiation_suffix(module_decl_->classes.size());
        const std::string managed_name = group_name + unique_suffix;
        (void)class_<ClassType>(managed_name, CppName{detail::qualified_type_name<ClassType>()});
        const std::size_t class_index = module_decl_->classes.size() - 1;
        ClassDecl& class_decl = module_decl_->classes[class_index];
        detail::mark_generic_instantiation(class_decl, group_name);
        instantiation_indices.push_back(class_index);
    };

    add_instantiation.template operator()<FirstClass>();
    (add_instantiation.template operator()<RestClasses>(), ...);

    return GenericClassBuilder<FirstClass, RestClasses...>(
        *owner_, *module_decl_, std::move(instantiation_indices), group_name);
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
