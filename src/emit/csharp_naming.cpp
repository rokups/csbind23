#include "csbind23/emit/csharp_naming.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace csbind23::emit
{
namespace
{

std::vector<std::string> split_words(std::string_view text)
{
    std::vector<std::string> words;
    std::string current;

    auto flush = [&]() {
        if (!current.empty())
        {
            words.push_back(current);
            current.clear();
        }
    };

    const auto is_separator = [](char ch) {
        return ch == '_' || ch == '-' || ch == '.' || ch == ':' || ch == ' ' || ch == '\t' || ch == '/';
    };

    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (is_separator(ch))
        {
            flush();
            continue;
        }

        const bool starts_new_word = !current.empty() && std::isupper(static_cast<unsigned char>(ch))
            && (std::islower(static_cast<unsigned char>(current.back()))
                || (index + 1 < text.size() && std::islower(static_cast<unsigned char>(text[index + 1]))));
        if (starts_new_word)
        {
            flush();
        }

        current.push_back(ch);
    }

    flush();
    return words;
}

} // namespace

std::string to_pascal_case(std::string_view input)
{
    const auto words = split_words(input);
    if (words.empty())
    {
        return std::string(input);
    }

    std::string result;
    for (auto word : words)
    {
        std::transform(word.begin(), word.end(), word.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        word.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(word.front())));
        result += word;
    }

    return result;
}

std::string to_snake_case(std::string_view input)
{
    const auto words = split_words(input);
    if (words.empty())
    {
        return std::string(input);
    }

    std::string result;
    for (std::size_t index = 0; index < words.size(); ++index)
    {
        std::string word = words[index];
        std::transform(word.begin(), word.end(), word.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (index > 0)
        {
            result += "_";
        }
        result += word;
    }

    return result;
}

std::string format_csharp_name(
    const ModuleDecl& module_decl, CSharpNameKind kind, std::string_view input)
{
    if (!module_decl.csharp_name_formatter)
    {
        if (kind == CSharpNameKind::Class)
        {
            return to_pascal_case(input);
        }
        return std::string(input);
    }

    const std::string formatted = module_decl.csharp_name_formatter(kind, input);
    if (formatted.empty())
    {
        return std::string(input);
    }

    return formatted;
}

} // namespace csbind23::emit
