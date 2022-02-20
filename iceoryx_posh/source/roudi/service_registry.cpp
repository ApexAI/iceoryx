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

#include "iceoryx_posh/internal/roudi/service_registry.hpp"

namespace iox
{
namespace roudi
{
ServiceRegistry::ServiceDescriptionEntry::ServiceDescriptionEntry(const capro::ServiceDescription& serviceDescription)
    : serviceDescription(serviceDescription)
{
}

cxx::expected<ServiceRegistry::Error> ServiceRegistry::add(const capro::ServiceDescription& serviceDescription,
                                                           ReferenceCounter_t ServiceDescriptionEntry::*count)
{
    auto index = findIndex(serviceDescription);
    if (index != NO_INDEX)
    {
        // multiple entries with the same service descripion are possible
        // and we just increase the count in this case (multi-set semantics)
        // slot exists, increment counter
        auto& slot = m_slots[index];
        ((*slot).*count)++;
        return cxx::success<>();
    }

    auto updateSlot = [&](Slot_t& slot) {
        slot.beginWrite();
        slot.data().emplace(serviceDescription);
        (*slot).*count = 1U;
        slot.endWrite(updateGeneration());
    };

    // slot does not exist, find a free slot if it exists

    // fast path to a free slot (which was occupied by previously removed slot),
    // prefer to fill entries close to the front
    if (m_freeIndex != NO_INDEX)
    {
        auto& slot = m_slots[m_freeIndex];
        updateSlot(slot);
        m_freeIndex = NO_INDEX;
        return cxx::success<>();
    }

    // search from start
    for (auto& slot : m_slots)
    {
        if (!slot)
        {
            updateSlot(slot);
            return cxx::success<>();
        }
    }

    // append new slot at the end (the size only grows up to capacity)
    if (m_slots.emplace_back())
    {
        auto& slot = m_slots.back();
        updateSlot(slot);
        return cxx::success<>();
    }

    return cxx::error<Error>(Error::SERVICE_REGISTRY_FULL);
}

cxx::expected<ServiceRegistry::Error>
ServiceRegistry::addPublisher(const capro::ServiceDescription& serviceDescription) noexcept
{
    return add(serviceDescription, &ServiceDescriptionEntry::publisherCount);
}

cxx::expected<ServiceRegistry::Error>
ServiceRegistry::addServer(const capro::ServiceDescription& serviceDescription) noexcept
{
    return add(serviceDescription, &ServiceDescriptionEntry::serverCount);
}

void ServiceRegistry::removePublisher(const capro::ServiceDescription& serviceDescription) noexcept
{
    auto index = findIndex(serviceDescription);
    if (index != NO_INDEX)
    {
        auto& slot = m_slots[index];

        if (slot && slot->publisherCount >= 1U)
        {
            if (--slot->publisherCount == 0U && slot->serverCount == 0)
            {
                slot.reset(updateGeneration());
                // reuse the slot in the next insertion
                m_freeIndex = index;
            }
        }
    }
}

void ServiceRegistry::removeServer(const capro::ServiceDescription& serviceDescription) noexcept
{
    auto index = findIndex(serviceDescription);
    if (index != NO_INDEX)
    {
        auto& slot = m_slots[index];

        if (slot && slot->serverCount >= 1U)
        {
            if (--slot->serverCount == 0U && slot->publisherCount == 0)
            {
                slot.reset(updateGeneration());
                // reuse the slot in the next insertion
                m_freeIndex = index;
            }
        }
    }
}

void ServiceRegistry::purge(const capro::ServiceDescription& serviceDescription) noexcept
{
    auto index = findIndex(serviceDescription);
    if (index != NO_INDEX)
    {
        auto& slot = m_slots[index];
        slot.beginWrite();
        slot.data().reset();
        slot.endWrite(updateGeneration());
        // reuse the slot in the next insertion
        m_freeIndex = index;
    }
}

void ServiceRegistry::find(ServiceDescriptionVector_t& searchResult,
                           const cxx::optional<capro::IdString_t>& service,
                           const cxx::optional<capro::IdString_t>& instance,
                           const cxx::optional<capro::IdString_t>& event) const noexcept
{
    auto function = [&](const ServiceDescriptionEntry& slot) { searchResult.emplace_back(slot); };
    find(service, instance, event, function);
}

void ServiceRegistry::find(const cxx::optional<capro::IdString_t>& service,
                           const cxx::optional<capro::IdString_t>& instance,
                           const cxx::optional<capro::IdString_t>& event,
                           cxx::function_ref<void(const ServiceDescriptionEntry&)> callable) const noexcept
{
    if (!callable)
    {
        return;
    }

    for (auto& slot : m_slots)
    {
        if (slot)
        {
            bool match = (service) ? (slot->serviceDescription.getServiceIDString() == *service) : true;
            match &= (instance) ? (slot->serviceDescription.getInstanceIDString() == *instance) : true;
            match &= (event) ? (slot->serviceDescription.getEventIDString() == *event) : true;

            if (match)
            {
                callable(*slot);
            }
        }
    }
}

const ServiceRegistry::ServiceDescriptionVector_t ServiceRegistry::getServices() const noexcept
{
    ServiceDescriptionVector_t allEntries;
    auto function = [&](const ServiceDescriptionEntry& slot) { allEntries.emplace_back(slot); };
    applyToAll(function);
    return allEntries;
}

uint32_t ServiceRegistry::findIndex(const capro::ServiceDescription& serviceDescription) const noexcept
{
    for (uint32_t i = 0; i < m_slots.size(); ++i)
    {
        auto& slot = m_slots[i];
        if (slot && slot->serviceDescription == serviceDescription)
        {
            return i;
        }
    }
    return NO_INDEX;
}

void ServiceRegistry::applyToAll(cxx::function_ref<void(const ServiceDescriptionEntry&)> callable) const noexcept
{
    if (!callable)
    {
        return;
    }

    for (auto& slot : m_slots)
    {
        if (slot)
        {
            callable(*slot);
        }
    }
}

} // namespace roudi
} // namespace iox
