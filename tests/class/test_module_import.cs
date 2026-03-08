using CsBind23.Generated;
using Xunit;

namespace CsBind23.Tests.E2E;

public class ModuleImportTests
{
    [Fact]
    public void ConsumerApi_AcceptsImportedBaseType()
    {
        SharedAnimal animal = ModuleImportAnimalsApi.make_shared_animal();

        Assert.Equal(1, ModuleImportConsumerApi.read_sound_code(animal));
        Assert.Equal(1, animal.sound_code());
    }

    [Fact]
    public void ConsumerApi_ReusesImportedPolymorphicWrapper()
    {
        SharedAnimal animal = ModuleImportConsumerApi.make_imported_dog_as_base();

        ImportedDog dog = Assert.IsType<ImportedDog>(animal);
        Assert.Equal(2, dog.sound_code());
        Assert.Equal(2, dog.sound_code_through_native());
    }

    [Fact]
    public void ConsumerApi_ReusesImportedMultipleInheritance_AsPrimaryBase()
    {
        ImportedShowDog dog = Assert.IsType<ImportedShowDog>(ModuleImportConsumerApi.make_imported_show_dog_as_animal());

        Assert.Equal(4, dog.sound_code());
        Assert.Equal(4, dog.sound_code_through_native());
        Assert.Equal(7, dog.trick_score());
        Assert.Equal(15, dog.bonus_score(5));
        Assert.Equal(22, dog.combined_score(5));

        ISharedTrickPerformer performer = dog;
        Assert.Equal(7, performer.trick_score());
        Assert.Equal(15, performer.bonus_score(5));
    }

    [Fact]
    public void ConsumerApi_ReusesImportedMultipleInheritance_AsSecondaryBase()
    {
        ImportedShowDog dog = Assert.IsType<ImportedShowDog>(ModuleImportConsumerApi.make_imported_show_dog_as_performer());

        Assert.Equal(4, dog.sound_code());
        ISharedTrickPerformer performer = dog;
        Assert.Equal(15, performer.bonus_score(5));
        Assert.Equal(15, dog.bonus_score(5));
        Assert.Equal(22, dog.combined_score(5));
    }
}
