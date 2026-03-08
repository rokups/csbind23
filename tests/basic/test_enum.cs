using CsBind23.Tests.E2E.EnumModule;
using Xunit;

namespace CsBind23.Tests.E2E;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Method)]
public sealed class BindingMarkerAttribute : Attribute
{
}

public class EnumTests
{
    [Fact]
    public void Enum_RoundTrips()
    {
        const BasicColor value = BasicColor.Green;
        Assert.Equal(value, EnumApi.get_and_return_color(value));
    }

    [Fact]
    public void Enum_RefParameter_MutatesManagedValue()
    {
        var value = BasicColor.Red;
        EnumApi.set_color_by_ref(ref value, BasicColor.Blue);
        Assert.Equal(BasicColor.Blue, value);
    }

    [Fact]
    public void Enum_PointerParameter_MutatesManagedValue()
    {
        var value = BasicColor.Green;
        EnumApi.set_color_by_ptr(ref value, BasicColor.Red);
        Assert.Equal(BasicColor.Red, value);
    }

    [Fact]
    public void FlagsEnum_BitwiseHelpers_RoundTrip()
    {
        var combined = EnumApi.bit_or_permissions(Permissions.Read, Permissions.Execute);
        Assert.True(EnumApi.has_permission(combined, Permissions.Read));
        Assert.True(EnumApi.has_permission(combined, Permissions.Execute));
        Assert.False(EnumApi.has_permission(combined, Permissions.Write));
    }

    [Fact]
    public void FlagsEnum_HasExpectedUnderlyingType()
    {
        Assert.Equal(typeof(byte), Enum.GetUnderlyingType(typeof(Permissions)));
    }

    [Fact]
    public void FlagsEnum_HasFlagsAttribute()
    {
        Assert.NotNull(typeof(Permissions).GetCustomAttributes(typeof(FlagsAttribute), inherit: false).SingleOrDefault());
    }

    [Fact]
    public void AttributeOption_EmitsOnEnumClassFunctionAndMethod()
    {
        Assert.NotNull(typeof(BasicColor).GetCustomAttributes(typeof(BindingMarkerAttribute), inherit: false)
            .SingleOrDefault());
        Assert.NotNull(typeof(AttributedCounter).GetCustomAttributes(typeof(BindingMarkerAttribute), inherit: false)
            .SingleOrDefault());

        var function = typeof(EnumApi).GetMethod(nameof(EnumApi.get_and_return_color));
        Assert.NotNull(function);
        Assert.NotNull(function!.GetCustomAttributes(typeof(BindingMarkerAttribute), inherit: false)
            .SingleOrDefault());

        var method = typeof(AttributedCounter).GetMethod(nameof(AttributedCounter.add));
        Assert.NotNull(method);
        Assert.NotNull(method!.GetCustomAttributes(typeof(BindingMarkerAttribute), inherit: false)
            .SingleOrDefault());
    }
}
