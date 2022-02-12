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
#include "iceoryx_hoofs/cxx/helplets.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/cxx/type_traits.hpp"
#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/popo/sample.hpp"

#include <atomic>
#include <cstring>

namespace iox
{
namespace popo
{
/// @todo #982 Create common .hpp for ActiveChunk and Transaction
enum class ActiveChunk : uint32_t
{
    FIRST = 0,
    SECOND = 1
};

struct Transaction
{
    ActiveChunk activeChunk{ActiveChunk::FIRST};
    // We need a world-view counter to detect if StatusPortWriter::store operation overtook a
    // StatusPortReader::take operation
    uint32_t abaCounter{0U};

    bool operator==(const Transaction& rhs) const
    {
        return (activeChunk == rhs.activeChunk) && (abaCounter == rhs.abaCounter);
    }

    bool operator!=(const Transaction& rhs) const
    {
        return !operator==(rhs);
    }
};

/// @todo #982 Remove
template <typename T>
struct ChunkInSharedMemory
{
    cxx::optional<T> data{cxx::nullopt_t()};
};

/// @todo #982 StatusPortData is untyped in shared memory payload segment
template <typename T>
struct StatusPortData
{
    static_assert(std::is_trivially_copyable<T>::value);
    StatusPortData() noexcept
    {
        // Lifetime of two chunks are bound to the lifetime of a StatusPortData object
        // chunks[0] = m_memoryMgr->getChunk(sizeOfDataType);
        // chunks[1] = m_memoryMgr->getChunk(sizeOfDataType);
        // The SharedChunk d'tor will free memory via RAII & a reference counter
        // once a StatusPortData object is destroyed
    }
    ~StatusPortData() noexcept = default;
    StatusPortData(StatusPortData&& rhs) = delete;
    StatusPortData& operator=(StatusPortData&& rhs) = delete;
    StatusPortData(const StatusPortData&) = delete;
    StatusPortData& operator=(const StatusPortData&) = delete;

    // This data needs to live in payload segement acquired via memory manager
    ChunkInSharedMemory<T> chunks[2];

    std::atomic<Transaction> latestTransaction;
    capro::ServiceDescription serviceDescription;
};

template <typename T>
class StatusPortReader
{
  public:
    StatusPortReader(const StatusPortData<T>& statusPortDataRef) noexcept
        : m_statusPortDataRef(statusPortDataRef)
    /// @todo #982 Replace once the RouDi infrastructure is ready
    // m_statusPortDataRef(iox::runtime::PoshRuntime::getInstance().getMiddlewareStatusPort(sizeof(T)))
    {
        cxx::Expects(m_statusPortDataRef.latestTransaction.is_lock_free());
    }

    StatusPortReader(StatusPortReader&& rhs) = delete;
    StatusPortReader& operator=(StatusPortReader&& rhs) = delete;
    StatusPortReader(const StatusPortReader&) = delete;
    StatusPortReader& operator=(const StatusPortReader&) = delete;

    void take(cxx::function_ref<void(const T&)> callable) const noexcept
    {
        // The user needs to provide a callable which can deal with Frankenstein objects (half-written data)
        Transaction currentTransaction;

        do
        {
            // Get current world view
            currentTransaction = m_statusPortDataRef.latestTransaction.load(std::memory_order_acquire);
            auto currentReadPosition =
                static_cast<std::underlying_type<ActiveChunk>::type>(currentTransaction.activeChunk);

            if (!m_statusPortDataRef.chunks[currentReadPosition].data.has_value())
            {
                return;
            }

            // Do we have to tell the StatusPortWriter that we took ownership of the chunk?
            // Nope, we just need to detect if the StatusPortWriter has stored something in the meantime aka the world a
            // turned one step further. In such a case, we'll just copy the data again and re-call the callable

            callable(*(m_statusPortDataRef.chunks[currentReadPosition].data));

            // Re-call the callable if the world changed in the meantime
        } while (currentTransaction != m_statusPortDataRef.latestTransaction.load(std::memory_order_acquire));
    }

  private:
    const StatusPortData<T>& m_statusPortDataRef;
    /// @todo #982 add getMembers() when integrating into RouDi infrastructure
};

template <typename T>
class StatusPortWriter
{
  public:
    StatusPortWriter(StatusPortData<T>& statusPortDataRef) noexcept
        : m_statusPortDataRef(statusPortDataRef)
    /// @todo #982 Replace once the RouDi infrastructure is ready
    // m_statusPortDataRef(iox::runtime::PoshRuntime::getInstance().getMiddlewareStatusPort(sizeof(T)))
    {
    }

    StatusPortWriter(StatusPortWriter&& rhs) = delete;
    StatusPortWriter& operator=(StatusPortWriter&& rhs) = delete;
    StatusPortWriter(const StatusPortWriter&) = delete;
    StatusPortWriter& operator=(const StatusPortWriter&) = delete;

    void store(cxx::function_ref<void(T&)> callable) noexcept
    {
        // Against what do we have to protect us in store?
        // Against nothing we are the boss and the single entitiy changing latestTransaction, hence no CAS is needed

        // Get current world view
        auto currentTransaction = m_statusPortDataRef.latestTransaction.load(std::memory_order_relaxed);
        // Get the readPosition and toggle it to get write position
        auto currentWritePosition =
            1 xor static_cast<std::underlying_type<ActiveChunk>::type>(currentTransaction.activeChunk);

        /// @todo #982 Later we'll directly pass the dereferenced raw shared memory pointer
        // callable(*(m_statusPortDataRef.chunks[currentWritePosition].data));

        // Update our new world view, it can't fail because we're the only writer
        T valueToStore;
        callable(valueToStore);
        m_statusPortDataRef.chunks[currentWritePosition].data.emplace(valueToStore);

        Transaction newTransaction{static_cast<ActiveChunk>(currentWritePosition), ++currentTransaction.abaCounter};
        m_statusPortDataRef.latestTransaction.store(newTransaction, std::memory_order_release);
        // Store operation aka world view is now observable for all readers
    }

  private:
    StatusPortData<T>& m_statusPortDataRef;
    /// @todo #982 add getMembers() when integrating into RouDi infrastructure
};

#endif // IOX_POSH_POPO_STATUS_PORT_HPP

} // namespace popo
} // namespace iox
