#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23
{
class BindingsGenerator;
}

namespace csbind23::testing::class_wrapping
{

inline int sum(int left, int right)
{
    return left + right;
}

class Counter
{
public:
    explicit Counter(int start)
        : value_(start)
    {
    }

    int add(int delta)
    {
        value_ += delta;
        return value_;
    }

    int read() const
    {
        return value_;
    }

    void reset(int value)
    {
        value_ = value;
    }

private:
    int value_;
};

} // namespace csbind23::testing::class_wrapping

namespace csbind23::testing
{

inline void register_bindings_class(BindingsGenerator& generator, std::string_view module_name)
{
    (void)module_name;
    auto module = generator.module("class_module");
    module.csharp_api_class("ClassApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_class.hpp\"")
        .def<&class_wrapping::sum>();

    module.class_<class_wrapping::Counter>()
        .ctor<int>()
        .def<&class_wrapping::Counter::add>()
        .def<&class_wrapping::Counter::read>()
        .def<&class_wrapping::Counter::reset>();
}

} // namespace csbind23::testing
