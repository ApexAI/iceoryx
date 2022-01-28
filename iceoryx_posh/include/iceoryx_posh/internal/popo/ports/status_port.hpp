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
#include "iceoryx_hoofs/cxx/type_traits.hpp"
#include "iceoryx_posh/popo/sample.hpp"

#include <atomic>
#include <cstring>

namespace iox
{
namespace popo
{
enum class UsedChunk : uint32_t
{
    FIRST = 0,
    SECOND = 1
};

struct Transaction
{
    UsedChunk usedChunk{UsedChunk::FIRST};
    uint32_t abaCounter{0U};

    bool operator==(const Transaction& rhs) const
    {
        return (usedChunk == rhs.usedChunk) && (abaCounter == rhs.abaCounter);
    }

    bool operator!=(const Transaction& rhs) const
    {
        return !operator==(rhs);
    }
};


/// @todo move to status_port_writer_data.hpp?
template <typename T>
struct SharedMemory
{
    cxx::optional<T> data{cxx::nullopt_t()};
};

template <typename T>
struct StatusPortData
{
    StatusPortData()
    {
        // chunks[0] = m_memoryMgr>getChunk(sizeof(T));
        // chunks[1] = m_memoryMgr>getChunk(sizeof(T));
        // The SharedChunk d'tor will free memory via RAII & a reference counter
        // once a StatusPortData object is destroyed
    }
    // This data needs to live in payload segement acquired via memory manager
    SharedMemory<T> chunks[2];
    // habe ich überhaupt ein ABA problem? Ja, wenn writer einen langsamen reader überholt
    std::atomic<Transaction> latestTransaction;
    // ich könnte auch den chunk pointer atomic machen -> Kein gute Idee, verbraucht komplett 64-bit
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
        Transaction currentTransaction;

        do
        {
            // Get current world view
            currentTransaction = m_statusPortDataPtr->latestTransaction.load(std::memory_order_relaxed);
            auto currentReadPosition = static_cast<std::underlying_type<UsedChunk>::type>(currentTransaction.usedChunk);

            // wie kann ich hier die Ownership mitteilen "Das ist mein Chunk!" ohne etwas zu schreiben?
            // Das brauch ich nicht, ich muss lediglich erkennen können, ob sich die Welt weitergedreht hat
            // und mein lesen fehlerhaft war, dann lese ich nochmal

            if (!m_statusPortDataPtr->chunks[currentReadPosition].data.has_value())
            {
                return;
            }

            // muss hier eine kopie machen, ansonsten crasht das Lambda, memcpy kann nie crashen bei torn-reads
            std::memcpy(reinterpret_cast<void*>(const_cast<T*>(&m_copyOfUserData)),
                        &m_statusPortDataPtr->chunks[currentReadPosition].data.value(),
                        sizeof(T));

            callable(m_copyOfUserData);
            // Re-call the callable if the world changed in the meantime
        } while (currentTransaction != m_statusPortDataPtr->latestTransaction.load(std::memory_order_acquire));
    }

  private:
    T m_copyOfUserData;
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
        // writePosition, readPosition oder abaCounter

        // Get current world view
        auto currentTransaction = m_statusPortDataPtr->latestTransaction.load(std::memory_order_relaxed);
        // Get the readPosition and toggle it to get write position
        auto currentWritePosition =
            1 xor static_cast<std::underlying_type<UsedChunk>::type>(currentTransaction.usedChunk);

        T valueToStore;
        // We could also safely pass the memory directly to the lambda, then we save a copy
        // callable(*(m_statusPortDataPtr->chunks[currentWritePosition].data));
        callable(valueToStore);

        // wie können wir hier sicherstellen, dass niemand mehr auf dieser speicherzelle liest zB ein gaaanz langsamer
        // Leser? brauche ich einen referenceCounter? nein, der leser checkt ob sich die welt weitergedreht hat

        // Update our new view on the world, it can't fail because we're the only writer
        m_statusPortDataPtr->chunks[currentWritePosition].data.emplace(valueToStore);

        Transaction newTransaction{static_cast<UsedChunk>(currentWritePosition), ++currentTransaction.abaCounter};
        m_statusPortDataPtr->latestTransaction.store(newTransaction, std::memory_order_release);
        // Store operation is now observable for all readers
    }

  private:
    StatusPortData<T>* m_statusPortDataPtr;
    /// @todo #982 add getMembers() when integrating into RouDi infrastructure
};

#endif // IOX_POSH_POPO_STATUS_PORT_HPP

} // namespace popo
} // namespace iox
