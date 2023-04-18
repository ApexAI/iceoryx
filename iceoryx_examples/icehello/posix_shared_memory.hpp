#ifndef IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_access_rights.hpp"
#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iox/attributes.hpp"
#include "iox/expected.hpp"
#include "iox/filesystem.hpp"
#include "shared_memory_concept.hpp"
#include "shm_bump_allocator.hpp"
#include "shm_pointer.hpp"

// later: separate files for implementations, e.g. posix_shared_memory.hpp
// - concept_abstractions
//    - shared_memory
//      - concept.hpp
//      - posix_shared_memory.hpp
//      - posix_typed_memory.hpp
// - iceoryx_hoofs
//    - posix
//      - shared_memory
//        - shared_memory.hpp
//        - shared_memory_object.hpp
namespace iox
{
namespace cal
{
SharedMemoryCreationError translateCreationError(const posix::SharedMemoryObjectError error) noexcept
{
    switch (error)
    {
    case posix::SharedMemoryObjectError::SHARED_MEMORY_CREATION_FAILED:
        return SharedMemoryCreationError::SHARED_MEMORY_CREATION_FAILED;
    case posix::SharedMemoryObjectError::MAPPING_SHARED_MEMORY_FAILED:
        return SharedMemoryCreationError::MAPPING_SHARED_MEMORY_FAILED;
    case posix::SharedMemoryObjectError::UNABLE_TO_VERIFY_MEMORY_SIZE:
        IOX_FALLTHROUGH;
    case posix::SharedMemoryObjectError::INTERNAL_LOGIC_FAILURE:
        return SharedMemoryCreationError::INTERNAL_LOGIC_FAILURE;
    default:
        return SharedMemoryCreationError::UNKNOWN;
    }
}

SharedMemoryOpenError translateOpenError(const posix::SharedMemoryObjectError error) noexcept
{
    switch (error)
    {
    case posix::SharedMemoryObjectError::SHARED_MEMORY_CREATION_FAILED:
        return SharedMemoryOpenError::SHARED_MEMORY_CREATION_FAILED;
    case posix::SharedMemoryObjectError::MAPPING_SHARED_MEMORY_FAILED:
        return SharedMemoryOpenError::MAPPING_SHARED_MEMORY_FAILED;
    case posix::SharedMemoryObjectError::REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE:
        return SharedMemoryOpenError::REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE;
    case posix::SharedMemoryObjectError::INTERNAL_LOGIC_FAILURE:
        return SharedMemoryOpenError::INTERNAL_LOGIC_FAILURE;
    default:
        return SharedMemoryOpenError::UNKNOWN;
    }
}

template <>
const Name_t& SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::getName() const noexcept
{
    return m_memory.getName();
}

template <>
uint64_t SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::getSizeInBytes() const noexcept
{
    return m_memory.get_size().expect("Failed to get shm size") - sizeof(ShmBumpAllocator);
}

template <>
uint64_t SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::getStartAddress() const noexcept
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
    return reinterpret_cast<uint64_t>(m_memory.getBaseAddress()) + sizeof(ShmBumpAllocator);
}

template <>
expected<ShmPointer, SharedMemoryAllocationError>
SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::allocate(uint64_t size, uint64_t alignment) noexcept
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
    auto* allocator = reinterpret_cast<ShmBumpAllocator*>(m_memory.getBaseAddress());
    auto distance = allocator->allocate(size, alignment);
    if (distance.has_error())
    {
        switch (distance.get_error())
        {
        case ShmBumpAllocatorError::REQUESTED_ZERO_SIZED_MEMORY:
            IOX_LOG(WARN) << "Cannot allocate memory of size 0.";
            return error<SharedMemoryAllocationError>(SharedMemoryAllocationError::REQUESTED_ZERO_SIZED_MEMORY);
        case ShmBumpAllocatorError::OUT_OF_MEMORY:
            IOX_LOG(WARN) << "Insufficient memory available.";
            return error<SharedMemoryAllocationError>(SharedMemoryAllocationError::OUT_OF_MEMORY);
        default:
            IOX_LOG(WARN) << "Cannot allocate memory since an unknown error occured.";
            return error<SharedMemoryAllocationError>(SharedMemoryAllocationError::UNKNOWN);
        }
    }
    return success<ShmPointer>(ShmPointer(*distance, reinterpret_cast<void*>(getStartAddress() + *distance)));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
}

// Shall every process be able to deallocate whole memory?
template <>
void SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::deallocate(PtrDistance_t value) noexcept = delete;

template <>
SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::SharedMemory(posix::SharedMemoryObject&& memory) noexcept
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>, SharedMemoryCreationError>
SharedMemoryCreator::create(const Name_t& name,
                            const posix::SharedMemoryObject::Configuration&,
                            const ShmBumpAllocator::Configuration&) noexcept
{
    // check configurations

    // ****should be done in SharedMemoryObject
    if (m_memorySizeInBytes == 0)
    {
        IOX_LOG(WARN) << "Cannot acquire memory of size 0.";
        return error<SharedMemoryCreationError>(SharedMemoryCreationError::SHARED_MEMORY_CREATION_FAILED);
    }
    //   ****

    // overflow unrealistic
    uint64_t effective_memory_size = m_memorySizeInBytes + sizeof(ShmBumpAllocator);

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(effective_memory_size)
                                  .permissions(m_permissions)
                                  .accessMode(posix::AccessMode::READ_WRITE)
                                  .openMode(posix::OpenMode::EXCLUSIVE_CREATE)
                                  .create();

    if (!sharedMemoryObject)
    {
        IOX_LOG(WARN) << "SharedMemoryObject creation failed.";
        return error<SharedMemoryCreationError>(translateCreationError(sharedMemoryObject.get_error()));
    }

    new (sharedMemoryObject->getBaseAddress())
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
        ShmBumpAllocator(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(sharedMemoryObject->getBaseAddress())
                                                 + sizeof(ShmBumpAllocator)),
                         m_memorySizeInBytes);

    return success<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>(std::move(*sharedMemoryObject)));
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>, SharedMemoryOpenError>
SharedMemoryOpener::open(const Name_t& name) noexcept
{
    // check requiredSize in SharedMemoryObject
    // overflow unrealistic
    uint64_t effective_memory_size = m_requiredMemorySize + sizeof(ShmBumpAllocator);

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(effective_memory_size) // replace with requiredMemorySize()
                                  .permissions(perms::owner_all) // remove? Opener shouldn't not set permissions
                                  .accessMode(m_accessMode)
                                  .openMode(posix::OpenMode::OPEN_EXISTING)
                                  .create();

    if (!sharedMemoryObject)
    {
        IOX_LOG(WARN) << "Open SharedMemoryObject failed.";
        return error<SharedMemoryOpenError>(translateOpenError(sharedMemoryObject.get_error()));
    }

    return success<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>(std::move(*sharedMemoryObject)));
}

} // namespace cal
} // namespace iox

#endif

