#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string>
#include <string_view>

namespace csbind23::testing::strings
{

inline std::string echo_by_value(std::string value)
{
    return value + "|value";
}

inline std::string consume_const_ref(const std::string& value)
{
    return value + "|const-ref";
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

inline std::string* string_create(std::string value)
{
    return new std::string(std::move(value));
}

inline void string_destroy(std::string* value)
{
    delete value;
}

inline std::string string_read(std::string* value)
{
    if (value == nullptr)
    {
        return {};
    }
    return *value;
}

inline void string_assign(std::string* value, std::string text)
{
    if (value == nullptr)
    {
        return;
    }
    *value = std::move(text);
}

} // namespace csbind23::testing::strings

namespace csbind23::cabi
{

template <> struct Converter<std::string&>
{
    using cpp_type = std::string&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "StringE2EConverters.ToNative({value})"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "StringE2EConverters.FromNative({value})"; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "{managed} = StringE2EConverters.FromNative({pinvoke}); StringE2EConverters.DestroyNative({pinvoke})";
    }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return ""; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<std::string*>(value); }
};

template <> struct Converter<const std::string&>
{
    using cpp_type = const std::string&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "StringE2EConverters.ToNative({value})"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "StringE2EConverters.FromNative({value})"; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement() { return "StringE2EConverters.DestroyNative({pinvoke})"; }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return ""; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const std::string*>(value); }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_string_bindings(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.def<&strings::echo_by_value>("string_echo_by_value")
        .def<&strings::consume_const_ref>("string_consume_const_ref")
        .def<&strings::get_const_ref>("string_get_const_ref")
        .def<&strings::get_ref>("string_get_ref")
        .def<&strings::assign_by_ref>("string_assign_by_ref")
        .def<&strings::append_by_const_ref>("string_append_by_const_ref")
        .def<&strings::write_output>("string_write_output")
        .def<&strings::string_create>("string_create")
        .def<&strings::string_destroy>("string_destroy")
        .def<&strings::string_read>("string_read")
        .def<&strings::string_assign>("string_assign");
}

} // namespace csbind23::testing
