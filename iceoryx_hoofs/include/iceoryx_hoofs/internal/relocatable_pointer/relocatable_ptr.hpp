// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef IOX_HOOFS_RELOCATABLE_POINTER_RELOCATABLE_PTR_HPP
#define IOX_HOOFS_RELOCATABLE_POINTER_RELOCATABLE_PTR_HPP

#include <cstdint>
#include <type_traits>

namespace iox
{
namespace rp
{
/// @brief Smart pointer type that allows objects using it to be trivially copyable.
///        This applies only if it points to memory owned by the object itself (i.e.
///        i.e. not to memory outside of the object).
///        This is useful to improve copy-efficiency and allow the types build with relocatable
///        pointers only to be stored in shared memory.
///        It is useable like a raw pointer of the corresponding type and can be implicily
///        converted to one.
///
/// @tparam T the native type wrapped by the relocatable_ptr.
///
/// @note It is advisable to use relocatable_ptr only for storage (e.g. member variables),
///       not to pass them around as function arguments or as return value.
///       There should be no need for this, since as
///       pass-around type regular pointers do the job just fine and do not incur
///       the slight runtime overhead of a relocatable_ptr.
///       There should be no memory overhead on 64 bit systems.
template <typename T>
class relocatable_ptr
{
  public:
    /// @brief Construct from raw pointer.
    relocatable_ptr(T* ptr = nullptr)
    {
        m_offset = to_offset(ptr);
    }

    /// @brief Construct from other relocatable pointer.
    relocatable_ptr(const relocatable_ptr& other)
    {
        m_offset = to_offset(other.get());
    }

    /// @brief Move construct from other relocatable pointer.
    relocatable_ptr(relocatable_ptr&& other)
    {
        m_offset = to_offset(other.get());
        other.m_offset = NULL_POINTER_OFFSET;
    }

    /// @brief Assign from relocatable pointer rhs.
    relocatable_ptr& operator=(const relocatable_ptr& rhs)
    {
        if (this != &rhs)
        {
            m_offset = to_offset(rhs.get());
        }
        return *this;
    }

    /// @brief Move assign from relocatable pointer rhs.
    relocatable_ptr& operator=(relocatable_ptr&& rhs)
    {
        if (this != &rhs)
        {
            m_offset = to_offset(rhs.get());
            rhs.m_offset = NULL_POINTER_OFFSET;
        }
        return *this;
    }

    /// @brief Get the corresponding raw pointer.
    /// @return corresponding raw pointer
    T* get()
    {
        return from_offset(m_offset);
    }

    /// @brief Get the corresponding raw pointer from const relocatable_ptr
    /// @return corresponding raw pointer
    const T* get() const
    {
        return from_offset(m_offset);
    }

    /// @brief Dereference a relocatable_ptr.
    /// @return reference to the pointee
    /// @note not available for T=void
    template <typename S = T>
    S& operator*()
    {
        // not actually evaluated in the error case (compiler fails earlier since S = void leads to void&)
        static_assert(!std::is_same<S, void>::value, "relocatable_ptr<void> does not support operator*");
        return *get();
    }

    /// @brief Dereference a const relocatable_ptr.
    /// @return reference to the pointee
    /// @note not available for T=void
    template <typename S = T>
    const S& operator*() const
    {
        // not actually evaluated in the error case (compiler fails earlier since S = void leads to void&)
        static_assert(!std::is_same<S, void>::value, "relocatable_ptr<void> does not support operator* const");
        return *get();
    }

    /// @brief Get the corresponding raw pointer with arrow operator syntax.
    /// @return corresponding raw pointer
    T* operator->()
    {
        return get();
    }

    /// @brief Get the corresponding raw pointer with arrow operator syntax
    ///        from a const relocatable_ptr.
    /// @return corresponding raw pointer
    const T* operator->() const
    {
        return get();
    }

    /// @brief Convert to the corresponding raw pointer.
    /// @return corresponding raw pointer
    operator T*()
    {
        return get();
    }

    /// @brief Convert to the corresponding const raw pointer.
    /// @return corresponding const raw pointer
    operator const T*() const
    {
        return get();
    }

  private:
    using offset_t = uint64_t;

    // This is safe since it is equivalent to point to the relocatable pointer
    // second byte of the relocatable pointer itself which we define to be illegal.
    // (it is no reasonable use-case)
    // Note that 0 is equivalent to point to the relocatable pointer itself (i.e. this).
    static constexpr offset_t NULL_POINTER_OFFSET = 1;

    offset_t m_offset;

    offset_t self() const
    {
        return reinterpret_cast<offset_t>(this);
    }

    offset_t to_offset(const void* ptr) const
    {
        if (ptr == nullptr)
        {
            return NULL_POINTER_OFFSET;
        }
        auto p = reinterpret_cast<offset_t>(ptr);
        return p - self();
    }

    T* from_offset(offset_t offset) const
    {
        if (offset == NULL_POINTER_OFFSET)
        {
            return nullptr;
        }
        return reinterpret_cast<T*>(offset + self());
    }
};

/// @brief Compare relocatable_ptr with respect to logical equality.
/// @return true if rhs and lhs point to the same location, false otherwise
template <typename T>
bool operator==(const relocatable_ptr<T>& lhs, const relocatable_ptr<T>& rhs)
{
    return lhs.get() == rhs.get();
}

/// @brief Compare relocatable_ptr to a raw pointer with respect to logical equality.
/// @return true if rhs and lhs point to the same location, false otherwise
template <typename T>
bool operator==(const T* lhs, const relocatable_ptr<T>& rhs)
{
    return lhs == rhs.get();
}

/// @brief Compare relocatable_ptr to a raw pointer with respect to logical equality.
/// @return true if rhs and lhs point to the same location, false otherwise
template <typename T>
bool operator==(const relocatable_ptr<T>& lhs, const T* rhs)
{
    return lhs.get() == rhs;
}


/// @brief Compare relocatable_ptr with respect to logical inequality.
/// @return true if rhs and lhs point to a different location, false otherwise
template <typename T>
bool operator!=(const relocatable_ptr<T>& lhs, const relocatable_ptr<T>& rhs)
{
    return !operator==(lhs, rhs);
}

/// @brief Compare relocatable_ptr to a raw pointer with respect to logical inequality.
/// @return true if rhs and lhs point to a different location, false otherwise
template <typename T>
bool operator!=(const T* lhs, const relocatable_ptr<T>& rhs)
{
    return !operator==(lhs, rhs);
}

/// @brief Compare relocatable_ptr to a raw pointer with respect to logical inequality.
/// @return true if rhs and lhs point to a different location, false otherwise
template <typename T>
bool operator!=(const relocatable_ptr<T>& lhs, const T* rhs)
{
    return !operator==(lhs, rhs);
}

} // namespace rp
} // namespace iox

#endif // IOX_HOOFS_RELOCATABLE_POINTER_RELOCATABLE_PTR_HPP