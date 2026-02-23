using CsBind23.Generated;
using Xunit;

namespace CsBind23.Generated
{
    internal static class StringE2EConverters
    {
        [System.Runtime.InteropServices.DllImport("e2e.C", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        private static extern System.IntPtr e2e_string_read(System.IntPtr arg0);

        [System.Runtime.InteropServices.DllImport("e2e.C", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        private static extern System.IntPtr e2e_string_echo_by_value(string arg0);

        [System.Runtime.InteropServices.DllImport("e2e.C", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        private static extern System.IntPtr e2e_string_consume_const_ref(System.IntPtr arg0);

        [System.Runtime.InteropServices.DllImport("libc", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        private static extern void free(System.IntPtr ptr);

        private static string PtrToManagedAndFree(System.IntPtr value)
        {
            if (value == System.IntPtr.Zero)
            {
                return string.Empty;
            }

            var managed = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(value) ?? string.Empty;
            free(value);
            return managed;
        }

        public static nint ToNative(string value)
        {
            return e2eApi.string_create(value ?? string.Empty);
        }

        public static string FromNative(nint value)
        {
            if (value == 0)
            {
                return string.Empty;
            }
            return PtrToManagedAndFree(e2e_string_read(value));
        }

        public static string EchoByValue(string value)
        {
            return PtrToManagedAndFree(e2e_string_echo_by_value(value ?? string.Empty));
        }

        public static string ConsumeConstRef(string value)
        {
            var native = ToNative(value);
            try
            {
                return PtrToManagedAndFree(e2e_string_consume_const_ref(native));
            }
            finally
            {
                DestroyNative(native);
            }
        }

        public static void DestroyNative(nint value)
        {
            if (value != 0)
            {
                e2eApi.string_destroy(value);
            }
        }
    }
}

namespace CsBind23.Tests.E2E
{
    public class StringBindingE2ETests
    {
        [Fact]
        public void String_ReturnByValue_Works()
        {
            Assert.Equal("abc|value", StringE2EConverters.EchoByValue("abc"));
        }

        [Fact]
        public void String_ConstRefParameter_Works()
        {
            Assert.Equal("hello|const-ref", StringE2EConverters.ConsumeConstRef("hello"));
        }

        [Fact]
        public void String_ReturnConstRef_Works()
        {
            Assert.Equal("const-ref", e2eApi.string_get_const_ref());
        }

        [Fact]
        public void String_ReturnNonConstRef_Works()
        {
            Assert.Equal("ref", e2eApi.string_get_ref());
        }

        [Fact]
        public void String_RefParameter_AssignsBackToManaged()
        {
            var value = "before";
            e2eApi.string_assign_by_ref(ref value, "after");
            Assert.Equal("after", value);
        }

        [Fact]
        public void String_RefAndConstRefParameters_WorkTogether()
        {
            var value = "base";
            e2eApi.string_append_by_const_ref(ref value, "+suffix");
            Assert.Equal("base+suffix", value);
        }

        [Fact]
        public void String_OutputParameter_Works()
        {
            var output = string.Empty;
            e2eApi.string_write_output(ref output);
            Assert.Equal("output", output);
        }
    }
}
