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

#ifndef IOX_HOOFS_POSIX_WRAPPER_CONDITION_HPP
#define IOX_HOOFS_POSIX_WRAPPER_CONDITION_HPP

#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iceoryx_hoofs/cxx/helplets.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/mutex.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"

namespace iox
{
namespace posix
{
enum class ConditionScope
{
    SINGLE_PROCESS,
    INTER_PROCESS
};

enum class ConditionError
{
    INSUFFICIENT_MEMORY,
    MEMORY_CORRUPTED,
    INTERNAL_LOGIC_ERROR
};

class Condition;
class ConditionBuilder
{
    IOX_BUILDER_PARAMETER(ConditionScope, scope, ConditionScope::INTER_PROCESS)

  public:
    cxx::expected<ConditionError> create(cxx::optional<Condition>& storage) noexcept;
};

class Condition
{
  public:
    ~Condition() noexcept;

    Condition(const Condition&) = delete;
    Condition(Condition&&) = delete;
    Condition& operator=(const Condition&) = delete;
    Condition& operator=(Condition&&) = delete;

    bool waitFor(const units::Duration& timeout) noexcept;
    void wait() noexcept;

    void notifyOne() noexcept;
    void notifyAll() noexcept;

  private:
    template <typename>
    friend class ConditionVariable;
    friend class ConditionBuilder;
    friend class cxx::optional<Condition>;

    pthread_cond_t m_conditionVariable;
    mutable mutex m_mutex{false};

  private:
    Condition() noexcept = default;

    bool waitForWithoutLock(struct timespec& timeout) noexcept;
    void waitWithoutLock() noexcept;
};
} // namespace posix
} // namespace iox

#endif
