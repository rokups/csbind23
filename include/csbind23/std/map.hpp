#pragma once

#include "csbind23/bindings_generator.hpp"
#include "csbind23/std/detail.hpp"
#include "csbind23/std/string.hpp"

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace csbind23
{

template <typename MapType>
auto csbind23_associative_iterator_at(MapType& value, int index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= value.size())
    {
        throw std::out_of_range("map index out of range");
    }

    auto it = value.begin();
    for (int current = 0; current < index; ++current)
    {
        ++it;
    }
    return it;
}

template <typename MapType>
void csbind23_associative_add(MapType& value, typename MapType::key_type key, typename MapType::mapped_type item)
{
    value.emplace(std::move(key), std::move(item));
}

template <typename MapType>
bool csbind23_associative_contains_key(const MapType& value, typename MapType::key_type key)
{
    return value.contains(key);
}

template <typename MapType>
decltype(auto) csbind23_associative_get(const MapType& value, typename MapType::key_type key)
{
    using MappedType = typename MapType::mapped_type;

    if constexpr (std::is_arithmetic_v<std::remove_cv_t<MappedType>> || std::is_enum_v<std::remove_cv_t<MappedType>>
        || std::is_pointer_v<MappedType>)
    {
        return static_cast<MappedType>(value.at(key));
    }
    else
    {
        return static_cast<const MappedType&>(value.at(key));
    }
}

template <typename MapType>
void csbind23_associative_set(MapType& value, typename MapType::key_type key, typename MapType::mapped_type item)
{
    value.insert_or_assign(std::move(key), std::move(item));
}

template <typename MapType>
decltype(auto) csbind23_associative_get_key_at(const MapType& value, int index)
{
    using KeyType = typename MapType::key_type;
    const auto it = csbind23_associative_iterator_at(value, index);

    if constexpr (std::is_arithmetic_v<std::remove_cv_t<KeyType>> || std::is_enum_v<std::remove_cv_t<KeyType>>
        || std::is_pointer_v<KeyType>)
    {
        return static_cast<KeyType>(it->first);
    }
    else
    {
        return static_cast<const KeyType&>(it->first);
    }
}

template <typename MapType>
decltype(auto) csbind23_associative_get_value_at(const MapType& value, int index)
{
    using MappedType = typename MapType::mapped_type;
    const auto it = csbind23_associative_iterator_at(value, index);

    if constexpr (std::is_arithmetic_v<std::remove_cv_t<MappedType>> || std::is_enum_v<std::remove_cv_t<MappedType>>
        || std::is_pointer_v<MappedType>)
    {
        return static_cast<MappedType>(it->second);
    }
    else
    {
        return static_cast<const MappedType&>(it->second);
    }
}

template <typename MapType>
bool csbind23_associative_remove(MapType& value, typename MapType::key_type key)
{
    return value.erase(key) != 0;
}

} // namespace csbind23

namespace csbind23::detail
{

template <typename KeyType, typename ValueType>
std::string associative_container_cpp_type_name_for_args(std::string_view container_name)
{
    return std::string(container_name) + "<" + fully_qualified_cpp_type_name<KeyType>() + ", "
        + fully_qualified_cpp_type_name<ValueType>() + ">";
}

template <typename Spec>
struct AssociativeContainerCppTypeName;

template <typename KeyType, typename ValueType>
struct AssociativeContainerCppTypeName<TemplateArgs<KeyType, ValueType>>
{
    static std::string render(std::string_view container_name)
    {
        return associative_container_cpp_type_name_for_args<KeyType, ValueType>(container_name);
    }
};

template <typename... Specs>
CppSymbols make_associative_helper_cpp_symbols(std::string_view helper_name, std::string_view container_name)
{
    CppSymbols symbols;
    (symbols.values.push_back(
         "csbind23::" + std::string(helper_name) + "<"
         + AssociativeContainerCppTypeName<Specs>::render(container_name) + ">"),
        ...);
    return symbols;
}

template <typename... Specs>
using associative_map_builder_t = GenericClassBuilder<detail::instantiate_template_from_spec_t<std::map, Specs>...>;

template <typename... Specs>
using associative_unordered_map_builder_t =
    GenericClassBuilder<detail::instantiate_template_from_spec_t<std::unordered_map, Specs>...>;

} // namespace csbind23::detail

