#pragma once

#include <filesystem>
#include <vector>

#include "csbind23/ir.hpp"

namespace csbind23::emit {

std::vector<std::filesystem::path> emit_cabi_module(const ModuleDecl& module_decl, const std::filesystem::path& output_root);

} // namespace csbind23::emit
