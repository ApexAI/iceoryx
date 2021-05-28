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

#ifndef IOX_POSH_POPO_CLIENT_INL
#define IOX_POSH_POPO_CLIENT_INL

namespace iox
{
namespace popo
{
template <typename Req, typename Res, typename Port>
Client<Req, Res, Port>::Client(const capro::ServiceDescription& service, const ClientOptions& clientOptions) noexcept
    : m_port(*iox::runtime::PoshRuntime::getInstance().getMiddlewareClient(service, clientOptions))
{
}

template <typename Req, typename Res, typename Port>
cxx::expected<RequestHeader*, AllocationError> Client<Req, Res, Port>::allocateRequest() noexcept
{
    return m_port.allocateRequest(sizeof(Req), alignof(Req));
}

template <typename Req, typename Res, typename Port>
void Client<Req, Res, Port>::freeRequest(RequestHeader* const requestHeader) noexcept
{
    m_port.freeRequest(requestHeader);
}

template <typename Req, typename Res, typename Port>
void Client<Req, Res, Port>::sendRequest(RequestHeader* const requestHeader) noexcept
{
    m_port.sendRequest(requestHeader);
}

template <typename Req, typename Res, typename Port>
cxx::expected<const ResponseHeader*, ChunkReceiveResult> Client<Req, Res, Port>::getResponse() noexcept
{
    return m_port.getResponse();
}

template <typename Req, typename Res, typename Port>
void Client<Req, Res, Port>::releaseResponse(const ResponseHeader* const responseHeader) noexcept
{
    m_port.releaseResponse(responseHeader);
}

template <typename Req, typename Res, typename Port>
inline void Client<Req, Res, Port>::enableEvent(iox::popo::TriggerHandle&& triggerHandle,
                                                const ClientEvent clientEvent) noexcept
{
    switch (clientEvent)
    {
    case ClientEvent::RESPONSE_RECEIVED:
        if (m_trigger)
        {
            LogWarn()
                << "The server is already attached with the ClientEvent::RESPONSE_RECEIVED to a WaitSet/Listener. "
                   "Detaching it from previous one and attaching it to the new one with "
                   "ClientEvent::RESPONSE_RECEIVED. Best practice is to call detach first.";

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
inline void Client<Req, Res, Port>::disableEvent(const ClientEvent clientEvent) noexcept
{
    switch (clientEvent)
    {
    case ClientEvent::RESPONSE_RECEIVED:
        m_trigger.reset();
        m_port.unsetConditionVariable();
        break;
    }
}

template <typename Req, typename Res, typename Port>
inline void Client<Req, Res, Port>::invalidateTrigger(const uint64_t uniqueTriggerId) noexcept
{
    if (m_trigger.getUniqueId() == uniqueTriggerId)
    {
        m_port.unsetConditionVariable();
        m_trigger.invalidate();
    }
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_CLIENT_INL
