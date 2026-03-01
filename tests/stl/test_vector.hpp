#pragma once

#include "csbind23/bindings_generator.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace csbind23::testing::vector_tests
{

class VectorItem
{
public:
    explicit VectorItem(int value)
        : value_(value)
    {
    }

    int get() const
    {
        return value_;
    }

    void set(int value)
    {
        value_ = value;
    }

    bool operator==(const VectorItem& other) const
    {
        return value_ == other.value_;
    }

private:
    int value_;
};

inline void add_int(std::vector<int>& value, int item)
{
    value.push_back(item);
}

inline void add_double(std::vector<double>& value, double item)
{
    value.push_back(item);
}

inline void add_item(std::vector<VectorItem>& value, const VectorItem& item)
{
    value.push_back(item);
}

inline bool contains_int(const std::vector<int>& value, int item)
{
    for (const auto& existing : value)
    {
        if (existing == item)
        {
            return true;
        }
    }
    return false;
}

inline bool contains_double(const std::vector<double>& value, double item)
{
    for (const auto& existing : value)
    {
        if (existing == item)
        {
            return true;
        }
    }
    return false;
}

inline bool contains_item(const std::vector<VectorItem>& value, const VectorItem& item)
{
    for (const auto& existing : value)
    {
        if (existing == item)
        {
            return true;
        }
    }
    return false;
}

inline int index_of_int(const std::vector<int>& value, int item)
{
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == item)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

inline int index_of_double(const std::vector<double>& value, double item)
{
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == item)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

inline int index_of_item(const std::vector<VectorItem>& value, const VectorItem& item)
{
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == item)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

inline int get_int(const std::vector<int>& value, int index)
{
    return value.at(static_cast<std::size_t>(index));
}

inline double get_double(const std::vector<double>& value, int index)
{
    return value.at(static_cast<std::size_t>(index));
}

inline const VectorItem& get_item(const std::vector<VectorItem>& value, int index)
{
    return value.at(static_cast<std::size_t>(index));
}

inline void set_int(std::vector<int>& value, int index, int item)
{
    value.at(static_cast<std::size_t>(index)) = item;
}

inline void set_double(std::vector<double>& value, int index, double item)
{
    value.at(static_cast<std::size_t>(index)) = item;
}

inline void set_item(std::vector<VectorItem>& value, int index, const VectorItem& item)
{
    value.at(static_cast<std::size_t>(index)) = item;
}

inline void insert_int(std::vector<int>& value, int index, int item)
{
    value.insert(value.begin() + static_cast<std::ptrdiff_t>(index), item);
}

inline void insert_double(std::vector<double>& value, int index, double item)
{
    value.insert(value.begin() + static_cast<std::ptrdiff_t>(index), item);
}

inline void insert_item(std::vector<VectorItem>& value, int index, const VectorItem& item)
{
    value.insert(value.begin() + static_cast<std::ptrdiff_t>(index), item);
}

template <typename T> inline void erase_at(std::vector<T>& value, int index)
{
    value.erase(value.begin() + static_cast<std::ptrdiff_t>(index));
}

inline bool remove_first_int(std::vector<int>& value, int item)
{
    const int index = index_of_int(value, item);
    if (index < 0)
    {
        return false;
    }

    erase_at(value, index);
    return true;
}

inline bool remove_first_double(std::vector<double>& value, double item)
{
    const int index = index_of_double(value, item);
    if (index < 0)
    {
        return false;
    }

    erase_at(value, index);
    return true;
}

inline bool remove_first_item(std::vector<VectorItem>& value, const VectorItem& item)
{
    const int index = index_of_item(value, item);
    if (index < 0)
    {
        return false;
    }

    erase_at(value, index);
    return true;
}

} // namespace csbind23::testing::vector_tests

using VectorItem = csbind23::testing::vector_tests::VectorItem;

namespace csbind23::cabi
{

template <> struct Converter<csbind23::testing::vector_tests::VectorItem>
{
    using cpp_type = csbind23::testing::vector_tests::VectorItem;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Generated.VectorItem"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "global::CsBind23.Generated.VectorItem.BorrowHandle({value})";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "(global::CsBind23.Generated.VectorItem)global::CsBind23.Generated.vectorRuntime.WrapPolymorphic_VectorItem({value}, false)";
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return static_cast<c_abi_type>(&value);
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return *static_cast<const cpp_type*>(value);
    }
};

