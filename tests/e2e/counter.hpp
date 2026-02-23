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
    virtual ~Accumulator() = default;

    explicit Accumulator(int start)
        : value_(start)
    {
    }

    virtual int add(int delta)
    {
        value_ += delta;
        return value_;
    }

    virtual int read() const { return value_; }

    int add_through_native(int delta)
    {
        return add(delta);
    }

    int read_through_native() const
    {
        return read();
    }

private:
    int value_;
};

class FancyAccumulator : public Accumulator
{
public:
    explicit FancyAccumulator(int start)
        : Accumulator(start)
    {
    }

    int add(int delta) override
    {
        return Accumulator::add(delta) + 1000;
    }
};

inline Accumulator* make_polymorphic(bool derived)
{
    static Accumulator base_accumulator{10};
    static FancyAccumulator fancy_accumulator{10};
    return derived ? static_cast<Accumulator*>(&fancy_accumulator) : static_cast<Accumulator*>(&base_accumulator);
}

} // namespace csbind23::testing::counter

namespace csbind23::testing
{

inline void register_bindings(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.pinvoke_library("e2e.C")
        .cabi_include("\"tests/e2e/counter.hpp\"")
        .def<&counter::sum>()
        .def(
            "make_polymorphic",
            &counter::make_polymorphic,
            Ownership::Borrowed,
            "csbind23::testing::counter::make_polymorphic");

    module.class_<counter::Accumulator>()
        .ctor<int>()
        .def_virtual<&counter::Accumulator::add>()
        .def_virtual<&counter::Accumulator::read>()
        .def<&counter::Accumulator::add_through_native>()
        .def<&counter::Accumulator::read_through_native>();

    module.class_<counter::FancyAccumulator>()
        .def<&counter::FancyAccumulator::add>();
}

} // namespace csbind23::testing
