#include "csbind23/bindings_generator.hpp"

#include <filesystem>

namespace csbind23::testing
{

void register_all_bindings(BindingsGenerator& generator);

} // namespace csbind23::testing

int main(int argc, char** argv)
{
    const std::filesystem::path output_root =
        argc > 1 ? std::filesystem::path(argv[1]) : (std::filesystem::current_path() / "generated");

    csbind23::BindingsGenerator generator;
    csbind23::testing::register_all_bindings(generator);

    (void)generator.generate_cabi(output_root / "cabi");
    (void)generator.generate_csharp(output_root / "csharp");
    return 0;
}
