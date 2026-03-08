namespace Std;

public sealed partial class Vector<T>
{
    public int Count => checked((int)size());

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

    public void Insert(int index, T item)
    {
        EnsureIndex(index, allowCount: true);
        __insert(index, item);
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

    internal static Vector<T> FromOwnedHandle(System.IntPtr handle, global::CsBind23.Generated.ItemOwnership itemOwnership = global::CsBind23.Generated.ItemOwnership.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new Vector<T>(itemOwnership);
        }

        return new Vector<T>(handle, global::CsBind23.Generated.ItemOwnership.Owned, itemOwnership);
    }

    internal static Vector<T> FromBorrowedHandle(System.IntPtr handle, global::CsBind23.Generated.ItemOwnership itemOwnership = global::CsBind23.Generated.ItemOwnership.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return new Vector<T>(itemOwnership);
        }

        return new Vector<T>(handle, global::CsBind23.Generated.ItemOwnership.Borrowed, itemOwnership);
    }

    internal static void DestroyOwnedHandle(System.IntPtr handle, global::CsBind23.Generated.ItemOwnership itemOwnership = global::CsBind23.Generated.ItemOwnership.Owned)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        using var owner = new Vector<T>(handle, global::CsBind23.Generated.ItemOwnership.Owned, itemOwnership);
    }

    internal static System.IntPtr CloneOrCreateOwned(Vector<T> value)
    {
        var clone = new Vector<T>(value?._itemOwnership ?? global::CsBind23.Generated.ItemOwnership.Owned);
        if (value != null)
        {
            foreach (var item in value)
            {
                clone.Add(item);
            }
        }

        var handle = clone._cPtr.Handle;
        clone._cPtr = new System.Runtime.InteropServices.HandleRef(clone, System.IntPtr.Zero);
        clone._ownership = global::CsBind23.Generated.ItemOwnership.Borrowed;
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
}
