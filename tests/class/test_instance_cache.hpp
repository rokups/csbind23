#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::instance_cache
{

class NativeHandleBackedCounter
{
public:
    explicit NativeHandleBackedCounter(int value)
        : value_(value)
    {
    }

    virtual ~NativeHandleBackedCounter() = default;

    virtual int add(int delta)
    {
        value_ += delta;
        return value_;
    }

    int add_through_native(int delta)
    {
        return add(delta);
    }

    void set_managed_handle(void* handle)
    {
        managed_handle_ = handle;
    }

    void* get_managed_handle() const
    {
        return managed_handle_;
    }

private:
    int value_;
    void* managed_handle_ = nullptr;
};

} // namespace csbind23::testing::instance_cache

namespace csbind23::testing
{

inline void register_bindings_instance_cache(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("InstanceCacheApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_instance_cache.hpp\"");

    module.class_<instance_cache::NativeHandleBackedCounter>()
        .set_instance_cache_type("CsBind23.Tests.E2E.NativeGcHandleInstanceCache<T>")
        .ctor<int>()
        .def<&instance_cache::NativeHandleBackedCounter::add>(csbind23::Virtual{})
        .def<&instance_cache::NativeHandleBackedCounter::add_through_native>()
        .def<&instance_cache::NativeHandleBackedCounter::set_managed_handle>(csbind23::PInvoke{})
        .def<&instance_cache::NativeHandleBackedCounter::get_managed_handle>(csbind23::PInvoke{});
}

} // namespace csbind23::testing
