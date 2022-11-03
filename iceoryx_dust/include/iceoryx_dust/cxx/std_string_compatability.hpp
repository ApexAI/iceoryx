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
#ifndef IOX_DUST_STD_STRING_COMPATABILITY_HPP
#define IOX_DUST_STD_STRING_COMPATABILITY_HPP

// rename to string conversion

#include "iceoryx_hoofs/cxx/string.hpp"

namespace iox
{
namespace cxx
{

/// @brief conversion constructor for std::string to string which truncates characters if the std::string size is
/// greater than the string capacity
///
/// @param [in] TruncateToCapacity_t is a compile time variable which is used to distinguish between
/// constructors with certain behavior
/// @param [in] other is the std::string to convert
/// @attention truncates characters if the std::string size is greater than the string capacity
///
/// @code
///     #include "iceoryx_hoofs/cxx/string.hpp"
///     using namespace iox::cxx;
///
///     int main()
///     {
///         std::string bar = "bar";
///         string<4> fuu(TruncateToCapacity, bar);
///     }
/// @endcode
// TruncateToCapacity_t is a compile time variable to distinguish between constructors
// NOLINTNEXTLINE(hicpp-named-parameter, readability-named-parameter)
// string(TruncateToCapacity_t, const std::string& other) noexcept;


/// @brief converts the string to a std::string
///
/// @return a std::string with data equivalent to those stored in the string
// NOLINTNEXTLINE(hicpp-explicit-conversions) @todo iox-#260 remove this conversion and implement toStdString method
// operator std::string() const noexcept;

template <typename Source, typename Destination>
struct From;

template <typename F, typename T>
constexpr T convertFrom(const F value) noexcept
{
    return From<F, T>::convertFrom(value);
}

template <uint64_t N>
struct From<string<N>, std::string>
{
    From() = default;
    static std::string convertFrom(const string<N>& value)
    {
        return std::string(value.c_str(), value.size());
    }
};

template <uint64_t N>
struct From<std::string, string<N>>
{
    From() = default;
    static string<N> convertFrom(const std::string& value)
    {
        return string<N>(TruncateToCapacity, value.c_str(), value.size());
    }
};

} // namespace cxx
} // namespace iox

#include "iceoryx_dust/internal/cxx/std_string_compatability.inl"

#endif // IOX_DUST_STD_STRING_COMPATABILITY_HPP