// Links for further reading: https://refactoring.guru/design-patterns/builder

#include <memory>

#include "iceoryx_hoofs/design_pattern/builder_pattern.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object/shared_memory.hpp"

using namespace iox;
using namespace iox::posix;

template <typename TypeToConstruct, typename ErrorType>
struct Creation : public TypeToConstruct
{
    template <typename... Targs>
    static cxx::expected<TypeToConstruct, ErrorType> create(Targs&&... args);

    Creation& operator=(Creation&& rhs)
    {
        return *this;
    }

    bool m_isInitialized = false;
    ErrorType m_error;
};

struct SharedMemoryObjectCtorArguments
{
    const SharedMemory::Name_t& name;
    const uint64_t memorySizeInBytes;
    const AccessMode accessMode;
    const OpenMode openMode;
    const cxx::optional<const void*>& baseAddressHint;
    const cxx::perms permissions;
};


struct SharedMemoryObject : public Creation<SharedMemoryObject, SharedMemoryError>
{
    // SharedMemoryObject(const SharedMemoryObjectCtorArguments& args);

    // SharedMemoryObject(const SharedMemory::Name_t& name,
    //                    const uint64_t memorySizeInBytes,
    //                    const AccessMode accessMode,
    //                    const OpenMode openMode,
    //                    const cxx::optional<const void*>& baseAddressHint = cxx::nullopt,
    //                    const cxx::perms permissions = cxx::perms::owner_read | cxx::perms::owner_write
    //                                                   | cxx::perms::group_read | cxx::perms::group_write) noexcept;

  private:
    SharedMemoryObject(int fdToShm, void* baseAddress);

    SharedMemoryObject& operator=(SharedMemoryObject&& rhs)
    {
        if (this != &rhs)
        {
            Creation<SharedMemoryObject, SharedMemoryError>::operator=(rhs);

            rhs.m_isInitialized = false;
        }

        return *this;
    }
};

class SharedMemoryObjectBuilder
{
  public:
    SharedMemoryObjectBuilder& name(const SharedMemory::Name_t& value)
    {
        m_name = value;
        return *this;
    }

  private:
    SharedMemory::Name_t& m_name;

  private:
    uint64_t memorySizeInBytes;
    AccessMode accessMode;
    OpenMode openMode;
    cxx::optional<const void*>& baseAddressHint;
    cxx::perms permissions;
};

struct C
{
    void copy() &&;
};

struct Destination
{
    C to() &&;
};

struct MemCpy
{
    Destination from() &&;
};

#include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"
int main()
{
    // #include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"
    // TypeState pattern
    MemCpy().from().to().copy();

    MemCpy a;

    errno = 0;
    shm_open();

    //

    SharedMemoryObject object = SharedMemoryObjectBuilder().name("asd").memorySizeInBytes(123).create();

    std::unique_ptr<int> bla{new int()};
}
