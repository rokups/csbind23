using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class ClassGenericTests
{
    [Fact]
    public void GenericClass_Set_Add_Get_Works_For_Int_And_Double()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(10);
        Assert.Equal(13, intCounter.add(3));
        Assert.Equal(13, intCounter.get());

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(1.5);
        Assert.Equal(3.75, doubleCounter.add(2.25));
        Assert.Equal(3.75, doubleCounter.get());
    }

    [Fact]
    public void GenericClass_Throws_For_Unmapped_Type()
    {
        var ex = Assert.Throws<NotSupportedException>(() => new GenericCounter<float>());
        Assert.Contains("GenericCounter", ex.Message);
    }

    [Fact]
    public void GenericClass_Ref_And_Value_Variants_Work()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(42);
        Assert.Equal(42, intCounter.get());

        int target = 10;
        intCounter.add_into_int_ref(ref target);
        Assert.Equal(52, target);

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(2.5);
        Assert.Equal(2.5, doubleCounter.get());

        int intTarget = 4;
        doubleCounter.add_into_int_ref(ref intTarget);
        Assert.Equal(6, intTarget);
    }

    [Fact]
    public void GenericClass_ByRef_And_Out_On_ClassType_Work()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(4);

        var intTarget = 10;
        intCounter.add_into_value_ref(ref intTarget);
        Assert.Equal(14, intTarget);
        intCounter.add_pair_into_value_out(6, out var intOut);
        Assert.Equal(10, intOut);
        intCounter.add_into_value_ptr(ref intTarget);
        Assert.Equal(18, intTarget);

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(1.5);

        var doubleTarget = 2.25;
        doubleCounter.add_into_value_ref(ref doubleTarget);
        Assert.Equal(3.75, doubleTarget);
        doubleCounter.add_pair_into_value_out(5.0, out var doubleOut);
        Assert.Equal(6.5, doubleOut);
        doubleCounter.add_into_value_ptr(ref doubleTarget);
        Assert.Equal(5.25, doubleTarget);
    }

    [Fact]
    public void GenericClass_ConstMethod_With_String_Parameter_Works()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(8);
        Assert.Equal("count:8", intCounter.describe("count"));

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(3.5);
        Assert.Equal("v:3.5", doubleCounter.describe("v"));
    }

    [Fact]
    public void GenericClass_Contains_GenericMethod_Const_Dispatches()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(7);
        Assert.Equal(10, intCounter.cast_add_int(3));
        Assert.Equal(7.5, intCounter.cast_add_double(0.5));

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(2.25);
        Assert.Equal(5, doubleCounter.cast_add_int(3));
        Assert.Equal(2.75, doubleCounter.cast_add_double(0.5));
    }

    [Fact]
    public void GenericClass_Contains_GenericMethod_NonConst_Dispatches_And_Mutates()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(5);
        Assert.Equal(8, intCounter.mutate_and_cast_int(3));
        Assert.Equal(8, intCounter.get());

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(1.5);
        Assert.Equal(3.75, doubleCounter.mutate_and_cast_double(2.25));
        Assert.Equal(3.75, doubleCounter.get());
    }

    [Fact]
    public void GenericClass_GenericMethod_TwoTypeParameter_Variants_Work()
    {
        using var intCounter = new GenericCounter<int>();
        intCounter.set(4);
        Assert.Equal(12, intCounter.add_pair_int_double(5, 3.8));
        Assert.Equal(12.5, intCounter.add_pair_double_int(5.5, 3));

        using var doubleCounter = new GenericCounter<double>();
        doubleCounter.set(1.5);
        Assert.Equal(9, doubleCounter.add_pair_int_double(5, 3.8));
        Assert.Equal(10.0, doubleCounter.add_pair_double_int(5.5, 3));
    }

    [Fact]
    public void GenericMethod_On_NonGenericClass_RefParameter_Dispatches()
    {
        using var ops = new RefGenericOps();

        var intValue = 3;
        ops.add_to_ref_generic<int>(ref intValue, 4);
        Assert.Equal(7, intValue);

        var doubleValue = 2.5;
        ops.add_to_ref_generic<double>(ref doubleValue, 1.25);
        Assert.Equal(3.75, doubleValue);
    }

    [Fact]
    public void GenericMethod_On_NonGenericClass_OutParameter_Dispatches()
    {
        using var ops = new RefGenericOps();

        ops.add_to_out_generic<int>(3, 4, out var intValue);
        Assert.Equal(7, intValue);

        ops.add_to_out_generic<double>(2.5, 1.25, out var doubleValue);
        Assert.Equal(3.75, doubleValue);
    }

    [Fact]
    public void GenericMethod_On_NonGenericClass_RefParameter_Throws_Unmapped()
    {
        using var ops = new RefGenericOps();
        var value = 1.0f;
        var ex = Assert.Throws<NotSupportedException>(() => ops.add_to_ref_generic<float>(ref value, 0.5f));
        Assert.Contains("add_to_ref_generic", ex.Message);
    }

    [Fact]
    public void GenericMethod_On_NonGenericClass_OutParameter_Throws_Unmapped()
    {
        using var ops = new RefGenericOps();
        var value = 0.0f;
        var ex = Assert.Throws<NotSupportedException>(() => ops.add_to_out_generic<float>(1.0f, 0.5f, out value));
        Assert.Contains("add_to_out_generic", ex.Message);
        Assert.Contains("Expected type tuples", ex.Message);
    }

    [Fact]
    public void GenericMethod_On_NonGenericClass_PointerParameter_Dispatches()
    {
        using var ops = new RefGenericOps();

        var intByPtr = 7;
        ops.add_to_ptr_generic<int>(ref intByPtr, 5);
        Assert.Equal(12, intByPtr);

        var doubleByPtr = 2.0;
        ops.add_to_ptr_generic<double>(ref doubleByPtr, 0.5);
        Assert.Equal(2.5, doubleByPtr);
    }
}
