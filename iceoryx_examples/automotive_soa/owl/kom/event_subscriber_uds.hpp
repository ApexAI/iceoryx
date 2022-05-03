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
template <typename SampleType>
class EventSubscriberUds
{
  public:
    EventSubscriberUds(const core::String&, const core::String& instance, const core::String&) noexcept
        : m_uds(std::move(iox::posix::UnixDomainSocket::create(instance, iox::posix::IpcChannelSide::SERVER)
                              .or_else([](auto&) {
                                  std::cout << "Failed to create UNIX domain socket!" << std::endl;
                                  std::terminate();
                              })
                              .value()))
    {
    }

    void Subscribe(std::size_t) noexcept
    {
    }

    void Unsubscribe() noexcept
    {
    }

    template <typename Callable>
    owl::core::Result<size_t> GetNewSamples(Callable&& callable,
                                            size_t maxNumberOfSamples = std::numeric_limits<size_t>::max()) noexcept
    {
        IOX_DISCARD_RESULT(maxNumberOfSamples);

        owl::core::Result<size_t> numberOfSamples{1};

        auto samplePtr = std::make_unique<SampleType>();
        SampleType& sample = *samplePtr;

        std::string tempBuffer;

        // Receive the first (up to) 4095 Bytes
        m_uds.receive().and_then([&](auto& msg) { tempBuffer.append(msg); }).or_else([](auto&) {
            std::cout << "Error occurred while receiving UNIX domain socket!" << std::endl;
        });

        if (tempBuffer.size() == 0)
        {
            return 0;
        }

        // Deserialize the counter
        sample.counter = tempBuffer[0];

        // Deserialize the timestamp
        uint64_t sendTimestamp =
            (static_cast<uint64_t>(tempBuffer[1]) & 0xFF) << 56 | (static_cast<uint64_t>(tempBuffer[2]) & 0xFF) << 48
            | (static_cast<uint64_t>(tempBuffer[3]) & 0xFF) << 40 | (static_cast<uint64_t>(tempBuffer[4]) & 0xFF) << 32
            | (static_cast<uint64_t>(tempBuffer[5]) & 0xFF) << 24 | (static_cast<uint64_t>(tempBuffer[6]) & 0xFF) << 16
            | (static_cast<uint64_t>(tempBuffer[7]) & 0xFF) << 8 | (static_cast<uint64_t>(tempBuffer[8]) & 0xFF);

        // std::cout << "Timestamp[1]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[1]
        //           << std::endl;
        // std::cout << "Timestamp[2]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[2]
        //           << std::endl;
        // std::cout << "Timestamp[3]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[3]
        //           << std::endl;
        // std::cout << "Timestamp[4]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[4]
        //           << std::endl;
        // std::cout << "Timestamp[5]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[5]
        //           << std::endl;
        // std::cout << "Timestamp[6]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[6]
        //           << std::endl;
        // std::cout << "Timestamp[7]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[7]
        //           << std::endl;
        // std::cout << "Timestamp[8]: " << std::hex << std::setfill('0') << std::setw(2) << (int)tempBuffer[8]
        //           << std::endl;

        int64_t castedSendTimestamp = static_cast<int64_t>(sendTimestamp);

        // std::cout << "unsigned Timestamp: " << std::hex << std::setfill('0') << std::setw(2) << sendTimestamp
        //           << std::endl;
        // std::cout << "Signed Timestamp: " << std::hex << std::setfill('0') << std::setw(2) << castedSendTimestamp
        //           << std::endl;

        sample.sendTimestamp = std::chrono::time_point<std::chrono::steady_clock>(
            std::chrono::duration<int64_t, std::nano>(castedSendTimestamp));

        // Deserialize subPackets
        uint32_t subPackets =
            (static_cast<uint32_t>(tempBuffer[9]) & 0xFF) << 24 | (static_cast<uint32_t>(tempBuffer[10]) & 0xFF) << 16
            | (static_cast<uint32_t>(tempBuffer[11]) & 0xFF) << 8 | (static_cast<uint32_t>(tempBuffer[12]) & 0xFF);
        sample.subPackets = subPackets;

        std::cout << "We received " << subPackets << " packets!" << std::endl;


        // If more than 4095 Bytes were send int consecutive message, receive them now
        if (subPackets > 1)
        {
            for (uint32_t i = 0U; i < subPackets - 1; ++i)
            {
                m_uds.receive().and_then([&](auto& msg) { tempBuffer.append(msg); }).or_else([](auto&) {
                    std::cout << "Error occurred while receiving UNIX domain socket!" << std::endl;
                });
            }
        }

        std::cout << "Buffer size: " << tempBuffer.size() << std::endl;

        // Complete fragmented message was received, now we call the user-defined callable
        callable(samplePtr);
        return numberOfSamples;
    }

    void SetReceiveHandler(EventReceiveHandler handler)
    {
        std::lock_guard<iox::posix::mutex> guard(m_mutex);
        m_receiveHandler.emplace(handler);
        std::thread([&]() {
            while (true)
            {
                // As UDS has a blocking receive, we call the user callback in an endless loop and
                // wait till having received a complete message
                m_receiveHandler.and_then([](iox::cxx::function<void()>& userCallable) { userCallable(); });
            }
        }).detach();
    }

    void UnsetReceiveHandler()
    {
        std::lock_guard<iox::posix::mutex> guard(m_mutex);
        m_receiveHandler.reset();
    }

    bool HasReceiverHandler() const
    {
        return m_receiveHandler.has_value();
    }

  private:
    iox::cxx::optional<iox::cxx::function<void()>> m_receiveHandler;
    static constexpr bool isRecursive{true};
    iox::posix::mutex m_mutex{isRecursive};
    iox::posix::UnixDomainSocket m_uds;
};

} // namespace kom
} // namespace owl

#endif // IOX_EXAMPLES_AUTOMOTIVE_SOA_EVENT_SUBSCRIBER_UDS_HPP
