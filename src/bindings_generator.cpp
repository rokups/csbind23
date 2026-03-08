#include "csbind23/bindings_generator.hpp"

#include "csbind23/emit/cabi_emitter.hpp"
#include "csbind23/emit/csharp_emitter.hpp"

#include <algorithm>

namespace csbind23
{

std::string BindingsGenerator::name() const
{
    return "csbind23";
}

ModuleBuilder BindingsGenerator::module(std::string_view name)
{
    return ModuleBuilder(*this, upsert_module(name));
}

const std::vector<ModuleDecl>& BindingsGenerator::modules() const
{
    return modules_;
}

std::vector<std::filesystem::path> BindingsGenerator::generate_cabi(const std::filesystem::path& output_root) const
{
    BindingsGenerator generator = *this;
    generator.append_internal_modules();
    const auto modules = generator.resolved_modules();
    std::vector<std::filesystem::path> generated_files;
    for (const auto& module_decl : modules)
    {
        auto module_files = emit::emit_cabi_module(module_decl, modules, output_root);
        generated_files.insert(generated_files.end(), module_files.begin(), module_files.end());
    }
    return generated_files;
}

std::vector<std::filesystem::path> BindingsGenerator::generate_csharp(const std::filesystem::path& output_root) const
{
    BindingsGenerator generator = *this;
    generator.append_internal_modules();
    const auto modules = generator.resolved_modules();
    std::vector<std::filesystem::path> generated_files = emit::copy_csharp_support_files(output_root);
    for (const auto& module_decl : modules)
    {
        auto module_files = emit::emit_csharp_module(module_decl, modules, output_root);
        generated_files.insert(generated_files.end(), module_files.begin(), module_files.end());
    }
    return generated_files;
}

ModuleDecl& BindingsGenerator::upsert_module(std::string_view name)
{
    const auto existing_it = std::find_if(
        modules_.begin(), modules_.end(), [name](const ModuleDecl& module_decl) { return module_decl.name == name; });

    if (existing_it != modules_.end())
    {
        return *existing_it;
    }

    ModuleDecl module_decl;
    module_decl.name = std::string(name);
    module_decl.pinvoke_library = module_decl.name;
    modules_.push_back(std::move(module_decl));
    return modules_.back();
}

ModuleBuilder::ModuleBuilder(BindingsGenerator& owner, ModuleDecl& module_decl)
    : owner_(&owner)
    , module_decl_(&module_decl)
{
}

ClassBuilder::ClassBuilder(BindingsGenerator& owner, ModuleDecl& module_decl, ClassDecl& class_decl)
    : owner_(&owner)
    , module_decl_(&module_decl)
    , class_decl_(&class_decl)
{
}

} // namespace csbind23
