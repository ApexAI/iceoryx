#ifndef IOX_CONCEPTS_SHARED_MEMORY_POINTER_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_POINTER_HPP

#include <cstdint>

using PtrDistance_t = uint64_t; // use NewType?

class ShmPointer
{
  public:
    ShmPointer() = default;
    ShmPointer(const ShmPointer&) = delete;
    ShmPointer& operator=(const ShmPointer&) = delete;
    ShmPointer(ShmPointer&&) noexcept = default;
    ShmPointer& operator=(ShmPointer&&) noexcept = default;
    ~ShmPointer() noexcept = default;

    ShmPointer(const uint64_t distance, void* const memory) noexcept
        : m_distance_to_data(distance)
        , m_mapped_ptr(memory)
    {
    }

    PtrDistance_t distance() const noexcept
    {
        return m_distance_to_data;
    }

    void* mapped_ptr() noexcept
    {
        return m_mapped_ptr;
    }

    const void* mapped_ptr() const noexcept
    {
        return m_mapped_ptr;
    }

  private:
    PtrDistance_t m_distance_to_data{0};
    void* m_mapped_ptr{nullptr};
};

#endif

