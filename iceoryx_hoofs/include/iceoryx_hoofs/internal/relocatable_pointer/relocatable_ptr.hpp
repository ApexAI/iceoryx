#pragma once

#include <cstdint>
#include <type_traits>

namespace iox
{
namespace rp
{
template <typename T>
class relocatable_ptr
{
  private:
    using offset_t = uint64_t;

  public:
    relocatable_ptr(T* ptr)
    {
        m_offset = to_offset(ptr);
    }

    relocatable_ptr(const relocatable_ptr& other)
    {
        m_offset = to_offset(other.get());
    }

    relocatable_ptr(relocatable_ptr&& other)
    {
        m_offset = to_offset(other.get());
    }

    relocatable_ptr& operator=(const relocatable_ptr& rhs)
    {
        if (this != &rhs)
        {
            m_offset = to_offset(rhs.get());
        }
        return *this;
    }

    relocatable_ptr& operator=(relocatable_ptr&& rhs)
    {
        if (this != &rhs)
        {
            m_offset = to_offset(rhs.get());
        }
        return *this;
    }

    T* get()
    {
        return from_offset(m_offset);
    }

    // for non-void type only
    template <typename S = T>
    typename std::enable_if<!std::is_same<S, void>::value, T>::type& operator*()
    {
        return *get();
    }

    // for non-void type only
    template <typename S = T>
    typename std::enable_if<!std::is_same<S, void>::value, T>::type* operator->()
    {
        return get();
    }

    operator T*()
    {
        return get();
    }

  private:
    offset_t m_offset;

    offset_t self()
    {
        return reinterpret_cast<offset_t>(this);
    }

    offset_t to_offset(void* ptr)
    {
        auto p = reinterpret_cast<offset_t>(ptr);
        return p - self();
    }

    T* from_offset(offset_t offset)
    {
        return reinterpret_cast<T*>(offset + self());
    }
};

// TODO: const T specialization

} // namespace rp
} // namespace iox