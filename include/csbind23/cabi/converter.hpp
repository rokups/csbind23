#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace csbind23::cabi
{

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
    static constexpr std::string_view managed_to_pinvoke_expression() { return ""; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return ""; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement() { return ""; }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return ""; }
    static c_abi_type to_c_abi(const cpp_type& value) { return value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return value; }
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

struct ConstStringManagedInterop
{
    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_create({value} ?? string.Empty)";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "({value} == 0 ? string.Empty : global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_read({value}))";
    }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_destroy({pinvoke})";
    }
};

struct MutableStringManagedInterop
{
    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_create({value} ?? string.Empty)";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "({value} == 0 ? string.Empty : global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_read({value}))";
    }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "{managed} = ({pinvoke} == 0 ? string.Empty : global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_read({pinvoke}));\n"
               "global::CsBind23.Generated.CsBind23CoreApi.csbind23_string_destroy({pinvoke})";
    }
};

} // namespace detail

template <> struct Converter<std::string>
{
    using cpp_type = std::string;
    using c_abi_type = const char*;

    static constexpr std::string_view c_abi_type_name() { return "const char*"; }
    static constexpr std::string_view pinvoke_type_name() { return "string"; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        auto* buffer = static_cast<char*>(std::malloc(value.size() + 1));
        if (buffer == nullptr)
        {
            return nullptr;
        }
        std::memcpy(buffer, value.c_str(), value.size() + 1);
        return buffer;
    }

    static cpp_type from_c_abi(c_abi_type value) { return value == nullptr ? std::string{} : std::string{value}; }
};

template <> struct Converter<const char*>
{
    using cpp_type = const char*;
    using c_abi_type = const char*;

    static constexpr std::string_view c_abi_type_name() { return "const char*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi({value} ?? string.Empty)";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "System.Runtime.InteropServices.Marshal.PtrToStringAnsi({value}) ?? string.Empty";
    }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "System.Runtime.InteropServices.Marshal.FreeHGlobal({pinvoke})";
    }

    static c_abi_type to_c_abi(cpp_type value) { return value; }
    static cpp_type from_c_abi(c_abi_type value) { return value; }
};

template <> struct Converter<std::string_view>
{
    using cpp_type = std::string_view;
    using c_abi_type = StringView;

    static constexpr std::string_view c_abi_type_name() { return "csbind23::cabi::StringView"; }
    static constexpr std::string_view c_abi_param_type_name() { return "const char*"; }
    static constexpr std::string_view c_abi_return_type_name() { return "csbind23::cabi::StringView"; }
    static constexpr std::string_view pinvoke_type_name() { return "CsBind23StringView"; }
    static constexpr std::string_view pinvoke_param_type_name() { return "string"; }
    static constexpr std::string_view pinvoke_return_type_name() { return "CsBind23StringView"; }
    static constexpr std::string_view managed_type_name() { return "string"; }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "global::CsBind23.Generated.CsBind23StringViewExtensions.ToManaged({value})";
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{.str = value.data(), .length = value.size()};
    }

    static cpp_type from_c_abi(const char* value)
    {
        return value == nullptr ? std::string_view{} : std::string_view{value};
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        if (value.str == nullptr)
        {
            return std::string_view{};
        }
        return std::string_view{value.str, value.length};
    }
};

template <> struct Converter<std::string&> : detail::MutableStringManagedInterop
{
    using cpp_type = std::string&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<std::string*>(value); }
};

template <> struct Converter<const std::string&> : detail::ConstStringManagedInterop
{
    using cpp_type = const std::string&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const std::string*>(value); }
};

template <> struct Converter<std::string*> : detail::MutableStringManagedInterop
{
    using cpp_type = std::string*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <> struct Converter<const std::string*> : detail::ConstStringManagedInterop
{
    using cpp_type = const std::string*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
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
