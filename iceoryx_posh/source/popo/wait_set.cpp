// Copyright (c) 2020 by Robert Bosch GmbH, Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

namespace iox
{
namespace popo
{
WaitSet::WaitSet() noexcept
    : WaitSet(runtime::PoshRuntime::getInstance().getMiddlewareConditionVariable())
{
}

WaitSet::WaitSet(cxx::not_null<ConditionVariableData* const> condVarDataPtr) noexcept
    : m_conditionVariableDataPtr(condVarDataPtr)
    , m_conditionVariableWaiter(m_conditionVariableDataPtr)
{
}

WaitSet::~WaitSet() noexcept
{
    removeAllTriggers();
    /// @todo Notify RouDi that the condition variable data shall be destroyed
}

void WaitSet::removeTrigger(const uint64_t uniqueTriggerId) noexcept
{
    for (auto& currentTrigger : m_triggerVector)
    {
        if (currentTrigger.getUniqueId() == uniqueTriggerId)
        {
            currentTrigger.invalidate();
            m_triggerVector.erase(&currentTrigger);
            return;
        }
    }
}

void WaitSet::removeAllTriggers() noexcept
{
    for (auto& trigger : m_triggerVector)
    {
        trigger.reset();
    }

    m_triggerVector.clear();
}

typename WaitSet::TriggerInfoVector WaitSet::timedWait(const units::Duration timeout) noexcept
{
    return waitAndReturnTriggeredTriggers([this, timeout] { return !m_conditionVariableWaiter.timedWait(timeout); });
}

typename WaitSet::TriggerInfoVector WaitSet::wait() noexcept
{
    return waitAndReturnTriggeredTriggers([this] {
        m_conditionVariableWaiter.wait();
        return false;
    });
}

typename WaitSet::TriggerInfoVector WaitSet::createVectorWithTriggeredTriggers() noexcept
{
    TriggerInfoVector triggers;
    for (auto& currentTrigger : m_triggerVector)
    {
        if (currentTrigger.hasTriggered())
        {
            // We do not need to verify if push_back was successful since
            // m_conditionVector and triggers are having the same type, a
            // vector with the same guaranteed capacity.
            // Therefore it is guaranteed that push_back works!
            triggers.push_back(currentTrigger.getTriggerInfo());
        }
    }

    return triggers;
}

template <typename WaitFunction>
typename WaitSet::TriggerInfoVector WaitSet::waitAndReturnTriggeredTriggers(const WaitFunction& wait) noexcept
{
    WaitSet::TriggerInfoVector triggers;

    /// Inbetween here and last wait someone could have set the trigger to true, hence reset it.
    m_conditionVariableWaiter.reset();
    triggers = createVectorWithTriggeredTriggers();

    // It is possible that after the reset call and before the createVectorWithTriggeredTriggers call
    // another trigger came in. Then createVectorWithTriggeredTriggers would have already handled it.
    // But this would lead to an empty triggers vector in the next run if no other trigger
    // came in.
    if (!triggers.empty())
    {
        return triggers;
    }

    // reset
    // uniqueLock.unlock();

    return (wait()) ? triggers : (lock)createVectorWithTriggeredTriggers();
}

uint64_t WaitSet::size() const noexcept
{
    return m_triggerVector.size();
}

uint64_t WaitSet::triggerCapacity() const noexcept
{
    return m_triggerVector.capacity();
}


} // namespace popo
} // namespace iox
