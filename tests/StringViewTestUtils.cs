using System.Runtime.InteropServices;
using System.Text;

namespace CsBind23.Tests.E2E
{
    internal sealed class UnsafeStringViewLease : IDisposable
    {
        private readonly IntPtr _memory;

        public UnsafeStringViewLease(string value)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(value ?? string.Empty);
            _memory = bytes.Length == 0 ? IntPtr.Zero : Marshal.AllocHGlobal(bytes.Length);
            if (bytes.Length != 0)
            {
                Marshal.Copy(bytes, 0, _memory, bytes.Length);
            }

            View = new Std.StringView(_memory, checked((nuint)bytes.Length));
        }

        public Std.StringView View { get; }

        public void Dispose()
        {
            if (_memory != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(_memory);
            }
        }
    }

    internal static class StringViewTestUtils
    {
        public static UnsafeStringViewLease Lease(string value) => new UnsafeStringViewLease(value);
    }
}
