// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/popo/reactal.hpp"

namespace iox
{
namespace popo
{
Reactal::Reactal() noexcept
    : m_thread(&Reactal::reactAndListenLoop, this)
{
    m_cleanupTrigger.attachTo(m_waitset);
}

Reactal::~Reactal()
{
    m_keepRunning.store(false, std::memory_order_relaxed);
    m_thread.join();
}

void Reactal::reactAndListenLoop() noexcept
{
    while (m_keepRunning.load(std::memory_order_relaxed))
    {
        std::unique_lock<std::recursive_mutex> lock(m_waitsetMutex);
        auto triggerInfoVector = m_waitset.wait(); // move lock inside

        for (auto& triggerInfo : triggerInfoVector)
        {
            triggerInfo();
            if (m_hasRemoveCalled)
            {
                continue;
            }
        }
        lock.unlock(); ///
    }
}

void Reactal::removeTrigger(const uint64_t uniqueTriggerId) noexcept
{
    m_hasRemoveCalled = true;
    m_cleanupTrigger.trigger();

    std::lock_guard<std::recursive_mutex> lock(m_waitsetMutex);

    m_waitset.removeTrigger(uniqueTriggerId);
}


} // namespace popo
} // namespace iox
