#include "csharp_emitter_text_utils.hpp"

#include <cctype>
#include <format>

namespace csbind23::emit
{

namespace
{

std::string module_placeholder_value(std::string_view module_name)
{
    if (module_name.size() >= 6 && module_name.substr(module_name.size() - 6) == "Native")
    {
        return std::string(module_name);
    }

    std::string result;
    bool capitalize_next = true;
    for (const unsigned char ch : module_name)
    {
        if (std::isalnum(ch) == 0)
        {
            capitalize_next = true;
            continue;
        }

        result.push_back(capitalize_next ? static_cast<char>(std::toupper(ch)) : static_cast<char>(ch));
        capitalize_next = false;
    }

    if (result.empty())
    {
        return std::string(module_name);
    }

    result += "Native";
    return result;
}

} // namespace

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
    templ = replace_all(std::move(templ), "{module}", module_placeholder_value(module_name));
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
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        const auto& line = lines[index];
        const bool opens_block_on_next_line = index + 1 < lines.size() && lines[index + 1] == "{";
        const bool keeps_own_terminator = !line.empty()
            && (line.back() == ';' || line.back() == '{' || line.back() == '}' || line.back() == ':');

        if (keeps_own_terminator || opens_block_on_next_line)
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
