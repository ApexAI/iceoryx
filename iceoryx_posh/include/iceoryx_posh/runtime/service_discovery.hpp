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

    iox::popo::Listener m_listener;

    iox::popo::UserTrigger m_trigger; // needs to live in Roudi
};


} // namespace runtime

} // namespace iox

#endif // IOX_POSH_RUNTIME_SERVICE_DISCOVERY_HPP