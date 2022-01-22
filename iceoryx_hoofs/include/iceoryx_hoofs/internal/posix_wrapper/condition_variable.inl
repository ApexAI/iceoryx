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

#ifndef IOX_HOOFS_POSIX_WRAPPER_CONDITION_VARIABLE_INL
#define IOX_HOOFS_POSIX_WRAPPER_CONDITION_VARIABLE_INL

namespace iox
{
namespace posix
{
template <typename T>
template <typename... Targs>
inline ConditionVariable<T>::ConditionVariable(const ConditionScope scope,
                                               const predicate_t& predicate,
                                               Targs&&... args) noexcept
    : m_condition{scope}
    , m_predicate{predicate}
    , m_base{std::forward<Targs>(args)...}
{
}

template <typename T>
inline typename ConditionVariable<T>::Proxy ConditionVariable<T>::operator->() noexcept
{
    return Proxy(m_base, m_condition.m_mutex);
}

template <typename T>
inline const typename ConditionVariable<T>::Proxy ConditionVariable<T>::operator->() const noexcept
{
    return Proxy(m_base, m_condition.m_mutex);
}

template <typename T>
inline typename ConditionVariable<T>::Proxy ConditionVariable<T>::getScopeGuard() noexcept
{
    return Proxy(m_base, m_condition.m_mutex);
}

template <typename T>
inline const typename ConditionVariable<T>::Proxy ConditionVariable<T>::getScopeGuard() const noexcept
{
    return Proxy(m_base, m_condition.m_mutex);
}

template <typename T>
inline void ConditionVariable<T>::waitFor(const units::Duration& timeout) noexcept
{
    m_condition.waitFor(timeout);
    // add waitForWithNoLock
}

template <typename T>
inline void ConditionVariable<T>::wait() noexcept
{
    m_condition.wait();
    // add waitNoLock
    // mutex.lock();
    // while( !m_predicate(m_base) )
    //   m_condition.waitNoLock();
    // mutex.unlock();
}

template <typename T>
inline void ConditionVariable<T>::notifyOne() noexcept
{
    m_condition.notifyOne();
}

template <typename T>
inline void ConditionVariable<T>::notifyAll() noexcept
{
    m_condition.notifyAll();
}
} // namespace posix
} // namespace iox

#endif
