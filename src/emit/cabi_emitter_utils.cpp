#include "cabi_emitter_utils.hpp"

#include <format>

namespace csbind23::emit
{

std::string render_cpp_type(const TypeRef& type_ref)
{
    std::string rendered;
    if (type_ref.is_const)
    {
        rendered += "const ";
    }
    rendered += type_ref.cpp_name;
    if (type_ref.is_pointer)
    {
        rendered += "*";
    }
    if (type_ref.is_reference)
    {
        rendered += "&";
    }
    return rendered;
}

std::string render_converted_argument_name(std::size_t index)
{
    return std::format("__csbind23_arg{}_cpp", index);
}

std::string render_callback_argument_name(std::size_t index)
{
    return std::format("__csbind23_arg{}_c", index);
}

std::size_t normalized_trailing_default_count(const FunctionDecl& function_decl)
{
    return function_decl.trailing_default_argument_count > function_decl.parameters.size()
        ? function_decl.parameters.size()
        : function_decl.trailing_default_argument_count;
}

std::vector<ParameterDecl> leading_parameters(const FunctionDecl& function_decl, std::size_t parameter_count)
{
    const std::size_t clamped = parameter_count > function_decl.parameters.size()
        ? function_decl.parameters.size()
        : parameter_count;
    return std::vector<ParameterDecl>(function_decl.parameters.begin(), function_decl.parameters.begin() + clamped);
}

void append_converted_arguments(TextWriter& output, const std::vector<ParameterDecl>& parameters)
{
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const auto& parameter = parameters[index];
        const auto converted_name = render_converted_argument_name(index);
        const auto rendered_cpp_type = render_cpp_type(parameter.type);

        output.append_line_format(
            "    decltype(auto) {} = csbind23::cabi::detail::from_c_abi_for<{}>({});",
            converted_name,
            rendered_cpp_type,
            parameter.name);
    }
}

std::string render_call_arguments(const std::vector<ParameterDecl>& parameters)
{
    std::string arguments;
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        arguments += render_converted_argument_name(index);
        if (index + 1 < parameters.size())
        {
            arguments += ", ";
        }
    }
    return arguments;
}

std::string callback_typedef_name(
    const std::string& module_name, const std::string& class_name, const FunctionDecl& method_decl)
{
    return std::format("{}_{}_Callback_{}", module_name, class_name, method_decl.virtual_slot_name);
}

} // namespace csbind23::emit
