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

#ifndef IOX_POSH_POPO_STATUS_PORT_HPP
#define IOX_POSH_POPO_STATUS_PORT_HPP

#include "iceoryx_hoofs/cxx/function_ref.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_posh/popo/sample.hpp"

#include <atomic>

namespace iox
{
namespace popo
{
/// @todo move to status_port_writer_data.hpp?
template <typename T>
struct Transaction
{
    cxx::optional<T> data{cxx::nullopt_t()};
};

template <typename T>
struct StatusPortData
{
    // This data needs to live in payload segement acquired via memory manager
    Transaction<T> acknowledgedTransactions[1];

    const uint64_t INVALID{0};
    const uint64_t UPDATING{1};
    // std::atomic<uint64_t> abaCounter{0U};
    std::atomic<uint64_t> readPosition{0U};
    std::atomic<uint64_t> writePosition{1U};
};

// template <typename T>
// using StatusPortData = Sample<T, StatusPortTransactions<T>>; // eigentlich StatusPortSample?

template <typename T>
class StatusPortReader
{
  public:
    StatusPortReader(cxx::not_null<StatusPortData<T>* const> statusPortDataPtr) noexcept
        : m_statusPortDataPtr(statusPortDataPtr)
    {
    }

    StatusPortReader(StatusPortReader&& rhs) = delete;
    StatusPortReader& operator=(StatusPortReader&& rhs) = delete;

    StatusPortReader(const StatusPortReader&) = delete;
    StatusPortReader& operator=(const StatusPortReader&) = delete;

    void takeChunk(cxx::function_ref<void(const T&)> callable) const
    {
        // Get current world view
        auto currentReadPosition = m_statusPortDataPtr->readPosition.load(std::memory_order_relaxed);

        if (!m_statusPortDataPtr->acknowledgedTransactions[currentReadPosition].data.has_value())
        {
            return;
        }

        do
        {
            callable(m_statusPortDataPtr->acknowledgedTransactions[currentReadPosition].data.value());
            // Re-call the callable if the world changed in the meantime
        } while (currentReadPosition != m_statusPortDataPtr->readPosition.load(std::memory_order_acq_rel)); //memory_order_acquire is enough?
    }

  private:
    StatusPortData<T>* m_statusPortDataPtr;
    /// @todo #982 add getMembers() when integrating into RouDi infrastructure
};

template <typename T>
class StatusPortWriter
{
  public:
    StatusPortWriter(cxx::not_null<StatusPortData<T>*> statusPortDataPtr) noexcept
        : m_statusPortDataPtr(statusPortDataPtr)
    {
    }

    StatusPortWriter(StatusPortWriter&& rhs) = delete;
    StatusPortWriter& operator=(StatusPortWriter&& rhs) = delete;

    StatusPortWriter(const StatusPortWriter&) = delete;
    StatusPortWriter& operator=(const StatusPortWriter&) = delete;

    void storeChunk(cxx::function_ref<void(T&)> callable)
    {
        // wogegen muss ich mich bei schreiben schützen? gegen nichts ich bin der Boss und niemand anderes ändert die writePosition

        // Get current world view
        auto currentWritePosition = m_statusPortDataPtr->writePosition.load(std::memory_order_relaxed);
        auto currentReadPosition = m_statusPortDataPtr->readPosition.load(std::memory_order_relaxed);

        T valueToStore;
        callable(valueToStore);
        // Try to update our new view on the world, if it fails try again
        while (m_statusPortDataPtr->readPosition.compare_exchange_strong( // first write the data then update the readPosition, is exchange() enough?
            currentReadPosition, currentWritePosition, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            m_statusPortDataPtr->acknowledgedTransactions[currentWritePosition].data.emplace(valueToStore);
        }
        m_statusPortDataPtr->writePosition.fetch_xor(1, std::memory_order_relaxed);
    }

  private:
    StatusPortData<T>* m_statusPortDataPtr;
    /// @todo #982 add getMembers() when integrating into RouDi infrastructure
};

#endif // IOX_POSH_POPO_STATUS_PORT_HPP

} // namespace popo
} // namespace iox
