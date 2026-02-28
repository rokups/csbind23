#include "csbind23/bindings_generator.hpp"
#include "csbind23/core_runtime.hpp"

#include <filesystem>

namespace csbind23::testing
{

void register_bindings(BindingsGenerator& generator);

} // namespace csbind23::testing

int main(int argc, char** argv)
{
    const std::filesystem::path output_root =
        argc > 1 ? std::filesystem::path(argv[1]) : (std::filesystem::current_path() / "generated");

    csbind23::BindingsGenerator generator;
    csbind23::core::register_core_bindings(generator, "csbind23_core", "e2e.C");
    csbind23::testing::register_bindings(generator);

    (void)generator.generate_cabi(output_root / "cabi");
    (void)generator.generate_csharp(output_root / "csharp");
    return 0;
}
