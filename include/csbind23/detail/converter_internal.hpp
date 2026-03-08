#pragma once

#include "csbind23/cabi/converter.hpp"

#include <format>
#include <string>
#include <string_view>
#include <type_traits>

namespace csbind23::cabi::detail
{

template <typename Type> std::string c_abi_type_name_for();

template <typename> inline constexpr bool always_false_v = false;

template <typename Type, typename HasCommon, typename HasParam, typename HasReturn> constexpr void validate_type_name_group()
{
    constexpr bool has_common = HasCommon{}.template operator()<Type>();
    constexpr bool has_param = HasParam{}.template operator()<Type>();
    constexpr bool has_return = HasReturn{}.template operator()<Type>();

    static_assert(!(has_common && (has_param || has_return)),
        "Converter type-name specification is invalid: use either *_type_name() or the *_param_type_name()/"
        "*_return_type_name() pair, but not both.");
    static_assert(has_param == has_return,
        "Converter type-name specification is invalid: *_param_type_name() and *_return_type_name() must be "
        "provided together.");
}

struct has_c_abi_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::c_abi_type_name(); };
    }
};

struct has_c_abi_param_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::c_abi_param_type_name(); };
    }
};

struct has_c_abi_return_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::c_abi_return_type_name(); };
    }
};

struct has_pinvoke_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::pinvoke_type_name(); };
    }
};

struct has_pinvoke_param_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::pinvoke_param_type_name(); };
    }
};

struct has_pinvoke_return_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::pinvoke_return_type_name(); };
    }
};

struct has_managed_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_type_name(); };
    }
};

struct has_managed_marshaller_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_marshaller_type_name(); };
    }
};

struct has_managed_param_marshaller_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_param_marshaller_type_name(); };
    }
};

struct has_managed_return_marshaller_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_return_marshaller_type_name(); };
    }
};

struct has_managed_pinvoke_attribute_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_pinvoke_attribute(); };
    }
};

struct has_managed_pinvoke_param_attribute_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_pinvoke_param_attribute(); };
    }
};

struct has_managed_pinvoke_return_attribute_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_pinvoke_return_attribute(); };
    }
};

struct has_managed_param_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_param_type_name(); };
    }
};

struct has_managed_return_type_name_fn
{
    template <typename Type> constexpr bool operator()() const
    {
        return requires { Converter<Type>::managed_return_type_name(); };
    }
};

template <typename Type> constexpr void validate_c_abi_type_names()
{
    validate_type_name_group<Type, has_c_abi_type_name_fn, has_c_abi_param_type_name_fn, has_c_abi_return_type_name_fn>();
}

template <typename Type> constexpr void validate_pinvoke_type_names()
{
    validate_type_name_group<Type, has_pinvoke_type_name_fn, has_pinvoke_param_type_name_fn, has_pinvoke_return_type_name_fn>();
}

template <typename Type> constexpr void validate_managed_type_names()
{
    validate_type_name_group<Type, has_managed_type_name_fn, has_managed_param_type_name_fn,
        has_managed_return_type_name_fn>();
}

template <typename Type> constexpr void validate_managed_marshaller_type_names()
{
    constexpr bool has_common = has_managed_marshaller_type_name_fn{}.template operator()<Type>();
    constexpr bool has_param = has_managed_param_marshaller_type_name_fn{}.template operator()<Type>();
    constexpr bool has_return = has_managed_return_marshaller_type_name_fn{}.template operator()<Type>();

    static_assert(!(has_common && (has_param || has_return)),
        "Converter marshaller specification is invalid: use either managed_marshaller_type_name() or the "
        "managed_param_marshaller_type_name()/managed_return_marshaller_type_name() overrides, but not both.");
}

template <typename Type> constexpr void validate_managed_pinvoke_attributes()
{
    constexpr bool has_common = has_managed_pinvoke_attribute_fn{}.template operator()<Type>();
    constexpr bool has_param = has_managed_pinvoke_param_attribute_fn{}.template operator()<Type>();
    constexpr bool has_return = has_managed_pinvoke_return_attribute_fn{}.template operator()<Type>();

    static_assert(!(has_common && (has_param || has_return)),
        "Converter P/Invoke attribute specification is invalid: use either managed_pinvoke_attribute() or the "
        "managed_pinvoke_param_attribute()/managed_pinvoke_return_attribute() overrides, but not both.");
}

