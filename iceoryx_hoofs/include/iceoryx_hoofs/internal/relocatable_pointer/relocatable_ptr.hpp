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

#pragma once

#include <cstdint>
#include <type_traits>

namespace iox
{
namespace rp
{
// TODO: doxygen
// relocatable_ptr is regular
template <typename T>
class relocatable_ptr
{
  private:
    using offset_t = uint64_t;

    static constexpr offset_t NULL_POINTER_OFFSET = 1;

  public:
    relocatable_ptr(T* ptr = nullptr)
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
        other.m_offset = NULL_POINTER_OFFSET;
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
            rhs.m_offset = NULL_POINTER_OFFSET;
        }
        return *this;
    }

    T* get()
    {
        return from_offset(m_offset);
    }

    const T* get() const
    {
        return from_offset(m_offset);
    }

    // for non-void type only
    // template <typename S = T>
    // typename std::enable_if<!std::is_same<S, void>::value, T>::type& operator*()
    // {
    //     return *get();
    // }

    template <typename S = T>
    S& operator*()
    {
        static_assert(!std::is_same<S, void>::value, "relocatable_ptr<void> does not support operator*");
        return *get();
    }

    template <typename S = T>
    const S& operator*() const
    {
        static_assert(!std::is_same<S, void>::value, "relocatable_ptr<void> does not support operator* const");
        return *get();
    }

    T* operator->()
    {
        return get();
    }

    const T* operator->() const
    {
        return get();
    }

    operator T*()
    {
        return get();
    }

    operator const T*() const
    {
        return get();
    }

  private:
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

template <typename T>
bool operator==(const relocatable_ptr<T>& rhs, const relocatable_ptr<T>& lhs)
{
    return lhs.get() == rhs.get();
}

template <typename T>
bool operator!=(const relocatable_ptr<T>& rhs, const relocatable_ptr<T>& lhs)
{
    return !operator==(lhs, rhs);
}

// TODO: const T specialization

} // namespace rp
} // namespace iox