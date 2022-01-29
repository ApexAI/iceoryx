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

#include "iceoryx_hoofs/cxx/generic_raii.hpp"
#include "iceoryx_hoofs/posix_wrapper/condition_variable.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"
#include "iceoryx_hoofs/posix_wrapper/thread.hpp"

namespace iox
{
namespace posix
{
static void printLogicWarning() noexcept
{
    std::cerr << "This should never happen. Internal logic error, maybe your system is not POSIX compliant or the "
                 "memory of the condition variable was corrupted?"
              << std::endl;
}

cxx::expected<ConditionError> ConditionBuilder::create(cxx::optional<Condition>& storage) noexcept
{
    storage.emplace();

    bool creationFailed = true;
    cxx::GenericRAII resetStorageWhenNotSuccessful{[&] {
        if (creationFailed)
        {
            storage.reset();
        }
    }};

    pthread_condattr_t attributes;

    auto result = posixCall(pthread_condattr_init)(&attributes).returnValueMatchesErrno().evaluate();
    if (result.has_error())
    {
        std::cerr << "Failed to create condition variable attribute." << std::endl;
        switch (result.get_error().errnum)
        {
        case ENOMEM:
            std::cerr << "Insufficient memory to initialize required condition variable attribute" << std::endl;
            return cxx::error<ConditionError>(ConditionError::INSUFFICIENT_MEMORY);
        default:
            printLogicWarning();
            return cxx::error<ConditionError>(ConditionError::INTERNAL_LOGIC_ERROR);
        }
    }

    result =
        posixCall(pthread_condattr_setpshared)(&attributes, static_cast<int>(m_scope == ConditionScope::INTER_PROCESS))
            .returnValueMatchesErrno()
            .evaluate();
    if (result.has_error())
    {
        std::cerr << "Failed to set condition variable scope in condition variable attribute." << std::endl;
        printLogicWarning();
        return cxx::error<ConditionError>(ConditionError::INTERNAL_LOGIC_ERROR);
    }

    result =
        posixCall(pthread_cond_init)(&storage->m_conditionVariable, &attributes).returnValueMatchesErrno().evaluate();
    if (result.has_error())
    {
        std::cerr << "Failed to create condition variable." << std::endl;
        switch (result.get_error().errnum)
        {
        case ENOMEM:
            std::cerr << "Insufficient memory to initialize condition variable" << std::endl;
            return cxx::error<ConditionError>(ConditionError::INSUFFICIENT_MEMORY);
        case EBUSY:
            std::cerr << "It seems that the memory of the condition variable is already initialized with a condition "
                         "variable. This can be a sign of a internal logic error or corrupted memory."
                      << std::endl;
            return cxx::error<ConditionError>(ConditionError::MEMORY_CORRUPTED);
        default:
            printLogicWarning();
            return cxx::error<ConditionError>(ConditionError::INTERNAL_LOGIC_ERROR);
        }
    }

    result = posixCall(pthread_condattr_destroy)(&attributes).returnValueMatchesErrno().evaluate();
    if (result.has_error())
    {
        std::cerr << "Failed to remove condition variable attribute." << std::endl;
        printLogicWarning();
        return cxx::error<ConditionError>(ConditionError::INTERNAL_LOGIC_ERROR);
    }

    creationFailed = false;
    return cxx::success<void>();
}

Condition::~Condition() noexcept
{
    auto result = posixCall(pthread_cond_destroy)(&m_conditionVariable).returnValueMatchesErrno().evaluate();
    if (result.has_error())
    {
        switch (result.get_error().errnum)
        {
        case EBUSY:
            std::cerr << "Trying to remove condition while it is still being used." << std::endl;
            break;
        default:
            printLogicWarning();
            break;
        }
    }
    cxx::Ensures(!result.has_error() && "Condition variable resource may be leaked.");
}

void Condition::notifyOne() noexcept
{
    auto result = posixCall(pthread_cond_signal)(&m_conditionVariable).returnValueMatchesErrno().evaluate();
    cxx::Ensures(!result.has_error() && "The underlying memory of the condition variable has been corrupted.");
}

void Condition::notifyAll() noexcept
{
    auto result = posixCall(pthread_cond_broadcast)(&m_conditionVariable).returnValueMatchesErrno().evaluate();
    cxx::Ensures(!result.has_error() && "The underlying memory of the condition variable has been corrupted.");
}

bool Condition::waitFor(const units::Duration& timeout) noexcept
{
    cxx::Ensures(m_mutex.lock() && "Underlying mutex of Condition is corrupted!");
    timespec t = timeout.timespec(units::TimeSpecReference::Epoch);
    auto hasTimeOut = waitForWithoutLock(t);
    cxx::Ensures(m_mutex.unlock() && "Underlying mutex of Condition is corrupted!");
    return hasTimeOut;
}

void Condition::wait() noexcept
{
    cxx::Ensures(m_mutex.lock() && "Underlying mutex of Condition is corrupted!");
    waitWithoutLock();
    cxx::Ensures(m_mutex.unlock() && "Underlying mutex of Condition is corrupted!");
}

bool Condition::waitForWithoutLock(struct timespec& timeout) noexcept
{
    auto result = posixCall(pthread_cond_timedwait)(&m_conditionVariable, &m_mutex.m_handle, &timeout)
                      .returnValueMatchesErrno()
                      .ignoreErrnos(ETIMEDOUT)
                      .evaluate();

    if (result.has_error())
    {
        printLogicWarning();
    }
    cxx::Ensures(!result.has_error() && "Error during waitFor in condition occurred.");

    return (result.value().value != ETIMEDOUT);
}

void Condition::waitWithoutLock() noexcept
{
    auto result =
        posixCall(pthread_cond_wait)(&m_conditionVariable, &m_mutex.m_handle).returnValueMatchesErrno().evaluate();

    if (result.has_error())
    {
        printLogicWarning();
    }
    cxx::Ensures(!result.has_error() && "Error during wait in condition occurred.");
}

} // namespace posix
} // namespace iox
