#ifndef IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP

#include "iox/bump_allocator.hpp"
#include "iox/filesystem.hpp"
#include "iox/vector.hpp"
#include "shared_memory_concept.hpp"

namespace iox
{
namespace cal
{

struct MemoryNameMap
{
    MemoryNameMap(const Name_t& name, const access_rights permissions, void* const memory)
        : m_name(name)
        , m_permissions(permissions)
        , m_memory(memory)
    {
    }

    Name_t m_name;
    access_rights m_permissions{perms::none};
    void* m_memory{nullptr};
};
constexpr uint32_t MAX_PROCESS_LOCAL_MEMORY{5};
vector<MemoryNameMap, MAX_PROCESS_LOCAL_MEMORY> memory_vector;

MemoryNameMap* findNameInVector(const Name_t& name) noexcept
{
    for (auto* iter = memory_vector.begin(); iter != memory_vector.end(); ++iter)
    {
        if (iter->m_name == name)
        {
            return iter;
        }
    }
    return nullptr;
}

enum class ProcessLocalError
{
    PROCESS_LOCAL_MEMORY_CREATION_FAILED,
    PROCESS_LOCAL_MEMORY_NAME_ALREADY_EXISTS,
    OPEN_PROCESS_LOCAL_MEMORY_FAILED,
    PROCESS_LOCAL_MEMORY_ALLOCATION_ERROR,
};

class ProcessLocal
{
  public:
    using Configuration = int;

    ProcessLocal() = default;
    ProcessLocal(const ProcessLocal&) = delete;
    ProcessLocal& operator=(const ProcessLocal&) = delete;
    ProcessLocal(ProcessLocal&& other) noexcept
    {
        *this = std::move(other);
    }

    ProcessLocal& operator=(ProcessLocal&& other) noexcept
    {
        if (this != &other)
        {
            m_name = std::move(other.m_name);
            m_size = other.m_size;
            m_memory = other.m_memory;

            other.m_memory = nullptr;
            other.m_size = 0;
            other.m_name.clear();
        }
        return *this;
    }

    ~ProcessLocal() noexcept
    {
        if (m_memory != nullptr)
        {
            free(m_memory);
        }
        m_memory = nullptr;

        auto* iter = findNameInVector(m_name);
        if (iter != nullptr)
        {
            memory_vector.erase(iter);
        }
    }

    expected<void*, ProcessLocalError> allocate(const uint64_t size, const uint64_t alignment) noexcept
    {
        // call DummyAllocator::allocate()
        if (size > m_size || size == 0)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_ALLOCATION_ERROR);
        }
        return success<void*>(m_memory);
    }

    const Name_t& getName() const noexcept
    {
        return m_name;
    }

    uint64_t getSizeInBytes() const noexcept
    {
        return m_size;
    }

    void* getStartAddress() const noexcept
    {
        return m_memory;
    }

    friend class ProcessLocalBuilder;

  private:
    ProcessLocal(const Name_t& name, const uint64_t size, const access_rights permissions)
        : m_name(name)
        , m_size(size)
    {
        m_memory = malloc(size);

        auto* iter = findNameInVector(m_name);
        if (iter == nullptr)
        {
            memory_vector.push_back(MemoryNameMap(m_name, permissions, m_memory));
        }
    }

    Name_t m_name;
    uint64_t m_size{0};
    void* m_memory{nullptr};
};

class ProcessLocalBuilder
{
    IOX_BUILDER_PARAMETER(Name_t, name, "")

    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::none)

    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_ONLY)

  public:
    expected<ProcessLocal, ProcessLocalError> create() noexcept
    {
        if (memory_vector.size() == MAX_PROCESS_LOCAL_MEMORY)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_ALLOCATION_ERROR);
        }

        if (m_name.empty())
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED);
        }
        auto* iter = findNameInVector(m_name);
        if (iter != nullptr)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_NAME_ALREADY_EXISTS);
        }

        if (m_memorySizeInBytes == 0)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED);
        }

        return success<ProcessLocal>(ProcessLocal(m_name, m_memorySizeInBytes, m_permissions));
    }

    expected<ProcessLocal, ProcessLocalError> open() noexcept
    {
        if (m_name.empty())
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }
        auto* iter = findNameInVector(m_name);
        if (iter == nullptr)
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }

        if (m_accessMode == posix::AccessMode::READ_WRITE && iter->m_permissions == perms::others_read)
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }

        return success<ProcessLocal>(ProcessLocal(m_name, m_memorySizeInBytes, m_permissions));
    }
};

struct DummyAllocator
{
};


// *************concept implementation*************************

SharedMemoryError translateError(const ProcessLocalError error)
{
    if (error == ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED)
    {
        return SharedMemoryError::SHARED_MEMORY_CREATION_FAILED;
    }
    if (error == ProcessLocalError::PROCESS_LOCAL_MEMORY_NAME_ALREADY_EXISTS)
    {
        return SharedMemoryError::SHARED_MEMORY_CREATION_FAILED;
    }
    if (error == ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED)
    {
        return SharedMemoryError::SHARED_MEMORY_CREATION_FAILED;
    }
    if (error == ProcessLocalError::PROCESS_LOCAL_MEMORY_ALLOCATION_ERROR)
    {
        return SharedMemoryError::SHARED_MEMORY_ALLOCATION_ERROR;
    }
    return SharedMemoryError::UNKNOWN;
}

template <>
expected<void*, SharedMemoryError> SharedMemory<ProcessLocal, DummyAllocator>::allocate(const uint64_t size,
                                                                                        const uint64_t alignment)
{
    auto mem = m_memory.allocate(size, alignment);
    if (mem.has_error())
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }
    return success<void*>(mem.value());
}

template <>
const Name_t& SharedMemory<ProcessLocal, DummyAllocator>::getName() const
{
    return m_memory.getName();
}

template <>
uint64_t SharedMemory<ProcessLocal, DummyAllocator>::getSizeInBytes() const
{
    return m_memory.getSizeInBytes();
}

template <>
const void* SharedMemory<ProcessLocal, DummyAllocator>::getStartAddress() const
{
    return m_memory.getStartAddress();
}

template <>
SharedMemory<ProcessLocal, DummyAllocator>::SharedMemory(ProcessLocal&& memory)
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<ProcessLocal, DummyAllocator>, SharedMemoryError>
SharedMemoryCreator::create(const Name_t& name, const ProcessLocal::Configuration& config)
{
    auto mem =
        ProcessLocalBuilder().name(name).memorySizeInBytes(m_memorySizeInBytes).permissions(m_permissions).create();

    if (!mem)
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }

    return success<SharedMemory<ProcessLocal, DummyAllocator>>(
        SharedMemory<ProcessLocal, DummyAllocator>(std::move(*mem)));
}

template <>
expected<SharedMemory<ProcessLocal, DummyAllocator>, SharedMemoryError> SharedMemoryOpener::open(const Name_t& name)
{
    auto mem = ProcessLocalBuilder().name(name).accessMode(m_accessMode).open();
    if (!mem)
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }

    return success<SharedMemory<ProcessLocal, DummyAllocator>>(
        SharedMemory<ProcessLocal, DummyAllocator>(std::move(*mem)));
}
} // namespace cal
} // namespace iox

#endif
