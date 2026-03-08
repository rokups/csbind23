#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std/detail.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

namespace csbind23
{

inline const char* csbind23_string_snapshot_utf8(const std::string& value)
{
    return value.c_str();
}

inline std::string_view csbind23_string_snapshot_view(const std::string& value)
{
    return value;
}

inline void csbind23_string_assign_utf8(std::string& value, const char* text)
{
    value = text == nullptr ? std::string{} : std::string{text};
}

inline void csbind23_string_clear(std::string& value)
{
    value.clear();
}

inline void csbind23_string_append_utf8(std::string& value, const char* suffix)
{
    value.append(suffix == nullptr ? "" : suffix);
}

inline void csbind23_string_append_string(std::string& value, const std::string& suffix)
{
    value.append(suffix);
}

inline std::size_t csbind23_utf8_sequence_length(unsigned char lead)
{
    if ((lead & 0x80u) == 0)
    {
        return 1;
    }
    if ((lead & 0xE0u) == 0xC0u)
    {
        return 2;
    }
    if ((lead & 0xF0u) == 0xE0u)
    {
        return 3;
    }
    return 4;
}

inline char32_t csbind23_utf8_decode_at(std::string_view value, std::size_t index)
{
    const unsigned char lead = static_cast<unsigned char>(value[index]);
    if ((lead & 0x80u) == 0)
    {
        return lead;
    }
    if ((lead & 0xE0u) == 0xC0u)
    {
        return ((lead & 0x1Fu) << 6) | (static_cast<unsigned char>(value[index + 1]) & 0x3Fu);
    }
    if ((lead & 0xF0u) == 0xE0u)
    {
        return ((lead & 0x0Fu) << 12) | ((static_cast<unsigned char>(value[index + 1]) & 0x3Fu) << 6)
            | (static_cast<unsigned char>(value[index + 2]) & 0x3Fu);
    }

    return ((lead & 0x07u) << 18) | ((static_cast<unsigned char>(value[index + 1]) & 0x3Fu) << 12)
        | ((static_cast<unsigned char>(value[index + 2]) & 0x3Fu) << 6)
        | (static_cast<unsigned char>(value[index + 3]) & 0x3Fu);
}

inline int csbind23_string_utf8_length(const std::string& value)
{
    int count = 0;
    for (std::size_t index = 0; index < value.size();)
    {
        index += csbind23_utf8_sequence_length(static_cast<unsigned char>(value[index]));
        ++count;
    }

    return count;
}

inline unsigned short csbind23_string_char_at_utf8(const std::string& value, int index)
{
    if (index < 0)
    {
        throw std::out_of_range("string index out of range");
    }

    int current = 0;
    for (std::size_t byte_index = 0; byte_index < value.size();)
    {
        const char32_t codepoint = csbind23_utf8_decode_at(value, byte_index);
        if (current == index)
        {
            return codepoint <= 0xFFFFu ? static_cast<unsigned short>(codepoint) : static_cast<unsigned short>(0xFFFD);
        }

        byte_index += csbind23_utf8_sequence_length(static_cast<unsigned char>(value[byte_index]));
        ++current;
    }

    throw std::out_of_range("string index out of range");
}

inline bool csbind23_string_contains_utf8(const std::string& value, const char* other)
{
    return value.find(other == nullptr ? "" : other) != std::string::npos;
}

inline bool csbind23_string_starts_with_utf8(const std::string& value, const char* other)
{
    return value.starts_with(other == nullptr ? "" : other);
}

inline bool csbind23_string_ends_with_utf8(const std::string& value, const char* other)
{
    return value.ends_with(other == nullptr ? "" : other);
}

inline int csbind23_string_index_of_utf8(const std::string& value, const char* other)
{
    const std::size_t index = value.find(other == nullptr ? "" : other);
    return index == std::string::npos ? -1 : static_cast<int>(index);
}

inline int csbind23_string_last_index_of_utf8(const std::string& value, const char* other)
{
    const std::size_t index = value.rfind(other == nullptr ? "" : other);
    return index == std::string::npos ? -1 : static_cast<int>(index);
}

inline std::size_t csbind23_string_checked_index(int index, std::size_t size)
{
    if (index < 0 || static_cast<std::size_t>(index) > size)
    {
        throw std::out_of_range("string index out of range");
    }

    return static_cast<std::size_t>(index);
}

inline std::string csbind23_string_substring(const std::string& value, int start_index)
{
    const std::size_t start = csbind23_string_checked_index(start_index, value.size());
    return value.substr(start);
}

inline std::string csbind23_string_substring_len(const std::string& value, int start_index, int length)
{
    if (length < 0)
    {
        throw std::out_of_range("string length out of range");
    }

    const std::size_t start = csbind23_string_checked_index(start_index, value.size());
    const std::size_t count = static_cast<std::size_t>(length);
    if (count > value.size() - start)
    {
        throw std::out_of_range("string length out of range");
    }

    return value.substr(start, count);
}

inline std::string csbind23_string_replace_utf8(
    const std::string& value, const char* old_value, const char* new_value)
{
    const std::string_view old_text = old_value == nullptr ? std::string_view{} : std::string_view{old_value};
    const std::string_view new_text = new_value == nullptr ? std::string_view{} : std::string_view{new_value};
    if (old_text.empty())
    {
        return value;
    }

    std::string result = value;
    std::size_t start = 0;
    while (true)
    {
        const std::size_t index = result.find(old_text, start);
        if (index == std::string::npos)
        {
            break;
        }

        result.replace(index, old_text.size(), new_text);
        start = index + new_text.size();
    }

    return result;
}

inline std::size_t csbind23_string_trim_begin_index(const std::string& value)
{
    std::size_t index = 0;
    while (index < value.size() && std::isspace(static_cast<unsigned char>(value[index])) != 0)
    {
        ++index;
    }

    return index;
}

inline std::size_t csbind23_string_trim_end_index(const std::string& value)
{
    std::size_t index = value.size();
    while (index > 0 && std::isspace(static_cast<unsigned char>(value[index - 1])) != 0)
    {
        --index;
    }

    return index;
}

inline std::string csbind23_string_trim(const std::string& value)
{
    const std::size_t begin = csbind23_string_trim_begin_index(value);
    const std::size_t end = csbind23_string_trim_end_index(value);
    if (begin >= end)
    {
        return {};
    }

    return value.substr(begin, end - begin);
}

inline std::string csbind23_string_trim_start(const std::string& value)
{
    const std::size_t begin = csbind23_string_trim_begin_index(value);
    return value.substr(begin);
}

inline std::string csbind23_string_trim_end(const std::string& value)
{
    const std::size_t end = csbind23_string_trim_end_index(value);
    return value.substr(0, end);
}

inline std::string csbind23_string_to_upper_invariant(const std::string& value)
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

inline std::string csbind23_string_to_lower_invariant(const std::string& value)
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

inline int csbind23_string_compare_utf8(const std::string& value, const char* other)
{
    return value.compare(other == nullptr ? "" : other);
}

inline int csbind23_string_compare_string(const std::string& value, const std::string& other)
{
    return value.compare(other);
}

inline bool csbind23_string_equals_utf8(const std::string& value, const char* other)
{
    return csbind23_string_compare_utf8(value, other) == 0;
}

inline bool csbind23_string_equals_string(const std::string& value, const std::string& other)
{
    return csbind23_string_compare_string(value, other) == 0;
}

inline std::string csbind23_string_clone(const std::string& value)
{
    return value;
}

} // namespace csbind23

