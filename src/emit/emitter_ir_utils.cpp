#include "emitter_ir_utils.hpp"

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
    const ModuleDecl& module_decl, const ClassDecl& class_decl, const MethodCollectionOptions& options)
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

    for (const auto* secondary_base : secondary_base_classes(module_decl, class_decl))
    {
        for (const auto& base_method : secondary_base->methods)
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
            inherited_method.class_name = secondary_base->cpp_name;
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
