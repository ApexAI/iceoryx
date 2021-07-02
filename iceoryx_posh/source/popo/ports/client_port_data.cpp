// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/popo/ports/client_port_data.hpp"

namespace iox
{
namespace popo
{
/// @todo iox-#27 shouldn't this be in part of ChunkQueueData?
///       With QueueFullPolicy as only parameter and automatic queue type deduction?
///       This wouldn't work for client-server if OneToManyPolicy is used
cxx::VariantQueueTypes getQueueType(ResponseQueueFullPolicy policy) noexcept
{
    return policy == ResponseQueueFullPolicy::DISCARD_OLDEST_DATA
               ? cxx::VariantQueueTypes::SoFi_MultiProducerSingleConsumer
               : cxx::VariantQueueTypes::FiFo_MultiProducerSingleConsumer;
}

ClientPortData::ClientPortData(const capro::ServiceDescription& serviceDescription,
                               const RuntimeName_t& runtimeName,
                               const ClientOptions& clientOptions,
                               mepoo::MemoryManager* const memoryManager,
                               const mepoo::MemoryInfo& memoryInfo) noexcept
    : BasePortData(serviceDescription, runtimeName, clientOptions.nodeName)
    , m_chunkSenderData(
          memoryManager, static_cast<SubscriberTooSlowPolicy>(clientOptions.serverTooSlowPolicy), 0, memoryInfo)
    , m_chunkReceiverData(getQueueType(clientOptions.responseQueueFullPolicy),
                          static_cast<QueueFullPolicy>(clientOptions.responseQueueFullPolicy))
    , m_connectRequested(clientOptions.connectOnCreate)
{
    m_chunkReceiverData.m_queue.setCapacity(clientOptions.responseQueueCapacity);
}

} // namespace popo
} // namespace iox