namespace csbind23::detail
{
inline std::string make_string_wrapper_csharp_code(std::string_view wrapper_name)
{
    std::string code = R"csbind23(
    private string _managedCache = string.Empty;
    private bool _cacheValid;

    private bool CanUseManagedCache => _ownership == global::CsBind23.Generated.ItemOwnership.Owned;

    public __CSBIND23_STRING_WRAPPER__(string value) : this()
    {
        Set(value);
    }

    public __CSBIND23_STRING_WRAPPER__(__CSBIND23_STRING_WRAPPER__ value) : this(value is null ? string.Empty : value.ToString())
    {
    }

    internal static __CSBIND23_STRING_WRAPPER__ FromManaged(string value)
    {
        return new __CSBIND23_STRING_WRAPPER__(value);
    }

    internal static __CSBIND23_STRING_WRAPPER__ FromOwnedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new __CSBIND23_STRING_WRAPPER__();
        }

        var value = new __CSBIND23_STRING_WRAPPER__(handle, __CSBIND23_ITEM_OWNERSHIP__.Owned);
        value._cacheValid = false;
        value._managedCache = string.Empty;
        return value;
    }

    internal static __CSBIND23_STRING_WRAPPER__ FromBorrowedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new __CSBIND23_STRING_WRAPPER__();
        }

        var value = new __CSBIND23_STRING_WRAPPER__(handle, __CSBIND23_ITEM_OWNERSHIP__.Borrowed);
        value._cacheValid = false;
        value._managedCache = string.Empty;
        return value;
    }

    internal System.IntPtr EnsureHandle()
    {
        ThrowIfDisposed();
        return _cPtr.Handle;
    }

    internal void InvalidateManagedCache()
    {
        ThrowIfDisposed();
        _cacheValid = false;
    }

    private void UpdateManagedCacheAfterSet(string value)
    {
        if (CanUseManagedCache)
        {
            _managedCache = value ?? string.Empty;
            _cacheValid = true;
        }
        else
        {
            _cacheValid = false;
        }
    }

    private void UpdateManagedCacheAfterAppend(string value)
    {
        if (!CanUseManagedCache)
        {
            _cacheValid = false;
            return;
        }

        if (_cacheValid)
        {
            _managedCache = (_managedCache ?? string.Empty) + (value ?? string.Empty);
        }
        else
        {
            _managedCache = __snapshot_utf8();
        }

        _cacheValid = true;
    }

    private void ThrowIfDisposed()
    {
        if (_cPtr.Handle == System.IntPtr.Zero)
        {
            throw new System.ObjectDisposedException(nameof(__CSBIND23_STRING_WRAPPER__));
        }
    }

    private string SnapshotManaged()
    {
        ThrowIfDisposed();

        if (!CanUseManagedCache)
        {
            return __snapshot_utf8();
        }

        if (!_cacheValid)
        {
            _managedCache = __snapshot_utf8();
            _cacheValid = true;
        }

        return _managedCache ?? string.Empty;
    }

    public int Length => __utf8_length();

    public int Count => Length;

    public char this[int index] => unchecked((char)__char_at_utf8(index));

    public global::Std.StringView AsView()
    {
        ThrowIfDisposed();
        return __snapshot_view();
    }

    public __CSBIND23_STRING_WRAPPER__ Set(string value)
    {
        ThrowIfDisposed();
        string text = value ?? string.Empty;
        __assign_utf8(text);
        UpdateManagedCacheAfterSet(text);
        return this;
    }

    public __CSBIND23_STRING_WRAPPER__ Clear()
    {
        ThrowIfDisposed();
        __clear();
        UpdateManagedCacheAfterSet(string.Empty);
        return this;
    }

    public __CSBIND23_STRING_WRAPPER__ Append(string value)
    {
        ThrowIfDisposed();
        string text = value ?? string.Empty;
        __append_utf8(text);
        UpdateManagedCacheAfterAppend(text);
        return this;
    }

    public __CSBIND23_STRING_WRAPPER__ Append(__CSBIND23_STRING_WRAPPER__ value)
    {
        ThrowIfDisposed();
        if (value is null)
        {
            return this;
        }

        __append_string(value);
        UpdateManagedCacheAfterAppend(value.ToString());
        return this;
    }

    public bool Contains(string value, global::System.StringComparison comparisonType)
    {
        return comparisonType == global::System.StringComparison.Ordinal
            ? Contains(value ?? string.Empty)
            : SnapshotManaged().Contains(value ?? string.Empty, comparisonType);
    }

    public bool StartsWith(string value, global::System.StringComparison comparisonType)
    {
        return comparisonType == global::System.StringComparison.Ordinal
            ? StartsWith(value ?? string.Empty)
            : SnapshotManaged().StartsWith(value ?? string.Empty, comparisonType);
    }

    public bool EndsWith(string value, global::System.StringComparison comparisonType)
    {
        return comparisonType == global::System.StringComparison.Ordinal
            ? EndsWith(value ?? string.Empty)
            : SnapshotManaged().EndsWith(value ?? string.Empty, comparisonType);
    }

    public int IndexOf(string value, global::System.StringComparison comparisonType)
    {
        return comparisonType == global::System.StringComparison.Ordinal
            ? IndexOf(value ?? string.Empty)
            : SnapshotManaged().IndexOf(value ?? string.Empty, comparisonType);
    }

    public string[] Split(params char[] separators)
    {
        return SnapshotManaged().Split(separators);
    }

    public override string ToString()
    {
        return SnapshotManaged();
    }

    public override bool Equals(object obj)
    {
        if (obj is __CSBIND23_STRING_WRAPPER__ other)
        {
            return Equals(other);
        }

        if (obj is string text)
        {
            return Equals(text);
        }

        return false;
    }

    public override int GetHashCode()
    {
        return global::System.StringComparer.Ordinal.GetHashCode(SnapshotManaged());
    }

    public global::System.Collections.Generic.IEnumerator<char> GetEnumerator()
    {
        for (int i = 0; i < Count; ++i)
        {
            yield return this[i];
        }
    }

    global::System.Collections.IEnumerator global::System.Collections.IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }

    public static bool IsNullOrEmpty(__CSBIND23_STRING_WRAPPER__ value)
    {
        return value is null || value.Length == 0;
    }

    public static bool IsNullOrWhiteSpace(__CSBIND23_STRING_WRAPPER__ value)
    {
        return value is null || string.IsNullOrWhiteSpace(value.ToString());
    }

    public static implicit operator __CSBIND23_STRING_WRAPPER__(string value)
    {
        return FromManaged(value);
    }

    public static implicit operator string(__CSBIND23_STRING_WRAPPER__ value)
    {
        return value is null ? string.Empty : value.ToString();
    }

    public static bool operator ==(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        if (object.ReferenceEquals(left, right))
        {
            return true;
        }

        if (left is null || right is null)
        {
            return false;
        }

        return left.Equals(right);
    }

    public static bool operator !=(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return !(left == right);
    }

    public static bool operator ==(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        return left is null ? string.IsNullOrEmpty(right) : left.Equals(right);
    }

    public static bool operator !=(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        return !(left == right);
    }

    public static bool operator ==(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return right == left;
    }

    public static bool operator !=(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return !(right == left);
    }

    public static bool operator <(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return left is null ? right is not null : left.CompareTo(right) < 0;
    }

    public static bool operator <=(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return left is null || left.CompareTo(right) <= 0;
    }

    public static bool operator >(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return left is not null && left.CompareTo(right) > 0;
    }

    public static bool operator >=(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return right is null || left is not null && left.CompareTo(right) >= 0;
    }

    public static bool operator <(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        return left is null ? !string.IsNullOrEmpty(right) : left.CompareTo(right) < 0;
    }

    public static bool operator <=(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        return left is null || left.CompareTo(right) <= 0;
    }

    public static bool operator >(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        return left is not null && left.CompareTo(right) > 0;
    }

    public static bool operator >=(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        return left is null ? string.IsNullOrEmpty(right) : left.CompareTo(right) >= 0;
    }

    public static bool operator <(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return right is null ? false : right.CompareTo(left ?? string.Empty) > 0;
    }

    public static bool operator <=(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return right is null || right.CompareTo(left ?? string.Empty) >= 0;
    }

    public static bool operator >(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return right is not null && right.CompareTo(left ?? string.Empty) < 0;
    }

    public static bool operator >=(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return right is null || right.CompareTo(left ?? string.Empty) <= 0;
    }

    public static __CSBIND23_STRING_WRAPPER__ operator +(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        var result = left?.Clone() ?? new __CSBIND23_STRING_WRAPPER__();
        return right is null ? result : result.Append(right);
    }

    public static __CSBIND23_STRING_WRAPPER__ operator +(__CSBIND23_STRING_WRAPPER__ left, string right)
    {
        var result = left?.Clone() ?? new __CSBIND23_STRING_WRAPPER__();
        return result.Append(right);
    }

    public static __CSBIND23_STRING_WRAPPER__ operator +(string left, __CSBIND23_STRING_WRAPPER__ right)
    {
        var result = new __CSBIND23_STRING_WRAPPER__(left ?? string.Empty);
        return right is null ? result : result.Append(right);
    }

    public static __CSBIND23_STRING_WRAPPER__ Concat(string left, string right)
    {
        return FromManaged((left ?? string.Empty) + (right ?? string.Empty));
    }

    public static __CSBIND23_STRING_WRAPPER__ Concat(__CSBIND23_STRING_WRAPPER__ left, __CSBIND23_STRING_WRAPPER__ right)
    {
        return left + right;
    }
)csbind23";

    return replace_all(std::move(code), "__CSBIND23_STRING_WRAPPER__", wrapper_name);
}

} // namespace csbind23::detail

