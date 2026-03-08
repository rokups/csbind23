#pragma once

#include "csbind23/bindings_generator.hpp"
#include "tests/basic/test_custom_struct_vec2.hpp"

#include <array>
#include <string_view>

namespace csbind23::testing::arrays
{

class ArrayItem
{
public:
    explicit ArrayItem(int value)
        : value_(value)
    {
    }

    int get() const
    {
        return value_;
    }

    void set(int value)
    {
        value_ = value;
    }

private:
    int value_;
};

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

inline std::array<float, 3> scale(std::array<float, 3> value, float factor)
{
    for (auto& element : value)
    {
        element *= factor;
    }
    return value;
}

inline float sum_ptr_counted_float(const float* value, int count)
{
    float total = 0.0f;
    for (int index = 0; index < count; ++index)
    {
        total += value[index];
    }
    return total;
}

inline void add_scalar_ptr_counted_float(float* value, int count, float delta)
{
    for (int index = 0; index < count; ++index)
    {
        value[index] += delta;
    }
}

inline std::array<custom_struct_vec2::Vec2, 2> shift_vec2_array(std::array<custom_struct_vec2::Vec2, 2> value, float delta)
{
    for (auto& element : value)
    {
        element = custom_struct_vec2::Vec2{element.GetX() + delta, element.GetY() - delta};
    }
    return value;
}

inline float sum_vec2_array(const std::array<custom_struct_vec2::Vec2, 2>& value)
{
    float total = 0.0f;
    for (const auto& element : value)
    {
        total += element.GetX() + element.GetY();
    }
    return total;
}

inline std::array<ArrayItem*, 3> rotate_item_pointer_array(std::array<ArrayItem*, 3> value)
{
    return {value[1], value[2], value[0]};
}

inline int sum_item_pointer_array(const std::array<ArrayItem*, 3>& value)
{
    int total = 0;
    for (ArrayItem* item : value)
    {
        if (item != nullptr)
        {
            total += item->get();
        }
    }
    return total;
}

} // namespace csbind23::testing::arrays

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::arrays::ArrayItem*>
{
    using cpp_type = csbind23::testing::arrays::ArrayItem*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.Array.ArrayItem"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} is null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "(global::CsBind23.Tests.E2E.Array.ArrayItem)global::CsBind23.Tests.E2E.Array.ArrayRuntime.WrapPolymorphicArrayItem({value}, global::CsBind23.Generated.ItemOwnership.Borrowed)";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

} // namespace csbind23::cabi

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
        .def<&arrays::scale>("array_scale")
        .def<&arrays::shift_vec2_array>("array_shift_vec2")
        .def<&arrays::sum_vec2_array>("array_sum_vec2")
        .def<&arrays::rotate_item_pointer_array>("array_rotate_item_pointers")
        .def<&arrays::sum_item_pointer_array>("array_sum_item_pointers")
        .def<&arrays::sum_ptr_fixed4>("array_sum_ptr_fixed4",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true})
        .def<&arrays::sum_ptr_counted>("array_sum_ptr_counted",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&arrays::add_scalar_ptr_counted>("array_add_scalar_ptr_counted",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&arrays::sum_ptr_counted_float>("array_sum_ptr_counted_float",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&arrays::add_scalar_ptr_counted_float>("array_add_scalar_ptr_counted_float",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1});

    module.class_<arrays::ArrayItem>()
        .ctor<int>()
        .def<&arrays::ArrayItem::get>()
        .def<&arrays::ArrayItem::set>();
}

} // namespace csbind23::testing
