#pragma once

#include "csbind23/emit/csharp_naming.hpp"
#include "csbind23/ir.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace csbind23::emit
{

struct GenericFunctionGroup
{
    std::string name;
    std::vector<const FunctionDecl*> instantiations;
};

struct GenericDispatchShape
{
    bool supported = false;
    std::vector<int> generic_parameter_slot_ids;
    int generic_return_slot_id = -1;
    std::size_t generic_arity = 0;
    std::string unsupported_reason;
};

std::string csharp_string_literal_escape(std::string_view text);

std::string generic_type_parameter_name(std::size_t index, std::size_t generic_arity);

std::string generic_type_parameter_list(std::size_t generic_arity);

std::string concrete_type_for_slot(
    const FunctionDecl& instantiation, const GenericDispatchShape& shape, int slot_id);

GenericDispatchShape analyze_generic_dispatch_shape(const GenericFunctionGroup& group);

std::string generic_dispatch_not_supported_message(
    const ModuleDecl& module_decl, CSharpNameKind name_kind, const GenericFunctionGroup& group,
    const GenericDispatchShape& shape);

} // namespace csbind23::emit
