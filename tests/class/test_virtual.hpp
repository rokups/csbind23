#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::virtual_dispatch
{

class VirtualCounter
{
public:
    virtual ~VirtualCounter() = default;

    explicit VirtualCounter(int value)
        : value_(value)
    {
    }

    virtual int add(int delta)
    {
        value_ += delta;
        return value_;
    }

    virtual int read() const
    {
        return value_;
    }

    int add_through_native(int delta)
    {
        return add(delta);
    }

    int read_through_native() const
    {
        return read();
    }

    std::string describe_through_native() const
    {
        return describe();
    }

    virtual std::string describe() const
    {
        return "VirtualCounter";
    }

private:
    int value_;
};

} // namespace csbind23::testing::virtual_dispatch

namespace csbind23::testing
{

inline void register_bindings_virtual(BindingsGenerator& generator, std::string_view module_name)
{
    (void)module_name;
    auto module = generator.module("virtual_module");
    module.csharp_api_class("VirtualApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_virtual.hpp\"");

    module.class_<virtual_dispatch::VirtualCounter>()
        .ctor<int>()
        .def_virtual<&virtual_dispatch::VirtualCounter::add>()
        .def_virtual<&virtual_dispatch::VirtualCounter::read>()
        .def_virtual<&virtual_dispatch::VirtualCounter::describe>()
        .def<&virtual_dispatch::VirtualCounter::add_through_native>()
        .def<&virtual_dispatch::VirtualCounter::read_through_native>()
        .def<&virtual_dispatch::VirtualCounter::describe_through_native>();
}

} // namespace csbind23::testing
