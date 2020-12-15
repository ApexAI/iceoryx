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

#ifndef IOX_POSH_POPO_REACTAL_HPP
#define IOX_POSH_POPO_REACTAL_HPP

#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

#include <atomic>
#include <mutex>
#include <thread>

namespace iox
{
namespace popo
{
class Reactal
{
  public:
    Reactal() noexcept;
    ~Reactal();

    template <typename T>
    cxx::expected<TriggerHandle, WaitSetError>
    acquireTrigger(T* const origin,
                   const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
                   const Trigger::Callback<T> callback) noexcept;


  private:
    void removeTrigger(const uint64_t uniqueTriggerId) noexcept;
    void reactAndListenLoop() noexcept;

  private:
    std::thread m_thread;
    std::atomic_bool m_keepRunning{true};

    std::recursive_mutex m_waitsetMutex;
    WaitSet m_waitset;
    UserTrigger m_cleanupTrigger;
    std::atomic<uint64_t> m_pendingCleanups{0U};
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/reactal.inl"

#endif
