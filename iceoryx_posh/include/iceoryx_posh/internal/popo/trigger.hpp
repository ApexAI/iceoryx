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

#ifndef IOX_POSH_POPO_TRIGGER_HPP
#define IOX_POSH_POPO_TRIGGER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include <limits>

namespace iox
{
namespace popo
{
class Condition;
struct TriggerId
{
  public:
    TriggerId() = default;
    TriggerId(const uint64_t value) noexcept
        : m_value(value)
    {
    }

    operator uint64_t() const noexcept
    {
        return m_value;
    }

  private:
    uint64_t m_value = std::numeric_limits<uint64_t>::max();
};

class Trigger
{
  public:
    Trigger() noexcept = default;
    Trigger(const void* const origin,
            ConditionVariableData* conditionVariableDataPtr,
            const cxx::ConstMethodCallback<bool>& hasTriggeredCallback,
            const cxx::MethodCallback<void>& invalidationCallback,
            const uint64_t triggerId = TriggerId()) noexcept;

    Trigger(const Trigger& other, const cxx::MethodCallback<void, Trigger&>& removalCallback) noexcept;

    Trigger(const Trigger&) = delete;
    Trigger& operator=(const Trigger&) = delete;
    Trigger(Trigger&& rhs) noexcept;
    Trigger& operator=(Trigger&& rhs) noexcept;

    ~Trigger();

    explicit operator bool() const noexcept;
    bool isValid() const noexcept;

    bool hasTriggered() const noexcept;
    void invalidate() noexcept;
    void reset() noexcept;
    ConditionVariableData* getConditionVariableData() noexcept;

    const TriggerId& getTriggerId() const noexcept;

    bool operator==(const Trigger& rhs) const noexcept;

  private:
    const void* m_origin;
    ConditionVariableData* m_conditionVariableDataPtr{nullptr};

    cxx::MethodCallback<void, Trigger&> m_removalCallback;
    cxx::MethodCallback<void> m_invalidationCallback;
    cxx::ConstMethodCallback<bool> m_hasTriggeredCallback;

    TriggerId m_triggerId;
};


} // namespace popo
} // namespace iox

#endif