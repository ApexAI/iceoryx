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

#ifndef IOX_POSH_POPO_REACTAL_INL
#define IOX_POSH_POPO_REACTAL_INL

namespace iox
{
namespace popo
{
template <typename T>
inline cxx::expected<TriggerHandle, WaitSetError>
Reactal::acquireTrigger(T* const origin,
                        const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
                        const Trigger::Callback<T> callback) noexcept
{
    m_cleanupTrigger.trigger();

    std::lock_guard<std::recursive_mutex> lock(m_waitsetMutex);

    auto result = m_waitset.acquireTrigger(
        origin, hasTriggeredCallback, {*this, &Reactal::removeTrigger}, Trigger::INVALID_TRIGGER_ID, callback);
    m_cleanupTrigger.resetTrigger();
}


} // namespace popo
} // namespace iox

#endif
