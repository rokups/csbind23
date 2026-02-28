#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::inheritance
{

inline int sum(int left, int right)
{
    return left + right;
}

class BaseValue
{
public:
    explicit BaseValue(int value)
        : value_(value)
    {
    }

    int increment(int delta)
    {
        value_ += delta;
        return value_;
    }

    int read() const
    {
        return value_;
    }

private:
    int value_;
};

class DerivedValue : public BaseValue
{
public:
    explicit DerivedValue(int value)
        : BaseValue(value)
    {
    }

    int increment_twice(int delta)
    {
        increment(delta);
        return increment(delta);
    }

    int increment_from_base(int delta)
    {
        return BaseValue::increment(delta);
    }

    int read_from_base() const
    {
        return BaseValue::read();
    }
};

} // namespace csbind23::testing::inheritance

namespace csbind23::testing
{

inline void register_bindings_inheritance(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("InheritanceApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_inheritance.hpp\"")
        .def<&inheritance::sum>();

    module.class_<inheritance::BaseValue>()
        .ctor<int>()
        .def<&inheritance::BaseValue::increment>()
        .def<&inheritance::BaseValue::read>();

    module.class_<inheritance::DerivedValue>()
        .ctor<int>()
        .def<&inheritance::DerivedValue::increment_twice>()
        .def<&inheritance::DerivedValue::increment_from_base>()
        .def<&inheritance::DerivedValue::read_from_base>();
}

} // namespace csbind23::testing
