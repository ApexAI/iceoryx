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

#ifndef IOX_POSH_POPO_CALLBACK_DELEGATOR_HPP
#define IOX_POSH_POPO_CALLBACK_DELEGATOR_HPP

#include "iceoryx_utils/cxx/function_ref.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/posix_wrapper/semaphore.hpp"

namespace iox
{
namespace popo
{
class CallbackDelegator
{
  public:
    CallbackDelegator(const CallbackDelegator&) = delete;
    CallbackDelegator(CallbackDelegator&&) = default;

    ~CallbackDelegator();

    CallbackDelegator& operator=(const CallbackDelegator&) = delete;
    CallbackDelegator& operator=(CallbackDelegator&&) = default;

    void delegateCall(const cxx::function_ref<void()>& callback) noexcept;

    template <uint64_t>
    friend class Reactal;

  private:
    CallbackDelegator(const cxx::MethodCallback<void, cxx::function_ref<void()>>& callbackDelegator,
                      const cxx::MethodCallback<void, CallbackDelegator&>& releaseCallback);

    void unblockDestructor() noexcept;

  private:
    posix::Semaphore m_destructorBlocker;
    cxx::MethodCallback<void, cxx::function_ref<void()>> m_callbackDelegator;
    cxx::MethodCallback<void, CallbackDelegator&> m_releaseCallback;
};


} // namespace popo
} // namespace iox

#endif
