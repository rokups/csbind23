#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace csbind23::core::strings
{

inline void* create(std::string value)
{
    return static_cast<void*>(new std::string(std::move(value)));
}

inline void destroy(void* value)
{
    delete static_cast<std::string*>(value);
}

inline std::string read(const void* value)
{
    if (value == nullptr)
    {
        return {};
    }
    return *static_cast<const std::string*>(value);
}

inline void assign(void* value, std::string text)
{
    if (value == nullptr)
    {
        return;
    }
    *static_cast<std::string*>(value) = std::move(text);
}

} // namespace csbind23::core::strings

namespace csbind23::core
{

inline void register_core_string_bindings(ModuleBuilder& module)
{
    module.cabi_include("<csbind23/core_runtime.hpp>")
        .def<&strings::create>("csbind23_string_create")
        .def<&strings::destroy>("csbind23_string_destroy")
        .def<&strings::read>("csbind23_string_read")
        .def<&strings::assign>("csbind23_string_assign");
}

inline void register_core_bindings(BindingsGenerator& generator, std::string_view module_name,
    std::string_view pinvoke_library)
{
    auto module = generator.module(module_name).csharp_api_class("CsBind23CoreApi");
    if (!pinvoke_library.empty())
    {
        module.pinvoke_library(pinvoke_library);
    }
    register_core_string_bindings(module);
}

} // namespace csbind23::core
