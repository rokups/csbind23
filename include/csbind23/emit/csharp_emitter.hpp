#pragma once

#include "csbind23/ir.hpp"

#include <filesystem>
#include <vector>

namespace csbind23::emit
{

std::vector<std::filesystem::path> emit_csharp_module(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root);

} // namespace csbind23::emit
