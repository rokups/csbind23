#pragma once

#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

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

template <typename ElementType> using bare_element_type_t = std::remove_cv_t<std::remove_reference_t<ElementType>>;

template <typename ElementType> std::string vector_element_managed_type_name()
{
    using BareElementType = bare_element_type_t<ElementType>;

    if constexpr (requires { Converter<BareElementType>::managed_type_name(); })
    {
        const std::string managed_name = std::string(Converter<BareElementType>::managed_type_name());
        if (!managed_name.empty())
        {
            return managed_name;
        }
    }

    return std::string(Converter<BareElementType>::pinvoke_type_name());
}

template <typename ElementType> std::string vector_managed_type_name()
{
    return "Std.Vector<" + vector_element_managed_type_name<ElementType>() + ">";
}

template <typename ElementType> std::string vector_item_ownership_expression()
{
    using BareElementType = bare_element_type_t<ElementType>;
    if constexpr (std::is_pointer_v<BareElementType>)
    {
        return "ItemOwnership.Borrowed";
    }

    return "ItemOwnership.Owned";
}

template <typename ElementType> std::string vector_managed_clone_expression()
{
    return vector_managed_type_name<ElementType>() + ".CloneOrCreateOwned({value})";
}

template <typename ElementType> std::string vector_managed_owned_expression()
{
    return vector_managed_type_name<ElementType>() + ".FromOwnedHandle({value}, "
        + vector_item_ownership_expression<ElementType>() + ")";
}

template <typename ElementType> std::string vector_managed_borrowed_expression()
{
    return vector_managed_type_name<ElementType>() + ".FromBorrowedHandle({value}, "
        + vector_item_ownership_expression<ElementType>() + ")";
}

template <typename ElementType> std::string vector_managed_borrow_expression()
{
    return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
}

template <typename ElementType> std::string vector_managed_destroy_expression()
{
    return vector_managed_type_name<ElementType>()
        + ".DestroyOwnedHandle({pinvoke}, ({managed}?._itemOwnership ?? ItemOwnership.Owned))";
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
        return Converter<underlying_type>::to_c_abi(static_cast<underlying_type>(value));
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return static_cast<cpp_type>(Converter<underlying_type>::from_c_abi(value));
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

template <std::size_t Extent> std::string array_int_to_pinvoke_expression()
{
    return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.IntArrayToNativeFixed({value}, ")
        + std::to_string(Extent) + ")";
}

template <std::size_t Extent> std::string array_int_from_pinvoke_expression()
{
    return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.NativeToNewIntArrayFixed({value}, ")
        + std::to_string(Extent) + ")";
}

template <std::size_t Extent> std::string array_int_finalize_copyback_to_pinvoke_statement()
{
    return std::string("global::CsBind23.Generated.CsBind23ArrayInterop.NativeToExistingIntArrayFixed({pinvoke}, {managed}, ")
        + std::to_string(Extent)
        + ")\n"
          "global::CsBind23.Generated.CsBind23ArrayInterop.FreeNativeBuffer({pinvoke})";
}

inline constexpr std::string_view array_int_finalize_free_statement()
{
    return "global::CsBind23.Generated.CsBind23ArrayInterop.FreeNativeBuffer({pinvoke})";
}

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

template <std::size_t Extent> struct Converter<std::array<int, Extent>>
{
    using cpp_type = std::array<int, Extent>;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view c_abi_return_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view pinvoke_param_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view pinvoke_return_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "int[]"; }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_int_to_pinvoke_expression<Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_int_from_pinvoke_expression<Extent>();
    }

    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        auto* buffer = static_cast<int*>(std::malloc(sizeof(int) * Extent));
        if (buffer == nullptr)
        {
            return nullptr;
        }
        std::memcpy(buffer, value.data(), sizeof(int) * Extent);
        return static_cast<c_abi_type>(buffer);
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        cpp_type managed{};
        if (value == nullptr)
        {
            return managed;
        }

        std::memcpy(managed.data(), static_cast<const int*>(value), sizeof(int) * Extent);
        return managed;
    }
};

