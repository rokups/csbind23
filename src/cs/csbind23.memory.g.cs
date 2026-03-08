namespace CsBind23.Generated;

internal static class CsBind23Memory
{
    internal static System.IntPtr Alloc(nuint byteCount)
    {
        if (byteCount == 0)
        {
            return System.IntPtr.Zero;
        }

        System.IntPtr ptr = CsBind23MemoryApi.Alloc(byteCount);
        if (ptr == System.IntPtr.Zero)
        {
            throw new System.OutOfMemoryException($"Unable to allocate {byteCount} bytes of temporary native memory.");
        }

        return ptr;
    }

    internal static void Free(System.IntPtr ptr)
    {
        if (ptr != System.IntPtr.Zero)
        {
            CsBind23MemoryApi.Free(ptr);
        }
    }
}