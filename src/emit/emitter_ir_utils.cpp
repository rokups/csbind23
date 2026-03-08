#include "emitter_ir_utils.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <unordered_set>

namespace csbind23::emit
{

std::string exported_symbol_name(const FunctionDecl& function_decl)
{
    return function_decl.exported_name.empty() ? function_decl.name : function_decl.exported_name;
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

namespace
{

const ModuleDecl* find_module_by_name(const std::vector<ModuleDecl>& modules, std::string_view module_name)
{
    for (const auto& module_decl : modules)
    {
        if (module_decl.name == module_name)
        {
            return &module_decl;
        }
    }

    return nullptr;
}

std::vector<const ModuleDecl*> visible_modules(const std::vector<ModuleDecl>& modules, const ModuleDecl& module_decl)
{
    std::vector<const ModuleDecl*> visible;
    visible.push_back(&module_decl);

    for (const auto& imported_module_name : module_decl.imported_modules)
    {
        if (imported_module_name == module_decl.name)
        {
            throw std::runtime_error(std::format("Module '{}' cannot import itself.", module_decl.name));
        }

        const ModuleDecl* imported_module = find_module_by_name(modules, imported_module_name);
        if (imported_module == nullptr)
        {
            throw std::runtime_error(
                std::format("Module '{}' imports unknown module '{}'.", module_decl.name, imported_module_name));
        }

        if (std::find(visible.begin(), visible.end(), imported_module) == visible.end())
        {
            visible.push_back(imported_module);
        }
    }

    return visible;
}

template <typename Match>
Match require_unique_match(
    std::vector<Match>&& matches, std::string_view kind, std::string_view cpp_name, std::string_view context_module)
{
    if (matches.empty())
    {
        return {};
    }

    if (matches.size() > 1)
    {
        std::string owners;
        for (std::size_t index = 0; index < matches.size(); ++index)
        {
            if (index > 0)
            {
                owners += ", ";
            }
            owners += matches[index].module_decl->name;
        }

        throw std::runtime_error(std::format(
            "Ambiguous {} binding for C++ type '{}' in module '{}'. Visible owners: {}.",
            kind,
            cpp_name,
            context_module,
            owners));
    }

    return matches.front();
}

bool class_derives_from_impl(const std::vector<ModuleDecl>& modules, const ClassDecl& class_decl,
    std::string_view base_cpp_name, std::unordered_set<std::string>& visited)
{
    for (const auto& base_decl : class_decl.base_classes)
    {
        if (base_decl.cpp_name == base_cpp_name)
        {
            return true;
        }

        if (!visited.insert(base_decl.cpp_name).second)
        {
            continue;
        }

        const ClassBinding base_class = find_unique_class_by_cpp_name(modules, base_decl.cpp_name);
        if (base_class.class_decl != nullptr
            && class_derives_from_impl(modules, *base_class.class_decl, base_cpp_name, visited))
        {
            return true;
        }
    }

    return false;
}

} // namespace

ClassBinding find_unique_class_by_cpp_name(const std::vector<ModuleDecl>& modules, std::string_view cpp_name)
{
    std::vector<ClassBinding> matches;
    for (const auto& module_decl : modules)
    {
        if (const ClassDecl* class_decl = find_class_by_cpp_name(module_decl, cpp_name); class_decl != nullptr)
        {
            matches.push_back(ClassBinding{&module_decl, class_decl});
        }
    }

    return require_unique_match(std::move(matches), "class", cpp_name, "<all>");
}

ClassBinding find_visible_class_by_cpp_name(
    const std::vector<ModuleDecl>& modules, const ModuleDecl& module_decl, std::string_view cpp_name)
{
    std::vector<ClassBinding> matches;
    for (const ModuleDecl* visible_module : visible_modules(modules, module_decl))
    {
        if (const ClassDecl* class_decl = find_class_by_cpp_name(*visible_module, cpp_name); class_decl != nullptr)
        {
            matches.push_back(ClassBinding{visible_module, class_decl});
        }
    }

    return require_unique_match(std::move(matches), "class", cpp_name, module_decl.name);
}

std::vector<ClassBinding> secondary_base_classes(
    const std::vector<ModuleDecl>& modules, const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    std::vector<ClassBinding> bases;
    if (!class_decl.base_classes.empty())
    {
        for (std::size_t index = 1; index < class_decl.base_classes.size(); ++index)
        {
            const auto base_class = find_visible_class_by_cpp_name(modules, module_decl, class_decl.base_classes[index].cpp_name);
            if (base_class.class_decl != nullptr)
            {
                bases.push_back(base_class);
            }
        }
    }
    return bases;
}

bool class_derives_from(
    const std::vector<ModuleDecl>& modules, const ClassDecl& class_decl, std::string_view base_cpp_name)
{
    std::unordered_set<std::string> visited;
    return class_derives_from_impl(modules, class_decl, base_cpp_name, visited);
}

std::vector<ClassBinding> polymorphic_assignable_classes(
    const std::vector<ModuleDecl>& modules, std::string_view base_cpp_name)
{
    std::vector<ClassBinding> classes;

    const ClassBinding base_class = find_unique_class_by_cpp_name(modules, base_cpp_name);
    if (base_class.class_decl != nullptr)
    {
        classes.push_back(base_class);
    }

    for (const auto& module_decl : modules)
    {
        for (const auto& class_decl : module_decl.classes)
        {
            if (class_decl.cpp_name == base_cpp_name)
            {
                continue;
            }

            if (class_derives_from(modules, class_decl, base_cpp_name))
            {
                classes.push_back(ClassBinding{&module_decl, &class_decl});
            }
        }
    }

    return classes;
}

bool is_wrapper_visible_method(const FunctionDecl& method_decl, bool exclude_pinvoke_only)
{
    if (exclude_pinvoke_only && method_decl.pinvoke_only)
    {
        return false;
    }

    return !method_decl.is_constructor && method_decl.is_method && !method_decl.is_property_getter
        && !method_decl.is_property_setter;
}

std::string method_signature_key(const FunctionDecl& method_decl, MethodSignatureKind signature_kind)
{
    std::string key = method_decl.name;
    key += method_decl.is_const ? "|const|" : "|mutable|";
    key += signature_kind == MethodSignatureKind::PInvoke ? method_decl.return_type.pinvoke_name : method_decl.return_type.c_abi_name;
    key += "|";
    for (const auto& parameter : method_decl.parameters)
    {
        key += signature_kind == MethodSignatureKind::PInvoke ? parameter.type.pinvoke_name : parameter.type.c_abi_name;
        key += parameter.type.is_const ? "#c" : "#m";
        key += parameter.type.is_pointer ? "#p" : "#v";
        key += parameter.type.is_reference ? "#r" : "#v";
        key += ";";
    }
    return key;
}

std::vector<EmittedMethod> collect_emitted_methods(
    const std::vector<ModuleDecl>& modules,
    const ModuleDecl& module_decl,
    const ClassDecl& class_decl,
    const MethodCollectionOptions& options)
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

        if (is_wrapper_visible_method(method_decl, options.exclude_pinvoke_only))
        {
            signatures.insert(method_signature_key(method_decl, options.signature_kind));
        }
        emitted_methods.push_back(EmittedMethod{method_decl, false});
    }

    for (const auto& secondary_base : secondary_base_classes(modules, module_decl, class_decl))
    {
        for (const auto& base_method : secondary_base.class_decl->methods)
        {
            if (!is_wrapper_visible_method(base_method, options.exclude_pinvoke_only))
            {
                continue;
            }

            const std::string signature = method_signature_key(base_method, options.signature_kind);
            if (signatures.contains(signature))
            {
                continue;
            }

            signatures.insert(signature);
            FunctionDecl inherited_method = base_method;
            inherited_method.class_name = secondary_base.class_decl->cpp_name;
            emitted_methods.push_back(EmittedMethod{std::move(inherited_method), true});
        }
    }

    return emitted_methods;
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

} // namespace csbind23::emit
