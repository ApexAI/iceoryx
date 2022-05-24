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

#include "iceoryx_hoofs/internal/posix_wrapper/unix_domain_socket.hpp"
#include "iceoryx_posh/popo/publisher.hpp"

#include "owl/types.hpp"

namespace owl
{
namespace kom
{
/// @brief Class solely for benchmarking iceoryx against UNIX domain sockets
template <typename T>
class EventPublisher<T, EventTransmission::UDS>
{
  public:
    using SampleType = T;

    static_assert(is_supported_topic<T>::value,
                  "Topic must have specific members, look at TimestampTopic1Byte as an example!");

    EventPublisher(const ServiceIdentifier& service,
                   const InstanceIdentifier& instance,
                   const EventIdentifier& event) noexcept;
    ~EventPublisher() noexcept = default;

    EventPublisher(const EventPublisher&) = delete;
    EventPublisher(EventPublisher&&) = delete;
    EventPublisher& operator=(const EventPublisher&) = delete;
    EventPublisher& operator=(EventPublisher&&) = delete;

    static constexpr uint64_t HISTORY_CAPACITY{1U};
    static constexpr bool OFFERED_ON_CREATE{true};

    std::unique_ptr<SampleType> Allocate();

    bool Send(std::unique_ptr<SampleType> userSamplePtr);

    void Offer() noexcept;
    void StopOffer() noexcept;

  private:
    /// @brief Not used, just for the service discovery to trigger the 'StartFindService' callback
    iox::popo::Publisher<SampleType> m_publisher;
    iox::posix::UnixDomainSocket m_uds;
    InstanceIdentifier m_instanceId;
};
} // namespace kom
} // namespace owl

#include "owl/kom/event_publisher_uds.inl"

#endif // IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_PUBLISHER_UDS_HPP
