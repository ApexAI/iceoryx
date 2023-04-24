#ifndef IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_POSIX_SHARED_MEMORY_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_access_rights.hpp"
#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iox/attributes.hpp"
#include "iox/expected.hpp"
#include "iox/file_name.hpp"
#include "iox/filesystem.hpp"
#include "shared_memory_concept.hpp"
#include "shm_bump_allocator.hpp"
#include "shm_pointer.hpp"

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
        return SharedMemoryOpenError::OPEN_SHARED_MEMORY_FAILED;
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

template <typename AllocatorType>
class PosixSharedMemory
{
  public:
    using memory_type = posix::SharedMemoryObject;

    struct PosixShmConfig
    {
    };
    using Configuration = PosixShmConfig;
    using allocator_type = AllocatorType;

    PosixSharedMemory() = default;
    PosixSharedMemory(const PosixSharedMemory&) = delete;
    PosixSharedMemory& operator=(const PosixSharedMemory&) = delete;
    PosixSharedMemory(PosixSharedMemory&&) noexcept = default;
    PosixSharedMemory& operator=(PosixSharedMemory&&) noexcept = default;
    ~PosixSharedMemory() noexcept = default;

    const FileName& getName() const noexcept
    {
        return m_memory.getName();
    }

    uint64_t getSizeInBytes() const noexcept
    {
        return m_memory.get_size().expect("Failed to get shm size") - sizeof(ShmBumpAllocator);
    }

    uint64_t getStartAddress() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
        return reinterpret_cast<uint64_t>(m_memory.getBaseAddress()) + sizeof(ShmBumpAllocator);
    }

    expected<ShmPointer, SharedMemoryAllocationError> allocate(uint64_t size, uint64_t alignment) noexcept
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

    void deallocate(PtrDistance_t value) noexcept = delete;

    template <template <typename> class ShmConcept>
    friend class SharedMemoryCreator;
    // friend class SharedMemoryOpener;

  private:
    explicit PosixSharedMemory(posix::SharedMemoryObject&& memory) noexcept
        : m_memory(std::move(memory))
    {
    }

    posix::SharedMemoryObject m_memory;
};

// maybe not necessary any longer
//// struct PosixSharedMemoryConcept
//// {
////     template <typename AllocatorType>
////     using memory_type = PosixSharedMemory<AllocatorType>;
//// };

template <>
template <typename AllocatorType>
expected<PosixSharedMemory<AllocatorType>, SharedMemoryCreationError>
// expected<typename PosixSharedMemory<AllocatorType>::memory_type, SharedMemoryCreationError>
SharedMemoryCreator<PosixSharedMemory>::create(
    const FileName& name,
    const typename PosixSharedMemory<AllocatorType>::memory_type::Configuration&,
    const typename AllocatorType::Configuration&) noexcept
{
    // check configurations

    if (m_memorySizeInBytes == 0)
    {
        IOX_LOG(WARN) << "Cannot acquire memory of size 0.";
        return error<SharedMemoryCreationError>(SharedMemoryCreationError::REQUESTED_ZERO_SIZED_MEMORY);
    }

    // overflow unrealistic
    uint64_t effective_memory_size = m_memorySizeInBytes + sizeof(AllocatorType);

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name.as_string())
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
        AllocatorType(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(sharedMemoryObject->getBaseAddress())
                                              + sizeof(AllocatorType)),
                      m_memorySizeInBytes);

    return success<PosixSharedMemory<AllocatorType>>(PosixSharedMemory<AllocatorType>(std::move(*sharedMemoryObject)));
}

} // namespace cal
} // namespace iox

#endif

