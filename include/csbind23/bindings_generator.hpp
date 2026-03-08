#pragma once

#include "csbind23/emit/csharp_naming.hpp"
#include "csbind23/ir.hpp"
#include "csbind23/type_utils.hpp"

#include <algorithm>
#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <format>
#include <initializer_list>
#include <stdexcept>
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

struct Private
{
};

struct ExportName
{
    std::string_view value;
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
    bool c_array = false;
    std::size_t size_param_idx = std::numeric_limits<std::size_t>::max();
};

template <typename... Types>
struct TemplateArgs
{
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
        parameter.is_c_array = arg_option.c_array;
        if (arg_option.c_array && arg_option.size_param_idx < parameters.size())
        {
            parameter.c_array_size_param_index = arg_option.size_param_idx;
        }
    }
}

template <typename Spec>
struct TemplateSpecExpander
{
    template <template <typename...> typename ClassTemplate>
    using class_type = ClassTemplate<Spec>;

    template <auto Resolver>
    static consteval auto resolve_nontype()
    {
        return Resolver.template operator()<Spec>();
    }
};

template <typename... Types>
struct TemplateSpecExpander<TemplateArgs<Types...>>
{
    template <template <typename...> typename ClassTemplate>
    using class_type = ClassTemplate<Types...>;

    template <auto Resolver>
    static consteval auto resolve_nontype()
    {
        return Resolver.template operator()<Types...>();
    }
};

template <template <typename...> typename ClassTemplate, typename Spec>
using instantiate_template_from_spec_t = typename TemplateSpecExpander<Spec>::template class_type<ClassTemplate>;

template <auto Resolver, typename Spec>
consteval auto resolve_nontype_from_spec()
{
    return TemplateSpecExpander<Spec>::template resolve_nontype<Resolver>();
}

template <typename Spec>
struct TemplateSpecRenderer
{
    static void append_cpp_args(std::vector<std::string>& args)
    {
        args.push_back(detail::cpp_type_name<Spec>());
    }
};

template <typename... Types>
struct TemplateSpecRenderer<TemplateArgs<Types...>>
{
    static void append_cpp_args(std::vector<std::string>& args)
    {
        (args.push_back(detail::cpp_type_name<Types>()), ...);
    }
};

template <typename Spec>
std::vector<std::string> template_spec_cpp_args()
{
    std::vector<std::string> args;
    TemplateSpecRenderer<Spec>::append_cpp_args(args);
    return args;
}

inline bool symbol_has_function_template_args(std::string_view symbol)
{
    const std::size_t last_scope = symbol.rfind("::");
    const std::string_view trailing =
        last_scope == std::string_view::npos ? symbol : symbol.substr(last_scope + 2);
    return trailing.find('<') != std::string_view::npos;
}

