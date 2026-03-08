#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace csbind23::testing::strings
{

inline std::array<char, 64>& live_string_view_storage()
{
    static std::array<char, 64> storage = [] {
        std::array<char, 64> value{};
        constexpr std::string_view text = "live-view";
        std::copy(text.begin(), text.end(), value.begin());
        return value;
    }();
    return storage;
}

inline std::size_t& live_string_view_length()
{
    static std::size_t length = std::string_view{"live-view"}.size();
    return length;
}

inline std::string echo_by_value(std::string value)
{
    return value + "|value";
}

inline std::string echo_const_value(const std::string value)
{
    return value + "|const-value";
}

inline std::string consume_const_ref(const std::string& value)
{
    return value + "|const-ref";
}

inline std::string consume_cstr(const char* value)
{
    return std::string{value == nullptr ? "" : value} + "|cstr";
}

inline const char* get_cstr()
{
    return "cstr-return";
}

inline std::string consume_string_view(std::string_view value)
{
    return std::string{value} + "|view";
}

inline std::string_view get_string_view()
{
    return "view-return";
}

inline std::string_view get_live_string_view()
{
    auto& storage = live_string_view_storage();
    const std::size_t length = live_string_view_length();
    return std::string_view{storage.data(), length};
}

inline void set_live_string_view(std::string_view value)
{
    auto& storage = live_string_view_storage();

    const std::size_t count = std::min<std::size_t>(value.size(), storage.size());
    std::copy_n(value.data(), count, storage.data());
    if (count < storage.size())
    {
        std::fill(storage.begin() + static_cast<std::ptrdiff_t>(count), storage.end(), '\0');
    }
    live_string_view_length() = count;
}

inline const std::string& get_const_ref()
{
    static std::string value{"const-ref"};
    return value;
}

inline std::string& get_ref()
{
    static std::string value{"ref"};
    return value;
}

inline void set_ref_value(const std::string& value)
{
    get_ref() = value;
}

inline void assign_by_ref(std::string& io, const std::string& value)
{
    io = value;
}

inline void append_by_const_ref(std::string& io, const std::string& suffix)
{
    io += suffix;
}

inline void write_output(std::string& out)
{
    out = "output";
}

inline void write_output_ptr(std::string* out)
{
    if (out == nullptr)
    {
        return;
    }

    *out = "output-ptr";
}

inline std::string consume_const_ptr(const std::string* value)
{
    if (value == nullptr)
    {
        return {};
    }

    return *value + "|const-ptr";
}

} // namespace csbind23::testing::strings

namespace csbind23::testing
{

inline void register_bindings_string(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    csbind23::add_string(generator);

    module.pinvoke_library("e2e.C")
        .cabi_include("\"tests/stl/test_string.hpp\"")
        .def<&strings::echo_by_value>("string_echo_by_value")
        .def<&strings::echo_const_value>("string_echo_const_value")
        .def<&strings::consume_const_ref>("string_consume_const_ref")
        .def<&strings::consume_cstr>("string_consume_cstr")
        .def<&strings::get_cstr>("string_get_cstr")
        .def<&strings::consume_string_view>("string_consume_string_view")
        .def<&strings::get_string_view>("string_get_string_view")
        .def<&strings::get_live_string_view>("string_get_live_string_view")
        .def<&strings::set_live_string_view>("string_set_live_string_view")
        .def<&strings::get_const_ref>("string_get_const_ref")
        .def<&strings::get_ref>("string_get_ref")
        .def<&strings::set_ref_value>("string_set_ref_value")
        .def<&strings::assign_by_ref>("string_assign_by_ref")
        .def<&strings::append_by_const_ref>("string_append_by_const_ref")
        .def<&strings::write_output>("string_write_output")
        .def<&strings::write_output_ptr>("string_write_output_ptr")
        .def<&strings::consume_const_ptr>("string_consume_const_ptr");
}

} // namespace csbind23::testing
