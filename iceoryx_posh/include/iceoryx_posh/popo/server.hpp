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

#ifndef IOX_POSH_POPO_SERVER_HPP
#define IOX_POSH_POPO_SERVER_HPP

#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/internal/popo/ports/server_port_user.hpp"
#include "iceoryx_posh/popo/server_options.hpp"
#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

namespace iox
{
namespace popo
{
template <typename Req, typename Res, typename Port = ServerPortUser>
class Server
{
  public:
    Server(const capro::ServiceDescription& service, const ServerOptions& serverOptions = {}) noexcept;

    cxx::expected<const RequestHeader*, ChunkReceiveResult> getRequest() noexcept;
    void releaseRequest(const RequestHeader* const requestHeader) noexcept;

    cxx::expected<ResponseHeader*, AllocationError> allocateResponse(const RequestHeader* const requestHeader) noexcept;
    void freeResponse(ResponseHeader* const responseHeader) noexcept;
    void sendResponse(ResponseHeader* const responseHeader) noexcept;

    friend class NotificationAttorney;

  private:
    void enableEvent(iox::popo::TriggerHandle&& triggerHandle, const ServerEvent serverEvent) noexcept;
    void disableEvent(const ServerEvent serverEvent) noexcept;
    void invalidateTrigger(const uint64_t trigger) noexcept;

  private:
    Port m_port;
    TriggerHandle m_trigger;
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/server.inl"

#endif // IOX_POSH_POPO_SERVER_HPP
