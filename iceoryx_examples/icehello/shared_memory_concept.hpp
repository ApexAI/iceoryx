#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_access_rights.hpp"
#include "iox/builder.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/expected.hpp"
#include "iox/string.hpp"

namespace iox
{
namespace cal
{
static constexpr uint64_t NAME_SIZE = platform::IOX_MAX_SHM_NAME_LENGTH;
using Name_t = string<NAME_SIZE>;

enum class SharedMemoryError
{
};

// move to concept_abstractions/shared_memory/concept.hpp
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

    Name_t& getName();

    uint64_t getSizeInBytes();

    void* getStartAddress();

    friend class SharedMemoryCreator;
    friend class SharedMemoryOpener;

  private:
    SharedMemory(const Name_t& name, MemoryType&& memory);

    Name_t m_name; // implement getName() in SharedMemoryObject and remove member
    MemoryType m_memory;
};

class SharedMemoryCreator
{
    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::none)

    IOX_BUILDER_PARAMETER(posix::PosixUser, user, posix::PosixUser::getUserOfCurrentProcess())

    IOX_BUILDER_PARAMETER(posix::PosixGroup, group, posix::PosixGroup::getGroupOfCurrentProcess())

    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_WRITE)

  public:
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
template <>
Name_t& SharedMemory<posix::SharedMemoryObject, BumpAllocator>::getName()
{
    return m_name;
}

template <>
uint64_t SharedMemory<posix::SharedMemoryObject, BumpAllocator>::getSizeInBytes()
{
    return m_memory.getSizeInBytes();
}

template <>
void* SharedMemory<posix::SharedMemoryObject, BumpAllocator>::getStartAddress()
{
    return m_memory.getBaseAddress();
}

template <>
SharedMemory<posix::SharedMemoryObject, BumpAllocator>::SharedMemory(const Name_t& name,
                                                                     posix::SharedMemoryObject&& memory)
    : m_name(name)
    , m_memory(std::move(memory))
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
        // translate posix::SharedMemoryObjectError to cal::SharedMemoryError
        return error<SharedMemoryError>();
    }
    return success<SharedMemory<posix::SharedMemoryObject, BumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, BumpAllocator>(name, std::move(*sharedMemoryObject)));
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, BumpAllocator>, SharedMemoryError>
SharedMemoryOpener::open(const Name_t& name)
{
    // check requiredSize in SharedMemoryObject(Allocator)

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(name)
                                  .memorySizeInBytes(m_requiredMemorySize) // replace with requiredSizeInBytes
                                  .permissions(perms::owner_all)           // remove?
                                  .accessMode(m_accessMode)
                                  .openMode(posix::OpenMode::OPEN_EXISTING)
                                  .create();

    if (!sharedMemoryObject)
    {
        // translate posix::SharedMemoryObjectError to cal::SharedMemoryError
        return error<SharedMemoryError>();
    }
    return success<SharedMemory<posix::SharedMemoryObject, BumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, BumpAllocator>(name, std::move(*sharedMemoryObject)));
}


} // namespace cal
} // namespace iox

