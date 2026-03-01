#include "generic_dispatch_utils.hpp"

#include "csharp_emitter_text_utils.hpp"

#include <algorithm>
#include <format>
#include <unordered_map>

namespace csbind23::emit
{
namespace
{

std::string wrapper_type_name(const TypeRef& type_ref)
{
    if (type_ref.has_managed_converter() && !type_ref.managed_type_name.empty())
    {
        return type_ref.managed_type_name;
    }
    return type_ref.pinvoke_name;
}

std::string managed_name(const ModuleDecl& module_decl, CSharpNameKind kind, std::string_view raw_name)
{
    return format_csharp_name(module_decl, kind, raw_name);
}

bool same_wrapper_type_shape(const TypeRef& lhs, const TypeRef& rhs)
{
    return wrapper_type_name(lhs) == wrapper_type_name(rhs)
        && lhs.is_const == rhs.is_const
        && lhs.is_pointer == rhs.is_pointer
        && lhs.is_reference == rhs.is_reference;
}

std::string generic_expected_type_tuples(const GenericFunctionGroup& group, const GenericDispatchShape& shape)
{
    if (shape.generic_arity == 0)
    {
        return {};
    }

    std::vector<std::string> tuples;
    tuples.reserve(group.instantiations.size());
    for (const auto* instantiation : group.instantiations)
    {
        std::string tuple = "(";
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            if (slot > 0)
            {
                tuple += ", ";
            }
            tuple += concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
        }
        tuple += ")";
        tuples.push_back(std::move(tuple));
    }

    std::sort(tuples.begin(), tuples.end());
    tuples.erase(std::unique(tuples.begin(), tuples.end()), tuples.end());
    return join_arguments(tuples);
}

} // namespace

std::string csharp_string_literal_escape(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size() + 16);
    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

std::string generic_type_parameter_name(std::size_t index, std::size_t generic_arity)
{
    if (generic_arity == 1)
    {
        return "T";
    }

    return std::format("T{}", index);
}

std::string generic_type_parameter_list(std::size_t generic_arity)
{
    std::string rendered;
    for (std::size_t index = 0; index < generic_arity; ++index)
    {
        if (index > 0)
        {
            rendered += ", ";
        }
        rendered += generic_type_parameter_name(index, generic_arity);
    }
    return rendered;
}

std::string concrete_type_for_slot(
    const FunctionDecl& instantiation, const GenericDispatchShape& shape, int slot_id)
{
    for (std::size_t index = 0; index < instantiation.parameters.size(); ++index)
    {
        if (shape.generic_parameter_slot_ids[index] == slot_id)
        {
            return wrapper_type_name(instantiation.parameters[index].type);
        }
    }

    if (shape.generic_return_slot_id == slot_id)
    {
        return wrapper_type_name(instantiation.return_type);
    }

    return {};
}

GenericDispatchShape analyze_generic_dispatch_shape(const GenericFunctionGroup& group)
{
    GenericDispatchShape shape;
    if (group.instantiations.empty())
    {
        return shape;
    }

    const FunctionDecl& first = *group.instantiations.front();
    const std::size_t parameter_count = first.parameters.size();
    shape.generic_parameter_slot_ids.assign(parameter_count, -1);

    for (const auto* instantiation : group.instantiations)
    {
        if (instantiation->parameters.size() != parameter_count)
        {
            return GenericDispatchShape{};
        }
    }

    std::vector<bool> varying_parameters(parameter_count, false);
    bool has_unsupported_varying_shape = false;

    for (std::size_t index = 0; index < parameter_count; ++index)
    {
        bool all_same = true;
        const auto& first_parameter = first.parameters[index];
        for (const auto* instantiation : group.instantiations)
        {
            const auto& parameter = instantiation->parameters[index];
            if (!same_wrapper_type_shape(parameter.type, first_parameter.type))
            {
                all_same = false;
            }
        }

        if (!all_same)
        {
            varying_parameters[index] = true;
        }
    }

    bool varying_return = false;
    bool return_all_same = true;
    for (const auto* instantiation : group.instantiations)
    {
        const auto& return_type = instantiation->return_type;
        if (!same_wrapper_type_shape(return_type, first.return_type))
        {
            return_all_same = false;
        }
    }

    if (!return_all_same)
    {
        varying_return = true;
    }

    bool saw_generic_slot = varying_return;
    for (const bool varying_parameter : varying_parameters)
    {
        saw_generic_slot = saw_generic_slot || varying_parameter;
    }

    if (!saw_generic_slot)
    {
        return GenericDispatchShape{};
    }

    std::unordered_map<std::string, int> slot_groups;
    auto assign_slot_group = [&](std::size_t index, bool is_return_slot) {
        std::string key;
        for (const auto* instantiation : group.instantiations)
        {
            const std::string slot_type = is_return_slot
                ? wrapper_type_name(instantiation->return_type)
                : wrapper_type_name(instantiation->parameters[index].type);
            key += slot_type;
            key += "|";
        }

        auto it = slot_groups.find(key);
        if (it != slot_groups.end())
        {
            return it->second;
        }

        const int group_id = static_cast<int>(slot_groups.size());
        slot_groups.emplace(std::move(key), group_id);
        return group_id;
    };

    for (std::size_t index = 0; index < parameter_count; ++index)
    {
        if (!varying_parameters[index])
        {
            continue;
        }

        shape.generic_parameter_slot_ids[index] = assign_slot_group(index, false);
    }

    if (varying_return)
    {
        shape.generic_return_slot_id = assign_slot_group(0, true);
    }

    shape.generic_arity = slot_groups.size();
    if (shape.generic_arity == 0)
    {
        return GenericDispatchShape{};
    }

    for (const auto* instantiation : group.instantiations)
    {
        for (std::size_t slot = 0; slot < shape.generic_arity; ++slot)
        {
            const std::string concrete_type =
                concrete_type_for_slot(*instantiation, shape, static_cast<int>(slot));
            if (concrete_type.empty())
            {
                return GenericDispatchShape{};
            }
        }
    }

    shape.supported = saw_generic_slot && !has_unsupported_varying_shape;
    return shape;
}

std::string generic_dispatch_not_supported_message(
    const ModuleDecl& module_decl, CSharpNameKind name_kind, const GenericFunctionGroup& group,
    const GenericDispatchShape& shape)
{
    std::string message = std::format(
        "No generic mapping for {} with provided generic type arguments.",
        managed_name(module_decl, name_kind, group.name));

    const std::string expected_tuples = generic_expected_type_tuples(group, shape);
    if (!expected_tuples.empty())
    {
        message += std::format(" Expected type tuples: {}.", expected_tuples);
    }

    if (!shape.supported && !shape.unsupported_reason.empty())
    {
        message += std::format(" Dispatch shape is not supported: {}.", shape.unsupported_reason);
    }

    return message;
}

} // namespace csbind23::emit
