#ifndef IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_PROCESS_LOCAL_HPP

#include "iceoryx_hoofs/internal/concurrent/smart_lock.hpp"
#include "iox/file_name.hpp"
#include "iox/filesystem.hpp"
#include "iox/memory.hpp"
#include "iox/scope_guard.hpp"
#include "iox/vector.hpp"
#include "shared_memory_concept.hpp"
#include "shm_bump_allocator.hpp"
#include "shm_pointer.hpp"

#include <atomic>

namespace iox
{
namespace cal
{
struct ProcessLocalMembers
{
    // shared memory must be page size aligned when used in inter process communication, otherwise alignment can be
    // corrupted in certain processes because of the offset
    ProcessLocalMembers(const uint64_t size, const access_rights& permissions) noexcept
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
        : m_memory(reinterpret_cast<uint64_t*>(alignedAlloc(static_cast<uint64_t>(iox_page_size), size)))
        , m_size(size)
        , m_permissions(permissions)
        , m_allocator(ShmBumpAllocator(m_memory, m_size))
    {
        ++m_refCounter;
    }

    void* m_memory{nullptr};
    uint64_t m_size{0};
    access_rights m_permissions{perms::none};
    ShmBumpAllocator m_allocator{m_memory, m_size};
    std::atomic<uint64_t> m_refCounter{0};
};

struct ShmProcessLocalMap
{
    ShmProcessLocalMap(const FileName& name, ProcessLocalMembers* const plm) noexcept
        : m_name(name)
        , m_plm(plm)
    {
    }

    FileName m_name;
    ProcessLocalMembers* m_plm;
};
constexpr uint32_t MAX_PROCESS_LOCAL_MEMORY{5};
concurrent::smart_lock<vector<ShmProcessLocalMap, MAX_PROCESS_LOCAL_MEMORY>, std::recursive_mutex> memory_vector;

ShmProcessLocalMap* findNameInVector(const FileName& name,
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
        : m_plm(other.m_plm)
        , m_name(std::move(other.m_name))
        , m_isOwner(other.m_isOwner)
    {
        other.m_isOwner = false;
        other.m_plm = nullptr;
    }
    ProcessLocal& operator=(ProcessLocal&& other) noexcept
    {
        if (this != &other)
        {
            m_plm = other.m_plm;
            m_name = other.m_name;
            m_isOwner = other.m_isOwner;

            other.m_isOwner = false;
            other.m_plm = nullptr;
        }
        return *this;
    }

    ~ProcessLocal() noexcept = default;

    const FileName& getName() const noexcept
    {
        return m_name;
    }

    uint64_t getSizeInBytes() const noexcept
    {
        return m_plm->m_size;
    }

    uint64_t getStartAddress() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
        return reinterpret_cast<uint64_t>(m_plm->m_memory);
    }

    ShmBumpAllocator& getAllocator() const noexcept
    {
        return m_plm->m_allocator;
    }

    friend class SharedMemoryCreator;
    friend class SharedMemoryOpener;

  private:
    ProcessLocal(ProcessLocalMembers* const plm, const FileName& name, const bool isOwner) noexcept
        : m_plm(plm)
        , m_name(name)
        , m_isOwner(isOwner)
    {
    }

    ProcessLocalMembers* m_plm{nullptr};
    FileName m_name;
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


template <>
const FileName& SharedMemory<ProcessLocal, ShmBumpAllocator>::getName() const noexcept
{
    return m_memory.getName();
}

template <>
uint64_t SharedMemory<ProcessLocal, ShmBumpAllocator>::getSizeInBytes() const noexcept
{
    return m_memory.getSizeInBytes();
}

template <>
uint64_t SharedMemory<ProcessLocal, ShmBumpAllocator>::getStartAddress() const noexcept
{
    return m_memory.getStartAddress();
}

template <>
expected<ShmPointer, SharedMemoryAllocationError>
SharedMemory<ProcessLocal, ShmBumpAllocator>::allocate(uint64_t size, uint64_t alignment) noexcept
{
    auto distance = m_memory.getAllocator().allocate(size, alignment);
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
            return error<SharedMemoryAllocationError>(SharedMemoryAllocationError::UNKNOWN);
        }
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) required for low level memory management
    return success<ShmPointer>(ShmPointer(*distance, reinterpret_cast<void*>(getStartAddress() + *distance)));
}

