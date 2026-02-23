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

struct ManagedNumber
{
    int value = 0;
};

struct ManagedLong
{
    long value = 0;
};

int add(int left, int right)
{
    return left + right;
}

ManagedNumber add_managed(ManagedNumber left, ManagedNumber right)
{
    return ManagedNumber{left.value + right.value};
}

struct Counter
{
    int increment(int value) { return value + 1; }

    int read() const { return 42; }
};

struct VirtualCounter
{
    virtual ~VirtualCounter() = default;

    virtual int increment(int value) { return value + 10; }

    virtual int read() const { return 100; }
};

struct ManagedConverterSurface
{
    explicit ManagedConverterSurface(ManagedNumber seed)
        : value(seed)
    {
    }

    ManagedNumber add(ManagedNumber delta)
    {
        value.value += delta.value;
        return value;
    }

    virtual ManagedNumber addVirtual(ManagedNumber delta)
    {
        value.value += delta.value;
        return value;
    }

    ManagedNumber value;
};

struct PolyBase
{
    virtual ~PolyBase() = default;
    virtual int kind() const { return 1; }
};

struct PolyDerived : PolyBase
{
    int kind() const override { return 2; }
};

PolyBase* make_poly(bool derived)
{
    static PolyBase base{};
    static PolyDerived poly_derived{};
    return derived ? static_cast<PolyBase*>(&poly_derived) : static_cast<PolyBase*>(&base);
}

ManagedLong scale_managed_long(ManagedLong value)
{
    return ManagedLong{value.value * 2};
}

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

    static constexpr std::string_view managed_type_name() { return ""; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return ""; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return ""; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement() { return ""; }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return ""; }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return c_abi_type{.payload = value.payload, .finalize_call_count = value.finalize_call_count};
    }

    static cpp_type from_c_abi(const c_abi_type& value)
    {
        return cpp_type{.payload = value.payload, .finalize_call_count = value.finalize_call_count};
    }
};

template <> struct Converter<ManagedNumber>
{
    using cpp_type = ManagedNumber;
    using c_abi_type = int;

    static constexpr std::string_view c_abi_type_name() { return "int"; }
    static constexpr std::string_view pinvoke_type_name() { return "int"; }

    static constexpr std::string_view managed_type_name() { return "MyInt"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "Converters.ToNative({value})"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "Converters.FromNative({value})"; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement() { return "Converters.FinalizeInput({managed}, {pinvoke})"; }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return "Converters.FinalizeOutput({managed}, {pinvoke})"; }

    static c_abi_type to_c_abi(const cpp_type& value) { return value.value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return cpp_type{value}; }
};

template <> struct Converter<ManagedLong>
{
    using cpp_type = ManagedLong;
    using c_abi_type = long;

    static constexpr std::string_view c_abi_type_name() { return "long"; }
    static constexpr std::string_view pinvoke_type_name() { return "nint"; }

    static constexpr std::string_view managed_type_name() { return "MyLong"; }
    static constexpr std::string_view managed_to_pinvoke_expression() { return "CppConverters.ToNative({value})"; }
    static constexpr std::string_view managed_from_pinvoke_expression() { return "CppConverters.FromNative({value})"; }
    static constexpr std::string_view managed_finalize_to_pinvoke_statement() { return "CppConverters.FinalizeInput({managed}, {pinvoke})"; }
    static constexpr std::string_view managed_finalize_from_pinvoke_statement() { return "CppConverters.FinalizeOutput({managed}, {pinvoke})"; }

    static c_abi_type to_c_abi(const cpp_type& value) { return value.value; }
    static cpp_type from_c_abi(const c_abi_type& value) { return cpp_type{value}; }
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

TEST(ConverterTypeNameTests, EmitsExpectedCAbiAndPInvokeNames)
{
    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<int>(), "int");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<int>(), "int");

    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<int*>(), "void*");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<int*>(), "System.IntPtr");

    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<const int*>(), "const void*");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<const int*>(), "System.IntPtr");

    EXPECT_EQ(csbind23::cabi::c_abi_type_name_for<const int&>(), "const void*");
    EXPECT_EQ(csbind23::cabi::pinvoke_type_name_for<const int&>(), "System.IntPtr");
}

