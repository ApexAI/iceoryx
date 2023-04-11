#ifndef IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP

#include "iceoryx_hoofs/internal/concurrent/smart_lock.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/mutex.hpp"
#include "iox/bump_allocator.hpp"
#include "iox/filesystem.hpp"
#include "iox/memory.hpp"
#include "iox/scope_guard.hpp"
#include "iox/vector.hpp"
#include "shared_memory_allocator.hpp"
#include "shared_memory_concept.hpp"

#include <atomic>

namespace iox
{
namespace cal
{
// **** Allocator not yet thread safe!!

// *****concrete implementation of ProcessLocal memory******

enum class ProcessLocalError
{
    PROCESS_LOCAL_MEMORY_CREATION_FAILED,
    PROCESS_LOCAL_MEMORY_NAME_ALREADY_EXISTS,
    OPEN_PROCESS_LOCAL_MEMORY_FAILED,
};

struct ProcessLocalMembers
{
    ProcessLocalMembers(const uint64_t size, const access_rights& permissions) noexcept
        : m_memory(reinterpret_cast<uint64_t*>(alignedAlloc(static_cast<uint64_t>(iox_page_size), size)))
        , m_size(size)
        , m_permissions(permissions)
        , m_allocator(BumpAllocator(m_memory, m_size))
    {
        ++m_refCounter;
    }

    void* m_memory{nullptr};
    uint64_t m_size{0};
    access_rights m_permissions{perms::none};
    BumpAllocator m_allocator{m_memory, m_size};
    std::atomic<uint64_t> m_refCounter{0};
};

struct ShmProcessLocalMap
{
    ShmProcessLocalMap(const Name_t& name, ProcessLocalMembers* const plm) noexcept
        : m_name(name)
        , m_plm(plm)
    {
    }

    Name_t m_name;
    ProcessLocalMembers* m_plm;
};
constexpr uint32_t MAX_PROCESS_LOCAL_MEMORY{5};
concurrent::smart_lock<vector<ShmProcessLocalMap, MAX_PROCESS_LOCAL_MEMORY>, std::recursive_mutex> memory_vector;

ShmProcessLocalMap* findNameInVector(const Name_t& name,
                                     vector<ShmProcessLocalMap, MAX_PROCESS_LOCAL_MEMORY>& vec) noexcept
{
    for (auto* iter = vec.begin(); iter != vec.end(); ++iter)
    {
        if (iter->m_name == name)
        {
            return iter;
        }
    }
    return nullptr;
}

class ProcessLocal
{
  public:
    using Configuration = int;

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
            m_plm = other.m_plm;
            m_name = other.m_name;
            m_isOwner = other.m_isOwner;

            other.m_isOwner = false;
            other.m_name.clear();
            other.m_plm = nullptr;
        }
        return *this;
    }

    ~ProcessLocal() noexcept = default;

    const Name_t& getName() const noexcept
    {
        return m_name;
    }

    uint64_t getSizeInBytes() const noexcept
    {
        return m_plm->m_size;
    }

    uint64_t getStartAddress() const noexcept
    {
        return reinterpret_cast<uint64_t>(m_plm->m_memory);
    }

    BumpAllocator& getAllocator() const noexcept
    {
        return m_plm->m_allocator;
    }

    friend class ProcessLocalBuilder;

  private:
    ProcessLocal(ProcessLocalMembers* const plm, const Name_t& name, const bool isOwner) noexcept
        : m_plm(plm)
        , m_name(name)
        , m_isOwner(isOwner)
    {
    }

    ProcessLocalMembers* m_plm{nullptr};
    Name_t m_name;
    bool m_isOwner{false};
    ScopeGuard m_guard{[this]() {
        if (m_plm != nullptr)
        {
            auto guardedVector = memory_vector.getScopeGuard();
            --m_plm->m_refCounter;
            if (m_plm->m_refCounter == 0)
            {
                alignedFree(m_plm->m_memory);
                delete m_plm;
                m_plm = nullptr;
            }
            if (m_isOwner)
            {
                auto* iter = findNameInVector(m_name, *guardedVector);
                memory_vector->erase(iter);
            }
        }
    }};
};

class ProcessLocalBuilder
{
    IOX_BUILDER_PARAMETER(Name_t, name, "")

    IOX_BUILDER_PARAMETER(uint64_t, memorySizeInBytes, 0)

    IOX_BUILDER_PARAMETER(access_rights, permissions, perms::none)

