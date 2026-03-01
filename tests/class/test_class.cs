using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class ClassTests
{
    [Fact]
    public void Sum_BindsFreeFunction()
    {
        Assert.Equal(9, ClassApi.sum(4, 5));
    }

    [Fact]
    public void Counter_InstanceMethods_Work()
    {
        using var counter = new Counter(10);
        Assert.Equal(13, counter.add(3));
        Assert.Equal(13, counter.read());
        counter.reset(2);
        Assert.Equal(2, counter.read());
    }

    [Fact]
    public void Counter_GenericRead_DispatchesSingleTypeParameter()
    {
        using var counter = new Counter(7);
        Assert.Equal(7, counter.read_generic<int>());
        Assert.Equal(7.0, counter.read_generic<double>());
    }

    [Fact]
    public void Counter_GenericRead_ThrowsForUnmappedType()
    {
        using var counter = new Counter(7);
        var ex = Assert.Throws<NotSupportedException>(() => counter.read_generic<float>());
        Assert.Contains("read_generic", ex.Message);
    }

    [Fact]
    public void Counter_GenericAddPair_DispatchesDifferentTemplateParameters()
    {
        using var counter = new Counter(0);
        Assert.Equal(8, counter.add_pair_generic<int, double>(5, 3.8));
        Assert.Equal(8.5, counter.add_pair_generic<double, int>(5.5, 3));
    }

    [Fact]
    public void Counter_GenericAddPair_ThrowsForUnmappedCombination()
    {
        using var counter = new Counter(0);
        var ex = Assert.Throws<NotSupportedException>(() => counter.add_pair_generic<int, int>(1, 2));
        Assert.Contains("add_pair_generic", ex.Message);
    }

}
