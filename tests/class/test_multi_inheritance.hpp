#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::multi_inheritance
{

class PrimaryBase
{
public:
    explicit PrimaryBase(int value)
        : value_(value)
    {
    }

    int primary_add(int delta)
    {
        value_ += delta;
        return value_;
    }

private:
    int value_;
};

class SecondaryBase
{
public:
    virtual ~SecondaryBase() = default;

    int secondary_scale(int value) const
    {
        return value * 2;
    }

    virtual int secondary_virtual(int value)
    {
        return value + 1;
    }

    int secondary_virtual_through_native(int value)
    {
        return secondary_virtual(value);
    }
};

class TertiaryBase
{
public:
    int tertiary_subtract(int value) const
    {
        return value - 3;
    }
};

class MultiDerived : public PrimaryBase, public SecondaryBase, public TertiaryBase
{
public:
    explicit MultiDerived(int value)
        : PrimaryBase(value)
    {
    }

    int derived_twice_primary(int delta)
    {
        primary_add(delta);
        return primary_add(delta);
    }

    int call_secondary_virtual_from_cpp(int value)
    {
        return secondary_virtual_through_native(value);
    }
};

} // namespace csbind23::testing::multi_inheritance

namespace csbind23::testing
{

inline void register_bindings_multi_inheritance(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("MultiInheritanceApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_multi_inheritance.hpp\"");

    module.class_<multi_inheritance::PrimaryBase>()
        .ctor<int>()
        .def<&multi_inheritance::PrimaryBase::primary_add>();

    module.class_<multi_inheritance::SecondaryBase>()
        .def<&multi_inheritance::SecondaryBase::secondary_scale>()
        .def<&multi_inheritance::SecondaryBase::secondary_virtual>(csbind23::Virtual{})
        .def<&multi_inheritance::SecondaryBase::secondary_virtual_through_native>();

    module.class_<multi_inheritance::TertiaryBase>()
        .def<&multi_inheritance::TertiaryBase::tertiary_subtract>();

    module.class_<multi_inheritance::MultiDerived, multi_inheritance::PrimaryBase, multi_inheritance::SecondaryBase,
        multi_inheritance::TertiaryBase>()
        .ctor<int>()
        .def<&multi_inheritance::MultiDerived::derived_twice_primary>()
        .def<&multi_inheritance::MultiDerived::call_secondary_virtual_from_cpp>();
}

} // namespace csbind23::testing
