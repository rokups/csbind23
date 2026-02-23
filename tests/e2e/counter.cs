using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class OverriddenAccumulator : Accumulator
{
    public OverriddenAccumulator(int start)
        : base(start)
    {
    }

    public override int add(int arg0)
    {
        return base.add(arg0) + 100;
    }

    public override int read()
    {
        return base.read() + 7;
    }
}

public class BindingE2ETests
{
    [Fact]
    public void Sum_BindsFreeFunction()
    {
        Assert.Equal(9, e2eApi.sum(4, 5));
    }

    [Fact]
    public void Accumulator_InstanceMethods_Work()
    {
        using var accumulator = new Accumulator(10);
        Assert.Equal(13, accumulator.add(3));
        Assert.Equal(13, accumulator.read());
    }

    [Fact]
    public void Accumulator_NativeTrampolines_DispatchVirtuals()
    {
        using var accumulator = new Accumulator(10);
        Assert.Equal(13, accumulator.add_through_native(3));
        Assert.Equal(13, accumulator.read_through_native());
    }

    [Fact]
    public void PolymorphicFactory_ReturnsBaseType_WhenBaseRequested()
    {
        var basePolymorphic = e2eApi.make_polymorphic(false);
        Assert.IsType<Accumulator>(basePolymorphic);
    }

    [Fact]
    public void PolymorphicFactory_ReturnsDerivedType_WhenDerivedRequested()
    {
        var derivedPolymorphic = e2eApi.make_polymorphic(true);
        Assert.IsType<FancyAccumulator>(derivedPolymorphic);
    }

    [Fact]
    public void ManagedOverride_DirectCalls_UseOverrideLayer()
    {
        using var overridden = new OverriddenAccumulator(10);

        Assert.Equal(213, overridden.add(3));
        Assert.Equal(27, overridden.read());
    }

    [Fact]
    public void ManagedOverride_NativeDispatch_UsesManagedOverride()
    {
        using var overridden = new OverriddenAccumulator(10);

        Assert.Equal(113, overridden.add_through_native(3));
        Assert.Equal(20, overridden.read_through_native());
    }
}
