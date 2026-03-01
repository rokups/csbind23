using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class ManagedMultiDerived : MultiDerived
{
    public ManagedMultiDerived(int value)
        : base(value)
    {
    }

    public override int secondary_virtual(int arg0)
    {
        return base.secondary_virtual(arg0) + 100;
    }
}

public class MultiInheritanceTests
{
    [Fact]
    public void MultiDerived_ImplementsSecondaryAndTertiaryInterfaces()
    {
        using var value = new MultiDerived(10);

        ISecondaryBase secondary = value;
        ITertiaryBase tertiary = value;

        Assert.Equal(8, secondary.secondary_scale(4));
        Assert.Equal(27, secondary.secondary_bias_add(10));
        Assert.Equal(7, tertiary.tertiary_subtract(10));
        Assert.Equal(12, value.derived_twice_primary(1));
    }

    [Fact]
    public void MultiDerived_ExposesSecondaryAndTertiaryInheritedMethods()
    {
        using var value = new MultiDerived(10);

        Assert.Equal(8, value.secondary_scale(4));
        Assert.Equal(27, value.secondary_bias_add(10));
        Assert.Equal(7, value.tertiary_subtract(10));
        Assert.Equal(6, value.secondary_virtual_through_native(5));
        Assert.Equal(6, value.call_secondary_virtual_from_cpp(5));
    }

    [Fact]
    public void ManagedOverride_SecondaryBaseVirtual_DispatchesFromNative()
    {
        using var value = new ManagedMultiDerived(10);

        Assert.Equal(106, value.secondary_virtual(5));
        Assert.Equal(106, value.secondary_virtual_through_native(5));
        Assert.Equal(106, value.call_secondary_virtual_from_cpp(5));
    }
}