TEST(BindingsGeneratorTests, AppliesManagedConverterToQualifiedTypeRefs)
{
    csbind23::BindingsGenerator generator;

    const auto by_value = generator.make_bound_type_ref<ManagedNumber>();
    const auto by_const_ref = generator.make_bound_type_ref<const ManagedNumber&>();
    const auto by_pointer = generator.make_bound_type_ref<ManagedNumber*>();

    ASSERT_TRUE(by_value.has_managed_converter());
    ASSERT_TRUE(by_const_ref.has_managed_converter());
    ASSERT_TRUE(by_pointer.has_managed_converter());
    EXPECT_EQ(by_value.managed_type_name, "MyInt");
    EXPECT_EQ(by_const_ref.managed_type_name, "MyInt");
    EXPECT_EQ(by_pointer.managed_type_name, "MyInt");
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

TEST(BindingsGeneratorTests, CapturesVirtualOverrideMetadata)
{
    csbind23::BindingsGenerator generator;

    auto module = generator.module("virtuals");
    module.class_<VirtualCounter>()
        .def_virtual<&VirtualCounter::increment>()
        .def_virtual<&VirtualCounter::read>();

    ASSERT_EQ(generator.modules().size(), 1U);
    const auto& module_ir = generator.modules().front();
    ASSERT_EQ(module_ir.classes.size(), 1U);
    const auto& class_ir = module_ir.classes.front();
    EXPECT_TRUE(class_ir.enable_virtual_overrides);

    ASSERT_EQ(class_ir.methods.size(), 2U);
    EXPECT_TRUE(class_ir.methods[0].allow_override);
    EXPECT_TRUE(class_ir.methods[0].is_virtual);
    EXPECT_EQ(class_ir.methods[0].virtual_slot_name, "increment");
    EXPECT_TRUE(class_ir.methods[1].allow_override);
    EXPECT_TRUE(class_ir.methods[1].is_virtual);
    EXPECT_EQ(class_ir.methods[1].virtual_slot_name, "read");
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

TEST(BindingsGeneratorTests, EmitsVirtualOverrideDirectorArtifacts)
{
    csbind23::BindingsGenerator generator;

    generator.module("virtuals")
        .class_<VirtualCounter>()
        .def_virtual<&VirtualCounter::increment>()
        .def_virtual<&VirtualCounter::read>();

    const auto cabi_path = output_root() / "generated" / "cabi_virtuals";
    const auto csharp_path = output_root() / "generated" / "csharp_virtuals";
    std::filesystem::remove_all(cabi_path);
    std::filesystem::remove_all(csharp_path);

    const auto cabi_files = generator.generate_cabi(cabi_path);
    const auto csharp_files = generator.generate_csharp(csharp_path);

    ASSERT_EQ(cabi_files.size(), 1U);
    ASSERT_EQ(csharp_files.size(), 2U);

    const std::string cabi_content = read_text(cabi_files.front());
    EXPECT_NE(cabi_content.find("struct virtuals_VirtualCounter_CallbackTable"), std::string::npos);
    EXPECT_NE(cabi_content.find("class virtuals_VirtualCounter_Director"), std::string::npos);
    EXPECT_NE(cabi_content.find("virtuals_VirtualCounter_connect_director"), std::string::npos);
    EXPECT_NE(cabi_content.find("virtuals_VirtualCounter_disconnect_director"), std::string::npos);
    EXPECT_NE(cabi_content.find("virtuals_VirtualCounter_increment__base"), std::string::npos);
    EXPECT_NE(cabi_content.find("virtuals_VirtualCounter_read__base"), std::string::npos);

    const auto module_csharp = find_file_ending_with(csharp_files, "virtuals.g.cs");
    const auto class_csharp = find_file_ending_with(csharp_files, "virtuals.VirtualCounter.g.cs");
    ASSERT_TRUE(module_csharp.has_value());
    ASSERT_TRUE(class_csharp.has_value());

    const std::string csharp_module_content = read_text(*module_csharp);
    EXPECT_NE(csharp_module_content.find("virtuals_VirtualCounter_connect_director"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("virtuals_VirtualCounter_disconnect_director"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("virtuals_VirtualCounter_increment__base"), std::string::npos);

    const std::string csharp_class_content = read_text(*class_csharp);
    EXPECT_NE(csharp_class_content.find("public partial class VirtualCounter"), std::string::npos);
    EXPECT_NE(csharp_class_content.find("private void __csbind23_ConnectDirector()"), std::string::npos);
    EXPECT_NE(csharp_class_content.find("public virtual int increment(int arg0)"), std::string::npos);
}

TEST(BindingsGeneratorTests, EmitsPolymorphicWrapArtifactsForBoundClassPointers)
{
    csbind23::BindingsGenerator generator;

    auto module = generator.module("poly");
    module.def("make_poly", &make_poly, csbind23::Ownership::Borrowed);
    module.class_<PolyBase>().def_virtual<&PolyBase::kind>();
    module.class_<PolyDerived, PolyBase>().def<&PolyDerived::kind>();

    const auto cabi_path = output_root() / "generated" / "cabi_poly";
    const auto csharp_path = output_root() / "generated" / "csharp_poly";
    std::filesystem::remove_all(cabi_path);
    std::filesystem::remove_all(csharp_path);

    const auto cabi_files = generator.generate_cabi(cabi_path);
    const auto csharp_files = generator.generate_csharp(csharp_path);

    ASSERT_EQ(cabi_files.size(), 1U);
    const std::string cabi_content = read_text(cabi_files.front());
    EXPECT_NE(cabi_content.find("poly_PolyBase_static_type_name"), std::string::npos);
    EXPECT_NE(cabi_content.find("poly_PolyBase_dynamic_type_name"), std::string::npos);
    EXPECT_NE(cabi_content.find("poly_PolyDerived_static_type_name"), std::string::npos);

    const auto module_csharp = find_file_ending_with(csharp_files, "poly.g.cs");
    const auto derived_csharp = find_file_ending_with(csharp_files, "poly.PolyDerived.g.cs");
    ASSERT_TRUE(module_csharp.has_value());
    ASSERT_TRUE(derived_csharp.has_value());
    const std::string csharp_module_content = read_text(*module_csharp);
    const std::string csharp_derived_content = read_text(*derived_csharp);
    EXPECT_NE(csharp_module_content.find("internal static class polyRuntime"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("WrapPolymorphic_PolyBase"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("public static object make_poly(bool arg0)"), std::string::npos);
    EXPECT_NE(csharp_derived_content.find("public sealed class PolyDerived : PolyBase"), std::string::npos);
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

    generator.module("math").def<&add_managed>("add");

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

TEST(BindingsGeneratorTests, AppliesManagedConverterFromCppSpecialization)
{
    csbind23::BindingsGenerator generator;
    generator.module("spec").def<&scale_managed_long>("scale_long");

    const auto csharp_path = output_root() / "generated" / "csharp_cpp_specialization";
    std::filesystem::remove_all(csharp_path);

    const auto csharp_files = generator.generate_csharp(csharp_path);
    const auto module_csharp = find_file_ending_with(csharp_files, "spec.g.cs");
    ASSERT_TRUE(module_csharp.has_value());

    const std::string csharp_module_content = read_text(*module_csharp);
    EXPECT_NE(csharp_module_content.find("public static MyLong scale_long(MyLong arg0)"), std::string::npos);
    EXPECT_NE(csharp_module_content.find("nint __csbind23_arg0_pinvoke = CppConverters.ToNative(arg0);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("CppConverters.FinalizeInput(arg0, __csbind23_arg0_pinvoke);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("__csbind23_result_managed = CppConverters.FromNative(__csbind23_result_pinvoke);"),
        std::string::npos);
    EXPECT_NE(csharp_module_content.find("CppConverters.FinalizeOutput(__csbind23_result_managed, __csbind23_result_pinvoke);"),
        std::string::npos);
}

TEST(BindingsGeneratorTests, EmitsInlineManagedConvertersInClassAndVirtualWrappers)
{
    csbind23::BindingsGenerator generator;

    generator.module("conv")
        .class_<ManagedConverterSurface>("ConverterSurface")
        .ctor<ManagedNumber>()
        .def<&ManagedConverterSurface::add>()
        .def_virtual<&ManagedConverterSurface::addVirtual>();

    const auto csharp_path = output_root() / "generated" / "csharp_inline_managed_class";
    std::filesystem::remove_all(csharp_path);

    const auto csharp_files = generator.generate_csharp(csharp_path);
    const auto class_csharp = find_file_ending_with(csharp_files, "conv.ConverterSurface.g.cs");
    ASSERT_TRUE(class_csharp.has_value());

    const std::string csharp_class_content = read_text(*class_csharp);

    EXPECT_NE(csharp_class_content.find("public ConverterSurface(MyInt arg0)"), std::string::npos);
    EXPECT_NE(csharp_class_content.find("int __csbind23_arg0_pinvoke = Converters.ToNative(arg0);"),
        std::string::npos);
    EXPECT_NE(csharp_class_content.find("Converters.FinalizeInput(arg0, __csbind23_arg0_pinvoke);"),
        std::string::npos);

    EXPECT_NE(csharp_class_content.find("public MyInt add(MyInt arg0)"), std::string::npos);
    EXPECT_NE(csharp_class_content.find("MyInt __csbind23_result_managed = default!;"), std::string::npos);
    EXPECT_NE(csharp_class_content.find("__csbind23_result_managed = Converters.FromNative(__csbind23_result_pinvoke);"),
        std::string::npos);
    EXPECT_NE(csharp_class_content.find("Converters.FinalizeOutput(__csbind23_result_managed, __csbind23_result_pinvoke);"),
        std::string::npos);

    EXPECT_NE(csharp_class_content.find("private static int __csbind23_CallbackMethod0_addVirtual"),
        std::string::npos);
    EXPECT_NE(csharp_class_content.find("MyInt __csbind23_arg0_managed = Converters.FromNative(arg0);"),
        std::string::npos);
    EXPECT_NE(csharp_class_content.find("return Converters.ToNative(__csbind23_result);"),
        std::string::npos);
}
