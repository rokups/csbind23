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

    public void SetOwnership(ItemOwnership ownership)
    {
        if (_cPtr.Handle == System.IntPtr.Zero)
        {
            return;
        }

        _ownership = ownership;
    }

    protected virtual void Dispose(bool disposing)
    {
        if (disposing)
        {
            System.GC.SuppressFinalize(this);
        }

        if (_cPtr.Handle == System.IntPtr.Zero)
        {
            return;
        }

        _cPtr = new System.Runtime.InteropServices.HandleRef(this, System.IntPtr.Zero);
        _ownership = ItemOwnership.Borrowed;
    }

    public void Dispose()
    {
        Dispose(true);
    }

    ~CsBind23Object()
    {
        Dispose(false);
    }
}
