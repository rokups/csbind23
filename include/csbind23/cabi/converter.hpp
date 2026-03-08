#pragma once

#include "csbind23/detail/temporary_memory_allocator.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace csbind23::cabi
{

template <typename Type> struct Converter;

namespace detail
{

template <typename Type, bool IsEnum = std::is_enum_v<Type>> struct scalar_storage_type
{
    using type = Type;
};

template <typename Type> struct scalar_storage_type<Type, true>
{
    using type = std::underlying_type_t<Type>;
};

template <typename Type, typename Value> decltype(auto) to_c_abi_for(Value&& value)
{
    if constexpr (requires { Converter<Type>::to_c_abi(std::forward<Value>(value)); })
    {
        return Converter<Type>::to_c_abi(std::forward<Value>(value));
    }
    else
    {
        return std::forward<Value>(value);
    }
}

template <typename Type, typename Value> decltype(auto) from_c_abi_for(Value&& value)
{
    if constexpr (requires { Converter<Type>::from_c_abi(std::forward<Value>(value)); })
    {
        return Converter<Type>::from_c_abi(std::forward<Value>(value));
    }
    else
    {
        return std::forward<Value>(value);
    }
}

} // namespace detail

struct StringView
{
    const char* str;
    std::size_t length;
};

template <typename Type> struct Converter
{
    using cpp_type = Type;
    using c_abi_type = Type;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return ""; }
    static constexpr std::string_view managed_marshaller_type_name() { return ""; }
    static constexpr std::string_view managed_pinvoke_attribute() { return ""; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return ""; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return ""; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement() { return ""; }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return ""; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <typename Type>
    requires(std::is_enum_v<Type>)
struct Converter<Type>
{
    using cpp_type = Type;
    using underlying_type = std::underlying_type_t<Type>;
    using c_abi_type = typename Converter<underlying_type>::c_abi_type;

    static constexpr std::string_view c_abi_type_name() { return Converter<underlying_type>::c_abi_type_name(); }
    static constexpr std::string_view pinvoke_type_name() { return Converter<underlying_type>::pinvoke_type_name(); }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return detail::to_c_abi_for<underlying_type>(static_cast<underlying_type>(value));
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return static_cast<cpp_type>(detail::from_c_abi_for<underlying_type>(value));
    }
};

template <> struct Converter<void>
{
    using cpp_type = void;
    using c_abi_type = void;

    static constexpr std::string_view c_abi_type_name() { return "void"; }
    static constexpr std::string_view pinvoke_type_name() { return "void"; }
};

template <> struct Converter<bool>
{
    using cpp_type = bool;
    using c_abi_type = bool;

    static constexpr std::string_view c_abi_type_name() { return "bool"; }
    static constexpr std::string_view pinvoke_type_name() { return "bool"; }
    static constexpr std::string_view managed_pinvoke_attribute()
    {
        return "global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.I1)";
    }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<char>
{
    using cpp_type = char;
    using c_abi_type = char;

    static constexpr std::string_view c_abi_type_name() { return "char"; }
    static constexpr std::string_view pinvoke_type_name() { return "byte"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<char16_t>
{
    using cpp_type = char16_t;
    using c_abi_type = char16_t;

    static constexpr std::string_view c_abi_type_name() { return "char16_t"; }
    static constexpr std::string_view pinvoke_type_name() { return "char"; }
    static constexpr std::string_view managed_pinvoke_attribute()
    {
        return "global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.U2)";
    }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<signed char>
{
    using cpp_type = signed char;
    using c_abi_type = signed char;

    static constexpr std::string_view c_abi_type_name() { return "signed char"; }
    static constexpr std::string_view pinvoke_type_name() { return "sbyte"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<unsigned char>
{
    using cpp_type = unsigned char;
    using c_abi_type = unsigned char;

    static constexpr std::string_view c_abi_type_name() { return "unsigned char"; }
    static constexpr std::string_view pinvoke_type_name() { return "byte"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<short>
{
    using cpp_type = short;
    using c_abi_type = short;

    static constexpr std::string_view c_abi_type_name() { return "short"; }
    static constexpr std::string_view pinvoke_type_name() { return "short"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<unsigned short>
{
    using cpp_type = unsigned short;
    using c_abi_type = unsigned short;

    static constexpr std::string_view c_abi_type_name() { return "unsigned short"; }
    static constexpr std::string_view pinvoke_type_name() { return "ushort"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<int>
{
    using cpp_type = int;
    using c_abi_type = int;

    static constexpr std::string_view c_abi_type_name() { return "int"; }
    static constexpr std::string_view pinvoke_type_name() { return "int"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<unsigned int>
{
    using cpp_type = unsigned int;
    using c_abi_type = unsigned int;

    static constexpr std::string_view c_abi_type_name() { return "unsigned int"; }
    static constexpr std::string_view pinvoke_type_name() { return "uint"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<long>
{
    using cpp_type = long;
    using c_abi_type = long;

    static constexpr std::string_view c_abi_type_name() { return "long"; }
    static constexpr std::string_view pinvoke_type_name() { return "nint"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<unsigned long>
{
    using cpp_type = unsigned long;
    using c_abi_type = unsigned long;

    static constexpr std::string_view c_abi_type_name() { return "unsigned long"; }
    static constexpr std::string_view pinvoke_type_name() { return "nuint"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<long long>
{
    using cpp_type = long long;
    using c_abi_type = long long;

    static constexpr std::string_view c_abi_type_name() { return "long long"; }
    static constexpr std::string_view pinvoke_type_name() { return "long"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<unsigned long long>
{
    using cpp_type = unsigned long long;
    using c_abi_type = unsigned long long;

    static constexpr std::string_view c_abi_type_name() { return "unsigned long long"; }
    static constexpr std::string_view pinvoke_type_name() { return "ulong"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<float>
{
    using cpp_type = float;
    using c_abi_type = float;

    static constexpr std::string_view c_abi_type_name() { return "float"; }
    static constexpr std::string_view pinvoke_type_name() { return "float"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

template <> struct Converter<double>
{
    using cpp_type = double;
    using c_abi_type = double;

    static constexpr std::string_view c_abi_type_name() { return "double"; }
    static constexpr std::string_view pinvoke_type_name() { return "double"; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
};

namespace detail
{
inline std::string replace_array_placeholder(std::string text, std::string_view placeholder, std::string_view value)
{
    std::size_t start = 0;
    while (true)
    {
        const std::size_t pos = text.find(placeholder, start);
        if (pos == std::string::npos)
        {
            break;
        }

        text.replace(pos, placeholder.size(), value);
        start = pos + value.size();
    }
    return text;
}

inline std::string strip_csharp_parameter_modifier(std::string type_name)
{
    for (const std::string_view prefix : {std::string_view{"in "}, std::string_view{"ref "}, std::string_view{"out "}})
    {
        if (type_name.rfind(prefix, 0) == 0)
        {
            return type_name.substr(prefix.size());
        }
    }

    return type_name;
}

template <typename Type> std::string managed_array_element_type_name()
{
    using value_type = std::remove_cv_t<Type>;

    if constexpr (requires { Converter<value_type>::managed_type_name(); })
    {
        const std::string managed_name = std::string(Converter<value_type>::managed_type_name());
        if (!managed_name.empty())
        {
            return managed_name;
        }
    }

    return std::string(Converter<value_type>::pinvoke_type_name());
}

template <typename Type> std::string array_element_pinvoke_type_name()
{
    using value_type = std::remove_cv_t<Type>;
    return strip_csharp_parameter_modifier(std::string(Converter<value_type>::pinvoke_type_name()));
}

template <typename Type> std::string array_element_to_pinvoke_lambda_body()
{
    using value_type = std::remove_cv_t<Type>;
    if constexpr (requires { Converter<value_type>::managed_to_pinvoke_expression(); })
    {
        const std::string expression = std::string(Converter<value_type>::managed_to_pinvoke_expression());
        if (!expression.empty())
        {
            return replace_array_placeholder(expression, "{value}", "element");
        }
    }

    return {};
}

template <typename Type> std::string array_element_from_pinvoke_lambda_body()
{
    using value_type = std::remove_cv_t<Type>;
    if constexpr (requires { Converter<value_type>::managed_from_pinvoke_expression(); })
    {
        const std::string expression = std::string(Converter<value_type>::managed_from_pinvoke_expression());
        if (!expression.empty())
        {
            return replace_array_placeholder(expression, "{value}", "element");
        }
    }

    return {};
}

template <typename Type> bool array_uses_converter_overload()
{
    using value_type = std::remove_cv_t<Type>;
    return !array_element_to_pinvoke_lambda_body<value_type>().empty()
        || !array_element_from_pinvoke_lambda_body<value_type>().empty()
        || managed_array_element_type_name<value_type>() != array_element_pinvoke_type_name<value_type>();
}

template <typename Type> std::string managed_array_type_name()
{
    return managed_array_element_type_name<Type>() + "[]";
}

template <typename Type, std::size_t Extent> std::string array_to_pinvoke_expression()
{
    if (!array_uses_converter_overload<Type>())
    {
        return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.ArrayToNativeFixed<")
            + array_element_pinvoke_type_name<Type>() + ">({value}, " + std::to_string(Extent) + ")";
    }

    return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.ArrayToNativeFixed<")
        + managed_array_element_type_name<Type>() + ", " + array_element_pinvoke_type_name<Type>()
        + ">({value}, " + std::to_string(Extent) + ", static element => "
        + array_element_to_pinvoke_lambda_body<Type>() + ")";
}

template <typename Type, std::size_t Extent> std::string array_from_pinvoke_expression()
{
    if (!array_uses_converter_overload<Type>())
    {
        return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.NativeToNewArrayFixed<")
            + array_element_pinvoke_type_name<Type>() + ">({value}, " + std::to_string(Extent) + ")";
    }

    return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.NativeToNewArrayFixed<")
        + managed_array_element_type_name<Type>() + ", " + array_element_pinvoke_type_name<Type>()
        + ">({value}, " + std::to_string(Extent) + ", static element => "
        + array_element_from_pinvoke_lambda_body<Type>() + ")";
}

template <typename Type, std::size_t Extent> std::string array_finalize_copyback_to_pinvoke_statement()
{
    if (!array_uses_converter_overload<Type>())
    {
        return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.NativeToExistingArrayFixed<")
            + array_element_pinvoke_type_name<Type>() + ">({pinvoke}, {managed}, " + std::to_string(Extent)
                        + ")\n"
                            "global::CsBind23.Generated.CsBind23ArrayInterop.FreeNativeBuffer({pinvoke})";
    }

    return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.NativeToExistingArrayFixed<")
        + managed_array_element_type_name<Type>() + ", " + array_element_pinvoke_type_name<Type>()
        + ">({pinvoke}, {managed}, " + std::to_string(Extent) + ", static element => "
        + array_element_from_pinvoke_lambda_body<Type>()
        + ")\n"
          "global::CsBind23.Generated.CsBind23ArrayInterop.FreeNativeBuffer({pinvoke})";
}

inline constexpr std::string_view array_finalize_free_statement()
{
    return "global::CsBind23.Generated.CsBind23ArrayInterop.FreeNativeBuffer({pinvoke})";
}

template <typename Type>
using array_c_abi_element_type_t =
    std::remove_cv_t<std::remove_reference_t<decltype(to_c_abi_for<std::remove_cv_t<Type>>(std::declval<const std::remove_cv_t<Type>&>()))>>;

} // namespace detail

template <> struct Converter<const char*>
{
    using cpp_type = const char*;
    using c_abi_type = const char*;

    static constexpr std::string_view c_abi_type_name() { return "const char*"; }
    static constexpr std::string_view pinvoke_param_type_name() { return "string"; }
    static constexpr std::string_view pinvoke_return_type_name() { return "string"; }
    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_param_marshaller_type_name()
    {
        return "global::CsBind23.Generated.CsBind23Utf8StringMarshaller";
    }
    static constexpr std::string_view managed_return_marshaller_type_name()
    {
        return "global::CsBind23.Generated.CsBind23Utf8StringReturnMarshaller";
    }

    static c_abi_type to_c_abi(cpp_type value) { return value; }
    static cpp_type from_c_abi(c_abi_type value) { return value; }
};

template <typename Type, std::size_t Extent>
struct Converter<std::array<Type, Extent>>
{
    using value_type = std::remove_cv_t<Type>;
    using cpp_type = std::array<value_type, Extent>;
    using c_abi_element_type = detail::array_c_abi_element_type_t<value_type>;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::managed_array_type_name<value_type>(); }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_to_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_from_pinvoke_expression<value_type, Extent>();
    }

    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        auto* buffer = static_cast<c_abi_element_type*>(
            csbind23::detail::temporary_memory_malloc(sizeof(c_abi_element_type) * Extent));
        if (buffer == nullptr)
        {
            return nullptr;
        }

        for (std::size_t index = 0; index < Extent; ++index)
        {
            buffer[index] = detail::to_c_abi_for<value_type>(value[index]);
        }
        return static_cast<c_abi_type>(buffer);
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        cpp_type managed{};
        if (value == nullptr)
        {
            return managed;
        }

        const auto* buffer = static_cast<const c_abi_element_type*>(value);
        for (std::size_t index = 0; index < Extent; ++index)
        {
            managed[index] = detail::from_c_abi_for<value_type>(buffer[index]);
        }
        return managed;
    }
};

template <typename Type, std::size_t Extent>
struct Converter<std::array<Type, Extent>&>
{
    using value_type = std::remove_cv_t<Type>;
    using array_type = std::array<value_type, Extent>;
    using cpp_type = array_type&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::managed_array_type_name<value_type>(); }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_to_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_from_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::array_finalize_copyback_to_pinvoke_statement<value_type, Extent>();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<array_type*>(value); }
};

template <typename Type, std::size_t Extent>
struct Converter<const std::array<Type, Extent>&>
{
    using value_type = std::remove_cv_t<Type>;
    using array_type = std::array<value_type, Extent>;
    using cpp_type = const array_type&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::managed_array_type_name<value_type>(); }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_to_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_from_pinvoke_expression<value_type, Extent>();
    }

    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const array_type*>(value); }
};

template <typename Type, std::size_t Extent>
struct Converter<std::array<Type, Extent>*>
{
    using value_type = std::remove_cv_t<Type>;
    using array_type = std::array<value_type, Extent>;
    using cpp_type = array_type*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::managed_array_type_name<value_type>(); }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_to_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_from_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::array_finalize_copyback_to_pinvoke_statement<value_type, Extent>();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename Type, std::size_t Extent>
struct Converter<const std::array<Type, Extent>*>
{
    using value_type = std::remove_cv_t<Type>;
    using array_type = std::array<value_type, Extent>;
    using cpp_type = const array_type*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::managed_array_type_name<value_type>(); }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_to_pinvoke_expression<value_type, Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_from_pinvoke_expression<value_type, Extent>();
    }

    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename ReturnType, typename... Args>
struct Converter<ReturnType (*)(Args...)>
{
    using cpp_type = ReturnType (*)(Args...);
    using c_abi_type = cpp_type;

    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static c_abi_type to_c_abi(cpp_type value) { return value; }
    static cpp_type from_c_abi(c_abi_type value) { return value; }
};

template <typename Type>
    requires(std::is_arithmetic_v<std::remove_cv_t<Type>> || std::is_enum_v<std::remove_cv_t<Type>>)
struct Converter<Type*>
{
    using cpp_type = Type*;
    using value_type = std::remove_cv_t<Type>;
    using storage_type = typename detail::scalar_storage_type<value_type>::type;
    using c_abi_type = std::conditional_t<std::is_const_v<Type>, const storage_type*, storage_type*>;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return Converter<value_type>::pinvoke_type_name(); }
    static c_abi_type to_c_abi(cpp_type value)
    {
        return reinterpret_cast<c_abi_type>(value);
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return reinterpret_cast<cpp_type>(const_cast<storage_type*>(value));
    }
};

template <typename Type>
    requires(std::is_arithmetic_v<std::remove_cv_t<Type>> || std::is_enum_v<std::remove_cv_t<Type>>)
struct Converter<Type&>
{
    using cpp_type = Type&;
    using value_type = std::remove_cv_t<Type>;
    using storage_type = typename detail::scalar_storage_type<value_type>::type;
    using c_abi_type = std::conditional_t<std::is_const_v<Type>, const storage_type*, storage_type*>;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return Converter<value_type>::pinvoke_type_name(); }
    static c_abi_type to_c_abi(cpp_type value) { return reinterpret_cast<c_abi_type>(&value); }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return *reinterpret_cast<Type*>(const_cast<storage_type*>(value));
    }
};

template <typename Type>
    requires(std::is_arithmetic_v<std::remove_cv_t<Type>> || std::is_enum_v<std::remove_cv_t<Type>>)
struct Converter<const Type&>
{
    using cpp_type = const Type&;
    using value_type = std::remove_cv_t<Type>;
    using storage_type = typename detail::scalar_storage_type<value_type>::type;
    using c_abi_type = const storage_type*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return Converter<value_type>::pinvoke_type_name(); }
    static c_abi_type to_c_abi(cpp_type value) { return reinterpret_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *reinterpret_cast<const value_type*>(value); }
};

template <typename Type> struct Converter<Type*>
{
    using cpp_type = Type*;
    using c_abi_type = std::conditional_t<std::is_const_v<Type>, const void*, void*>;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return static_cast<cpp_type>(
            const_cast<std::remove_const_t<Type>*>(static_cast<const std::remove_const_t<Type>*>(value)));
    }
};

template <typename Type> struct Converter<Type&>
{
    using cpp_type = Type&;
    using c_abi_type = std::conditional_t<std::is_const_v<Type>, const void*, void*>;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return *static_cast<Type*>(
            const_cast<std::remove_const_t<Type>*>(static_cast<const std::remove_const_t<Type>*>(value)));
    }
};

template <typename Type> struct Converter<const Type&>
{
    using cpp_type = const Type&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const Type*>(value); }
};

} // namespace csbind23::cabi
