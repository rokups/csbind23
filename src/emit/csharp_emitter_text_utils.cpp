#include "csharp_emitter_text_utils.hpp"

#include <format>

namespace csbind23::emit
{

std::string replace_all(std::string value, std::string_view from, std::string_view to)
{
    std::size_t start = 0;
    while (true)
    {
        const std::size_t index = value.find(from, start);
        if (index == std::string::npos)
        {
            break;
        }
        value.replace(index, from.size(), to);
        start = index + to.size();
    }
    return value;
}

std::string render_inline_template(
    std::string templ, std::string_view managed_name, std::string_view pinvoke_name, std::string_view value_name,
    std::string_view module_name)
{
    templ = replace_all(std::move(templ), "{managed}", managed_name);
    templ = replace_all(std::move(templ), "{pinvoke}", pinvoke_name);
    templ = replace_all(std::move(templ), "{value}", value_name);
    templ = replace_all(std::move(templ), "{module}", module_name);
    return templ;
}

std::vector<std::string> split_trimmed_lines(std::string_view text)
{
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size())
    {
        std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos)
        {
            end = text.size();
        }

        std::string_view line = text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }

        const std::size_t first_non_ws = line.find_first_not_of(" \t");
        if (first_non_ws != std::string_view::npos)
        {
            lines.emplace_back(line.substr(first_non_ws));
        }

        if (end == text.size())
        {
            break;
        }
        start = end + 1;
    }

    return lines;
}

void append_embedded_statement(TextWriter& output, std::string_view indent, std::string_view statement)
{
    const auto lines = split_trimmed_lines(statement);
    for (const auto& line : lines)
    {
        if (!line.empty() && line.back() == ';')
        {
            output.append_line_format("{}{}", indent, line);
        }
        else
        {
            output.append_line_format("{}{};", indent, line);
        }
    }
}

void append_embedded_assignment(
    TextWriter& output, std::string_view indent, std::string_view assignment_prefix, std::string_view expression)
{
    const auto lines = split_trimmed_lines(expression);
    if (lines.empty())
    {
        output.append_line_format("{}{};", indent, assignment_prefix);
        return;
    }

    if (lines.size() == 1)
    {
        output.append_line_format("{}{}{};", indent, assignment_prefix, lines[0]);
        return;
    }

    output.append_line_format("{}{}{}", indent, assignment_prefix, lines[0]);
    for (std::size_t index = 1; index + 1 < lines.size(); ++index)
    {
        output.append_line_format("{}{}", indent, lines[index]);
    }
    output.append_line_format("{}{};", indent, lines.back());
}

void append_embedded_return(TextWriter& output, std::string_view indent, std::string_view expression)
{
    const auto lines = split_trimmed_lines(expression);
    if (lines.empty())
    {
        output.append_line_format("{}return;", indent);
        return;
    }

    if (lines.size() == 1)
    {
        output.append_line_format("{}return {};", indent, lines[0]);
        return;
    }

    output.append_line_format("{}return {}", indent, lines[0]);
    for (std::size_t index = 1; index + 1 < lines.size(); ++index)
    {
        output.append_line_format("{}{}", indent, lines[index]);
    }
    output.append_line_format("{}{};", indent, lines.back());
}

std::string join_arguments(const std::vector<std::string>& arguments)
{
    std::string rendered;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        rendered += arguments[index];
        if (index + 1 < arguments.size())
        {
            rendered += ", ";
        }
    }
    return rendered;
}

} // namespace csbind23::emit
