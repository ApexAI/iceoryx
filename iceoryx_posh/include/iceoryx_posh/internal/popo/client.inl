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

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_CLIENT_INL
