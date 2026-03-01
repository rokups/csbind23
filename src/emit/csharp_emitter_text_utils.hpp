#pragma once

#include "csbind23/text_writer.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace csbind23::emit
{

std::string replace_all(std::string value, std::string_view from, std::string_view to);

std::string render_inline_template(
    std::string templ, std::string_view managed_name, std::string_view pinvoke_name, std::string_view value_name,
    std::string_view module_name);

std::vector<std::string> split_trimmed_lines(std::string_view text);

void append_embedded_statement(TextWriter& output, std::string_view indent, std::string_view statement);

void append_embedded_assignment(
    TextWriter& output, std::string_view indent, std::string_view assignment_prefix, std::string_view expression);

void append_embedded_return(TextWriter& output, std::string_view indent, std::string_view expression);

std::string join_arguments(const std::vector<std::string>& arguments);

} // namespace csbind23::emit
