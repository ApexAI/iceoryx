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

#ifndef IOX_POSH_POPO_SERVER_INL
#define IOX_POSH_POPO_SERVER_INL

namespace iox
{
namespace popo
{
template <typename Req, typename Res, typename Port>
inline Server<Req, Res, Port>::Server(const capro::ServiceDescription& service,
                                      const ServerOptions& serverOptions) noexcept
    : m_port(*iox::runtime::PoshRuntime::getInstance().getMiddlewareServer(service, serverOptions))
{
}

template <typename Req, typename Res, typename Port>
inline cxx::expected<const RequestHeader*, ChunkReceiveResult> Server<Req, Res, Port>::getRequest() noexcept
{
    return m_port.getRequest();
}

template <typename Req, typename Res, typename Port>
inline void Server<Req, Res, Port>::releaseRequest(const RequestHeader* const requestHeader) noexcept
{
    m_port.releaseRequest(requestHeader);
}

template <typename Req, typename Res, typename Port>
inline cxx::expected<ResponseHeader*, AllocationError>
Server<Req, Res, Port>::allocateResponse(const RequestHeader* const requestHeader) noexcept
{
    return m_port.allocateResponse(requestHeader, sizeof(Res), alignof(Res));
}

template <typename Req, typename Res, typename Port>
inline void Server<Req, Res, Port>::freeResponse(ResponseHeader* const responseHeader) noexcept
{
    m_port.freeResponse(responseHeader);
}

template <typename Req, typename Res, typename Port>
inline void Server<Req, Res, Port>::sendResponse(ResponseHeader* const responseHeader) noexcept
{
    m_port.sendResponse(responseHeader);
}

template <typename Req, typename Res, typename Port>
inline void Server<Req, Res, Port>::enableEvent(iox::popo::TriggerHandle&& triggerHandle,
                                                const ServerEvent serverEvent) noexcept
{
    switch (serverEvent)
    {
    case ServerEvent::REQUEST_RECEIVED:
        if (m_trigger)
        {
            LogWarn() << "The server is already attached with the ServerEvent::REQUEST_RECEIVED to a WaitSet/Listener. "
                         "Detaching it from previous one and attaching it to the new one with "
                         "SubscriberEvent::REQUEST_RECEIVED. "
                         " Best practice is to call detach first.";

            /// @todo iox-#27 call error handler
            // errorHandler(
            //    Error::kPOPO__BASE_SUBSCRIBER_OVERRIDING_WITH_EVENT_SINCE_HAS_DATA_OR_DATA_RECEIVED_ALREADY_ATTACHED,
            //    nullptr,
            //    ErrorLevel::MODERATE);
        }
        m_trigger = std::move(triggerHandle);
        m_port.setConditionVariable(*m_trigger.getConditionVariableData(), m_trigger.getUniqueId());
        break;
    }
}

template <typename Req, typename Res, typename Port>
inline void Server<Req, Res, Port>::disableEvent(const ServerEvent serverEvent) noexcept
{
    switch (serverEvent)
    {
    case ServerEvent::REQUEST_RECEIVED:
        m_trigger.reset();
        m_port.unsetConditionVariable();
        break;
    }
}

template <typename Req, typename Res, typename Port>
inline void Server<Req, Res, Port>::invalidateTrigger(const uint64_t uniqueTriggerId) noexcept
{
    if (m_trigger.getUniqueId() == uniqueTriggerId)
    {
        m_port.unsetConditionVariable();
        m_trigger.invalidate();
    }
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_SERVER_INL
