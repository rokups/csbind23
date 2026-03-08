using CsBind23.Tests.E2E.String;
using Xunit;

namespace CsBind23.Tests.E2E
{
    public class StringTests
    {
        [Fact]
        public void ReturnByValue_Works()
        {
            var result = StringApi.string_echo_by_value("abc");
            Assert.Equal("abc|value", result);
        }

        [Fact]
        public void ConstByValueParameter_Works()
        {
            Assert.Equal("abc|const-value", StringApi.string_echo_const_value("abc"));
        }

        [Fact]
        public void ConstRefParameter_Works()
        {
            Assert.Equal("hello|const-ref", StringApi.string_consume_const_ref("hello"));
        }

        [Fact]
        public void ConstCharPointer_ParameterAndReturn_MapToString()
        {
            Assert.Equal("hello|cstr", StringApi.string_consume_cstr("hello"));
            Assert.Equal("cstr-return", StringApi.string_get_cstr());
        }

        [Fact]
        public void StringView_ParameterAndReturn_MapToString()
        {
            Assert.Equal("hello|view", StringApi.string_consume_string_view("hello"));
            Assert.Equal("view-return", StringApi.string_get_string_view());
        }

        [Fact]
        public void ReturnConstRef_Works()
        {
            Assert.Equal("const-ref", StringApi.string_get_const_ref());
        }

        [Fact]
        public void ReturnNonConstRef_Works()
        {
            Assert.Equal("ref", StringApi.string_get_ref());
        }

        [Fact]
        public void ReturnNonConstRef_ReflectsNativeMutation()
        {
            var result = StringApi.string_get_ref();
            Assert.Equal("ref", result);

            StringApi.string_set_ref_value("mutated");
            Assert.Equal("mutated", result);

            StringApi.string_set_ref_value("ref");
            Assert.Equal("ref", result);
        }

        [Fact]
        public void WrapperParameter_AssignsBackToManaged()
        {
            using var value = new Std.String("before");
            StringApi.string_assign_by_ref(value, "after");
            Assert.Equal("after", value);
        }

        [Fact]
        public void WrapperAndConstRefParameters_WorkTogether()
        {
            using var value = new Std.String("base");
            StringApi.string_append_by_const_ref(value, "+suffix");
            Assert.Equal("base+suffix", value);
        }

        [Fact]
        public void OutputParameter_Works()
        {
            using var output = new Std.String();
            StringApi.string_write_output(output);
            Assert.Equal("output", output);
        }

        [Fact]
        public void PointerParameter_WritesOutputLikeRef()
        {
            using var value = new Std.String("seed");
            StringApi.string_write_output_ptr(value);
            Assert.Equal("output-ptr", value);
        }

        [Fact]
        public void ConstPointerParameter_WorksLikeConstRef()
        {
            Assert.Equal("hello|const-ptr", StringApi.string_consume_const_ptr("hello"));
        }

        [Fact]
        public void ImplicitConversions_Work()
        {
            Std.String wrapper = "hello";
            string managed = wrapper;

            Assert.Equal("hello", managed);
            Assert.Equal("hello|const-ref", StringApi.string_consume_const_ref("hello"));
        }

        [Fact]
        public void Wrapper_Api_Covers_Common_String_Functionality()
        {
            using var value = new Std.String("  Hello World  ");

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
            using var value = new Std.String("héł");

            Assert.Equal(3, value.Length);
            Assert.Equal('h', value[0]);
            Assert.Equal('é', value[1]);
            Assert.Equal('ł', value[2]);
        }

        [Fact]
        public void Wrapper_MutationAndEquality_Work()
        {
            using var value = new Std.String("ab");
            using var same = new Std.String("abcd");
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
            Assert.True(Std.String.IsNullOrEmpty(value));
            Assert.True(Std.String.IsNullOrWhiteSpace("   "));
        }

        [Fact]
        public void Wrapper_Enumerates_Characters()
        {
            using var value = new Std.String("xyz");
            Assert.Equal(new[] { 'x', 'y', 'z' }, new[] { value[0], value[1], value[2] });
        }
    }
}
