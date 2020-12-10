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

#include "iceoryx_posh/popo/callback_delegator.hpp"
#include "iceoryx_utils/cxx/algorithm.hpp"
#include "iceoryx_utils/cxx/function_ref.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/cxx/variant.hpp"
#include "iceoryx_utils/cxx/vector.hpp"
#include "iceoryx_utils/internal/concurrent/trigger_queue.hpp"

#include <atomic>
#include <thread>

namespace iox
{
namespace popo
{
template <uint64_t NumberOfThreads>
class Reactal
{
  public:
    using callback_t = cxx::variant<cxx::function_ref<void()>, cxx::MethodCallback<void>>;
    enum class CallbackType : uint64_t
    {
        FUNCTION_REF = 0,
        METHOD_CALLBACK = 1
    };

    Reactal() noexcept;
    ~Reactal();

    CallbackDelegator acquireCallbackDelegator() noexcept;

  private:
    template <typename T>
    void execute(const T callback) noexcept;
    void releaseCallbackDelegator(CallbackDelegator& handle) noexcept;
    void reactAndListenLoop() noexcept;

  private:
    cxx::vector<std::thread, NumberOfThreads> m_threads;
    std::atomic_bool m_keepRunning{true};
    concurrent::TriggerQueue<callback_t, 100> m_callbacks;
};
} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/reactal.inl"

#endif
