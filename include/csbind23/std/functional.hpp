#pragma once

#include "csbind23/cabi/converter.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace csbind23::cabi::detail
{

template <typename Type>
using callable_param_c_abi_type_t = decltype(to_c_abi_for<Type>(std::declval<Type>()));

template <typename Type>
using callable_return_c_abi_type_t =
    std::conditional_t<std::is_void_v<Type>, void, decltype(to_c_abi_for<Type>(std::declval<Type>()))>;

template <typename ReturnType, typename... Args>
class StdFunctionBridge
{
public:
    using cpp_type = std::function<ReturnType(Args...)>;
    using release_fn = void (*)(void*);
    using managed_invoke_fn = callable_return_c_abi_type_t<ReturnType> (*)(void*, callable_param_c_abi_type_t<Args>...);

    explicit StdFunctionBridge(cpp_type native_function)
        : native_function_(std::move(native_function))
    {
    }

    StdFunctionBridge(managed_invoke_fn managed_invoke, void* managed_lifetime, release_fn managed_release)
        : managed_invoke_(managed_invoke)
        , managed_lifetime_(managed_lifetime)
        , managed_release_(managed_release)
    {
    }

    ~StdFunctionBridge()
    {
        if (managed_release_ != nullptr && managed_lifetime_ != nullptr)
        {
            managed_release_(managed_lifetime_);
        }
    }

    void add_ref()
    {
        ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void release()
    {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }

    ReturnType invoke(Args... args) const
    {
        if (native_function_)
        {
            if constexpr (std::is_void_v<ReturnType>)
            {
                native_function_(std::forward<Args>(args)...);
                return;
            }
            else
            {
                return native_function_(std::forward<Args>(args)...);
            }
        }

        if constexpr (std::is_void_v<ReturnType>)
        {
            managed_invoke_(managed_lifetime_, to_c_abi_for<Args>(std::forward<Args>(args))...);
            return;
        }
        else
        {
            auto result = managed_invoke_(managed_lifetime_, to_c_abi_for<Args>(std::forward<Args>(args))...);
            return from_c_abi_for<ReturnType>(result);
        }
    }

private:
    std::atomic<std::size_t> ref_count_{1};
    cpp_type native_function_;
    managed_invoke_fn managed_invoke_ = nullptr;
    void* managed_lifetime_ = nullptr;
    release_fn managed_release_ = nullptr;
};

template <typename ReturnType, typename... Args>
void* create_std_function_bridge(std::function<ReturnType(Args...)> value)
{
    return static_cast<void*>(new StdFunctionBridge<ReturnType, Args...>(std::move(value)));
}

template <typename ReturnType, typename... Args>
void* create_managed_std_function_bridge(
    typename StdFunctionBridge<ReturnType, Args...>::managed_invoke_fn invoke,
    void* managed_lifetime,
    typename StdFunctionBridge<ReturnType, Args...>::release_fn release)
{
    return static_cast<void*>(new StdFunctionBridge<ReturnType, Args...>(invoke, managed_lifetime, release));
}

template <typename ReturnType, typename... Args>
std::function<ReturnType(Args...)> std_function_from_bridge(void* value)
{
    if (value == nullptr)
    {
        return {};
    }

    auto* bridge = static_cast<StdFunctionBridge<ReturnType, Args...>*>(value);
    bridge->add_ref();
    auto owner = std::shared_ptr<StdFunctionBridge<ReturnType, Args...>>(
        bridge, [](StdFunctionBridge<ReturnType, Args...>* ptr) { ptr->release(); });

    return [owner = std::move(owner)](Args... args) -> ReturnType {
        if constexpr (std::is_void_v<ReturnType>)
        {
            owner->invoke(std::forward<Args>(args)...);
            return;
        }
        else
        {
            return owner->invoke(std::forward<Args>(args)...);
        }
    };
}

template <typename ReturnType, typename... Args>
auto invoke_std_function_bridge(void* value, callable_param_c_abi_type_t<Args>... args)
    -> callable_return_c_abi_type_t<ReturnType>
{
    auto* bridge = static_cast<StdFunctionBridge<ReturnType, Args...>*>(value);
    if (bridge == nullptr)
    {
        if constexpr (std::is_void_v<ReturnType>)
        {
            return;
        }
        else
        {
            return callable_return_c_abi_type_t<ReturnType>{};
        }
    }

    if constexpr (std::is_void_v<ReturnType>)
    {
        bridge->invoke(from_c_abi_for<Args>(args)...);
        return;
    }
    else
    {
        return to_c_abi_for<ReturnType>(bridge->invoke(from_c_abi_for<Args>(args)...));
    }
}

template <typename ReturnType, typename... Args>
void release_std_function_bridge(void* value)
{
    auto* bridge = static_cast<StdFunctionBridge<ReturnType, Args...>*>(value);
    if (bridge != nullptr)
    {
        bridge->release();
    }
}

} // namespace csbind23::cabi::detail

namespace csbind23::cabi
{

template <typename ReturnType, typename... Args>
struct Converter<std::function<ReturnType(Args...)>>
{
    using cpp_type = std::function<ReturnType(Args...)>;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(cpp_type value)
    {
        return detail::create_std_function_bridge<ReturnType, Args...>(std::move(value));
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        return detail::std_function_from_bridge<ReturnType, Args...>(value);
    }
};

template <typename ReturnType, typename... Args>
struct Converter<std::function<ReturnType(Args...)>&>
{
    using cpp_type = std::function<ReturnType(Args...)>&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(const std::function<ReturnType(Args...)>& value)
    {
        return detail::create_std_function_bridge<ReturnType, Args...>(value);
    }

    static std::function<ReturnType(Args...)> from_c_abi(c_abi_type value)
    {
        return detail::std_function_from_bridge<ReturnType, Args...>(value);
    }
};

template <typename ReturnType, typename... Args>
struct Converter<const std::function<ReturnType(Args...)>&>
{
    using cpp_type = const std::function<ReturnType(Args...)>&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }

    static c_abi_type to_c_abi(const std::function<ReturnType(Args...)>& value)
    {
        return detail::create_std_function_bridge<ReturnType, Args...>(value);
    }

    static std::function<ReturnType(Args...)> from_c_abi(c_abi_type value)
    {
        return detail::std_function_from_bridge<ReturnType, Args...>(const_cast<void*>(value));
    }
};

} // namespace csbind23::cabi
