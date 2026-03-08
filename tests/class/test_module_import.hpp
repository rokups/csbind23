#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::module_import
{

class SharedAnimal
{
public:
    virtual ~SharedAnimal() = default;

    virtual int sound_code() const
    {
        return 1;
    }

    int sound_code_through_native() const
    {
        return sound_code();
    }
};

class ImportedDog : public SharedAnimal
{
public:
    int sound_code() const override
    {
        return 2;
    }
};

class SharedTrickPerformer
{
public:
    virtual ~SharedTrickPerformer() = default;

    int trick_score() const
    {
        return 7;
    }

    virtual int bonus_score(int base_value) const
    {
        return base_value + 3;
    }
};

class ImportedShowDog : public SharedAnimal, public SharedTrickPerformer
{
public:
    int sound_code() const override
    {
        return 4;
    }

    int bonus_score(int base_value) const override
    {
        return base_value + 10;
    }

    int combined_score(int base_value) const
    {
        return trick_score() + bonus_score(base_value);
    }
};

inline SharedAnimal* make_shared_animal()
{
    static SharedAnimal animal;
    return &animal;
}

inline SharedAnimal* make_imported_dog_as_base()
{
    static ImportedDog dog;
    return &dog;
}

inline SharedAnimal* make_imported_show_dog_as_animal()
{
    static ImportedShowDog dog;
    return &dog;
}

inline SharedTrickPerformer* make_imported_show_dog_as_performer()
{
    static ImportedShowDog dog;
    return &dog;
}

inline int read_sound_code(const SharedAnimal& animal)
{
    return animal.sound_code();
}

inline int read_bonus_score(const SharedTrickPerformer& performer, int base_value)
{
    return performer.bonus_score(base_value);
}

} // namespace csbind23::testing::module_import

namespace csbind23::testing
{

inline void register_bindings_module_import(BindingsGenerator& generator, std::string_view module_name)
{
    (void)module_name;

    auto consumer_module = generator.module("module_import_consumer");
    consumer_module.import_module("module_import_animals")
        .csharp_api_class("ModuleImportConsumerApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_module_import.hpp\"")
        .def<&module_import::make_imported_dog_as_base>()
        .def<&module_import::make_imported_show_dog_as_animal>()
        .def<&module_import::make_imported_show_dog_as_performer>()
        .def<&module_import::read_sound_code>()
        .def<&module_import::read_bonus_score>();

    consumer_module.class_<module_import::ImportedDog, module_import::SharedAnimal>()
        .def<&module_import::ImportedDog::sound_code>();

    consumer_module.class_<module_import::ImportedShowDog, module_import::SharedAnimal, module_import::SharedTrickPerformer>()
        .def<&module_import::ImportedShowDog::sound_code>()
        .def<&module_import::ImportedShowDog::bonus_score>()
        .def<&module_import::ImportedShowDog::combined_score>();

    auto animals_module = generator.module("module_import_animals");
    animals_module.csharp_api_class("ModuleImportAnimalsApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_module_import.hpp\"")
        .def<&module_import::make_shared_animal>();

    animals_module.class_<module_import::SharedAnimal>()
        .def<&module_import::SharedAnimal::sound_code>(csbind23::Virtual{})
        .def<&module_import::SharedAnimal::sound_code_through_native>();

    animals_module.class_<module_import::SharedTrickPerformer>()
        .def<&module_import::SharedTrickPerformer::trick_score>()
        .def<&module_import::SharedTrickPerformer::bonus_score>(csbind23::Virtual{});
}

} // namespace csbind23::testing
