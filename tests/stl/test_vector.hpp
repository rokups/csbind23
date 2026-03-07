#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std.hpp"

#include <string_view>

namespace csbind23::testing::vector_tests
{

class VectorItem
{
public:
    explicit VectorItem(int value)
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

    bool operator==(const VectorItem& other) const
    {
        return value_ == other.value_;
    }

private:
    int value_;
};

} // namespace csbind23::testing::vector_tests

using VectorItem = csbind23::testing::vector_tests::VectorItem;

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::vector_tests::VectorItem>
{
    using cpp_type = csbind23::testing::vector_tests::VectorItem;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return static_cast<c_abi_type>(&value);
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return *static_cast<const cpp_type*>(value);
    }
};

template <> struct Converter<const csbind23::testing::vector_tests::VectorItem&>
{
    using cpp_type = const csbind23::testing::vector_tests::VectorItem&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const csbind23::testing::vector_tests::VectorItem*>(value); }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_bindings_vector(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("VectorApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/stl/test_vector.hpp\"");

    module.class_<vector_tests::VectorItem>()
        .ctor<int>()
        .def<&vector_tests::VectorItem::get>()
        .def<&vector_tests::VectorItem::set>();

    csbind23::add_vector<double>(module);
    csbind23::add_vector<vector_tests::VectorItem, int>(module);
    csbind23::add_vector<vector_tests::VectorItem*>(module);

}

} // namespace csbind23::testing
