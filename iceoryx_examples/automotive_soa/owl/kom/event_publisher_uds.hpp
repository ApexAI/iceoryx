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

#ifndef IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_PUBLISHER_UDS_HPP
#define IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_PUBLISHER_UDS_HPP

#include "iceoryx_hoofs/cxx/serialization.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/unix_domain_socket.hpp"
#include "iceoryx_posh/popo/publisher.hpp"

#include "owl/types.hpp"

#include <memory>

namespace owl
{
namespace kom
{
/// @brief Class solely for benchmarking iceoryx against UNIX domain sockets
template <typename SampleType>
class EventPublisherUds
{
  public:
    EventPublisherUds(const EventPublisherUds&) = delete;
    EventPublisherUds(EventPublisherUds&&) = delete;
    EventPublisherUds& operator=(const EventPublisherUds&) = delete;
    EventPublisherUds& operator=(EventPublisherUds&&) = delete;

    EventPublisherUds(const core::String& service, const core::String& instance, const core::String& event) noexcept
        : m_publisher({service, instance, event}, {1U, iox::NodeName_t(), true})
        , m_instanceId(instance)
    {
    }

    std::unique_ptr<SampleType> Allocate() noexcept
    {
        // The proxy needs some time to discover the service and create the EventSubscriberUds with the UDS server,
        // hence the creation of the UDS client is done here
        if (!m_calledForTheFirstTime)
        {
            m_uds = std::move(iox::posix::UnixDomainSocket::create(m_instanceId, iox::posix::IpcChannelSide::CLIENT)
                                  .or_else([](auto&) {
                                      std::cout << "Failed to create UNIX domain socket!" << std::endl;
                                      std::terminate();
                                  })
                                  .value());
            m_calledForTheFirstTime = true;
        }

        // Allocate the memory on the heap
        return std::make_unique<SampleType>();
    }

    void Send(std::unique_ptr<SampleType> userSamplePtr) noexcept
    {
        std::string tempBuffer;

        tempBuffer.append(reinterpret_cast<char*>(&userSamplePtr->counter), 4);

        auto sendTimeStampNs = userSamplePtr->sendTimestamp.time_since_epoch().count();

        tempBuffer.append(reinterpret_cast<char*>(&sendTimeStampNs), 8);

        constexpr uint8_t BYTES_OF_SUBPACKETS_INTEGER{4};
        uint32_t offset = static_cast<uint32_t>(tempBuffer.size()) + BYTES_OF_SUBPACKETS_INTEGER;
        uint32_t totalSize = userSamplePtr->payloadSizeInBytes + offset;
        userSamplePtr->subPackets = totalSize / static_cast<uint32_t>(iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE);
        if (totalSize % static_cast<uint32_t>(iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE) > 0)
        {
            userSamplePtr->subPackets += 1;
        }

        tempBuffer.append(reinterpret_cast<char*>(&userSamplePtr->subPackets), 4);

        uint32_t k{0};
        uint64_t bytesToSend{userSamplePtr->payloadSizeInBytes};
        uint64_t messageSize{0};

        // We handle the special case for the first message
        if (bytesToSend + offset <= iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE)
        {
            messageSize = bytesToSend;
        }
        else
        {
            messageSize = iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE - offset;
        }
        for (uint32_t j = 0U; j < messageSize; j++)
        {
            tempBuffer.push_back(userSamplePtr->data[k++]);
        }
        m_uds.send(tempBuffer).or_else([](auto&) {
            std::cerr << "Error occurred while sending on UNIX domain socket!" << std::endl;
        });
        tempBuffer.clear();
        bytesToSend -= messageSize;

        // Following subPackets are send in a loop
        for (uint32_t i = 0U; i < userSamplePtr->subPackets - 1; i++)
        {
            if (bytesToSend <= iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE)
            {
                messageSize = bytesToSend;
            }
            else
            {
                messageSize = iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE;
            }
            for (uint32_t j = 0U; j < messageSize; j++)
            {
                tempBuffer.push_back(userSamplePtr->data[k++]);
            }
            m_uds.send(tempBuffer).or_else([](auto&) {
                std::cerr << "Error occurred while sending on UNIX domain socket!" << std::endl;
            });
            tempBuffer.clear();
            bytesToSend -= messageSize;
        }
    }

    void Offer() noexcept
    {
        m_publisher.offer();
    }

    void StopOffer() noexcept
    {
        m_publisher.stopOffer();
    }

    /// @brief Not used, just for the service discovery to trigger the 'StartFindService' callback
    iox::popo::Publisher<SampleType> m_publisher;
    iox::posix::UnixDomainSocket m_uds;
    bool m_calledForTheFirstTime{false};
    core::String m_instanceId;
};
} // namespace kom
} // namespace owl

#endif // IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_PUBLISHER_UDS_HPP
