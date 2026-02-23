#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23
{
class BindingsGenerator;
}

namespace csbind23::testing::counter
{

inline int sum(int left, int right)
{
    return left + right;
}

class Accumulator
{
public:
    explicit Accumulator(int start)
        : value_(start)
    {
    }

    int add(int delta)
    {
        value_ += delta;
        return value_;
    }

    int read() const { return value_; }

private:
    int value_;
};

} // namespace csbind23::testing::counter

namespace csbind23::testing
{

inline void register_bindings(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.pinvoke_library("e2e.C")
        .cabi_include("\"tests/e2e/counter.hpp\"")
        .def<&counter::sum>()
        .class_<counter::Accumulator>()
        .ctor<int>()
        .def<&counter::Accumulator::add>()
        .def<&counter::Accumulator::read>();
}

} // namespace csbind23::testing
