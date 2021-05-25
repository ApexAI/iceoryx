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

#include "iceoryx_posh/internal/popo/ports/server_port_roudi.hpp"

namespace iox
{
namespace popo
{
ServerPortRouDi::ServerPortRouDi(cxx::not_null<MemberType_t* const> serverPortDataPtr) noexcept
    : BasePort(serverPortDataPtr)
    , m_chunkSender(&getMembers()->m_chunkSenderData)
    , m_chunkReceiver(&getMembers()->m_chunkReceiverData)

{
}

const ServerPortRouDi::MemberType_t* ServerPortRouDi::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(BasePort::getMembers());
}

ServerPortRouDi::MemberType_t* ServerPortRouDi::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(BasePort::getMembers());
}

ClientTooSlowPolicy ServerPortRouDi::getClientTooSlowPolicy() const noexcept
{
    return static_cast<ClientTooSlowPolicy>(getMembers()->m_chunkSenderData.m_subscriberTooSlowPolicy);
}

cxx::optional<capro::CaproMessage> ServerPortRouDi::tryGetCaProMessage() noexcept
{
    // get offer state request from user side
    const auto offeringRequested = getMembers()->m_offeringRequested.load(std::memory_order_relaxed);

    const auto isOffered = getMembers()->m_offered.load(std::memory_order_relaxed);

    if (offeringRequested && !isOffered)
    {
        getMembers()->m_offered.store(true, std::memory_order_relaxed);

        capro::CaproMessage caproMessage(capro::CaproMessageType::OFFER, this->getCaProServiceDescription());

        caproMessage.m_chunkQueueData = static_cast<void*>(&getMembers()->m_chunkReceiverData);
        caproMessage.m_historyCapacity = 0;

        // provide additional AUTOSAR Adaptive like information
        caproMessage.m_subType = capro::CaproMessageSubType::EVENT;

        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    else if ((!offeringRequested) && isOffered)
    {
        getMembers()->m_offered.store(false, std::memory_order_relaxed);

        // remove all the clients (represented by their chunk queues)
        m_chunkSender.removeAllQueues();

        capro::CaproMessage caproMessage(capro::CaproMessageType::STOP_OFFER, this->getCaProServiceDescription());
        return cxx::make_optional<capro::CaproMessage>(caproMessage);
    }
    else
    {
        // nothing to change
        return cxx::nullopt_t();
    }
}

cxx::optional<capro::CaproMessage>
ServerPortRouDi::dispatchCaProMessageAndGetPossibleResponse(const capro::CaproMessage& caProMessage) noexcept
{
    capro::CaproMessage responseMessage(
        capro::CaproMessageType::NACK, this->getCaProServiceDescription(), capro::CaproMessageSubType::NOSUBTYPE);

    if (getMembers()->m_offered.load(std::memory_order_relaxed))
    {
        if (capro::CaproMessageType::SUB == caProMessage.m_type)
        {
            const auto ret = m_chunkSender.tryAddQueue(
                static_cast<ClientChunkQueueData_t*>(caProMessage.m_chunkQueueData), caProMessage.m_historyCapacity);
            if (!ret.has_error())
            {
                responseMessage.m_type = capro::CaproMessageType::ACK;
            }
        }
        else if (capro::CaproMessageType::UNSUB == caProMessage.m_type)
        {
            const auto ret =
                m_chunkSender.tryRemoveQueue(static_cast<ClientChunkQueueData_t*>(caProMessage.m_chunkQueueData));
            if (!ret.has_error())
            {
                responseMessage.m_type = capro::CaproMessageType::ACK;
            }
        }
        else
        {
            errorHandler(Error::kPOPO__CAPRO_PROTOCOL_ERROR, nullptr, ErrorLevel::SEVERE);
        }
    }

    return cxx::make_optional<capro::CaproMessage>(responseMessage);
}

void ServerPortRouDi::releaseAllChunks() noexcept
{
    m_chunkSender.releaseAll();
    m_chunkReceiver.releaseAll();
}

} // namespace popo
} // namespace iox
