using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class NamingCustomizationTests
{
    [Fact]
    public void NamingCallback_Applies_PerElementKind()
    {
        Assert.Equal(34, NamingApi.MixValues(3, 4));

        using var box = new NamingBox();
        box.Prop_DisplayValue = 7;

        Assert.Equal(7, box.Prop_DisplayValue);
        Assert.Equal(10, box.compute_sum(3));

        Assert.Equal(1, box.Field_RawCount);
        box.Field_RawCount = 9;
        Assert.Equal(9, box.Field_RawCount);
    }
}
