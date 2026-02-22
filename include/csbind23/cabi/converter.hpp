#pragma once

#include <string>
#include <string_view>
#include <type_traits>

namespace csbind23::cabi {

template <typename Type>
struct Converter {
    using cpp_type = Type;
    using c_abi_type = Type;

    static constexpr std::string_view c_abi_type_name() {
        return "void*";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "System.IntPtr";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<void> {
    using cpp_type = void;
    using c_abi_type = void;

    static constexpr std::string_view c_abi_type_name() {
        return "void";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "void";
    }
};

template <>
struct Converter<bool> {
    using cpp_type = bool;
    using c_abi_type = bool;

    static constexpr std::string_view c_abi_type_name() {
        return "bool";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "bool";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<int> {
    using cpp_type = int;
    using c_abi_type = int;

    static constexpr std::string_view c_abi_type_name() {
        return "int";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "int";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<unsigned int> {
    using cpp_type = unsigned int;
    using c_abi_type = unsigned int;

    static constexpr std::string_view c_abi_type_name() {
        return "unsigned int";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "uint";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<long> {
    using cpp_type = long;
    using c_abi_type = long;

    static constexpr std::string_view c_abi_type_name() {
        return "long";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "nint";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<unsigned long> {
    using cpp_type = unsigned long;
    using c_abi_type = unsigned long;

    static constexpr std::string_view c_abi_type_name() {
        return "unsigned long";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "nuint";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<float> {
    using cpp_type = float;
    using c_abi_type = float;

    static constexpr std::string_view c_abi_type_name() {
        return "float";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "float";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<double> {
    using cpp_type = double;
    using c_abi_type = double;

    static constexpr std::string_view c_abi_type_name() {
        return "double";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "double";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value;
    }

    static cpp_type from_c_abi(const c_abi_type& value) {
        return value;
    }
};

template <>
struct Converter<std::string> {
    using cpp_type = std::string;
    using c_abi_type = const char*;

    static constexpr std::string_view c_abi_type_name() {
        return "const char*";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "string";
    }

    static c_abi_type to_c_abi(const cpp_type& value) {
        return value.c_str();
    }

    static cpp_type from_c_abi(c_abi_type value) {
        return value == nullptr ? std::string{} : std::string{value};
    }
};

template <typename Type>
struct Converter<Type*> {
    using cpp_type = Type*;
    using c_abi_type = std::conditional_t<std::is_const_v<Type>, const void*, void*>;

    static constexpr std::string_view c_abi_type_name() {
        return "void*";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "System.IntPtr";
    }

    static c_abi_type to_c_abi(cpp_type value) {
        return static_cast<c_abi_type>(value);
    }

    static cpp_type from_c_abi(c_abi_type value) {
        return static_cast<cpp_type>(const_cast<std::remove_const_t<Type>*>(static_cast<const std::remove_const_t<Type>*>(value)));
    }
};

template <typename Type>
struct Converter<Type&> {
    using cpp_type = Type&;
    using c_abi_type = std::conditional_t<std::is_const_v<Type>, const void*, void*>;

    static constexpr std::string_view c_abi_type_name() {
        return "void*";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "System.IntPtr";
    }

    static c_abi_type to_c_abi(cpp_type value) {
        return static_cast<c_abi_type>(&value);
    }

    static cpp_type from_c_abi(c_abi_type value) {
        return *static_cast<Type*>(const_cast<std::remove_const_t<Type>*>(static_cast<const std::remove_const_t<Type>*>(value)));
    }
};

template <typename Type>
struct Converter<const Type&> {
    using cpp_type = const Type&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() {
        return "const void*";
    }

    static constexpr std::string_view pinvoke_type_name() {
        return "System.IntPtr";
    }

    static c_abi_type to_c_abi(cpp_type value) {
        return static_cast<c_abi_type>(&value);
    }

    static cpp_type from_c_abi(c_abi_type value) {
        return *static_cast<const Type*>(value);
    }
};

template <typename Type>
std::string c_abi_type_name_for() {
    using Bare = std::remove_cv_t<std::remove_reference_t<Type>>;
    if constexpr (std::is_pointer_v<Bare> || std::is_reference_v<Type>) {
        if constexpr (std::is_const_v<std::remove_pointer_t<Bare>>) {
            return "const void*";
        }
        return "void*";
    }
    return std::string(Converter<Bare>::c_abi_type_name());
}

template <typename Type>
std::string pinvoke_type_name_for() {
    using Bare = std::remove_cv_t<std::remove_reference_t<Type>>;
    if constexpr (std::is_pointer_v<Bare> || std::is_reference_v<Type>) {
        return "System.IntPtr";
    }
    return std::string(Converter<Bare>::pinvoke_type_name());
}

} // namespace csbind23::cabi
