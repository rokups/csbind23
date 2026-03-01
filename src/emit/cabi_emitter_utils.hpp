#pragma once

#include "emitter_ir_utils.hpp"

#include "csbind23/text_writer.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace csbind23::emit
{

std::string render_cpp_type(const TypeRef& type_ref);

std::string render_converted_argument_name(std::size_t index);

std::string render_callback_argument_name(std::size_t index);

std::size_t normalized_trailing_default_count(const FunctionDecl& function_decl);

std::vector<ParameterDecl> leading_parameters(const FunctionDecl& function_decl, std::size_t parameter_count);

void append_converted_arguments(TextWriter& output, const std::vector<ParameterDecl>& parameters);

std::string render_call_arguments(const std::vector<ParameterDecl>& parameters);

std::string callback_typedef_name(
    const std::string& module_name, const std::string& class_name, const FunctionDecl& method_decl);

} // namespace csbind23::emit
