#pragma once

#include "csbind23/ir.hpp"

#include <string>
#include <string_view>

namespace csbind23::emit
{

std::string to_pascal_case(std::string_view input);
std::string to_snake_case(std::string_view input);

std::string format_csharp_name(
    const ModuleDecl& module_decl, CSharpNameKind kind, std::string_view input);

} // namespace csbind23::emit
