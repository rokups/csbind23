#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std/functional.hpp"

#include <functional>

namespace csbind23::testing::functional
{

inline std::function<int(int, int)>& stored_binary()
{
    static std::function<int(int, int)> value;
    return value;
}

inline std::function<int(int)>& stored_unary()
{
    static std::function<int(int)> value;
    return value;
}

inline int invoke_binary(const std::function<int(int, int)>& fn, int left, int right)
{
    return fn ? fn(left, right) : -7777;
}

inline int invoke_unary(const std::function<int(int)>& fn, int value)
{
    return fn ? fn(value) : -7777;
}

inline std::function<int(int, int)> choose_binary(bool subtract_mode)
{
    if (subtract_mode)
    {
        return [](int left, int right) { return left - right; };
    }

    return [](int left, int right) { return left + right; };
}

inline std::function<int(int)> choose_unary(int delta)
{
    return [delta](int value) { return value + delta; };
}

inline void set_stored_binary(std::function<int(int, int)> fn)
{
    stored_binary() = std::move(fn);
}

inline int invoke_stored_binary(int left, int right)
{
    return invoke_binary(stored_binary(), left, right);
}

inline void clear_stored_binary()
{
    stored_binary() = {};
}

inline void set_stored_unary(std::function<int(int)> fn)
{
    stored_unary() = std::move(fn);
}

inline int invoke_stored_unary(int value)
{
    return invoke_unary(stored_unary(), value);
}

inline void clear_stored_unary()
{
    stored_unary() = {};
}

} // namespace csbind23::testing::functional

namespace csbind23::testing
{

inline void register_bindings_functional(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("FunctionalApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/stl/test_functional.hpp\"")
        .def<&functional::invoke_binary>("functional_invoke_binary")
        .def<&functional::invoke_unary>("functional_invoke_unary")
        .def<&functional::choose_binary>("functional_choose_binary")
        .def<&functional::choose_unary>("functional_choose_unary")
        .def<&functional::set_stored_binary>("functional_set_stored_binary")
        .def<&functional::invoke_stored_binary>("functional_invoke_stored_binary")
        .def<&functional::clear_stored_binary>("functional_clear_stored_binary")
        .def<&functional::set_stored_unary>("functional_set_stored_unary")
        .def<&functional::invoke_stored_unary>("functional_invoke_stored_unary")
        .def<&functional::clear_stored_unary>("functional_clear_stored_unary");
}

} // namespace csbind23::testing
