namespace Std;

public sealed partial class Map<T0, T1>
{
    public int Count => checked((int)size());

    public bool IsReadOnly => false;

    public System.Collections.Generic.ICollection<T0> Keys => CreateKeysSnapshot();

    public System.Collections.Generic.ICollection<T1> Values => CreateValuesSnapshot();

    System.Collections.Generic.IEnumerable<T0> System.Collections.Generic.IReadOnlyDictionary<T0, T1>.Keys => Keys;

    System.Collections.Generic.IEnumerable<T1> System.Collections.Generic.IReadOnlyDictionary<T0, T1>.Values => Values;

    public T1 this[T0 key]
    {
        get
        {
            if (!ContainsKey(key))
            {
                throw new System.Collections.Generic.KeyNotFoundException();
            }

            return __get(key);
        }
        set
        {
            __set(key, value);
        }
    }

    public void Add(T0 key, T1 value)
    {
        if (ContainsKey(key))
        {
            throw new System.ArgumentException("An item with the same key has already been added.", nameof(key));
        }

        __add(key, value);
    }

    public void Add(System.Collections.Generic.KeyValuePair<T0, T1> item)
    {
        Add(item.Key, item.Value);
    }

    public bool Contains(System.Collections.Generic.KeyValuePair<T0, T1> item)
    {
        return TryGetValue(item.Key, out var value)
            && System.Collections.Generic.EqualityComparer<T1>.Default.Equals(value, item.Value);
    }

    public void CopyTo(System.Collections.Generic.KeyValuePair<T0, T1>[] array, int arrayIndex)
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
            var key = __get_key_at(i);
            array[arrayIndex + i] = new System.Collections.Generic.KeyValuePair<T0, T1>(key, __get(key));
        }
    }

    public System.Collections.Generic.IEnumerator<System.Collections.Generic.KeyValuePair<T0, T1>> GetEnumerator()
    {
        for (int i = 0; i < Count; i++)
        {
            var key = __get_key_at(i);
            yield return new System.Collections.Generic.KeyValuePair<T0, T1>(key, __get(key));
        }
    }

    public bool Remove(System.Collections.Generic.KeyValuePair<T0, T1> item)
    {
        if (!Contains(item))
        {
            return false;
        }

        return Remove(item.Key);
    }

    public bool TryGetValue(T0 key, out T1 value)
    {
        if (ContainsKey(key))
        {
            value = __get(key);
            return true;
        }

        value = default!;
        return false;
    }

    System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
    {
        return GetEnumerator();
    }

    internal static Map<T0, T1> FromOwnedHandle(System.IntPtr handle, global::CsBind23.Generated.ItemOwnership itemOwnership = global::CsBind23.Generated.ItemOwnership.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new Map<T0, T1>(itemOwnership);
        }

        return new Map<T0, T1>(handle, global::CsBind23.Generated.ItemOwnership.Owned, itemOwnership);
    }

    internal static Map<T0, T1> FromBorrowedHandle(System.IntPtr handle, global::CsBind23.Generated.ItemOwnership itemOwnership = global::CsBind23.Generated.ItemOwnership.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new Map<T0, T1>(itemOwnership);
        }

        return new Map<T0, T1>(handle, global::CsBind23.Generated.ItemOwnership.Borrowed, itemOwnership);
    }

    internal static void DestroyOwnedHandle(System.IntPtr handle, global::CsBind23.Generated.ItemOwnership itemOwnership = global::CsBind23.Generated.ItemOwnership.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        using var owner = new Map<T0, T1>(handle, global::CsBind23.Generated.ItemOwnership.Owned, itemOwnership);
    }

    internal static System.IntPtr CloneOrCreateOwned(Map<T0, T1> value)
    {
        var clone = new Map<T0, T1>(value?._itemOwnership ?? global::CsBind23.Generated.ItemOwnership.Owned);
        if (value != null)
        {
            foreach (var item in value)
            {
                clone.Add(item.Key, item.Value);
            }
        }

        var handle = clone._cPtr.Handle;
        clone._cPtr = new System.Runtime.InteropServices.HandleRef(clone, System.IntPtr.Zero);
        clone._ownership = global::CsBind23.Generated.ItemOwnership.Borrowed;
        System.GC.SuppressFinalize(clone);
        return handle;
    }

    private System.Collections.Generic.List<T0> CreateKeysSnapshot()
    {
        var result = new System.Collections.Generic.List<T0>(Count);
        for (int i = 0; i < Count; i++)
        {
            result.Add(__get_key_at(i));
        }

        return result;
    }

    private System.Collections.Generic.List<T1> CreateValuesSnapshot()
    {
        var result = new System.Collections.Generic.List<T1>(Count);
        for (int i = 0; i < Count; i++)
        {
            var key = __get_key_at(i);
            result.Add(__get(key));
        }

        return result;
    }
}
