using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class PolymorphismTests
{
    [Fact]
    public void Factory_ReturnsBaseInstance_WhenRequested()
    {
        var animal = Assert.IsType<Animal>(PolymorphismApi.make_animal(false));
        Assert.Equal(1, animal.sound_code());
        Assert.Equal(1, animal.sound_code_through_native());
    }

    [Fact]
    public void Factory_ReturnsDerivedInstance_WhenRequested()
    {
        var animal = Assert.IsType<Dog>(PolymorphismApi.make_animal(true));
        Assert.Equal(2, animal.sound_code());
        Assert.Equal(2, animal.sound_code_through_native());
    }
}
