#pragma once

#include "csbind23/cabi/converter.hpp"

#include <string_view>

namespace csbind23::cabi::detail
{

inline constexpr std::string_view std_string_view_wrapper_managed_type_name()
{
    return "global::Std.StringView";
}

} // namespace csbind23::cabi::detail

namespace csbind23::cabi
{

template <> struct Converter<std::string_view>
{
    using cpp_type = std::string_view;
    using c_abi_type = StringView;

    static constexpr std::string_view c_abi_param_type_name() { return "const csbind23::cabi::StringView*"; }
    static constexpr std::string_view c_abi_return_type_name() { return "csbind23::cabi::StringView"; }
    static constexpr std::string_view pinvoke_param_type_name() { return "in global::Std.StringView"; }
    static constexpr std::string_view pinvoke_return_type_name()
    {
        return "global::Std.StringView";
    }
    static constexpr std::string_view managed_type_name() { return detail::std_string_view_wrapper_managed_type_name(); }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "{value}";
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{.str = value.data(), .length = value.size()};
    }

    static cpp_type from_c_abi(const c_abi_type* value)
    {
        if (value == nullptr || value->str == nullptr)
        {
            return std::string_view{};
        }
        return std::string_view{value->str, value->length};
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        if (value.str == nullptr)
        {
            return std::string_view{};
        }
        return std::string_view{value.str, value.length};
    }
};

} // namespace csbind23::cabi