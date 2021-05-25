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

#include "iceoryx_posh/popo/server_options.hpp"
#include "iceoryx_posh/internal/log/posh_logging.hpp"

namespace iox
{
namespace popo
{
cxx::Serialization ServerOptions::serialize() const noexcept
{
    return cxx::Serialization::create(
        requestQueueCapacity,
        nodeName,
        offerOnCreate,
        static_cast<std::underlying_type<ClientTooSlowPolicy>::type>(clientTooSlowPolicy));
}

ServerOptions ServerOptions::deserialize(const cxx::Serialization& serialized) noexcept
{
    using ClientTooSlowPolicyUT = std::underlying_type<ClientTooSlowPolicy>::type;

    ServerOptions serverOptions;
    ClientTooSlowPolicyUT clientTooSlowPolicy;

    serialized.extract(
        serverOptions.requestQueueCapacity, serverOptions.nodeName, serverOptions.offerOnCreate, clientTooSlowPolicy);

    if (clientTooSlowPolicy > static_cast<ClientTooSlowPolicyUT>(ClientTooSlowPolicy::DISCARD_OLDEST_DATA))
    {
        /// @todo iox-#27 we cannot call cxx::Ensures here since it would terminate RouDi but the cxx::Serialization
        /// currently doesn't store a failed deserialization -> log warning and use sane default value
        LogWarn() << "Deserialization of ServerOptions::clientTooSlowPolicy failed! Got '" << clientTooSlowPolicy
                  << "' Using "
                     "ClientTooSlowPolicy::DISCARD_OLDEST_DATA as fallback";
        serverOptions.clientTooSlowPolicy = ClientTooSlowPolicy::DISCARD_OLDEST_DATA;
    }
    else
    {
        serverOptions.clientTooSlowPolicy = static_cast<ClientTooSlowPolicy>(clientTooSlowPolicy);
    }

    return serverOptions;
}
} // namespace popo
} // namespace iox
