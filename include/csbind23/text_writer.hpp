#pragma once

#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace csbind23
{

class TextWriter
{
public:
    explicit TextWriter(std::size_t reserved_size = 0, std::size_t initial_indent_level = 0)
        : indent_level_(initial_indent_level)
    {
        if (reserved_size > 0)
        {
            buffer_.reserve(reserved_size);
        }
    }

    TextWriter& append(std::string_view fragment)
    {
        append_block(fragment, false);
        return *this;
    }

    TextWriter& append_verbatim(std::string_view fragment)
    {
        flush_pending_line(false);
        buffer_.append(fragment);
        return *this;
    }

    TextWriter& append_line(std::string_view line = {})
    {
        append_block(line, true);
        return *this;
    }

    TextWriter& append_line_verbatim(std::string_view line = {})
    {
        flush_pending_line(false);
        buffer_.append(line);
        buffer_.push_back('\n');
        return *this;
    }

    template <typename... Args> TextWriter& append_format(std::format_string<Args...> format_string, Args&&... args)
    {
        append_block(std::format(format_string, std::forward<Args>(args)...), false);
        return *this;
    }

    template <typename... Args>
    TextWriter& append_format_verbatim(std::format_string<Args...> format_string, Args&&... args)
    {
        flush_pending_line(false);
        buffer_ += std::format(format_string, std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args>
    TextWriter& append_line_format(std::format_string<Args...> format_string, Args&&... args)
    {
        append_block(std::format(format_string, std::forward<Args>(args)...), true);
        return *this;
    }

    template <typename... Args>
    TextWriter& append_line_format_verbatim(std::format_string<Args...> format_string, Args&&... args)
    {
        flush_pending_line(false);
        buffer_ += std::format(format_string, std::forward<Args>(args)...);
        buffer_.push_back('\n');
        return *this;
    }

    void set_indent_level(std::size_t indent_level)
    {
        indent_level_ = indent_level;
    }

    std::size_t indent_level() const
    {
        return indent_level_;
    }

    const std::string& str() const&
    {
        auto* self = const_cast<TextWriter*>(this);
        self->flush_pending_line(false);
        return buffer_;
    }

    std::string&& str() &&
    {
        flush_pending_line(false);
        return std::move(buffer_);
    }

private:
    static constexpr std::string_view indent_unit_ = "    ";

    static std::string_view trim_left(std::string_view text)
    {
        const std::size_t first_non_ws = text.find_first_not_of(" \t");
        if (first_non_ws == std::string_view::npos)
        {
            return {};
        }
        return text.substr(first_non_ws);
    }

    static std::string_view trim(std::string_view text)
    {
        const std::size_t first_non_ws = text.find_first_not_of(" \t");
        if (first_non_ws == std::string_view::npos)
        {
            return {};
        }

        const std::size_t last_non_ws = text.find_last_not_of(" \t");
        return text.substr(first_non_ws, last_non_ws - first_non_ws + 1);
    }

    static bool should_decrease_indent_before(std::string_view trimmed_line)
    {
        if (trimmed_line == "}")
        {
            return true;
        }

        return !trimmed_line.empty() && trimmed_line.back() == '}';
    }

    static bool should_increase_indent_after(std::string_view trimmed_line)
    {
        if (trimmed_line == "{")
        {
            return true;
        }

        return !trimmed_line.empty() && trimmed_line.back() == '{';
    }

    void append_indent_prefix()
    {
        for (std::size_t index = 0; index < indent_level_; ++index)
        {
            buffer_.append(indent_unit_);
        }
    }

    void flush_pending_line(bool append_newline)
    {
        if (pending_line_.empty())
        {
            if (append_newline)
            {
                buffer_.push_back('\n');
            }
            return;
        }

        if (!pending_line_.empty() && pending_line_.back() == '\r')
        {
            pending_line_.pop_back();
        }

        const std::string_view trimmed_line = trim(pending_line_);
        if (trimmed_line.empty())
        {
            if (append_newline)
            {
                buffer_.push_back('\n');
            }
            pending_line_.clear();
            return;
        }

        if (should_decrease_indent_before(trimmed_line) && indent_level_ > 0)
        {
            --indent_level_;
        }

        append_indent_prefix();
        buffer_.append(trim_left(pending_line_));

        if (append_newline)
        {
            buffer_.push_back('\n');
        }

        if (should_increase_indent_after(trimmed_line))
        {
            ++indent_level_;
        }

        pending_line_.clear();
    }

    void append_block(std::string_view block, bool add_trailing_newline)
    {
        std::size_t start = 0;
        while (start < block.size())
        {
            const std::size_t newline = block.find('\n', start);
            if (newline == std::string_view::npos)
            {
                pending_line_.append(block.substr(start));
                start = block.size();
            }
            else
            {
                pending_line_.append(block.substr(start, newline - start));
                flush_pending_line(true);
                start = newline + 1;
            }
        }

        if (add_trailing_newline)
        {
            flush_pending_line(true);
        }
    }

    std::string buffer_;
    std::string pending_line_;
    std::size_t indent_level_ = 0;
};

} // namespace csbind23
