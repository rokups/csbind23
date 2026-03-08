// csbind23: helper-only
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

} // namespace csbind23::testing::basic

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

inline void register_bindings_basic_array(ModuleBuilder& module)
{
    module.def<&basic::sum_record_array>("sum_record_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::add_to_record_array>("add_to_record_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::sum_item_pointer_array>("sum_item_pointer_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1})
        .def<&basic::rotate_item_pointer_array>("rotate_item_pointer_array",
            csbind23::Arg{.idx = 0, .name = "value", .c_array = true, .size_param_idx = 1});

    module.class_<basic::BasicArrayItem>()
        .ctor<int>()
        .def<&basic::BasicArrayItem::get>()
        .def<&basic::BasicArrayItem::set>();
}

} // namespace csbind23::testing