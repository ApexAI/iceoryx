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

#include "iceoryx_hoofs/cxx/prefix_tree.hpp"
#include "iceoryx_hoofs/memory/typed_allocator.hpp"


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

    using OccurrenceCounter_t = uint64_t;
    struct ServiceDescriptionEntry
    {
        ServiceDescriptionEntry(const capro::ServiceDescription& desc, OccurrenceCounter_t count)
            : serviceDescription(desc)
            , count(count)
        {
        }
        capro::ServiceDescription serviceDescription{};
        OccurrenceCounter_t count = 0U;
    };

    /// @todo #415 should be connected with iox::MAX_NUMBER_OF_SERVICES
    static constexpr uint32_t MAX_SERVICE_DESCRIPTIONS = 5000U;


    using ServiceDescriptionVector_t = cxx::vector<ServiceDescriptionEntry, MAX_SERVICE_DESCRIPTIONS>;

    using Entry_t = cxx::optional<ServiceDescriptionEntry>;
    using ServiceDescriptionContainer_t = cxx::vector<Entry_t, MAX_SERVICE_DESCRIPTIONS>;

    /// @brief Adds given service description to registry
    /// @param[in] serviceDescription, service to be added
    /// @return ServiceRegistryError, error wrapped in cxx::expected
    cxx::expected<Error> add(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes given service description from registry if service is found
    /// @param[in] serviceDescription, service to be removed
    void remove(const capro::ServiceDescription& serviceDescription) noexcept;

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

  private:
    using string_t = capro::IdString_t;
    static constexpr uint32_t MAX_ID_STRING_LENGTH = string_t::capacity();
    using entry_t = ServiceDescriptionEntry;
    using handle_t = entry_t*;
    using handle_container_t = iox::cxx::vector<handle_t, MAX_SERVICE_DESCRIPTIONS>;
    using search_tree_t = iox::cxx::PrefixTree<handle_t, MAX_SERVICE_DESCRIPTIONS, MAX_ID_STRING_LENGTH>;
    using entry_storage_t = iox::memory::TypedAllocator<entry_t, MAX_SERVICE_DESCRIPTIONS>;

    search_tree_t m_serviceTree;
    search_tree_t m_instanceTree;
    search_tree_t m_eventTree;

    entry_storage_t m_entries;

    struct Wildcard
    {
    };

    static void sort(handle_container_t& a)
    {
        std::sort(a.begin(), a.end());
    }

    static auto intersect(handle_container_t& a, handle_container_t& b)
    {
        handle_container_t result;
        std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));

        return result;
    }

    static auto sortAndIntersect(handle_container_t& a, handle_container_t& b)
    {
        sort(a);
        sort(b);
        return intersect(a, b);
    }

    auto findServices(const capro::IdString_t& service) const
    {
        handle_container_t result;
        auto ptrs = m_serviceTree.find(service);
        for (auto p : ptrs)
        {
            result.push_back(*p);
        }
        return result;
    }

    auto findServices(const capro::ServiceDescription& sd) const
    {
        return findServices(sd.getServiceIDString());
    }

    auto findInstances(const capro::IdString_t& instance) const
    {
        handle_container_t result;
        auto ptrs = m_instanceTree.find(instance);
        for (auto p : ptrs)
        {
            result.push_back(*p);
        }
        return result;
    }

    auto findInstances(const capro::ServiceDescription& sd) const
    {
        return findInstances(sd.getInstanceIDString());
    }

    auto findEvents(const capro::IdString_t& event) const
    {
        handle_container_t result;
        auto ptrs = m_eventTree.find(event);
        for (auto p : ptrs)
        {
            result.push_back(*p);
        }
        return result;
    }

    auto findEvents(const capro::ServiceDescription& sd) const
    {
        return findEvents(sd.getEventIDString());
    }

    bool addNewEntry(const capro::ServiceDescription& sd)
    {
        auto entry = m_entries.allocate();
        if (!entry)
        {
            return false;
        }
        new (entry) entry_t{sd, 1};

        // cannot fail here if the capacities are not messed up (todo: check?)

        auto& service = sd.getServiceIDString();
        m_serviceTree.insert(service, entry);

        auto& instance = sd.getInstanceIDString();
        m_instanceTree.insert(instance, entry);

        auto& event = sd.getEventIDString();
        m_eventTree.insert(event, entry);

        // std::cerr << "SERVICES " << m_serviceTree.size() << " INSTANCES " << m_instanceTree.size() << " EVENTS "
        //           << m_eventTree.size() << std::endl;

        return true;
    }

    void removeEntry(entry_t* entry)
    {
        auto& service = entry->serviceDescription.getServiceIDString();
        m_serviceTree.remove(service, entry);

        auto& instance = entry->serviceDescription.getInstanceIDString();
        m_instanceTree.remove(instance, entry);

        auto& event = entry->serviceDescription.getEventIDString();
        m_eventTree.remove(event, entry);

        m_entries.destroy(entry);
    }

    void fillResult(handle_container_t& handles, ServiceDescriptionVector_t& searchResult) const
    {
        for (auto h : handles)
        {
            // todo: handle failure
            searchResult.push_back(*h);
        }
    }

    // heuristics to reduce the sorting and intersection effort:
    // intersect in ascending order of intermediate result sizes
    static handle_t intersection(handle_container_t& a, handle_container_t& b, handle_container_t& c)
    {
        cxx::vector<handle_container_t*, 3> matches;
        matches.push_back(&a);
        matches.push_back(&b);
        matches.push_back(&c);
        std::sort(matches.begin(), matches.end(), [](auto x, auto y) { return x->size() < y->size(); });
        *matches[0] = sortAndIntersect(*matches[0], *matches[1]);
        sort(*matches[2]);
        *matches[0] = intersect(*matches[0], *matches[2]);
        auto& result = *matches[0];

        // size is either 0 or 1
        if (result.size() > 0)
        {
            return result[0];
        }
        return nullptr;
    }