template <typename Type> std::string converter_c_abi_type_name_or_empty()
{
    validate_c_abi_type_names<Type>();

    if constexpr (requires { Converter<Type>::c_abi_type_name(); })
    {
        return std::string(Converter<Type>::c_abi_type_name());
    }

    if constexpr (requires { Converter<Type>::c_abi_param_type_name(); })
    {
        return std::string(Converter<Type>::c_abi_param_type_name());
    }

    if constexpr (requires { Converter<Type>::c_abi_return_type_name(); })
    {
        return std::string(Converter<Type>::c_abi_return_type_name());
    }

    return {};
}

template <typename Type> std::string converter_pinvoke_type_name_or_empty()
{
    validate_pinvoke_type_names<Type>();

    if constexpr (requires { Converter<Type>::pinvoke_type_name(); })
    {
        return std::string(Converter<Type>::pinvoke_type_name());
    }

    if constexpr (requires { Converter<Type>::pinvoke_param_type_name(); })
    {
        return std::string(Converter<Type>::pinvoke_param_type_name());
    }

    if constexpr (requires { Converter<Type>::pinvoke_return_type_name(); })
    {
        return std::string(Converter<Type>::pinvoke_return_type_name());
    }

    return {};
}

template <typename Type> std::string converter_managed_type_name_or_empty()
{
    validate_managed_type_names<Type>();

    if constexpr (requires { Converter<Type>::managed_type_name(); })
    {
        return std::string(Converter<Type>::managed_type_name());
    }

    if constexpr (requires { Converter<Type>::managed_param_type_name(); })
    {
        return std::string(Converter<Type>::managed_param_type_name());
    }

    if constexpr (requires { Converter<Type>::managed_return_type_name(); })
    {
        return std::string(Converter<Type>::managed_return_type_name());
    }

    return {};
}

template <typename Type> std::string converter_managed_marshaller_type_name_or_empty()
{
    validate_managed_marshaller_type_names<Type>();

    if constexpr (requires { Converter<Type>::managed_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_marshaller_type_name());
    }

    if constexpr (requires { Converter<Type>::managed_param_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_param_marshaller_type_name());
    }

    if constexpr (requires { Converter<Type>::managed_return_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_return_marshaller_type_name());
    }

    return {};
}

template <typename Type> std::string converter_managed_pinvoke_attribute_or_empty()
{
    validate_managed_pinvoke_attributes<Type>();

    if constexpr (requires { Converter<Type>::managed_pinvoke_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_attribute());
    }

    if constexpr (requires { Converter<Type>::managed_pinvoke_param_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_param_attribute());
    }

    if constexpr (requires { Converter<Type>::managed_pinvoke_return_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_return_attribute());
    }

    return {};
}

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
            if (const std::string name = converter_c_abi_type_name_or_empty<NoRef>(); !name.empty())
            {
                return name;
            }
            return std::is_const_v<std::remove_pointer_t<NoRef>> ? "const void*" : "void*";
        }

        if constexpr (std::is_const_v<std::remove_pointer_t<NoRef>>)
        {
            return "const void*";
        }
        return "void*";
    }

    if (const std::string name = converter_c_abi_type_name_or_empty<Bare>(); !name.empty())
    {
        return name;
    }

    return "void*";
}

template <typename Type> std::string managed_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if (const std::string name = converter_managed_type_name_or_empty<Type>(); !name.empty())
    {
        return name;
    }

    if constexpr (!std::is_same_v<Bare, Type>)
    {
        if (const std::string name = converter_managed_type_name_or_empty<Bare>(); !name.empty())
        {
            return name;
        }
    }

    return {};
}

template <typename Type> std::string managed_param_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;

    validate_managed_type_names<Type>();

    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::managed_param_type_name(); })
        {
            return std::string(Converter<Type>::managed_param_type_name());
        }
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (requires { Converter<NoRef>::managed_param_type_name(); })
        {
            return std::string(Converter<NoRef>::managed_param_type_name());
        }
    }

    if constexpr (requires { Converter<Bare>::managed_param_type_name(); })
    {
        return std::string(Converter<Bare>::managed_param_type_name());
    }

    return managed_type_name_for<Type>();
}