template <std::size_t Extent> struct Converter<std::array<int, Extent>&>
{
    using cpp_type = std::array<int, Extent>&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "int[]"; }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_int_to_pinvoke_expression<Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_int_from_pinvoke_expression<Extent>();
    }

    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::array_int_finalize_copyback_to_pinvoke_statement<Extent>();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<std::array<int, Extent>*>(value); }
};

template <std::size_t Extent> struct Converter<const std::array<int, Extent>&>
{
    using cpp_type = const std::array<int, Extent>&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "int[]"; }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_int_to_pinvoke_expression<Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_int_from_pinvoke_expression<Extent>();
    }

    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const std::array<int, Extent>*>(value); }
};

template <std::size_t Extent> struct Converter<std::array<int, Extent>*>
{
    using cpp_type = std::array<int, Extent>*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "int[]"; }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_int_to_pinvoke_expression<Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_int_from_pinvoke_expression<Extent>();
    }

    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::array_int_finalize_copyback_to_pinvoke_statement<Extent>();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <std::size_t Extent> struct Converter<const std::array<int, Extent>*>
{
    using cpp_type = const std::array<int, Extent>*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "int[]"; }

    static std::string managed_to_pinvoke_expression()
    {
        return detail::array_int_to_pinvoke_expression<Extent>();
    }

    static std::string managed_from_pinvoke_expression()
    {
        return detail::array_int_from_pinvoke_expression<Extent>();
    }

    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return detail::array_int_finalize_free_statement();
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename ElementType> struct Converter<std::vector<ElementType>>
{
    using cpp_type = std::vector<ElementType>;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::vector_managed_type_name<ElementType>(); }
    static std::string managed_to_pinvoke_expression() { return detail::vector_managed_clone_expression<ElementType>(); }
    static std::string managed_from_pinvoke_expression() { return detail::vector_managed_owned_expression<ElementType>(); }
    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::vector_managed_destroy_expression<ElementType>();
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return static_cast<c_abi_type>(new cpp_type(value));
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        if (value == nullptr)
        {
            return {};
        }
        return *static_cast<const cpp_type*>(value);
    }
};

template <typename ElementType> struct Converter<std::vector<ElementType>&>
{
    using cpp_type = std::vector<ElementType>&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::vector_managed_type_name<ElementType>(); }
    static std::string managed_to_pinvoke_expression() { return detail::vector_managed_borrow_expression<ElementType>(); }
    static std::string managed_from_pinvoke_expression() { return detail::vector_managed_borrowed_expression<ElementType>(); }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<std::vector<ElementType>*>(value); }
};

template <typename ElementType> struct Converter<const std::vector<ElementType>&>
{
    using cpp_type = const std::vector<ElementType>&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::vector_managed_type_name<ElementType>(); }
    static std::string managed_to_pinvoke_expression() { return detail::vector_managed_borrow_expression<ElementType>(); }
    static std::string managed_from_pinvoke_expression() { return detail::vector_managed_borrowed_expression<ElementType>(); }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const std::vector<ElementType>*>(value); }
};

template <typename ElementType> struct Converter<std::vector<ElementType>*>
{
    using cpp_type = std::vector<ElementType>*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::vector_managed_type_name<ElementType>(); }
    static std::string managed_to_pinvoke_expression() { return detail::vector_managed_borrow_expression<ElementType>(); }
    static std::string managed_from_pinvoke_expression() { return detail::vector_managed_borrowed_expression<ElementType>(); }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename ElementType> struct Converter<const std::vector<ElementType>*>
{
    using cpp_type = const std::vector<ElementType>*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::vector_managed_type_name<ElementType>(); }
    static std::string managed_to_pinvoke_expression() { return detail::vector_managed_borrow_expression<ElementType>(); }
    static std::string managed_from_pinvoke_expression() { return detail::vector_managed_borrowed_expression<ElementType>(); }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
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