namespace csbind23::cabi::detail
{

template <typename ValueType> std::string associative_item_ownership_expression()
{
    using BareValueType = std::remove_cv_t<std::remove_reference_t<ValueType>>;
    if constexpr (std::is_pointer_v<BareValueType>)
    {
        return "ItemOwnership.Borrowed";
    }

    return "ItemOwnership.Owned";
}

template <typename Type> std::string associative_managed_type_name()
{
    using BareType = std::remove_cv_t<std::remove_reference_t<Type>>;

    if constexpr (std::is_same_v<BareType, std::string>)
    {
        return std::string(std_string_wrapper_managed_type_name());
    }

    if constexpr (requires { Converter<BareType>::managed_type_name(); })
    {
        const std::string managed_name = std::string(Converter<BareType>::managed_type_name());
        if (!managed_name.empty())
        {
            return managed_name;
        }
    }

    return std::string(Converter<BareType>::pinvoke_type_name());
}

template <typename KeyType, typename ValueType> std::string map_managed_type_name()
{
    return "global::CsBind23.Generated.Std.Map<" + associative_managed_type_name<KeyType>() + ", "
        + associative_managed_type_name<ValueType>() + ">";
}

template <typename KeyType, typename ValueType> std::string unordered_map_managed_type_name()
{
    return "global::CsBind23.Generated.Std.UnorderedMap<" + associative_managed_type_name<KeyType>() + ", "
        + associative_managed_type_name<ValueType>() + ">";
}

template <typename ManagedTypeNameFn, typename KeyType, typename ValueType>
std::string associative_managed_owned_expression(ManagedTypeNameFn managed_type_name)
{
    return managed_type_name.template operator()<KeyType, ValueType>() + ".FromOwnedHandle({value}, "
        + associative_item_ownership_expression<ValueType>() + ")";
}

template <typename ManagedTypeNameFn, typename KeyType, typename ValueType>
std::string associative_managed_borrowed_expression(ManagedTypeNameFn managed_type_name)
{
    return managed_type_name.template operator()<KeyType, ValueType>() + ".FromBorrowedHandle({value}, "
        + associative_item_ownership_expression<ValueType>() + ")";
}

template <typename ManagedTypeNameFn, typename KeyType, typename ValueType>
std::string associative_managed_destroy_expression(ManagedTypeNameFn managed_type_name)
{
    return managed_type_name.template operator()<KeyType, ValueType>()
        + ".DestroyOwnedHandle({pinvoke}, ({managed}?._itemOwnership ?? ItemOwnership.Owned))";
}

struct map_managed_type_name_fn
{
    template <typename KeyType, typename ValueType> std::string operator()() const
    {
        return map_managed_type_name<KeyType, ValueType>();
    }
};

struct unordered_map_managed_type_name_fn
{
    template <typename KeyType, typename ValueType> std::string operator()() const
    {
        return unordered_map_managed_type_name<KeyType, ValueType>();
    }
};

} // namespace csbind23::cabi::detail

namespace csbind23::cabi
{

template <typename KeyType, typename ValueType> struct Converter<std::map<KeyType, ValueType>>
{
    using cpp_type = std::map<KeyType, ValueType>;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::map_managed_type_name<KeyType, ValueType>(); }
    static std::string managed_to_pinvoke_expression()
    {
        return detail::map_managed_type_name<KeyType, ValueType>() + ".CloneOrCreateOwned({value})";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_owned_expression<detail::map_managed_type_name_fn, KeyType, ValueType>({});
    }
    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::associative_managed_destroy_expression<detail::map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return static_cast<c_abi_type>(new cpp_type(value));
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        if (value == nullptr)
        {
            return {};
        }
        return *static_cast<const cpp_type*>(value);
    }
};

template <typename KeyType, typename ValueType> struct Converter<std::map<KeyType, ValueType>&>
{
    using cpp_type = std::map<KeyType, ValueType>&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<std::map<KeyType, ValueType>*>(value); }
};

template <typename KeyType, typename ValueType> struct Converter<const std::map<KeyType, ValueType>&>
{
    using cpp_type = const std::map<KeyType, ValueType>&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value) { return *static_cast<const std::map<KeyType, ValueType>*>(value); }
};

template <typename KeyType, typename ValueType> struct Converter<std::map<KeyType, ValueType>*>
{
    using cpp_type = std::map<KeyType, ValueType>*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename KeyType, typename ValueType> struct Converter<const std::map<KeyType, ValueType>*>
{
    using cpp_type = const std::map<KeyType, ValueType>*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename KeyType, typename ValueType> struct Converter<std::unordered_map<KeyType, ValueType>>
{
    using cpp_type = std::unordered_map<KeyType, ValueType>;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::unordered_map_managed_type_name<KeyType, ValueType>(); }
    static std::string managed_to_pinvoke_expression()
    {
        return detail::unordered_map_managed_type_name<KeyType, ValueType>() + ".CloneOrCreateOwned({value})";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_owned_expression<detail::unordered_map_managed_type_name_fn, KeyType, ValueType>({});
    }
    static std::string managed_finalize_to_pinvoke_statement()
    {
        return detail::associative_managed_destroy_expression<detail::unordered_map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(const cpp_type& value)
    {
        return static_cast<c_abi_type>(new cpp_type(value));
    }

    static cpp_type from_c_abi(c_abi_type value)
    {
        if (value == nullptr)
        {
            return {};
        }
        return *static_cast<const cpp_type*>(value);
    }
};

template <typename KeyType, typename ValueType> struct Converter<std::unordered_map<KeyType, ValueType>&>
{
    using cpp_type = std::unordered_map<KeyType, ValueType>&;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::unordered_map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::unordered_map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value)
    {
        return *static_cast<std::unordered_map<KeyType, ValueType>*>(value);
    }
};

template <typename KeyType, typename ValueType> struct Converter<const std::unordered_map<KeyType, ValueType>&>
{
    using cpp_type = const std::unordered_map<KeyType, ValueType>&;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::unordered_map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::unordered_map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(&value); }
    static cpp_type from_c_abi(c_abi_type value)
    {
        return *static_cast<const std::unordered_map<KeyType, ValueType>*>(value);
    }
};

