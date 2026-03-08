using CsBind23.Tests.E2E.Array;
using Xunit;

namespace CsBind23.Tests.E2E
{
    public class ArrayTests
    {
        [Fact]
        public void Array_ByValueRoundTrip_Works()
        {
            var input = new[] { 1, 2, 3, 4 };
            var output = ArrayApi.array_identity(input);
            Assert.Equal(new[] { 1, 2, 3, 4 }, output);
        }

        [Fact]
        public void Array_ByValueMutation_Works()
        {
            var output = ArrayApi.array_add_scalar(new[] { 3, 5, 7, 11 }, 2);
            Assert.Equal(new[] { 5, 7, 9, 13 }, output);
        }

        [Fact]
        public void Array_ConstRefInput_Works()
        {
            var total = ArrayApi.array_sum_const_ref(new[] { 10, 20, 30, 40 });
            Assert.Equal(100, total);
        }

        [Fact]
        public void Array_RefParameter_MutatesManagedArray()
        {
            var value = new[] { 2, 4, 6, 8 };
            ArrayApi.array_add_scalar_ref(ref value, 5);
            Assert.Equal(new[] { 7, 9, 11, 13 }, value);
        }

        [Fact]
        public void Array_RefParameter_ReassignsManagedArray()
        {
            var value = new[] { 0, 0, 0, 0 };
            ArrayApi.array_set_sequence_ref(ref value);
            Assert.Equal(new[] { 4, 3, 2, 1 }, value);
        }

        [Fact]
        public void Array_LengthMismatch_Throws()
        {
            Assert.Throws<System.ArgumentException>(() => ArrayApi.array_identity(new[] { 1, 2, 3 }));
        }

        [Fact]
        public void Array_FloatByValue_UsesGenericArrayMarshalling()
        {
            var output = ArrayApi.array_scale(new[] { 1.5f, 2.0f, 4.0f }, 0.5f);
            Assert.Equal(new[] { 0.75f, 1.0f, 2.0f }, output);
        }

        [Fact]
        public void Array_ObjectByValue_UsesConverterBackedElements()
        {
            var value = new[]
            {
                new SequentialVec2(1.0f, 2.0f),
                new SequentialVec2(3.0f, 4.0f),
            };

            var output = ArrayApi.array_shift_vec2(value, 0.5f);

            Assert.Equal(
                new[]
                {
                    new SequentialVec2(1.5f, 1.5f),
                    new SequentialVec2(3.5f, 3.5f),
                },
                output);
            Assert.Equal(10.0f, ArrayApi.array_sum_vec2(value));
        }

        [Fact]
        public void Array_ObjectPointerArray_UsesManagedWrappers()
        {
            using var first = new ArrayItem(5);
            using var second = new ArrayItem(7);
            using var third = new ArrayItem(11);

            Assert.Equal(23, ArrayApi.array_sum_item_pointers(new[] { first, second, third }));

            var rotated = ArrayApi.array_rotate_item_pointers(new[] { first, second, third });
            Assert.Equal(7, rotated[0].get());
            Assert.Equal(11, rotated[1].get());
            Assert.Equal(5, rotated[2].get());
        }

        [Fact]
        public void CArray_PointerOnly_UsesManagedArray()
        {
            var total = ArrayApi.array_sum_ptr_fixed4(new[] { 1, 3, 5, 7 });
            Assert.Equal(16, total);
        }

        [Fact]
        public void CArray_PointerWithSize_InfersCount()
        {
            var total = ArrayApi.array_sum_ptr_counted(new[] { 2, 4, 6, 8, 10 });
            Assert.Equal(30, total);
        }

        [Fact]
        public void CArray_PointerWithSize_MutableCopiesBack()
        {
            var value = new[] { 10, 20, 30, 40 };
            ArrayApi.array_add_scalar_ptr_counted(value, 5);
            Assert.Equal(new[] { 15, 25, 35, 45 }, value);
        }

        [Fact]
        public void CArray_FloatPointerWithSize_InfersCount()
        {
            var total = ArrayApi.array_sum_ptr_counted_float(new[] { 1.25f, 2.5f, 3.75f });
            Assert.Equal(7.5f, total);
        }

        [Fact]
        public void CArray_FloatPointerWithSize_MutableCopiesBack()
        {
            var value = new[] { 1.0f, 2.0f, 3.0f };
            ArrayApi.array_add_scalar_ptr_counted_float(value, 0.25f);
            Assert.Equal(new[] { 1.25f, 2.25f, 3.25f }, value);
        }
    }
}
