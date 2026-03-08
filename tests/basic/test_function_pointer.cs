using CsBind23.Tests.E2E.FunctionPointer;
using Xunit;

namespace CsBind23.Tests.E2E;

public class FunctionPointerTests
{
    [Fact]
    public void ManagedLambda_Can_Be_Passed_As_FunctionPointer_Argument()
    {
        int result = FunctionPointerApi.fp_invoke_binary((left, right) => left * right, 6, 7);
        Assert.Equal(42, result);
    }

    [Fact]
    public void NativeFunctionPointer_Can_Be_Returned_And_Invoked_From_Managed_Code()
    {
        var add = FunctionPointerApi.fp_choose_binary(false);
        var subtract = FunctionPointerApi.fp_choose_binary(true);

        Assert.Equal(9, add(4, 5));
        Assert.Equal(1, subtract(4, 3));
    }

    [Fact]
    public void ManagedFunctionPointer_Can_Be_Stored_And_Invoked_Later_From_Native_Code()
    {
        FunctionPointerApi.fp_set_stored_binary((left, right) => (left * right) + 1);
        Assert.Equal(31, FunctionPointerApi.fp_invoke_stored_binary(5, 6));

        FunctionPointerApi.fp_clear_stored_binary();
        Assert.Equal(-9999, FunctionPointerApi.fp_invoke_stored_binary(5, 6));
    }

    [Fact]
    public void Unary_FunctionPointer_RoundTrips_Through_Native_Storage()
    {
        FunctionPointerApi.fp_set_stored_unary(value => value + 10);
        Assert.Equal(17, FunctionPointerApi.fp_invoke_stored_unary(7));

        var nativeSquare = FunctionPointerApi.fp_choose_unary(false);
        var nativeNegate = FunctionPointerApi.fp_choose_unary(true);
        Assert.Equal(81, nativeSquare(9));
        Assert.Equal(-9, nativeNegate(9));

        FunctionPointerApi.fp_clear_stored_unary();
        Assert.Equal(-9999, FunctionPointerApi.fp_invoke_stored_unary(7));
    }

    [Fact]
    public void Null_FunctionPointer_Is_Marshaled_As_Nullptr()
    {
        Assert.Equal(-9999, FunctionPointerApi.fp_invoke_binary(null!, 1, 2));
        Assert.Equal(-9999, FunctionPointerApi.fp_invoke_unary(null!, 5));
    }
}
