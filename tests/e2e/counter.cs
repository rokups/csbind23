using CsBind23.Generated;
using Xunit;

public class BindingE2ETests
{
    [Fact]
    public void FullCycle_WorksThroughGeneratedWrappers()
    {
        Assert.Equal(9, counterApi.sum(4, 5));

        using var accumulator = new Accumulator(10);
        Assert.Equal(13, accumulator.add(3));
        Assert.Equal(13, accumulator.read());
    }
}
