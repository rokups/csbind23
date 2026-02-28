#pragma once

#include "csbind23/bindings_generator.hpp"

namespace csbind23::testing::basic
{

template<typename T>
T get_and_return(T value)
{
    return value;
}

}   // namespace csbind23::testing::basic

namespace csbind23::testing
{

inline void register_bindings_basic(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("BasicApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_basic.hpp\"")
        .def<&basic::get_and_return<bool>>("get_and_return_bool")
        .def<&basic::get_and_return<int8_t>>("get_and_return_int8")
        .def<&basic::get_and_return<uint8_t>>("get_and_return_uint8")
        .def<&basic::get_and_return<int16_t>>("get_and_return_int16")
        .def<&basic::get_and_return<int32_t>>("get_and_return_int32")
        .def<&basic::get_and_return<int64_t>>("get_and_return_int64")
        .def<&basic::get_and_return<uint16_t>>("get_and_return_uint16")
        .def<&basic::get_and_return<uint32_t>>("get_and_return_uint32")
        .def<&basic::get_and_return<uint64_t>>("get_and_return_uint64")
        .def<&basic::get_and_return<float>>("get_and_return_float")
        .def<&basic::get_and_return<double>>("get_and_return_double")
        .def<&basic::get_and_return<char16_t>>("get_and_return_char16");
}

} // namespace csbind23::testing
