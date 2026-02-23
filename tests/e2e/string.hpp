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

namespace csbind23::testing
{

inline void register_string_bindings(BindingsGenerator& generator, std::string_view module_name)
{
    csbind23::ManagedInlineConverter non_const_ref_converter;
    non_const_ref_converter.managed_type_name = "string";
    non_const_ref_converter.to_pinvoke_expression = "StringE2EConverters.ToNative({value})";
    non_const_ref_converter.from_pinvoke_expression = "StringE2EConverters.FromNative({value})";
    non_const_ref_converter.finalize_to_pinvoke_statement =
        "{managed} = StringE2EConverters.FromNative({pinvoke}); StringE2EConverters.DestroyNative({pinvoke})";

    csbind23::ManagedInlineConverter const_ref_converter;
    const_ref_converter.managed_type_name = "string";
    const_ref_converter.to_pinvoke_expression = "StringE2EConverters.ToNative({value})";
    const_ref_converter.from_pinvoke_expression = "StringE2EConverters.FromNative({value})";
    const_ref_converter.finalize_to_pinvoke_statement = "StringE2EConverters.DestroyNative({pinvoke})";

    generator.managed_converter<std::string&>(non_const_ref_converter);
    generator.managed_converter<const std::string&>(const_ref_converter);

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
