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

#ifndef IOX_POSH_POPO_STATUS_PORT_HPP
#define IOX_POSH_POPO_STATUS_PORT_HPP

#include "iceoryx_hoofs/cxx/function_ref.hpp"

namespace iox
{
namespace popo
{
template <typename T, typename H>
class StatusPortReader
{
  public:
    StatusPortReader() noexcept
    {
    }

    StatusPortReader(StatusPortReader&& rhs) = delete;
    StatusPortReader& operator=(StatusPortReader&& rhs) = delete;

    StatusPortReader(const StatusPortReader&) = delete;
    StatusPortReader& operator=(const StatusPortReader&) = delete;

    void takeChunk(cxx::function_ref<void(T&)> callable)
    {
    }
};

template <typename T, typename H>
class StatusPortWriter
{
  public:
    StatusPortWriter() noexcept
    {
    }

    StatusPortWriter(StatusPortWriter&& rhs) = delete;
    StatusPortWriter& operator=(StatusPortWriter&& rhs) = delete;

    StatusPortWriter(const StatusPortWriter&) = delete;
    StatusPortWriter& operator=(const StatusPortWriter&) = delete;

    void storeChunk(cxx::function_ref<void(T&)> callable)
    {
    }
};

#endif // IOX_POSH_POPO_STATUS_PORT_HPP

} // namespace popo
} // namespace iox
