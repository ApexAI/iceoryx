#ifndef IOX_CONCEPTS_SHARED_MEMORY_CONCEPT_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_CONCEPT_HPP

#include "shm_bump_allocator.hpp"

#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iox/builder.hpp"
#include "iox/expected.hpp"
#include "iox/file_name.hpp"
#include "iox/filesystem.hpp"
#include "iox/string.hpp"
#include "shm_pointer.hpp"

namespace iox
{
namespace cal
{
using Name_t = string<platform::IOX_MAX_SHM_NAME_LENGTH>;

enum class SharedMemoryCreationError
{
    REQUESTED_ZERO_SIZED_MEMORY,
    SHARED_MEMORY_ALREADY_EXISTS,
    MAPPING_SHARED_MEMORY_FAILED,
    SHARED_MEMORY_CREATION_FAILED,
    INTERNAL_LOGIC_FAILURE,
    UNKNOWN,
};

enum class SharedMemoryOpenError
{
    REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE,
    SHARED_MEMORY_DOES_NOT_EXIST,
    PERMISSION_DENIED,
    MAPPING_SHARED_MEMORY_FAILED,
    OPEN_SHARED_MEMORY_FAILED,
    INTERNAL_LOGIC_FAILURE,
    UNKNOWN,
};

enum class SharedMemoryAllocationError
{
    OUT_OF_MEMORY,
    REQUESTED_ZERO_SIZED_MEMORY,
    UNKNOWN,
};

template <typename MemoryType, typename Allocator>
class SharedMemoryConcept
{
  public:
    using memory_type = MemoryType;
    using allocator_type = Allocator;

    SharedMemoryConcept() = default;
    SharedMemoryConcept(const SharedMemoryConcept&) = delete;
    SharedMemoryConcept& operator=(const SharedMemoryConcept&) = delete;
    SharedMemoryConcept(SharedMemoryConcept&&) noexcept = default;
    SharedMemoryConcept& operator=(SharedMemoryConcept&&) noexcept = default;
    ~SharedMemoryConcept() noexcept = default;

    const FileName& getName() const noexcept;

    uint64_t getSizeInBytes() const noexcept;

    uint64_t getStartAddress() const noexcept;

    expected<ShmPointer, SharedMemoryAllocationError> allocate(uint64_t size, uint64_t alignment) noexcept;

    void deallocate(PtrDistance_t value) noexcept;

    template <template <typename> class ShmConcept>
    friend class SharedMemoryCreator;
    template <template <typename> class ShmConcept>
    friend class SharedMemoryOpener;

  private:
    explicit SharedMemoryConcept(MemoryType&& memory) noexcept;

    MemoryType m_memory;
};

template <template <typename> class ShmConcept>
class SharedMemoryCreator
{
    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    // trust everyone for now; change later to perms::none
    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::owner_all)

    // add builder parameter for PosixUser and PosixGroup later

  public:
    // Configuration parameter could be used when a shared memory specialization needs additional parameters, e.g. id
    // for GPU shared memory; maybe not needed

    // template <typename AllocatorType>
    // expected<typename ShmConcept<AllocatorType>::memory_type, SharedMemoryCreationError> create(
    // const FileName& name,
    // const typename ShmConcept<AllocatorType>::memory_type::Configuration& mem_config =
    // typename ShmConcept<AllocatorType>::memory_type::Configuration(),
    // const typename AllocatorType::Configuration& alloc_config = typename AllocatorType::Configuration()) noexcept;
    template <typename AllocatorType>
    expected<ShmConcept<AllocatorType>, SharedMemoryCreationError> create(
        const FileName& name,
        const typename ShmConcept<AllocatorType>::memory_type::Configuration& mem_config =
            typename ShmConcept<AllocatorType>::memory_type::Configuration(),
        const typename AllocatorType::Configuration& alloc_config = typename AllocatorType::Configuration()) noexcept;

    // template <typename AllocatorType>
    // expected<typename ShmConcept::template memory_type<AllocatorType>, SharedMemoryCreationError>
    // create(const FileName& name,
    // const typename AllocatorType::Configuration& alloc_config,
    // const typename ShmConcept::template memory_type<AllocatorType>::Configuration& mem_config =
    // typename ShmConcept::template memory_type<AllocatorType>::Configuration()) noexcept;
};

template <typename ShmConcept>
class SharedMemoryOpener
{
    IOX_BUILDER_PARAMETER(uint64_t, requiredMemorySize, 0)

    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_ONLY)

  public:
    template <typename AllocatorType>
    expected<typename ShmConcept::template memory_type<AllocatorType>, SharedMemoryOpenError>
    open(const FileName& name) noexcept;
};

} // namespace cal
} // namespace iox

#endif

