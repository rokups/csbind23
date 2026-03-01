#pragma once

#include "csbind23/bindings_generator.hpp"

#include <array>
#include <string_view>

namespace csbind23::testing::arrays
{

inline std::array<int, 4> identity(std::array<int, 4> value)
{
    return value;
}

inline std::array<int, 4> add_scalar(std::array<int, 4> value, int delta)
{
    for (auto& element : value)
    {
        element += delta;
    }
    return value;
}

inline int sum_const_ref(const std::array<int, 4>& value)
{
    int total = 0;
    for (const int element : value)
    {
        total += element;
    }
    return total;
}

inline void add_scalar_ref(std::array<int, 4>& value, int delta)
{
    for (auto& element : value)
    {
        element += delta;
    }
}

inline void set_sequence_ref(std::array<int, 4>& value)
{
    value = {4, 3, 2, 1};
}

inline int sum_ptr_fixed4(const int* value)
{
    int total = 0;
    for (int index = 0; index < 4; ++index)
    {
        total += value[index];
    }
    return total;
}

inline int sum_ptr_counted(const int* value, int count)
{
    int total = 0;
    for (int index = 0; index < count; ++index)
    {
        total += value[index];
    }
    return total;
}

inline void add_scalar_ptr_counted(int* value, int count, int delta)
{
    for (int index = 0; index < count; ++index)
    {
        value[index] += delta;
    }
}

} // namespace csbind23::testing::arrays

namespace csbind23::testing
{

inline void register_bindings_array(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.pinvoke_library("e2e.C")
        .cabi_include("\"tests/stl/test_array.hpp\"")
        .def<&arrays::identity>("array_identity")
        .def<&arrays::add_scalar>("array_add_scalar")
        .def<&arrays::sum_const_ref>("array_sum_const_ref")
        .def<&arrays::add_scalar_ref>("array_add_scalar_ref")
        .def<&arrays::set_sequence_ref>("array_set_sequence_ref")
        .def<&arrays::sum_ptr_fixed4>("array_sum_ptr_fixed4",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true})
        .def<&arrays::sum_ptr_counted>("array_sum_ptr_counted",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&arrays::add_scalar_ptr_counted>("array_add_scalar_ptr_counted",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1});
}

} // namespace csbind23::testing
