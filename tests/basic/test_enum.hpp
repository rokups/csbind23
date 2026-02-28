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
        .enum_<basic_enum::BasicColor>("BasicColor")
            .value("Red", basic_enum::BasicColor::Red)
            .value("Green", basic_enum::BasicColor::Green)
            .value("Blue", basic_enum::BasicColor::Blue);

    module.def<&basic_enum::get_and_return_color>("get_and_return_color")
        .def<&basic_enum::set_color_by_ref>("set_color_by_ref")
        .def<&basic_enum::set_color_by_ptr>("set_color_by_ptr");
}

} // namespace csbind23::testing
