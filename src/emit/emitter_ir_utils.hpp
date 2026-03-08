#pragma once

#include "csbind23/ir.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace csbind23::emit
{

struct ClassBinding
{
    const ModuleDecl* module_decl = nullptr;
    const ClassDecl* class_decl = nullptr;
};

struct EmittedMethod
{
    FunctionDecl method;
    bool inherited_from_secondary_base = false;
};

enum class MethodSignatureKind
{
    CAbi,
    PInvoke,
};

struct MethodCollectionOptions
{
    bool exclude_pinvoke_only = false;
    MethodSignatureKind signature_kind = MethodSignatureKind::CAbi;
};

std::string exported_symbol_name(const FunctionDecl& function_decl);

const ClassDecl* find_class_by_cpp_name(const ModuleDecl& module_decl, std::string_view cpp_name);

ClassBinding find_unique_class_by_cpp_name(const std::vector<ModuleDecl>& modules, std::string_view cpp_name);

ClassBinding find_visible_class_by_cpp_name(
    const std::vector<ModuleDecl>& modules, const ModuleDecl& module_decl, std::string_view cpp_name);

std::vector<ClassBinding> secondary_base_classes(
    const std::vector<ModuleDecl>& modules, const ModuleDecl& module_decl, const ClassDecl& class_decl);

bool class_derives_from(
    const std::vector<ModuleDecl>& modules, const ClassDecl& class_decl, std::string_view base_cpp_name);

std::vector<ClassBinding> polymorphic_assignable_classes(
    const std::vector<ModuleDecl>& modules, std::string_view base_cpp_name);

bool is_wrapper_visible_method(const FunctionDecl& method_decl, bool exclude_pinvoke_only);

std::string method_signature_key(const FunctionDecl& method_decl, MethodSignatureKind signature_kind);

std::vector<EmittedMethod> collect_emitted_methods(
    const std::vector<ModuleDecl>& modules,
    const ModuleDecl& module_decl,
    const ClassDecl& class_decl,
    const MethodCollectionOptions& options = {});

std::vector<const FunctionDecl*> collect_virtual_methods(
    const ClassDecl& class_decl, const std::vector<EmittedMethod>& emitted_methods);

} // namespace csbind23::emit
