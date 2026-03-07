#pragma once

#include "csbind23/std/string.hpp"
#include "csbind23/bindings_generator.hpp"

#include <format>
#include <string>
#include <string_view>

namespace csbind23::testing::class_generic
{

template <typename T>
class GenericCounter
{
public:
    GenericCounter() = default;

    void set(T value)
    {
        value_ = value;
    }

    T add(T delta)
    {
        value_ = static_cast<T>(value_ + delta);
        return value_;
    }

    T get() const
    {
        return value_;
    }

    void add_into_int_ref(int& target) const
    {
        target += static_cast<int>(value_);
    }

    void add_into_value_ref(T& target) const
    {
        target = static_cast<T>(target + value_);
    }

    void add_pair_into_value_out(T left, T& out_value) const
    {
        out_value = static_cast<T>(left + value_);
    }

    void add_into_value_ptr(T* target) const
    {
        if (target != nullptr)
        {
            *target = static_cast<T>(*target + value_);
        }
    }

    std::string describe(std::string_view label) const
    {
        return std::format("{}:{}", label, value_);
    }

    template <typename U>
    U cast_add(U delta) const
    {
        return static_cast<U>(value_) + delta;
    }

    template <typename U>
    U mutate_and_cast(U delta)
    {
        value_ = static_cast<T>(value_ + static_cast<T>(delta));
        return static_cast<U>(value_);
    }

    template <typename U, typename V>
    U add_pair(U left, V right) const
    {
        return static_cast<U>(left + static_cast<U>(right) + static_cast<U>(value_));
    }

    int cast_add_int(int delta) const
    {
        return cast_add<int>(delta);
    }

    double cast_add_double(double delta) const
    {
        return cast_add<double>(delta);
    }

    int mutate_and_cast_int(int delta)
    {
        return mutate_and_cast<int>(delta);
    }

    double mutate_and_cast_double(double delta)
    {
        return mutate_and_cast<double>(delta);
    }

    int add_pair_int_double(int left, double right) const
    {
        return add_pair<int, double>(left, right);
    }

    double add_pair_double_int(double left, int right) const
    {
        return add_pair<double, int>(left, right);
    }

private:
    T value_{};
};

template <typename T, typename U>
class GenericPair
{
public:
    GenericPair() = default;

    void set(T left, U right)
    {
        left_ = left;
        right_ = right;
    }

    double sum_as_double() const
    {
        return static_cast<double>(left_) + static_cast<double>(right_);
    }

private:
    T left_{};
    U right_{};
};

class RefGenericOps
{
public:
    template <typename T>
    void add_to_ref(T& value, T delta) const
    {
        value = static_cast<T>(value + delta);
    }

    template <typename T>
    void add_to_out(T left, T right, T& out_value) const
    {
        out_value = static_cast<T>(left + right);
    }

    template <typename T>
    void add_to_ptr(T* value, T delta) const
    {
        if (value != nullptr)
        {
            *value = static_cast<T>(*value + delta);
        }
    }

    template <typename T, typename U>
    U add_mixed(T left, U right) const
    {
        return static_cast<U>(left) + right;
    }
};

} // namespace csbind23::testing::class_generic

namespace csbind23::testing
{

inline void register_bindings_class_generic(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("ClassGenericApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_class_generic.hpp\"");

    module.class_generic<class_generic::GenericCounter, int, double>("GenericCounter")
        .ctor<>()
        .def_generic<[]<typename ClassType>() { return &ClassType::set; }>("set")
        .def_generic<[]<typename ClassType>() { return &ClassType::add; }>("add")
        .def_generic<[]<typename ClassType>() { return &ClassType::get; }>("get")
        .def_generic<[]<typename ClassType>() { return &ClassType::add_into_int_ref; }>("add_into_int_ref")
        .def_generic<[]<typename ClassType>() { return &ClassType::add_into_value_ref; }>("add_into_value_ref")
        .def_generic<[]<typename ClassType>() { return &ClassType::add_pair_into_value_out; }>(
            "add_pair_into_value_out",
            csbind23::Arg{.idx = 1, .name = "result", .output = true})
        .def_generic<[]<typename ClassType>() { return &ClassType::add_into_value_ptr; }>("add_into_value_ptr")
        .def_generic<[]<typename ClassType>() { return &ClassType::describe; }>(
            "describe")
        .def_generic<[]<typename ClassType>() { return &ClassType::cast_add_int; }>(
            "cast_add_int")
        .def_generic<[]<typename ClassType>() { return &ClassType::cast_add_double; }>("cast_add_double")
        .def_generic<[]<typename ClassType>() { return &ClassType::mutate_and_cast_int; }>("mutate_and_cast_int")
        .def_generic<[]<typename ClassType>() { return &ClassType::mutate_and_cast_double; }>(
            "mutate_and_cast_double")
        .def_generic<[]<typename ClassType>() { return &ClassType::add_pair_int_double; }>("add_pair_int_double")
        .def_generic<[]<typename ClassType>() { return &ClassType::add_pair_double_int; }>("add_pair_double_int");

    module.class_generic<
            class_generic::GenericPair,
            csbind23::TemplateArgs<int, double>,
            csbind23::TemplateArgs<double, int>>("GenericPair")
        .ctor<>()
        .def_generic<[]<typename ClassType>() { return &ClassType::set; }>("set")
        .def_generic<[]<typename ClassType>() { return &ClassType::sum_as_double; }>("sum_as_double");

    module.class_<class_generic::RefGenericOps>()
        .ctor<>()
        .def_generic<
            []<typename T>() { return &class_generic::RefGenericOps::add_to_ref<T>; },
            int,
            double>(
            "add_to_ref_generic")
        .def_generic<
            []<typename T>() { return &class_generic::RefGenericOps::add_to_out<T>; },
            int,
            double>(
            "add_to_out_generic",
            csbind23::Arg{.idx = 2, .name = "result", .output = true})
        .def_generic<
            []<typename T>() { return &class_generic::RefGenericOps::add_to_ptr<T>; },
            int,
            double>(
            "add_to_ptr_generic")
        .def_generic<
            []<typename T, typename U>() { return &class_generic::RefGenericOps::add_mixed<T, U>; },
            csbind23::TemplateArgs<int, double>,
            csbind23::TemplateArgs<double, int>>(
            "add_mixed_generic");
}

} // namespace csbind23::testing
