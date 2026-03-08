using CsBind23.Tests.E2E.BasicGeneric;
using Xunit;

namespace CsBind23.Tests.E2E;

public class BasicGenericTests
{
    [Fact]
    public void ValuePassthrough_Generic_Dispatches_Int32_And_Double()
    {
        Assert.Equal(123, BasicGenericApi.value_passthrough_generic<int>(123));
        Assert.Equal(45.75, BasicGenericApi.value_passthrough_generic<double>(45.75));
    }

    [Fact]
    public void ValuePassthrough_Generic_Throws_For_Unmapped_Type()
    {
        var ex = Assert.Throws<NotSupportedException>(() => BasicGenericApi.value_passthrough_generic<float>(1.5f));
        Assert.Contains("value_passthrough_generic", ex.Message);
    }

    [Fact]
    public void GenericRef_Dispatches_And_Mutates_Int_And_Double()
    {
        var intValue = 9;
        BasicGenericApi.add_assign_generic_ref<int>(ref intValue, 4);
        Assert.Equal(13, intValue);

        var doubleValue = 1.25;
        BasicGenericApi.add_assign_generic_ref<double>(ref doubleValue, 0.75);
        Assert.Equal(2.0, doubleValue);
    }

    [Fact]
    public void GenericOut_Dispatches_And_Assigns_Int_And_Double()
    {
        BasicGenericApi.add_assign_generic_out<int>(9, 4, out var intValue);
        Assert.Equal(13, intValue);

        BasicGenericApi.add_assign_generic_out<double>(1.25, 0.75, out var doubleValue);
        Assert.Equal(2.0, doubleValue);
    }

    [Fact]
    public void GenericRef_Throws_For_Unmapped_Type()
    {
        var floatValue = 1.0f;
        var ex = Assert.Throws<NotSupportedException>(() => BasicGenericApi.add_assign_generic_ref<float>(ref floatValue, 0.5f));
        Assert.Contains("add_assign_generic_ref", ex.Message);
    }

    [Fact]
    public void GenericOut_Throws_For_Unmapped_Type()
    {
        var floatValue = 0.0f;
        var ex = Assert.Throws<NotSupportedException>(() => BasicGenericApi.add_assign_generic_out<float>(1.0f, 0.5f, out floatValue));
        Assert.Contains("add_assign_generic_out", ex.Message);
        Assert.Contains("Expected type tuples", ex.Message);
    }

    [Fact]
    public void AddCasted_Generic_Dispatches_TwoTypeParameter_Variants()
    {
        Assert.Equal(17, BasicGenericApi.add_casted_generic<int, double>(12, 5.8));
        Assert.Equal(17.5, BasicGenericApi.add_casted_generic<double, int>(12.5, 5));
    }

    [Fact]
    public void AddCasted_Generic_Throws_For_Unmapped_Combination()
    {
        var ex = Assert.Throws<NotSupportedException>(() => BasicGenericApi.add_casted_generic<int, int>(1, 2));
        Assert.Contains("add_casted_generic", ex.Message);
    }

    [Fact]
    public void Ref_And_Ptr_Variants_Mutate_Int32_And_Double()
    {
        var intValue = 10;
        BasicGenericApi.bump_ref_int32(ref intValue, 7);
        Assert.Equal(17, intValue);
        BasicGenericApi.bump_ptr_int32(ref intValue, 3);
        Assert.Equal(20, intValue);

        var doubleValue = 2.5;
        BasicGenericApi.bump_ref_double(ref doubleValue, 0.5);
        Assert.Equal(3.0, doubleValue);
        BasicGenericApi.bump_ptr_double(ref doubleValue, 1.25);
        Assert.Equal(4.25, doubleValue);
    }

    [Fact]
    public void Generic_With_String_Companion_Parameter_Works_For_Int_And_Double()
    {
        Assert.Equal("id:42", BasicGenericApi.tagged_value_generic<int>(42, "id"));
        Assert.Equal("v:3.5", BasicGenericApi.tagged_value_generic<double>(3.5, "v"));
    }

    [Fact]
    public void Generic_With_String_Companion_Parameter_Throws_For_Unmapped_Type()
    {
        var ex = Assert.Throws<NotSupportedException>(() => BasicGenericApi.tagged_value_generic<float>(2.0f, "f"));
        Assert.Contains("tagged_value_generic", ex.Message);
    }

    [Fact]
    public void Generic_With_StringView_And_BasicType_Dispatches()
    {
        Assert.Equal(11, BasicGenericApi.prefixed_bias_generic<int>("x", 10));
        Assert.Equal(10, BasicGenericApi.prefixed_bias_generic<int>("", 10));
        Assert.Equal(2.25, BasicGenericApi.prefixed_bias_generic<double>("x", 1.25));
    }

    [Fact]
    public void Generic_With_Fixed_Bool_Parameter_Dispatches()
    {
        Assert.Equal(7, BasicGenericApi.choose_by_flag_generic<int>(7, 9, true));
        Assert.Equal(9, BasicGenericApi.choose_by_flag_generic<int>(7, 9, false));
        Assert.Equal(4.5, BasicGenericApi.choose_by_flag_generic<double>(4.5, 8.5, true));
    }
}
