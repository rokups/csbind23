#include "csbind23/emit/csharp_emitter.hpp"
#include "csbind23/emit/csharp_naming.hpp"

#include "csharp_emitter_text_utils.hpp"
#include "emitter_ir_utils.hpp"
#include "generic_dispatch_utils.hpp"

#include "csbind23/cabi/converter.hpp"
#include "csbind23/text_writer.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace csbind23::emit
{
namespace
{

std::string csharp_namespace_name(const ModuleDecl& module_decl);
std::string wrapper_type_name(const TypeRef& type_ref);

std::filesystem::path write_csharp_file(
    const std::filesystem::path& output_root, const std::string& filename, const std::string& content)
{
    const auto output_path = output_root / filename;
    std::ofstream output(output_path, std::ios::binary);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return output_path;
}

bool is_any_of(std::string_view value, std::initializer_list<std::string_view> candidates)
{
    for (const auto candidate : candidates)
    {
        if (value == candidate)
        {
            return true;
        }
    }
    return false;
}

bool is_csharp_enum_integral_underlying(std::string_view type_name)
{
    return is_any_of(type_name, {"byte", "sbyte", "short", "ushort", "int", "uint", "long", "ulong"});
}

bool is_csharp_builtin_value_type(std::string_view type_name)
{
    return is_any_of(type_name,
        {"bool", "byte", "sbyte", "short", "ushort", "int", "uint", "long", "ulong", "nint", "nuint",
            "float", "double", "char"});
}

bool is_pinvoke_pointer_sized_integral(std::string_view type_name)
{
    return is_any_of(type_name, {"nint", "nuint"});
}

bool is_unsigned_long_family(std::string_view type_name)
{
    return is_any_of(type_name, {"unsigned long", "unsigned long long"});
}

bool is_signed_long_family(std::string_view type_name)
{
    return is_any_of(type_name, {"long", "long long"});
}

bool is_cabi_void(std::string_view c_abi_name)
{
    return c_abi_name == "void";
}

bool is_pinvoke_int_ptr(std::string_view pinvoke_name)
{
    return pinvoke_name == "System.IntPtr";
}

bool is_cpp_std_string(std::string_view cpp_name)
{
    return cpp_name == "std::string";
}

std::optional<std::string_view> pinvoke_integral_type_from_cabi(std::string_view c_abi_name)
{
    using csbind23::cabi::Converter;

    if (c_abi_name == Converter<signed char>::c_abi_type_name())
    {
        return Converter<signed char>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned char>::c_abi_type_name() || c_abi_name == Converter<char>::c_abi_type_name())
    {
        return Converter<unsigned char>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<short>::c_abi_type_name())
    {
        return Converter<short>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned short>::c_abi_type_name())
    {
        return Converter<unsigned short>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<int>::c_abi_type_name())
    {
        return Converter<int>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned int>::c_abi_type_name())
    {
        return Converter<unsigned int>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<long>::c_abi_type_name())
    {
        return Converter<long>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned long>::c_abi_type_name())
    {
        return Converter<unsigned long>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<long long>::c_abi_type_name())
    {
        return Converter<long long>::pinvoke_type_name();
    }
    if (c_abi_name == Converter<unsigned long long>::c_abi_type_name())
    {
        return Converter<unsigned long long>::pinvoke_type_name();
    }

    return std::nullopt;
}

std::string normalize_enum_underlying_integral(
    std::string_view pinvoke_name, std::string_view cpp_name, std::string_view c_abi_name)
{
    if (!is_pinvoke_pointer_sized_integral(pinvoke_name))
    {
        return std::string(pinvoke_name);
    }

    if (is_unsigned_long_family(cpp_name) || is_unsigned_long_family(c_abi_name))
    {
        return "ulong";
    }

    if (is_signed_long_family(cpp_name) || is_signed_long_family(c_abi_name))
    {
        return "long";
    }

    return "long";
}

bool module_uses_pinvoke_type(const ModuleDecl& module_decl, std::string_view pinvoke_type)
{
    auto type_matches = [pinvoke_type](const TypeRef& type_ref) {
        return type_ref.pinvoke_name == pinvoke_type;
    };

    for (const auto& function_decl : module_decl.functions)
    {
        if (type_matches(function_decl.return_type))
        {
            return true;
        }
        for (const auto& parameter : function_decl.parameters)
        {
            if (type_matches(parameter.type))
            {
                return true;
            }
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        for (const auto& method_decl : class_decl.methods)
        {
            if (type_matches(method_decl.return_type))
            {
                return true;
            }
            for (const auto& parameter : method_decl.parameters)
            {
                if (type_matches(parameter.type))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool module_uses_managed_type(const ModuleDecl& module_decl, std::string_view managed_type)
{
    auto type_matches = [managed_type](const TypeRef& type_ref) {
        return type_ref.managed_type_name == managed_type;
    };

    for (const auto& function_decl : module_decl.functions)
    {
        if (type_matches(function_decl.return_type))
        {
            return true;
        }
        for (const auto& parameter : function_decl.parameters)
        {
            if (type_matches(parameter.type))
            {
                return true;
            }
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        for (const auto& method_decl : class_decl.methods)
        {
            if (type_matches(method_decl.return_type))
            {
                return true;
            }
            for (const auto& parameter : method_decl.parameters)
            {
                if (type_matches(parameter.type))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool module_uses_c_array_int_parameters(const ModuleDecl& module_decl)
{
    auto is_int_array_parameter = [](const ParameterDecl& parameter) {
        return parameter.is_c_array && wrapper_type_name(parameter.type) == "int";
    };

    for (const auto& function_decl : module_decl.functions)
    {
        for (const auto& parameter : function_decl.parameters)
        {
            if (is_int_array_parameter(parameter))
            {
                return true;
            }
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        for (const auto& method_decl : class_decl.methods)
        {
            for (const auto& parameter : method_decl.parameters)
            {
                if (is_int_array_parameter(parameter))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool parameter_is_supported_c_array_int(const ParameterDecl& parameter)
{
    return parameter.is_c_array && wrapper_type_name(parameter.type) == "int";
}

std::optional<std::size_t> try_parse_fixed_int_array_extent_from_expression(std::string_view expression)
{
    constexpr std::string_view marker = "IntArrayToNativeFixed({value},";
    const std::size_t marker_pos = expression.find(marker);
    if (marker_pos == std::string_view::npos)
    {
        return std::nullopt;
    }

    std::size_t cursor = marker_pos + marker.size();
    while (cursor < expression.size() && std::isspace(static_cast<unsigned char>(expression[cursor])) != 0)
    {
        ++cursor;
    }

    std::size_t end = cursor;
    while (end < expression.size() && std::isdigit(static_cast<unsigned char>(expression[end])) != 0)
    {
        ++end;
    }

    if (cursor == end)
    {
        return std::nullopt;
    }

    std::size_t extent = 0;
    for (std::size_t index = cursor; index < end; ++index)
    {
        extent = extent * 10 + static_cast<std::size_t>(expression[index] - '0');
    }
    return extent;
}

std::optional<std::size_t> fixed_std_array_int_extent(const ParameterDecl& parameter)
{
    if (parameter.is_c_array)
    {
        return std::nullopt;
    }
    if (wrapper_type_name(parameter.type) != "int[]")
    {
        return std::nullopt;
    }
    if (parameter.type.pinvoke_name != "System.IntPtr")
    {
        return std::nullopt;
    }
    if (parameter.type.managed_to_pinvoke_expression.empty())
    {
        return std::nullopt;
    }

    return try_parse_fixed_int_array_extent_from_expression(parameter.type.managed_to_pinvoke_expression);
}

std::optional<std::size_t> inferred_c_array_source_index(
    const FunctionDecl& function_decl, std::size_t size_parameter_index)
{
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        if (!parameter_is_supported_c_array_int(parameter))
        {
            continue;
        }

        if (parameter.c_array_size_param_index == size_parameter_index)
        {
            return index;
        }
    }

    return std::nullopt;
}

bool parameter_is_inferred_c_array_size(const FunctionDecl& function_decl, std::size_t parameter_index)
{
    return inferred_c_array_source_index(function_decl, parameter_index).has_value();
}

std::string c_array_count_expression(const FunctionDecl& function_decl, std::size_t parameter_index)
{
    const auto& parameter = function_decl.parameters[parameter_index];
    if (parameter.c_array_size_param_index < function_decl.parameters.size())
    {
        if (parameter_is_inferred_c_array_size(function_decl, parameter.c_array_size_param_index))
        {
            return std::format("{}.Length", parameter.name);
        }
        return std::format("checked((int){})", function_decl.parameters[parameter.c_array_size_param_index].name);
    }

    return std::format("{}.Length", parameter.name);
}

std::string public_parameter_type_name(const ParameterDecl& parameter)
{
    if (parameter_is_supported_c_array_int(parameter))
    {
        return "int[]";
    }

    return wrapper_type_name(parameter.type);
}

void emit_shared_pinvoke_types_if_needed(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root, std::vector<std::filesystem::path>& generated_files)
{
    if (!module_uses_pinvoke_type(module_decl, "CsBind23StringView") && !module_uses_managed_type(module_decl, "string"))
    {
        return;
    }

    TextWriter shared(256);
    shared.append_line_format("namespace {};", csharp_namespace_name(module_decl));
    shared.append_line();
    shared.append_line("public static class CsBind23Utf8Interop");
    shared.append_line("{");
    shared.append_line("    public static System.IntPtr StringToNative(string value)");
    shared.append_line("    {");
    shared.append_line("        string text = value ?? string.Empty;");
    shared.append_line("        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(text);");
    shared.append_line("        System.IntPtr ptr = System.Runtime.InteropServices.Marshal.AllocHGlobal(bytes.Length + 1);");
    shared.append_line("        if (bytes.Length != 0)");
    shared.append_line("        {");
    shared.append_line("            System.Runtime.InteropServices.Marshal.Copy(bytes, 0, ptr, bytes.Length);");
    shared.append_line("        }");
    shared.append_line("        System.Runtime.InteropServices.Marshal.WriteByte(ptr, bytes.Length, 0);");
    shared.append_line("        return ptr;");
    shared.append_line("    }");
    shared.append_line();
    shared.append_line("    public static string NativeToString(System.IntPtr ptr)");
    shared.append_line("    {");
    shared.append_line("        if (ptr == System.IntPtr.Zero)");
    shared.append_line("        {");
    shared.append_line("            return string.Empty;");
    shared.append_line("        }");
    shared.append_line();
    shared.append_line("        int length = 0;");
    shared.append_line("        while (System.Runtime.InteropServices.Marshal.ReadByte(ptr, length) != 0)");
    shared.append_line("        {");
    shared.append_line("            length++; ");
    shared.append_line("        }");
    shared.append_line();
    shared.append_line("        if (length == 0)");
    shared.append_line("        {");
    shared.append_line("            return string.Empty;");
    shared.append_line("        }");
    shared.append_line();
    shared.append_line("        byte[] bytes = new byte[length];");
    shared.append_line("        System.Runtime.InteropServices.Marshal.Copy(ptr, bytes, 0, length);");
    shared.append_line("        return System.Text.Encoding.UTF8.GetString(bytes);");
    shared.append_line("    }");
    shared.append_line();
    shared.append_line("    public static void Free(System.IntPtr ptr)");
    shared.append_line("    {");
    shared.append_line("        if (ptr != System.IntPtr.Zero)");
    shared.append_line("        {");
    shared.append_line("            System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr);");
    shared.append_line("        }");
    shared.append_line("    }");
    shared.append_line("}");
    shared.append_line();

    shared.append_line("[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]");
    shared.append_line("public readonly struct CsBind23StringView");
    shared.append_line("{");
    shared.append_line("    public readonly System.IntPtr str;");
    shared.append_line("    public readonly nuint length;");
    shared.append_line();
    shared.append_line("    public CsBind23StringView(System.IntPtr str, nuint length)");
    shared.append_line("    {");
    shared.append_line("        this.str = str;");
    shared.append_line("        this.length = length;");
    shared.append_line("    }");
    shared.append_line("}");
    shared.append_line();
    shared.append_line("public static class CsBind23StringViewExtensions");
    shared.append_line("{");
    shared.append_line("    public static string ToManaged(CsBind23StringView view)");
    shared.append_line("    {");
    shared.append_line("        if (view.str == System.IntPtr.Zero || view.length == 0)");
    shared.append_line("        {");
    shared.append_line("            return string.Empty;");
    shared.append_line("        }");
    shared.append_line();
    shared.append_line("        int length = checked((int)view.length);");
    shared.append_line("        byte[] bytes = new byte[length];");
    shared.append_line("        System.Runtime.InteropServices.Marshal.Copy(view.str, bytes, 0, length);");
    shared.append_line("        return System.Text.Encoding.UTF8.GetString(bytes);");
    shared.append_line("    }");
    shared.append_line("}");

    generated_files.push_back(write_csharp_file(output_root, "csbind23.types.g.cs", shared.str()));
}

void emit_shared_instance_cache_types_if_needed(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root, std::vector<std::filesystem::path>& generated_files)
{
    const auto shared_path = output_root / "csbind23.instance_cache.g.cs";
    if (std::filesystem::exists(shared_path))
    {
        return;
    }

    TextWriter shared(1024);
    shared.append_line_format("namespace {};", csharp_namespace_name(module_decl));
    shared.append_line();
    shared.append_line("internal interface IInstanceCache<T>");
    shared.append_line("    where T : class");
    shared.append_line("{");
    shared.append_line("    void Register(System.IntPtr handle, T instance);");
    shared.append_line("    void Unregister(System.IntPtr handle);");
    shared.append_line("    bool TryGet(System.IntPtr handle, out T instance);");
    shared.append_line("}");
    shared.append_line();

    shared.append_line("internal sealed class DefaultInstanceCache<T> : IInstanceCache<T>");
    shared.append_line("    where T : class");
    shared.append_line("{");
    shared.append_line("    private readonly System.Threading.ReaderWriterLockSlim _lock = new System.Threading.ReaderWriterLockSlim(System.Threading.LockRecursionPolicy.NoRecursion);");
    shared.append_line("    private readonly System.Collections.Generic.Dictionary<System.IntPtr, System.WeakReference<T>> _instances =");
    shared.append_line("        new System.Collections.Generic.Dictionary<System.IntPtr, System.WeakReference<T>>();");
    shared.append_line();
    shared.append_line("    public void Register(System.IntPtr handle, T instance)");
    shared.append_line("    {");
    shared.append_line("        if (handle == System.IntPtr.Zero)");
    shared.append_line("        {");
    shared.append_line("            return;");
    shared.append_line("        }");
    shared.append_line("        _lock.EnterWriteLock();");
    shared.append_line("        try");
    shared.append_line("        {");
    shared.append_line("            _instances[handle] = new System.WeakReference<T>(instance);");
    shared.append_line("        }");
    shared.append_line("        finally");
    shared.append_line("        {");
    shared.append_line("            _lock.ExitWriteLock();");
    shared.append_line("        }");
    shared.append_line("    }");
    shared.append_line();
    shared.append_line("    public void Unregister(System.IntPtr handle)");
    shared.append_line("    {");
    shared.append_line("        if (handle == System.IntPtr.Zero)");
    shared.append_line("        {");
    shared.append_line("            return;");
    shared.append_line("        }");
    shared.append_line("        _lock.EnterWriteLock();");
    shared.append_line("        try");
    shared.append_line("        {");
    shared.append_line("            _instances.Remove(handle);");
    shared.append_line("        }");
    shared.append_line("        finally");
    shared.append_line("        {");
    shared.append_line("            _lock.ExitWriteLock();");
    shared.append_line("        }");
    shared.append_line("    }");
    shared.append_line();
    shared.append_line("    public bool TryGet(System.IntPtr handle, out T instance)");
    shared.append_line("    {");
    shared.append_line("        if (handle == System.IntPtr.Zero)");
    shared.append_line("        {");
    shared.append_line("            instance = null!;");
    shared.append_line("            return false;");
    shared.append_line("        }");
    shared.append_line("        _lock.EnterReadLock();");
    shared.append_line("        try");
    shared.append_line("        {");
    shared.append_line("            if (_instances.TryGetValue(handle, out var weak) && weak.TryGetTarget(out instance))");
    shared.append_line("            {");
    shared.append_line("                return true;");
    shared.append_line("            }");
    shared.append_line("        }");
    shared.append_line("        finally");
    shared.append_line("        {");
    shared.append_line("            _lock.ExitReadLock();");
    shared.append_line("        }");
    shared.append_line("        instance = null!;");
    shared.append_line("        return false;");
    shared.append_line("    }");
    shared.append_line("}");

    generated_files.push_back(write_csharp_file(output_root, "csbind23.instance_cache.g.cs", shared.str()));
}

void emit_shared_array_interop_types_if_needed(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root, std::vector<std::filesystem::path>& generated_files)
{
    if (!module_uses_managed_type(module_decl, "int[]") && !module_uses_c_array_int_parameters(module_decl))
    {
        return;
    }

    const auto shared_path = output_root / "csbind23.array.g.cs";
    if (std::filesystem::exists(shared_path))
    {
        return;
    }

    TextWriter shared(1024);
    shared.append_line_format("namespace {};", csharp_namespace_name(module_decl));
    shared.append_line();
    shared.append(R"(public static class CsBind23ArrayInterop
    {
        public static System.IntPtr IntArrayToNativeFixed(int[] value, int expectedLength)
        {
            if (value == null || value.Length != expectedLength)
            {
                throw new System.ArgumentException($"Expected array length {expectedLength}", nameof(value));
            }

            System.IntPtr ptr = System.Runtime.InteropServices.Marshal.AllocHGlobal(sizeof(int) * expectedLength);
            System.Runtime.InteropServices.Marshal.Copy(value, 0, ptr, expectedLength);
            return ptr;
        }

        public static System.IntPtr IntArrayToNativeCounted(int[] value, int count, string paramName)
        {
            if (value == null)
            {
                throw new System.ArgumentNullException(paramName);
            }
            if (count < 0 || count > value.Length)
            {
                throw new System.ArgumentOutOfRangeException(paramName, $"Count {count} must be between 0 and array length {value.Length}.");
            }
            if (count == 0)
            {
                return System.IntPtr.Zero;
            }

            System.IntPtr ptr = System.Runtime.InteropServices.Marshal.AllocHGlobal(sizeof(int) * count);
            System.Runtime.InteropServices.Marshal.Copy(value, 0, ptr, count);
            return ptr;
        }

        public static int[] NativeToNewIntArrayFixed(System.IntPtr ptr, int expectedLength)
        {
            int[] value = new int[expectedLength];
            if (ptr != System.IntPtr.Zero)
            {
                System.Runtime.InteropServices.Marshal.Copy(ptr, value, 0, expectedLength);
            }
            return value;
        }

        public static void NativeToExistingIntArrayFixed(System.IntPtr ptr, int[] target, int expectedLength)
        {
            if (ptr == System.IntPtr.Zero)
            {
                return;
            }
            if (target == null || target.Length != expectedLength)
            {
                throw new System.ArgumentException($"Expected array length {expectedLength}", nameof(target));
            }
            System.Runtime.InteropServices.Marshal.Copy(ptr, target, 0, expectedLength);
        }

        public static void NativeToExistingIntArrayCounted(System.IntPtr ptr, int[] target, int count, string paramName)
        {
            if (target == null)
            {
                throw new System.ArgumentNullException(paramName);
            }
            if (count < 0 || count > target.Length)
            {
                throw new System.ArgumentOutOfRangeException(paramName, $"Count {count} must be between 0 and array length {target.Length}.");
            }
            if (ptr == System.IntPtr.Zero || count == 0)
            {
                return;
            }
            System.Runtime.InteropServices.Marshal.Copy(ptr, target, 0, count);
        }

        public static void FreeNativeBuffer(System.IntPtr ptr)
        {
            if (ptr != System.IntPtr.Zero)
            {
                System.Runtime.InteropServices.Marshal.FreeHGlobal(ptr);
            }
        }
    }
)" );

    generated_files.push_back(write_csharp_file(output_root, "csbind23.array.g.cs", shared.str()));
}

void emit_shared_item_ownership_type_if_needed(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root, std::vector<std::filesystem::path>& generated_files)
{
    const auto shared_path = output_root / "csbind23.ownership.g.cs";
    if (std::filesystem::exists(shared_path))
    {
        return;
    }

    TextWriter shared(128);
    shared.append_line_format("namespace {};", csharp_namespace_name(module_decl));
    shared.append_line();
    shared.append_line("public enum ItemOwnership");
    shared.append_line("{");
    shared.append_line("    Borrowed = 0,");
    shared.append_line("    Owned = 1,");
    shared.append_line("}");

    generated_files.push_back(write_csharp_file(output_root, "csbind23.ownership.g.cs", shared.str()));
}

std::string pinvoke_return_type(const FunctionDecl& function_decl)
{
    if (function_decl.is_constructor)
    {
        return "System.IntPtr";
    }
    return function_decl.return_type.pinvoke_name;
}

std::string wrapper_type_name(const TypeRef& type_ref)
{
    if (type_ref.has_managed_converter() && !type_ref.managed_type_name.empty())
    {
        return type_ref.managed_type_name;
    }
    return type_ref.pinvoke_name;
}

bool parameter_is_ref(const ParameterDecl& parameter)
{
    if (parameter.is_c_array)
    {
        return false;
    }

    if (is_cpp_std_string(parameter.type.cpp_name) && parameter.type.has_managed_converter()
        && parameter.type.managed_type_name != "string")
    {
        return false;
    }

    if (parameter.type.is_reference && !parameter.type.is_const)
    {
        return true;
    }

    if (parameter.type.is_pointer && !parameter.type.is_const && !is_pinvoke_int_ptr(parameter.type.pinvoke_name))
    {
        return true;
    }

    if (parameter.type.has_managed_converter() && parameter.type.is_pointer && !parameter.type.is_const
        && is_cpp_std_string(parameter.type.cpp_name))
    {
        return true;
    }

    return false;
}

bool parameter_is_direct_ref(const ParameterDecl& parameter)
{
    return parameter_is_ref(parameter) && parameter.type.managed_to_pinvoke_expression.empty();
}

bool parameter_is_direct_out(const ParameterDecl& parameter)
{
    return parameter.is_output && parameter_is_direct_ref(parameter);
}

std::string parameter_direct_byref_keyword(const ParameterDecl& parameter)
{
    if (!parameter_is_direct_ref(parameter))
    {
        return {};
    }

    return parameter_is_direct_out(parameter) ? "out " : "ref ";
}

std::string parameter_signature_byref_keyword(const ParameterDecl& parameter)
{
    if (!parameter_is_ref(parameter))
    {
        return {};
    }

    return parameter_is_direct_out(parameter) ? "out " : "ref ";
}

std::string parameter_call_argument(const ParameterDecl& parameter)
{
    return parameter_direct_byref_keyword(parameter) + parameter.name;
}

std::string parameter_call_argument(const ParameterDecl& parameter, std::string_view argument_expression)
{
    return parameter_direct_byref_keyword(parameter) + std::string(argument_expression);
}

std::string reflection_parameter_type_expression(const ParameterDecl& parameter)
{
    std::string type_expression = std::format("typeof({})", wrapper_type_name(parameter.type));
    if (parameter_is_ref(parameter))
    {
        type_expression += ".MakeByRefType()";
    }
    return type_expression;
}

const ClassDecl* primary_base_class(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    if (!class_decl.base_classes.empty())
    {
        return find_class_by_cpp_name(module_decl, class_decl.base_classes.front().cpp_name);
    }

    if (!class_decl.base_cpp_name.empty())
    {
        return find_class_by_cpp_name(module_decl, class_decl.base_cpp_name);
    }

    return nullptr;
}

MethodCollectionOptions csharp_method_collection_options()
{
    MethodCollectionOptions options;
    options.exclude_pinvoke_only = true;
    options.signature_kind = MethodSignatureKind::PInvoke;
    return options;
}

struct GenericClassGroup
{
    std::string name;
    std::vector<const ClassDecl*> instantiations;
};

std::vector<GenericFunctionGroup> collect_generic_function_groups(const std::vector<FunctionDecl>& functions)
{
    std::vector<GenericFunctionGroup> groups;
    std::unordered_map<std::string, std::size_t> by_name;
    for (const auto& function_decl : functions)
    {
        if (!function_decl.is_generic_instantiation || function_decl.generic_group_name.empty()
            || function_decl.pinvoke_only)
        {
            continue;
        }

        const std::string& name = function_decl.generic_group_name;
        auto it = by_name.find(name);
        if (it == by_name.end())
        {
            by_name.emplace(name, groups.size());
            groups.push_back(GenericFunctionGroup{name, {&function_decl}});
        }
        else
        {
            groups[it->second].instantiations.push_back(&function_decl);
        }
    }
    return groups;
}

std::vector<GenericFunctionGroup> collect_generic_method_groups(const std::vector<EmittedMethod>& methods)
{
    std::vector<GenericFunctionGroup> groups;
    std::unordered_map<std::string, std::size_t> by_name;
    for (const auto& emitted_method : methods)
    {
        const auto& method_decl = emitted_method.method;
        if (!method_decl.is_generic_instantiation || method_decl.generic_group_name.empty()
            || method_decl.pinvoke_only)
        {
            continue;
        }

        const std::string& name = method_decl.generic_group_name;
        auto it = by_name.find(name);
        if (it == by_name.end())
        {
            by_name.emplace(name, groups.size());
            groups.push_back(GenericFunctionGroup{name, {&method_decl}});
        }
        else
        {
            groups[it->second].instantiations.push_back(&method_decl);
        }
    }
    return groups;
}

bool class_has_owned_ctor(const ClassDecl& class_decl);

std::vector<GenericClassGroup> collect_generic_class_groups(const std::vector<ClassDecl>& classes)
{
    std::vector<GenericClassGroup> groups;
    std::unordered_map<std::string, std::size_t> by_name;
    for (const auto& class_decl : classes)
    {
        if (!class_decl.is_generic_instantiation || class_decl.generic_group_name.empty())
        {
            continue;
        }

        const std::string& name = class_decl.generic_group_name;
        auto it = by_name.find(name);
        if (it == by_name.end())
        {
            by_name.emplace(name, groups.size());
            groups.push_back(GenericClassGroup{name, {&class_decl}});
        }
        else
        {
            groups[it->second].instantiations.push_back(&class_decl);
        }
    }

    return groups;
}

const ClassDecl* find_pointer_class_return(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    const auto& return_type = function_decl.return_type;
    if (!return_type.is_pointer && !return_type.is_reference)
    {
        return nullptr;
    }

    if (return_type.has_managed_converter())
    {
        return nullptr;
    }

    return find_class_by_cpp_name(module_decl, return_type.cpp_name);
}

std::string wrapper_return_type(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    if (is_cabi_void(function_decl.return_type.c_abi_name))
    {
        return "void";
    }

    if (find_pointer_class_return(module_decl, function_decl) != nullptr)
    {
        return "object";
    }

    return wrapper_type_name(function_decl.return_type);
}

std::string default_variant_condition(std::size_t omitted, const std::vector<std::string>& has_value_expressions)
{
    const std::size_t optional_count = has_value_expressions.size();
    std::string condition;
    for (std::size_t index = 0; index < optional_count; ++index)
    {
        const bool should_be_present = index < (optional_count - omitted);
        if (!condition.empty())
        {
            condition += " && ";
        }
        if (should_be_present)
        {
            condition += has_value_expressions[index];
        }
        else
        {
            condition += "!(" + has_value_expressions[index] + ")";
        }
    }

    return condition.empty() ? "true" : condition;
}

void append_default_variant_if_chain(TextWriter& output, std::string_view indent, const std::string& module_name,
    const std::string& native_name, const std::vector<std::string>& call_arguments, std::size_t defaults,
    const std::vector<std::string>& has_value_expressions, std::string_view assign_target, bool emit_return)
{
    for (std::size_t omitted = 0; omitted <= defaults; ++omitted)
    {
        const std::size_t count = call_arguments.size() - omitted;
        std::vector<std::string> prefix_args(call_arguments.begin(), call_arguments.begin() + count);
        const std::string call_arguments_rendered = join_arguments(prefix_args);
        const std::string variant_name =
            omitted == 0 ? native_name : native_name + std::format("__default_{}", omitted);
        const std::string variant_call = std::format("{}Native.{}({})", module_name, variant_name, call_arguments_rendered);
        const std::string condition = default_variant_condition(omitted, has_value_expressions);

        if (omitted == 0)
        {
            output.append_line_format("{}if ({})", indent, condition);
        }
        else
        {
            output.append_line_format("{}else if ({})", indent, condition);
        }
        output.append_line_format("{}{{", indent);

        if (emit_return)
        {
            output.append_line_format("{}    return {} ;", indent, variant_call);
        }
        else if (!assign_target.empty())
        {
            output.append_line_format("{}    {} = {} ;", indent, assign_target, variant_call);
        }
        else
        {
            output.append_line_format("{}    {} ;", indent, variant_call);
        }

        output.append_line_format("{}}}", indent);
    }

    output.append_line_format("{}else", indent);
    output.append_line_format("{}{{", indent);
    output.append_line_format(
        "{}    throw new System.ArgumentException(\"Optional arguments must be omitted from right to left.\");", indent);
    output.append_line_format("{}}}", indent);
}

std::string csharp_api_class_name(const ModuleDecl& module_decl)
{
    const std::string default_name = !module_decl.csharp_api_class.empty()
        ? module_decl.csharp_api_class
        : module_decl.name + "Api";
    return format_csharp_name(module_decl, CSharpNameKind::Class, default_name);
}

std::string csharp_namespace_name(const ModuleDecl& module_decl)
{
    if (!module_decl.csharp_namespace.empty())
    {
        return module_decl.csharp_namespace;
    }
    return "CsBind23.Generated";
}

std::string csharp_namespace_name(const ModuleDecl& module_decl, std::string_view relative_namespace)
{
    const std::string base_namespace = csharp_namespace_name(module_decl);
    if (relative_namespace.empty())
    {
        return base_namespace;
    }

    return std::format("{}.{}", base_namespace, relative_namespace);
}

std::string csharp_namespace_name(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    return csharp_namespace_name(module_decl, class_decl.csharp_namespace);
}

std::string item_ownership_type_name(const ModuleDecl& module_decl)
{
    return std::format("global::{}.ItemOwnership", csharp_namespace_name(module_decl));
}

std::string item_ownership_literal(const ModuleDecl& module_decl, bool owned)
{
    return item_ownership_type_name(module_decl) + (owned ? ".Owned" : ".Borrowed");
}

std::string resolved_instance_cache_type(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    const std::string configured = !class_decl.instance_cache_type.empty()
        ? class_decl.instance_cache_type
        : (!module_decl.instance_cache_type.empty() ? module_decl.instance_cache_type : "DefaultInstanceCache<T>");
    const std::string managed_class = format_csharp_name(module_decl, CSharpNameKind::Class, class_decl.name);
    return replace_all(configured, "<T>", std::format("<{}>", managed_class));
}

std::string managed_name(const ModuleDecl& module_decl, CSharpNameKind kind, std::string_view raw_name)
{
    return format_csharp_name(module_decl, kind, raw_name);
}

std::string managed_class_name(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    return managed_name(module_decl, CSharpNameKind::Class, class_decl.name);
}

std::string qualified_managed_class_name(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    return "global::" + csharp_namespace_name(module_decl, class_decl) + "." + managed_class_name(module_decl, class_decl);
}

std::string managed_function_name(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    return managed_name(module_decl, CSharpNameKind::Function, function_decl.name);
}

std::string managed_method_name(const ModuleDecl& module_decl, const FunctionDecl& method_decl)
{
    return managed_name(module_decl, CSharpNameKind::Method, method_decl.name);
}

std::string managed_property_name(const ModuleDecl& module_decl, const PropertyDecl& property_decl)
{
    return managed_name(module_decl,
        property_decl.is_field_projection ? CSharpNameKind::MemberVar : CSharpNameKind::Property,
        property_decl.name);
}

std::string managed_enum_name(const ModuleDecl& module_decl, const EnumDecl& enum_decl)
{
    return managed_name(module_decl, CSharpNameKind::Class, enum_decl.name);
}

std::string managed_enum_value_name(const ModuleDecl& module_decl, const EnumValueDecl& value_decl)
{
    return managed_name(module_decl, CSharpNameKind::MemberVar, value_decl.name);
}

void append_csharp_comment(TextWriter& output, std::string_view indent, std::string_view comment)
{
    const std::size_t first = comment.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
    {
        return;
    }

    const std::size_t last = comment.find_last_not_of(" \t\r\n");
    std::string normalized(comment.substr(first, last - first + 1));
    std::string current_line;
    for (char ch : normalized)
    {
        if (ch == '\r')
        {
            continue;
        }
        if (ch == '\n')
        {
            output.append_line_format("{}// {}", indent, current_line);
            current_line.clear();
            continue;
        }
        current_line.push_back(ch);
    }
    output.append_line_format("{}// {}", indent, current_line);
}

std::string csharp_enum_underlying_type(const EnumDecl& enum_decl)
{
    const auto& type_name = enum_decl.underlying_type.pinvoke_name;
    if (is_csharp_enum_integral_underlying(type_name))
    {
        return type_name;
    }

    const auto& cpp_name = enum_decl.underlying_type.cpp_name;
    const auto& c_abi_name = enum_decl.underlying_type.c_abi_name;

    if (is_pinvoke_pointer_sized_integral(type_name))
    {
        return normalize_enum_underlying_integral(type_name, cpp_name, c_abi_name);
    }

    if (const auto mapped = pinvoke_integral_type_from_cabi(c_abi_name); mapped.has_value())
    {
        return normalize_enum_underlying_integral(*mapped, cpp_name, c_abi_name);
    }

    return "int";
}

std::string normalize_csharp_attribute(std::string_view attribute)
{
    const std::size_t first = attribute.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
    {
        return {};
    }
    const std::size_t last = attribute.find_last_not_of(" \t\r\n");
    const std::string_view trimmed = attribute.substr(first, last - first + 1);
    if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']')
    {
        return std::string(trimmed);
    }
    return std::format("[{}]", trimmed);
}

void append_csharp_attributes(
    TextWriter& output, std::string_view indent, const std::vector<std::string>& csharp_attributes)
{
    for (const auto& attribute : csharp_attributes)
    {
        const std::string rendered = normalize_csharp_attribute(attribute);
        if (!rendered.empty())
        {
            output.append_line_format("{}{}", indent, rendered);
        }
    }
}

std::string csharp_enum_value_literal(const EnumDecl& enum_decl, const EnumValueDecl& value_decl)
{
    const std::string underlying = csharp_enum_underlying_type(enum_decl);
    if (value_decl.is_signed)
    {
        return std::format("unchecked(({}){}L)", underlying, static_cast<std::int64_t>(value_decl.value));
    }

    return std::format("unchecked(({}){}UL)", underlying, value_decl.value);
}

std::string parameter_list_without_self(const FunctionDecl& function_decl)
{
    std::string rendered;
    bool first = true;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        if (parameter_is_inferred_c_array_size(function_decl, index))
        {
            continue;
        }

        const auto& parameter = function_decl.parameters[index];
        if (!first)
        {
            rendered += ", ";
        }
        rendered += std::format("{}{} {}", parameter_signature_byref_keyword(parameter), public_parameter_type_name(parameter),
            parameter.name);
        first = false;
    }
    return rendered;
}

bool csharp_type_is_value_type(const ModuleDecl& module_decl, std::string_view type_name)
{
    if (is_csharp_builtin_value_type(type_name))
    {
        return true;
    }

    for (const auto& enum_decl : module_decl.enums)
    {
        if (enum_decl.name == type_name)
        {
            return true;
        }
    }

    return false;
}

bool parameter_can_be_nullable_optional(const ParameterDecl& parameter)
{
    return !parameter_is_ref(parameter) && parameter.type.managed_to_pinvoke_expression.empty()
        && parameter.type.managed_finalize_to_pinvoke_statement.empty();
}

std::size_t free_function_default_count(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    (void)module_decl;
    const std::size_t declared = function_decl.trailing_default_argument_count > function_decl.parameters.size()
        ? function_decl.parameters.size()
        : function_decl.trailing_default_argument_count;

    std::size_t effective = 0;
    for (std::size_t offset = 0; offset < declared; ++offset)
    {
        const auto& parameter = function_decl.parameters[function_decl.parameters.size() - 1 - offset];
        if (!parameter_can_be_nullable_optional(parameter))
        {
            break;
        }
        ++effective;
    }

    return effective;
}

bool is_nullable_optional_parameter(const ModuleDecl& module_decl, const FunctionDecl& function_decl, std::size_t index)
{
    const std::size_t defaults = free_function_default_count(module_decl, function_decl);
    if (defaults == 0 || index >= function_decl.parameters.size())
    {
        return false;
    }

    return index >= (function_decl.parameters.size() - defaults);
}

std::string parameter_type_for_public_signature(
    const ModuleDecl& module_decl, const FunctionDecl& function_decl, const ParameterDecl& parameter, std::size_t index)
{
    std::string type_name = public_parameter_type_name(parameter);
    if (!is_nullable_optional_parameter(module_decl, function_decl, index))
    {
        return type_name;
    }

    type_name += "?";
    return type_name;
}

std::string free_function_parameter_list(const ModuleDecl& module_decl, const FunctionDecl& function_decl)
{
    std::string rendered;
    bool first = true;
    for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
    {
        if (parameter_is_inferred_c_array_size(function_decl, index))
        {
            continue;
        }

        const auto& parameter = function_decl.parameters[index];
        if (!first)
        {
            rendered += ", ";
        }
        rendered += std::format("{}{} {}", parameter_signature_byref_keyword(parameter),
            parameter_type_for_public_signature(module_decl, function_decl, parameter, index), parameter.name);
        first = false;
    }
    return rendered;
}

std::string nullable_parameter_unwrap_expression(
    const ModuleDecl& module_decl, const FunctionDecl& function_decl, const ParameterDecl& parameter, std::size_t index)
{
    if (!is_nullable_optional_parameter(module_decl, function_decl, index))
    {
        return parameter.name;
    }

    const std::string wrapper_type = wrapper_type_name(parameter.type);
    if (csharp_type_is_value_type(module_decl, wrapper_type))
    {
        return parameter.name + ".Value";
    }

    return parameter.name + "!";
}

std::string optional_parameter_has_value_expression(
    const ModuleDecl& module_decl, const FunctionDecl& function_decl, const ParameterDecl& parameter, std::size_t index)
{
    if (!is_nullable_optional_parameter(module_decl, function_decl, index))
    {
        return "true";
    }

    const std::string wrapper_type = wrapper_type_name(parameter.type);
    if (csharp_type_is_value_type(module_decl, wrapper_type))
    {
        return parameter.name + ".HasValue";
    }

    return parameter.name + " != null";
}

void append_free_generic_dispatch_wrapper(TextWriter& output, const ModuleDecl& module_decl, const GenericFunctionGroup& group)
{
    const auto shape = analyze_generic_dispatch_shape(group);
    if (shape.generic_arity == 0)
    {
        return;
    }

    const FunctionDecl& first = *group.instantiations.front();
    const std::string return_type = shape.generic_return_slot_id >= 0
        ? generic_type_parameter_name(static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity)
        : wrapper_return_type(module_decl, first);
    const std::string generic_parameter_list = generic_type_parameter_list(shape.generic_arity);

    output.append_format("    public static {} {}<{}>(", return_type,
        managed_name(module_decl, CSharpNameKind::Function, group.name), generic_parameter_list);
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = shape.generic_parameter_slot_ids[index];
        const std::string parameter_type = slot_id >= 0
            ? generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    const std::string diagnostics_message =
        csharp_string_literal_escape(
            generic_dispatch_not_supported_message(module_decl, CSharpNameKind::Function, group, shape));
    if (!shape.supported)
    {
        output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
        output.append_line("    }");
        output.append_line();
        return;
    }

    for (const auto* instantiation : group.instantiations)
    {
        std::string condition;
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            if (!condition.empty())
            {
                condition += " && ";
            }

            const std::string generic_name = generic_type_parameter_name(slot, shape.generic_arity);
            const std::string concrete_name =
                concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
            condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
        }

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = shape.generic_parameter_slot_ids[index];
            if (slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);

                    if (!parameter.type.managed_to_pinvoke_expression.empty())
                    {
                        const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                        const std::string converted = render_inline_template(
                            parameter.type.managed_to_pinvoke_expression,
                            typed_name,
                            pinvoke_name,
                            typed_name,
                            module_decl.name);
                        append_embedded_assignment(
                            output,
                            "            ",
                            std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                            converted);
                        call_arguments.push_back(pinvoke_name);

                        if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                        {
                            finalize_statements.push_back(render_inline_template(
                                parameter.type.managed_finalize_to_pinvoke_statement,
                                typed_name,
                                pinvoke_name,
                                pinvoke_name,
                                module_decl.name));
                        }
                    }
                    else
                    {
                        call_arguments.push_back(typed_name);
                    }
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call =
            std::format("{}({})", managed_method_name(module_decl, *instantiation), join_arguments(call_arguments));
        if (return_type == "void")
        {
            output.append_line_format("            {} ;", call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line("            return;");
        }
        else if (shape.generic_return_slot_id >= 0)
        {
            const std::string concrete_name = wrapper_type_name(instantiation->return_type);
            const std::string generic_name = generic_type_parameter_name(
                static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity);
            output.append_line_format("            {} __csbind23_typed_result = {} ;", concrete_name, call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format(
                "            return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                concrete_name,
                generic_name);
        }
        else
        {
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format("            return {};", call);
        }
        output.append_line("        }");
    }

    output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
    output.append_line("    }");
    output.append_line();
}

void append_method_generic_dispatch_wrapper(TextWriter& output, const ModuleDecl& module_decl, const GenericFunctionGroup& group)
{
    const auto shape = analyze_generic_dispatch_shape(group);
    if (shape.generic_arity == 0)
    {
        return;
    }

    const FunctionDecl& first = *group.instantiations.front();
    const std::string return_type = shape.generic_return_slot_id >= 0
        ? generic_type_parameter_name(static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity)
        : wrapper_return_type(module_decl, first);
    const std::string generic_parameter_list = generic_type_parameter_list(shape.generic_arity);

    output.append_format("    public {} {}<{}>(", return_type,
        managed_name(module_decl, CSharpNameKind::Method, group.name), generic_parameter_list);
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = shape.generic_parameter_slot_ids[index];
        const std::string parameter_type = slot_id >= 0
            ? generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    const std::string diagnostics_message =
        csharp_string_literal_escape(
            generic_dispatch_not_supported_message(module_decl, CSharpNameKind::Method, group, shape));
    if (!shape.supported)
    {
        output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
        output.append_line("    }");
        output.append_line();
        return;
    }

    for (const auto* instantiation : group.instantiations)
    {
        std::string condition;
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            if (!condition.empty())
            {
                condition += " && ";
            }

            const std::string generic_name = generic_type_parameter_name(slot, shape.generic_arity);
            const std::string concrete_name =
                concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
            condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
        }

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = shape.generic_parameter_slot_ids[index];
            if (slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_type_parameter_name(static_cast<std::size_t>(slot_id), shape.generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);
                    call_arguments.push_back(typed_name);
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call =
            std::format("{}({})", managed_method_name(module_decl, *instantiation), join_arguments(call_arguments));
        if (return_type == "void")
        {
            output.append_line_format("            {} ;", call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line("            return;");
        }
        else if (shape.generic_return_slot_id >= 0)
        {
            const std::string concrete_name = wrapper_type_name(instantiation->return_type);
            const std::string generic_name = generic_type_parameter_name(
                static_cast<std::size_t>(shape.generic_return_slot_id), shape.generic_arity);
            output.append_line_format("            {} __csbind23_typed_result = {} ;", concrete_name, call);
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format(
                "            return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                concrete_name,
                generic_name);
        }
        else
        {
            for (const auto& post_assignment : post_assignments)
            {
                output.append_line_format("            {}", post_assignment);
            }
            output.append_line_format("            return {};", call);
        }
        output.append_line("        }");
    }

    output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
    output.append_line("    }");
    output.append_line();
}

std::string generic_class_member_shape_key(const FunctionDecl& method_decl)
{
    std::string key = method_decl.name;
    key += method_decl.is_const ? "|const|" : "|mutable|";
    key += std::format("{}|", method_decl.parameters.size());
    for (const auto& parameter : method_decl.parameters)
    {
        key += parameter.is_output ? "#out" : "#in";
        key += ";";
    }
    return key;
}

std::vector<const FunctionDecl*> collect_declared_constructors(const ClassDecl& class_decl)
{
    std::vector<const FunctionDecl*> constructors;
    constructors.reserve(class_decl.methods.size());
    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor)
        {
            constructors.push_back(&method_decl);
        }
    }
    return constructors;
}

std::vector<GenericFunctionGroup> collect_generic_class_ctor_groups(const GenericClassGroup& class_group)
{
    std::vector<GenericFunctionGroup> groups;
    if (class_group.instantiations.empty())
    {
        return groups;
    }

    const auto first_ctors = collect_declared_constructors(*class_group.instantiations.front());
    for (const auto* first_ctor : first_ctors)
    {
        const std::string key = generic_class_member_shape_key(*first_ctor);
        GenericFunctionGroup ctor_group;
        ctor_group.name = class_group.name;
        ctor_group.instantiations.push_back(first_ctor);

        bool complete = true;
        for (std::size_t class_index = 1; class_index < class_group.instantiations.size(); ++class_index)
        {
            const auto* class_decl = class_group.instantiations[class_index];
            const FunctionDecl* matched_ctor = nullptr;
            for (const auto& candidate : class_decl->methods)
            {
                if (!candidate.is_constructor)
                {
                    continue;
                }
                if (generic_class_member_shape_key(candidate) == key)
                {
                    matched_ctor = &candidate;
                    break;
                }
            }

            if (matched_ctor == nullptr)
            {
                complete = false;
                break;
            }

            ctor_group.instantiations.push_back(matched_ctor);
        }

        if (complete)
        {
            groups.push_back(std::move(ctor_group));
        }
    }

    return groups;
}

std::vector<GenericFunctionGroup> collect_generic_class_method_groups(const GenericClassGroup& class_group)
{
    std::vector<GenericFunctionGroup> groups;
    if (class_group.instantiations.empty())
    {
        return groups;
    }

    const auto* first_class = class_group.instantiations.front();
    for (const auto& first_method : first_class->methods)
    {
        if (!is_wrapper_visible_method(first_method, true))
        {
            continue;
        }

        const std::string key = generic_class_member_shape_key(first_method);
        GenericFunctionGroup method_group;
        method_group.name = first_method.name;
        method_group.instantiations.push_back(&first_method);

        bool complete = true;
        for (std::size_t class_index = 1; class_index < class_group.instantiations.size(); ++class_index)
        {
            const auto* class_decl = class_group.instantiations[class_index];
            const FunctionDecl* matched_method = nullptr;
            for (const auto& candidate : class_decl->methods)
            {
                if (!is_wrapper_visible_method(candidate, true))
                {
                    continue;
                }

                if (generic_class_member_shape_key(candidate) == key)
                {
                    matched_method = &candidate;
                    break;
                }
            }

            if (matched_method == nullptr)
            {
                complete = false;
                break;
            }

            method_group.instantiations.push_back(matched_method);
        }

        if (complete)
        {
            groups.push_back(std::move(method_group));
        }
    }

    return groups;
}

std::string generic_class_type_parameter_name(std::size_t slot, std::size_t class_generic_arity)
{
    return generic_type_parameter_name(slot, class_generic_arity);
}

const TypeRef* concrete_type_ref_for_slot(
    const FunctionDecl& instantiation, const GenericDispatchShape& shape, int slot_id)
{
    for (std::size_t index = 0; index < instantiation.parameters.size(); ++index)
    {
        if (shape.generic_parameter_slot_ids[index] == slot_id)
        {
            return &instantiation.parameters[index].type;
        }
    }

    if (shape.generic_return_slot_id == slot_id)
    {
        return &instantiation.return_type;
    }

    return nullptr;
}

std::string generic_class_slot_tuple_key(const std::vector<std::string>& slot_types)
{
    std::string key;
    for (const auto& slot_type : slot_types)
    {
        key += slot_type;
        key += '|';
    }
    return key;
}

std::vector<bool> collect_generic_class_tuple_ambiguity(const std::vector<std::vector<std::string>>& class_slot_types)
{
    std::unordered_map<std::string, std::size_t> counts;
    for (const auto& slot_types : class_slot_types)
    {
        ++counts[generic_class_slot_tuple_key(slot_types)];
    }

    std::vector<bool> ambiguous(class_slot_types.size(), false);
    for (std::size_t index = 0; index < class_slot_types.size(); ++index)
    {
        ambiguous[index] = counts[generic_class_slot_tuple_key(class_slot_types[index])] > 1;
    }
    return ambiguous;
}

std::vector<bool> collect_generic_class_borrowed_item_modes(
    const GenericClassGroup& class_group, const std::vector<GenericFunctionGroup>& ctor_groups,
    const std::vector<GenericFunctionGroup>& method_groups, std::size_t class_generic_arity)
{
    std::vector<bool> borrowed_modes(class_group.instantiations.size(), false);

    auto collect_from_groups = [&borrowed_modes, class_generic_arity](const auto& groups) {
        for (const auto& group : groups)
        {
            const auto shape = analyze_generic_dispatch_shape(group);
            if (!shape.supported)
            {
                continue;
            }

            for (std::size_t inst_index = 0; inst_index < group.instantiations.size(); ++inst_index)
            {
                const auto* instantiation = group.instantiations[inst_index];
                for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
                {
                    const auto* type_ref = concrete_type_ref_for_slot(*instantiation, shape, static_cast<int>(slot));
                    if (type_ref != nullptr && type_ref->is_pointer)
                    {
                        borrowed_modes[inst_index] = true;
                    }
                }
            }
        }
    };

    collect_from_groups(method_groups);
    collect_from_groups(ctor_groups);
    return borrowed_modes;
}

std::string generic_class_instantiation_condition(const ModuleDecl& module_decl, std::size_t inst_index,
    std::size_t class_generic_arity, const std::vector<std::vector<std::string>>& class_slot_types,
    const std::vector<bool>& ambiguous_tuples, const std::vector<bool>& borrowed_item_modes,
    std::string_view item_ownership_expression)
{
    std::string condition;
    for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
    {
        const std::string generic_name = generic_class_type_parameter_name(slot, class_generic_arity);
        const std::string concrete_name = class_slot_types[inst_index][slot];
        if (concrete_name.empty())
        {
            continue;
        }

        if (!condition.empty())
        {
            condition += " && ";
        }
        condition += std::format("typeof({}) == typeof({})", generic_name, concrete_name);
    }

    if (ambiguous_tuples[inst_index])
    {
        if (!condition.empty())
        {
            condition += " && ";
        }
        condition += std::format("{} == {}", item_ownership_expression,
            item_ownership_literal(module_decl, !borrowed_item_modes[inst_index]));
    }

    return condition.empty() ? "true" : condition;
}

void append_generic_class_constructor(TextWriter& output, const GenericClassGroup& class_group,
    const ModuleDecl& module_decl, const GenericFunctionGroup& ctor_group, std::size_t class_generic_arity,
    const std::vector<std::vector<std::string>>& class_slot_types, const std::vector<bool>& ambiguous_tuples,
    const std::vector<bool>& borrowed_item_modes)
{
    const auto shape = analyze_generic_dispatch_shape(ctor_group);
    const bool use_shape_for_parameters = shape.supported;
    const FunctionDecl& first = *ctor_group.instantiations.front();
    output.append_format("    public {}(", managed_name(module_decl, CSharpNameKind::Class, class_group.name));
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
        const std::string parameter_type = (use_shape_for_parameters && slot_id >= 0)
            ? generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    if (!first.parameters.empty())
    {
        output.append(", ");
    }
    output.append_format("{} itemOwnership = {}", item_ownership_type_name(module_decl), item_ownership_literal(module_decl, true));
    output.append_line(")");
    output.append_line("    {");

    for (std::size_t inst_index = 0; inst_index < ctor_group.instantiations.size(); ++inst_index)
    {
        const auto* instantiation = ctor_group.instantiations[inst_index];
        const auto* class_decl = class_group.instantiations[inst_index];
        const std::string condition = generic_class_instantiation_condition(
            module_decl, inst_index, class_generic_arity, class_slot_types, ambiguous_tuples, borrowed_item_modes,
            "itemOwnership");

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
            if (use_shape_for_parameters && slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);
                    call_arguments.push_back(typed_name);
                }
            }
            else if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                const std::string converted = render_inline_template(
                    parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                    module_decl.name);
                append_embedded_assignment(
                    output,
                    "            ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    converted);
                call_arguments.push_back(pinvoke_name);

                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call_arguments_rendered = join_arguments(call_arguments);
        const std::string native_call = call_arguments_rendered.empty()
            ? std::format("{}Native.{}_{}_create()", module_decl.name, module_decl.name, class_decl->name)
            : std::format("{}Native.{}_{}_create({})", module_decl.name, module_decl.name, class_decl->name,
                call_arguments_rendered);

        if (finalize_statements.empty())
        {
            output.append_line_format("            _cPtr = new System.Runtime.InteropServices.HandleRef(this, {}) ;", native_call);
        }
        else
        {
            output.append_line("            try");
            output.append_line("            {");
            output.append_line_format("                _cPtr = new System.Runtime.InteropServices.HandleRef(this, {}) ;", native_call);
            output.append_line("            }");
            output.append_line("            finally");
            output.append_line("            {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "                ", finalize_statement);
            }
            output.append_line("            }");
        }

        output.append_line_format(
            "            _ownership = {} ;", item_ownership_literal(module_decl, class_has_owned_ctor(*class_decl)));
        output.append_line("            _itemOwnership = itemOwnership ;");
        output.append_line("            return;");
        output.append_line("        }");
    }

    output.append_line_format(
        "        throw new System.NotSupportedException(\"No generic mapping for {} with provided generic type arguments.\");",
        managed_name(module_decl, CSharpNameKind::Class, class_group.name));
    output.append_line("    }");
    output.append_line();
}

std::vector<std::vector<std::string>> collect_generic_class_slot_types(
    const GenericClassGroup& class_group, const std::vector<GenericFunctionGroup>& ctor_groups,
    const std::vector<GenericFunctionGroup>& method_groups,
    std::size_t class_generic_arity)
{
    std::vector<std::vector<std::string>> slot_types(
        class_group.instantiations.size(), std::vector<std::string>(class_generic_arity));

    for (const auto& method_group : method_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(method_group);
        if (!shape.supported)
        {
            continue;
        }

        for (std::size_t inst_index = 0; inst_index < method_group.instantiations.size(); ++inst_index)
        {
            const auto* instantiation = method_group.instantiations[inst_index];
            for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
            {
                const std::string concrete = concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
                if (!concrete.empty() && slot_types[inst_index][slot].empty())
                {
                    slot_types[inst_index][slot] = concrete;
                }
            }
        }
    }

    for (const auto& ctor_group : ctor_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(ctor_group);
        if (!shape.supported)
        {
            continue;
        }

        for (std::size_t inst_index = 0; inst_index < ctor_group.instantiations.size(); ++inst_index)
        {
            const auto* instantiation = ctor_group.instantiations[inst_index];
            for (std::size_t slot = 0; slot < class_generic_arity; ++slot)
            {
                const std::string concrete = concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
                if (!concrete.empty() && slot_types[inst_index][slot].empty())
                {
                    slot_types[inst_index][slot] = concrete;
                }
            }
        }
    }

    return slot_types;
}

void append_generic_class_method(TextWriter& output, const ModuleDecl& module_decl, const GenericClassGroup& class_group,
    const GenericFunctionGroup& method_group, std::size_t class_generic_arity,
    const std::vector<std::vector<std::string>>& class_slot_types, const std::vector<bool>& ambiguous_tuples,
    const std::vector<bool>& borrowed_item_modes)
{
    const auto shape = analyze_generic_dispatch_shape(method_group);
    const bool use_shape_for_parameters = shape.supported;
    const bool use_shape_for_return = shape.supported && shape.generic_return_slot_id >= 0;

    const FunctionDecl& first = *method_group.instantiations.front();
    const std::string return_type = use_shape_for_return
        ? generic_class_type_parameter_name(static_cast<std::size_t>(shape.generic_return_slot_id), class_generic_arity)
        : wrapper_return_type(module_decl, first);

    const std::string method_access = first.csharp_private ? "private" : "public";
    output.append_format("    {} {} {}(", method_access, return_type,
        managed_name(module_decl, CSharpNameKind::Method, method_group.name));
    for (std::size_t index = 0; index < first.parameters.size(); ++index)
    {
        const auto& parameter = first.parameters[index];
        const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
        const std::string parameter_type = (use_shape_for_parameters && slot_id >= 0)
            ? generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity)
            : wrapper_type_name(parameter.type);
        output.append_format("{}{} {}", parameter_signature_byref_keyword(parameter), parameter_type, parameter.name);
        if (index + 1 < first.parameters.size())
        {
            output.append(", ");
        }
    }
    output.append_line(")");
    output.append_line("    {");

    const std::string diagnostics_message = csharp_string_literal_escape(std::format(
        "No generic mapping for {}.{} with provided generic type arguments.",
        managed_name(module_decl, CSharpNameKind::Class, class_group.name),
        managed_name(module_decl, CSharpNameKind::Method, method_group.name)));

    for (std::size_t inst_index = 0; inst_index < method_group.instantiations.size(); ++inst_index)
    {
        const auto* instantiation = method_group.instantiations[inst_index];
        const std::string condition = generic_class_instantiation_condition(
            module_decl, inst_index, class_generic_arity, class_slot_types, ambiguous_tuples, borrowed_item_modes,
            "_itemOwnership");

        output.append_line_format("        if ({})", condition);
        output.append_line("        {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::string> post_assignments;
        call_arguments.reserve(instantiation->parameters.size());
        for (std::size_t index = 0; index < instantiation->parameters.size(); ++index)
        {
            const auto& parameter = instantiation->parameters[index];
            const int slot_id = use_shape_for_parameters ? shape.generic_parameter_slot_ids[index] : -1;
            if (use_shape_for_parameters && slot_id >= 0)
            {
                const std::string typed_name = std::format("__csbind23_typed_arg{}", index);
                const std::string generic_name =
                    generic_class_type_parameter_name(static_cast<std::size_t>(slot_id), class_generic_arity);
                const std::string concrete_name = wrapper_type_name(parameter.type);
                if (parameter_is_ref(parameter))
                {
                    if (parameter_is_direct_out(parameter))
                    {
                        output.append_line_format("            {} {} = default!;", concrete_name, typed_name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                        post_assignments.push_back(std::format(
                            "{} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            parameter.name,
                            concrete_name,
                            generic_name,
                            typed_name));
                    }
                    else
                    {
                        output.append_line_format(
                            "            ref {} {} = ref System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                            concrete_name,
                            typed_name,
                            generic_name,
                            concrete_name,
                            parameter.name);
                        call_arguments.push_back(parameter_call_argument(parameter, typed_name));
                    }
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref {});",
                        concrete_name,
                        typed_name,
                        generic_name,
                        concrete_name,
                        parameter.name);

                    if (!parameter.type.managed_to_pinvoke_expression.empty())
                    {
                        const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                        const std::string converted = render_inline_template(
                            parameter.type.managed_to_pinvoke_expression,
                            typed_name,
                            pinvoke_name,
                            typed_name,
                            module_decl.name);
                        append_embedded_assignment(
                            output,
                            "            ",
                            std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                            converted);
                        call_arguments.push_back(pinvoke_name);

                        if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                        {
                            finalize_statements.push_back(render_inline_template(
                                parameter.type.managed_finalize_to_pinvoke_statement,
                                typed_name,
                                pinvoke_name,
                                pinvoke_name,
                                module_decl.name));
                        }
                    }
                    else
                    {
                        call_arguments.push_back(typed_name);
                    }
                }
            }
            else if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                const std::string converted = render_inline_template(
                    parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                    module_decl.name);
                append_embedded_assignment(
                    output,
                    "            ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    converted);
                call_arguments.push_back(pinvoke_name);

                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
            }
            else
            {
                call_arguments.push_back(parameter_call_argument(parameter));
            }
        }

        const std::string call_arguments_rendered = join_arguments(call_arguments);
        const std::string call = call_arguments_rendered.empty()
            ? std::format("{}Native.{}_{}_{}(_cPtr.Handle)", module_decl.name, module_decl.name,
                class_group.instantiations[inst_index]->name, exported_symbol_name(*instantiation))
            : std::format("{}Native.{}_{}_{}(_cPtr.Handle, {})", module_decl.name, module_decl.name,
                class_group.instantiations[inst_index]->name, exported_symbol_name(*instantiation),
                call_arguments_rendered);

        const bool has_return_converter = !instantiation->return_type.managed_from_pinvoke_expression.empty();
        const bool has_return_finalize = !instantiation->return_type.managed_finalize_from_pinvoke_statement.empty();
        const bool needs_finally = !finalize_statements.empty() || has_return_finalize;
        const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, *instantiation);

        if (return_type == "void")
        {
            if (!needs_finally)
            {
                output.append_line_format("            {} ;", call);
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("            {}", post_assignment);
                }
                output.append_line("            return;");
            }
            else
            {
                output.append_line("            try");
                output.append_line("            {");
                output.append_line_format("                {} ;", call);
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("                {}", post_assignment);
                }
                output.append_line("                return;");
                output.append_line("            }");
                output.append_line("            finally");
                output.append_line("            {");
                for (const auto& finalize_statement : finalize_statements)
                {
                    append_embedded_statement(output, "                ", finalize_statement);
                }
                output.append_line("            }");
            }
        }
        else if (use_shape_for_return)
        {
            const std::string concrete_name = wrapper_type_name(instantiation->return_type);
            const std::string generic_name = generic_class_type_parameter_name(
                static_cast<std::size_t>(shape.generic_return_slot_id), class_generic_arity);
            const bool has_generic_return_converter = !instantiation->return_type.managed_from_pinvoke_expression.empty();
            if (!needs_finally)
            {
                if (has_generic_return_converter)
                {
                    output.append_line_format(
                        "            {} __csbind23_result_ptr = {} ;", instantiation->return_type.pinvoke_name, call);
                    const std::string converted = render_inline_template(
                        instantiation->return_type.managed_from_pinvoke_expression,
                        "__csbind23_result_ptr",
                        "__csbind23_result_ptr",
                        "__csbind23_result_ptr",
                        module_decl.name);
                    append_embedded_assignment(
                        output,
                        "            ",
                        std::format("{} __csbind23_typed_result = ", concrete_name),
                        converted);
                }
                else
                {
                    output.append_line_format("            {} __csbind23_typed_result = {} ;", concrete_name, call);
                }
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("            {}", post_assignment);
                }
                output.append_line_format(
                    "            return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                    concrete_name,
                    generic_name);
            }
            else
            {
                output.append_line_format("            {} __csbind23_typed_result = default!;", concrete_name);
                output.append_line("            try");
                output.append_line("            {");
                if (has_generic_return_converter)
                {
                    output.append_line_format(
                        "                {} __csbind23_result_ptr = {} ;", instantiation->return_type.pinvoke_name, call);
                    const std::string converted = render_inline_template(
                        instantiation->return_type.managed_from_pinvoke_expression,
                        "__csbind23_result_ptr",
                        "__csbind23_result_ptr",
                        "__csbind23_result_ptr",
                        module_decl.name);
                    append_embedded_assignment(
                        output,
                        "                ",
                        "__csbind23_typed_result = ",
                        converted);
                }
                else
                {
                    output.append_line_format("                __csbind23_typed_result = {} ;", call);
                }
                for (const auto& post_assignment : post_assignments)
                {
                    output.append_line_format("                {}", post_assignment);
                }
                output.append_line_format(
                    "                return System.Runtime.CompilerServices.Unsafe.As<{}, {}>(ref __csbind23_typed_result);",
                    concrete_name,
                    generic_name);
                output.append_line("            }");
                output.append_line("            finally");
                output.append_line("            {");
                for (const auto& finalize_statement : finalize_statements)
                {
                    append_embedded_statement(output, "                ", finalize_statement);
                }
                output.append_line("            }");
            }
        }
        else
        {
            if (polymorphic_return_class != nullptr)
            {
                if (!needs_finally)
                {
                    output.append_line_format("            System.IntPtr __csbind23_result_ptr = {} ;", call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("            {}", post_assignment);
                    }
                    output.append_line_format("            return {}Runtime.WrapPolymorphic_{}(__csbind23_result_ptr, {});",
                        module_decl.name,
                        polymorphic_return_class->name,
                        item_ownership_literal(module_decl, false));
                }
                else
                {
                    output.append_line("            try");
                    output.append_line("            {");
                    output.append_line_format("                System.IntPtr __csbind23_result_ptr = {} ;", call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("                {}", post_assignment);
                    }
                    output.append_line_format("                return {}Runtime.WrapPolymorphic_{}(__csbind23_result_ptr, {});",
                        module_decl.name,
                        polymorphic_return_class->name,
                        item_ownership_literal(module_decl, false));
                    output.append_line("            }");
                    output.append_line("            finally");
                    output.append_line("            {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(output, "                ", finalize_statement);
                    }
                    output.append_line("            }");
                }
            }
            else if (!has_return_converter)
            {
                if (!needs_finally)
                {
                    output.append_line_format("            {} __csbind23_result = {} ;", return_type, call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("            {}", post_assignment);
                    }
                    output.append_line("            return __csbind23_result ;");
                }
                else
                {
                    output.append_line_format("            {} __csbind23_result = default!;", return_type);
                    output.append_line("            try");
                    output.append_line("            {");
                    output.append_line_format("                __csbind23_result = {} ;", call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("                {}", post_assignment);
                    }
                    output.append_line("                return __csbind23_result ;");
                    output.append_line("            }");
                    output.append_line("            finally");
                    output.append_line("            {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(output, "                ", finalize_statement);
                    }
                    output.append_line("            }");
                }
            }
            else
            {
                const std::string native_result_name = "__csbind23_result_pinvoke";
                const std::string managed_result_name = "__csbind23_result_managed";
                const std::string converted_expression = render_inline_template(
                    instantiation->return_type.managed_from_pinvoke_expression, managed_result_name, native_result_name,
                    native_result_name, module_decl.name);

                if (!needs_finally)
                {
                    output.append_line_format("            {} {} = {} ;", instantiation->return_type.pinvoke_name,
                        native_result_name, call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("            {}", post_assignment);
                    }
                    append_embedded_assignment(
                        output,
                        "            ",
                        std::format("{} {} = ", return_type, managed_result_name),
                        converted_expression);
                    output.append_line_format("            return {} ;", managed_result_name);
                }
                else
                {
                    output.append_line_format(
                        "            {} {} = default!;", instantiation->return_type.pinvoke_name, native_result_name);
                    output.append_line_format("            {} {} = default!;", return_type, managed_result_name);
                    output.append_line("            try");
                    output.append_line("            {");
                    output.append_line_format("                {} = {} ;", native_result_name, call);
                    for (const auto& post_assignment : post_assignments)
                    {
                        output.append_line_format("                {}", post_assignment);
                    }
                    append_embedded_assignment(
                        output,
                        "                ",
                        std::format("{} = ", managed_result_name),
                        converted_expression);
                    output.append_line_format("                return {} ;", managed_result_name);
                    output.append_line("            }");
                    output.append_line("            finally");
                    output.append_line("            {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(output, "                ", finalize_statement);
                    }
                    if (has_return_finalize)
                    {
                        append_embedded_statement(output, "                ", render_inline_template(
                            instantiation->return_type.managed_finalize_from_pinvoke_statement, managed_result_name,
                            native_result_name, native_result_name, module_decl.name));
                    }
                    output.append_line("            }");
                }
            }
        }
        output.append_line("        }");
    }

    output.append_line_format("        throw new System.NotSupportedException(\"{}\");", diagnostics_message);
    output.append_line("    }");
    output.append_line();
}

void append_generic_class_wrapper(TextWriter& output, const ModuleDecl& module_decl, const GenericClassGroup& class_group)
{
    if (class_group.instantiations.empty())
    {
        return;
    }

    const auto ctor_groups = collect_generic_class_ctor_groups(class_group);
    const auto method_groups = collect_generic_class_method_groups(class_group);

    std::size_t class_generic_arity = 0;
    for (const auto& ctor_group : ctor_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(ctor_group);
        if (shape.supported)
        {
            class_generic_arity = std::max(class_generic_arity, shape.generic_arity);
        }
    }
    for (const auto& method_group : method_groups)
    {
        const auto shape = analyze_generic_dispatch_shape(method_group);
        if (shape.supported)
        {
            class_generic_arity = std::max(class_generic_arity, shape.generic_arity);
        }
    }

    if (class_generic_arity == 0)
    {
        return;
    }

    const auto class_slot_types =
        collect_generic_class_slot_types(class_group, ctor_groups, method_groups, class_generic_arity);
    const auto ambiguous_tuples = collect_generic_class_tuple_ambiguity(class_slot_types);
    const auto borrowed_item_modes =
        collect_generic_class_borrowed_item_modes(class_group, ctor_groups, method_groups, class_generic_arity);

    std::vector<std::string> base_types;
    base_types.push_back("System.IDisposable");
    const auto& first_class_decl = *class_group.instantiations.front();
    for (const auto& interface_name : first_class_decl.csharp_interfaces)
    {
        if (!interface_name.empty())
        {
            base_types.push_back(interface_name);
        }
    }

    std::string base_clause;
    for (std::size_t index = 0; index < base_types.size(); ++index)
    {
        if (index > 0)
        {
            base_clause += ", ";
        }
        base_clause += base_types[index];
    }

    output.append_line_format("public sealed class {}<{}> : {}",
        managed_name(module_decl, CSharpNameKind::Class, class_group.name),
        generic_type_parameter_list(class_generic_arity),
        base_clause);
    output.append_line("{");
    output.append_line("    internal System.Runtime.InteropServices.HandleRef _cPtr;");
    output.append_line_format("    private {} _ownership;", item_ownership_type_name(module_decl));
    output.append_line_format("    internal {} _itemOwnership;", item_ownership_type_name(module_decl));
    output.append_line();

    output.append_line_format("    internal {}(System.IntPtr handle, {} ownership, {} itemOwnership = {})",
        managed_name(module_decl, CSharpNameKind::Class, class_group.name), item_ownership_type_name(module_decl),
        item_ownership_type_name(module_decl), item_ownership_literal(module_decl, true));
    output.append_line("    {");
    output.append_line("        _cPtr = new System.Runtime.InteropServices.HandleRef(this, handle);");
    output.append_line("        _ownership = ownership;");
    output.append_line("        _itemOwnership = itemOwnership;");
    output.append_line("    }");
    output.append_line();

    for (const auto& ctor_group : ctor_groups)
    {
        append_generic_class_constructor(output, class_group, module_decl, ctor_group, class_generic_arity,
            class_slot_types, ambiguous_tuples, borrowed_item_modes);
    }

    for (const auto& method_group : method_groups)
    {
        append_generic_class_method(output, module_decl, class_group, method_group, class_generic_arity,
            class_slot_types, ambiguous_tuples, borrowed_item_modes);
    }

    for (const auto& snippet : first_class_decl.csharp_member_snippets)
    {
        if (!snippet.empty())
        {
            output.append_line(replace_all(snippet, "__CSBIND23_ITEM_OWNERSHIP__", item_ownership_type_name(module_decl)));
            output.append_line();
        }
    }

    output.append_line("    ~" + managed_name(module_decl, CSharpNameKind::Class, class_group.name) + "()");
    output.append_line("    {");
    output.append_line("        ReleaseUnmanaged();");
    output.append_line("    }");
    output.append_line();

    output.append_line("    private void ReleaseUnmanaged()");
    output.append_line("    {");
    output.append_line("        if (_cPtr.Handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");
    output.append_line();
    output.append_line_format("        if (_ownership == {})", item_ownership_literal(module_decl, true));
    output.append_line("        {");
    for (std::size_t inst_index = 0; inst_index < class_group.instantiations.size(); ++inst_index)
    {
        const auto* class_decl = class_group.instantiations[inst_index];
        const std::string condition = generic_class_instantiation_condition(
            module_decl, inst_index, class_generic_arity, class_slot_types, ambiguous_tuples, borrowed_item_modes,
            "_itemOwnership");

        if (inst_index == 0)
        {
            output.append_line_format("            if ({})", condition);
        }
        else
        {
            output.append_line_format("            else if ({})", condition);
        }
        output.append_line("            {");
        if (class_has_owned_ctor(*class_decl))
        {
            output.append_line_format("                {}Native.{}_{}_destroy(_cPtr.Handle);", module_decl.name, module_decl.name, class_decl->name);
        }
        output.append_line("            }");
    }
    output.append_line("        }");
    output.append_line();
    output.append_line("        _cPtr = new System.Runtime.InteropServices.HandleRef(this, System.IntPtr.Zero);");
    output.append_line_format("        _ownership = {};", item_ownership_literal(module_decl, false));
    output.append_line("    }");
    output.append_line();

    output.append_line("    public void Dispose()");
    output.append_line("    {");
    output.append_line("        ReleaseUnmanaged();");
    output.append_line("        System.GC.SuppressFinalize(this);");
    output.append_line("    }");
    output.append_line("}");
    output.append_line();
}

std::string interface_name_for_class(const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    return "I" + managed_class_name(module_decl, class_decl);
}

bool is_primary_base_for_any_class(const ModuleDecl& module_decl, const ClassDecl& candidate)
{
    for (const auto& class_decl : module_decl.classes)
    {
        if (class_decl.cpp_name == candidate.cpp_name)
        {
            continue;
        }

        const auto* primary_base = primary_base_class(module_decl, class_decl);
        if (primary_base != nullptr && primary_base->cpp_name == candidate.cpp_name)
        {
            return true;
        }
    }

    return false;
}

void append_interface_declaration(TextWriter& output, const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    output.append_line_format("public interface {}", interface_name_for_class(module_decl, class_decl));
    output.append_line("{");
    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.pinvoke_only)
        {
            continue;
        }

        output.append_line_format(
            "    {} {}({});",
            wrapper_return_type(module_decl, method_decl),
            managed_method_name(module_decl, method_decl),
            parameter_list_without_self(method_decl));
    }
    output.append_line("}");
    output.append_line();
}

bool module_has_virtual_callbacks(const ModuleDecl& module_decl)
{
    for (const auto& class_decl : module_decl.classes)
    {
        const auto emitted_methods = collect_emitted_methods(module_decl, class_decl, csharp_method_collection_options());
        const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);
        if (!virtual_methods.empty())
        {
            return true;
        }
    }

    return false;
}

std::string callback_delegate_name(const FunctionDecl& method_decl, std::size_t index)
{
    return std::format("__csbind23_CallbackDelegate{}_{}", index, method_decl.virtual_slot_name);
}

std::string callback_method_name(const FunctionDecl& method_decl, std::size_t index)
{
    return std::format("__csbind23_CallbackMethod{}_{}", index, method_decl.virtual_slot_name);
}

std::size_t virtual_mask_chunk(std::size_t virtual_index)
{
    return virtual_index / 64;
}

std::size_t virtual_mask_bit(std::size_t virtual_index)
{
    return virtual_index % 64;
}

std::string virtual_mask_field_name(std::size_t chunk_index)
{
    return std::format("__csbind23_derivedOverrideMask{}", chunk_index);
}

std::string virtual_mask_bit_literal(std::size_t virtual_index)
{
    return std::format("(1UL << {})", virtual_mask_bit(virtual_index));
}

std::string virtual_mask_test_expression(std::string_view instance_prefix, std::size_t virtual_index)
{
    return std::format(
        "(({}{} & {}) != 0)",
        instance_prefix,
        virtual_mask_field_name(virtual_mask_chunk(virtual_index)),
        virtual_mask_bit_literal(virtual_index));
}

void append_native_signature(TextWriter& output, const FunctionDecl& function_decl, std::string_view pinvoke_library,
    const std::string& exported_name, bool include_self,
    std::size_t parameter_count = std::numeric_limits<std::size_t>::max())
{
    output.append_line_format(
        "    [System.Runtime.InteropServices.DllImport(\"{}\", CallingConvention = "
        "System.Runtime.InteropServices.CallingConvention.Cdecl)]",
        pinvoke_library);
    output.append_format("    internal static extern {} {}(", pinvoke_return_type(function_decl), exported_name);

    bool needs_separator = false;
    if (include_self)
    {
        output.append("System.IntPtr self");
        needs_separator = true;
    }

    const std::size_t count = parameter_count > function_decl.parameters.size()
        ? function_decl.parameters.size()
        : parameter_count;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto& parameter = function_decl.parameters[index];
        if (needs_separator)
        {
            output.append(", ");
        }
        const std::string pinvoke_parameter_type =
            parameter_is_supported_c_array_int(parameter) ? "System.IntPtr" : parameter.type.pinvoke_name;
        output.append_format("{}{} {}", parameter_direct_byref_keyword(parameter), pinvoke_parameter_type,
            parameter.name);
        needs_separator = true;
    }

    output.append_line(");");
    output.append_line();
}

void append_native_connect_signature(TextWriter& output, std::string_view pinvoke_library,
    const std::string& exported_name, const std::vector<const FunctionDecl*>& virtual_methods)
{
    (void)virtual_methods;
    output.append_line_format(
        "    [System.Runtime.InteropServices.DllImport(\"{}\", CallingConvention = "
        "System.Runtime.InteropServices.CallingConvention.Cdecl)]",
        pinvoke_library);
    output.append_line_format(
        "    internal static extern void {}(System.IntPtr self, System.IntPtr callbacks, nuint callbackCount);",
        exported_name);
    output.append_line();
}

bool class_has_owned_ctor(const ClassDecl& class_decl)
{
    for (const auto& method_decl : class_decl.methods)
    {
        if (method_decl.is_constructor && infer_ownership(method_decl) == Ownership::Owned)
        {
            return true;
        }
    }
    return false;
}

void append_wrapper_method(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl,
    const FunctionDecl& method_decl, bool is_virtual, bool is_override, std::size_t virtual_index)
{
    const std::string managed_method = managed_method_name(module_decl, method_decl);
    const std::string parameter_list = parameter_list_without_self(method_decl);
    const std::string native_name =
        std::format("{}_{}_{}", module_name, class_decl.name, exported_symbol_name(method_decl));
    const std::string base_native_name = native_name + "__base";
    const std::string return_type = wrapper_return_type(module_decl, method_decl);

    append_csharp_attributes(output, "    ", method_decl.csharp_attributes);
    append_csharp_comment(output, "    ", method_decl.csharp_comment);

    if (method_decl.is_generic_instantiation)
    {
        output.append_format("    private {} {}(", return_type, managed_method);
    }
    else if (method_decl.is_property_getter || method_decl.is_property_setter)
    {
        output.append_format("    private {} {}(", return_type, managed_method);
    }
    else if (is_override)
    {
        output.append_format("    public override {} {}(", return_type, managed_method);
    }
    else if (is_virtual)
    {
        output.append_format("    public virtual {} {}(", return_type, managed_method);
    }
    else
    {
        output.append_format("    {} {} {}(", method_decl.csharp_private ? "private" : "public", return_type,
            managed_method);
    }
    output.append(parameter_list);
    output.append_line(")");
    output.append_line("    {");

    std::vector<std::string> call_arguments;
    std::vector<std::string> finalize_statements;
    std::vector<std::pair<std::string, std::string>> pinned_array_parameters;
    std::unordered_set<std::string> null_checked_pinned_arrays;
    call_arguments.reserve(method_decl.parameters.size());
    for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
    {
        if (const auto source_index = inferred_c_array_source_index(method_decl, index); source_index.has_value())
        {
            call_arguments.push_back(std::format("checked((int){}.Length)", method_decl.parameters[*source_index].name));
            continue;
        }

        const auto& parameter = method_decl.parameters[index];
        if (parameter_is_supported_c_array_int(parameter))
        {
            const std::string pinned_name = std::format("__csbind23_arg{}_pinned", index);
            pinned_array_parameters.emplace_back(pinned_name, parameter.name);
            call_arguments.push_back(std::format("(System.IntPtr){}", pinned_name));
            continue;
        }

        if (const auto fixed_extent = fixed_std_array_int_extent(parameter); fixed_extent.has_value())
        {
            output.append_line_format("        if ({} == null)", parameter.name);
            output.append_line("        {");
            output.append_line_format("            throw new System.ArgumentNullException(nameof({}));", parameter.name);
            output.append_line("        }");
            output.append_line_format("        if ({}.Length != {})", parameter.name, *fixed_extent);
            output.append_line("        {");
            output.append_line_format(
                "            throw new System.ArgumentException($\"Expected array length {}\", nameof({}));",
                *fixed_extent,
                parameter.name);
            output.append_line("        }");

            const std::string pinned_name = std::format("__csbind23_arg{}_pinned", index);
            pinned_array_parameters.emplace_back(pinned_name, parameter.name);
            null_checked_pinned_arrays.insert(parameter.name);
            call_arguments.push_back(std::format("(System.IntPtr){}", pinned_name));
            continue;
        }

        if (!parameter.type.managed_to_pinvoke_expression.empty())
        {
            const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
            const std::string converted = render_inline_template(
                parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                module_name);
            append_embedded_assignment(
                output, "        ", std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name), converted);
            call_arguments.push_back(pinvoke_name);

            if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
            {
                finalize_statements.push_back(render_inline_template(
                    parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                    module_name));
            }
            continue;
        }

        call_arguments.push_back(parameter_call_argument(parameter));
    }

    const std::string call_arguments_rendered = join_arguments(call_arguments);
    const std::string native_call = call_arguments_rendered.empty()
        ? std::format("{}Native.{}(_cPtr.Handle)", module_name, native_name)
        : std::format("{}Native.{}(_cPtr.Handle, {})", module_name, native_name, call_arguments_rendered);
    const std::string base_native_call = call_arguments_rendered.empty()
        ? std::format("{}Native.{}(_cPtr.Handle)", module_name, base_native_name)
        : std::format("{}Native.{}(_cPtr.Handle, {})", module_name, base_native_name, call_arguments_rendered);

    const std::string dispatch_native_call = is_virtual
        ? std::format("({} ? {} : {})", virtual_mask_test_expression("", virtual_index), base_native_call, native_call)
        : native_call;

    const bool has_return_converter = !method_decl.return_type.managed_from_pinvoke_expression.empty();
    const bool has_return_finalize = !method_decl.return_type.managed_finalize_from_pinvoke_statement.empty();
    const bool needs_finally = !finalize_statements.empty() || has_return_finalize;

    for (const auto& pinned_parameter : pinned_array_parameters)
    {
        if (null_checked_pinned_arrays.contains(pinned_parameter.second))
        {
            continue;
        }
        output.append_line_format("        if ({} == null)", pinned_parameter.second);
        output.append_line("        {");
        output.append_line_format("            throw new System.ArgumentNullException(nameof({}));", pinned_parameter.second);
        output.append_line("        }");
    }
    if (!pinned_array_parameters.empty())
    {
        output.append_line("        unsafe");
        output.append_line("        {");
        for (const auto& pinned_parameter : pinned_array_parameters)
        {
            output.append_line_format("            fixed (int* {} = {})", pinned_parameter.first, pinned_parameter.second);
            output.append_line("            {");
        }
    }

    const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, method_decl);

    if (return_type == "void")
    {
        if (!needs_finally)
        {
            output.append_line_format("        {};", dispatch_native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {};", dispatch_native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }
        output.append_line("    }");
        output.append_line();
        return;
    }

    if (polymorphic_return_class != nullptr)
    {
        const std::string native_result_name = "__csbind23_result_ptr";
        const std::string wrapped_result = std::format(
            "{}Runtime.WrapPolymorphic_{}({}, {})", module_name, polymorphic_return_class->name, native_result_name,
            item_ownership_literal(module_decl, false));

        if (!needs_finally)
        {
            output.append_line_format("        System.IntPtr {} = {};", native_result_name, dispatch_native_call);
            output.append_line_format("        return {};", wrapped_result);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            System.IntPtr {} = {};", native_result_name, dispatch_native_call);
            output.append_line_format("            return {};", wrapped_result);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }
    }
    else if (!has_return_converter)
    {
        if (!needs_finally)
        {
            output.append_line_format("        return {};", dispatch_native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            return {};", dispatch_native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }
    }
    else
    {
        const std::string native_result_name = "__csbind23_result_pinvoke";
        const std::string managed_result_name = "__csbind23_result_managed";
        const std::string converted_expression =
            render_inline_template(method_decl.return_type.managed_from_pinvoke_expression, managed_result_name, native_result_name,
                native_result_name, module_name);

        if (!needs_finally)
        {
            output.append_line_format("        {} {} = {};", method_decl.return_type.pinvoke_name, native_result_name,
                dispatch_native_call);
            append_embedded_assignment(
                output, "        ", std::format("{} {} = ", return_type, managed_result_name), converted_expression);
            output.append_line_format("        return {};", managed_result_name);
        }
        else
        {
            output.append_line_format("        {} {} = default!;", method_decl.return_type.pinvoke_name, native_result_name);
            output.append_line_format("        {} {} = default!;", return_type, managed_result_name);
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            {} = {};", native_result_name, dispatch_native_call);
            append_embedded_assignment(
                output, "            ", std::format("{} = ", managed_result_name), converted_expression);
            output.append_line_format("            return {};", managed_result_name);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            if (has_return_finalize)
            {
                append_embedded_statement(output, "            ", render_inline_template(
                    method_decl.return_type.managed_finalize_from_pinvoke_statement, managed_result_name,
                    native_result_name, native_result_name, module_name));
            }
            output.append_line("        }");
        }
    }

    if (!pinned_array_parameters.empty())
    {
        for (std::size_t index = 0; index < pinned_array_parameters.size(); ++index)
        {
            output.append_line("            }");
        }
        output.append_line("        }");
    }

    output.append_line("    }");
    output.append_line();
}

void append_instance_cache_support(TextWriter& output, const ModuleDecl& module_decl, const ClassDecl& class_decl)
{
    output.append_line_format(
        "    private static readonly {} __csbind23_registry = new();",
        resolved_instance_cache_type(module_decl, class_decl));
    output.append_line();
}

void append_virtual_director_support(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl,
    const std::vector<const FunctionDecl*>& virtual_methods)
{
    const std::string managed_class = managed_class_name(module_decl, class_decl);

    const std::size_t mask_field_count = (virtual_methods.size() + 63) / 64;
    for (std::size_t chunk = 0; chunk < mask_field_count; ++chunk)
    {
        output.append_line_format("    private ulong {};", virtual_mask_field_name(chunk));
    }
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        if (method_decl.parameters.empty())
        {
            output.append_line_format("    private static readonly System.Type[] __csbind23_methodTypes{} = System.Type.EmptyTypes;", index);
        }
        else
        {
            std::string type_list;
            for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
            {
                const auto& parameter = method_decl.parameters[p];
                if (!type_list.empty())
                {
                    type_list += ", ";
                }
                type_list += reflection_parameter_type_expression(parameter);
            }
            output.append_line_format(
                "    private static readonly System.Type[] __csbind23_methodTypes{} = new System.Type[] {{ {} }};",
                index,
                type_list);
        }
    }
    output.append_line();

    output.append_line("    private void __csbind23_InitializeDerivedOverrideFlags()");
    output.append_line("    {");
    for (std::size_t chunk = 0; chunk < mask_field_count; ++chunk)
    {
        output.append_line_format("        {} = 0UL;", virtual_mask_field_name(chunk));
    }
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        const std::size_t chunk = virtual_mask_chunk(index);
        output.append_line_format(
            "        {} |= {}.__csbind23_DerivedClassHasMethod(this, \"{}\", typeof({}), __csbind23_methodTypes{}, {});",
            virtual_mask_field_name(chunk),
            csharp_api_class_name(module_decl),
            managed_method_name(module_decl, method_decl),
            managed_class,
            index,
            virtual_mask_bit_literal(index));
    }
    output.append_line("    }");
    output.append_line();

    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        output.append_line("    [System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.Cdecl)]");
        output.append_format("    private delegate {} {}(System.IntPtr self", method_decl.return_type.pinvoke_name,
            callback_delegate_name(method_decl, index));
        for (const auto& parameter : method_decl.parameters)
        {
            output.append_format(", {}{} {}", parameter_direct_byref_keyword(parameter), parameter.type.pinvoke_name,
                parameter.name);
        }
        output.append_line(");");
        output.append_line_format(
            "    private static readonly {} __csbind23_staticCallbackDelegate{} = {};",
            callback_delegate_name(method_decl, index),
            index,
            callback_method_name(method_decl, index));
        output.append_line_format(
            "    private static readonly System.IntPtr __csbind23_staticCallbackPtr{} = System.Runtime.InteropServices.Marshal.GetFunctionPointerForDelegate(__csbind23_staticCallbackDelegate{});",
            index,
            index);
        output.append_line();
    }

    output.append_line("    private void __csbind23_ConnectDirector()");
    output.append_line("    {");
    output.append_line("        if (_cPtr.Handle == System.IntPtr.Zero)");
    output.append_line("        {");
    output.append_line("            return;");
    output.append_line("        }");

    output.append_line_format("        System.Span<System.IntPtr> __csbind23_callbacks = stackalloc System.IntPtr[{}];", virtual_methods.size());
    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        output.append_line_format("        if ({})", virtual_mask_test_expression("", index));
        output.append_line("        {");
        output.append_line_format("            __csbind23_callbacks[{}] = __csbind23_staticCallbackPtr{};", index, index);
        output.append_line("        }");
    }
    output.append_line("        unsafe");
    output.append_line("        {");
    output.append_line("            fixed (System.IntPtr* __csbind23_callbacksPtr = __csbind23_callbacks)");
    output.append_line("            {");
    output.append_line_format(
        "                {}Native.{}_{}_connect_director(_cPtr.Handle, (System.IntPtr)__csbind23_callbacksPtr, (nuint){});",
        module_name,
        module_name,
        class_decl.name,
        virtual_methods.size());
    output.append_line("            }");
    output.append_line("        }");
    output.append_line("    }");
    output.append_line();

    for (std::size_t index = 0; index < virtual_methods.size(); ++index)
    {
        const auto& method_decl = *virtual_methods[index];
        const std::string method_name = callback_method_name(method_decl, index);

        output.append_format("    private static {} {}(System.IntPtr self", method_decl.return_type.pinvoke_name, method_name);
        for (const auto& parameter : method_decl.parameters)
        {
            output.append_format(", {}{} {}", parameter_direct_byref_keyword(parameter), parameter.type.pinvoke_name,
                parameter.name);
        }
        output.append_line(")");
        output.append_line("    {");
        output.append_line_format("        if (!__csbind23_registry.TryGet(self, out var instance))");
        output.append_line("        {");
        if (!is_cabi_void(method_decl.return_type.c_abi_name))
        {
            output.append_line("            return default!;");
        }
        else
        {
            output.append_line("            return;");
        }
        output.append_line("        }");

        std::vector<std::string> base_call_arguments;
        base_call_arguments.reserve(method_decl.parameters.size() + 1);
        base_call_arguments.push_back("self");
        for (const auto& parameter : method_decl.parameters)
        {
            base_call_arguments.push_back(parameter_call_argument(parameter));
        }
        const std::string base_call_arguments_rendered = join_arguments(base_call_arguments);
        const std::string base_native_call = std::format(
            "{}Native.{}_{}__base({})",
            module_name,
            module_name,
            class_decl.name + "_" + method_decl.name,
            base_call_arguments_rendered);

        output.append_line_format("        if (!{})", virtual_mask_test_expression("instance.", index));
        output.append_line("        {");
        if (is_cabi_void(method_decl.return_type.c_abi_name))
        {
            output.append_line_format("            {} ;", base_native_call);
            output.append_line("            return;");
        }
        else
        {
            output.append_line_format("            return {};", base_native_call);
        }
        output.append_line("        }");

        std::vector<std::string> managed_args;
        managed_args.reserve(method_decl.parameters.size());
        for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
        {
            const auto& parameter = method_decl.parameters[p];
            if (!parameter.type.managed_from_pinvoke_expression.empty())
            {
                const std::string managed_name = std::format("__csbind23_arg{}_managed", p);
                append_embedded_assignment(
                    output,
                    "        ",
                    std::format("{} {} = ", wrapper_type_name(parameter.type), managed_name),
                    render_inline_template(
                        parameter.type.managed_from_pinvoke_expression, managed_name, parameter.name, parameter.name,
                        module_name));
                managed_args.push_back(managed_name);
            }
            else
            {
                if (parameter_is_ref(parameter))
                {
                    managed_args.push_back(parameter_signature_byref_keyword(parameter) + parameter.name);
                }
                else
                {
                    managed_args.push_back(parameter.name);
                }
            }
        }

        const std::string invoke_args = join_arguments(managed_args);

        if (is_cabi_void(method_decl.return_type.c_abi_name))
        {
            output.append_line_format("        instance.{}({});", managed_method_name(module_decl, method_decl), invoke_args);
            output.append_line("        return;");
        }
        else
        {
            output.append_line_format(
                "        {} __csbind23_result = instance.{}({});",
                wrapper_return_type(module_decl, method_decl),
                managed_method_name(module_decl, method_decl),
                invoke_args);

            if (!method_decl.return_type.managed_to_pinvoke_expression.empty())
            {
                append_embedded_return(output, "        ", render_inline_template(
                    method_decl.return_type.managed_to_pinvoke_expression, "__csbind23_result", "__csbind23_result",
                    "__csbind23_result", module_name));
            }
            else
            {
                output.append_line("        return __csbind23_result;");
            }
        }

        output.append_line("    }");
        output.append_line();
    }
}

void append_wrapper_class(TextWriter& output, const ModuleDecl& module_decl, const std::string& module_name, const ClassDecl& class_decl)
{
    const std::string managed_class = managed_class_name(module_decl, class_decl);
    const ClassDecl* base_class = primary_base_class(module_decl, class_decl);
    const bool has_base_class = base_class != nullptr;
    const bool emits_destroy = class_has_owned_ctor(class_decl);
    const auto emitted_methods = collect_emitted_methods(module_decl, class_decl, csharp_method_collection_options());
    const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);
    const bool has_virtual_support = !virtual_methods.empty();
    const bool is_primary_base_class = is_primary_base_for_any_class(module_decl, class_decl);

    std::vector<std::string> base_types;
    if (has_base_class)
    {
        base_types.push_back(managed_class_name(module_decl, *base_class));
    }
    else
    {
        base_types.push_back("System.IDisposable");
    }
    for (const auto* secondary_base : secondary_base_classes(module_decl, class_decl))
    {
        base_types.push_back(interface_name_for_class(module_decl, *secondary_base));
    }
    for (const auto& interface_name : class_decl.csharp_interfaces)
    {
        if (!interface_name.empty())
        {
            base_types.push_back(interface_name);
        }
    }

    std::string base_clause;
    for (std::size_t index = 0; index < base_types.size(); ++index)
    {
        if (index > 0)
        {
            base_clause += ", ";
        }
        base_clause += base_types[index];
    }

    const std::string class_declaration_kind = has_virtual_support
        ? "partial class"
        : (is_primary_base_class ? "class" : "sealed class");
    const std::string class_visibility = class_decl.is_generic_instantiation ? "internal" : "public";

    append_csharp_attributes(output, "", class_decl.csharp_attributes);
    append_csharp_comment(output, "", class_decl.csharp_comment);
    output.append_line_format("{} {} {} : {}", class_visibility, class_declaration_kind, managed_class, base_clause);
    output.append_line("{");
    if (!has_base_class)
    {
        const std::string handle_field_visibility = (has_virtual_support || is_primary_base_class) ? "protected" : "private";
        output.append_line("    internal System.Runtime.InteropServices.HandleRef _cPtr;");
        output.append_line_format("    {} {} _ownership;", handle_field_visibility, item_ownership_type_name(module_decl));
    }
    output.append_line();

    output.append_format("    internal {}(System.IntPtr handle, {} ownership)", managed_class, item_ownership_type_name(module_decl));
    if (has_base_class)
    {
        output.append(" : base(handle, ownership)");
    }
    output.append_line("");
    output.append_line("    {");
    if (!has_base_class)
    {
        output.append_line("        _cPtr = new System.Runtime.InteropServices.HandleRef(this, handle);");
        output.append_line("        _ownership = ownership;");
        if (has_virtual_support)
        {
            output.append_line("        __csbind23_InitializeDerivedOverrideFlags();");
        }
    }
    output.append_line("        __csbind23_registry.Register(_cPtr.Handle, this);");
    if (!has_base_class && has_virtual_support)
    {
        output.append_line("        __csbind23_ConnectDirector();");
    }
    output.append_line("    }");
    output.append_line();

    for (const auto& method_decl : class_decl.methods)
    {
        if (!method_decl.is_constructor)
        {
            continue;
        }

        const std::string parameter_list = parameter_list_without_self(method_decl);
        const std::string create_name = std::format("{}_{}_create", module_name, class_decl.name);
        const bool owns_handle = infer_ownership(method_decl) == Ownership::Owned;

        output.append_format("    public {}(", managed_class);
        output.append(parameter_list);
        output.append(")");
        if (has_base_class)
        {
            output.append_format(" : base(System.IntPtr.Zero, {})", item_ownership_literal(module_decl, false));
        }
        output.append_line("");
        output.append_line("    {");

        std::vector<std::string> converted_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::pair<std::string, std::string>> pinned_array_parameters;
        converted_arguments.reserve(method_decl.parameters.size());
        for (std::size_t index = 0; index < method_decl.parameters.size(); ++index)
        {
            const auto& parameter = method_decl.parameters[index];
            if (const auto fixed_extent = fixed_std_array_int_extent(parameter); fixed_extent.has_value())
            {
                output.append_line_format("        if ({} == null)", parameter.name);
                output.append_line("        {");
                output.append_line_format("            throw new System.ArgumentNullException(nameof({}));", parameter.name);
                output.append_line("        }");
                output.append_line_format("        if ({}.Length != {})", parameter.name, *fixed_extent);
                output.append_line("        {");
                output.append_line_format(
                    "            throw new System.ArgumentException($\"Expected array length {}\", nameof({}));",
                    *fixed_extent,
                    parameter.name);
                output.append_line("        }");

                const std::string pinned_name = std::format("__csbind23_arg{}_pinned", index);
                pinned_array_parameters.emplace_back(pinned_name, parameter.name);
                converted_arguments.push_back(std::format("(System.IntPtr){}", pinned_name));
                continue;
            }

            if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                append_embedded_assignment(
                    output,
                    "        ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    render_inline_template(
                        parameter.type.managed_to_pinvoke_expression, parameter.name, pinvoke_name, parameter.name,
                        module_name));
                converted_arguments.push_back(pinvoke_name);
                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, parameter.name, pinvoke_name, pinvoke_name,
                        module_name));
                }
                continue;
            }

            converted_arguments.push_back(parameter_call_argument(parameter));
        }

        const std::string converted_arguments_rendered = join_arguments(converted_arguments);
        const std::string native_call = converted_arguments_rendered.empty()
            ? std::format("{}Native.{}()", module_name, create_name)
            : std::format("{}Native.{}({})", module_name, create_name, converted_arguments_rendered);

        if (!pinned_array_parameters.empty())
        {
            output.append_line("        unsafe");
            output.append_line("        {");
            for (const auto& pinned_parameter : pinned_array_parameters)
            {
                output.append_line_format("            fixed (int* {} = {})", pinned_parameter.first, pinned_parameter.second);
                output.append_line("            {");
            }
        }

        if (finalize_statements.empty())
        {
            output.append_line_format("        _cPtr = new System.Runtime.InteropServices.HandleRef(this, {});", native_call);
        }
        else
        {
            output.append_line("        try");
            output.append_line("        {");
            output.append_line_format("            _cPtr = new System.Runtime.InteropServices.HandleRef(this, {});", native_call);
            output.append_line("        }");
            output.append_line("        finally");
            output.append_line("        {");
            for (const auto& finalize_statement : finalize_statements)
            {
                append_embedded_statement(output, "            ", finalize_statement);
            }
            output.append_line("        }");
        }

        if (!pinned_array_parameters.empty())
        {
            for (std::size_t index = 0; index < pinned_array_parameters.size(); ++index)
            {
                output.append_line("            }");
            }
            output.append_line("        }");
        }

        output.append_line_format("        _ownership = {};", item_ownership_literal(module_decl, owns_handle));
        if (has_virtual_support)
        {
            output.append_line("        __csbind23_InitializeDerivedOverrideFlags();");
        }
        output.append_line("        __csbind23_registry.Register(_cPtr.Handle, this);");
        if (has_virtual_support)
        {
            output.append_line("        __csbind23_ConnectDirector();");
        }
        output.append_line("    }");
        output.append_line();
    }

    if (!has_base_class)
    {
        output.append_line("    public void Dispose()");
        output.append_line("    {");
        output.append_line("        ReleaseUnmanaged();");
        output.append_line("        System.GC.SuppressFinalize(this);");
        output.append_line("    }");
        output.append_line();

        if (emits_destroy)
        {
            output.append_line("    public void TakeOwnership()");
            output.append_line("    {");
            output.append_line("        if (_cPtr.Handle == System.IntPtr.Zero)");
            output.append_line("        {");
            output.append_line("            return;");
            output.append_line("        }");
            output.append_line();
            output.append_line_format("        _ownership = {};", item_ownership_literal(module_decl, true));
            output.append_line("    }");
            output.append_line();

            output.append_line("    public void ReleaseOwnership()");
            output.append_line("    {");
            output.append_line_format("        _ownership = {};", item_ownership_literal(module_decl, false));
            output.append_line("    }");
            output.append_line();

            output.append_line("    public void DestroyNative()");
            output.append_line("    {");
            output.append_line("        if (_cPtr.Handle == System.IntPtr.Zero)");
            output.append_line("        {");
            output.append_line("            return;");
            output.append_line("        }");
            output.append_line();
            output.append_line("        var __csbind23_oldHandle = _cPtr.Handle;");

            if (has_virtual_support)
            {
                output.append_line_format(
                    "        {}Native.{}_{}_disconnect_director(_cPtr.Handle);", module_name, module_name, class_decl.name);
            }

            output.append_line("        __csbind23_registry.Unregister(__csbind23_oldHandle);");
            output.append_line_format(
                "        {}Native.{}_{}_destroy(_cPtr.Handle);", module_name, module_name, class_decl.name);
            output.append_line("        _cPtr = new System.Runtime.InteropServices.HandleRef(this, System.IntPtr.Zero);");
            output.append_line_format("        _ownership = {};", item_ownership_literal(module_decl, false));
            output.append_line("        System.GC.SuppressFinalize(this);");
            output.append_line("    }");
            output.append_line();
        }

        output.append_line_format("    ~{}()", managed_class);
        output.append_line("    {");
        output.append_line("        ReleaseUnmanaged();");
        output.append_line("    }");
        output.append_line();

        output.append_line("    private void ReleaseUnmanaged()");
        output.append_line("    {");
        output.append_line("        if (_cPtr.Handle == System.IntPtr.Zero)");
        output.append_line("        {");
        output.append_line("            return;");
        output.append_line("        }");
        output.append_line();

        output.append_line("        var __csbind23_oldHandle = _cPtr.Handle;");

        if (has_virtual_support)
        {
            output.append_line_format("        {}Native.{}_{}_disconnect_director(_cPtr.Handle);", module_name, module_name, class_decl.name);
        }

        output.append_line("        __csbind23_registry.Unregister(__csbind23_oldHandle);");
        output.append_line();

        output.append_line_format("        if (_ownership == {})", item_ownership_literal(module_decl, true));
        output.append_line("        {");
        if (emits_destroy)
        {
            output.append_line_format(
                "            {}Native.{}_{}_destroy(_cPtr.Handle);", module_name, module_name, class_decl.name);
        }
        output.append_line("        }");
        output.append_line();
        output.append_line("        _cPtr = new System.Runtime.InteropServices.HandleRef(this, System.IntPtr.Zero);");
        output.append_line_format("        _ownership = {};", item_ownership_literal(module_decl, false));
        output.append_line("    }");
        output.append_line();
    }

    append_instance_cache_support(output, module_decl, class_decl);

    if (has_virtual_support)
    {
        append_virtual_director_support(output, module_decl, module_name, class_decl, virtual_methods);
    }

    for (std::size_t index = 0; index < emitted_methods.size(); ++index)
    {
        const auto& method_decl = emitted_methods[index].method;
        if (method_decl.is_constructor)
        {
            continue;
        }

        if (method_decl.pinvoke_only)
        {
            continue;
        }

        std::size_t virtual_index = 0;
        bool is_virtual = false;
        for (std::size_t i = 0; i < virtual_methods.size(); ++i)
        {
            if (virtual_methods[i] == &emitted_methods[index].method)
            {
                is_virtual = true;
                virtual_index = i;
                break;
            }
        }

        bool is_override = false;
        if (has_base_class)
        {
            for (const auto& base_method : base_class->methods)
            {
                if (base_method.is_constructor || base_method.name != method_decl.name)
                {
                    continue;
                }

                if (base_method.parameters.size() != method_decl.parameters.size())
                {
                    continue;
                }

                bool same_signature = true;
                for (std::size_t p = 0; p < method_decl.parameters.size(); ++p)
                {
                    if (base_method.parameters[p].type.pinvoke_name != method_decl.parameters[p].type.pinvoke_name)
                    {
                        same_signature = false;
                        break;
                    }
                }

                if (same_signature)
                {
                    is_override = true;
                    break;
                }
            }
        }

        append_wrapper_method(output, module_decl, module_name, class_decl, method_decl, is_virtual, is_override,
            virtual_index);
    }

    const auto generic_method_groups = collect_generic_method_groups(emitted_methods);
    for (const auto& group : generic_method_groups)
    {
        append_method_generic_dispatch_wrapper(output, module_decl, group);
    }

    for (const auto& property_decl : class_decl.properties)
    {
        const FunctionDecl* getter_decl = nullptr;
        for (const auto& method_decl : class_decl.methods)
        {
            if (method_decl.name == property_decl.getter_name)
            {
                getter_decl = &method_decl;
                break;
            }
        }

        std::string property_type = getter_decl != nullptr
            ? wrapper_return_type(module_decl, *getter_decl)
            : wrapper_type_name(property_decl.type);

        append_csharp_comment(output, "    ", property_decl.csharp_comment);
        output.append_line_format("    public {} {}", property_type, managed_property_name(module_decl, property_decl));
        output.append_line("    {");
        if (property_decl.has_getter)
        {
            const auto getter = std::find_if(class_decl.methods.begin(), class_decl.methods.end(),
                [&property_decl](const FunctionDecl& method_decl) { return method_decl.name == property_decl.getter_name; });
            if (getter != class_decl.methods.end())
            {
                output.append_line_format("        get => {}();", managed_method_name(module_decl, *getter));
            }
        }
        if (property_decl.has_setter)
        {
            const auto setter = std::find_if(class_decl.methods.begin(), class_decl.methods.end(),
                [&property_decl](const FunctionDecl& method_decl) { return method_decl.name == property_decl.setter_name; });
            if (setter != class_decl.methods.end())
            {
                output.append_line_format("        set => {}(value);", managed_method_name(module_decl, *setter));
            }
        }

        output.append_line("    }");
        output.append_line();
    }

    for (const auto& snippet : class_decl.csharp_member_snippets)
    {
        if (!snippet.empty())
        {
            output.append_line(replace_all(snippet, "__CSBIND23_ITEM_OWNERSHIP__", item_ownership_type_name(module_decl)));
            output.append_line();
        }
    }

    output.append_line("}");
    output.append_line();
}

} // namespace

