#pragma once

#include <string>
#include <string_view>

namespace csbind23::detail
{
inline std::string replace_all(std::string value, std::string_view from, std::string_view to)
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

inline std::string make_safe_csharp_namespace_segment(std::string_view text)
{
    std::string result;
    result.reserve(text.size() + 8);

    for (char ch : text)
    {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')
        {
            result.push_back(ch);
        }
        else
        {
            result.push_back('_');
        }
    }

    if (result.empty())
    {
        result = "module";
    }

    if (result.front() >= '0' && result.front() <= '9')
    {
        result.insert(result.begin(), '_');
    }

    if (result == "string" || result == "namespace" || result == "class" || result == "public"
        || result == "internal" || result == "private")
    {
        result += "_module";
    }

    return result;
}

} // namespace csbind23::detail
