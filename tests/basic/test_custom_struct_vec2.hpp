#pragma once

#include "csbind23/bindings_generator.hpp"

#include <array>
#include <string_view>

namespace csbind23::testing::custom_struct_vec2
{

class Vec2
{
public:
    Vec2() = default;

    Vec2(float x, float y)
        : components_{x, y}
    {
    }

    float GetX() const
    {
        return components_[0];
    }

    float GetY() const
    {
        return components_[1];
    }

    float LengthSquared() const
    {
        return (GetX() * GetX()) + (GetY() * GetY());
    }

private:
    std::array<float, 2> components_{};
};

inline Vec2 make_vec2(float x, float y)
{
    return Vec2{x, y};
}

inline float component_sum(const Vec2& value)
{
    return value.GetX() + value.GetY();
}

inline float dot_vec2(const Vec2& left, const Vec2& right)
{
    return (left.GetX() * right.GetX()) + (left.GetY() * right.GetY());
}

inline Vec2 add_vec2(const Vec2& left, const Vec2& right)
{
    return Vec2{left.GetX() + right.GetX(), left.GetY() + right.GetY()};
}

inline Vec2 scale_vec2(const Vec2& value, float factor)
{
    return Vec2{value.GetX() * factor, value.GetY() * factor};
}

} // namespace csbind23::testing::custom_struct_vec2

namespace csbind23::cabi
{

struct PODVec2
{
    float x;
    float y;
};

static_assert(sizeof(PODVec2) == sizeof(testing::custom_struct_vec2::Vec2));

} // namespace csbind23::cabi

namespace csbind23::testing::custom_struct_vec2
{

inline csbind23::cabi::PODVec2 add_pod_vec2(csbind23::cabi::PODVec2 left, csbind23::cabi::PODVec2 right)
{
    return csbind23::cabi::PODVec2{left.x + right.x, left.y + right.y};
}

} // namespace csbind23::testing::custom_struct_vec2

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::custom_struct_vec2::Vec2>
{
    using cpp_type = csbind23::testing::custom_struct_vec2::Vec2;
    using c_abi_type = PODVec2;

    static constexpr std::string_view c_abi_type_name() { return "csbind23::cabi::PODVec2"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::CsBind23.Tests.E2E.SequentialVec2"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.SequentialVec2"; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{value.GetX(), value.GetY()};
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return cpp_type{value.x, value.y};
    }
};

template <> struct Converter<const csbind23::testing::custom_struct_vec2::Vec2&>
{
    using cpp_type = csbind23::testing::custom_struct_vec2::Vec2;
    using c_abi_type = const PODVec2*;

    static constexpr std::string_view c_abi_type_name() { return "const csbind23::cabi::PODVec2*"; }
    static constexpr std::string_view pinvoke_type_name() { return "in global::CsBind23.Tests.E2E.SequentialVec2"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.SequentialVec2"; }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return cpp_type{value->x, value->y};
    }
};

template <> struct Converter<PODVec2>
{
    using cpp_type = PODVec2;
    using c_abi_type = PODVec2;

    static constexpr std::string_view c_abi_type_name() { return "csbind23::cabi::PODVec2"; }
    static constexpr std::string_view pinvoke_type_name() { return "global::CsBind23.Tests.E2E.SequentialVec2"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Tests.E2E.SequentialVec2"; }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_bindings_custom_struct_vec2(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("CustomStructVec2Api")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_custom_struct_vec2.hpp\"")
        .def<&custom_struct_vec2::make_vec2>()
        .def<&custom_struct_vec2::component_sum>()
        .def<&custom_struct_vec2::dot_vec2>()
        .def<&custom_struct_vec2::add_vec2>()
        .def<&custom_struct_vec2::scale_vec2>()
        .def<&custom_struct_vec2::add_pod_vec2>();
}

} // namespace csbind23::testing
