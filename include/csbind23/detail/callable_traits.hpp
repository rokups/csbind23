#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace csbind23::detail
{

template <typename Type> struct is_std_function : std::false_type
{
};

template <typename ReturnType, typename... Args>
struct is_std_function<std::function<ReturnType(Args...)>> : std::true_type
{
};

template <typename Type>
inline constexpr bool is_std_function_v = is_std_function<std::remove_cv_t<std::remove_reference_t<Type>>>::value;

template <typename Type>
inline constexpr bool is_function_pointer_v = std::is_pointer_v<std::remove_reference_t<Type>>
    && std::is_function_v<std::remove_pointer_t<std::remove_reference_t<Type>>>;

template <typename Type>
inline constexpr bool is_callable_v = is_std_function_v<Type> || is_function_pointer_v<Type>;

template <typename Type> struct CallableTraits;

template <typename ReturnType, typename... Args>
struct CallableTraits<ReturnType (*)(Args...)>
{
    using return_type = ReturnType;
    using args_tuple = std::tuple<Args...>;

    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
};

template <typename ReturnType, typename... Args>
struct CallableTraits<std::function<ReturnType(Args...)>>
{
    using return_type = ReturnType;
    using args_tuple = std::tuple<Args...>;

    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
};

template <typename Type>
using callable_traits_t = CallableTraits<std::conditional_t<is_function_pointer_v<Type>,
    std::remove_reference_t<Type>,
    std::remove_cv_t<std::remove_reference_t<Type>>>>;

} // namespace csbind23::detail
