#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std/detail.hpp"
#include "csbind23/std/string.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace csbind23
{

template <typename T>
void csbind23_vector_add(std::vector<T>& value, T item)
{
    value.push_back(std::move(item));
}

template <typename T>
bool csbind23_vector_contains(const std::vector<T>& value, T item)
{
    return std::find(value.begin(), value.end(), item) != value.end();
}

template <typename T>
int csbind23_vector_index_of(const std::vector<T>& value, T item)
{
    const auto it = std::find(value.begin(), value.end(), item);
    if (it == value.end())
    {
        return -1;
    }

    return static_cast<int>(std::distance(value.begin(), it));
}

template <typename T>
decltype(auto) csbind23_vector_get(const std::vector<T>& value, int index)
{
    if constexpr (std::is_arithmetic_v<std::remove_cv_t<T>> || std::is_enum_v<std::remove_cv_t<T>>
        || std::is_pointer_v<T>)
    {
        return static_cast<T>(value.at(static_cast<std::size_t>(index)));
    }
    else
    {
        return static_cast<const T&>(value.at(static_cast<std::size_t>(index)));
    }
}

template <typename T>
void csbind23_vector_set(std::vector<T>& value, int index, T item)
{
    value.at(static_cast<std::size_t>(index)) = std::move(item);
}

template <typename T>
void csbind23_vector_insert(std::vector<T>& value, int index, T item)
{
    value.insert(value.begin() + static_cast<std::ptrdiff_t>(index), std::move(item));
}

template <typename T>
void csbind23_vector_erase_at(std::vector<T>& value, int index)
{
    value.erase(value.begin() + static_cast<std::ptrdiff_t>(index));
}

template <typename T>
bool csbind23_vector_remove_first(std::vector<T>& value, T item)
{
    const auto it = std::find(value.begin(), value.end(), item);
    if (it == value.end())
    {
        return false;
    }

    value.erase(it);
    return true;
}

} // namespace csbind23

namespace csbind23::detail
{
template <typename... Types>
CppSymbols make_vector_helper_cpp_symbols(std::string_view helper_name)
{
    CppSymbols symbols;
    (symbols.values.push_back(
         "csbind23::" + std::string(helper_name) + "<" + fully_qualified_cpp_type_name<Types>() + ">"),
        ...);
    return symbols;
}

inline std::string make_vector_wrapper_csharp_code(std::string_view wrapper_name)
{
    std::string code = R"(    public int Count => checked((int)size());

    public bool IsReadOnly => false;

    public T this[int index]
    {
        get
        {
            EnsureIndex(index, allowCount: false);
            return __get(index);
        }
        set
        {
            EnsureIndex(index, allowCount: false);
            __set(index, value);
        }
    }

    public void Add(T item)
    {
        __add(item);
    }

    public void Clear()
    {
        clear();
    }

    public bool Contains(T item)
    {
        return __contains(item);
    }

    public void CopyTo(T[] array, int arrayIndex)
    {
        System.ArgumentNullException.ThrowIfNull(array);
        if (arrayIndex < 0)
        {
            throw new System.ArgumentOutOfRangeException(nameof(arrayIndex));
        }
        if (array.Length - arrayIndex < Count)
        {
            throw new System.ArgumentException("Destination array is too small.", nameof(array));
        }

        for (int i = 0; i < Count; i++)
        {
            array[arrayIndex + i] = __get(i);
        }
    }

    public System.Collections.Generic.IEnumerator<T> GetEnumerator()
    {
        for (int i = 0; i < Count; i++)
        {
            yield return __get(i);
        }
    }

    public int IndexOf(T item)
    {
        return __index_of(item);
    }

    public void Insert(int index, T item)
    {
        EnsureIndex(index, allowCount: true);
        __insert(index, item);
    }

    public bool Remove(T item)
    {
        return __remove_first(item);
    }

    public void RemoveAt(int index)
    {
        EnsureIndex(index, allowCount: false);
        __erase_at(index);
    }

    System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }

    internal static __CSBIND23_VECTOR_WRAPPER__<T> FromOwnedHandle(System.IntPtr handle, __CSBIND23_ITEM_OWNERSHIP__ itemOwnership = __CSBIND23_ITEM_OWNERSHIP__.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new __CSBIND23_VECTOR_WRAPPER__<T>(itemOwnership);
        }

