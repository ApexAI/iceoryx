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

#ifndef IOX_HOOFS_POSIX_WRAPPER_CONDITION_VARIABLE_HPP
#define IOX_HOOFS_POSIX_WRAPPER_CONDITION_VARIABLE_HPP

#include "iceoryx_hoofs/cxx/function.hpp"
#include "iceoryx_hoofs/internal/concurrent/smart_lock.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"
#include "iceoryx_hoofs/posix_wrapper/condition.hpp"

namespace iox
{
namespace posix
{
template <typename T>
class ConditionVariable
{
  public:
    using Proxy = typename concurrent::smart_lock<T, posix::mutex>::Proxy;

    using predicate_t = cxx::function<bool(T&)>;
    template <typename... Targs>
    explicit ConditionVariable(const ConditionScope scope, const predicate_t& predicate, Targs&&... args) noexcept;
    ~ConditionVariable() noexcept = default;

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable(ConditionVariable&&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;
    ConditionVariable& operator=(ConditionVariable&&) = delete;

    Proxy operator->() noexcept;
    const Proxy operator->() const noexcept;

    Proxy getScopeGuard() noexcept;
    const Proxy getScopeGuard() const noexcept;

    T read() const noexcept;
    void write(const T& t) noexcept;
    void writeAndNotifyOne(const T& t) noexcept;
    void writeAndNotifyAll(const T& t) noexcept;

    bool waitFor(const units::Duration& timeout) noexcept;
    void wait() noexcept;

    void notifyOne() noexcept;
    void notifyAll() noexcept;

  private:
    Condition m_condition;
    predicate_t m_predicate;
    T m_base;
};
} // namespace posix
} // namespace iox

#include "iceoryx_hoofs/internal/posix_wrapper/condition_variable.inl"

#endif
