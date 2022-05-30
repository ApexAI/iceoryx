// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2022 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_HOOFS_CXX_VARIANT_QUEUE_INL
#define IOX_HOOFS_CXX_VARIANT_QUEUE_INL

#include "iceoryx_hoofs/cxx/variant_queue.hpp"

namespace iox
{
namespace cxx
{
template <typename ValueType, uint64_t Capacity>
inline VariantQueue<ValueType, Capacity>::VariantQueue(const VariantQueueTypes type) noexcept
    : m_type(type)
{
    m_fifo.template emplace<concurrent::MutexQueue<ValueType, Capacity>>();
}

template <typename ValueType, uint64_t Capacity>
optional<ValueType> VariantQueue<ValueType, Capacity>::push(const ValueType& value) noexcept
{
    return m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
        ->push(value);
}

template <typename ValueType, uint64_t Capacity>
inline optional<ValueType> VariantQueue<ValueType, Capacity>::pop() noexcept
{
    return m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
        ->pop();
}

template <typename ValueType, uint64_t Capacity>
inline bool VariantQueue<ValueType, Capacity>::empty() const noexcept
{
    return m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
        ->empty();
}

template <typename ValueType, uint64_t Capacity>
inline uint64_t VariantQueue<ValueType, Capacity>::size() noexcept
{
    return m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
        ->size();
}


template <typename ValueType, uint64_t Capacity>
inline bool VariantQueue<ValueType, Capacity>::setCapacity(const uint64_t) noexcept
{
    assert(false);
    return false;
}

template <typename ValueType, uint64_t Capacity>
inline uint64_t VariantQueue<ValueType, Capacity>::capacity() const noexcept
{
    return m_fifo.template get_at_index<static_cast<uint64_t>(VariantQueueTypes::SoFi_SingleProducerSingleConsumer)>()
        ->capacity();
}

template <typename ValueType, uint64_t Capacity>
inline typename VariantQueue<ValueType, Capacity>::fifo_t&
VariantQueue<ValueType, Capacity>::getUnderlyingFiFo() noexcept
{
    return m_fifo;
}

} // namespace cxx
} // namespace iox

#endif // IOX_HOOFS_CXX_VARIANT_QUEUE_INL
