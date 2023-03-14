#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
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

    Name_t& getName();

    uint64_t getSizeInBytes();

    friend class SharedMemoryBuilder;

  private:
    SharedMemory(const Name_t& name, const uint64_t memorySizeInBytes);

    Name_t m_name;
    uint64_t m_memorySizeInBytes;
};

class SharedMemoryBuilder
{
    IOX_BUILDER_PARAMETER(Name_t, name, "")

    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::none)

  public:
    template <typename SharedMemory>
    expected<SharedMemory, SharedMemoryError> create(const typename SharedMemory::memory_type::Configuration& config =
                                                         typename SharedMemory::memory_type::Configuration());
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
    return m_memorySizeInBytes;
}

template <>
SharedMemory<posix::SharedMemoryObject, BumpAllocator>::SharedMemory(const Name_t& name,
                                                                     const uint64_t memorySizeInBytes)
    : m_name(name)
    , m_memorySizeInBytes(memorySizeInBytes)
{
}

template <>
expected<SharedMemory<posix::SharedMemoryObject, BumpAllocator>, SharedMemoryError>
SharedMemoryBuilder::create(const posix::SharedMemoryObject::Configuration& config)
{
    // check configuration, translate errors, ...

    auto sharedMemoryObject = posix::SharedMemoryObjectBuilder()
                                  .name(m_name)
                                  .memorySizeInBytes(m_memorySizeInBytes)
                                  .permissions(m_permissions)
                                  .accessMode(posix::AccessMode::READ_WRITE) // from config?
                                  .openMode(posix::OpenMode::OPEN_OR_CREATE) // from config?
                                  .create();

    if (!sharedMemoryObject)
    {
        // translate posix::SharedMemoryObjectError to cal::SharedMemoryError
        return error<SharedMemoryError>();
    }
    return success<SharedMemory<posix::SharedMemoryObject, BumpAllocator>>(
        SharedMemory<posix::SharedMemoryObject, BumpAllocator>(m_name, m_memorySizeInBytes));
}

} // namespace cal
} // namespace iox

