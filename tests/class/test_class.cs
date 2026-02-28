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

}