template <typename Type> std::string managed_return_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;

    validate_managed_type_names<Type>();

    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::managed_return_type_name(); })
        {
            return std::string(Converter<Type>::managed_return_type_name());
        }
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (requires { Converter<NoRef>::managed_return_type_name(); })
        {
            return std::string(Converter<NoRef>::managed_return_type_name());
        }
    }

    if constexpr (requires { Converter<Bare>::managed_return_type_name(); })
    {
        return std::string(Converter<Bare>::managed_return_type_name());
    }

    return managed_type_name_for<Type>();
}

template <typename Type> std::string managed_param_marshaller_type_name_for()
{
    validate_managed_marshaller_type_names<Type>();

    if constexpr (requires { Converter<Type>::managed_param_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_param_marshaller_type_name());
    }

    if constexpr (requires { Converter<Type>::managed_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_marshaller_type_name());
    }

    return {};
}

template <typename Type> std::string managed_return_marshaller_type_name_for()
{
    validate_managed_marshaller_type_names<Type>();

    if constexpr (requires { Converter<Type>::managed_return_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_return_marshaller_type_name());
    }

    if constexpr (requires { Converter<Type>::managed_marshaller_type_name(); })
    {
        return std::string(Converter<Type>::managed_marshaller_type_name());
    }

    return {};
}

template <typename Type> std::string managed_param_pinvoke_attribute_for()
{
    validate_managed_pinvoke_attributes<Type>();

    if constexpr (requires { Converter<Type>::managed_pinvoke_param_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_param_attribute());
    }

    if constexpr (requires { Converter<Type>::managed_pinvoke_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_attribute());
    }

    if (const std::string marshaller = managed_param_marshaller_type_name_for<Type>(); !marshaller.empty())
    {
        return std::format("global::System.Runtime.InteropServices.Marshalling.MarshalUsing(typeof({}))", marshaller);
    }

    return {};
}

template <typename Type> std::string managed_return_pinvoke_attribute_for()
{
    validate_managed_pinvoke_attributes<Type>();

    if constexpr (requires { Converter<Type>::managed_pinvoke_return_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_return_attribute());
    }

    if constexpr (requires { Converter<Type>::managed_pinvoke_attribute(); })
    {
        return std::string(Converter<Type>::managed_pinvoke_attribute());
    }

    if (const std::string marshaller = managed_return_marshaller_type_name_for<Type>(); !marshaller.empty())
    {
        return std::format("global::System.Runtime.InteropServices.Marshalling.MarshalUsing(typeof({}))", marshaller);
    }

    return {};
}

template <typename Type> std::string managed_to_pinvoke_expression_for()
{
    if constexpr (requires { Converter<Type>::managed_to_pinvoke_expression(); })
    {
        return std::string(Converter<Type>::managed_to_pinvoke_expression());
    }
    return {};
}

template <typename Type> std::string managed_from_pinvoke_expression_for()
{
    if constexpr (requires { Converter<Type>::managed_from_pinvoke_expression(); })
    {
        return std::string(Converter<Type>::managed_from_pinvoke_expression());
    }
    return {};
}

template <typename Type> std::string managed_finalize_to_pinvoke_statement_for()
{
    if constexpr (requires { Converter<Type>::managed_finalize_to_pinvoke_statement(); })
    {
        return std::string(Converter<Type>::managed_finalize_to_pinvoke_statement());
    }
    return {};
}

template <typename Type> std::string managed_finalize_from_pinvoke_statement_for()
{
    if constexpr (requires { Converter<Type>::managed_finalize_from_pinvoke_statement(); })
    {
        return std::string(Converter<Type>::managed_finalize_from_pinvoke_statement());
    }
    return {};
}

template <typename Type> std::string c_abi_param_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    validate_c_abi_type_names<Type>();

    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::c_abi_param_type_name(); })
        {
            return std::string(Converter<Type>::c_abi_param_type_name());
        }

        if constexpr (uses_typed_indirection_v<NoRef>)
        {
            return cv_qualified_c_abi_scalar_name_for<NoRef>() + "*";
        }

        if constexpr (requires { Converter<Type>::c_abi_type_name(); })
        {
            return std::string(Converter<Type>::c_abi_type_name());
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

        if constexpr (requires { Converter<NoRef>::c_abi_type_name(); })
        {
            return std::string(Converter<NoRef>::c_abi_type_name());
        }
        return c_abi_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::c_abi_param_type_name(); })
    {
        return std::string(Converter<Bare>::c_abi_param_type_name());
    }

    if constexpr (requires { Converter<Bare>::c_abi_type_name(); })
    {
        return std::string(Converter<Bare>::c_abi_type_name());
    }

    return c_abi_type_name_for<Type>();
}

