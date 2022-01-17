// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_POSH_RUNTIME_SERVICE_DISCOVERY_HPP
#define IOX_POSH_RUNTIME_SERVICE_DISCOVERY_HPP

#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"

namespace iox
{
namespace runtime
{
// public/private can be sorted out later
class DiscoveryStateIndicator
{
  public:
    uint64_t serviceChangeCounter{0};

    bool operator==(const DiscoveryStateIndicator& other)
    {
        return other.serviceChangeCounter == serviceChangeCounter;
    }

    bool operator!=(const DiscoveryStateIndicator& other)
    {
        return !(*this == other);
    }
};

class ServiceDiscovery
{
  public:
    ServiceDiscovery() noexcept = default;
    ServiceDiscovery(const ServiceDiscovery&) = delete;
    ServiceDiscovery& operator=(const ServiceDiscovery&) = delete;
    ServiceDiscovery(ServiceDiscovery&&) = delete;
    ServiceDiscovery& operator=(ServiceDiscovery&&) = delete;
    ~ServiceDiscovery() noexcept = default;

    /// @brief find all services that match the provided service description
    /// @param[in] service service string to search for (wildcards allowed)
    /// @param[in] instance instance string to search for (wildcards allowed)
    /// @return cxx::expected<ServiceContainer, FindServiceError>
    /// ServiceContainer: on success, container that is filled with all matching instances
    /// FindServiceError: if any, encountered during the operation
    cxx::expected<ServiceContainer, FindServiceError>
    findService(const cxx::variant<Wildcard_t, capro::IdString_t> service,
                const cxx::variant<Wildcard_t, capro::IdString_t> instance) noexcept;

    /// @brief offer the provided service, sends the offer from application to RouDi daemon
    /// @param[in] service valid ServiceDescription to offer
    /// @return bool, if service is offered returns true else false
    bool offerService(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief stop offering the provided service
    /// @param[in] service valid ServiceDescription that shall be no more offered
    /// @return bool, if service is not offered anymore returns true else false
    bool stopOfferService(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief requests the serviceRegistryChangeCounter from the shared memory
    /// @return pointer to the serviceRegistryChangeCounter
    virtual const std::atomic<uint64_t>* getServiceRegistryChangeCounter() noexcept;

    DiscoveryStateIndicator getStateIndicator()
    {
        return m_stateIndicator;
    }

    const DiscoveryStateIndicator& update()
    {
        m_stateIndicator.serviceChangeCounter = getServiceRegistryChangeCounter()->load();
        return m_stateIndicator;
    }

    // DiscoveryStateIndicator waitForService(Servicedescription, DiscoveryStateIndicator){

    // };

    // DiscoveryStateIndicator waitForService(Servicedescription){

    // };

    // DiscoveryStateIndicator waitForAnyServiceChange(){

    // };

    void wait();

  private:
    popo::ApplicationPort m_applicationPort{PoshRuntime::getInstance().getMiddlewareApplication()};

    DiscoveryStateIndicator m_stateIndicator;
};

// working title
class CallbackServiceDiscovery
{
  public:
    struct Context
    {
        uint64_t knownCounter{0}; // atomic?
    };

    // not inuitive to have this static (required for listener...)
    // is there a way to deal with more general callbacks?
    static void wake_up(CallbackServiceDiscovery* const self)
    {
        (void)self;
    }

    static void wake_up(CallbackServiceDiscovery* const self, Context* const)
    {
        (void)self;
    }

    static void wake_up_callback(iox::popo::UserTrigger* trigger, Context* const context)
    {
        (void)trigger;
        context->knownCounter = getCounterFromRoudi();
    }

    // there is only one roudi so a static method is OK
    static uint64_t getCounterFromRoudi()
    {
        return 0; // stub, needs to get the updated counter
    }

    // generalize for any condition
    bool conditionOfInterest()
    {
        return true;
    }

    void blockingWaitUntilCounterChanges(uint64_t oldCounter)
    {
        // TODO: block
    }


    void waitForChange(uint64_t oldCounter)
    {
        m_knownCounter = getCounterFromRoudi();

        if (m_knownCounter != oldCounter)
        {
            return; // change happened already
        }

        // no change, register callback to be woken up on change

        // auto callback1 = iox::popo::createNotificationCallback(wake_up);
        // auto callback2 = iox::popo::createNotificationCallback(wake_up, m_context);
        auto callback3 = iox::popo::createNotificationCallback(wake_up_callback, m_context);

        // we want to register some code to be executed on wakeup or to wake_up
        // the wake-up notification ghas to come from the port or Roudi

        m_listener.attachEvent(m_trigger, callback3).or_else([](auto) {
            std::cerr << "unable to attach event" << std::endl;
        });

        // callback is registered

        // we cannot be sure we do not miss the wake-up otherwise
        m_knownCounter = getCounterFromRoudi();
        if (m_knownCounter != oldCounter)
        {
            m_listener.detachEvent(m_trigger);
            return; // change happened during notification
        }

        // TODO: need the waitset
        blockingWaitUntilCounterChanges(oldCounter);

        m_listener.detachEvent(m_trigger);

        // TODO: looks way to heavy
        // 1) notification has to come from roudi (or a port)
        // 2) code executed is a callback in general but in a specific case just something that unblocks us
        // 3) need a local blocking mechanism
        // 4) how many waiters do we need to support?
        // 5) listener vs. waitset here? (we need the waitset if we want to block and the listener if we want to
        //    call async callbacks)
    }

    Context m_context;
    iox::popo::Listener m_listener;

    // needs to live in Roudi to be activated on change of the registry
    // (alternatively we can use the built-in trigger of a port)
    iox::popo::UserTrigger m_trigger;

    uint64_t m_knownCounter{0};
};


} // namespace runtime

} // namespace iox

#endif // IOX_POSH_RUNTIME_SERVICE_DISCOVERY_HPP