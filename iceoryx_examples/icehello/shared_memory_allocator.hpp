#include "shared_memory_concept.hpp"
#include "shm_bump_allocator.hpp"

namespace iox
{
namespace cal
{
SharedMemoryError translateAllocatorToShmError(const ShmBumpAllocatorError error)
{
    if (error == ShmBumpAllocatorError::OUT_OF_MEMORY)
    {
        return SharedMemoryError::SHARED_MEMORY_ALLOCATION_ERROR;
    }
    if (error == ShmBumpAllocatorError::REQUESTED_ZERO_SIZED_MEMORY)
    {
        return SharedMemoryError::SHARED_MEMORY_ALLOCATION_ERROR;
    }
    return SharedMemoryError::UNKNOWN;
}

// implement concept
class SharedMemoryAllocator
{
};
} // namespace cal
} // namespace iox
