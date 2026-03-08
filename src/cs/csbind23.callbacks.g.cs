namespace CsBind23.Generated;

internal static class CsBind23DelegateRegistry
{
    private static readonly object Gate = new();
    private static readonly System.Collections.Generic.Dictionary<nint, System.Runtime.InteropServices.GCHandle> FunctionPointers = new();
    private static readonly System.Runtime.CompilerServices.ConditionalWeakTable<System.Delegate, object> NativeOwners = new();

    internal static System.IntPtr RegisterFunctionPointer(System.Delegate callback)
    {
        if (callback is null)
        {
            return System.IntPtr.Zero;
        }

        lock (Gate)
        {
            System.IntPtr ptr = System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(callback);
            if (!FunctionPointers.ContainsKey(ptr))
            {
                FunctionPointers[ptr] = System.Runtime.InteropServices.GCHandle.Alloc(callback);
            }
            return ptr;
        }
    }

    internal static System.IntPtr CreateLifetimeHandle(System.Delegate callback)
    {
        if (callback is null)
        {
            return System.IntPtr.Zero;
        }

        return System.Runtime.InteropServices.GCHandle.ToIntPtr(System.Runtime.InteropServices.GCHandle.Alloc(callback));
    }

    internal static void FreeLifetimeHandle(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }

        System.Runtime.InteropServices.GCHandle gcHandle = System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
        if (gcHandle.IsAllocated)
        {
            gcHandle.Free();
        }
    }

    internal static void AttachNativeOwner(System.Delegate callback, object owner)
    {
        if (callback is null)
        {
            return;
        }

        NativeOwners.Add(callback, owner);
    }
}

internal sealed class CsBind23NativeDelegateHandle : System.IDisposable
{
    private System.IntPtr _handle;
    private readonly System.Action<System.IntPtr> _release;

    internal CsBind23NativeDelegateHandle(System.IntPtr handle, System.Action<System.IntPtr> release)
    {
        _handle = handle;
        _release = release;
    }

    internal System.IntPtr Handle => _handle;

    public void Dispose()
    {
        Dispose(true);
    }

    private void Dispose(bool disposing)
    {
        if (_handle != System.IntPtr.Zero)
        {
            _release(_handle);
            _handle = System.IntPtr.Zero;
        }

        if (disposing)
        {
            System.GC.SuppressFinalize(this);
        }
    }

    ~CsBind23NativeDelegateHandle()
    {
        Dispose(false);
    }
}
