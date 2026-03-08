namespace CsBind23.Generated;

[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(string),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedIn,
    typeof(CsBind23Utf8StringMarshaller))]
public static class CsBind23Utf8StringMarshaller
{
    public static System.IntPtr ConvertToUnmanaged(string managed)
    {
        return CsBind23Utf8Interop.StringToNative(managed ?? string.Empty);
    }

    public static void Free(System.IntPtr unmanaged)
    {
        CsBind23Utf8Interop.Free(unmanaged);
    }
}

[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(string),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedOut,
    typeof(CsBind23Utf8StringReturnMarshaller))]
public static class CsBind23Utf8StringReturnMarshaller
{
    public static string ConvertToManaged(System.IntPtr unmanaged)
    {
        return CsBind23Utf8Interop.NativeToString(unmanaged);
    }
}

[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(global::Std.String),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedIn,
    typeof(CsBind23StdStringValueMarshaller.ManagedToNative))]
[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(global::Std.String),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedOut,
    typeof(CsBind23StdStringValueMarshaller.NativeToManaged))]
public static class CsBind23StdStringValueMarshaller
{
    public static class ManagedToNative
    {
        public static System.IntPtr ConvertToUnmanaged(global::Std.String managed)
        {
            return CsBind23Utf8Interop.StringToNative(managed is null ? string.Empty : managed.ToString());
        }

        public static void Free(System.IntPtr unmanaged)
        {
            CsBind23Utf8Interop.Free(unmanaged);
        }
    }

    public static class NativeToManaged
    {
        public static global::Std.String ConvertToManaged(System.IntPtr unmanaged)
        {
            return global::Std.String.FromManaged(CsBind23Utf8Interop.NativeToString(unmanaged));
        }

        public static void Free(System.IntPtr unmanaged)
        {
            CsBind23Utf8Interop.Free(unmanaged);
        }
    }
}

[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(global::Std.String),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedIn,
    typeof(CsBind23StdStringHandleMarshaller.ManagedToNative))]
[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(global::Std.String),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedOut,
    typeof(CsBind23StdStringHandleMarshaller.NativeToManaged))]
public static class CsBind23StdStringHandleMarshaller
{
    public static class ManagedToNative
    {
        public static System.IntPtr ConvertToUnmanaged(global::Std.String managed)
        {
            return managed is null ? System.IntPtr.Zero : managed.EnsureHandle();
        }
    }

    public static class NativeToManaged
    {
        public static global::Std.String ConvertToManaged(System.IntPtr unmanaged)
        {
            return global::Std.String.FromBorrowedHandle(unmanaged);
        }
    }
}

[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(global::Std.String),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedIn,
    typeof(CsBind23StdStringMutableHandleMarshaller.ManagedToNative))]
[global::System.Runtime.InteropServices.Marshalling.CustomMarshaller(
    typeof(global::Std.String),
    global::System.Runtime.InteropServices.Marshalling.MarshalMode.ManagedToUnmanagedOut,
    typeof(CsBind23StdStringMutableHandleMarshaller.NativeToManaged))]
public static class CsBind23StdStringMutableHandleMarshaller
{
    public struct ManagedToNative
    {
        private global::Std.String _managed;

        public void FromManaged(global::Std.String managed)
        {
            _managed = managed;
        }

        public System.IntPtr ToUnmanaged()
        {
            return _managed is null ? System.IntPtr.Zero : _managed.EnsureHandle();
        }

        public void OnInvoked()
        {
            if (_managed is not null)
            {
                _managed.InvalidateManagedCache();
            }
        }
    }

    public static class NativeToManaged
    {
        public static global::Std.String ConvertToManaged(System.IntPtr unmanaged)
        {
            return global::Std.String.FromBorrowedHandle(unmanaged);
        }
    }
}