template <> struct Converter<const csbind23::testing::vector_tests::VectorItem&>
{
    using cpp_type = const csbind23::testing::vector_tests::VectorItem&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return "global::CsBind23.Generated.VectorItem"; }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "global::CsBind23.Generated.VectorItem.BorrowHandle({value})";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "(global::CsBind23.Generated.VectorItem)global::CsBind23.Generated.vectorRuntime.WrapPolymorphic_VectorItem({value}, false)";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const csbind23::testing::vector_tests::VectorItem*>(value); }
};

} // namespace csbind23::cabi

namespace csbind23::testing
{

inline void register_bindings_vector(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("VectorApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/stl/test_vector.hpp\"");

    module.class_<vector_tests::VectorItem>()
        .ctor<int>()
        .def<&vector_tests::VectorItem::get>()
        .def<&vector_tests::VectorItem::set>()
        .csharp_code(R"(    internal static System.IntPtr BorrowHandle(VectorItem value)
    {
        return value?._handle ?? System.IntPtr.Zero;
    }
)");

    module.class_generic<std::vector<int>, std::vector<double>, std::vector<vector_tests::VectorItem>>("VectorList")
        .ctor<>()
        .csharp_interface("System.Collections.Generic.IList<T>")
        .csharp_interface("System.Collections.Generic.IReadOnlyList<T>")
        .def<&std::vector<int>::size, &std::vector<double>::size, &std::vector<vector_tests::VectorItem>::size>("size", csbind23::Private{})
        .def<&std::vector<int>::clear, &std::vector<double>::clear, &std::vector<vector_tests::VectorItem>::clear>("clear", csbind23::Private{})
        .def<&vector_tests::add_int, &vector_tests::add_double, &vector_tests::add_item>(
            "__add",
            csbind23::Private{})
        .def<&vector_tests::contains_int, &vector_tests::contains_double, &vector_tests::contains_item>(
            "__contains",
            csbind23::Private{})
        .def<&vector_tests::index_of_int, &vector_tests::index_of_double, &vector_tests::index_of_item>(
            "__index_of",
            csbind23::Private{})
        .def<&vector_tests::get_int, &vector_tests::get_double, &vector_tests::get_item>(
            "__get",
            csbind23::Private{})
        .def<&vector_tests::set_int, &vector_tests::set_double, &vector_tests::set_item>(
            "__set",
            csbind23::Private{})
        .def<&vector_tests::insert_int, &vector_tests::insert_double, &vector_tests::insert_item>(
            "__insert",
            csbind23::Private{})
        .def<&vector_tests::erase_at<int>, &vector_tests::erase_at<double>, &vector_tests::erase_at<vector_tests::VectorItem>>(
            "__erase_at",
            csbind23::Private{})
        .def<&vector_tests::remove_first_int,
            &vector_tests::remove_first_double,
            &vector_tests::remove_first_item>(
            "__remove_first",
            csbind23::Private{})
        .csharp_code(R"(    public int Count => checked((int)size());

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

    internal static VectorList<T> FromOwnedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new VectorList<T>();
        }

        return new VectorList<T>(handle, true);
    }

    internal static VectorList<T> FromBorrowedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new VectorList<T>();
        }

        return new VectorList<T>(handle, false);
    }

    internal static System.IntPtr BorrowHandle(VectorList<T> value)
    {
        return value?._handle ?? System.IntPtr.Zero;
    }

    internal static void DestroyOwnedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        using var owner = new VectorList<T>(handle, true);
    }

    internal static System.IntPtr CloneOrCreateOwned(VectorList<T> value)
    {
        var clone = new VectorList<T>();
        if (value != null)
        {
            foreach (var item in value)
            {
                clone.Add(item);
            }
        }

        var handle = clone._handle;
        clone._handle = System.IntPtr.Zero;
        clone._ownsHandle = false;
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
    }
)");

}

} // namespace csbind23::testing
