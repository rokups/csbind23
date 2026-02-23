#include "bindings_declarations.hpp"

#include "csbind23/bindings_generator.hpp"

namespace example::domain
{

int add(int left, int right)
{
    return left + right;
}

double scale(double value, double factor)
{
    return value * factor;
}

Counter::Counter(int start)
    : value_(start)
{
}

int Counter::increment(int delta)
{
    value_ += delta;
    return value_;
}

int Counter::read() const
{
    return value_;
}

} // namespace example::domain

namespace example
{

void register_bindings(csbind23::BindingsGenerator& generator)
{
    auto module = generator.module("playground");
    module.pinvoke_library("playground.C")
        .cabi_include("\"example/bindings_declarations.hpp\"")
        .def<&domain::add>()
        .def<&domain::scale>()
        .class_<domain::Counter>()
        .ctor<int>()
        .def<&domain::Counter::increment>()
        .def<&domain::Counter::read>();
}

} // namespace example
