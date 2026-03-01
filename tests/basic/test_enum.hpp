#pragma once

#include "csbind23/bindings_generator.hpp"

#include <cstdint>
#include <string_view>

namespace csbind23::testing::basic_enum
{

enum class BasicColor : std::int32_t
{
    Red = 1,
    Green = 2,
    Blue = 4,
};

enum class Permissions : std::uint8_t
{
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4,
};

inline BasicColor get_and_return_color(BasicColor color)
{
    return color;
}

inline void set_color_by_ref(BasicColor& color, BasicColor next)
{
    color = next;
}

inline void set_color_by_ptr(BasicColor* color, BasicColor next)
{
    if (color == nullptr)
    {
        return;
    }

    *color = next;
}

inline Permissions bit_or_permissions(Permissions lhs, Permissions rhs)
{
    return static_cast<Permissions>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

inline bool has_permission(Permissions value, Permissions flag)
{
    return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(flag)) != 0;
}

class AttributedCounter
{
public:
    explicit AttributedCounter(std::int32_t initial)
        : value_(initial)
    {
    }

    void add(std::int32_t delta)
    {
        value_ += delta;
    }

    std::int32_t read() const
    {
        return value_;
    }

private:
    std::int32_t value_ = 0;
};

} // namespace csbind23::testing::basic_enum

namespace csbind23::testing
{

inline void register_bindings_enum(BindingsGenerator& generator, std::string_view module_name)
{
    (void)module_name;
    auto module = generator.module("enum_module");
    module.csharp_api_class("EnumApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_enum.hpp\"")
        .enum_<basic_enum::BasicColor>("BasicColor",
            csbind23::Attribute{"global::CsBind23.Tests.E2E.BindingMarkerAttribute"})
            .value("Red", basic_enum::BasicColor::Red)
            .value("Green", basic_enum::BasicColor::Green)
            .value("Blue", basic_enum::BasicColor::Blue);

    module.enum_<basic_enum::Permissions>("Permissions", csbind23::Flags{},
              csbind23::Attribute{"global::CsBind23.Tests.E2E.BindingMarkerAttribute"})
        .value("None", basic_enum::Permissions::None)
        .value("Read", basic_enum::Permissions::Read)
        .value("Write", basic_enum::Permissions::Write)
        .value("Execute", basic_enum::Permissions::Execute);

    module.class_<basic_enum::AttributedCounter>("AttributedCounter",
              csbind23::CppName{"csbind23::testing::basic_enum::AttributedCounter"},
              csbind23::Attribute{"global::CsBind23.Tests.E2E.BindingMarkerAttribute"})
        .ctor<std::int32_t>()
        .def<&basic_enum::AttributedCounter::add>(
            csbind23::Attribute{"global::CsBind23.Tests.E2E.BindingMarkerAttribute"})
        .def<&basic_enum::AttributedCounter::read>();

    module.def<&basic_enum::get_and_return_color>("get_and_return_color",
              csbind23::Attribute{"global::CsBind23.Tests.E2E.BindingMarkerAttribute"})
        .def<&basic_enum::set_color_by_ref>("set_color_by_ref")
        .def<&basic_enum::set_color_by_ptr>("set_color_by_ptr")
        .def<&basic_enum::bit_or_permissions>("bit_or_permissions")
        .def<&basic_enum::has_permission>("has_permission");
}

} // namespace csbind23::testing
