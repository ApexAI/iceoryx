// Copyright (c) 2019, 2020 by Robert Bosch GmbH, Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/internal/runtime/ipc_runtime_interface.hpp"
#include "iceoryx_posh/version/version_info.hpp"
#include "iceoryx_utils/posix_wrapper/posix_access_rights.hpp"

#include <thread>

namespace iox
{
namespace runtime
{
IpcRuntimeInterface::IpcRuntimeInterface(const ProcessName_t& roudiName,
                                         const ProcessName_t& appName,
                                         const units::Duration roudiWaitingTimeout) noexcept
    : m_appName(appName)
    , m_AppMqInterface(appName)
    , m_RoudiMqInterface(roudiName)
{
    if (!m_AppMqInterface.isInitialized())
    {
        errorHandler(Error::kMQ_INTERFACE__UNABLE_TO_CREATE_APPLICATION_MQ);
        return;
    }

    cxx::DeadlineTimer timer(roudiWaitingTimeout);

    enum class RegState
    {
        WAIT_FOR_ROUDI,
        SEND_REGISTER_REQUEST,
        WAIT_FOR_REGISTER_ACK,
        FINISHED
    };

    int64_t transmissionTimestamp{0};
    auto regState = RegState::WAIT_FOR_ROUDI;
    while (!timer.hasExpired() && regState != RegState::FINISHED)
    {
        if (!m_RoudiMqInterface.isInitialized() || !m_RoudiMqInterface.mqMapsToFile())
        {
            LogDebug() << "reopen RouDi mqueue!";
            m_RoudiMqInterface.reopen();
            regState = RegState::WAIT_FOR_ROUDI;
        }

        switch (regState)
        {
        case RegState::WAIT_FOR_ROUDI:
        {
            waitForRoudi(timer);
            if (m_RoudiMqInterface.isInitialized())
            {
                regState = RegState::SEND_REGISTER_REQUEST;
            }
            break;
        }
        case RegState::SEND_REGISTER_REQUEST:
        {
            using namespace units;
            using namespace std::chrono;
            auto timestamp = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
            while (transmissionTimestamp == timestamp)
            {
                timestamp = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
            }
            transmissionTimestamp = timestamp;

            // send IpcMessageType::REG to RouDi

            IpcMessage sendBuffer;
            sendBuffer << IpcMessageTypeToString(IpcMessageType::REG) << m_appName << std::to_string(getpid())
                       << std::to_string(posix::PosixUser::getUserOfCurrentProcess().getID())
                       << std::to_string(transmissionTimestamp)
                       << static_cast<cxx::Serialization>(version::VersionInfo::getCurrentVersion()).toString();

            bool successfullySent = m_RoudiMqInterface.timedSend(sendBuffer, 100_ms);

            if (successfullySent)
            {
                regState = RegState::WAIT_FOR_REGISTER_ACK;
            }
            else
            {
                regState = RegState::WAIT_FOR_ROUDI;
            }
            break;
        }
        case RegState::WAIT_FOR_REGISTER_ACK:
            if (waitForRegAck(transmissionTimestamp) == RegAckResult::SUCCESS)
            {
                regState = RegState::FINISHED;
            }
            else
            {
                regState = RegState::WAIT_FOR_ROUDI;
            }
            break;
        case RegState::FINISHED:
            // nothing to do, move along
            break;
        }
    }

    if (regState != RegState::FINISHED)
    {
        m_AppMqInterface.cleanupResource();
    }
    switch (regState)
    {
    case RegState::WAIT_FOR_ROUDI:
        errorHandler(Error::kMQ_INTERFACE__REG_ROUDI_NOT_AVAILABLE);
        break;
    case RegState::SEND_REGISTER_REQUEST:
        errorHandler(Error::kMQ_INTERFACE__REG_UNABLE_TO_WRITE_TO_ROUDI_MQ);
        break;
    case RegState::WAIT_FOR_REGISTER_ACK:
        errorHandler(Error::kMQ_INTERFACE__REG_ACK_NO_RESPONSE);
        break;
    case RegState::FINISHED:
        // nothing to do, move along
        break;
    }
}

bool IpcRuntimeInterface::sendKeepalive() noexcept
{
    return m_RoudiMqInterface.send({IpcMessageTypeToString(IpcMessageType::KEEPALIVE), m_appName});
}

RelativePointer::offset_t IpcRuntimeInterface::getSegmentManagerAddressOffset() const noexcept
{
    cxx::Ensures(m_segmentManagerAddressOffset.has_value()
                 && "No segment manager available! Should have been fetched in the c'tor");
    return m_segmentManagerAddressOffset.value();
}

bool IpcRuntimeInterface::sendRequestToRouDi(const IpcMessage& msg, IpcMessage& answer) noexcept
{
    if (!m_RoudiMqInterface.send(msg))
    {
        LogError() << "Could not send request via RouDi messagequeue interface.\n";
        return false;
    }

    if (!m_AppMqInterface.receive(answer))
    {
        LogError() << "Could not receive request via App messagequeue interface.\n";
        return false;
    }

    return true;
}

bool IpcRuntimeInterface::sendMessageToRouDi(const IpcMessage& msg) noexcept
{
    if (!m_RoudiMqInterface.send(msg))
    {
        LogError() << "Could not send message via RouDi messagequeue interface.\n";
        return false;
    }
    return true;
}

size_t IpcRuntimeInterface::getShmTopicSize() noexcept
{
    return m_shmTopicSize;
}

void IpcRuntimeInterface::waitForRoudi(cxx::DeadlineTimer& timer) noexcept
{
    bool printWaitingWarning = true;
    bool printFoundMessage = false;
    while (!timer.hasExpired() && !m_RoudiMqInterface.isInitialized())
    {
        m_RoudiMqInterface.reopen();

        if (m_RoudiMqInterface.isInitialized())
        {
            LogDebug() << "RouDi mqueue found!";
            break;
        }

        if (printWaitingWarning)
        {
            LogWarn() << "RouDi not found - waiting ...";
            printWaitingWarning = false;
            printFoundMessage = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (printFoundMessage && m_RoudiMqInterface.isInitialized())
    {
        LogWarn() << "... RouDi found.";
    }
}

IpcRuntimeInterface::RegAckResult IpcRuntimeInterface::waitForRegAck(int64_t transmissionTimestamp) noexcept
{
    // wait for the register ack from the RouDi daemon. If we receive another response we do a retry
    // @todo if message queues are properly setup and cleaned up, always the expected REG_ACK should be received here,
    // so if not this issue should be passed to the error handling later
    constexpr size_t MAX_RETRY_COUNT = 3;
    size_t retryCounter = 0;
    while (retryCounter++ < MAX_RETRY_COUNT)
    {
        using namespace units::duration_literals;
        IpcMessage receiveBuffer;
        // wait for IpcMessageType::REG_ACK from RouDi for 1 seconds
        if (m_AppMqInterface.timedReceive(1_s, receiveBuffer))
        {
            std::string cmd = receiveBuffer.getElementAtIndex(0U);

            if (stringToIpcMessageType(cmd.c_str()) == IpcMessageType::REG_ACK)
            {
                constexpr uint32_t REGISTER_ACK_PARAMETERS = 5U;
                if (receiveBuffer.getNumberOfElements() != REGISTER_ACK_PARAMETERS)
                {
                    errorHandler(Error::kMQ_INTERFACE__REG_ACK_INVALIG_NUMBER_OF_PARAMS);
                }

                // read out the shared memory base address and save it
                iox::cxx::convert::fromString(receiveBuffer.getElementAtIndex(1U).c_str(), m_shmTopicSize);
                RelativePointer::offset_t offset{0U};
                iox::cxx::convert::fromString(receiveBuffer.getElementAtIndex(2U).c_str(), offset);
                m_segmentManagerAddressOffset.emplace(offset);

                int64_t receivedTimestamp{0U};
                cxx::convert::fromString(receiveBuffer.getElementAtIndex(3U).c_str(), receivedTimestamp);
                cxx::convert::fromString(receiveBuffer.getElementAtIndex(4U).c_str(), m_segmentId);
                if (transmissionTimestamp == receivedTimestamp)
                {
                    return RegAckResult::SUCCESS;
                }
                else
                {
                    LogWarn() << "Received a REG_ACK with an outdated timestamp!";
                }
            }
            else
            {
                LogError() << "Wrong response received " << receiveBuffer.getMessage();
            }
        }
    }

    return RegAckResult::TIMEOUT;
}

uint64_t IpcRuntimeInterface::getSegmentId() const noexcept
{
    return m_segmentId;
}
} // namespace runtime
} // namespace iox