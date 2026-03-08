#pragma once

#include <cstddef>

namespace csbind23::detail
{

void* temporary_memory_malloc(std::size_t size);
void temporary_memory_free(void* ptr) noexcept;

} // namespace csbind23::detail
