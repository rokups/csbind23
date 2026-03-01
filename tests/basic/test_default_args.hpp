#pragma once

#include "csbind23/bindings_generator.hpp"

namespace csbind23::testing::default_args
{

inline int sum3(int a, int b = 5, int c = 7)
{
    return a + b + c;
}

inline int weighted(int base, int multiplier = 2, int offset = 1)
{
    return base * multiplier + offset;
}

inline double affine(double value, double scale = 1.5, double bias = 0.25)
{
    return value * scale + bias;
}

inline int four_defaults(int a, int b = 10, int c = 20, int d = 30)
{
    return a + b + c + d;
}

} // namespace csbind23::testing::default_args

namespace csbind23::testing
{

inline void register_bindings_default_args(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("DefaultArgsApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_default_args.hpp\"")
        .def_with_defaults<&default_args::sum3>("sum3", 2)
        .def_with_defaults<&default_args::weighted>("weighted", 2)
        .def_with_defaults<&default_args::affine>("affine", 2)
        .def_with_defaults<&default_args::four_defaults>("four_defaults", 3);
}

} // namespace csbind23::testing
