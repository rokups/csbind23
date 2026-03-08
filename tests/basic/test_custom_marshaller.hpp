#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/detail/temporary_memory_allocator.hpp"

#include <cstring>
#include <string>
#include <string_view>

namespace csbind23::testing::custom_marshaller
{

struct CustomText
{
    std::string value;
};

inline CustomText echo(CustomText value)
{
    value.value += "|native";
    return value;
}

inline CustomText make_default()
{
    return CustomText{"seed"};
}

} // namespace csbind23::testing::custom_marshaller

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::custom_marshaller::CustomText>
{
    using cpp_type = csbind23::testing::custom_marshaller::CustomText;
    using c_abi_type = const char*;

    static constexpr std::string_view c_abi_type_name() { return "const char*"; }
    static constexpr std::string_view pinvoke_type_name()
    {
        return "global::CsBind23.Tests.E2E.CustomMarshaller.CustomText";
    }
    static constexpr std::string_view managed_type_name()
    {
        return "global::CsBind23.Tests.E2E.CustomMarshaller.CustomText";
    }
    static constexpr std::string_view managed_marshaller_type_name()
    {
        return "global::CsBind23.Tests.E2E.CustomMarshaller.CustomTextMarshaller";
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        auto* buffer = static_cast<char*>(csbind23::detail::temporary_memory_malloc(value.value.size() + 1));
        if (buffer == nullptr)
        {
            return nullptr;
        }

        std::memcpy(buffer, value.value.c_str(), value.value.size() + 1);
        return buffer;
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return cpp_type{value == nullptr ? std::string{} : std::string{value}};
    }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_bindings_custom_marshaller(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_custom_marshaller.hpp\"")
        .def<&custom_marshaller::echo>()
        .def<&custom_marshaller::make_default>();
}

} // namespace csbind23::testing
