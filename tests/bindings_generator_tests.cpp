#include "csbind23/bindings_generator.hpp"
#include "csbind23/cabi/converter.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace
{

struct FinalizableValue
{
    int payload = 0;
    int* finalize_call_count = nullptr;

    ~FinalizableValue()
    {
        if (finalize_call_count != nullptr)
        {
            ++(*finalize_call_count);
        }
    }
};

struct FinalizableValueAbi
{
    int payload = 0;
    int* finalize_call_count = nullptr;
};

int add(int left, int right)
{
    return left + right;
}

struct Counter
{
    int increment(int value) { return value + 1; }

    int read() const { return 42; }
};

std::filesystem::path output_root()
{
    return std::filesystem::temp_directory_path() / "csbind23_codegen_tests";
}

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream input(path);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::optional<std::filesystem::path> find_file_ending_with(
    const std::vector<std::filesystem::path>& files, std::string_view suffix)
{
    for (const auto& file : files)
    {
        if (file.filename().string().ends_with(suffix))
        {
            return file;
        }
    }
    return std::nullopt;
}

} // namespace

namespace csbind23::cabi
{

template <> struct Converter<FinalizableValue>
{
    using cpp_type = FinalizableValue;
    using c_abi_type = FinalizableValueAbi;

    static constexpr std::string_view c_abi_type_name() { return "FinalizableValueAbi"; }

    static constexpr std::string_view pinvoke_type_name() { return "int"; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{.payload = value.payload, .finalize_call_count = value.finalize_call_count};
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return cpp_type{.payload = value.payload, .finalize_call_count = value.finalize_call_count};
    }
};

} // namespace csbind23::cabi

TEST(ConverterFinalizationTests, CallsConverterDestructorAfterFromCAbi)
{
    int converter_finalize_call_count = 0;

    {
        auto converted = csbind23::cabi::Converter<FinalizableValue>::from_c_abi(
            FinalizableValueAbi{.payload = 7, .finalize_call_count = &converter_finalize_call_count});

        EXPECT_EQ(converted.payload, 7);
        EXPECT_EQ(converter_finalize_call_count, 0);
    }

    EXPECT_EQ(converter_finalize_call_count, 1);
}

TEST(BindingsGeneratorTests, CapturesDeclarationsIntoModuleIr)
{
    csbind23::BindingsGenerator generator;

    auto module = generator.module("math");
    module.def("add", &add);
    module.class_<Counter>("Counter")
        .ctor<>()
        .def("increment", &Counter::increment)
        .def("read", &Counter::read, csbind23::Ownership::Borrowed);

    ASSERT_EQ(generator.modules().size(), 1U);
    const auto& module_ir = generator.modules().front();
    EXPECT_EQ(module_ir.name, "math");

    ASSERT_EQ(module_ir.functions.size(), 1U);
    EXPECT_EQ(module_ir.functions.front().name, "add");
    EXPECT_EQ(module_ir.functions.front().return_type.cpp_name, "int");
    EXPECT_EQ(module_ir.functions.front().return_ownership, csbind23::Ownership::Auto);
    EXPECT_EQ(csbind23::infer_ownership(module_ir.functions.front()), csbind23::Ownership::Owned);

    ASSERT_EQ(module_ir.classes.size(), 1U);
    EXPECT_EQ(module_ir.classes.front().name, "Counter");
    ASSERT_EQ(module_ir.classes.front().methods.size(), 3U);

    const auto& ctor_decl = module_ir.classes.front().methods[0];
    EXPECT_EQ(ctor_decl.return_ownership, csbind23::Ownership::Auto);
    EXPECT_EQ(csbind23::infer_ownership(ctor_decl), csbind23::Ownership::Owned);

    const auto& read_decl = module_ir.classes.front().methods[2];
    EXPECT_EQ(read_decl.return_ownership, csbind23::Ownership::Borrowed);
    EXPECT_EQ(csbind23::infer_ownership(read_decl), csbind23::Ownership::Borrowed);
}