template <typename KeyType, typename ValueType> struct Converter<std::unordered_map<KeyType, ValueType>*>
{
    using cpp_type = std::unordered_map<KeyType, ValueType>*;
    using c_abi_type = void*;

    static constexpr std::string_view c_abi_type_name() { return "void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::unordered_map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::unordered_map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

template <typename KeyType, typename ValueType> struct Converter<const std::unordered_map<KeyType, ValueType>*>
{
    using cpp_type = const std::unordered_map<KeyType, ValueType>*;
    using c_abi_type = const void*;

    static constexpr std::string_view c_abi_type_name() { return "const void*"; }
    static constexpr std::string_view pinvoke_type_name() { return "System.IntPtr"; }
    static std::string managed_type_name() { return detail::unordered_map_managed_type_name<KeyType, ValueType>(); }
    static constexpr std::string_view managed_to_pinvoke_expression()
    {
        return "({value} == null ? System.IntPtr.Zero : {value}._cPtr.Handle)";
    }
    static std::string managed_from_pinvoke_expression()
    {
        return detail::associative_managed_borrowed_expression<detail::unordered_map_managed_type_name_fn, KeyType, ValueType>({});
    }

    static c_abi_type to_c_abi(cpp_type value) { return static_cast<c_abi_type>(value); }
    static cpp_type from_c_abi(c_abi_type value) { return static_cast<cpp_type>(value); }
};

} // namespace csbind23::cabi

namespace csbind23
{

template <typename... Specs>
detail::associative_map_builder_t<Specs...> add_map(BindingsGenerator& generator)
{
    auto module = detail::ensure_stl_module(generator);

    auto builder = module.class_generic<std::map, Specs...>("Map");
    builder.template ctor<>();
    builder.csharp_interface("System.Collections.Generic.IDictionary<T0, T1>");
    builder.csharp_interface("System.Collections.Generic.IReadOnlyDictionary<T0, T1>");
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::size; }>("size", Private{});
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::clear; }>("Clear");
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_add<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__add", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_add", "std::map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_contains_key<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "ContainsKey", detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_contains_key", "std::map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_get<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__get", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_get", "std::map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_set<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__set", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_set", "std::map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_get_key_at<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__get_key_at", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_get_key_at", "std::map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_get_value_at<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__get_value_at", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_get_value_at", "std::map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_remove<std::map<KeyType, ValueType>>;
        }, Specs>()...>(
        "Remove", detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_remove", "std::map"));
    return builder;
}

template <typename... Specs>
detail::associative_unordered_map_builder_t<Specs...> add_unordered_map(BindingsGenerator& generator)
{
    auto module = detail::ensure_stl_module(generator);

    auto builder = module.class_generic<std::unordered_map, Specs...>("UnorderedMap");
    builder.template ctor<>();
    builder.csharp_interface("System.Collections.Generic.IDictionary<T0, T1>");
    builder.csharp_interface("System.Collections.Generic.IReadOnlyDictionary<T0, T1>");
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::size; }>("size", Private{});
    builder.template def_generic<[]<typename ClassType>() { return &ClassType::clear; }>("Clear");
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_add<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__add", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_add", "std::unordered_map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_contains_key<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "ContainsKey", detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_contains_key", "std::unordered_map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_get<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__get", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_get", "std::unordered_map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_set<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__set", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_set", "std::unordered_map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_get_key_at<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__get_key_at", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_get_key_at", "std::unordered_map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_get_value_at<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "__get_value_at", Private{}, detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_get_value_at", "std::unordered_map"));
    builder.template def_generic<
        detail::resolve_nontype_from_spec<[]<typename KeyType, typename ValueType>() {
            return &csbind23::csbind23_associative_remove<std::unordered_map<KeyType, ValueType>>;
        }, Specs>()...>(
        "Remove", detail::make_associative_helper_cpp_symbols<Specs...>("csbind23_associative_remove", "std::unordered_map"));
    return builder;
}

} // namespace csbind23