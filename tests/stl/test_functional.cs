using CsBind23.Tests.E2E.Functional;
using Xunit;

namespace CsBind23.Tests.E2E;

public class FunctionalTests
{
    [Fact]
    public void ManagedLambda_Can_Be_Marshaled_As_StdFunction_Parameter()
    {
        int result = FunctionalApi.functional_invoke_binary((left, right) => left * right, 6, 7);
        Assert.Equal(42, result);
    }

    [Fact]
    public void NativeStdFunction_Can_Be_Returned_And_Invoked_From_Managed_Code()
    {
        var add = FunctionalApi.functional_choose_binary(false);
        var subtract = FunctionalApi.functional_choose_binary(true);

        Assert.Equal(11, add(5, 6));
        Assert.Equal(-1, subtract(5, 6));
    }

    [Fact]
    public void ReturnedStdFunction_Can_Be_Passed_Back_To_Native_Code()
    {
        var add = FunctionalApi.functional_choose_binary(false);
        var subtract = FunctionalApi.functional_choose_binary(true);

        Assert.Equal(9, FunctionalApi.functional_invoke_binary(add, 4, 5));
        Assert.Equal(1, FunctionalApi.functional_invoke_binary(subtract, 4, 3));
    }

    [Fact]
    public void ManagedStdFunction_Can_Be_Stored_And_Invoked_Later_From_Native_Code()
    {
        FunctionalApi.functional_set_stored_binary((left, right) => left * right);
        Assert.Equal(54, FunctionalApi.functional_invoke_stored_binary(6, 9));

        FunctionalApi.functional_clear_stored_binary();
        Assert.Equal(-7777, FunctionalApi.functional_invoke_stored_binary(6, 9));
    }

    [Fact]
    public void UnaryStdFunction_RoundTrips_Through_Native_Storage()
    {
        FunctionalApi.functional_set_stored_unary(value => value - 3);
        Assert.Equal(8, FunctionalApi.functional_invoke_stored_unary(11));

        var plusFive = FunctionalApi.functional_choose_unary(5);
        Assert.Equal(19, plusFive(14));
        Assert.Equal(19, FunctionalApi.functional_invoke_unary(plusFive, 14));

        FunctionalApi.functional_clear_stored_unary();
        Assert.Equal(-7777, FunctionalApi.functional_invoke_stored_unary(11));
    }

    [Fact]
    public void NullStdFunction_Is_Marshaled_As_Nullptr()
    {
        Assert.Equal(-7777, FunctionalApi.functional_invoke_binary(null!, 1, 2));
        Assert.Equal(-7777, FunctionalApi.functional_invoke_unary(null!, 9));
    }
}
