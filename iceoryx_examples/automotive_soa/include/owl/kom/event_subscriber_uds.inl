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

#ifndef IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_INL
#define IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_INL

#include "owl/kom/event_subscriber_uds.hpp"

namespace owl
{
namespace kom
{
template <typename T>
inline EventSubscriberUds<T>::EventSubscriberUds(const core::String&,
                                                 const core::String& instance,
                                                 const core::String&) noexcept
    : m_uds(std::move(iox::posix::UnixDomainSocket::create(instance, iox::posix::IpcChannelSide::SERVER)
                          .or_else([](auto&) {
                              std::cout << "Failed to create UNIX domain socket!" << std::endl;
                              std::terminate();
                          })
                          .value()))
{
}

template <typename T>
inline void EventSubscriberUds<T>::Subscribe(std::size_t) noexcept
{
    // Subscribe not supported with UDS
}

template <typename T>
void EventSubscriberUds<T>::Unsubscribe() noexcept
{
    // Unsubscribe not supported with UDS
}

template <typename T>
template <typename Callable>
core::Result<size_t> inline EventSubscriberUds<T>::GetNewSamples(Callable&& callable,
                                                                 size_t maxNumberOfSamples) noexcept
{
    IOX_DISCARD_RESULT(maxNumberOfSamples);

    core::Result<size_t> numberOfSamples{1};

    auto samplePtr = std::make_unique<SampleType>();
    SampleType& sample = *samplePtr;

    std::string tempBuffer;

    // Receive the first (up to) 4095 Bytes
    m_uds.receive().and_then([&](auto& msg) { tempBuffer.append(msg); }).or_else([](auto&) {
        std::cerr << "Error occurred while receiving UNIX domain socket!" << std::endl;
    });

    if (tempBuffer.size() == 0)
    {
        return 0;
    }

    const char* readPtr = tempBuffer.c_str();

    // Deserialize the counter
    std::memcpy(&sample.counter, readPtr, sizeof(sample.counter));
    readPtr += sizeof(sample.counter);

    // Deserialize the timestamp
    uint64_t sendTimestamp;
    std::memcpy(&sendTimestamp, readPtr, sizeof(sendTimestamp));
    readPtr += sizeof(sendTimestamp);

    int64_t castedSendTimestamp = static_cast<int64_t>(sendTimestamp);
    sample.sendTimestamp = std::chrono::time_point<std::chrono::steady_clock>(
        std::chrono::duration<int64_t, std::nano>(castedSendTimestamp));

    // Deserialize subPackets
    std::memcpy(&sample.subPackets, readPtr, sizeof(sample.subPackets));

    // If more than 4095 Bytes were send in consecutive messages, receive them now
    if (sample.subPackets > 1)
    {
        for (uint32_t i = 0U; i < sample.subPackets - 1; ++i)
        {
            m_uds.receive().and_then([&](auto& msg) { tempBuffer.append(msg); }).or_else([](auto&) {
                std::cerr << "Error occurred while receiving UNIX domain socket!" << std::endl;
            });
        }
    }

    // Complete fragmented message was received, no need to copy the data[] of the message to the sample as it is not used in
    // the example, now we call the user-defined callable
    callable(samplePtr);
    return numberOfSamples;
}

template <typename T>
inline void EventSubscriberUds<T>::SetReceiveHandler(EventReceiveHandler handler)
{
    std::lock_guard<iox::posix::mutex> guard(m_mutex);
    if (HasReceiveHandler())
    {
        std::cout << "Re-attaching a receiver handler is not supported with UNIX domain sockets!" << std::endl;
        return;
    }
    m_receiveHandler.emplace(handler);
    std::thread([&]() {
        while (true)
        {
            // We call the user callback in an endless loop and wait till having received a complete message
            m_receiveHandler.and_then([](iox::cxx::function<void()>& userCallable) { userCallable(); });
        }
    }).detach();
}

template <typename T>
inline void EventSubscriberUds<T>::UnsetReceiveHandler() noexcept
{
    std::cout << "Unsetting the receive handler is not supported with UNIX domain sockets!" << std::endl;
}

template <typename T>
inline bool EventSubscriberUds<T>::HasReceiveHandler() noexcept
{
    std::lock_guard<iox::posix::mutex> guard(m_mutex);
    return m_receiveHandler.has_value();
}
} // namespace kom
} // namespace owl

#endif // IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_INL
