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

#ifndef IOX_POSH_POPO_CLIENT_HPP
#define IOX_POSH_POPO_CLIENT_HPP

#include "iceoryx_posh/capro/service_description.hpp"

namespace iox
{
namespace popo
{
template <typename Req, typename Res>
class Client
{
  public:
    Client(const capro::ServiceDescription& service) noexcept;

  private:
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/client.inl"

#endif // IOX_POSH_POPO_CLIENT_HPP
