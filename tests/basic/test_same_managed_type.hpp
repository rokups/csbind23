#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::same_managed_type
{

struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;

    Vector2(float x_value, float y_value)
        : x(x_value)
        , y(y_value)
    {
    }

    float GetX() const
    {
        return x;
    }

    float GetY() const
    {
        return y;
    }
};

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

inline Vector2 make_vector2(float x, float y)
{
    return Vector2{x, y};
}

inline Vec2 make_vec2(float x, float y)
{
    return Vec2{x, y};
}

inline float dot_vector2(Vector2 left, Vector2 right)
{
    return (left.GetX() * right.GetX()) + (left.GetY() * right.GetY());
}

inline float dot_vec2(Vec2 left, Vec2 right)
{
    return (left.x * right.x) + (left.y * right.y);
}

inline Vec2 add_mixed(Vector2 left, Vec2 right)
{
    return Vec2{left.GetX() + right.x, left.GetY() + right.y};
}

inline Vector2 scale_vec2(Vec2 value, float factor)
{
    return Vector2{value.x * factor, value.y * factor};
}

} // namespace csbind23::testing::same_managed_type

namespace csbind23::cabi
{

struct SystemNumericsVector2
{
    float x;
    float y;
};

template <> struct Converter<csbind23::testing::same_managed_type::Vector2>
{
    using cpp_type = csbind23::testing::same_managed_type::Vector2;
    using c_abi_type = SystemNumericsVector2;

    static constexpr std::string_view c_abi_type_name() { return "csbind23::cabi::SystemNumericsVector2"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::System.Numerics.Vector2"; }
    static constexpr std::string_view managed_type_name() { return "global::System.Numerics.Vector2"; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{value.GetX(), value.GetY()};
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return cpp_type{value.x, value.y};
    }
};

template <> struct Converter<csbind23::testing::same_managed_type::Vec2>
{
    using cpp_type = csbind23::testing::same_managed_type::Vec2;
    using c_abi_type = SystemNumericsVector2;

    static constexpr std::string_view c_abi_type_name() { return "csbind23::cabi::SystemNumericsVector2"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::System.Numerics.Vector2"; }
    static constexpr std::string_view managed_type_name() { return "global::System.Numerics.Vector2"; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{value.x, value.y};
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return cpp_type{value.x, value.y};
    }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_bindings_same_managed_type(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("SameManagedTypeApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_same_managed_type.hpp\"")
        .def<&same_managed_type::make_vector2>()
        .def<&same_managed_type::make_vec2>()
        .def<&same_managed_type::dot_vector2>()
        .def<&same_managed_type::dot_vec2>()
        .def<&same_managed_type::add_mixed>()
        .def<&same_managed_type::scale_vec2>();
}

} // namespace csbind23::testing
