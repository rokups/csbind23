#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace csbind23
{

class TextWriter
{
public:
    explicit TextWriter(std::size_t reserved_size = 0)
    {
        if (reserved_size > 0)
        {
            buffer_.reserve(reserved_size);
        }
    }

    TextWriter& append(std::string_view fragment)
    {
        buffer_.append(fragment);
        return *this;
    }

    TextWriter& append_line(std::string_view line = {})
    {
        buffer_.append(line);
        buffer_.push_back('\n');
        return *this;
    }

    template <typename... Args> TextWriter& append_format(std::format_string<Args...> format_string, Args&&... args)
    {
        buffer_ += std::format(format_string, std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args>
    TextWriter& append_line_format(std::format_string<Args...> format_string, Args&&... args)
    {
        append_format(format_string, std::forward<Args>(args)...);
        buffer_.push_back('\n');
        return *this;
    }

    const std::string& str() const& { return buffer_; }

    std::string&& str() && { return std::move(buffer_); }

private:
    std::string buffer_;
};

} // namespace csbind23
