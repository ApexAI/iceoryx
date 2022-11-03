// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_DUST_STD_STRING_COMPATABILITY_INL
#define IOX_DUST_STD_STRING_COMPATABILITY_INL

#include "iceoryx_dust/cxx/std_string_compatability.hpp"

namespace iox
{
namespace cxx
{

// template <uint64_t Capacity>
// inline string<Capacity>::operator std::string() const noexcept
// {
//     return std::string(c_str());
// }


// template <uint64_t Capacity>
// // TruncateToCapacity_t is a compile time variable to distinguish between constructors
// // NOLINTNEXTLINE(hicpp-named-parameter, readability-named-parameter)
// inline string<Capacity>::string(TruncateToCapacity_t, const std::string& other) noexcept
//     : string(TruncateToCapacity, other.c_str(), other.size())
// {
// }

} // namespace cxx
} // namespace iox

#endif // IOX_DUST_STD_STRING_COMPATABILITY_INL
