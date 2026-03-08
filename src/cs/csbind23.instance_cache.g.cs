namespace CsBind23.Generated;

internal interface IInstanceCache<T>
    where T : class
{
    void Register(System.IntPtr handle, T instance);
    void Unregister(System.IntPtr handle);
    bool TryGet(System.IntPtr handle, out T instance);
}

internal sealed class DefaultInstanceCache<T> : IInstanceCache<T>
    where T : class
{
    private readonly System.Threading.ReaderWriterLockSlim _lock = new System.Threading.ReaderWriterLockSlim(System.Threading.LockRecursionPolicy.NoRecursion);
    private readonly System.Collections.Generic.Dictionary<System.IntPtr, System.WeakReference<T>> _instances =
        new System.Collections.Generic.Dictionary<System.IntPtr, System.WeakReference<T>>();

    public void Register(System.IntPtr handle, T instance)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }
        _lock.EnterWriteLock();
        try
        {
            _instances[handle] = new System.WeakReference<T>(instance);
        }
        finally
        {
            _lock.ExitWriteLock();
        }
    }

    public void Unregister(System.IntPtr handle)
    {
        if (handle == System.IntPtr.Zero)
        {
            return;
        }
        _lock.EnterWriteLock();
        try
        {
            _instances.Remove(handle);
        }
        finally
        {
            _lock.ExitWriteLock();
        }
    }

    public bool TryGet(System.IntPtr handle, out T instance)
    {
        if (handle == System.IntPtr.Zero)
        {
            instance = null!;
            return false;
        }
        _lock.EnterReadLock();
        try
        {
            if (_instances.TryGetValue(handle, out var weak) && weak.TryGetTarget(out instance))
            {
                return true;
            }
        }
        finally
        {
            _lock.ExitReadLock();
        }
        instance = null!;
        return false;
    }
}
