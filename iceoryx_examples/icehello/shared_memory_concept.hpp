#ifndef IOX_CONCEPTS_SHARED_MEMORY_CONCEPT_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_CONCEPT_HPP

#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iox/builder.hpp"
#include "iox/expected.hpp"
#include "iox/file_name.hpp"
#include "iox/filesystem.hpp"
#include "iox/string.hpp"
#include "shm_pointer.hpp"

// move to concept_abstractions/shared_memory/concept.hpp
// later: separate files for implementations
// - concept_abstractions
//   - shared_memory
//     - concept.hpp
//     - posix_shared_memory.hpp
//     - posix_typed_memory.hpp
// - iceoryx_hoofs
//   - posix
//     - shared_memory
//       - shared_memory.hpp
//       - shared_memory_object.hpp
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
class SharedMemory
{
  public:
    using memory_type = MemoryType;
    using allocator_type = Allocator;

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    SharedMemory(SharedMemory&&) noexcept = default;
    SharedMemory& operator=(SharedMemory&&) noexcept = default;
    ~SharedMemory() noexcept = default;

    const FileName& getName() const noexcept;

    uint64_t getSizeInBytes() const noexcept;

    uint64_t getStartAddress() const noexcept;

    expected<ShmPointer, SharedMemoryAllocationError> allocate(uint64_t size, uint64_t alignment) noexcept;

    void deallocate(PtrDistance_t value) noexcept;

    friend class SharedMemoryCreator;
    friend class SharedMemoryOpener;

  private:
    explicit SharedMemory(MemoryType&& memory) noexcept;

    MemoryType m_memory;
};

class SharedMemoryCreator
{
    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    // trust everyone for now; change later to perms::none
    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::owner_all)

    // add builder parameter for PosixUser and PosixGroup later

  public:
    // Configuration parameter could be used when a shared memory specialization needs additional parameters, e.g. id
    // for GPU shared memory; maybe not needed
    template <typename SharedMemory>
    expected<SharedMemory, SharedMemoryCreationError>
    create(const FileName& name,
           const typename SharedMemory::memory_type::Configuration& mem_config =
               typename SharedMemory::memory_type::Configuration(),
           const typename SharedMemory::allocator_type::Configuration& alloc_config =
               typename SharedMemory::allocator_type::Configuration()) noexcept;

    template <typename SharedMemory>
    expected<SharedMemory, SharedMemoryCreationError>
    create(const FileName& name,
           const typename SharedMemory::allocator_type::Configuration& alloc_config,
           const typename SharedMemory::memory_type::Configuration& mem_config =
               typename SharedMemory::memory_type::Configuration()) noexcept;
};

class SharedMemoryOpener
{
    IOX_BUILDER_PARAMETER(uint64_t, requiredMemorySize, 0)

    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_ONLY)

  public:
    template <typename SharedMemory>
    expected<SharedMemory, SharedMemoryOpenError> open(const FileName& name) noexcept;
};

} // namespace cal
} // namespace iox

#endif

