#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#include "csbind23/cabi/converter.hpp"
#include "csbind23/ir.hpp"

namespace csbind23::detail {

constexpr std::string_view trim_prefix(std::string_view value) {
    while (!value.empty() && (value.front() == '&' || value.front() == ' ')) {
        value.remove_prefix(1);
    }
    return value;
}

inline std::string unqualified_name(std::string_view qualified_name) {
    const std::size_t last_scope = qualified_name.rfind("::");
    if (last_scope == std::string_view::npos) {
        return std::string(qualified_name);
    }
    return std::string(qualified_name.substr(last_scope + 2));
}

constexpr std::string_view extract_pretty_function_value(std::string_view pretty, std::string_view key) {
    const std::size_t key_pos = pretty.find(key);
    if (key_pos == std::string_view::npos) {
        return {};
    }

    const std::size_t value_begin = key_pos + key.size();
    std::size_t value_end = pretty.find(';', value_begin);
    if (value_end == std::string_view::npos) {
        value_end = pretty.find(']', value_begin);
    }

    if (value_end == std::string_view::npos || value_end <= value_begin) {
        return {};
    }

    return trim_prefix(pretty.substr(value_begin, value_end - value_begin));
}

template <typename Type>
std::string qualified_type_name() {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view pretty = __PRETTY_FUNCTION__;
    constexpr std::string_view key = "Type = ";
    constexpr std::string_view extracted = extract_pretty_function_value(pretty, key);
    if (!extracted.empty()) {
        return std::string(extracted);
    }
#endif
    return typeid(Type).name();
}

template <typename Type>
std::string unqualified_type_name() {
    return unqualified_name(qualified_type_name<Type>());
}

template <auto Function>
std::string function_symbol_name() {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view pretty = __PRETTY_FUNCTION__;
    constexpr std::string_view key = "Function = ";
    constexpr std::string_view extracted = extract_pretty_function_value(pretty, key);
    if (!extracted.empty()) {
        return std::string(extracted);
    }
#endif
    return {};
}

template <auto Function>
std::string function_export_name() {
    const std::string symbol = function_symbol_name<Function>();
    if (!symbol.empty()) {
        return unqualified_name(symbol);
    }
    return "function";
}

template <typename MethodPtr>
struct MethodPointerTraits;

template <typename ClassType, typename ReturnType, typename... Args>
struct MethodPointerTraits<ReturnType (ClassType::*)(Args...)> {
    using class_type = ClassType;
    using return_type = ReturnType;
    static constexpr bool is_const = false;
};

template <typename ClassType, typename ReturnType, typename... Args>
struct MethodPointerTraits<ReturnType (ClassType::*)(Args...) const> {
    using class_type = ClassType;
    using return_type = ReturnType;
    static constexpr bool is_const = true;
};

template <typename Type>
TypeRef make_type_ref();

template <typename Type>
std::string base_type_name() {
    using Decayed = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<Type>>>;

    if constexpr (std::is_same_v<Decayed, void>) {
        return "void";
    } else if constexpr (std::is_same_v<Decayed, bool>) {
        return "bool";
    } else if constexpr (std::is_same_v<Decayed, char>) {
        return "char";
    } else if constexpr (std::is_same_v<Decayed, signed char>) {
        return "signed char";
    } else if constexpr (std::is_same_v<Decayed, unsigned char>) {
        return "unsigned char";
    } else if constexpr (std::is_same_v<Decayed, short>) {
        return "short";
    } else if constexpr (std::is_same_v<Decayed, unsigned short>) {
        return "unsigned short";
    } else if constexpr (std::is_same_v<Decayed, int>) {
        return "int";
    } else if constexpr (std::is_same_v<Decayed, unsigned int>) {
        return "unsigned int";
    } else if constexpr (std::is_same_v<Decayed, long>) {
        return "long";
    } else if constexpr (std::is_same_v<Decayed, unsigned long>) {
        return "unsigned long";
    } else if constexpr (std::is_same_v<Decayed, long long>) {
        return "long long";
    } else if constexpr (std::is_same_v<Decayed, unsigned long long>) {
        return "unsigned long long";
    } else if constexpr (std::is_same_v<Decayed, float>) {
        return "float";
    } else if constexpr (std::is_same_v<Decayed, double>) {
        return "double";
    } else if constexpr (std::is_same_v<Decayed, std::string>) {
        return "std::string";
    } else {
        return qualified_type_name<Decayed>();
    }
}

template <typename Type>
std::string cpp_type_name() {
    TypeRef type_ref = make_type_ref<Type>();
    std::string full_name;

    if (type_ref.is_const) {
        full_name += "const ";
    }

    full_name += type_ref.cpp_name;

    if (type_ref.is_pointer) {
        full_name += "*";
    }

    if (type_ref.is_reference) {
        full_name += "&";
    }

    return full_name;
}

template <typename Type>
TypeRef make_type_ref() {
    TypeRef type_ref;
    type_ref.cpp_name = base_type_name<Type>();
    type_ref.c_abi_name = cabi::c_abi_type_name_for<Type>();
    type_ref.pinvoke_name = cabi::pinvoke_type_name_for<Type>();
    type_ref.is_const = std::is_const_v<std::remove_reference_t<Type>>;
    type_ref.is_pointer = std::is_pointer_v<std::remove_reference_t<Type>>;
    type_ref.is_reference = std::is_reference_v<Type>;
    return type_ref;
}

} // namespace csbind23::detail
