#include "csbind23/detail/temporary_memory_allocator.hpp"

#include "OffsetAllocator/offsetAllocator.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>

namespace csbind23::detail
{

#ifndef CSBIND23_TEMPORARY_ALLOCATOR_SIZE
#define CSBIND23_TEMPORARY_ALLOCATOR_SIZE (1024u * 1024u)
#endif

std::uint32_t max_allocations_for_buffer(std::size_t bufferSize) noexcept
{
    const std::size_t scaled = std::max<std::size_t>(bufferSize / 256u, 256u);
    return static_cast<std::uint32_t>(std::min<std::size_t>(scaled, 16u * 1024u));
}

constexpr std::size_t kHeaderSize = sizeof(OffsetAllocator::Allocation);
constexpr std::size_t kBufferSize = CSBIND23_TEMPORARY_ALLOCATOR_SIZE;
static std::size_t kMaxAllocations = max_allocations_for_buffer(kBufferSize);

struct TemporaryMemoryAllocatorState
{
    explicit TemporaryMemoryAllocatorState()
        : allocator(kBufferSize, kMaxAllocations)
        , buffer(static_cast<std::byte*>(std::malloc(kBufferSize)))
    {
        if (buffer == nullptr)
        {
            throw std::bad_alloc();
        }
    }

    ~TemporaryMemoryAllocatorState()
    {
        std::free(buffer);
        buffer = nullptr;
    }

    OffsetAllocator::Allocator allocator;
    std::byte* buffer;
};

static thread_local TemporaryMemoryAllocatorState __csbind23_arg_allocator;

void* temporary_memory_malloc(std::size_t size)
{
    auto& s = __csbind23_arg_allocator;

    if (size == 0)
    {
        return nullptr;
    }

    const std::size_t totalSize = kHeaderSize + size;
    if (totalSize <= kBufferSize)
    {
        const OffsetAllocator::Allocation allocation = s.allocator.allocate(totalSize);
        if (allocation.offset != OffsetAllocator::Allocation::NO_SPACE)
        {
            auto* header = reinterpret_cast<OffsetAllocator::Allocation*>(s.buffer + allocation.offset);
            *header = allocation;
            return static_cast<void*>(reinterpret_cast<std::byte*>(header) + kHeaderSize);
        }
    }

    return std::malloc(size);
}

void temporary_memory_free(void* ptr) noexcept
{
    auto& s = __csbind23_arg_allocator;

    if (ptr == nullptr)
    {
        return;
    }

    auto* payload = static_cast<std::byte*>(ptr);
    std::byte* const begin = s.buffer;
    std::byte* const end = begin + kBufferSize;
    if (payload >= begin + kHeaderSize && payload < end)
    {
        auto* header = reinterpret_cast<OffsetAllocator::Allocation*>(payload - kHeaderSize);
        s.allocator.free(*header);
        return;
    }

    std::free(ptr);
}

} // namespace csbind23::detail
