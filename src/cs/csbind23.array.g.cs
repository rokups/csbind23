namespace CsBind23.Generated;

public static class CsBind23ArrayInterop
{
    public static System.IntPtr IntArrayToNativeFixed(int[] value, int expectedLength)
    {
        if (value == null || value.Length != expectedLength)
        {
            throw new System.ArgumentException($"Expected array length {expectedLength}", nameof(value));
        }

        System.IntPtr ptr = System.Runtime.InteropServices.Marshal.AllocHGlobal(sizeof(int) * expectedLength);
        System.Runtime.InteropServices.Marshal.Copy(value, 0, ptr, expectedLength);
        return ptr;
    }

    public static System.IntPtr IntArrayToNativeCounted(int[] value, int count, string paramName)
    {
        if (value == null)
        {
            throw new System.ArgumentNullException(paramName);
        }
        if (count < 0 || count > value.Length)
        {
            throw new System.ArgumentOutOfRangeException(paramName, $"Count {count} must be between 0 and array length {value.Length}.");
        }
        if (count == 0)
        {
            return System.IntPtr.Zero;
        }

        System.IntPtr ptr = System.Runtime.InteropServices.Marshal.AllocHGlobal(sizeof(int) * count);
        System.Runtime.InteropServices.Marshal.Copy(value, 0, ptr, count);
        return ptr;
    }

    public static int[] NativeToNewIntArrayFixed(System.IntPtr ptr, int expectedLength)
    {
        int[] value = new int[expectedLength];
        if (ptr != System.IntPtr.Zero)
        {
            System.Runtime.InteropServices.Marshal.Copy(ptr, value, 0, expectedLength);
        }
        return value;
    }

    public static void NativeToExistingIntArrayFixed(System.IntPtr ptr, int[] target, int expectedLength)
    {
        if (ptr == System.IntPtr.Zero)
        {
            return;
        }
        if (target == null || target.Length != expectedLength)
        {
            throw new System.ArgumentException($"Expected array length {expectedLength}", nameof(target));
        }
        System.Runtime.InteropServices.Marshal.Copy(ptr, target, 0, expectedLength);
    }

    public static void NativeToExistingIntArrayCounted(System.IntPtr ptr, int[] target, int count, string paramName)
    {
        if (target == null)
        {
            throw new System.ArgumentNullException(paramName);
        }
        if (count < 0 || count > target.Length)
        {
            throw new System.ArgumentOutOfRangeException(paramName, $"Count {count} must be between 0 and array length {target.Length}.");
        }
        if (ptr == System.IntPtr.Zero || count == 0)
        {
            return;
        }
        System.Runtime.InteropServices.Marshal.Copy(ptr, target, 0, count);
    }

    public static void FreeNativeBuffer(System.IntPtr ptr)
    {
        if (ptr != System.IntPtr.Zero)
        {
            System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr);
        }
    }
}
