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

#include "iceoryx_posh/popo/callback_delegator.hpp"

namespace iox
{
namespace popo
{
CallbackDelegator::CallbackDelegator(const cxx::MethodCallback<void, cxx::function_ref<void()>>& callbackDelegator,
                                     const cxx::MethodCallback<void, CallbackDelegator&>& releaseCallback)
    : m_callbackDelegator{callbackDelegator}
    , m_releaseCallback{releaseCallback}
{
}

CallbackDelegator::~CallbackDelegator()
{
    if (m_releaseCallback)
    {
        m_releaseCallback(*this);
    }

    m_destructorBlocker.wait();
}

void CallbackDelegator::delegateCall(const cxx::function_ref<void()>& callback) noexcept
{
    if (m_callbackDelegator)
    {
        m_callbackDelegator(callback);
    }
}

void CallbackDelegator::unblockDestructor() noexcept
{
    m_destructorBlocker.post();
}


} // namespace popo
} // namespace iox
