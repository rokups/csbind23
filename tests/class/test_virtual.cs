using CsBind23.Tests.E2E.VirtualModule;
using Xunit;

namespace CsBind23.Tests.E2E;

public class ManagedVirtualCounter : VirtualCounter
{
    public ManagedVirtualCounter(int value)
        : base(value)
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

    public override Std.String describe()
    {
        return "ManagedVirtualCounter";
    }
}

public class ManagedPartialVirtualCounter : VirtualCounter
{
    public ManagedPartialVirtualCounter(int value)
        : base(value)
    {
    }

    public override Std.String describe()
    {
        return "ManagedPartialVirtualCounter";
    }
}

public class ManagedNoOverrideVirtualCounter : VirtualCounter
{
    public ManagedNoOverrideVirtualCounter(int value)
        : base(value)
    {
    }
}

public class VirtualTests
{
    [Fact]
    public void VirtualCounter_BaseBehavior_Works()
    {
        using var counter = new VirtualCounter(10);
        Assert.Equal(13, counter.add(3));
        Assert.Equal(13, counter.read());
        Assert.Equal("VirtualCounter", counter.describe());
    }

    [Fact]
    public void VirtualCounter_NativeDispatch_UsesBaseLayer()
    {
        using var counter = new VirtualCounter(10);
        Assert.Equal(13, counter.add_through_native(3));
        Assert.Equal(13, counter.read_through_native());
        Assert.Equal("VirtualCounter", counter.describe_through_native());
    }

    [Fact]
    public void ManagedOverride_DirectCalls_UseOverrideLayer()
    {
        using var counter = new ManagedVirtualCounter(10);
        Assert.Equal(113, counter.add(3));
        Assert.Equal(20, counter.read());
        Assert.Equal("ManagedVirtualCounter", counter.describe());
    }

    [Fact]
    public void ManagedOverride_NativeDispatch_UsesManagedOverride()
    {
        using var counter = new ManagedVirtualCounter(10);
        Assert.Equal(113, counter.add_through_native(3));
        Assert.Equal(20, counter.read_through_native());
        Assert.Equal("ManagedVirtualCounter", counter.describe_through_native());
    }

    [Fact]
    public void PartialOverride_OnlyOverriddenVirtual_IsRedirected()
    {
        using var counter = new ManagedPartialVirtualCounter(10);

        Assert.Equal(13, counter.add(3));
        Assert.Equal(13, counter.read());
        Assert.Equal("ManagedPartialVirtualCounter", counter.describe());

        Assert.Equal(16, counter.add_through_native(3));
        Assert.Equal(16, counter.read_through_native());
        Assert.Equal("ManagedPartialVirtualCounter", counter.describe_through_native());
    }

    [Fact]
    public void NoOverride_DerivedClass_UsesNativeBaseBehavior()
    {
        using var counter = new ManagedNoOverrideVirtualCounter(10);

        Assert.Equal(13, counter.add(3));
        Assert.Equal(13, counter.read());
        Assert.Equal("VirtualCounter", counter.describe());

        Assert.Equal(16, counter.add_through_native(3));
        Assert.Equal(16, counter.read_through_native());
        Assert.Equal("VirtualCounter", counter.describe_through_native());
    }

}