TEST(BindingsGeneratorTests, GeneratesCabiAndCsharpArtifacts)
{
    csbind23::BindingsGenerator generator;
    generator.module("math").def<&add>().class_<Counter>().ctor<>().def<&Counter::increment>().def<&Counter::read>();

    const auto cabi_path = output_root() / "generated" / "cabi";
    const auto csharp_path = output_root() / "generated" / "csharp";
    std::filesystem::remove_all(output_root());

    const auto cabi_files = generator.generate_cabi(cabi_path);
    const auto csharp_files = generator.generate_csharp(csharp_path);

    ASSERT_EQ(cabi_files.size(), 1U);
    ASSERT_EQ(csharp_files.size(), 2U);
    EXPECT_TRUE(std::filesystem::exists(cabi_files.front()));

    const auto module_csharp = find_file_ending_with(csharp_files, "math.g.cs");
    const auto class_csharp = find_file_ending_with(csharp_files, "math.Counter.g.cs");
    ASSERT_TRUE(module_csharp.has_value());
    ASSERT_TRUE(class_csharp.has_value());
    EXPECT_TRUE(std::filesystem::exists(*module_csharp));
    EXPECT_TRUE(std::filesystem::exists(*class_csharp));

    const std::string cabi_content = read_text(cabi_files.front());
    EXPECT_NE(cabi_content.find("extern \"C\" int math_add("), std::string::npos);
    EXPECT_NE(cabi_content.find("math_Counter_create"), std::string::npos);
    EXPECT_NE(cabi_content.find("math_Counter_destroy"), std::string::npos);
    EXPECT_NE(cabi_content.find("decltype(auto) __csbind23_arg0_cpp = csbind23::cabi::Converter<int>::from_c_abi(arg0);"), std::string::npos);
    EXPECT_NE(cabi_content.find("decltype(auto) __csbind23_self_cpp = csbind23::cabi::Converter<"), std::string::npos);

    const std::string csharp_module_content = read_text(*module_csharp);
    EXPECT_NE(csharp_module_content.find("[System.Runtime.InteropServices.DllImport(\"math\""),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("internal static extern int math_add("), std::string::npos);
    EXPECT_NE(
        csharp_module_content.find("internal static extern System.IntPtr math_Counter_create("), std::string::npos);
    EXPECT_NE(csharp_module_content.find("internal static extern void math_Counter_destroy(System.IntPtr self);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("public static class mathApi"), std::string::npos);

    const std::string csharp_class_content = read_text(*class_csharp);
    EXPECT_NE(csharp_class_content.find("public sealed class Counter : System.IDisposable"), std::string::npos);
    EXPECT_NE(csharp_class_content.find("public int increment(int arg0)"), std::string::npos);
}

TEST(BindingsGeneratorTests, AutoAndOverrideNamesAreSupported)
{
    csbind23::BindingsGenerator generator;

    generator.module("names")
        .def<&add>()
        .def<&add>("sum")
        .class_<Counter>()
        .def<&Counter::increment>()
        .def<&Counter::read>("peek");

    ASSERT_EQ(generator.modules().size(), 1U);
    const auto& module_ir = generator.modules().front();

    ASSERT_EQ(module_ir.functions.size(), 2U);
    EXPECT_EQ(module_ir.functions[0].name, "add");
    EXPECT_EQ(module_ir.functions[1].name, "sum");
    EXPECT_NE(module_ir.functions[0].cpp_symbol.find("add"), std::string::npos);
    EXPECT_NE(module_ir.functions[1].cpp_symbol.find("add"), std::string::npos);

    ASSERT_EQ(module_ir.classes.size(), 1U);
    const auto& class_ir = module_ir.classes.front();
    EXPECT_EQ(class_ir.name, "Counter");
    EXPECT_NE(class_ir.cpp_name.find("Counter"), std::string::npos);

    ASSERT_EQ(class_ir.methods.size(), 2U);
    EXPECT_EQ(class_ir.methods[0].name, "increment");
    EXPECT_EQ(class_ir.methods[1].name, "peek");
}

TEST(BindingsGeneratorTests, EmitsConfiguredPInvokeLibraryName)
{
    csbind23::BindingsGenerator generator;

    auto module = generator.module("math");
    module.pinvoke_library("math.C").def<&add>();

    const auto csharp_path = output_root() / "generated" / "csharp_custom_pinvoke";
    std::filesystem::remove_all(csharp_path);

    const auto csharp_files = generator.generate_csharp(csharp_path);
    const auto module_csharp = find_file_ending_with(csharp_files, "math.g.cs");
    ASSERT_TRUE(module_csharp.has_value());

    const std::string csharp_module_content = read_text(*module_csharp);
    EXPECT_NE(csharp_module_content.find("[System.Runtime.InteropServices.DllImport(\"math.C\""), std::string::npos);
}

TEST(BindingsGeneratorTests, EmitsInlineManagedConvertersInCsharpWrappers)
{
    csbind23::BindingsGenerator generator;

    csbind23::ManagedInlineConverter int_converter;
    int_converter.managed_type_name = "MyInt";
    int_converter.to_pinvoke_expression = "Converters.ToNative({value})";
    int_converter.from_pinvoke_expression = "Converters.FromNative({value})";
    int_converter.finalize_to_pinvoke_statement = "Converters.FinalizeInput({managed}, {pinvoke})";
    int_converter.finalize_from_pinvoke_statement = "Converters.FinalizeOutput({managed}, {pinvoke})";

    generator.managed_converter<int>(int_converter);
    generator.module("math").def<&add>();

    const auto csharp_path = output_root() / "generated" / "csharp_inline_managed";
    std::filesystem::remove_all(csharp_path);

    const auto csharp_files = generator.generate_csharp(csharp_path);
    const auto module_csharp = find_file_ending_with(csharp_files, "math.g.cs");
    ASSERT_TRUE(module_csharp.has_value());

    const std::string csharp_module_content = read_text(*module_csharp);
    EXPECT_NE(csharp_module_content.find("internal static extern int math_add(int arg0, int arg1);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("public static MyInt add(MyInt arg0, MyInt arg1)"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("int __csbind23_arg0_pinvoke = Converters.ToNative(arg0);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("MyInt __csbind23_result_managed = default!;"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("__csbind23_result_managed = Converters.FromNative(__csbind23_result_pinvoke);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("Converters.FinalizeInput(arg0, __csbind23_arg0_pinvoke);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("Converters.FinalizeOutput(__csbind23_result_managed, __csbind23_result_pinvoke);"),
        std::string::npos);
}
