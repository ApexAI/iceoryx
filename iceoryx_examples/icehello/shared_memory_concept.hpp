#ifndef IOX_CONCEPTS_SHARED_MEMORY_CONCEPT_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_CONCEPT_HPP

#include "iceoryx_hoofs/posix_wrapper/posix_access_rights.hpp"
#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iox/builder.hpp"
#include "iox/expected.hpp"
#include "iox/filesystem.hpp"
#include "iox/string.hpp"

// move to concept_abstractions/shared_memory/concept.hpp
// later: separate files for implementations
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
using Name_t = string<platform::IOX_MAX_SHM_NAME_LENGTH>;

enum class SharedMemoryError
{
    SHARED_MEMORY_CREATION_FAILED,
    MAPPING_SHARED_MEMORY_FAILED,
    SHARED_MEMORY_ALLOCATION_ERROR,
    INTERNAL_LOGIC_FAILURE,
    UNKNOWN,
};

template <typename MemoryType, typename Allocator>
class SharedMemory
{
  public:
    using memory_type = MemoryType;

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    SharedMemory(SharedMemory&&) noexcept = default;
    SharedMemory& operator=(SharedMemory&&) noexcept = default;
    ~SharedMemory() noexcept = default;

    // really needed? can it be replaced with something like getAllocator()?
    expected<void*, SharedMemoryError> allocate(const uint64_t size, const uint64_t alignment);

    const Name_t& getName() const;

    uint64_t getSizeInBytes() const;

    const void* getStartAddress() const;

    friend class SharedMemoryCreator;
    friend class SharedMemoryOpener;

  private:
    SharedMemory(MemoryType&& memory);

    MemoryType m_memory;
};

class SharedMemoryCreator
{
    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::none)

    // maybe change deault values once new implementations are available
    IOX_BUILDER_PARAMETER(posix::PosixUser, user, posix::PosixUser::getUserOfCurrentProcess())

    IOX_BUILDER_PARAMETER(posix::PosixGroup, group, posix::PosixGroup::getGroupOfCurrentProcess())

    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_WRITE)

  public:
    // Configuration parameter could be used when a shared memory specialization needs additional parameters, e.g. id
    // for GPU shared memory; maybe not needed
    template <typename SharedMemory>
    expected<SharedMemory, SharedMemoryError> create(const Name_t& name,
                                                     const typename SharedMemory::memory_type::Configuration& config =
                                                         typename SharedMemory::memory_type::Configuration());
};

class SharedMemoryOpener
{
    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_ONLY)

    IOX_BUILDER_PARAMETER(uint64_t, requiredMemorySize, 0)

  public:
    template <typename SharedMemory>
    expected<SharedMemory, SharedMemoryError> open(const Name_t& name);
};

} // namespace cal
} // namespace iox

#endif

