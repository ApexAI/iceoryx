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

#ifndef IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_HPP
#define IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_HPP

#include "iceoryx_hoofs/internal/posix_wrapper/mutex.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/unix_domain_socket.hpp"

#include "owl/types.hpp"

#include <limits>
#include <memory>
#include <thread>


namespace owl
{
namespace kom
{
/// @brief Class solely for benchmarking iceoryx against UNIX domain sockets
template <typename T>
class EventSubscriber<T, EventTransmission::UDS>
{
  public:
    using SampleType = T;

    static_assert(is_supported_topic<T>::value,
                  "Topic must have specific members, look at TimestampTopic1Byte as an example!");

    EventSubscriber(const ServiceIdentifier&, const InstanceIdentifier& instance, const EventIdentifier&) noexcept;

    ~EventSubscriber() noexcept;

    void Subscribe(std::size_t) noexcept;
    void Unsubscribe() noexcept;

    template <typename Callable>
    core::Result<size_t> GetNewSamples(Callable&& callable,
                                       size_t maxNumberOfSamples = std::numeric_limits<size_t>::max());

    void SetReceiveHandler(EventReceiveHandler handler);
    void UnsetReceiveHandler() noexcept;
    bool HasReceiveHandler() noexcept;

  private:
    iox::cxx::optional<iox::cxx::function<void()>> m_receiveHandler;
    static constexpr bool IS_RECURSIVE{true};
    iox::posix::mutex m_mutex{IS_RECURSIVE};
    iox::posix::UnixDomainSocket m_uds;
    std::atomic_bool m_run{true};
    std::thread m_thread{[&]() {
        while (m_run)
        {
            // We call the user callback in an endless loop and wait till having received a complete message
            m_receiveHandler.and_then([](iox::cxx::function<void()>& userCallable) {
                if (!userCallable)
                {
                    std::cerr << "Tried to call an empty receive handler!" << std::endl;
                    return;
                }
                userCallable();
            });
        }
    }};
};
} // namespace kom
} // namespace owl

#include "owl/kom/event_subscriber_uds.inl"

#endif // IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_HPP