namespace csbind23::cabi::detail
{

inline constexpr std::string_view std_string_wrapper_managed_type_name()
{
    return "global::CsBind23.Generated.Std.String";
}

} // namespace csbind23::cabi::detail

namespace csbind23::cabi
{

template <> struct Converter<std::string>
{
    using cpp_type = std::string;
    using c_abi_type = const char*;

    static constexpr std::string_view c_abi_type_name() { return "const char*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return detail::std_string_wrapper_managed_type_name(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "global::CsBind23.Generated.CsBind23Utf8Interop.StringToNative(({value} is null ? string.Empty : {value}.ToString()))";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "global::CsBind23.Generated.Std.String.FromManaged(global::CsBind23.Generated.CsBind23Utf8Interop.NativeToString({value}))";
    }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "global::CsBind23.Generated.CsBind23Utf8Interop.Free({pinvoke})";
    }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement()
    {
        return "global::CsBind23.Generated.CsBind23Utf8Interop.Free({pinvoke})";
    }

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

template <> struct Converter<std::string&>
{
    using cpp_type = std::string&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return detail::std_string_wrapper_managed_type_name(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} is null ? System.IntPtr.Zero : {value}.EnsureHandle())";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "global::CsBind23.Generated.Std.String.FromBorrowedHandle({value})";
    }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "if ({managed} is not null)\n{\n    {managed}.InvalidateManagedCache();\n}";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<std::string*>(value); }
};

