#ifndef IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_access_rights.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/expected.hpp"
#include "iox/string.hpp"
#include "shared_memory_concept.hpp"

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
SharedMemoryError translateError(const posix::SharedMemoryObjectError error)
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
expected<void*, SharedMemoryError>
SharedMemory<posix::SharedMemoryObject, BumpAllocator>::allocate(const uint64_t size, const uint64_t alignment)
{
    auto mem = m_memory.allocate(size, alignment);
    if (mem.has_error())
    {
        return error<SharedMemoryError>(SharedMemoryError::SHARED_MEMORY_ALLOCATION_ERROR);
    }
    return success<void*>(mem.value());
}

template <>
const Name_t& SharedMemory<posix::SharedMemoryObject, BumpAllocator>::getName() const
{
    return m_memory.getName();
}

template <>
uint64_t SharedMemory<posix::SharedMemoryObject, BumpAllocator>::getSizeInBytes() const
{
    return m_memory.get_size().expect("Failed to get shm size");
}

template <>
const void* SharedMemory<posix::SharedMemoryObject, BumpAllocator>::getStartAddress() const
{
    return m_memory.getBaseAddress();
}

template <>
SharedMemory<posix::SharedMemoryObject, BumpAllocator>::SharedMemory(posix::SharedMemoryObject&& memory)
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, BumpAllocator>, SharedMemoryError>
SharedMemoryCreator::create(const Name_t& name, const posix::SharedMemoryObject::Configuration& config)
{
    // check configuration, translate errors, ...

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(m_memorySizeInBytes)
                                  .permissions(m_permissions)
                                  .accessMode(m_accessMode)
                                  .openMode(posix::OpenMode::OPEN_OR_CREATE) // replace with m_user and m_group
                                  .create();

    if (!sharedMemoryObject)
    {
        return error<SharedMemoryError>(translateError(sharedMemoryObject.get_error()));
    }
    return success<SharedMemory<posix::SharedMemoryObject, BumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, BumpAllocator>(std::move(*sharedMemoryObject)));
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, BumpAllocator>, SharedMemoryError>
SharedMemoryOpener::open(const Name_t& name)
{
    // check requiredSize in SharedMemoryObject

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(m_requiredMemorySize) // replace with requiredMemorySize
                                  .permissions(perms::owner_all)           // remove?
                                  .accessMode(m_accessMode)
                                  .openMode(posix::OpenMode::OPEN_EXISTING)
                                  .create();

    if (!sharedMemoryObject)
    {
        return error<SharedMemoryError>(translateError(sharedMemoryObject.get_error()));
    }
    return success<SharedMemory<posix::SharedMemoryObject, BumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, BumpAllocator>(std::move(*sharedMemoryObject)));
}

} // namespace cal
} // namespace iox

#endif

