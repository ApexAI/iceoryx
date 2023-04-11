#ifndef IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_access_rights.hpp"
#include "iox/expected.hpp"
#include "iox/memory.hpp"
#include "iox/string.hpp"
#include "shared_memory_concept.hpp"
#include "shm_bump_allocator.hpp"

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
SharedMemoryError translateError(const posix::SharedMemoryObjectError error) noexcept
{
    if (error == posix::SharedMemoryObjectError::SHARED_MEMORY_CREATION_FAILED)
    {
        return SharedMemoryError::SHARED_MEMORY_CREATION_FAILED;
    }
    if (error == posix::SharedMemoryObjectError::MAPPING_SHARED_MEMORY_FAILED)
    {
        return SharedMemoryError::SHARED_MEMORY_CREATION_FAILED;
    }
    if (error == posix::SharedMemoryObjectError::INTERNAL_LOGIC_FAILURE)
    {
        return SharedMemoryError::INTERNAL_LOGIC_FAILURE;
    }
    return SharedMemoryError::UNKNOWN;
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
    return reinterpret_cast<uint64_t>(m_memory.getBaseAddress()) + sizeof(ShmBumpAllocator);
}

template <>
ShmPointer SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::allocate(uint64_t size,
                                                                               uint64_t alignment) noexcept
{
    // works when m_allocator->allocate() returns creator allocate address
    // -> m_allocator->allocate() should return distance so that this allocate() m_memory.getBaseAddress() + distance
    auto* allocator = reinterpret_cast<ShmBumpAllocator*>(m_memory.getBaseAddress());
    auto res = allocator->allocate(size, alignment);
    if (res.has_error())
    {
        return nullptr;
    }
    return reinterpret_cast<ShmPointer>(reinterpret_cast<uint64_t>(*res)
                                        + reinterpret_cast<uint64_t>(m_memory.getBaseAddress())
                                        + sizeof(ShmBumpAllocator));
}

// template <>
// void SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::deallocate(ShmPointer value) noexcept
//{
// m_allocator->deallocate();
//}

template <>
SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>::SharedMemory(posix::SharedMemoryObject&& memory) noexcept
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>, SharedMemoryError>
SharedMemoryCreator::create(const Name_t& name, const posix::SharedMemoryObject::Configuration& config) noexcept
{
    // check configuration

    // ****should be done in SharedMemoryObject
    if (m_memorySizeInBytes == 0)
    {
        IOX_LOG(WARN) << "Cannot acquire memory of size 0.";
        return error<SharedMemoryError>(SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);
    }
    //   ****

    // overflow unrealistic
    uint64_t effective_memory_size = m_memorySizeInBytes + sizeof(ShmBumpAllocator);

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(effective_memory_size)
                                  .permissions(m_permissions)
                                  .accessMode(posix::AccessMode::READ_WRITE)
                                  .openMode(posix::OpenMode::EXCLUSIVE_CREATE) // replace with m_user and m_group
                                  .create();

    if (!sharedMemoryObject)
    {
        return error<SharedMemoryError>(translateError(sharedMemoryObject.get_error()));
    }

    new (sharedMemoryObject->getBaseAddress())
        ShmBumpAllocator(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(sharedMemoryObject->getBaseAddress())
                                                 + sizeof(ShmBumpAllocator)),
                         m_memorySizeInBytes);

    return success<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>(std::move(*sharedMemoryObject)));
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>, SharedMemoryError>
SharedMemoryOpener::open(const Name_t& name) noexcept
{
    // check requiredSize in SharedMemoryObject
    // overflow unrealistic
    uint64_t effective_memory_size = m_requiredMemorySize + sizeof(ShmBumpAllocator);

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(effective_memory_size) // replace with requiredMemorySize
                                  .permissions(perms::owner_all)            // remove?
                                  .accessMode(m_accessMode)
                                  .openMode(posix::OpenMode::OPEN_EXISTING) // remove?
                                  .create();

    if (!sharedMemoryObject)
    {
        return error<SharedMemoryError>(translateError(sharedMemoryObject.get_error()));
    }

    return success<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>(std::move(*sharedMemoryObject)));
}

} // namespace cal
} // namespace iox

#endif