template <> struct Converter<const std::string&>
{
    using cpp_type = const std::string&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return detail::std_string_wrapper_managed_type_name(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} is null ? System.IntPtr.Zero : {value}.EnsureHandle())";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "global::CsBind23.Generated.Std.String.FromBorrowedHandle({value})";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const std::string*>(value); }
};

template <> struct Converter<std::string*>
{
    using cpp_type = std::string*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return detail::std_string_wrapper_managed_type_name(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} is null ? System.IntPtr.Zero : {value}.EnsureHandle())";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "global::CsBind23.Generated.Std.String.FromBorrowedHandle({value})";
    }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement()
    {
        return "if ({managed} is not null)\n{\n    {managed}.InvalidateManagedCache();\n}";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <> struct Converter<const std::string*>
{
    using cpp_type = const std::string*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static constexpr std::string_view managed_type_name() { return detail::std_string_wrapper_managed_type_name(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} is null ? System.IntPtr.Zero : {value}.EnsureHandle())";
    }
    static constexpr std::string_view managed_from_pinvoke_expression()
    {
        return "global::CsBind23.Generated.Std.String.FromBorrowedHandle({value})";
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

} // namespace csbind23::cabi

namespace csbind23
{

inline ClassBuilder add_string(BindingsGenerator& generator)
{
    auto module = detail::ensure_stl_module(generator);

    auto builder = module.class_<std::string>("String", CppName{"std::string"});
    builder.csharp_interface("System.Collections.Generic.IEnumerable<char>");
    builder.csharp_interface("System.Collections.IEnumerable");
    builder.csharp_interface("global::Std.IStringUtf8View");
    builder.ctor<>();
    builder.def<&csbind23::csbind23_string_utf8_length>("__utf8_length", Private{});
    builder.def<&csbind23::csbind23_string_char_at_utf8>("__char_at_utf8", Private{});
    builder.def<&csbind23::csbind23_string_clear>("__clear", Private{});
    builder.def<&csbind23::csbind23_string_snapshot_utf8>("__snapshot_utf8", Private{});
    builder.def<&csbind23::csbind23_string_snapshot_view>("__snapshot_view", Private{});
    builder.def<&csbind23::csbind23_string_assign_utf8>("__assign_utf8", Private{});
    builder.def<&csbind23::csbind23_string_append_utf8>("__append_utf8", Private{});
    builder.def<&csbind23::csbind23_string_append_string>("__append_string", Private{});
    builder.def<&csbind23::csbind23_string_contains_utf8>("Contains");
    builder.def<&csbind23::csbind23_string_starts_with_utf8>("StartsWith");
    builder.def<&csbind23::csbind23_string_ends_with_utf8>("EndsWith");
    builder.def<&csbind23::csbind23_string_index_of_utf8>("IndexOf");
    builder.def<&csbind23::csbind23_string_last_index_of_utf8>("LastIndexOf");
    builder.def<&csbind23::csbind23_string_substring>("Substring", ExportName{"__substring"});
    builder.def<&csbind23::csbind23_string_substring_len>("Substring", ExportName{"__substring_len"});
    builder.def<&csbind23::csbind23_string_replace_utf8>("Replace");
    builder.def<&csbind23::csbind23_string_trim>("Trim");
    builder.def<&csbind23::csbind23_string_trim_start>("TrimStart");
    builder.def<&csbind23::csbind23_string_trim_end>("TrimEnd");
    builder.def<&csbind23::csbind23_string_to_upper_invariant>("ToUpperInvariant");
    builder.def<&csbind23::csbind23_string_to_lower_invariant>("ToLowerInvariant");
    builder.def<&csbind23::csbind23_string_compare_utf8>("CompareTo", ExportName{"__compare_utf8"});
    builder.def<&csbind23::csbind23_string_compare_string>("CompareTo", ExportName{"__compare_string"});
    builder.def<&csbind23::csbind23_string_equals_utf8>("Equals", ExportName{"__equals_utf8"});
    builder.def<&csbind23::csbind23_string_equals_string>("Equals", ExportName{"__equals_string"});
    builder.def<&csbind23::csbind23_string_clone>("Clone");
    builder.csharp_code(detail::make_string_wrapper_csharp_code("String"));
    return builder;
}

} // namespace csbind23
