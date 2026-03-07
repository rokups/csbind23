using CsBind23.Generated;
using StdString = CsBind23.Generated.string_module.Std.String;
using Xunit;

namespace CsBind23.Tests.E2E
{
    public class StringTests
    {
        [Fact]
        public void ReturnByValue_Works()
        {
            var result = stringApi.string_echo_by_value("abc");
            Assert.Equal("abc|value", result);
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
            var result = stringApi.string_get_ref();
            Assert.Equal("ref", result);

            stringApi.string_set_ref_value("mutated");
            Assert.Equal("mutated", result);

            stringApi.string_set_ref_value("ref");
            Assert.Equal("ref", result);
        }

        [Fact]
        public void WrapperParameter_AssignsBackToManaged()
        {
            using var value = new StdString("before");
            stringApi.string_assign_by_ref(value, "after");
            Assert.Equal("after", value);
        }

        [Fact]
        public void WrapperAndConstRefParameters_WorkTogether()
        {
            using var value = new StdString("base");
            stringApi.string_append_by_const_ref(value, "+suffix");
            Assert.Equal("base+suffix", value);
        }

        [Fact]
        public void OutputParameter_Works()
        {
            using var output = new StdString();
            stringApi.string_write_output(output);
            Assert.Equal("output", output);
        }

        [Fact]
        public void PointerParameter_WritesOutputLikeRef()
        {
            using var value = new StdString("seed");
            stringApi.string_write_output_ptr(value);
            Assert.Equal("output-ptr", value);
        }

        [Fact]
        public void ConstPointerParameter_WorksLikeConstRef()
        {
            Assert.Equal("hello|const-ptr", stringApi.string_consume_const_ptr("hello"));
        }

        [Fact]
        public void ImplicitConversions_Work()
        {
            StdString wrapper = "hello";
            string managed = wrapper;

            Assert.Equal("hello", managed);
            Assert.Equal("hello|const-ref", stringApi.string_consume_const_ref("hello"));
        }

        [Fact]
        public void Wrapper_Api_Covers_Common_String_Functionality()
        {
            using var value = new StdString("  Hello World  ");

            Assert.Equal(15, value.Length);
            Assert.Equal('H', value.Trim()[0]);
            Assert.True(value.Contains("Hello"));
            Assert.True(value.StartsWith("  He"));
            Assert.True(value.EndsWith("ld  "));
            Assert.Equal(8, value.IndexOf("World"));
            Assert.Equal(8, value.LastIndexOf("World"));
            Assert.Equal("Hello World", value.Trim());
            Assert.Equal("Hello", value.Trim().Substring(0, 5));
            Assert.Equal("  Hello There  ", value.Replace("World", "There"));
            Assert.Equal("  HELLO WORLD  ", value.ToUpperInvariant());
            Assert.Equal("  hello world  ", value.ToLowerInvariant());
            Assert.Equal(new[] { "", "", "Hello", "World", "", "" }, value.Split(' '));
        }

        [Fact]
        public void Wrapper_Indexes_Utf8_By_Character()
        {
            using var value = new StdString("héł");

            Assert.Equal(3, value.Length);
            Assert.Equal('h', value[0]);
            Assert.Equal('é', value[1]);
            Assert.Equal('ł', value[2]);
        }

        [Fact]
        public void Wrapper_MutationAndEquality_Work()
        {
            using var value = new StdString("ab");
            using var same = new StdString("abcd");
            value.Append("cd");

            Assert.Equal("abcd", value);
            Assert.Equal("abcd", value);
            Assert.Equal(same, value);
            Assert.Equal("abcd", value);
            Assert.NotEqual("efgh", value);
            Assert.Equal(0, value.CompareTo("abcd"));
            Assert.True(value < "abce");
            Assert.True("abca" < value);
            Assert.Equal("abcdef", value + "ef");

            value.Clear();
            Assert.True(StdString.IsNullOrEmpty(value));
            Assert.True(StdString.IsNullOrWhiteSpace("   "));
        }

        [Fact]
        public void Wrapper_Enumerates_Characters()
        {
            using var value = new StdString("xyz");
            Assert.Equal(new[] { 'x', 'y', 'z' }, new[] { value[0], value[1], value[2] });
        }
    }
}
