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
    // habe ich überhaupt ein ABA problem? Ja, wenn writer einen langsamen reader überholt
    // std::atomic<uint64_t> transactionCounter{0U};
    std::atomic<uint64_t> readPosition{0U};
    std::atomic<uint64_t> writePosition{1U};
    // ich könnte auch den chunk pointer atomic machen
    // std::atomic<T*> activeChunk{nullptr};
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
        uint64_t currentReadPosition{0};
        do
        {
            // Get current world view
            currentReadPosition = m_statusPortDataPtr->readPosition.load(std::memory_order_relaxed);

            // wie kann ich hier die Ownership mitteilen "Das ist mein Chunk!" ohne etwas zu schreiben?
            // Das brauch ich nicht, ich muss lediglich erkennen können, ob sich die Welt weitergedreht hat
            // und mein lesen fehlerhaft war, dann lese ich nochmal

            if (!m_statusPortDataPtr->acknowledgedTransactions[currentReadPosition].data.has_value())
            {
                return;
            }

            callable(m_statusPortDataPtr->acknowledgedTransactions[currentReadPosition].data.value());
            // Re-call the callable if the world changed in the meantime
        } while (
            currentReadPosition
            != m_statusPortDataPtr->readPosition.load(std::memory_order_acq_rel)); // memory_order_acquire is enough?
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
        // wogegen muss ich mich bei schreiben schützen? gegen nichts ich bin der Boss und niemand anderes ändert die
        // writePosition

        // Get current world view
        auto currentWritePosition = m_statusPortDataPtr->writePosition.load(std::memory_order_relaxed);
        auto currentReadPosition = m_statusPortDataPtr->readPosition.load(std::memory_order_relaxed);

        T valueToStore;
        callable(valueToStore);
        // wie können wir hier sicherstellen, dass niemand mehr auf dieser speicherzelle liest zB ein gaaanz langsamer
        // Leser? brauche ich einen referenceCounter? nein, der leser checkt ob sich die welt weitergedreht hat
        // Try to update our new view on the world, if it fails try again
        // This is not needed as we have only 1 writer!
        while (m_statusPortDataPtr->readPosition
                   .compare_exchange_strong( // first write the data then update the readPosition, is exchange() enough?
                       currentReadPosition,
                       currentWritePosition,
                       std::memory_order_acq_rel,
                       std::memory_order_relaxed))
        {
            m_statusPortDataPtr->acknowledgedTransactions[currentWritePosition].data.emplace(valueToStore);
        }
        // das darf ich nur tun, wenn es keine Leser mehr gibt!
        m_statusPortDataPtr->writePosition.fetch_xor(1, std::memory_order_relaxed);
    }

  private:
    StatusPortData<T>* m_statusPortDataPtr;
    /// @todo #982 add getMembers() when integrating into RouDi infrastructure
};

#endif // IOX_POSH_POPO_STATUS_PORT_HPP

} // namespace popo
} // namespace iox