template <>
void SharedMemory<ProcessLocal, ShmBumpAllocator>::deallocate(PtrDistance_t value) noexcept = delete;

template <>
SharedMemory<ProcessLocal, ShmBumpAllocator>::SharedMemory(ProcessLocal&& memory) noexcept
    : m_memory(std::move(memory))
{
}

template <>
expected<SharedMemory<ProcessLocal, ShmBumpAllocator>, SharedMemoryCreationError> SharedMemoryCreator::create(
    const FileName& name, const ProcessLocal::Configuration&, const ShmBumpAllocator::Configuration&) noexcept
{
    // check configurations

    if (m_memorySizeInBytes == 0)
    {
        IOX_LOG(WARN) << "Cannot acquire memory of size 0.";
        return error<SharedMemoryCreationError>(SharedMemoryCreationError::REQUESTED_ZERO_SIZED_MEMORY);
    }

    if (memory_vector->size() == MAX_PROCESS_LOCAL_MEMORY)
    {
        IOX_LOG(WARN) << "Cannot acquire memory since limit is reached.";
        return error<SharedMemoryCreationError>(SharedMemoryCreationError::SHARED_MEMORY_CREATION_FAILED);
    }

    auto guardedVector = memory_vector.getScopeGuard();
    auto* iter = findNameInVector(name, *guardedVector);
    if (iter != nullptr)
    {
        IOX_LOG(WARN) << "Cannot acquire memory since " << name.as_string() << " it already exists.";
        return error<SharedMemoryCreationError>(SharedMemoryCreationError::SHARED_MEMORY_ALREADY_EXISTS);
    }

    auto* plm = new ProcessLocalMembers(m_memorySizeInBytes, m_permissions);
    memory_vector->push_back(ShmProcessLocalMap(name, plm));
    ProcessLocal pl(plm, name, true);
    return success<SharedMemory<ProcessLocal, ShmBumpAllocator>>(
        SharedMemory<ProcessLocal, ShmBumpAllocator>(std::move(pl)));
}

template <>
expected<SharedMemory<ProcessLocal, ShmBumpAllocator>, SharedMemoryCreationError>
SharedMemoryCreator::create(const FileName& name,
                            const ShmBumpAllocator::Configuration& alloc_config,
                            const ProcessLocal::Configuration& mem_config) noexcept
{
    return create<SharedMemory<ProcessLocal, ShmBumpAllocator>>(name, mem_config, alloc_config);
}

template <>
expected<SharedMemory<ProcessLocal, ShmBumpAllocator>, SharedMemoryOpenError>
SharedMemoryOpener::open(const FileName& name) noexcept
{
    auto guardedVector = memory_vector.getScopeGuard();
    auto* iter = findNameInVector(name, *guardedVector);
    if (iter == nullptr)
    {
        IOX_LOG(WARN) << "Cannot open memory since " << name.as_string() << " does not exist.";
        return error<SharedMemoryOpenError>(SharedMemoryOpenError::SHARED_MEMORY_DOES_NOT_EXIST);
    }

    // permission check for test purpose; will be implemented in more detail later
    if (m_accessMode == posix::AccessMode::READ_WRITE && iter->m_plm->m_permissions == perms::owner_read)
    {
        IOX_LOG(WARN) << "Cannot open memory due to unsufficient permissions.";
        return error<SharedMemoryOpenError>(SharedMemoryOpenError::PERMISSION_DENIED);
    }

    if (m_requiredMemorySize > iter->m_plm->m_size)
    {
        IOX_LOG(WARN) << "Cannot open memory of required size " << m_requiredMemorySize
                      << " since it exceeds the actual size of " << iter->m_plm->m_size;
        return error<SharedMemoryOpenError>(SharedMemoryOpenError::REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE);
    }

    ++iter->m_plm->m_refCounter;
    ProcessLocal pl(iter->m_plm, name, false);
    return success<SharedMemory<ProcessLocal, ShmBumpAllocator>>(
        SharedMemory<ProcessLocal, ShmBumpAllocator>(std::move(pl)));
}
} // namespace cal
} // namespace iox

#endif
