#pragma once

#include "csbind23/bindings_generator.hpp"

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace csbind23::testing::basic_generic
{

template <typename T>
T value_passthrough(T value)
{
    return value;
}

template <typename T>
void add_assign_generic_ref(T& value, T delta)
{
    value = static_cast<T>(value + delta);
}

template <typename T>
void add_assign_generic_out(T left, T right, T& out_value)
{
    out_value = static_cast<T>(left + right);
}

template <typename T, typename U>
T add_casted(T value, U delta)
{
    return static_cast<T>(value + static_cast<T>(delta));
}

template <typename T>
void bump_ref(T& value, T delta)
{
    value = static_cast<T>(value + delta);
}

template <typename T>
void bump_ptr(T* value, T delta)
{
    if (value == nullptr)
    {
        return;
    }

    *value = static_cast<T>(*value + delta);
}

template <typename T>
std::string tagged_value(T value, std::string_view tag)
{
    return std::format("{}:{}", tag, value);
}

template <typename T>
T prefixed_bias(std::string_view prefix, T value)
{
    return prefix.empty() ? value : static_cast<T>(value + static_cast<T>(1));
}

template <typename T>
T choose_by_flag(T left, T right, bool choose_left)
{
    return choose_left ? left : right;
}

} // namespace csbind23::testing::basic_generic

namespace csbind23::testing
{

inline void register_bindings_basic_generic(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("BasicGenericApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_basic_generic.hpp\"")
        .def_generic<&basic_generic::value_passthrough<int32_t>, &basic_generic::value_passthrough<double>>(
            "value_passthrough_generic")
        .def_generic<&basic_generic::add_assign_generic_ref<int32_t>, &basic_generic::add_assign_generic_ref<double>>(
            "add_assign_generic_ref")
        .def_generic<&basic_generic::add_assign_generic_out<int32_t>, &basic_generic::add_assign_generic_out<double>>(
            "add_assign_generic_out",
            csbind23::Arg{.idx = 2, .name = "result", .output = true})
        .def_generic<&basic_generic::add_casted<int32_t, double>, &basic_generic::add_casted<double, int32_t>>(
            "add_casted_generic")
        .def<&basic_generic::bump_ref<int32_t>>("bump_ref_int32")
        .def<&basic_generic::bump_ref<double>>("bump_ref_double")
        .def<&basic_generic::bump_ptr<int32_t>>("bump_ptr_int32")
        .def<&basic_generic::bump_ptr<double>>("bump_ptr_double")
        .def_generic<&basic_generic::tagged_value<int32_t>, &basic_generic::tagged_value<double>>(
            "tagged_value_generic")
        .def_generic<&basic_generic::prefixed_bias<int32_t>, &basic_generic::prefixed_bias<double>>(
            "prefixed_bias_generic")
        .def_generic<&basic_generic::choose_by_flag<int32_t>, &basic_generic::choose_by_flag<double>>(
            "choose_by_flag_generic");
}

} // namespace csbind23::testing
