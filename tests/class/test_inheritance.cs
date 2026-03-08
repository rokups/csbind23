using CsBind23.Tests.E2E.Inheritance;
using Xunit;

namespace CsBind23.Tests.E2E;

public class InheritanceTests
{
    [Fact]
    public void Sum_BindsFreeFunction()
    {
        Assert.Equal(11, InheritanceApi.sum(5, 6));
    }

    [Fact]
    public void Derived_ExposesBaseMethods()
    {
        using var value = new DerivedValue(10);
        Assert.Equal(13, value.increment_from_base(3));
        Assert.Equal(13, value.read_from_base());
    }

    [Fact]
    public void Derived_ExposesDerivedMethods()
    {
        using var value = new DerivedValue(10);
        Assert.Equal(14, value.increment_twice(2));
        Assert.Equal(14, value.read_from_base());
    }
}
