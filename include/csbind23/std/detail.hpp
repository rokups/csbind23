#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/type_utils.hpp"

#include <string>
#include <string_view>
#include <type_traits>

namespace csbind23::detail
{
inline ModuleBuilder ensure_stl_module(BindingsGenerator& generator)
{
    auto module = generator.module(stl_module_name());
    module.cabi_include("<csbind23/std.hpp>");
    return module;
}

template <typename Type>
std::string fully_qualified_cpp_type_name()
{
    using NoRef = std::remove_reference_t<Type>;
    using Base = std::remove_cv_t<std::remove_pointer_t<NoRef>>;

    std::string full_name;
    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (std::is_const_v<std::remove_pointer_t<NoRef>>)
        {
            full_name += "const ";
        }
    }
    else if constexpr (std::is_const_v<NoRef>)
    {
        full_name += "const ";
    }

    full_name += detail::qualified_type_name<Base>();

    if constexpr (std::is_pointer_v<NoRef>)
    {
        full_name += "*";
    }

    if constexpr (std::is_reference_v<Type>)
    {
        full_name += "&";
    }

    return full_name;
}

} // namespace csbind23::detail
