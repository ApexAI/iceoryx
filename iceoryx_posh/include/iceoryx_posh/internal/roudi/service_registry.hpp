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
#ifndef IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP
#define IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP

#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"
#include "iceoryx_posh/capro/service_description.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <utility>

namespace iox
{
namespace roudi
{
class ServiceRegistry
{
  public:
    enum class Error
    {
        SERVICE_REGISTRY_FULL,
    };

    using ReferenceCounter_t = uint64_t;
    using generation_t = uint64_t;

    struct ServiceDescriptionEntry
    {
        ServiceDescriptionEntry(const capro::ServiceDescription& sd, ReferenceCounter_t count)
            : serviceDescription(sd)
            , count(count)
        {
        }

        capro::ServiceDescription serviceDescription{};
        ReferenceCounter_t count{0U};
    };


    /// @todo #415 should be connected with iox::MAX_NUMBER_OF_SERVICES
    static constexpr uint32_t MAX_SERVICE_DESCRIPTIONS = 5000U;

    using ServiceDescriptionVector_t = cxx::vector<ServiceDescriptionEntry, MAX_SERVICE_DESCRIPTIONS>;
    using Entry_t = cxx::optional<ServiceDescriptionEntry>;

    // dirty wrapper (do not change much other code)
    struct Entry
    {
        Entry_t entry;

        operator bool() const
        {
            return entry.has_value();
        }

        Entry_t& operator->()
        {
            return entry;
        }

        const Entry_t& operator->() const
        {
            return entry;
        }

        ServiceDescriptionEntry& operator*()
        {
            return entry.value();
        }

        const ServiceDescriptionEntry& operator*() const
        {
            return entry.value();
        }

        void reset()
        {
            entry.reset();
        }

        void update(generation_t newGeneration) const
        {
            // assume exclusive access to generation
            m_generation.store(newGeneration);
        }

        generation_t generation() const
        {
            return m_generation;
        }

      private:
        mutable std::atomic<generation_t> m_generation{0};
    };

    using index_t = uint32_t;
    using IndexContainer_t = cxx::vector<index_t, MAX_SERVICE_DESCRIPTIONS>;
    using ServiceDescriptionContainer_t = cxx::vector<Entry, MAX_SERVICE_DESCRIPTIONS>;

    // !!! assume the modification and find operations happen with exclusive access (no concurrency)

    /// @brief Adds given service description to registry
    /// @param[in] serviceDescription, service to be added
    /// @return ServiceRegistryError, error wrapped in cxx::expected
    cxx::expected<Error> add(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes given service description from registry if service is found,
    ///        in case of multiple occurrences only one occurrence is removed
    /// @param[in] serviceDescription, service to be removed
    void remove(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes given service description from registry if service is found,
    ///        all occurences are removed
    /// @param[in] serviceDescription, service to be removed
    void removeAll(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Searches for given service description in registry
    /// @param[in] searchResult, reference to the vector which will be filled with the results
    /// @param[in] service, string or wildcard (= iox::cxx::nullopt) to search for
    /// @param[in] instance, string or wildcard (= iox::cxx::nullopt) to search for
    /// @param[in] event, string or wildcard (= iox::cxx::nullopt) to search for
    void find(ServiceDescriptionVector_t& searchResult,
              const cxx::optional<capro::IdString_t>& service,
              const cxx::optional<capro::IdString_t>& instance,
              const cxx::optional<capro::IdString_t>& event) const noexcept;

    /// @brief Returns all service descriptions as copy
    /// @return ServiceDescriptionVector_t, copy of complete service registry
    const ServiceDescriptionVector_t getServices() const noexcept;

    // todo: need those variations for all cases
    generation_t findIndex(const capro::IdString_t& service,
                           const capro::IdString_t& instance,
                           const capro::IdString_t& event,
                           IndexContainer_t& searchResult) const noexcept
    {
        auto g = generation();
        for (index_t index = 0; index < m_serviceDescriptions.size(); ++index)
        {
            auto& entry = m_serviceDescriptions[index];

            if (entry && entry->serviceDescription.getServiceIDString() == service
                && entry->serviceDescription.getInstanceIDString() == instance
                && entry->serviceDescription.getEventIDString() == event)
            {
                entry.update(g);
                searchResult.emplace_back(index);
            }
        }

        return g;
    }

    void readIndices(generation_t queryGeneration,
                     const IndexContainer_t& indices,
                     ServiceDescriptionVector_t& result) const noexcept
    {
        for (auto index : indices)
        {
            // the reference is always valid
            auto& entry = m_serviceDescriptions[index];

            //!!! need a trivial copy here
            // we copy out of an optional that may be concurrently updated
            // is this ok?
            // can we perform a memcpy
            auto copy = entry.entry.value();

            // entry.generation was updated at least during query
            if (entry.generation() <= queryGeneration)
            {
                // copy is valid (ignore wraparound)
                // entry was modified later, consider it not to be in the result set
                // valid, since we can assume the query was taken at some point earlier
                // and the entry was either removed or replaced (even if replaced with the same the result is valid)
                result.emplace_back(copy);
            }
        }
    }

  private:
    static constexpr uint32_t NO_INDEX = MAX_SERVICE_DESCRIPTIONS;

    // tag type for internal overloads
    struct Wildcard
    {
    };

    ServiceDescriptionContainer_t m_serviceDescriptions;

    // store the last known free Index (if any is known)
    // we should not use a queue (or stack) here since they are not optimal
    // for the filling pattern of a vector (prefer entries close to the front)
    uint32_t m_freeIndex{NO_INDEX};

    std::atomic<generation_t> m_generation{0};

    generation_t generation() const
    {
        return m_generation;
    }

  private:
    // functions for the different search cases
    // tag types are not needed in all cases to distinguish overloads but kept for consistency

    uint32_t find(const capro::ServiceDescription& serviceDescription) const noexcept;

    void
    find(const capro::IdString_t& service, Wildcard, Wildcard, ServiceDescriptionVector_t& searchResult) const noexcept;

    void find(Wildcard,
              const capro::IdString_t& instance,
              Wildcard,
              ServiceDescriptionVector_t& searchResult) const noexcept;

    void
    find(Wildcard, Wildcard, const capro::IdString_t& event, ServiceDescriptionVector_t& searchResult) const noexcept;

    void find(const capro::IdString_t& service,
              const capro::IdString_t& instance,
              Wildcard,
              ServiceDescriptionVector_t& searchResult) const noexcept;

    void find(const capro::IdString_t& service,
              Wildcard,
              const capro::IdString_t& event,
              ServiceDescriptionVector_t& searchResult) const noexcept;

    void find(Wildcard,
              const capro::IdString_t& instance,
              const capro::IdString_t& event,
              ServiceDescriptionVector_t& searchResult) const noexcept;

    void find(const capro::IdString_t& service,
              const capro::IdString_t& instance,
              const capro::IdString_t& event,
              ServiceDescriptionVector_t& searchResult) const noexcept;

    void findAll(ServiceDescriptionVector_t& searchResult) const noexcept;

    generation_t update()
    {
        return m_generation.fetch_add(1) + 1;
    }
};
} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP
