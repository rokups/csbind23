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

int Counter::increment_through_native(int delta)
{
    return increment(delta);
}

int Counter::read_through_native() const
{
    return read();
}

int FancyCounter::increment(int delta)
{
    return Counter::increment(delta) + 1000;
}

Counter* make_polymorphic_counter(bool derived)
{
    static Counter base_counter{10};
    static FancyCounter fancy_counter{10};
    return derived ? static_cast<Counter*>(&fancy_counter) : static_cast<Counter*>(&base_counter);
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
        .def<&domain::make_polymorphic_counter>("make_polymorphic_counter");

    module.class_<domain::Counter>()
        .ctor<int>()
        .def_virtual<&domain::Counter::increment>()
        .def_virtual<&domain::Counter::read>()
        .def<&domain::Counter::increment_through_native>()
        .def<&domain::Counter::read_through_native>();

    module.class_<domain::FancyCounter, domain::Counter>()
        .def<&domain::FancyCounter::increment>();
}

} // namespace example