template <typename Type> std::string c_abi_return_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    validate_c_abi_type_names<Type>();

    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::c_abi_return_type_name(); })
        {
            return std::string(Converter<Type>::c_abi_return_type_name());
        }

        if constexpr (uses_typed_indirection_v<NoRef>)
        {
            return cv_qualified_c_abi_scalar_name_for<NoRef>() + "*";
        }

        if constexpr (requires { Converter<Type>::c_abi_type_name(); })
        {
            return std::string(Converter<Type>::c_abi_type_name());
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

        if constexpr (requires { Converter<NoRef>::c_abi_type_name(); })
        {
            return std::string(Converter<NoRef>::c_abi_type_name());
        }
        return c_abi_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::c_abi_return_type_name(); })
    {
        return std::string(Converter<Bare>::c_abi_return_type_name());
    }

    if constexpr (requires { Converter<Bare>::c_abi_type_name(); })
    {
        return std::string(Converter<Bare>::c_abi_type_name());
    }

    return c_abi_type_name_for<Type>();
}

template <typename Type> std::string pinvoke_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    if constexpr (std::is_pointer_v<NoRef>)
    {
        if (const std::string name = converter_pinvoke_type_name_or_empty<NoRef>(); !name.empty())
        {
            return name;
        }
        return "System.IntPtr";
    }
    if constexpr (std::is_reference_v<Type>)
    {
        if (const std::string name = converter_pinvoke_type_name_or_empty<Type>(); !name.empty())
        {
            return name;
        }
        return "System.IntPtr";
    }

    if (const std::string name = converter_pinvoke_type_name_or_empty<Bare>(); !name.empty())
    {
        return name;
    }

    return "System.IntPtr";
}

template <typename Type> std::string pinvoke_param_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    validate_pinvoke_type_names<Type>();

    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::pinvoke_param_type_name(); })
        {
            return std::string(Converter<Type>::pinvoke_param_type_name());
        }

        if constexpr (requires { Converter<Type>::pinvoke_type_name(); })
        {
            return std::string(Converter<Type>::pinvoke_type_name());
        }
        return pinvoke_type_name_for<Type>();
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (requires { Converter<NoRef>::pinvoke_param_type_name(); })
        {
            return std::string(Converter<NoRef>::pinvoke_param_type_name());
        }

        if constexpr (requires { Converter<NoRef>::pinvoke_type_name(); })
        {
            return std::string(Converter<NoRef>::pinvoke_type_name());
        }
        return pinvoke_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::pinvoke_param_type_name(); })
    {
        return std::string(Converter<Bare>::pinvoke_param_type_name());
    }

    if constexpr (requires { Converter<Bare>::pinvoke_type_name(); })
    {
        return std::string(Converter<Bare>::pinvoke_type_name());
    }

    return pinvoke_type_name_for<Type>();
}

template <typename Type> std::string pinvoke_return_type_name_for()
{
    using NoRef = std::remove_reference_t<Type>;
    using Bare = std::remove_cv_t<NoRef>;
    validate_pinvoke_type_names<Type>();

    if constexpr (std::is_reference_v<Type>)
    {
        if constexpr (requires { Converter<Type>::pinvoke_return_type_name(); })
        {
            return std::string(Converter<Type>::pinvoke_return_type_name());
        }

        if constexpr (requires { Converter<Type>::pinvoke_type_name(); })
        {
            return std::string(Converter<Type>::pinvoke_type_name());
        }

        return "System.IntPtr";
    }

    if constexpr (std::is_pointer_v<NoRef>)
    {
        if constexpr (requires { Converter<NoRef>::pinvoke_return_type_name(); })
        {
            return std::string(Converter<NoRef>::pinvoke_return_type_name());
        }

        if constexpr (requires { Converter<NoRef>::pinvoke_type_name(); })
        {
            return std::string(Converter<NoRef>::pinvoke_type_name());
        }
        return pinvoke_type_name_for<Type>();
    }

    if constexpr (requires { Converter<Bare>::pinvoke_return_type_name(); })
    {
        return std::string(Converter<Bare>::pinvoke_return_type_name());
    }

    if constexpr (requires { Converter<Bare>::pinvoke_type_name(); })
    {
        return std::string(Converter<Bare>::pinvoke_type_name());
    }

    return pinvoke_type_name_for<Type>();
}

} // namespace csbind23::cabi::detail
