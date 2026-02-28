using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

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
}
