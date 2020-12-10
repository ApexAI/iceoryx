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
template <uint64_t NumberOfThreads>
inline Reactal<NumberOfThreads>::Reactal() noexcept
{
    for (uint64_t i = 0U; i < NumberOfThreads; ++i)
    {
        m_threads.emplace_back(&Reactal::reactAndListenLoop, this);
    }
}

template <uint64_t NumberOfThreads>
inline Reactal<NumberOfThreads>::~Reactal()
{
    m_keepRunning.store(false, std::memory_order_relaxed);
    for (uint64_t i = 0U; i < NumberOfThreads; ++i)
    {
        m_callbacks.send_wakeup_trigger();
    }

    algorithm::forEach(m_threads, [](auto& t) { t.join(); });
}

template <uint64_t NumberOfThreads>
inline CallbackDelegator Reactal<NumberOfThreads>::acquireCallbackDelegator() noexcept
{
    return CallbackDelegator({*this, &Reactal::executeCallback}, {*this, &Reactal::releaseCallbackDelegator});
}


template <uint64_t NumberOfThreads>
template <typename T>
inline void Reactal<NumberOfThreads>::executeCallback(const T& callback) noexcept
{
    m_callbacks.blocking_push(callback_t(cxx::in_place_type<T>(), callback));
}

template <uint64_t NumberOfThreads>
inline void Reactal<NumberOfThreads>::releaseCallbackDelegator(CallbackDelegator& handle) noexcept
{
    executeCallback(cxx::MethodCallback<void>(handle, &CallbackDelegator::unblockDestructor));
}

template <uint64_t NumberOfThreads>
inline void Reactal<NumberOfThreads>::reactAndListenLoop() noexcept
{
    while (m_keepRunning.load(std::memory_order_relaxed)
           || (!m_keepRunning.load(std::memory_order_relaxed) && !m_callbacks.empty()))
    {
        callback_t callback;
        if (m_callbacks.blocking_pop(callback))
        {
            switch (callback.index())
            {
            case static_cast<uint64_t>(CallbackType::FUNCTION_REF):
                (*callback.get_at_index<static_cast<uint64_t>(CallbackType::FUNCTION_REF)>())();
                break;
            case static_cast<uint64_t>(CallbackType::METHOD_CALLBACK):
                (*callback.get_at_index<static_cast<uint64_t>(CallbackType::METHOD_CALLBACK)>())();
                break;
            }
        }
    }
}


} // namespace popo
} // namespace iox

#endif