template <typename Spec>
std::string instantiate_symbol_from_spec(std::string base_symbol)
{
    if (base_symbol.empty() || symbol_has_function_template_args(base_symbol))
    {
        return base_symbol;
    }

    const auto template_args = template_spec_cpp_args<Spec>();
    if (template_args.empty())
    {
        return base_symbol;
    }

    base_symbol += "<";
    for (std::size_t index = 0; index < template_args.size(); ++index)
    {
        base_symbol += template_args[index];
        if (index + 1 < template_args.size())
        {
            base_symbol += ", ";
        }
    }
    base_symbol += ">";
    return base_symbol;
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

    template <typename Type> TypeRef make_bound_param_type_ref(const ModuleDecl* current_module = nullptr) const
    {
        TypeRef type_ref = detail::make_param_type_ref<Type>();
        apply_managed_converter<Type>(type_ref, current_module, false);
        return type_ref;
    }

    template <typename Type> TypeRef make_bound_return_type_ref(const ModuleDecl* current_module = nullptr) const
    {
        TypeRef type_ref = detail::make_return_type_ref<Type>();
        apply_managed_converter<Type>(type_ref, current_module, true);
        return type_ref;
    }

    template <typename Type> TypeRef make_bound_type_ref(const ModuleDecl* current_module = nullptr) const
    {
        return make_bound_param_type_ref<Type>(current_module);
    }

private:
    struct BoundClassMatch
    {
        const ModuleDecl* module_decl = nullptr;
        const ClassDecl* class_decl = nullptr;
    };

    struct BoundEnumMatch
    {
        const ModuleDecl* module_decl = nullptr;
        const EnumDecl* enum_decl = nullptr;
    };

    template <typename Type> static void assign_managed_converter(TypeRef& type_ref, bool for_return_type)
    {
        type_ref.managed_type_name = for_return_type
            ? std::string(cabi::detail::managed_return_type_name_for<Type>())
            : std::string(cabi::detail::managed_param_type_name_for<Type>());
        type_ref.managed_to_pinvoke_expression =
            std::string(cabi::detail::managed_to_pinvoke_expression_for<Type>());
        type_ref.managed_from_pinvoke_expression =
            std::string(cabi::detail::managed_from_pinvoke_expression_for<Type>());
        type_ref.managed_finalize_to_pinvoke_statement =
            std::string(cabi::detail::managed_finalize_to_pinvoke_statement_for<Type>());
        type_ref.managed_finalize_from_pinvoke_statement =
            std::string(cabi::detail::managed_finalize_from_pinvoke_statement_for<Type>());
    }

    static std::string csharp_namespace_name_for(const ModuleDecl& module_decl, const ClassDecl& class_decl)
    {
        const std::string base_namespace = csharp_namespace_name_for(module_decl);
        if (class_decl.csharp_namespace.empty())
        {
            return base_namespace;
        }

        if (base_namespace.empty())
        {
            return class_decl.csharp_namespace;
        }

        return base_namespace + "." + class_decl.csharp_namespace;
    }

    static std::string csharp_namespace_name_for(const ModuleDecl& module_decl)
    {
        if (!module_decl.csharp_namespace.empty())
        {
            return module_decl.csharp_namespace;
        }

        if (module_decl.name == stl_module_name())
        {
            return std::string(default_stl_csharp_namespace());
        }

        return make_safe_csharp_namespace_segment(
            emit::format_csharp_name(module_decl, CSharpNameKind::Class, module_decl.name));
    }

    static std::string managed_class_name_for(const ModuleDecl& module_decl, const ClassDecl& class_decl)
    {
        return emit::format_csharp_name(module_decl, CSharpNameKind::Class, class_decl.name);
    }

    static std::string managed_enum_name_for(const ModuleDecl& module_decl, const EnumDecl& enum_decl)
    {
        return emit::format_csharp_name(module_decl, CSharpNameKind::Class, enum_decl.name);
    }

    const ModuleDecl* find_module(std::string_view name) const
    {
        const auto it = std::find_if(modules_.begin(), modules_.end(),
            [name](const ModuleDecl& module_decl) { return module_decl.name == name; });
        return it == modules_.end() ? nullptr : &*it;
    }

    std::vector<const ModuleDecl*> visible_modules_for(const ModuleDecl& current_module) const
    {
        std::vector<const ModuleDecl*> visible_modules;
        visible_modules.push_back(&current_module);

        if (current_module.name == stl_module_name())
        {
            for (const auto& module_decl : modules_)
            {
                if (module_decl.name == current_module.name)
                {
                    continue;
                }

                visible_modules.push_back(&module_decl);
            }
            return visible_modules;
        }

        if (current_module.name != stl_module_name())
        {
            if (const ModuleDecl* stl_module = find_module(stl_module_name()); stl_module != nullptr)
            {
                visible_modules.push_back(stl_module);
            }
        }

        for (const auto& imported_module_name : current_module.imported_modules)
        {
            if (imported_module_name == current_module.name)
            {
                throw std::runtime_error(std::format(
                    "Module '{}' cannot import itself.", current_module.name));
            }

            const ModuleDecl* imported_module = find_module(imported_module_name);
            if (imported_module == nullptr)
            {
                continue;
            }

            if (std::find(visible_modules.begin(), visible_modules.end(), imported_module) == visible_modules.end())
            {
                visible_modules.push_back(imported_module);
            }
        }

        return visible_modules;
    }

    static void append_unique_strings(std::vector<std::string>& destination, const std::vector<std::string>& source)
    {
        for (const auto& value : source)
        {
            if (std::find(destination.begin(), destination.end(), value) == destination.end())
            {
                destination.push_back(value);
            }
        }
    }

    static void finalize_stl_module(std::vector<ModuleDecl>& modules)
    {
        auto stl_module_it = std::find_if(modules.begin(), modules.end(),
            [](const ModuleDecl& module_decl) { return module_decl.name == stl_module_name(); });
        if (stl_module_it == modules.end())
        {
            return;
        }

        ModuleDecl& stl_module = *stl_module_it;
        for (const auto& module_decl : modules)
        {
            if (module_decl.name == stl_module.name)
            {
                continue;
            }

            append_unique_strings(stl_module.cabi_includes, module_decl.cabi_includes);
        }

        if (stl_module.pinvoke_library.empty() || stl_module.pinvoke_library == stl_module.name)
        {
            std::string selected_library;
            for (const auto& module_decl : modules)
            {
                if (module_decl.name == stl_module.name || module_decl.pinvoke_library.empty())
                {
                    continue;
                }

                if (selected_library.empty())
                {
                    selected_library = module_decl.pinvoke_library;
                    continue;
                }

                if (module_decl.pinvoke_library != selected_library)
                {
                    throw std::runtime_error(std::format(
                        "STL module '{}' cannot infer a single pinvoke library. Saw '{}' and '{}'.",
                        stl_module.name,
                        selected_library,
                        module_decl.pinvoke_library));
                }
            }

            if (!selected_library.empty())
            {
                stl_module.pinvoke_library = selected_library;
            }
        }
    }

    void validate_imports() const
    {
        for (const auto& module_decl : modules_)
        {
            for (const auto& imported_module_name : module_decl.imported_modules)
            {
                if (imported_module_name == module_decl.name)
                {
                    throw std::runtime_error(std::format(
                        "Module '{}' cannot import itself.", module_decl.name));
                }

                if (find_module(imported_module_name) == nullptr)
                {
                    throw std::runtime_error(std::format(
                        "Module '{}' imports unknown module '{}'.", module_decl.name, imported_module_name));
                }
            }
        }
    }

    template <typename Match, typename Finder>
    Match find_visible_unique_match(
        std::string_view kind, std::string_view cpp_name, const ModuleDecl* current_module, Finder&& finder) const
    {
        if (current_module == nullptr)
        {
            return finder(nullptr);
        }

        std::vector<Match> matches;
        for (const ModuleDecl* visible_module : visible_modules_for(*current_module))
        {
            Match match = finder(visible_module);
            if (match.module_decl != nullptr)
            {
                matches.push_back(match);
            }
        }

        if (matches.empty())
        {
            return {};
        }

        if (matches.size() > 1)
        {
            std::string owner_names;
            for (std::size_t index = 0; index < matches.size(); ++index)
            {
                if (index > 0)
                {
                    owner_names += ", ";
                }
                owner_names += matches[index].module_decl->name;
            }

            throw std::runtime_error(std::format(
                "Ambiguous {} reuse for C++ type '{}' in module '{}'. Visible owners: {}.",
                kind,
                cpp_name,
                current_module->name,
                owner_names));
        }

        return matches.front();
    }

    BoundClassMatch find_bound_class_any(std::string_view cpp_name) const
    {
        for (const auto& module_decl : modules_)
        {
            for (const auto& class_decl : module_decl.classes)
            {
                if (class_decl.cpp_name == cpp_name)
                {
                    return BoundClassMatch{&module_decl, &class_decl};
                }
            }
        }

        return {};
    }

    BoundClassMatch find_bound_class_in_module(const ModuleDecl& module_decl, std::string_view cpp_name) const
    {
        for (const auto& class_decl : module_decl.classes)
        {
            if (class_decl.cpp_name == cpp_name)
            {
                return BoundClassMatch{&module_decl, &class_decl};
            }
        }

        return {};
    }

    BoundClassMatch find_visible_bound_class(const ModuleDecl* current_module, std::string_view cpp_name) const
    {
        return find_visible_unique_match<BoundClassMatch>("class", cpp_name, current_module,
            [&](const ModuleDecl* visible_module) {
                if (visible_module == nullptr)
                {
                    return find_bound_class_any(cpp_name);
                }
                return find_bound_class_in_module(*visible_module, cpp_name);
            });
    }

    BoundEnumMatch find_bound_enum_any(std::string_view cpp_name) const
    {
        for (const auto& module_decl : modules_)
        {
            for (const auto& enum_decl : module_decl.enums)
            {
                if (enum_decl.cpp_name == cpp_name)
                {
                    return BoundEnumMatch{&module_decl, &enum_decl};
                }
            }
        }

        return {};
    }

    BoundEnumMatch find_bound_enum_in_module(const ModuleDecl& module_decl, std::string_view cpp_name) const
    {
        for (const auto& enum_decl : module_decl.enums)
        {
            if (enum_decl.cpp_name == cpp_name)
            {
                return BoundEnumMatch{&module_decl, &enum_decl};
            }
        }

        return {};
    }

    BoundEnumMatch find_visible_bound_enum(const ModuleDecl* current_module, std::string_view cpp_name) const
    {
        return find_visible_unique_match<BoundEnumMatch>("enum", cpp_name, current_module,
            [&](const ModuleDecl* visible_module) {
                if (visible_module == nullptr)
                {
                    return find_bound_enum_any(cpp_name);
                }
                return find_bound_enum_in_module(*visible_module, cpp_name);
            });
    }

    bool is_secondary_base_for_any_class(std::string_view cpp_name) const
    {
        for (const auto& module_decl : modules_)
        {
            for (const auto& class_decl : module_decl.classes)
            {
                for (std::size_t index = 1; index < class_decl.base_classes.size(); ++index)
                {
                    if (class_decl.base_classes[index].cpp_name == cpp_name)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void assign_bound_std_string_managed_converter(TypeRef& type_ref, const ModuleDecl* current_module) const
    {
        if (current_module == nullptr || type_ref.cpp_name != "std::string")
        {
            return;
        }

        const auto match = find_visible_bound_class(current_module, "std::string");
        if (match.module_decl == nullptr || match.class_decl == nullptr)
        {
            return;
        }

        if ((type_ref.is_reference || type_ref.is_pointer) && is_secondary_base_for_any_class(type_ref.cpp_name))
        {
            return;
        }

        const std::string csharp_namespace = csharp_namespace_name_for(*match.module_decl, *match.class_decl);
        const std::string managed_class = managed_class_name_for(*match.module_decl, *match.class_decl);
        const std::string qualified_managed_class = "global::" + csharp_namespace + "." + managed_class;

        type_ref.managed_type_name = qualified_managed_class;

        if (!type_ref.is_pointer && !type_ref.is_reference)
        {
            type_ref.managed_to_pinvoke_expression =
                "global::CsBind23.Generated.CsBind23Utf8Interop.StringToNative({value} is null ? string.Empty : {value}.ToString())";
            type_ref.managed_from_pinvoke_expression =
                qualified_managed_class + ".FromManaged(global::CsBind23.Generated.CsBind23Utf8Interop.NativeToString({value}))";
            type_ref.managed_finalize_to_pinvoke_statement =
                "global::CsBind23.Generated.CsBind23Utf8Interop.Free({pinvoke})";
            type_ref.managed_finalize_from_pinvoke_statement =
                "global::CsBind23.Generated.CsBind23Utf8Interop.Free({pinvoke})";
            return;
        }

        if (type_ref.is_reference || type_ref.is_pointer)
        {
            type_ref.managed_to_pinvoke_expression = "({value} is null ? System.IntPtr.Zero : {value}.EnsureHandle())";

            if (!type_ref.is_const)
            {
                type_ref.managed_finalize_to_pinvoke_statement = "if ({managed} is not null)\n{\n    {managed}.InvalidateManagedCache();\n}";
            }

            type_ref.managed_from_pinvoke_expression = qualified_managed_class + ".FromBorrowedHandle({value})";
        }
    }

    void assign_bound_class_managed_converter(TypeRef& type_ref, const ModuleDecl* current_module) const
    {
        if (type_ref.cpp_name.empty() || type_ref.cpp_name == "std::string")
        {
            return;
        }

        const auto match = find_visible_bound_class(current_module, type_ref.cpp_name);
        if (match.module_decl == nullptr || match.class_decl == nullptr)
        {
            return;
        }

        const std::string csharp_namespace = csharp_namespace_name_for(*match.module_decl, *match.class_decl);
        const std::string managed_class = managed_class_name_for(*match.module_decl, *match.class_decl);
        const std::string qualified_managed_class = "global::" + csharp_namespace + "." + managed_class;
        const std::string ownership_type = "global::CsBind23.Generated.ItemOwnership";
        const std::string ownership_literal = ownership_type + (!type_ref.is_reference && !type_ref.is_pointer ? ".Owned" : ".Borrowed");
        const std::string runtime_class =
            emit::format_csharp_name(*match.module_decl, CSharpNameKind::Class, match.module_decl->name + "Runtime");
        const std::string wrap_method = emit::format_csharp_name(
            *match.module_decl, CSharpNameKind::Class, "WrapPolymorphic_" + match.class_decl->name);

        type_ref.managed_type_name = qualified_managed_class;
        type_ref.managed_to_pinvoke_expression = "({value} is null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
        type_ref.managed_from_pinvoke_expression = std::format(
            "({})global::{}.{}.{}({{value}}, {})",
            qualified_managed_class,
            csharp_namespace,
            runtime_class,
            wrap_method,
            ownership_literal);
    }

    void retarget_bound_class_managed_converter(TypeRef& type_ref, const ModuleDecl* current_module) const
    {
        if (!type_ref.has_managed_converter() || type_ref.managed_type_name.empty())
        {
            return;
        }

        const auto match = find_visible_bound_class(current_module, type_ref.cpp_name);
        if (match.module_decl == nullptr || match.class_decl == nullptr)
        {
            return;
        }

        const std::string qualified_managed_class =
            "global::" + csharp_namespace_name_for(*match.module_decl, *match.class_decl) + "."
            + managed_class_name_for(*match.module_decl, *match.class_decl);
        const std::string legacy_namespace = match.module_decl->name == stl_module_name()
            ? std::string(default_stl_csharp_namespace())
            : make_safe_csharp_namespace_segment(
                emit::format_csharp_name(*match.module_decl, CSharpNameKind::Class, match.module_decl->name));
        const std::string prior_managed_type_name = type_ref.managed_type_name;
        const std::string legacy_runtime_name = "global::" + legacy_namespace + "."
            + emit::format_csharp_name(*match.module_decl, CSharpNameKind::Class, match.module_decl->name + "Runtime");
        const std::string qualified_runtime_name = "global::" + csharp_namespace_name_for(*match.module_decl) + "."
            + emit::format_csharp_name(*match.module_decl, CSharpNameKind::Class, match.module_decl->name + "Runtime");

        type_ref.managed_type_name = qualified_managed_class;
        type_ref.managed_to_pinvoke_expression =
            replace_all(type_ref.managed_to_pinvoke_expression, prior_managed_type_name, qualified_managed_class);
        type_ref.managed_to_pinvoke_expression =
            replace_all(type_ref.managed_to_pinvoke_expression, legacy_runtime_name, qualified_runtime_name);
        type_ref.managed_from_pinvoke_expression =
            replace_all(type_ref.managed_from_pinvoke_expression, prior_managed_type_name, qualified_managed_class);
        type_ref.managed_from_pinvoke_expression =
            replace_all(type_ref.managed_from_pinvoke_expression, legacy_runtime_name, qualified_runtime_name);
        type_ref.managed_finalize_to_pinvoke_statement = replace_all(
            type_ref.managed_finalize_to_pinvoke_statement, prior_managed_type_name, qualified_managed_class);
        type_ref.managed_finalize_to_pinvoke_statement = replace_all(
            type_ref.managed_finalize_to_pinvoke_statement, legacy_runtime_name, qualified_runtime_name);
        type_ref.managed_finalize_from_pinvoke_statement = replace_all(
            type_ref.managed_finalize_from_pinvoke_statement, prior_managed_type_name, qualified_managed_class);
        type_ref.managed_finalize_from_pinvoke_statement = replace_all(
            type_ref.managed_finalize_from_pinvoke_statement, legacy_runtime_name, qualified_runtime_name);
    }

    void assign_bound_enum_managed_converter(TypeRef& type_ref, const ModuleDecl* current_module) const
    {
        if (type_ref.cpp_name.empty())
        {
            return;
        }

        const auto match = find_visible_bound_enum(current_module, type_ref.cpp_name);
        if (match.module_decl == nullptr || match.enum_decl == nullptr)
        {
            return;
        }

        type_ref.managed_type_name = "global::" + csharp_namespace_name_for(*match.module_decl);
        type_ref.managed_type_name += "." + managed_enum_name_for(*match.module_decl, *match.enum_decl);
        type_ref.pinvoke_name = type_ref.managed_type_name;
    }

    void finalize_visible_type_ref(TypeRef& type_ref, const ModuleDecl* current_module) const
    {
        if (type_ref.cpp_name == "std::string")
        {
            assign_bound_std_string_managed_converter(type_ref, current_module);
        }

        retarget_bound_class_managed_converter(type_ref, current_module);

        if (!type_ref.has_managed_converter())
        {
            assign_bound_class_managed_converter(type_ref, current_module);
        }

        assign_bound_enum_managed_converter(type_ref, current_module);
    }

    void finalize_visible_type_refs(std::vector<ModuleDecl>& modules) const
    {
        for (auto& module_decl : modules)
        {
            for (auto& function_decl : module_decl.functions)
            {
                finalize_visible_type_ref(function_decl.return_type, &module_decl);
                for (auto& parameter : function_decl.parameters)
                {
                    finalize_visible_type_ref(parameter.type, &module_decl);
                }
            }

            for (auto& class_decl : module_decl.classes)
            {
                for (auto& method_decl : class_decl.methods)
                {
                    finalize_visible_type_ref(method_decl.return_type, &module_decl);
                    for (auto& parameter : method_decl.parameters)
                    {
                        finalize_visible_type_ref(parameter.type, &module_decl);
                    }
                }

                for (auto& property_decl : class_decl.properties)
                {
                    finalize_visible_type_ref(property_decl.type, &module_decl);
                }
            }

            for (auto& enum_decl : module_decl.enums)
            {
                finalize_visible_type_ref(enum_decl.underlying_type, &module_decl);
            }
        }
    }

    std::vector<ModuleDecl> resolved_modules() const
    {
        validate_imports();
        std::vector<ModuleDecl> modules = modules_;
        finalize_stl_module(modules);
        finalize_visible_type_refs(modules);
        return modules;
    }

    template <typename Type>
    void apply_managed_converter(TypeRef& type_ref, const ModuleDecl* current_module, bool for_return_type) const
    {
        assign_bound_std_string_managed_converter(type_ref, current_module);
        if (type_ref.has_managed_converter())
        {
            return;
        }

        assign_managed_converter<Type>(type_ref, for_return_type);
        if (type_ref.has_managed_converter())
        {
            return;
        }

        using NoRef = std::remove_reference_t<Type>;
        using Bare = std::remove_cv_t<NoRef>;
        if constexpr (!std::is_same_v<Bare, Type>)
        {
            assign_managed_converter<Bare>(type_ref, for_return_type);
            if (type_ref.has_managed_converter())
            {
                return;
            }
        }

        using Base = std::remove_cv_t<std::remove_pointer_t<Bare>>;
        if constexpr (!std::is_same_v<Base, Bare>)
        {
            assign_managed_converter<Base>(type_ref, for_return_type);
            if (type_ref.has_managed_converter())
            {
                return;
            }
        }

        assign_bound_class_managed_converter(type_ref, current_module);
        if (type_ref.has_managed_converter())
        {
            return;
        }

        using EnumBase = std::remove_cv_t<std::remove_pointer_t<NoRef>>;
        if constexpr (std::is_enum_v<EnumBase>)
        {
            assign_bound_enum_managed_converter(type_ref, current_module);
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

    ModuleBuilder& import_module(std::string_view module_name)
    {
        if (module_name == module_decl_->name)
        {
            throw std::runtime_error(std::format("Module '{}' cannot import itself.", module_decl_->name));
        }

        const std::string imported_name(module_name);
        if (std::find(module_decl_->imported_modules.begin(), module_decl_->imported_modules.end(), imported_name)
            == module_decl_->imported_modules.end())
        {
            module_decl_->imported_modules.push_back(imported_name);
        }
        return *this;
    }

    ModuleBuilder& csharp_namespace(std::string_view namespace_name)
    {
        module_decl_->csharp_namespace = std::string(namespace_name);
        return *this;
    }

    std::string_view name() const
    {
        return module_decl_->name;
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
        function_decl.return_type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
        function_decl.return_ownership = return_ownership;
        function_decl.trailing_default_argument_count =
            trailing_default_argument_count > sizeof...(Args) ? sizeof...(Args) : trailing_default_argument_count;
        function_decl.pinvoke_only = pinvoke_only;
        function_decl.csharp_attributes = std::move(csharp_attributes);
        function_decl.csharp_comment = std::move(csharp_comment);

        function_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((function_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>(module_decl_)})),
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

    template <template <typename...> typename ClassTemplate, typename FirstSpec, typename... RestSpecs>
    GenericClassBuilder<
        detail::instantiate_template_from_spec_t<ClassTemplate, FirstSpec>,
        detail::instantiate_template_from_spec_t<ClassTemplate, RestSpecs>...>
    class_generic();

    template <template <typename...> typename ClassTemplate, typename FirstSpec, typename... RestSpecs>
    GenericClassBuilder<
        detail::instantiate_template_from_spec_t<ClassTemplate, FirstSpec>,
        detail::instantiate_template_from_spec_t<ClassTemplate, RestSpecs>...>
    class_generic(std::string_view name);

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
    ClassBuilder(BindingsGenerator& owner, ModuleDecl& module_decl, ClassDecl& class_decl);

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

    ClassBuilder& csharp_interface(std::string_view interface_name)
    {
        class_decl_->csharp_interfaces.push_back(std::string(interface_name));
        return *this;
    }

    ClassBuilder& csharp_namespace(std::string_view namespace_name)
    {
        class_decl_->csharp_namespace = std::string(namespace_name);
        return *this;
    }

    ClassBuilder& csharp_code(std::string_view member_code)
    {
        class_decl_->csharp_member_snippets.push_back(std::string(member_code));
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
        if constexpr (std::is_member_function_pointer_v<decltype(MethodPtr)>)
        {
            return def(name, MethodPtr, CppSymbol{detail::unqualified_name(detail::function_symbol_name<MethodPtr>())}, std::forward<Options>(options)...);
        }
        else
        {
            return def(name, MethodPtr, CppSymbol{detail::function_symbol_name<MethodPtr>()},
                std::forward<Options>(options)...);
        }
    }

    template <typename... Args> ClassBuilder& ctor(Ownership ownership = Ownership::Auto)
    {
        FunctionDecl ctor_decl;
        ctor_decl.name = "__ctor";
        ctor_decl.cpp_symbol = class_decl_->cpp_name;
        ctor_decl.return_type = owner_->make_bound_return_type_ref<void*>(module_decl_);
        ctor_decl.return_ownership = ownership;
        ctor_decl.is_constructor = true;
        ctor_decl.class_name = class_decl_->cpp_name;

        ctor_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((ctor_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>(module_decl_)})),
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
            def_options.cpp_symbol, def_options.exported_name, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only, def_options.csharp_private, false);
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
            def_options.cpp_symbol, def_options.exported_name, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only, def_options.csharp_private, false);
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename... Args, typename... Options>
    ClassBuilder& def(std::string_view name, ReturnType (*function_ptr)(ClassType&, Args...), Options&&... options)
    {
        (void)function_ptr;
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        add_method<ClassType, ReturnType, Args...>(name, false, def_options.return_ownership,
            def_options.trailing_default_argument_count, false, false, false,
            def_options.cpp_symbol, def_options.exported_name, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only, def_options.csharp_private, true);
        return *this;
    }

    template <typename ClassType, typename ReturnType, typename... Args, typename... Options>
    ClassBuilder& def(
        std::string_view name, ReturnType (*function_ptr)(const ClassType&, Args...), Options&&... options)
    {
        (void)function_ptr;
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        add_method<ClassType, ReturnType, Args...>(name, true, def_options.return_ownership,
            def_options.trailing_default_argument_count, false, false, false,
            def_options.cpp_symbol, def_options.exported_name, def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only, def_options.csharp_private, true);
        return *this;
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs>
        requires(!std::is_class_v<std::remove_cv_t<decltype(FirstMethodPtr)>>
            && (!std::is_class_v<std::remove_cv_t<decltype(RestMethodPtrs)>> && ...))
    ClassBuilder& def_generic()
    {
        return def_generic<FirstMethodPtr, RestMethodPtrs...>(detail::function_export_name<FirstMethodPtr>());
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs, typename... Options>
        requires(!std::is_class_v<std::remove_cv_t<decltype(FirstMethodPtr)>>
            && (!std::is_class_v<std::remove_cv_t<decltype(RestMethodPtrs)>> && ...))
    ClassBuilder& def_generic(std::string_view name, Options&&... options)
    {
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        detail::for_each_nontype_indexed<FirstMethodPtr, RestMethodPtrs...>([&]<auto MethodPtr>(std::size_t index) {
            add_generic_method_instantiation_auto<MethodPtr>(
                name, def_options, detail::generic_cpp_symbol_for(def_options.cpp_symbols, index));
        });
        return *this;
    }

    template <auto MethodResolver, typename FirstSpec, typename... RestSpecs>
        requires(std::is_class_v<std::remove_cv_t<decltype(MethodResolver)>>)
    ClassBuilder& def_generic()
    {
        constexpr auto first_method_ptr = detail::resolve_nontype_from_spec<MethodResolver, FirstSpec>();
        return def_generic<MethodResolver, FirstSpec, RestSpecs...>(
            detail::function_export_name<first_method_ptr>());
    }

    template <auto MethodResolver, typename FirstSpec, typename... RestSpecs, typename... Options>
        requires(std::is_class_v<std::remove_cv_t<decltype(MethodResolver)>>)
    ClassBuilder& def_generic(std::string_view name, Options&&... options)
    {
        const auto def_options = make_method_def_options(std::forward<Options>(options)...);
        constexpr auto first_method_ptr = detail::resolve_nontype_from_spec<MethodResolver, FirstSpec>();
        add_generic_method_instantiation_auto<first_method_ptr>(name, def_options,
            resolve_generic_method_cpp_symbol<first_method_ptr, FirstSpec>(
                def_options, detail::generic_cpp_symbol_for(def_options.cpp_symbols, 0)));

        std::size_t index = 1;
        ([&] {
            constexpr auto method_ptr = detail::resolve_nontype_from_spec<MethodResolver, RestSpecs>();
            add_generic_method_instantiation_auto<method_ptr>(name, def_options,
                resolve_generic_method_cpp_symbol<method_ptr, RestSpecs>(
                    def_options, detail::generic_cpp_symbol_for(def_options.cpp_symbols, index++)));
        }(),
            ...);
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
        property_decl.type = owner_->make_bound_type_ref<BareFieldType>(module_decl_);
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
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        property_decl.type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        std::string exported_name;
        std::string_view cpp_symbol = {};
        std::vector<std::string> cpp_symbols;
        std::vector<std::string> csharp_attributes;
        std::string csharp_comment;
        std::vector<Arg> arg_options;
        bool csharp_private = false;
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

    static void apply_method_def_option(MethodDefOptions& options, Private)
    {
        options.csharp_private = true;
    }

    static void apply_method_def_option(MethodDefOptions& options, ExportName export_name)
    {
        options.exported_name = std::string(export_name.value);
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
            "Unsupported ClassBuilder::def option. Supported tags: Ownership, WithDefaults, Virtual, PInvoke, Private, ExportName, CppSymbol, CppSymbols, Attribute, Comment, Arg.");
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    void add_method(std::string_view name, bool is_const, Ownership return_ownership,
        std::size_t trailing_default_argument_count, bool allow_override,
        bool is_property_getter, bool is_property_setter, std::string_view cpp_symbol,
        std::vector<std::string> csharp_attributes, std::string csharp_comment,
        std::vector<Arg> arg_options,
        bool is_field_accessor, bool pinvoke_only = false, bool csharp_private = false,
        bool is_extension_method = false)
    {
        add_method<ClassType, ReturnType, Args...>(name, is_const, return_ownership,
            trailing_default_argument_count, allow_override, is_property_getter, is_property_setter, cpp_symbol,
            {}, std::move(csharp_attributes), std::move(csharp_comment), std::move(arg_options),
            is_field_accessor, pinvoke_only, csharp_private, is_extension_method);
    }

    template <typename ClassType, typename ReturnType, typename... Args>
    void add_method(std::string_view name, bool is_const, Ownership return_ownership,
        std::size_t trailing_default_argument_count = 0, bool allow_override = false,
        bool is_property_getter = false, bool is_property_setter = false, std::string_view cpp_symbol = {},
        std::string exported_name = {},
        std::vector<std::string> csharp_attributes = {}, std::string csharp_comment = {},
        std::vector<Arg> arg_options = {},
        bool is_field_accessor = false, bool pinvoke_only = false, bool csharp_private = false,
        bool is_extension_method = false)
    {
        FunctionDecl method_decl;
        method_decl.name = std::string(name);
        method_decl.exported_name = exported_name.empty() ? std::string(name) : std::move(exported_name);
        method_decl.cpp_symbol = cpp_symbol.empty() ? std::string(name) : std::string(cpp_symbol);
        method_decl.return_type = owner_->make_bound_return_type_ref<ReturnType>(module_decl_);
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
        method_decl.csharp_private = csharp_private;
        method_decl.is_extension_method = is_extension_method;
        method_decl.class_name = class_decl_->cpp_name;
        method_decl.virtual_slot_name = std::string(name);
        method_decl.csharp_attributes = std::move(csharp_attributes);
        method_decl.csharp_comment = std::move(csharp_comment);

        method_decl.parameters.reserve(sizeof...(Args));
        std::size_t index = 0;
        ((method_decl.parameters.push_back(
             ParameterDecl{"arg" + std::to_string(index++), owner_->make_bound_param_type_ref<Args>(module_decl_)})),
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

    template <auto MethodPtr, typename Spec>
    static std::string resolve_generic_method_cpp_symbol(
        const MethodDefOptions& def_options, std::string_view cpp_symbol_override = {})
    {
        if (!cpp_symbol_override.empty())
        {
            return std::string(cpp_symbol_override);
        }

        if (!def_options.cpp_symbol.empty())
        {
            return detail::instantiate_symbol_from_spec<Spec>(std::string(def_options.cpp_symbol));
        }

        return detail::instantiate_symbol_from_spec<Spec>(std::string(detail::function_symbol_name<MethodPtr>()));
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
        const std::string unique_export_name = (def_options.exported_name.empty() ? group_name : def_options.exported_name) + unique_suffix;

        add_method<ClassType, ReturnType, Args...>(group_name, false, def_options.return_ownership,
            def_options.trailing_default_argument_count, def_options.allow_override, false, false,
            cpp_symbol_override, unique_export_name,
            def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only, def_options.csharp_private, false);

        FunctionDecl& inserted = class_decl_->methods.back();
        detail::mark_generic_instantiation(inserted, group_name);
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
        const std::string unique_export_name = (def_options.exported_name.empty() ? group_name : def_options.exported_name) + unique_suffix;

        add_method<ClassType, ReturnType, Args...>(group_name, true, def_options.return_ownership,
            def_options.trailing_default_argument_count, def_options.allow_override, false, false,
            cpp_symbol_override, unique_export_name,
            def_options.csharp_attributes, def_options.csharp_comment,
            def_options.arg_options, false, def_options.pinvoke_only, def_options.csharp_private, false);

        FunctionDecl& inserted = class_decl_->methods.back();
        detail::mark_generic_instantiation(inserted, group_name);
    }

    BindingsGenerator* owner_;
    ModuleDecl* module_decl_;
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
            ClassBuilder(*owner_, *module_decl_, module_decl_->classes[class_index]).template ctor<Args...>(ownership);
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

    GenericClassBuilder& csharp_interface(std::string_view interface_name)
    {
        for (const auto class_index : class_indices_)
        {
            module_decl_->classes[class_index].csharp_interfaces.push_back(std::string(interface_name));
        }
        return *this;
    }

    GenericClassBuilder& csharp_namespace(std::string_view namespace_name)
    {
        for (const auto class_index : class_indices_)
        {
            module_decl_->classes[class_index].csharp_namespace = std::string(namespace_name);
        }
        return *this;
    }

    GenericClassBuilder& csharp_code(std::string_view member_code)
    {
        for (const auto class_index : class_indices_)
        {
            module_decl_->classes[class_index].csharp_member_snippets.push_back(std::string(member_code));
        }
        return *this;
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs>
        requires(!std::is_class_v<std::remove_cv_t<decltype(FirstMethodPtr)>>
            && (!std::is_class_v<std::remove_cv_t<decltype(RestMethodPtrs)>> && ...))
    GenericClassBuilder& def()
    {
        return def_generic<FirstMethodPtr, RestMethodPtrs...>(detail::function_export_name<FirstMethodPtr>());
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs, typename... Options>
        requires(!std::is_class_v<std::remove_cv_t<decltype(FirstMethodPtr)>>
            && (!std::is_class_v<std::remove_cv_t<decltype(RestMethodPtrs)>> && ...))
    GenericClassBuilder& def(std::string_view name, Options&&... options)
    {
        return def_generic<FirstMethodPtr, RestMethodPtrs...>(name, std::forward<Options>(options)...);
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs>
        requires(!std::is_class_v<std::remove_cv_t<decltype(FirstMethodPtr)>>
            && (!std::is_class_v<std::remove_cv_t<decltype(RestMethodPtrs)>> && ...))
    GenericClassBuilder& def_generic()
    {
        return def_generic<FirstMethodPtr, RestMethodPtrs...>(detail::function_export_name<FirstMethodPtr>());
    }

    template <auto FirstMethodPtr, auto... RestMethodPtrs, typename... Options>
        requires(!std::is_class_v<std::remove_cv_t<decltype(FirstMethodPtr)>>
            && (!std::is_class_v<std::remove_cv_t<decltype(RestMethodPtrs)>> && ...))
    GenericClassBuilder& def_generic(std::string_view name, Options&&... options)
    {
        constexpr std::size_t method_count = 1 + sizeof...(RestMethodPtrs);
        constexpr std::size_t class_count = sizeof...(ClassTypes);
        static_assert(method_count == class_count,
            "GenericClassBuilder::def requires one method pointer per class instantiation.");

        auto options_tuple = std::make_tuple(std::forward<Options>(options)...);
        detail::for_each_nontype_indexed<FirstMethodPtr, RestMethodPtrs...>([&]<auto MethodPtr>(std::size_t index) {
            ClassBuilder class_builder(*owner_, *module_decl_, module_decl_->classes[class_indices_[index]]);
            std::apply(
                [&]<typename... OptionTypes>(const OptionTypes&... unpacked_options) {
                    std::string selected_cpp_symbol;
                    std::string selected_export_name;
                    ([&] {
                        using OptionType = std::decay_t<OptionTypes>;
                        if constexpr (std::is_same_v<OptionType, CppSymbols>)
                        {
                            const std::string_view symbol = detail::generic_cpp_symbol_for(unpacked_options.values, index);
                            if (!symbol.empty())
                            {
                                selected_cpp_symbol = std::string(symbol);
                            }
                        }
                        else if constexpr (std::is_same_v<OptionType, ExportName>)
                        {
                            selected_export_name = std::string(unpacked_options.value);
                        }
                    }(),
                        ...);

                    if (selected_export_name.empty())
                    {
                        selected_export_name = std::string(name);
                    }

                    if (selected_cpp_symbol.empty())
                    {
                        class_builder.template def<MethodPtr>(name, ExportName{selected_export_name}, unpacked_options...);
                    }
                    else
                    {
                        class_builder.template def<MethodPtr>(
                            name, CppSymbol{selected_cpp_symbol}, ExportName{selected_export_name}, unpacked_options...);
                    }
                },
                options_tuple);
        });

        return *this;
    }

    template <auto MethodResolver, typename... Options>
        requires(std::is_class_v<std::remove_cvref_t<decltype(MethodResolver)>>)
    GenericClassBuilder& def_generic(std::string_view name, Options&&... options)
    {
        auto options_tuple = std::make_tuple(std::forward<Options>(options)...);
        def_generic_with_resolver<MethodResolver>(name, options_tuple, std::index_sequence_for<ClassTypes...>{});
        return *this;
    }

    template <auto MethodResolver, typename... Options>
        requires(std::is_class_v<std::remove_cvref_t<decltype(MethodResolver)>>)
    GenericClassBuilder& def(std::string_view name, Options&&... options)
    {
        return def_generic<MethodResolver>(name, std::forward<Options>(options)...);
    }

private:
    template <auto MethodResolver, typename OptionsTuple, std::size_t... Indices>
    void def_generic_with_resolver(
        std::string_view name, const OptionsTuple& options_tuple, std::index_sequence<Indices...>)
    {
        (def_generic_with_resolver_for_index<MethodResolver, Indices>(name, options_tuple), ...);
    }

    template <auto MethodResolver, std::size_t Index, typename OptionsTuple>
    void def_generic_with_resolver_for_index(std::string_view name, const OptionsTuple& options_tuple)
    {
        using ClassType = std::tuple_element_t<Index, std::tuple<ClassTypes...>>;
        constexpr auto method_ptr = MethodResolver.template operator()<ClassType>();
        ClassBuilder class_builder(*owner_, *module_decl_, module_decl_->classes[class_indices_[Index]]);
        std::apply(
            [&]<typename... OptionTypes>(const OptionTypes&... unpacked_options) {
                class_builder.template def<method_ptr>(name, unpacked_options...);
            },
            options_tuple);
    }

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
    return ClassBuilder(*owner_, *module_decl_, module_decl_->classes.back());
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

template <template <typename...> typename ClassTemplate, typename FirstSpec, typename... RestSpecs>
GenericClassBuilder<
    detail::instantiate_template_from_spec_t<ClassTemplate, FirstSpec>,
    detail::instantiate_template_from_spec_t<ClassTemplate, RestSpecs>...>
ModuleBuilder::class_generic()
{
    return class_generic<
        detail::instantiate_template_from_spec_t<ClassTemplate, FirstSpec>,
        detail::instantiate_template_from_spec_t<ClassTemplate, RestSpecs>...>();
}

template <template <typename...> typename ClassTemplate, typename FirstSpec, typename... RestSpecs>
GenericClassBuilder<
    detail::instantiate_template_from_spec_t<ClassTemplate, FirstSpec>,
    detail::instantiate_template_from_spec_t<ClassTemplate, RestSpecs>...>
ModuleBuilder::class_generic(std::string_view name)
{
    return class_generic<
        detail::instantiate_template_from_spec_t<ClassTemplate, FirstSpec>,
        detail::instantiate_template_from_spec_t<ClassTemplate, RestSpecs>...>(name);
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
    enum_decl.underlying_type = owner_->make_bound_type_ref<Underlying>(module_decl_);
    enum_decl.is_flags = enum_options.is_flags;
    enum_decl.csharp_attributes = enum_options.csharp_attributes;
    module_decl_->enums.push_back(std::move(enum_decl));
    return EnumBuilder(*owner_, module_decl_->enums.back());
}

} // namespace csbind23
