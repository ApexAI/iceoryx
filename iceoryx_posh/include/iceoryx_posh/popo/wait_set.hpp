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
#ifndef IOX_POSH_POPO_WAIT_SET_HPP
#define IOX_POSH_POPO_WAIT_SET_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_data.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/condition_variable_waiter.hpp"
#include "iceoryx_posh/popo/trigger.hpp"
#include "iceoryx_posh/popo/trigger_handle.hpp"
#include "iceoryx_utils/cxx/function_ref.hpp"
#include "iceoryx_utils/cxx/method_callback.hpp"
#include "iceoryx_utils/cxx/vector.hpp"

namespace iox
{
namespace popo
{
class Condition;

enum class WaitSetError : uint8_t
{
    TRIGGER_VECTOR_OVERFLOW,
    TRIGGER_ALREADY_ACQUIRED,
};

/// @brief Logical disjunction of a certain number of Triggers
///
/// The WaitSet stores Triggers and allows the user to wait till one or more of those Triggers are triggered. It works
/// over process borders. With the creation of a WaitSet it requests a condition variable from RouDi and destroys it
/// with the destructor. Hence the lifetime of the condition variable is bound to the lifetime of the WaitSet.
class WaitSet
{
  public:
    using TriggerVector = cxx::vector<Trigger, MAX_NUMBER_OF_TRIGGERS_PER_WAITSET>;
    using TriggerInfoVector = cxx::vector<TriggerInfo, MAX_NUMBER_OF_TRIGGERS_PER_WAITSET>;

    WaitSet() noexcept;
    ~WaitSet() noexcept;

    /// @brief all the Trigger have a pointer pointing to this waitset for cleanup
    ///        calls, therefore the WaitSet cannot be moved
    WaitSet(const WaitSet& rhs) = delete;
    WaitSet(WaitSet&& rhs) = delete;
    WaitSet& operator=(const WaitSet& rhs) = delete;
    WaitSet& operator=(WaitSet&& rhs) = delete;

    /// @brief Acquires a trigger from the waitset  The trigger is then attached to an object, the origin,
    ///        which triggers the Trigger if a specific event happens. The object must then signal the
    ///        trigger that it was triggered via the triggerCallback. If the WaitSet goes out of scope
    ///        before the object does it calls the invalidationCallback to invalidate the Trigger inside
    ///        of the object
    ///        You cannot acquire an already logically equal acquired trigger. This means if you acquire a trigger
    ///        twice with the same: origin, triggerCallback and triggerId this method will return
    ///        TRIGGER_ALREADY_ACQUIRED
    /// @param[in] origin the pointer to the object which will attach the trigger
    /// @param[in] triggerCallback a method from the object which will signal the Trigger that it was triggered
    /// @param[in] invalidationCallback callback which will be called in the destructor of the waitset, important when
    /// the waitset goes out of scope before the origin does.
    /// @param[in] triggerId an arbitrary id to identify the trigger later when a list of triggers is returned via wait
    /// or timedWait
    /// @param[in] callback a callback which is attached to the trigger and can be later called when the trigger will be
    ///            returned via wait or timedWait
    /// @return returns the newly created trigger if the WaitSet has space left otherwise it returns
    /// WaitSetError::TRIGGER_VECTOR_OVERFLOW
    template <typename T>
    cxx::expected<TriggerHandle, WaitSetError>
    acquireTrigger(T* const origin,
                   const cxx::ConstMethodCallback<bool>& triggerCallback,
                   const cxx::MethodCallback<void, uint64_t>& invalidationCallback,
                   const uint64_t triggerId = Trigger::INVALID_TRIGGER_ID,
                   const Trigger::Callback<T> callback = nullptr) noexcept;

    /// @brief Blocking wait with time limit till one or more of the triggers are triggered
    /// @param[in] timeout How long shall we waite for a trigger
    /// @return TriggerInfoVector of TriggerInfos that have been triggered
    TriggerInfoVector timedWait(const units::Duration timeout) noexcept;

    /// @brief Blocking wait till one or more of the triggers are triggered
    /// @return TriggerInfoVector of TriggerInfos that have been triggered
    TriggerInfoVector wait() noexcept;

    /// @brief Returns the amount of stored Trigger inside of the WaitSet
    uint64_t size() const noexcept;

    /// @brief returns the maximum amount of triggers which can be acquired from a waitset
    uint64_t triggerCapacity() const noexcept;

    friend class Reactal;

  protected:
    explicit WaitSet(cxx::not_null<ConditionVariableData* const>) noexcept;

  private:
    TriggerInfoVector waitAndReturnTriggeredTriggers(const units::Duration& timeout) noexcept;
    template <typename WaitFunction>
    TriggerInfoVector waitAndReturnTriggeredTriggers(const WaitFunction& wait) noexcept;
    TriggerInfoVector createVectorWithTriggeredTriggers() noexcept;

    template <typename T>
    void moveOriginOfTrigger(const Trigger& trigger, T* const newOrigin) noexcept;

    void removeTrigger(const uint64_t uniqueTriggerId) noexcept;
    void removeAllTriggers() noexcept;

  private:
    TriggerVector m_triggerVector;
    ConditionVariableData* m_conditionVariableDataPtr{nullptr};
    ConditionVariableWaiter m_conditionVariableWaiter;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/wait_set.inl"

#endif // IOX_POSH_POPO_WAIT_SET_HPP
