#include <filesystem>
#include <iostream>

#include "bindings_declarations.hpp"
#include "csbind23/bindings_generator.hpp"

int main(int argc, char** argv) {
#ifdef CSBIND23_SOURCE_DIR
    const std::filesystem::path default_output_root = std::filesystem::path(CSBIND23_SOURCE_DIR) / "example" / "generated";
#else
    const std::filesystem::path default_output_root = std::filesystem::current_path() / "example" / "generated";
#endif

    const std::filesystem::path output_root = argc > 1
        ? std::filesystem::path(argv[1])
        : default_output_root;

    csbind23::BindingsGenerator generator;
    example::register_bindings(generator);

    const auto cabi_files = generator.generate_cabi(output_root / "cabi");
    const auto csharp_files = generator.generate_csharp(output_root / "csharp");

    std::cout << "Generated C-ABI files:\n";
    for (const auto& file : cabi_files) {
        std::cout << "  - " << file << '\n';
    }

    std::cout << "Generated C# files:\n";
    for (const auto& file : csharp_files) {
        std::cout << "  - " << file << '\n';
    }

    std::cout << "Output root: " << output_root << '\n';
    return 0;
}
