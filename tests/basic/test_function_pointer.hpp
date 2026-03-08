#pragma once

#include "csbind23/bindings_generator.hpp"

namespace csbind23::testing::function_pointer
{

using BinaryOp = int (*)(int, int);
using UnaryOp = int (*)(int);

inline int add(int left, int right)
{
    return left + right;
}

inline int subtract(int left, int right)
{
    return left - right;
}

inline int square(int value)
{
    return value * value;
}

inline int negate(int value)
{
    return -value;
}

inline BinaryOp& stored_binary()
{
    static BinaryOp value = nullptr;
    return value;
}

inline UnaryOp& stored_unary()
{
    static UnaryOp value = nullptr;
    return value;
}

inline int invoke_binary(BinaryOp op, int left, int right)
{
    return op == nullptr ? -9999 : op(left, right);
}

inline int invoke_unary(UnaryOp op, int value)
{
    return op == nullptr ? -9999 : op(value);
}

inline BinaryOp choose_binary(bool subtract_mode)
{
    return subtract_mode ? &subtract : &add;
}

inline UnaryOp choose_unary(bool negate_mode)
{
    return negate_mode ? &negate : &square;
}

inline void set_stored_binary(BinaryOp op)
{
    stored_binary() = op;
}

inline int invoke_stored_binary(int left, int right)
{
    return invoke_binary(stored_binary(), left, right);
}

inline void clear_stored_binary()
{
    stored_binary() = nullptr;
}

inline void set_stored_unary(UnaryOp op)
{
    stored_unary() = op;
}

inline int invoke_stored_unary(int value)
{
    return invoke_unary(stored_unary(), value);
}

inline void clear_stored_unary()
{
    stored_unary() = nullptr;
}

} // namespace csbind23::testing::function_pointer

namespace csbind23::testing
{

inline void register_bindings_function_pointer(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("FunctionPointerApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_function_pointer.hpp\"")
        .def<&function_pointer::invoke_binary>("fp_invoke_binary")
        .def<&function_pointer::invoke_unary>("fp_invoke_unary")
        .def<&function_pointer::choose_binary>("fp_choose_binary")
        .def<&function_pointer::choose_unary>("fp_choose_unary")
        .def<&function_pointer::set_stored_binary>("fp_set_stored_binary")
        .def<&function_pointer::invoke_stored_binary>("fp_invoke_stored_binary")
        .def<&function_pointer::clear_stored_binary>("fp_clear_stored_binary")
        .def<&function_pointer::set_stored_unary>("fp_set_stored_unary")
        .def<&function_pointer::invoke_stored_unary>("fp_invoke_stored_unary")
        .def<&function_pointer::clear_stored_unary>("fp_clear_stored_unary");
}

} // namespace csbind23::testing