    IOX_BUILDER_PARAMETER(posix::AccessMode, accessMode, posix::AccessMode::READ_ONLY)

    optional<posix::mutex> builderMutex;

  public:
    expected<ProcessLocal, ProcessLocalError> create() noexcept
    {
        if (m_memorySizeInBytes == 0)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED);
        }

        if (m_name.empty())
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED);
        }

        if (memory_vector->size() == MAX_PROCESS_LOCAL_MEMORY)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_CREATION_FAILED);
        }

        auto guardedVector = memory_vector.getScopeGuard();
        auto* iter = findNameInVector(m_name, *guardedVector);
        if (iter != nullptr)
        {
            return error<ProcessLocalError>(ProcessLocalError::PROCESS_LOCAL_MEMORY_NAME_ALREADY_EXISTS);
        }

        ProcessLocalMembers* plm = new ProcessLocalMembers(m_memorySizeInBytes, m_permissions);
        memory_vector->push_back(ShmProcessLocalMap(m_name, plm));
        ProcessLocal pl(plm, m_name, true);
        return success<ProcessLocal>(std::move(pl));
    }

    expected<ProcessLocal, ProcessLocalError> open() noexcept
    {
        if (m_name.empty())
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }

        auto guardedVector = memory_vector.getScopeGuard();
        auto* iter = findNameInVector(m_name, *guardedVector);
        if (iter == nullptr)
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }

        // check permissions
        if (m_accessMode == posix::AccessMode::READ_WRITE && iter->m_plm->m_permissions == perms::owner_read)
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }

        // check size
        if (m_memorySizeInBytes > iter->m_plm->m_size)
        {
            return error<ProcessLocalError>(ProcessLocalError::OPEN_PROCESS_LOCAL_MEMORY_FAILED);
        }

        ++iter->m_plm->m_refCounter;
        ProcessLocal pl(iter->m_plm, m_name, false);
        return success<ProcessLocal>(std::move(pl));
    }
};


// *************concept implementation*************************
// will probably be moved to:
// - concept_abstractions
//   - shared_memory
//     - process_local.hpp

SharedMemoryError translateError(const ProcessLocalError error) noexcept
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
    return SharedMemoryError::UNKNOWN;
}

template <>
const Name_t& SharedMemory<ProcessLocal, BumpAllocator>::getName() const noexcept
{
    return m_memory.getName();
}

template <>
uint64_t SharedMemory<ProcessLocal, BumpAllocator>::getSizeInBytes() const noexcept
{
    return m_memory.getSizeInBytes();
}

template <>
uint64_t SharedMemory<ProcessLocal, BumpAllocator>::getStartAddress() const noexcept
{
    return m_memory.getStartAddress();
}

template <>
ShmPointer SharedMemory<ProcessLocal, BumpAllocator>::allocate(uint64_t size, uint64_t alignment) noexcept
{
    auto res = m_memory.getAllocator().allocate(size, alignment);
    if (res.has_error())
    {
        return nullptr;
    }
    return *res;
}

// template <>
// void SharedMemory<ProcessLocal, BumpAllocator>::deallocate(ShmPointer value) noexcept
//{
// m_allocator->deallocate();
//}

template <>
SharedMemory<ProcessLocal, BumpAllocator>::SharedMemory(ProcessLocal&& memory) noexcept
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<ProcessLocal, BumpAllocator>, SharedMemoryError>
SharedMemoryCreator::create(const Name_t& name, const ProcessLocal::Configuration& config) noexcept
{
    // check configuration

    auto mem =
        ProcessLocalBuilder().name(name).memorySizeInBytes(m_memorySizeInBytes).permissions(m_permissions).create();

    if (!mem)
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }

    return success<SharedMemory<ProcessLocal, BumpAllocator>>(
        SharedMemory<ProcessLocal, BumpAllocator>(std::move(*mem)));
}

template <>
expected<SharedMemory<ProcessLocal, BumpAllocator>, SharedMemoryError>
SharedMemoryOpener::open(const Name_t& name) noexcept
{
    auto mem = ProcessLocalBuilder().name(name).memorySizeInBytes(m_requiredMemorySize).accessMode(m_accessMode).open();
    if (!mem)
    {
        return error<SharedMemoryError>(translateError(mem.get_error()));
    }

    return success<SharedMemory<ProcessLocal, BumpAllocator>>(
        SharedMemory<ProcessLocal, BumpAllocator>(std::move(*mem)));
}
} // namespace cal
} // namespace iox

#endif
