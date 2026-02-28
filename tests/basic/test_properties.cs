using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class PropertiesTests
{
    [Fact]
    public void ReadWriteIntProperty_Works()
    {
        using var bag = new PropertyBag();
        Assert.Equal(0, bag.Count);

        bag.Count = 42;
        Assert.Equal(42, bag.Count);
    }

    [Fact]
    public void ReadWriteStringProperty_Works()
    {
        using var bag = new PropertyBag();
        Assert.Equal("initial", bag.Label);

        bag.Label = "updated";
        Assert.Equal("updated", bag.Label);
    }

    [Fact]
    public void GetterOnlyProperty_Works()
    {
        using var bag = new PropertyBag();
        bag.Label = "v1";
        Assert.Equal("tag:v1", bag.ReadOnlyTag);
    }
}
