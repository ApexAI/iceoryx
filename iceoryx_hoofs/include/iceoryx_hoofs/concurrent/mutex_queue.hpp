// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_HOOFS_CONCURRENT_MUTEX_QUEUE_HPP
#define IOX_HOOFS_CONCURRENT_MUTEX_QUEUE_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/mutex.hpp"

#include <mutex>

namespace iox
{
namespace concurrent
{
template <typename ElementType, uint64_t Capacity>
class MutexQueue
{
  public:
    using element_t = ElementType;

    constexpr uint64_t capacity() const noexcept
    {
        return Capacity;
    }

    bool tryPush(const ElementType& value) noexcept
    {
        std::lock_guard<posix::mutex> lock{m_mutex};

        if (size() < capacity())
        {
            m_data[(m_writeIndex++) % Capacity] = value;
            return true;
        }

        return false;
    }

    iox::cxx::optional<ElementType> push(const ElementType& value) noexcept
    {
        std::lock_guard<posix::mutex> lock{m_mutex};
        iox::cxx::optional<ElementType> returnValue;
        if (size() == capacity())
        {
            returnValue = pop();
        }

        m_data[(m_writeIndex++) % Capacity] = value;
        return returnValue;
    }

    iox::cxx::optional<ElementType> pop() noexcept
    {
        std::lock_guard<posix::mutex> lock{m_mutex};

        if (empty())
        {
            return cxx::nullopt;
        }

        return m_data[(m_readIndex++) % Capacity];
    }

    bool empty() const noexcept
    {
        std::lock_guard<posix::mutex> lock{m_mutex};
        return m_readIndex == m_writeIndex;
    }

    uint64_t size() const noexcept
    {
        std::lock_guard<posix::mutex> lock{m_mutex};
        return m_writeIndex - m_readIndex;
    }

  private:
    ElementType m_data[Capacity];
    uint64_t m_readIndex = 0U;
    uint64_t m_writeIndex = 0U;
    mutable posix::mutex m_mutex{true};
};
} // namespace concurrent
} // namespace iox


#endif
