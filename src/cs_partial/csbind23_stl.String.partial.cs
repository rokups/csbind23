namespace Std;

public sealed partial class String
{
    private string _managedCache = string.Empty;
    private bool _cacheValid;

    private bool CanUseManagedCache => _ownership == global::CsBind23.Generated.ItemOwnership.Owned;

    public String(string value) : this()
    {
        Set(value);
    }

    public String(String value) : this(value is null ? string.Empty : value.ToString())
    {
    }

    internal static String FromManaged(string value)
    {
        return new String(value);
    }

    internal static String FromOwnedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new String();
        }

        var value = new String(handle, global::CsBind23.Generated.ItemOwnership.Owned);
        value._cacheValid = false;
        value._managedCache = string.Empty;
        return value;
    }

    internal static String FromBorrowedHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new String();
        }

        var value = new String(handle, global::CsBind23.Generated.ItemOwnership.Borrowed);
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
            throw new System.ObjectDisposedException(nameof(String));
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

    public String Set(string value)
    {
        ThrowIfDisposed();
        string text = value ?? string.Empty;
        __assign_utf8(text);
        UpdateManagedCacheAfterSet(text);
        return this;
    }

    public String Clear()
    {
        ThrowIfDisposed();
        __clear();
        UpdateManagedCacheAfterSet(string.Empty);
        return this;
    }

    public String Append(string value)
    {
        ThrowIfDisposed();
        string text = value ?? string.Empty;
        __append_utf8(text);
        UpdateManagedCacheAfterAppend(text);
        return this;
    }

    public String Append(String value)
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

    public override bool Equals(object? obj)
    {
        if (obj is String other)
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

    public static bool IsNullOrEmpty(String value)
    {
        return value is null || value.Length == 0;
    }

    public static bool IsNullOrWhiteSpace(String value)
    {
        return value is null || string.IsNullOrWhiteSpace(value.ToString());
    }

    public static implicit operator String(string value)
    {
        return FromManaged(value);
    }

    public static implicit operator string(String value)
    {
        return value is null ? string.Empty : value.ToString();
    }

    public static bool operator ==(String left, String right)
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

    public static bool operator !=(String left, String right)
    {
        return !(left == right);
    }

    public static bool operator ==(String left, string right)
    {
        return left is null ? string.IsNullOrEmpty(right) : left.Equals(right);
    }

    public static bool operator !=(String left, string right)
    {
        return !(left == right);
    }

    public static bool operator ==(string left, String right)
    {
        return right == left;
    }

    public static bool operator !=(string left, String right)
    {
        return !(right == left);
    }

    public static bool operator <(String left, String right)
    {
        return left is null ? right is not null : left.CompareTo(right) < 0;
    }

    public static bool operator <=(String left, String right)
    {
        return left is null || left.CompareTo(right) <= 0;
    }

    public static bool operator >(String left, String right)
    {
        return left is not null && left.CompareTo(right) > 0;
    }

    public static bool operator >=(String left, String right)
    {
        return right is null || left is not null && left.CompareTo(right) >= 0;
    }

    public static bool operator <(String left, string right)
    {
        return left is null ? !string.IsNullOrEmpty(right) : left.CompareTo(right) < 0;
    }

    public static bool operator <=(String left, string right)
    {
        return left is null || left.CompareTo(right) <= 0;
    }

    public static bool operator >(String left, string right)
    {
        return left is not null && left.CompareTo(right) > 0;
    }

    public static bool operator >=(String left, string right)
    {
        return left is null ? string.IsNullOrEmpty(right) : left.CompareTo(right) >= 0;
    }

    public static bool operator <(string left, String right)
    {
        return right is null ? false : right.CompareTo(left ?? string.Empty) > 0;
    }

    public static bool operator <=(string left, String right)
    {
        return right is null || right.CompareTo(left ?? string.Empty) >= 0;
    }

    public static bool operator >(string left, String right)
    {
        return right is not null && right.CompareTo(left ?? string.Empty) < 0;
    }

    public static bool operator >=(string left, String right)
    {
        return right is null || right.CompareTo(left ?? string.Empty) <= 0;
    }

    public static String operator +(String left, String right)
    {
        var result = left?.Clone() ?? new String();
        return right is null ? result : result.Append(right);
    }

    public static String operator +(String left, string right)
    {
        var result = left?.Clone() ?? new String();
        return result.Append(right);
    }

    public static String operator +(string left, String right)
    {
        var result = new String(left ?? string.Empty);
        return right is null ? result : result.Append(right);
    }

    public static String Concat(string left, string right)
    {
        return FromManaged((left ?? string.Empty) + (right ?? string.Empty));
    }

    public static String Concat(String left, String right)
    {
        return left + right;
    }
}
