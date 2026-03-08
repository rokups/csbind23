#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std.hpp"

#include <string>
#include <string_view>

namespace csbind23::testing::map_tests
{

class MapItem
{
public:
    explicit MapItem(int value)
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

    bool operator==(const MapItem& other) const
    {
        return value_ == other.value_;
    }

private:
    int value_;
};

} // namespace csbind23::testing::map_tests

using MapItem = csbind23::testing::map_tests::MapItem;

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::map_tests::MapItem>
{
    using cpp_type = csbind23::testing::map_tests::MapItem;
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

template <> struct Converter<const csbind23::testing::map_tests::MapItem&>
{
    using cpp_type = const csbind23::testing::map_tests::MapItem&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const csbind23::testing::map_tests::MapItem*>(value); }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_bindings_map(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("MapApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/stl/test_map.hpp\"");

    module.class_<map_tests::MapItem>()
        .ctor<int>()
        .def<&map_tests::MapItem::get>()
        .def<&map_tests::MapItem::set>();

    csbind23::add_map<
        TemplateArgs<long long, int>,
        TemplateArgs<std::string, int>,
        TemplateArgs<long long, map_tests::MapItem>>(generator);
    csbind23::add_unordered_map<
        TemplateArgs<long long, int>,
        TemplateArgs<std::string, int>,
        TemplateArgs<long long, map_tests::MapItem*>>(generator);
}

} // namespace csbind23::testing