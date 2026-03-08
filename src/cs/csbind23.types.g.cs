namespace CsBind23.Generated;

public static class CsBind23Utf8Interop
{
    public static System.IntPtr StringToNative(string value)
    {
        string text = value ?? string.Empty;
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(text);
        System.IntPtr ptr = CsBind23Memory.Alloc(checked((nuint)(bytes.Length + 1)));
        if (bytes.Length != 0)
        {
            System.Runtime.InteropServices.Marshal.Copy(bytes, 0, ptr, bytes.Length);
        }
        System.Runtime.InteropServices.Marshal.WriteByte(ptr, bytes.Length, 0);
        return ptr;
    }

    public static string NativeToString(System.IntPtr ptr)
    {
        if (ptr == System.IntPtr.Zero)
        {
            return string.Empty;
        }

        int length = 0;
        while (System.Runtime.InteropServices.Marshal.ReadByte(ptr, length) != 0)
        {
            length++;
        }

        if (length == 0)
        {
            return string.Empty;
        }

        byte[] bytes = new byte[length];
        System.Runtime.InteropServices.Marshal.Copy(ptr, bytes, 0, length);
        return System.Text.Encoding.UTF8.GetString(bytes);
    }

    public static string NativeToString(System.IntPtr ptr, nuint length)
    {
        if (ptr == System.IntPtr.Zero || length == 0)
        {
            return string.Empty;
        }

        int byteLength = checked((int)length);
        byte[] bytes = new byte[byteLength];
        System.Runtime.InteropServices.Marshal.Copy(ptr, bytes, 0, byteLength);
        return System.Text.Encoding.UTF8.GetString(bytes);
    }

    public static void Free(System.IntPtr ptr)
    {
        CsBind23Memory.Free(ptr);
    }
}
