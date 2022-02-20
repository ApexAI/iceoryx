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
#include "iceoryx_hoofs/cxx/function_ref.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"
#include "iceoryx_posh/capro/service_description.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"


#include <cstdint>
#include <utility>

namespace iox
{
namespace roudi
{
// TODO: internal class and definitions, not to be tested directly
using generation_t = uint64_t;
template <typename T>
class Slot
{
    // TODO: cannot assert that with ServiceDescription (as there is a copy ctor but it is memcpyable anyway...)
    // static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

  public:
    Slot() = default;

    Slot(const Slot& other)
    {
        // shaky with atomics and general T but ok as we use it
        void* p = this;
        std::memcpy(p, &other, sizeof(Slot));
    }

    Slot& operator=(const Slot& rhs)
    {
        if (this != &rhs)
        {
            // shaky with atomics and general T but ok as we use it
            void* p = this;
            std::memcpy(p, &rhs, sizeof(Slot));
        }
        return *this;
    }

    void beginWrite()
    {
        m_value.store(UPDATING, std::memory_order_relaxed);
        // fence is needed to prevent the store to be ordered after the actual update,
        // atomic release store alone cannot achieve this
        std::atomic_thread_fence(std::memory_order_release);
    }

    // we want to set the counter to something we control externally
    // but the least significant bit will always be 1 afterwards
    void endWrite(generation_t generation)
    {
        auto value = (generation << 1) + 1;
        m_value.store(value, std::memory_order_release);
    }

    void updateGeneration(generation_t generation)
    {
        m_value.store(generation << 1, std::memory_order_relaxed);
    }

    void reset(generation_t generation)
    {
        beginWrite();
        m_data.reset();
        endWrite(generation);
    }

    void write(const T& data, generation_t generation)
    {
        beginWrite();
        m_data.emplace(data);
        endWrite(generation);
    }

    // can modify with these functions
    cxx::optional<T>& data()
    {
        return m_data;
    }

    T& operator*()
    {
        return *m_data;
    }

    const T& operator*() const
    {
        return *m_data;
    }

    T* operator->()
    {
        return m_data.has_value() ? &(*m_data) : nullptr;
    }

    const T* operator->() const
    {
        return m_data.has_value() ? &(*m_data) : nullptr;
    }

    bool read(T& buffer, generation_t& generation) const
    {
        do
        {
            auto oldValue = value();
            if (m_data)
            {
                auto src = &(*m_data);
                void* dst = &buffer;
                std::memcpy(dst, src, sizeof(T));
            }
            else
            {
                return false;
            }

            if (oldValue == value())
            {
                generation = oldValue >> 1;
                return true;
            }
        } while (true);
        return false;
    }

    // TODO
    bool read(Slot& slot) const
    {
        return false;
    }

    auto value() const
    {
        return m_value.load(std::memory_order_acquire);
    }

    auto generation() const
    {
        return m_value.load(std::memory_order_relaxed) >> 1;
    }

    bool isUpdating() const
    {
        return m_value.load(std::memory_order_relaxed) == UPDATING;
    }

    operator bool() const
    {
        return m_data.has_value();
    }

  private:
    // most importantly the least significant bit is 0
    static constexpr generation_t UPDATING = 0U;

    std::atomic<generation_t> m_value{1U};
    cxx::optional<T> m_data;
};

class ServiceRegistry
{
  public:
    enum class Error
    {
        SERVICE_REGISTRY_FULL,
    };

    using ReferenceCounter_t = uint64_t;

    struct ServiceDescriptionEntry
    {
        ServiceDescriptionEntry(const capro::ServiceDescription& serviceDescription);

        capro::ServiceDescription serviceDescription;

        // note that we can have publishers and servers with the same ServiceDescription
        // and using the counters we save space
        ReferenceCounter_t publisherCount{0U};
        ReferenceCounter_t serverCount{0U};
    };

    /// @todo #415 #1074 set limits properly and define location for the limits,
    ///       e.g posh_types.hpp
    static constexpr uint32_t MAX_SERVICE_DESCRIPTIONS = iox::MAX_PUBLISHERS;

    using ServiceDescriptionVector_t = cxx::vector<ServiceDescriptionEntry, MAX_SERVICE_DESCRIPTIONS>;

    ServiceRegistry() = default;

    // TODO: forbid later when we stop copying the registry
    ServiceRegistry(const ServiceRegistry& other)
    {
        void* p = this;
        std::memcpy(p, &other, sizeof(ServiceRegistry));
    }

    ServiceRegistry& operator=(const ServiceRegistry& rhs)
    {
        if (&rhs != this)
        {
            void* p = this;
            std::memcpy(p, &rhs, sizeof(ServiceRegistry));
        }
        return *this;
    }

