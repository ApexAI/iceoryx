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
Server<Req, Res, Port>::Server(const capro::ServiceDescription& service, const ServerOptions& serverOptions) noexcept
    : m_port(*iox::runtime::PoshRuntime::getInstance().getMiddlewareServer(service, serverOptions))
{
}

template <typename Req, typename Res, typename Port>
cxx::expected<const RequestHeader*, ChunkReceiveResult> Server<Req, Res, Port>::getRequest() noexcept
{
    return m_port.getRequest();
}

template <typename Req, typename Res, typename Port>
void Server<Req, Res, Port>::releaseRequest(const RequestHeader* const requestHeader) noexcept
{
    m_port.releaseRequest(requestHeader);
}

template <typename Req, typename Res, typename Port>
cxx::expected<ResponseHeader*, AllocationError> Server<Req, Res, Port>::allocateResponse() noexcept
{
    return m_port.allocateResponse(sizeof(Res), alignof(Res));
}

template <typename Req, typename Res, typename Port>
void Server<Req, Res, Port>::freeResponse(ResponseHeader* const responseHeader) noexcept
{
    m_port.freeResponse(responseHeader);
}

template <typename Req, typename Res, typename Port>
void Server<Req, Res, Port>::sendResponse(ResponseHeader* const responseHeader) noexcept
{
    m_port.sendResponse(responseHeader);
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_SERVER_INL
