#pragma once

#include "csbind23/bindings_generator.hpp"

#include <string>
#include <string_view>

namespace csbind23::testing::properties
{

class PropertyBag
{
public:
    PropertyBag() = default;

    int get_count() const
    {
        return count_;
    }

    void set_count(int value)
    {
        count_ = value;
    }

    const std::string& get_label() const
    {
        return label_;
    }

    void set_label(const std::string& value)
    {
        label_ = value;
    }

    std::string get_read_only_tag() const
    {
        return "tag:" + label_;
    }

private:
    int count_ = 0;
    std::string label_ = "initial";
};

} // namespace csbind23::testing::properties

namespace csbind23::testing
{

inline void register_bindings_properties(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("PropertiesApi")
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_properties.hpp\"");

    module.class_<properties::PropertyBag>()
        .ctor<>()
        .property<&properties::PropertyBag::get_count, &properties::PropertyBag::set_count>("Count")
        .property<&properties::PropertyBag::get_label, &properties::PropertyBag::set_label>("Label")
        .property<&properties::PropertyBag::get_read_only_tag>("ReadOnlyTag");
}

} // namespace csbind23::testing
