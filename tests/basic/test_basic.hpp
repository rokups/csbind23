#pragma once

#include "csbind23/bindings_generator.hpp"

namespace csbind23::testing::basic
{

template<typename T>
T get_and_return(T value)
{
    return value;
}

template<typename T>
void add_assign_ref(T& value, T delta)
{
    value = static_cast<T>(value + delta);
}

template<typename T>
void add_assign_ptr(T* value, T delta)
{
    if (value == nullptr)
    {
        return;
    }

    *value = static_cast<T>(*value + delta);
}

template<typename T>
void assign_ref(T& value, T next)
{
    value = next;
}

template<typename T>
void assign_ptr(T* value, T next)
{
    if (value == nullptr)
    {
        return;
    }

    *value = next;
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
        .def<&basic::get_and_return<char16_t>>("get_and_return_char16")
        .def<&basic::add_assign_ref<int32_t>>("add_assign_ref_int32")
        .def<&basic::add_assign_ref<double>>("add_assign_ref_double")
        .def<&basic::add_assign_ptr<int32_t>>("add_assign_ptr_int32")
        .def<&basic::add_assign_ptr<double>>("add_assign_ptr_double")
        .def<&basic::assign_ref<bool>>("assign_ref_bool")
        .def<&basic::assign_ref<int8_t>>("assign_ref_int8")
        .def<&basic::assign_ref<uint8_t>>("assign_ref_uint8")
        .def<&basic::assign_ref<int16_t>>("assign_ref_int16")
        .def<&basic::assign_ref<int32_t>>("assign_ref_int32")
        .def<&basic::assign_ref<int64_t>>("assign_ref_int64")
        .def<&basic::assign_ref<uint16_t>>("assign_ref_uint16")
        .def<&basic::assign_ref<uint32_t>>("assign_ref_uint32")
        .def<&basic::assign_ref<uint64_t>>("assign_ref_uint64")
        .def<&basic::assign_ref<float>>("assign_ref_float")
        .def<&basic::assign_ref<double>>("assign_ref_double")
        .def<&basic::assign_ref<char16_t>>("assign_ref_char16")
        .def<&basic::assign_ptr<bool>>("assign_ptr_bool")
        .def<&basic::assign_ptr<int8_t>>("assign_ptr_int8")
        .def<&basic::assign_ptr<uint8_t>>("assign_ptr_uint8")
        .def<&basic::assign_ptr<int16_t>>("assign_ptr_int16")
        .def<&basic::assign_ptr<int32_t>>("assign_ptr_int32")
        .def<&basic::assign_ptr<int64_t>>("assign_ptr_int64")
        .def<&basic::assign_ptr<uint16_t>>("assign_ptr_uint16")
        .def<&basic::assign_ptr<uint32_t>>("assign_ptr_uint32")
        .def<&basic::assign_ptr<uint64_t>>("assign_ptr_uint64")
        .def<&basic::assign_ptr<float>>("assign_ptr_float")
        .def<&basic::assign_ptr<double>>("assign_ptr_double")
        .def<&basic::assign_ptr<char16_t>>("assign_ptr_char16");
}

} // namespace csbind23::testing
