// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_posh/internal/popo/ports/client_port_roudi.hpp"

namespace iox
{
namespace popo
{
ClientPortRouDi::ClientPortRouDi(cxx::not_null<MemberType_t* const> clientPortDataPtr) noexcept
    : BasePort(clientPortDataPtr)
    , m_chunkSender(&getMembers()->m_chunkSenderData)
    , m_chunkReceiver(&getMembers()->m_chunkReceiverData)

{
}

const ClientPortRouDi::MemberType_t* ClientPortRouDi::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

ClientPortRouDi::MemberType_t* ClientPortRouDi::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}

ResponseQueueFullPolicy ClientPortRouDi::getResponseQueueFullPolicy() const noexcept
{
    return static_cast<ResponseQueueFullPolicy>(getMembers()->m_chunkReceiverData.m_queueFullPolicy);
}

cxx::optional<capro::CaproMessage> ClientPortRouDi::tryGetCaProMessage() noexcept
{
    // get subscribe request from user side
    const auto currentConnectRequest = getMembers()->m_connectRequested.load(std::memory_order_relaxed);

    const auto currentConnectionState = getMembers()->m_connectionState.load(std::memory_order_relaxed);

    if (currentConnectRequest && (ConnectionState::NOT_CONNECTED == currentConnectionState))
    {
        getMembers()->m_connectionState.store(ConnectionState::CONNECTED, std::memory_order_relaxed);

        /// @todo iox-#27 can we reuse CaproMessageType::SUB or should we create CaproMessageType::CON
        capro::CaproMessage caproMessage(capro::CaproMessageType::SUB, BasePort::getMembers()->m_serviceDescription);
        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);
        caproMessage.m_historyCapacity = 0;

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    else if (!currentConnectRequest && (ConnectionState::CONNECTED == currentConnectionState))
    {
        getMembers()->m_connectionState.store(ConnectionState::NOT_CONNECTED, std::memory_order_relaxed);

        /// @todo iox-#27 can we reuse CaproMessageType::UNSUB or should we create CaproMessageType::DISCON
        capro::CaproMessage caproMessage(capro::CaproMessageType::UNSUB, BasePort::getMembers()->m_serviceDescription);
        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    else
    {
        // nothing to change
        return cxx::nullopt_t();
    }
}

cxx::optional<capro::CaproMessage>
ClientPortRouDi::dispatchCaProMessageAndGetPossibleResponse(const capro::CaproMessage& caProMessage) noexcept
{
    const auto currentConnectionState = getMembers()->m_connectionState.load(std::memory_order_relaxed);

    if ((capro::CaproMessageType::OFFER == caProMessage.m_type)
        && (ConnectionState::CONNECTED == currentConnectionState))
    {
        capro::CaproMessage caproMessage(capro::CaproMessageType::SUB, BasePort::getMembers()->m_serviceDescription);
        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);
        caproMessage.m_historyCapacity = 0;

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    else if ((capro::CaproMessageType::OFFER == caProMessage.m_type)
             && (ConnectionState::NOT_CONNECTED == currentConnectionState))
    {
        // No state change
        return cxx::nullopt_t();
    }
    else if ((capro::CaproMessageType::ACK == caProMessage.m_type)
             || (capro::CaproMessageType::NACK == caProMessage.m_type)
             || (capro::CaproMessageType::STOP_OFFER == caProMessage.m_type))
    {
        // we ignore all these messages for multi-producer
        return cxx::nullopt_t();
    }
    else
    {
        // but others should not be received here
        errorHandler(Error::kPOPO__CAPRO_PROTOCOL_ERROR, nullptr, ErrorLevel::SEVERE);
        return cxx::nullopt_t();
    }
}

void ClientPortRouDi::releaseAllChunks() noexcept
{
    m_chunkSender.releaseAll();
    m_chunkReceiver.releaseAll();
}

} // namespace popo
} // namespace iox
