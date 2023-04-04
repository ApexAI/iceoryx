#ifndef IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/mutex.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/filesystem.hpp"
#include "iox/memory.hpp"
#include "iox/vector.hpp"
#include "shared_memory_concept.hpp"

#include <atomic>

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
        ++referenceCounter;
    }

    MemoryNameMap(const MemoryNameMap&) = delete;
    MemoryNameMap& operator=(const MemoryNameMap&) = delete;

    MemoryNameMap(MemoryNameMap&& other) noexcept
    {
        *this = std::move(other);
    }

    MemoryNameMap& operator=(MemoryNameMap&& other) noexcept
    {
        if (this != &other)
        {
            m_name = std::move(other.m_name);
            m_permissions = other.m_permissions;
            m_memory = other.m_memory;
            referenceCounter.exchange(other.referenceCounter);

            other.referenceCounter.exchange(0);
            other.m_memory = nullptr;
            other.m_permissions = perms::none;
            other.m_name.clear();
        }
        return *this;
    }

    ~MemoryNameMap()
    {
        --referenceCounter;
    }

    Name_t m_name;
    access_rights m_permissions{perms::none};
    void* m_memory{nullptr};

    std::atomic<uint64_t> referenceCounter{0};
};
constexpr uint32_t MAX_PROCESS_LOCAL_MEMORY{5};
vector<MemoryNameMap, MAX_PROCESS_LOCAL_MEMORY> memory_vector;
posix::mutex memory_vector_mutex{true};

MemoryNameMap* findNameInVector(const Name_t& name) noexcept
{
    std::lock_guard<posix::mutex> g(memory_vector_mutex);
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

// ProcessLocalError used for simplicity
struct ProcessLocalAllocator
{
    ProcessLocalAllocator(void* const startAddress, const uint64_t length) noexcept
        : m_allocator(startAddress, length)
    {
    }

    ProcessLocalAllocator(const ProcessLocalAllocator&) = delete;
    ProcessLocalAllocator(ProcessLocalAllocator&&) noexcept = default;
    ProcessLocalAllocator& operator=(const ProcessLocalAllocator&) noexcept = delete;
    ProcessLocalAllocator& operator=(ProcessLocalAllocator&&) noexcept = default;
    ~ProcessLocalAllocator() noexcept = default;

    expected<void*, ProcessLocalError> allocate(const uint64_t size, const uint64_t alignment) noexcept
    {
        auto allocation_result = m_allocator.allocate(size, alignment);
        if (allocation_result.has_error())
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_ALLOCATION_ERROR);
        }
        if (allocation_result.value() == nullptr)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_ALLOCATION_ERROR);
        }
        return success<void*>(allocation_result.value());
    }

    BumpAllocator m_allocator;
};

class ProcessLocal
{
  public:
    using Configuration = int;

    ProcessLocal(const ProcessLocal&) = delete;
    ProcessLocal& operator=(const ProcessLocal&) = delete;
    ProcessLocal(ProcessLocal&& other) noexcept
        : m_allocator(other.m_memory, other.m_size)
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
            m_allocator = std::move(other.m_allocator);
            m_isCreator = other.m_isCreator;

            other.m_isCreator = false;
            other.m_memory = nullptr;
            other.m_size = 0;
            other.m_name.clear();
        }
        return *this;
    }

    ~ProcessLocal() noexcept
    {
        std::lock_guard<posix::mutex> g(memory_vector_mutex);
        auto* iter = findNameInVector(m_name);
        if (iter != nullptr)
        {
            if (iter->referenceCounter > 1)
            {
                if (m_isCreator)
                {
                    memory_vector.erase(iter);
                    m_memory = nullptr;
                }
                else
                {
                    --iter->referenceCounter;
                    m_memory = nullptr;
                }
            }
            else
            {
                if (m_isCreator)
                {
                    memory_vector.erase(iter);
                }
                alignedFree(m_memory);
                m_memory = nullptr;
            }
        }
    }

    expected<void*, ProcessLocalError> allocate(const uint64_t size, const uint64_t alignment) noexcept
    {
        return m_allocator.allocate(size, alignment);
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
        , m_allocator(m_memory, m_size)
    {
        std::lock_guard<posix::mutex> g(memory_vector_mutex);
        auto* iter = findNameInVector(m_name);
        if (iter == nullptr)
        {
            m_memory = alignedAlloc(static_cast<uint64_t>(iox_page_size), size);
            m_isCreator = true;

            memory_vector.push_back(MemoryNameMap(m_name, permissions, m_memory));
        }
        else
        {
            m_memory = iter->m_memory;
            ++iter->referenceCounter;
        }

        m_allocator = ProcessLocalAllocator(m_memory, m_size);
    }

    Name_t m_name;
    uint64_t m_size{0};
    void* m_memory{nullptr};
    ProcessLocalAllocator m_allocator;
    bool m_isCreator{false};
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
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED);
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
expected<void*, SharedMemoryError> SharedMemory<ProcessLocal, ProcessLocalAllocator>::allocate(const uint64_t size,
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
const Name_t& SharedMemory<ProcessLocal, ProcessLocalAllocator>::getName() const
{
    return m_memory.getName();
}

template <>
uint64_t SharedMemory<ProcessLocal, ProcessLocalAllocator>::getSizeInBytes() const
{
    return m_memory.getSizeInBytes();
}

template <>
const void* SharedMemory<ProcessLocal, ProcessLocalAllocator>::getStartAddress() const
{
    return m_memory.getStartAddress();
}

template <>
SharedMemory<ProcessLocal, ProcessLocalAllocator>::SharedMemory(ProcessLocal&& memory)
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<ProcessLocal, ProcessLocalAllocator>, SharedMemoryError>
SharedMemoryCreator::create(const Name_t& name, const ProcessLocal::Configuration& config)
{
    // check configuration

    auto mem =
        ProcessLocalBuilder().name(name).memorySizeInBytes(m_memorySizeInBytes).permissions(m_permissions).create();

    if (!mem)
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }

    return success<SharedMemory<ProcessLocal, ProcessLocalAllocator>>(
        SharedMemory<ProcessLocal, ProcessLocalAllocator>(std::move(*mem)));
}

template <>
expected<SharedMemory<ProcessLocal, ProcessLocalAllocator>, SharedMemoryError>
SharedMemoryOpener::open(const Name_t& name)
{
    auto mem = ProcessLocalBuilder().name(name).memorySizeInBytes(m_requiredMemorySize).accessMode(m_accessMode).open();
    if (!mem)
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }

    return success<SharedMemory<ProcessLocal, ProcessLocalAllocator>>(
        SharedMemory<ProcessLocal, ProcessLocalAllocator>(std::move(*mem)));
}
} // namespace cal
} // namespace iox

#endif