    /// @brief Adds a given publisher service description to registry
    /// @param[in] serviceDescription, service to be added
    /// @return ServiceRegistryError, error wrapped in cxx::expected
    cxx::expected<Error> addPublisher(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes a given publisher service description from registry if service is found,
    ///        in case of multiple occurrences only one occurrence is removed
    /// @param[in] serviceDescription, service to be removed
    void removePublisher(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Adds a given server service description to registry
    /// @param[in] serviceDescription, service to be added
    /// @return ServiceRegistryError, error wrapped in cxx::expected
    cxx::expected<Error> addServer(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes a given server service description from registry if service is found,
    ///        in case of multiple occurrences only one occurrence is removed
    /// @param[in] serviceDescription, service to be removed
    void removeServer(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Removes given service description from registry if service is found,
    ///        all occurences are removed
    /// @param[in] serviceDescription, service to be removed
    void purge(const capro::ServiceDescription& serviceDescription) noexcept;

    /// @brief Searches for given service description in registry
    /// @param[in] searchResult, reference to the vector which will be filled with the results
    /// @param[in] service, string or wildcard (= iox::cxx::nullopt) to search for
    /// @param[in] instance, string or wildcard (= iox::cxx::nullopt) to search for
    /// @param[in] event, string or wildcard (= iox::cxx::nullopt) to search for
    void find(ServiceDescriptionVector_t& searchResult,
              const cxx::optional<capro::IdString_t>& service,
              const cxx::optional<capro::IdString_t>& instance,
              const cxx::optional<capro::IdString_t>& event) const noexcept;

    /// @copydoc ServiceDiscovery::findService
    void find(const cxx::optional<capro::IdString_t>& service,
              const cxx::optional<capro::IdString_t>& instance,
              const cxx::optional<capro::IdString_t>& event,
              cxx::function_ref<void(const ServiceDescriptionEntry&)> callable) const noexcept;

    /// @todo #415 this may not be needed later or we can move applyToAll to the public interface,
    ///       (we want to avoid large containers on the stack)
    /// @brief Returns all service descriptions as copy
    /// @return ServiceDescriptionVector_t, copy of complete service registry
    const ServiceDescriptionVector_t getServices() const noexcept;

    generation_t generation() const
    {
        // do not use the lowest bit
        // TODO: can be made more efficient with slots without shifts, leave it for clarity now
        return m_generation.load() >> 1;
    }

    generation_t updateGeneration()
    {
        // new generation (do not use the lowest bit)
        // do not really need fetch add as we have no concurrent updates
        auto generation = m_generation.fetch_add(2U, std::memory_order_relaxed) + 2U;
        if (generation == 0U)
        {
            // overflow (very rare), update generation of all used slots
            for (auto& slot : m_slots)
            {
                if (slot)
                {
                    slot.updateGeneration(0U);
                }
            }
        }
        return (generation >> 1);
    }

    // semantically not the same as a copy in general, as it can be used concurrently
    // TODO: fix logic error
    void updateFrom(const ServiceRegistry& source)
    {
        // we assume we are outdated compared to source
        auto sourceGen = source.generation();
        auto thisGen = generation();

        if (sourceGen == thisGen)
        {
            // we are up to date, nothing to do
            return;
        }

        // TODO: simplify update logic
        auto& sourceSlots = source.m_slots;
        auto sourceSize = source.m_slots.size();
        if (sourceSize > m_slots.size())
        {
            m_slots.resize(sourceSlots.size());
        }

        if (thisGen > sourceGen)
        {
            // generation overflow, need to update all
            for (uint32_t i = 0; i < sourceSize; ++i)
            {
                auto& slot = source.m_slots[i];
                auto& dstSlot = m_slots[i];
                auto& buffer = *m_slots[i].data();
                auto g = slot.generation();

                slot.read(buffer, g);
                if (slot.read(buffer, g))
                {
                    dstSlot.updateGeneration(g);
                }
                else
                {
                    dstSlot.reset(g);
                }
            }
            m_generation.store(sourceGen);
            return;
        }

        // thisGen < sourceGen, only update the slots we need to
        // (unfortunately we still need to iterate but this can be improved if we send the changed slot
        // later as well, but we need to incorporate the generation in checks then to ensure we did not lose
        // updates)


        for (uint32_t i = 0; i < sourceSlots.size(); ++i)
        {
            auto& slot = source.m_slots[i];
            auto g = slot.generation();
            if (g > thisGen)
            {
                auto& dstSlot = m_slots[i];
                // TODO: use slot reading abstraction
                if (slot)
                {
                    auto& buffer = *dstSlot.data();
                    if (slot.read(buffer, g))
                    {
                        dstSlot.updateGeneration(g);
                    }
                    else
                    {
                        dstSlot.reset(g);
                    }
                }
                else
                {
                    dstSlot.reset(g);
                }
            }
        }
    }

  private:
    // using Entry_t = cxx::optional<ServiceDescriptionEntry>;
    using Slot_t = Slot<ServiceDescriptionEntry>;
    using SlotContainer_t = cxx::vector<Slot_t, MAX_SERVICE_DESCRIPTIONS>;

    static constexpr uint32_t NO_INDEX = MAX_SERVICE_DESCRIPTIONS;

    std::atomic<generation_t> m_generation{0U};

    SlotContainer_t m_slots;

    // store the last known free Index (if any is known)
    // we could use a queue (or stack) here since they are not optimal
    // for the filling pattern of a vector (prefer entries close to the front)
    uint32_t m_freeIndex{NO_INDEX};

  private:
    uint32_t findIndex(const capro::ServiceDescription& serviceDescription) const noexcept;

    void getAll(ServiceDescriptionVector_t& searchResult) const noexcept;

    void applyToAll(cxx::function_ref<void(const ServiceDescriptionEntry&)> callable) const noexcept;

    cxx::expected<Error> add(const capro::ServiceDescription& serviceDescription,
                             ReferenceCounter_t ServiceDescriptionEntry::*count);
};
} // namespace roudi
} // namespace iox

#endif // IOX_POSH_ROUDI_SERVICE_REGISTRY_HPP
