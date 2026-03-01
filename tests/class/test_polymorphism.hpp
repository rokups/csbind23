#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string_view>

namespace csbind23::testing::polymorphism
{

class Animal
{
public:
    virtual ~Animal() = default;

    virtual int sound_code() const
    {
        return 1;
    }

    int sound_code_through_native() const
    {
        return sound_code();
    }
};

class Dog : public Animal
{
public:
    int sound_code() const override
    {
        return 2;
    }
};

inline Animal* make_animal(bool dog)
{
    static Animal base_animal;
    static Dog dog_animal;
    return dog ? static_cast<Animal*>(&dog_animal) : static_cast<Animal*>(&base_animal);
}

} // namespace csbind23::testing::polymorphism

namespace csbind23::testing
{

inline void register_bindings_polymorphism(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("PolymorphismApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/class/test_polymorphism.hpp\"")
        .def<&polymorphism::make_animal>();

    module.class_<polymorphism::Animal>()
        .def<&polymorphism::Animal::sound_code>(csbind23::Virtual{})
        .def<&polymorphism::Animal::sound_code_through_native>();

    module.class_<polymorphism::Dog, polymorphism::Animal>()
        .def<&polymorphism::Dog::sound_code>();
}

} // namespace csbind23::testing
