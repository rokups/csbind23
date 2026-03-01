#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/emit/csharp_naming.hpp"

namespace csbind23::testing::naming
{

class naming_box
{
public:
    naming_box() = default;

    int GetValue() const
    {
        return value_;
    }

    void SetValue(int next)
    {
        value_ = next;
    }

    int ComputeSum(int delta) const
    {
        return value_ + delta;
    }

    int raw_count = 1;

private:
    int value_ = 0;
};

inline int mix_values(int left, int right)
{
    return (left * 10) + right;
}

} // namespace csbind23::testing::naming

namespace csbind23::testing
{

inline void register_bindings_naming(BindingsGenerator& generator, std::string_view module_name)
{
    auto module = generator.module(module_name);
    module.csharp_api_class("naming_api")
        .csharp_name_formatter([](CSharpNameKind kind, std::string_view name) {
            using csbind23::emit::to_pascal_case;
            using csbind23::emit::to_snake_case;

            switch (kind)
            {
            case CSharpNameKind::Class:
                return to_pascal_case(name);
            case CSharpNameKind::Function:
                return to_pascal_case(name);
            case CSharpNameKind::Method:
                return to_snake_case(name);
            case CSharpNameKind::Property:
                return std::string("Prop_") + to_pascal_case(name);
            case CSharpNameKind::MemberVar:
                return std::string("Field_") + to_pascal_case(name);
            }

            return std::string(name);
        })
        .pinvoke_library("e2e.C")
        .cabi_include("\"tests/basic/test_naming.hpp\"")
        .def<&naming::mix_values>("mixValues", Comment{"free func comment"});

    module.class_<naming::naming_box>("naming_box", Comment{"class comment"})
        .ctor<>()
        .def<&naming::naming_box::ComputeSum>("ComputeSum", Comment{"method comment"})
        .property<&naming::naming_box::GetValue, &naming::naming_box::SetValue>("DisplayValue")
        .property_comment("DisplayValue", "property comment")
        .field<&naming::naming_box::raw_count>("rawCount")
        .property_comment("rawCount", "member var comment");
}

} // namespace csbind23::testing
