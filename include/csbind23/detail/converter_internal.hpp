#pragma once

#include "csbind23/cabi/converter.hpp"

#include <string>
#include <string_view>
#include <type_traits>

namespace csbind23::cabi::detail
{

template <typename Type> std::string c_abi_type_name_for();

template <typename Type> constexpr bool uses_typed_indirection_v =
    std::is_arithmetic_v<std::remove_cv_t<Type>> || std::is_enum_v<std::remove_cv_t<Type>>;

template <typename Type> std::string cv_qualified_c_abi_scalar_name_for()
{
    using Bare = std::remove_cv_t<Type>;
    std::string type_name = c_abi_type_name_for<Bare>();
    if constexpr (std::is_const_v<Type>)
    {
        return "const " + type_name;
    }
    return type_name;
}

template <typename Type> std::string c_abi_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_pointer_v<NoRef> || std::is_reference_v<Type>)
    {
        if constexpr (std::is_reference_v<Type>)
        {
            if constexpr (std::is_const_v<NoRef>)
            {
                return "const void*";
            }
            return "void*";
        }

        if constexpr (std::is_pointer_v<NoRef>)
        {
            return std::string(Converter<NoRef>::c_abi_type_name());
        }

        if constexpr (std::is_const_v<std::remove_pointer_t<NoRef>>)
        {
            return "const void*";
        }
        return "void*";
    }
    return std::string(Converter<Bare>::c_abi_type_name());
}

template <typename Type> constexpr std::string_view managed_type_name_for()
{
    if constexpr (requires { Converter<Type>::managed_type_name(); })
    {
        return Converter<Type>::managed_type_name();
    }
    return "";
}

template <typename Type> constexpr std::string_view managed_to_pinvoke_expression_for()
{
    if constexpr (requires { Converter<Type>::managed_to_pinvoke_expression(); })
    {
        return Converter<Type>::managed_to_pinvoke_expression();
    }
    return "";
}

template <typename Type> constexpr std::string_view managed_from_pinvoke_expression_for()
{
    if constexpr (requires { Converter<Type>::managed_from_pinvoke_expression(); })
    {
        return Converter<Type>::managed_from_pinvoke_expression();
    }
    return "";
}

template <typename Type> constexpr std::string_view managed_finalize_to_pinvoke_statement_for()
{
    if constexpr (requires { Converter<Type>::managed_finalize_to_pinvoke_statement(); })
    {
        return Converter<Type>::managed_finalize_to_pinvoke_statement();
    }
    return "";
}

template <typename Type> constexpr std::string_view managed_finalize_from_pinvoke_statement_for()
{
    if constexpr (requires { Converter<Type>::managed_finalize_from_pinvoke_statement(); })
    {
        return Converter<Type>::managed_finalize_from_pinvoke_statement();
    }
    return "";
}

template <typename Type> std::string c_abi_param_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (uses_typed_indirection_v<NoRef>)
        {
            return cv_qualified_c_abi_scalar_name_for<NoRef>() + "*";
        }

        if constexpr (std::is_const_v<NoRef>)
        {
            return "const void*";
        }
        return "void*";
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        using Pointee = std::remove_pointer_t<NoRef>;
        if constexpr (uses_typed_indirection_v<Pointee>)
        {
            return cv_qualified_c_abi_scalar_name_for<Pointee>() + "*";
        }

        if constexpr (requires { Converter<NoRef>::c_abi_param_type_name(); })
        {
            return std::string(Converter<NoRef>::c_abi_param_type_name());
        }
        return c_abi_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::c_abi_param_type_name(); })
    {
        return std::string(Converter<Bare>::c_abi_param_type_name());
    }

    return c_abi_type_name_for<Type>();
}

template <typename Type> std::string c_abi_return_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (uses_typed_indirection_v<NoRef>)
        {
            return cv_qualified_c_abi_scalar_name_for<NoRef>() + "*";
        }

        if constexpr (std::is_const_v<NoRef>)
        {
            return "const void*";
        }
        return "void*";
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        using Pointee = std::remove_pointer_t<NoRef>;
        if constexpr (uses_typed_indirection_v<Pointee>)
        {
            return cv_qualified_c_abi_scalar_name_for<Pointee>() + "*";
        }

        if constexpr (requires { Converter<NoRef>::c_abi_return_type_name(); })
        {
            return std::string(Converter<NoRef>::c_abi_return_type_name());
        }
        return c_abi_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::c_abi_return_type_name(); })
    {
        return std::string(Converter<Bare>::c_abi_return_type_name());
    }

    return c_abi_type_name_for<Type>();
}

template <typename Type> std::string pinvoke_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_pointer_v<NoRef>)
    {
        return std::string(Converter<NoRef>::pinvoke_type_name());
    }
    if constexpr (std::is_reference_v<Type>)
    {
        return std::string(Converter<Type>::pinvoke_type_name());
    }
    return std::string(Converter<Bare>::pinvoke_type_name());
}

template <typename Type> std::string pinvoke_param_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::pinvoke_param_type_name(); })
        {
            return std::string(Converter<Type>::pinvoke_param_type_name());
        }
        return pinvoke_type_name_for<Type>();
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (requires { Converter<NoRef>::pinvoke_param_type_name(); })
        {
            return std::string(Converter<NoRef>::pinvoke_param_type_name());
        }
        return pinvoke_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::pinvoke_param_type_name(); })
    {
        return std::string(Converter<Bare>::pinvoke_param_type_name());
    }

    return pinvoke_type_name_for<Type>();
}

template <typename Type> std::string pinvoke_return_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_reference_v<Type>)
    {
        return "System.IntPtr";
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (requires { Converter<NoRef>::pinvoke_return_type_name(); })
        {
            return std::string(Converter<NoRef>::pinvoke_return_type_name());
        }
        return pinvoke_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::pinvoke_return_type_name(); })
    {
        return std::string(Converter<Bare>::pinvoke_return_type_name());
    }

    return pinvoke_type_name_for<Type>();
}

} // namespace csbind23::cabi::detail