std::vector<std::filesystem::path> emit_csharp_module(
    const ModuleDecl& module_decl, const std::filesystem::path& output_root)
{
    std::filesystem::create_directories(output_root);

    std::vector<std::filesystem::path> generated_files;
    emit_shared_item_ownership_type_if_needed(module_decl, output_root, generated_files);
    emit_shared_pinvoke_types_if_needed(module_decl, output_root, generated_files);
    emit_shared_instance_cache_types_if_needed(module_decl, output_root, generated_files);
    emit_shared_array_interop_types_if_needed(module_decl, output_root, generated_files);

    std::unordered_set<std::string> emitted_interfaces;
    for (const auto& class_decl : module_decl.classes)
    {
        for (const auto* secondary_base : secondary_base_classes(module_decl, class_decl))
        {
            if (!emitted_interfaces.insert(secondary_base->cpp_name).second)
            {
                continue;
            }

            TextWriter interface_file(512);
            interface_file.append_line_format("namespace {};", csharp_namespace_name(module_decl, *secondary_base));
            interface_file.append_line();
            append_interface_declaration(interface_file, module_decl, *secondary_base);
            generated_files.push_back(write_csharp_file(
                output_root,
                module_decl.name + "." + interface_name_for_class(module_decl, *secondary_base) + ".g.cs",
                interface_file.str()));
        }
    }

    TextWriter generated(3072);

    generated.append_line_format("namespace {};", csharp_namespace_name(module_decl));
    generated.append_line();
    generated.append_line_format("internal static class {}Native", module_decl.name);
    generated.append_line("{");

    for (const auto& function_decl : module_decl.functions)
    {
        append_native_signature(generated, function_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + exported_symbol_name(function_decl), false);

        const std::size_t defaults = free_function_default_count(module_decl, function_decl);
        for (std::size_t omitted = 1; omitted <= defaults; ++omitted)
        {
            append_native_signature(
                generated,
                function_decl,
                module_decl.pinvoke_library,
                module_decl.name + "_" + exported_symbol_name(function_decl) + std::format("__default_{}", omitted),
                false,
                function_decl.parameters.size() - omitted);
        }
    }

    for (const auto& class_decl : module_decl.classes)
    {
        const auto emitted_methods = collect_emitted_methods(module_decl, class_decl, csharp_method_collection_options());
        const auto virtual_methods = collect_virtual_methods(class_decl, emitted_methods);

        FunctionDecl type_name_decl;
        type_name_decl.return_type.c_abi_name = "int";
        type_name_decl.return_type.pinvoke_name = "int";
        append_native_signature(generated, type_name_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + class_decl.name + "_static_type_id", false);

        FunctionDecl dynamic_type_name_decl;
        dynamic_type_name_decl.return_type.c_abi_name = "int";
        dynamic_type_name_decl.return_type.pinvoke_name = "int";
        append_native_signature(generated, dynamic_type_name_decl, module_decl.pinvoke_library,
            module_decl.name + "_" + class_decl.name + "_dynamic_type_id", true);

        if (class_has_owned_ctor(class_decl))
        {
            FunctionDecl destroy_decl;
            destroy_decl.return_type.c_abi_name = "void";
            destroy_decl.return_type.pinvoke_name = "void";
            append_native_signature(generated, destroy_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_destroy", true);
        }

        if (!virtual_methods.empty())
        {
            append_native_connect_signature(generated, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_connect_director", virtual_methods);

            FunctionDecl disconnect_decl;
            disconnect_decl.return_type.c_abi_name = "void";
            disconnect_decl.return_type.pinvoke_name = "void";
            append_native_signature(generated, disconnect_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_disconnect_director", true);
        }

        for (const auto& method_decl : class_decl.methods)
        {
            if (method_decl.is_constructor)
            {
                append_native_signature(
                    generated, method_decl, module_decl.pinvoke_library,
                    module_decl.name + "_" + class_decl.name + "_create", false);
            }
        }

        for (const auto& emitted_method : emitted_methods)
        {
            const auto& method_decl = emitted_method.method;
            append_native_signature(generated, method_decl, module_decl.pinvoke_library,
                module_decl.name + "_" + class_decl.name + "_" + exported_symbol_name(method_decl), true);

            if (!virtual_methods.empty() && method_decl.allow_override)
            {
                append_native_signature(generated, method_decl, module_decl.pinvoke_library,
                    module_decl.name + "_" + class_decl.name + "_" + exported_symbol_name(method_decl) + "__base", true);
            }
        }
    }

    generated.append_line("}");
    generated.append_line();

    for (const auto& enum_decl : module_decl.enums)
    {
        if (enum_decl.is_flags)
        {
            generated.append_line("[System.Flags]");
        }
        append_csharp_attributes(generated, "", enum_decl.csharp_attributes);
        generated.append_line_format("public enum {} : {}", managed_enum_name(module_decl, enum_decl), csharp_enum_underlying_type(enum_decl));
        generated.append_line("{");
        for (const auto& enum_value : enum_decl.values)
        {
            generated.append_line_format("    {} = {},", managed_enum_value_name(module_decl, enum_value), csharp_enum_value_literal(enum_decl, enum_value));
        }
        generated.append_line("}");
        generated.append_line();
    }

    generated.append_line_format("internal static class {}Runtime", module_decl.name);
    generated.append_line("{");
    generated.append_line_format(
        "    private static readonly System.Func<System.IntPtr, {}, object>[] __csbind23_typeFactories =",
        item_ownership_type_name(module_decl));
    generated.append_line_format("        new System.Func<System.IntPtr, {}, object>[]", item_ownership_type_name(module_decl));
    generated.append_line("        {");
    for (const auto& class_decl : module_decl.classes)
    {
        if (class_decl.is_generic_instantiation)
        {
            generated.append_line("            (handle, ownership) => handle,");
        }
        else
        {
            generated.append_line_format(
                "            (handle, ownership) => new {}(handle, ownership),", qualified_managed_class_name(module_decl, class_decl));
        }
    }
    generated.append_line("        };");
    generated.append_line();

    for (const auto& class_decl : module_decl.classes)
    {
        generated.append_line_format(
            "    internal static object WrapPolymorphic_{}(System.IntPtr handle, {} ownership)", class_decl.name,
            item_ownership_type_name(module_decl));
        generated.append_line("    {");
        generated.append_line("        if (handle == System.IntPtr.Zero)");
        generated.append_line("        {");
        generated.append_line("            return null!;");
        generated.append_line("        }");
        generated.append_line();
        generated.append_line_format(
            "        int dynamicTypeId = {}Native.{}_{}_dynamic_type_id(handle);",
            module_decl.name,
            module_decl.name,
            class_decl.name);
        generated.append_line("        if (dynamicTypeId >= 0 && dynamicTypeId < __csbind23_typeFactories.Length)");
        generated.append_line("        {");
        generated.append_line("            return __csbind23_typeFactories[dynamicTypeId](handle, ownership);");
        generated.append_line("        }");
        generated.append_line();
        if (class_decl.is_generic_instantiation)
        {
            generated.append_line("        return handle;");
        }
        else
        {
            generated.append_line_format("        return new {}(handle, ownership);", qualified_managed_class_name(module_decl, class_decl));
        }
        generated.append_line("    }");
        generated.append_line();
    }

    generated.append_line("}");
    generated.append_line();

    generated.append_line_format("public static class {}", csharp_api_class_name(module_decl));
    generated.append_line("{");
    if (module_has_virtual_callbacks(module_decl))
    {
        generated.append_line("    internal static ulong __csbind23_DerivedClassHasMethod(");
        generated.append_line("        object instance,");
        generated.append_line("        string methodName,");
        generated.append_line("        System.Type classType,");
        generated.append_line("        System.Type[] methodTypes,");
        generated.append_line("        ulong derivedFlag)");
        generated.append_line("    {");
        generated.append_line("        var methodInfo = instance.GetType().GetMethod(");
        generated.append_line("            methodName,");
        generated.append_line("            System.Reflection.BindingFlags.Public |");
        generated.append_line("            System.Reflection.BindingFlags.NonPublic |");
        generated.append_line("            System.Reflection.BindingFlags.Instance,");
        generated.append_line("            null,");
        generated.append_line("            methodTypes,");
        generated.append_line("            null);");
        generated.append_line("        if (methodInfo == null)");
        generated.append_line("        {");
        generated.append_line("            return 0UL;");
        generated.append_line("        }");
        generated.append_line();
        generated.append_line("        bool hasDerivedMethod = methodInfo.IsVirtual");
        generated.append_line("            && !methodInfo.IsAbstract");
        generated.append_line("            && (methodInfo.DeclaringType!.IsSubclassOf(classType) || methodInfo.DeclaringType == classType)");
        generated.append_line("            && methodInfo.DeclaringType != methodInfo.GetBaseDefinition().DeclaringType;");
        generated.append_line("        return hasDerivedMethod ? derivedFlag : 0UL;");
        generated.append_line("    }");
        generated.append_line();
    }

    for (const auto& function_decl : module_decl.functions)
    {
        if (function_decl.pinvoke_only)
        {
            continue;
        }

        append_csharp_attributes(generated, "    ", function_decl.csharp_attributes);
        append_csharp_comment(generated, "    ", function_decl.csharp_comment);
        const std::string params = free_function_parameter_list(module_decl, function_decl);
        const std::string return_type = wrapper_return_type(module_decl, function_decl);
        const std::string native_name = module_decl.name + "_" + exported_symbol_name(function_decl);
        const std::size_t defaults = free_function_default_count(module_decl, function_decl);
        const std::size_t optional_start = function_decl.parameters.size() - defaults;
        const std::string method_visibility = function_decl.is_generic_instantiation ? "private" : "public";

        generated.append_format(
            "    {} static {} {}(", method_visibility, return_type, managed_function_name(module_decl, function_decl));
        generated.append(params);
        generated.append_line(")");
        generated.append_line("    {");

        std::vector<std::string> call_arguments;
        std::vector<std::string> finalize_statements;
        std::vector<std::pair<std::string, std::string>> pinned_array_parameters;
        std::unordered_set<std::string> null_checked_pinned_arrays;
        call_arguments.reserve(function_decl.parameters.size());
        for (std::size_t index = 0; index < function_decl.parameters.size(); ++index)
        {
            if (const auto source_index = inferred_c_array_source_index(function_decl, index); source_index.has_value())
            {
                const std::string source_argument_expr = nullable_parameter_unwrap_expression(
                    module_decl, function_decl, function_decl.parameters[*source_index], *source_index);
                call_arguments.push_back(std::format("checked((int){}.Length)", source_argument_expr));
                continue;
            }

            const auto& parameter = function_decl.parameters[index];
            const std::string managed_argument_expr =
                nullable_parameter_unwrap_expression(module_decl, function_decl, parameter, index);
            if (parameter_is_supported_c_array_int(parameter))
            {
                const std::string pinned_name = std::format("__csbind23_arg{}_pinned", index);
                pinned_array_parameters.emplace_back(pinned_name, parameter.name);
                call_arguments.push_back(std::format("(System.IntPtr){}", pinned_name));
                continue;
            }

            if (const auto fixed_extent = fixed_std_array_int_extent(parameter); fixed_extent.has_value())
            {
                generated.append_line_format("        if ({} == null)", parameter.name);
                generated.append_line("        {");
                generated.append_line_format("            throw new System.ArgumentNullException(nameof({}));", parameter.name);
                generated.append_line("        }");
                generated.append_line_format("        if ({}.Length != {})", parameter.name, *fixed_extent);
                generated.append_line("        {");
                generated.append_line_format(
                    "            throw new System.ArgumentException($\"Expected array length {}\", nameof({}));",
                    *fixed_extent,
                    parameter.name);
                generated.append_line("        }");

                const std::string pinned_name = std::format("__csbind23_arg{}_pinned", index);
                pinned_array_parameters.emplace_back(pinned_name, parameter.name);
                null_checked_pinned_arrays.insert(parameter.name);
                call_arguments.push_back(std::format("(System.IntPtr){}", pinned_name));
                continue;
            }

            if (!parameter.type.managed_to_pinvoke_expression.empty())
            {
                const std::string pinvoke_name = std::format("__csbind23_arg{}_pinvoke", index);
                append_embedded_assignment(
                    generated,
                    "        ",
                    std::format("{} {} = ", parameter.type.pinvoke_name, pinvoke_name),
                    render_inline_template(parameter.type.managed_to_pinvoke_expression, managed_argument_expr, pinvoke_name,
                        managed_argument_expr, module_decl.name));
                call_arguments.push_back(pinvoke_name);
                if (!parameter.type.managed_finalize_to_pinvoke_statement.empty())
                {
                    finalize_statements.push_back(render_inline_template(
                        parameter.type.managed_finalize_to_pinvoke_statement, managed_argument_expr, pinvoke_name, pinvoke_name,
                        module_decl.name));
                }
                continue;
            }

            call_arguments.push_back(parameter_call_argument(parameter, managed_argument_expr));
        }

        std::string native_call;
        std::vector<std::string> default_variant_has_value_expressions;
        if (defaults == 0)
        {
            const std::string call_arguments_rendered = join_arguments(call_arguments);
            native_call = std::format("{}Native.{}({})", module_decl.name, native_name, call_arguments_rendered);
        }
        else
        {
            default_variant_has_value_expressions.reserve(defaults);
            for (std::size_t index = optional_start; index < function_decl.parameters.size(); ++index)
            {
                default_variant_has_value_expressions.push_back(optional_parameter_has_value_expression(
                    module_decl, function_decl, function_decl.parameters[index], index));
            }
        }
        const bool has_return_converter = !function_decl.return_type.managed_from_pinvoke_expression.empty();
        const bool has_return_finalize = !function_decl.return_type.managed_finalize_from_pinvoke_statement.empty();
        const bool needs_finally = !finalize_statements.empty() || has_return_finalize;

        for (const auto& pinned_parameter : pinned_array_parameters)
        {
            if (null_checked_pinned_arrays.contains(pinned_parameter.second))
            {
                continue;
            }
            generated.append_line_format("        if ({} == null)", pinned_parameter.second);
            generated.append_line("        {");
            generated.append_line_format("            throw new System.ArgumentNullException(nameof({}));", pinned_parameter.second);
            generated.append_line("        }");
        }
        if (!pinned_array_parameters.empty())
        {
            generated.append_line("        unsafe");
            generated.append_line("        {");
            for (const auto& pinned_parameter : pinned_array_parameters)
            {
                generated.append_line_format("            fixed (int* {} = {})", pinned_parameter.first, pinned_parameter.second);
                generated.append_line("            {");
            }
        }

        const ClassDecl* polymorphic_return_class = find_pointer_class_return(module_decl, function_decl);

        if (return_type == "void")
        {
            if (!needs_finally)
            {
                if (defaults == 0)
                {
                    generated.append_line_format("        {};", native_call);
                }
                else
                {
                    append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                        call_arguments, defaults, default_variant_has_value_expressions, "", false);
                }
            }
            else
            {
                generated.append_line("        try");
                generated.append_line("        {");
                if (defaults == 0)
                {
                    generated.append_line_format("            {};", native_call);
                }
                else
                {
                    append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                        call_arguments, defaults, default_variant_has_value_expressions, "", false);
                }
                generated.append_line("        }");
                generated.append_line("        finally");
                generated.append_line("        {");
                for (const auto& finalize_statement : finalize_statements)
                {
                    append_embedded_statement(generated, "            ", finalize_statement);
                }
                generated.append_line("        }");
            }
        }
        else
        {
            if (polymorphic_return_class != nullptr)
            {
                const std::string native_result_name = "__csbind23_result_ptr";
                const std::string wrapped_result = std::format(
                    "{}Runtime.WrapPolymorphic_{}({}, {})",
                    module_decl.name,
                    polymorphic_return_class->name,
                    native_result_name,
                    item_ownership_literal(module_decl, false));

                if (!needs_finally)
                {
                    if (defaults == 0)
                    {
                        generated.append_line_format("        System.IntPtr {} = {};", native_result_name, native_call);
                    }
                    else
                    {
                        generated.append_line_format("        System.IntPtr {} = default!;", native_result_name);
                        append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
                    generated.append_line_format("        return {};", wrapped_result);
                }
                else
                {
                    generated.append_line("        try");
                    generated.append_line("        {");
                    if (defaults == 0)
                    {
                        generated.append_line_format("            System.IntPtr {} = {};", native_result_name, native_call);
                    }
                    else
                    {
                        generated.append_line_format("            System.IntPtr {} = default!;", native_result_name);
                        append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
                    generated.append_line_format("            return {};", wrapped_result);
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(generated, "            ", finalize_statement);
                    }
                    generated.append_line("        }");
                }
            }
            else if (!has_return_converter)
            {
                if (!needs_finally)
                {
                    if (defaults == 0)
                    {
                        generated.append_line_format("        return {};", native_call);
                    }
                    else
                    {
                        append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, "", true);
                    }
                }
                else
                {
                    generated.append_line("        try");
                    generated.append_line("        {");
                    if (defaults == 0)
                    {
                        generated.append_line_format("            return {};", native_call);
                    }
                    else
                    {
                        append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, "", true);
                    }
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(generated, "            ", finalize_statement);
                    }
                    generated.append_line("        }");
                }
            }
            else
            {
                const std::string native_result_name = "__csbind23_result_pinvoke";
                const std::string managed_result_name = "__csbind23_result_managed";
                const std::string converted_expression =
                    render_inline_template(function_decl.return_type.managed_from_pinvoke_expression, managed_result_name, native_result_name,
                        native_result_name, module_decl.name);

                if (!needs_finally)
                {
                    if (defaults == 0)
                    {
                        generated.append_line_format(
                            "        {} {} = {};", function_decl.return_type.pinvoke_name, native_result_name, native_call);
                    }
                    else
                    {
                        generated.append_line_format(
                            "        {} {} = default!;", function_decl.return_type.pinvoke_name, native_result_name);
                        append_default_variant_if_chain(generated, "        ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
                    append_embedded_assignment(
                        generated,
                        "        ",
                        std::format("{} {} = ", return_type, managed_result_name),
                        converted_expression);
                    generated.append_line_format("        return {};", managed_result_name);
                }
                else
                {
                    generated.append_line_format(
                        "        {} {} = default!;", function_decl.return_type.pinvoke_name, native_result_name);
                    generated.append_line_format("        {} {} = default!;", return_type, managed_result_name);
                    generated.append_line("        try");
                    generated.append_line("        {");
                    if (defaults == 0)
                    {
                        generated.append_line_format("            {} = {};", native_result_name, native_call);
                    }
                    else
                    {
                        append_default_variant_if_chain(generated, "            ", module_decl.name, native_name,
                            call_arguments, defaults, default_variant_has_value_expressions, native_result_name, false);
                    }
                    append_embedded_assignment(
                        generated,
                        "            ",
                        std::format("{} = ", managed_result_name),
                        converted_expression);
                    generated.append_line_format("            return {};", managed_result_name);
                    generated.append_line("        }");
                    generated.append_line("        finally");
                    generated.append_line("        {");
                    for (const auto& finalize_statement : finalize_statements)
                    {
                        append_embedded_statement(generated, "            ", finalize_statement);
                    }
                    if (has_return_finalize)
                    {
                        append_embedded_statement(generated, "            ", render_inline_template(
                            function_decl.return_type.managed_finalize_from_pinvoke_statement, managed_result_name,
                            native_result_name, native_result_name, module_decl.name));
                    }
                    generated.append_line("        }");
                }
            }
        }

        if (!pinned_array_parameters.empty())
        {
            for (std::size_t index = 0; index < pinned_array_parameters.size(); ++index)
            {
                generated.append_line("            }");
            }
            generated.append_line("        }");
        }

        generated.append_line("    }");
        generated.append_line();
    }

    const auto generic_function_groups = collect_generic_function_groups(module_decl.functions);
    for (const auto& group : generic_function_groups)
    {
        append_free_generic_dispatch_wrapper(generated, module_decl, group);
    }
    generated.append_line("}");
    generated.append_line();

    const auto& written = generated.str();
    generated_files.push_back(write_csharp_file(output_root, module_decl.name + ".g.cs", written));

    for (const auto& class_decl : module_decl.classes)
    {
        if (class_decl.is_generic_instantiation)
        {
            continue;
        }

        TextWriter class_file(2048);
        class_file.append_line_format("namespace {};", csharp_namespace_name(module_decl, class_decl));
        class_file.append_line();
        append_wrapper_class(class_file, module_decl, module_decl.name, class_decl);

        const auto& class_written = class_file.str();
        generated_files.push_back(
            write_csharp_file(output_root, module_decl.name + "." + managed_class_name(module_decl, class_decl) + ".g.cs", class_written));
    }

    const auto generic_class_groups = collect_generic_class_groups(module_decl.classes);
    for (const auto& generic_class_group : generic_class_groups)
    {
        TextWriter generic_class_file(2048);
        generic_class_file.append_line_format(
            "namespace {};", csharp_namespace_name(module_decl, *generic_class_group.instantiations.front()));
        generic_class_file.append_line();
        append_generic_class_wrapper(generic_class_file, module_decl, generic_class_group);

        const auto& generic_class_written = generic_class_file.str();
        generated_files.push_back(write_csharp_file(
            output_root,
            module_decl.name + "." + managed_name(module_decl, CSharpNameKind::Class, generic_class_group.name)
                + ".g.cs",
            generic_class_written));
    }

    return generated_files;
}

} // namespace csbind23::emit
