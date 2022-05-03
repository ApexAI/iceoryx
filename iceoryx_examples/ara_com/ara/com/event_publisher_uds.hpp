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

#ifndef IOX_EXAMPLES_ARA_COM_EVENT_PUBLISHER_UDS_HPP
#define IOX_EXAMPLES_ARA_COM_EVENT_PUBLISHER_UDS_HPP

#include "iceoryx_hoofs/cxx/serialization.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/unix_domain_socket.hpp"
#include "iceoryx_posh/popo/publisher.hpp"

#include "ara/types.hpp"

#include <memory>

namespace ara
{
namespace com
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
        : m_publisher({service, instance, event}, {1U, "", true})
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
        /// @todo #1332 Use cxx::Serialization?

        /// @todo #1332 replace push_back with append(charArray, count)

        std::string tempBuffer;
        /// @todo store the uint32_t in four chars
        tempBuffer.push_back(static_cast<uint8_t>(userSamplePtr->counter));
        auto sendTimeStampNs = userSamplePtr->sendTimestamp.time_since_epoch().count();
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0xFF00000000000000) >> 56));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x00FF000000000000) >> 48));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x0000FF0000000000) >> 40));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x000000FF00000000) >> 32));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x00000000FF000000) >> 24));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x0000000000FF0000) >> 16));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x000000000000FF00) >> 8));
        tempBuffer.push_back(static_cast<uint8_t>((sendTimeStampNs & 0x00000000000000FF)));


        uint32_t offset = static_cast<uint32_t>(tempBuffer.size()) + 4;

        uint32_t totalSize = userSamplePtr->payloadSizeInBytes + offset;

        userSamplePtr->subPackets = totalSize / static_cast<uint32_t>(iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE);

        if (totalSize % static_cast<uint32_t>(iox::posix::UnixDomainSocket::MAX_MESSAGE_SIZE) > 0)
        {
            userSamplePtr->subPackets += 1;
        }

        tempBuffer.push_back(static_cast<uint8_t>((userSamplePtr->subPackets & 0xFF000000) >> 24));
        tempBuffer.push_back(static_cast<uint8_t>((userSamplePtr->subPackets & 0x00FF0000) >> 16));
        tempBuffer.push_back(static_cast<uint8_t>((userSamplePtr->subPackets & 0x0000FF00) >> 8));
        tempBuffer.push_back(static_cast<uint8_t>((userSamplePtr->subPackets & 0x000000FF)));

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
            std::cout << "Error occurred while sending on UNIX domain socket!" << std::endl;
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
                std::cout << "Error occurred while sending on UNIX domain socket!" << std::endl;
            });
            tempBuffer.clear();
            bytesToSend -= messageSize;
        }

        std::cout << "We sent " << userSamplePtr->subPackets << " packets" << std::endl;
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
} // namespace com
} // namespace ara

#endif // IOX_EXAMPLES_ARA_COM_EVENT_PUBLISHER_UDS_HPP