#if 0

// better heuristics (intersection order)
    handle_t
    findExact(const capro::IdString_t& service, const capro::IdString_t& instance, const capro::IdString_t& event) const
    {
        auto a = findServices(service);
        auto b = findInstances(instance);
        auto c = findEvents(event);
        return intersection(a, b, c);
    }
#else
    // even better heuristics, on delayed intersection and brute force if intermediate sets are small
    handle_t
    findExact(const capro::IdString_t& service, const capro::IdString_t& instance, const capro::IdString_t& event) const
    {
        constexpr uint32_t SMALL = 100;
        constexpr uint32_t M = 2;

        auto bruteForce = [&](handle_container_t& handles) -> handle_t {
            auto match = linearSearch(service, instance, event, handles);
            if (match)
            {
                return match;
            }
            return nullptr;
        };

        auto sameOrder = [&](uint64_t x, uint64_t y) {
            if (x < y)
            {
                if (x >= y / M)
                    return true;
                return false;
            }

            if (x <= M * y)
                return true;
            return false;
        };

        auto a = findEvents(event);
        auto sa = a.size();
        if (sa <= SMALL)
        {
            return bruteForce(a);
        }

        auto b = findInstances(instance);
        auto sb = b.size();
        if (sb <= SMALL)
        {
            return bruteForce(b);
        }

        if (sameOrder(sa, sb))
        {
            // postpone intersection, check services first
            auto c = findServices(service);
            auto sc = c.size();
            if (sc <= SMALL)
            {
                return bruteForce(c);
            }

            // potential optimization: intersect smallest first
            a = sortAndIntersect(a, b);
            sa = a.size();
            if (sa <= SMALL)
            {
                return bruteForce(a);
            }

            sort(c);
            a = intersect(a, c);
            return bruteForce(a);
        }


        // otherwise we intersect first

        a = sortAndIntersect(a, b);
        sa = a.size();
        if (sa <= SMALL)
        {
            return bruteForce(a);
        }

        // if the intersection is still large we check the services

        b = findServices(service);
        sb = b.size();
        if (sb <= SMALL)
        {
            return bruteForce(b);
        }

        if (sameOrder(sa, sb))
        {
            sort(b);
            a = intersect(a, b);

            // can be only 0 or 1 (exhaustive intersection by that point and
            // if we check for all three strings there is at most one match)
            if (a.size() > 0)
            {
                return a[0];
            }
        }

        // different order
        // a is sorted but b is not

        if (sb > M * sa)
        {
            // prefer brute force over sorting b
            return bruteForce(a);
        }

        // fallback to intersection
        sort(b);
        a = intersect(a, b);
        if (a.size() > 0)
        {
            return a[0];
        }
        return nullptr;
    }

#endif

    handle_t linearSearch(const capro::IdString_t& service,
                          const capro::IdString_t& instance,
                          const capro::IdString_t& event,
                          const handle_container_t& handles) const
    {
        for (auto& handle : handles)
        {
            auto& d = handle->serviceDescription;
            // avoid constructing a service description
            if (d.getServiceIDString() == service && d.getInstanceIDString() == instance
                && d.getEventIDString() == event)
            {
                return handle;
            }
        }
        return nullptr;
    }

    handle_t findExact(const capro::ServiceDescription& sd) const
    {
        return findExact(sd.getServiceIDString(), sd.getInstanceIDString(), sd.getEventIDString());
    }

    void find_(const capro::IdString_t& service, Wildcard, Wildcard, ServiceDescriptionVector_t& searchResult) const
    {
        auto matches = findServices(service);
        fillResult(matches, searchResult);
    }

    void find_(Wildcard, const capro::IdString_t& instance, Wildcard, ServiceDescriptionVector_t& searchResult) const
    {
        auto matches = findInstances(instance);
        fillResult(matches, searchResult);
    }

    void find_(Wildcard, Wildcard, const capro::IdString_t& event, ServiceDescriptionVector_t& searchResult) const
    {
        auto matches = findEvents(event);
        fillResult(matches, searchResult);
    }

    void find_(const capro::IdString_t& service,
               const capro::IdString_t& instance,
               Wildcard,
               ServiceDescriptionVector_t& searchResult) const
    {
        auto matches = findServices(service);
        auto instances = findInstances(instance);
        matches = sortAndIntersect(matches, instances);
        fillResult(matches, searchResult);
    }

    void find_(const capro::IdString_t& service,
               Wildcard,
               const capro::IdString_t& event,
               ServiceDescriptionVector_t& searchResult) const
    {
        auto matches = findServices(service);
        auto events = findEvents(event);
        matches = sortAndIntersect(matches, events);
        fillResult(matches, searchResult);
    }

    void find_(Wildcard,
               const capro::IdString_t& instance,
               const capro::IdString_t& event,
               ServiceDescriptionVector_t& searchResult) const
    {
        auto matches = findInstances(instance);
        auto events = findEvents(event);
        matches = sortAndIntersect(matches, events);
        fillResult(matches, searchResult);
    }

    void find_(const capro::IdString_t& service,
               const capro::IdString_t& instance,
               const capro::IdString_t& event,
               ServiceDescriptionVector_t& searchResult) const
    {
        auto match = findExact(service, instance, event);
        if (match)
        {
            searchResult.push_back(*match);
        }
    }

    void findAll(ServiceDescriptionVector_t& searchResult) const
    {
        //(void)searchResult;
        auto matches = m_serviceTree.values();
        for (auto& match : matches)
        {
            searchResult.emplace_back(**match);
        }
    }
};
} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP
