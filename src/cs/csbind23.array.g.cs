namespace CsBind23.Generated;

public static class CsBind23ArrayInterop
{
    private static void ValidateArrayLength<T>(T[] value, int expectedLength, string paramName)
    {
        if (value == null || value.Length != expectedLength)
        {
            throw new System.ArgumentException($"Expected array length {expectedLength}", paramName);
        }
    }

    private static void ValidateArrayCount<T>(T[] value, int count, string paramName)
    {
        if (value == null)
        {
            throw new System.ArgumentNullException(paramName);
        }
        if (count < 0 || count > value.Length)
        {
            throw new System.ArgumentOutOfRangeException(paramName, $"Count {count} must be between 0 and array length {value.Length}.");
        }
    }

    public static unsafe System.IntPtr ArrayToNativeFixed<T>(
        T[] value,
        int expectedLength)
        where T : unmanaged
    {
        ValidateArrayLength(value, expectedLength, nameof(value));

        if (expectedLength == 0)
        {
            return System.IntPtr.Zero;
        }

        System.IntPtr ptr = CsBind23Memory.Alloc(checked((nuint)(sizeof(T) * expectedLength)));
        new System.ReadOnlySpan<T>(value, 0, expectedLength).CopyTo(new System.Span<T>((void*)ptr, expectedLength));
        return ptr;
    }

    public static unsafe System.IntPtr ArrayToNativeCounted<T>(
        T[] value,
        int count,
        string paramName)
        where T : unmanaged
    {
        ValidateArrayCount(value, count, paramName);
        if (count == 0)
        {
            return System.IntPtr.Zero;
        }

        System.IntPtr ptr = CsBind23Memory.Alloc(checked((nuint)(sizeof(T) * count)));
        new System.ReadOnlySpan<T>(value, 0, count).CopyTo(new System.Span<T>((void*)ptr, count));
        return ptr;
    }

    public static unsafe System.IntPtr ArrayToNativeFixed<TManaged, TNative>(
        TManaged[] value,
        int expectedLength,
        System.Func<TManaged, TNative> converter)
        where TNative : unmanaged
    {
        ValidateArrayLength(value, expectedLength, nameof(value));
        if (expectedLength == 0)
        {
            return System.IntPtr.Zero;
        }

        System.IntPtr ptr = CsBind23Memory.Alloc(checked((nuint)(sizeof(TNative) * expectedLength)));
        var span = new System.Span<TNative>((void*)ptr, expectedLength);
        for (int index = 0; index < expectedLength; ++index)
        {
            span[index] = converter(value[index]);
        }
        return ptr;
    }

    public static unsafe System.IntPtr ArrayToNativeCounted<TManaged, TNative>(
        TManaged[] value,
        int count,
        string paramName,
        System.Func<TManaged, TNative> converter)
        where TNative : unmanaged
    {
        ValidateArrayCount(value, count, paramName);
        if (count == 0)
        {
            return System.IntPtr.Zero;
        }

        System.IntPtr ptr = CsBind23Memory.Alloc(checked((nuint)(sizeof(TNative) * count)));
        var span = new System.Span<TNative>((void*)ptr, count);
        for (int index = 0; index < count; ++index)
        {
            span[index] = converter(value[index]);
        }
        return ptr;
    }

    public static unsafe T[] NativeToNewArrayFixed<T>(System.IntPtr ptr, int expectedLength)
        where T : unmanaged
    {
        T[] value = new T[expectedLength];
        if (ptr != System.IntPtr.Zero)
        {
            new System.ReadOnlySpan<T>((void*)ptr, expectedLength).CopyTo(value);
        }
        return value;
    }

    public static unsafe void NativeToExistingArrayFixed<T>(System.IntPtr ptr, T[] target, int expectedLength)
        where T : unmanaged
    {
        if (ptr == System.IntPtr.Zero)
        {
            return;
        }
        ValidateArrayLength(target, expectedLength, nameof(target));
        new System.ReadOnlySpan<T>((void*)ptr, expectedLength).CopyTo(target);
    }

    public static unsafe void NativeToExistingArrayCounted<T>(System.IntPtr ptr, T[] target, int count, string paramName)
        where T : unmanaged
    {
        ValidateArrayCount(target, count, paramName);
        if (ptr == System.IntPtr.Zero || count == 0)
        {
            return;
        }
        new System.ReadOnlySpan<T>((void*)ptr, count).CopyTo(new System.Span<T>(target, 0, count));
    }

    public static unsafe TManaged[] NativeToNewArrayFixed<TManaged, TNative>(
        System.IntPtr ptr,
        int expectedLength,
        System.Func<TNative, TManaged> converter)
        where TNative : unmanaged
    {
        TManaged[] value = new TManaged[expectedLength];
        if (ptr == System.IntPtr.Zero)
        {
            return value;
        }

        var span = new System.ReadOnlySpan<TNative>((void*)ptr, expectedLength);
        for (int index = 0; index < expectedLength; ++index)
        {
            value[index] = converter(span[index]);
        }
        return value;
    }

    public static unsafe void NativeToExistingArrayFixed<TManaged, TNative>(
        System.IntPtr ptr,
        TManaged[] target,
        int expectedLength,
        System.Func<TNative, TManaged> converter)
        where TNative : unmanaged
    {
        if (ptr == System.IntPtr.Zero)
        {
            return;
        }

        ValidateArrayLength(target, expectedLength, nameof(target));
        var span = new System.ReadOnlySpan<TNative>((void*)ptr, expectedLength);
        for (int index = 0; index < expectedLength; ++index)
        {
            target[index] = converter(span[index]);
        }
    }

    public static unsafe void NativeToExistingArrayCounted<TManaged, TNative>(
        System.IntPtr ptr,
        TManaged[] target,
        int count,
        string paramName,
        System.Func<TNative, TManaged> converter)
        where TNative : unmanaged
    {
        ValidateArrayCount(target, count, paramName);
        if (ptr == System.IntPtr.Zero || count == 0)
        {
            return;
        }

        var span = new System.ReadOnlySpan<TNative>((void*)ptr, count);
        for (int index = 0; index < count; ++index)
        {
            target[index] = converter(span[index]);
        }
    }

    public static void FreeNativeBuffer(System.IntPtr ptr)
    {
        CsBind23Memory.Free(ptr);
    }
}
