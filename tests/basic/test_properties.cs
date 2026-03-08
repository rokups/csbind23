using CsBind23.Tests.E2E.Properties;
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

    [Fact]
    public void FieldBackedIntProperty_ReadWrite_Works()
    {
        using var bag = new PublicFieldBag();
        Assert.Equal(3, bag.Count);
        bag.Count = 21;
        Assert.Equal(21, bag.Count);
    }

    [Fact]
    public void FieldBackedStringProperty_ReadWrite_Works()
    {
        using var bag = new PublicFieldBag();
        Assert.Equal("field", bag.Label);
        bag.Label = "updated-field";
        Assert.Equal("updated-field", bag.Label);
    }

    [Fact]
    public void ConstFieldProjection_IsReadOnlyProperty()
    {
        using var bag = new PublicFieldBag();
        Assert.Equal(7, bag.Version);

        var property = typeof(PublicFieldBag).GetProperty("Version");
        Assert.NotNull(property);
        Assert.False(property!.CanWrite);
    }
}