        return new __CSBIND23_VECTOR_WRAPPER__<T>(handle, __CSBIND23_ITEM_OWNERSHIP__.Owned, itemOwnership);
    }

    internal static __CSBIND23_VECTOR_WRAPPER__<T> FromBorrowedHandle(System.IntPtr handle, __CSBIND23_ITEM_OWNERSHIP__ itemOwnership = __CSBIND23_ITEM_OWNERSHIP__.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new __CSBIND23_VECTOR_WRAPPER__<T>(itemOwnership);
        }

        return new __CSBIND23_VECTOR_WRAPPER__<T>(handle, __CSBIND23_ITEM_OWNERSHIP__.Borrowed, itemOwnership);
    }

    internal static void DestroyOwnedHandle(System.IntPtr handle, __CSBIND23_ITEM_OWNERSHIP__ itemOwnership = __CSBIND23_ITEM_OWNERSHIP__.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        using var owner = new __CSBIND23_VECTOR_WRAPPER__<T>(handle, __CSBIND23_ITEM_OWNERSHIP__.Owned, itemOwnership);
    }

    internal static System.IntPtr CloneOrCreateOwned(__CSBIND23_VECTOR_WRAPPER__<T> value)
    {
        var clone = new __CSBIND23_VECTOR_WRAPPER__<T>(value?._itemOwnership ?? __CSBIND23_ITEM_OWNERSHIP__.Owned);
        if (value != null)
        {
            foreach (var item in value)
            {
                clone.Add(item);
            }
        }

        var handle = clone._cPtr.Handle;
        clone._cPtr = new System.Runtime.InteropServices.HandleRef(clone, System.IntPtr.Zero);
        clone._ownership = __CSBIND23_ITEM_OWNERSHIP__.Borrowed;
        System.GC.SuppressFinalize(clone);
        return handle;
    }

    private void EnsureIndex(int index, bool allowCount)
    {
        int upperBound = allowCount ? Count : Count - 1;
        if (index < 0 || index > upperBound)
        {
            throw new System.ArgumentOutOfRangeException(nameof(index));
        }
    })";

    return replace_all(std::move(code), "__CSBIND23_VECTOR_WRAPPER__", wrapper_name);
}

} // namespace csbind23::detail

namespace csbind23::cabi::detail
{

template <typename ElementType> using vector_bare_element_type_t = std::remove_cv_t<std::remove_reference_t<ElementType>>;

template <typename ElementType> std::string vector_element_managed_type_name()
{
    using BareElementType = vector_bare_element_type_t<ElementType>;

    if constexpr (std::is_same_v<BareElementType, std::string>)
    {
        return std::string(std_string_wrapper_managed_type_name());
    }

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
    return "global::CsBind23.Generated.Std.Vector<" + vector_element_managed_type_name<ElementType>() + ">";
}

template <typename ElementType> std::string vector_item_ownership_expression()
{
    using BareElementType = vector_bare_element_type_t<ElementType>;
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

} // namespace csbind23::cabi::detail

namespace csbind23::cabi
{

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

} // namespace csbind23::cabi

namespace csbind23
{

template <typename... Types>
GenericClassBuilder<std::vector<Types>...> add_vector(BindingsGenerator& generator)
{
    auto module = detail::ensure_stl_module(generator);

    auto builder = module.class_generic<std::vector, Types...>("Vector");
    builder.template ctor<>();
    builder.csharp_namespace("Std");
    builder.csharp_interface("System.Collections.Generic.IList<T>");
    builder.csharp_interface("System.Collections.Generic.IReadOnlyList<T>");
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::size; }>("size", Private{});
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::clear; }>("clear", Private{});
    builder.template def<&csbind23::csbind23_vector_add<Types>...>(
        "__add", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_add"));
    builder.template def<&csbind23::csbind23_vector_contains<Types>...>(
        "__contains", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_contains"));
    builder.template def<&csbind23::csbind23_vector_index_of<Types>...>(
        "__index_of", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_index_of"));
    builder.template def<&csbind23::csbind23_vector_get<Types>...>(
        "__get", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_get"));
    builder.template def<&csbind23::csbind23_vector_set<Types>...>(
        "__set", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_set"));
    builder.template def<&csbind23::csbind23_vector_insert<Types>...>(
        "__insert", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_insert"));
    builder.template def<&csbind23::csbind23_vector_erase_at<Types>...>(
        "__erase_at", Private{}, detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_erase_at"));
    builder.template def<&csbind23::csbind23_vector_remove_first<Types>...>("__remove_first", Private{},
        detail::make_vector_helper_cpp_symbols<Types...>("csbind23_vector_remove_first"));
    builder.csharp_code(detail::make_vector_wrapper_csharp_code("Vector"));
    return builder;
}

} // namespace csbind23
