namespace CsBind23.Generated;

public enum ItemOwnership
{
    Borrowed = 0,
    Owned = 1,
}

public abstract class CsBind23Object : System.IDisposable
{
    internal System.Runtime.InteropServices.HandleRef _cPtr;
    protected ItemOwnership _ownership;

    protected CsBind23Object(System.IntPtr handle, ItemOwnership ownership)
    {
        _cPtr = new System.Runtime.InteropServices.HandleRef(this, handle);
        _ownership = ownership;
    }

    protected void SetHandle(System.IntPtr handle, ItemOwnership ownership)
    {
        _cPtr = new System.Runtime.InteropServices.HandleRef(this, handle);
        _ownership = ownership;
    }

    protected void TakeOwnershipCore()
    {
        if (_cPtr.Handle == System.IntPtr.Zero)
        {
            return;
        }

        _ownership = ItemOwnership.Owned;
    }

    protected void ReleaseOwnershipCore()
    {
        _ownership = ItemOwnership.Borrowed;
    }

    protected void ReleaseUnmanagedCore()
    {
        ReleaseHandle(_ownership);
    }

    protected void DestroyOwnedHandleCore()
    {
        ReleaseHandle(ItemOwnership.Owned);
        System.GC.SuppressFinalize(this);
    }

    private void ReleaseHandle(ItemOwnership ownership)
    {
        if (_cPtr.Handle == System.IntPtr.Zero)
        {
            return;
        }

        var handle = _cPtr.Handle;
        ReleaseNativeHandle(handle, ownership);
        _cPtr = new System.Runtime.InteropServices.HandleRef(this, System.IntPtr.Zero);
        _ownership = ItemOwnership.Borrowed;
    }

    protected abstract void ReleaseNativeHandle(System.IntPtr handle, ItemOwnership ownership);

    public void Dispose()
    {
        ReleaseUnmanagedCore();
        System.GC.SuppressFinalize(this);
    }

    ~CsBind23Object()
    {
        ReleaseUnmanagedCore();
    }
}
