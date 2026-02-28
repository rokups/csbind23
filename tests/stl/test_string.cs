using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E
{
    public class StringTests
    {
        [Fact]
        public void ReturnByValue_Works()
        {
            Assert.Equal("abc|value", stringApi.string_echo_by_value("abc"));
        }

        [Fact]
        public void ConstByValueParameter_Works()
        {
            Assert.Equal("abc|const-value", stringApi.string_echo_const_value("abc"));
        }

        [Fact]
        public void ConstRefParameter_Works()
        {
            Assert.Equal("hello|const-ref", stringApi.string_consume_const_ref("hello"));
        }

        [Fact]
        public void ConstCharPointer_ParameterAndReturn_MapToString()
        {
            Assert.Equal("hello|cstr", stringApi.string_consume_cstr("hello"));
            Assert.Equal("cstr-return", stringApi.string_get_cstr());
        }

        [Fact]
        public void StringView_ParameterAndReturn_MapToString()
        {
            Assert.Equal("hello|view", stringApi.string_consume_string_view("hello"));
            Assert.Equal("view-return", stringApi.string_get_string_view());
        }

        [Fact]
        public void ReturnConstRef_Works()
        {
            Assert.Equal("const-ref", stringApi.string_get_const_ref());
        }

        [Fact]
        public void ReturnNonConstRef_Works()
        {
            Assert.Equal("ref", stringApi.string_get_ref());
        }

        [Fact]
        public void ReturnNonConstRef_ReflectsNativeMutation()
        {
            stringApi.string_set_ref_value("mutated");
            Assert.Equal("mutated", stringApi.string_get_ref());

            stringApi.string_set_ref_value("ref");
            Assert.Equal("ref", stringApi.string_get_ref());
        }

        [Fact]
        public void RefParameter_AssignsBackToManaged()
        {
            var value = "before";
            stringApi.string_assign_by_ref(ref value, "after");
            Assert.Equal("after", value);
        }

        [Fact]
        public void RefAndConstRefParameters_WorkTogether()
        {
            var value = "base";
            stringApi.string_append_by_const_ref(ref value, "+suffix");
            Assert.Equal("base+suffix", value);
        }

        [Fact]
        public void OutputParameter_Works()
        {
            var output = string.Empty;
            stringApi.string_write_output(ref output);
            Assert.Equal("output", output);
        }

        [Fact]
        public void PointerParameter_WritesOutputLikeRef()
        {
            var value = "seed";
            stringApi.string_write_output_ptr(ref value);
            Assert.Equal("output-ptr", value);
        }

        [Fact]
        public void ConstPointerParameter_WorksLikeConstRef()
        {
            Assert.Equal("hello|const-ptr", stringApi.string_consume_const_ptr("hello"));
        }
    }
}
