using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E
{
    public class ArrayTests
    {
        [Fact]
        public void Array_ByValueRoundTrip_Works()
        {
            var input = new[] { 1, 2, 3, 4 };
            var output = arrayApi.array_identity(input);
            Assert.Equal(new[] { 1, 2, 3, 4 }, output);
        }

        [Fact]
        public void Array_ByValueMutation_Works()
        {
            var output = arrayApi.array_add_scalar(new[] { 3, 5, 7, 11 }, 2);
            Assert.Equal(new[] { 5, 7, 9, 13 }, output);
        }

        [Fact]
        public void Array_ConstRefInput_Works()
        {
            var total = arrayApi.array_sum_const_ref(new[] { 10, 20, 30, 40 });
            Assert.Equal(100, total);
        }

        [Fact]
        public void Array_RefParameter_MutatesManagedArray()
        {
            var value = new[] { 2, 4, 6, 8 };
            arrayApi.array_add_scalar_ref(ref value, 5);
            Assert.Equal(new[] { 7, 9, 11, 13 }, value);
        }

        [Fact]
        public void Array_RefParameter_ReassignsManagedArray()
        {
            var value = new[] { 0, 0, 0, 0 };
            arrayApi.array_set_sequence_ref(ref value);
            Assert.Equal(new[] { 4, 3, 2, 1 }, value);
        }

        [Fact]
        public void Array_LengthMismatch_Throws()
        {
            Assert.Throws<System.ArgumentException>(() => arrayApi.array_identity(new[] { 1, 2, 3 }));
        }

        [Fact]
        public void CArray_PointerOnly_UsesManagedArray()
        {
            var total = arrayApi.array_sum_ptr_fixed4(new[] { 1, 3, 5, 7 });
            Assert.Equal(16, total);
        }

        [Fact]
        public void CArray_PointerWithSize_InfersCount()
        {
            var total = arrayApi.array_sum_ptr_counted(new[] { 2, 4, 6, 8, 10 });
            Assert.Equal(30, total);
        }

        [Fact]
        public void CArray_PointerWithSize_MutableCopiesBack()
        {
            var value = new[] { 10, 20, 30, 40 };
            arrayApi.array_add_scalar_ptr_counted(value, 5);
            Assert.Equal(new[] { 15, 25, 35, 45 }, value);
        }
    }
}
