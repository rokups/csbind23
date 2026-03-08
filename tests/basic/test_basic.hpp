#pragma once

#include "csbind23/bindings_generator.hpp"

namespace csbind23::testing::basic
{

struct BasicArrayRecord
{
    int left;
    int right;
};

class BasicArrayItem
{
public:
    explicit BasicArrayItem(int value)
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

template<typename T>
T get_and_return(T value)
{
    return value;
}

template<typename T, typename U>
T add_casted(T value, U delta)
{
    return static_cast<T>(value + static_cast<T>(delta));
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

inline void write_pair(int& first, int& second)
{
    first = 11;
    second = 22;
}

inline int sum_record_array(const BasicArrayRecord* value, int count)
{
    int total = 0;
    for (int index = 0; index < count; ++index)
    {
        total += value[index].left + value[index].right;
    }
    return total;
}

inline void add_to_record_array(BasicArrayRecord* value, int count, int delta)
{
    for (int index = 0; index < count; ++index)
    {
        value[index].left += delta;
        value[index].right += delta;
    }
}

inline int sum_item_pointer_array(BasicArrayItem** value, int count)
{
    int total = 0;
    for (int index = 0; index < count; ++index)
    {
        if (value[index] != nullptr)
        {
            total += value[index]->get();
        }
    }
    return total;
}

inline void rotate_item_pointer_array(BasicArrayItem** value, int count)
{
    if (count <= 1)
    {
        return;
    }

    BasicArrayItem* first = value[0];
    for (int index = 1; index < count; ++index)
    {
        value[index - 1] = value[index];
    }
    value[count - 1] = first;
}

}   // namespace csbind23::testing::basic

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::basic::BasicArrayRecord>
{
    using cpp_type = csbind23::testing::basic::BasicArrayRecord;
    using c_abi_type = cpp_type;

    static constexpr std::string_view c_abi_type_name() { return "csbind23::testing::basic::BasicArrayRecord"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::CsBind23.Tests.E2E.BasicArrayRecord"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.BasicArrayRecord"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "{value}"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "{value}"; }

    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<csbind23::testing::basic::BasicArrayRecord*>
{
    using cpp_type = csbind23::testing::basic::BasicArrayRecord*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::CsBind23.Tests.E2E.BasicArrayRecord"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.BasicArrayRecord"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "{value}"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "{value}"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <> struct Converter<const csbind23::testing::basic::BasicArrayRecord*>
{
    using cpp_type = const csbind23::testing::basic::BasicArrayRecord*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::CsBind23.Tests.E2E.BasicArrayRecord"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.BasicArrayRecord"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "{value}"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "{value}"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <> struct Converter<csbind23::testing::basic::BasicArrayItem*>
{
    using cpp_type = csbind23::testing::basic::BasicArrayItem*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.Basic.BasicArrayItem"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} is null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "(global::CsBind23.Tests.E2E.Basic.BasicArrayItem)global::CsBind23.Tests.E2E.Basic.BasicRuntime.WrapPolymorphicBasicArrayItem({value}, global::CsBind23.Generated.ItemOwnership.Borrowed)";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

} // namespace csbind23::cabi

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
        .def<&basic::assign_ptr<char16_t>>("assign_ptr_char16")
        .def<&basic::sum_record_array>("sum_record_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::add_to_record_array>("add_to_record_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::sum_item_pointer_array>("sum_item_pointer_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::rotate_item_pointer_array>("rotate_item_pointer_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::write_pair>("write_pair",
            csbind23::Arg{0, "left", true},
            csbind23::Arg{.idx = 1, .name = "right", .output = true});

    module.class_<basic::BasicArrayItem>()
        .ctor<int>()
        .def<&basic::BasicArrayItem::get>()
        .def<&basic::BasicArrayItem::set>();
}

} // namespace csbind23::testing
