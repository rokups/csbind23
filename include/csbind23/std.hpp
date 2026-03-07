#pragma once

#include "csbind23/bindings_generator.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

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

namespace csbind23::detail
{
inline std::string replace_all(std::string value, std::string_view from, std::string_view to)
{
    std::size_t start = 0;
    while (true)
    {
        const std::size_t index = value.find(from, start);
        if (index == std::string::npos)
        {
            break;
        }

        value.replace(index, from.size(), to);
        start = index + to.size();
    }

    return value;
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

namespace csbind23
{

template <typename... Types>
GenericClassBuilder<std::vector<Types>...> add_vector(ModuleBuilder& module)
{
    module.cabi_include("<csbind23/std.hpp>");

    auto builder = module.class_generic<std::vector, Types...>("Vector");
    builder.template ctor<>();
    builder.csharp_namespace("Std");
    builder.csharp_interface("System.Collections.Generic.IList<T>");
    builder.csharp_interface("System.Collections.Generic.IReadOnlyList<T>");
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::size; }>("size", Private{});
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::clear; }>("clear", Private{});
    builder.template def<&::csbind23_vector_add<Types>...>("__add", Private{});
    builder.template def<&::csbind23_vector_contains<Types>...>("__contains", Private{});
    builder.template def<&::csbind23_vector_index_of<Types>...>("__index_of", Private{});
    builder.template def<&::csbind23_vector_get<Types>...>("__get", Private{});
    builder.template def<&::csbind23_vector_set<Types>...>("__set", Private{});
    builder.template def<&::csbind23_vector_insert<Types>...>("__insert", Private{});
    builder.template def<&::csbind23_vector_erase_at<Types>...>("__erase_at", Private{});
    builder.template def<&::csbind23_vector_remove_first<Types>...>("__remove_first", Private{});
    builder.csharp_code(detail::make_vector_wrapper_csharp_code("Vector"));
    return builder;
}

} // namespace csbind23