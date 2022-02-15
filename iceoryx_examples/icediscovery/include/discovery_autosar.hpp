#pragma once

#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/service_discovery.hpp"

#include "iceoryx_hoofs/cxx/function.hpp"
#include "iceoryx_hoofs/cxx/optional.hpp"

namespace discovery
{
using ServiceDiscovery = iox::runtime::ServiceDiscovery;

ServiceDiscovery& serviceDiscovery()
{
    static ServiceDiscovery instance;
    return instance;
}

// @note cannot have exacty Autosar semantics without condition mutex
// to make sure it cannot change back and forth unnoticed
// alternatively we need a semaphore for each registered condition


template <uint32_t MaxCallbacks = 16>
class AutosarDiscovery
{
    // since the maximum number of callbacks is also limited by the listener
    static_assert(MaxCallbacks < 128U, "Maximum number of callbacks exceeded");

    using handle_t = uint32_t;
    static constexpr handle_t NO_HANDLE = MaxCallbacks;

  public:
    using string_t = iox::capro::IdString_t;
    class Handle
    {
      public:
        operator bool()
        {
            return m_value != NO_HANDLE;
        }

      private:
        friend class AutosarDiscovery;
        handle_t m_value{NO_HANDLE};

        // only the AutosarDiscovery is supposed to create a Handle
        // and pass them to the user
        Handle(handle_t value = NO_HANDLE)
            : m_value(value)
        {
        }
    };

    AutosarDiscovery()
        : m_discovery(&serviceDiscovery())
    {
    }

    // not thread-safe with other calls
    template <typename Condition, typename Action>
    Handle startMonitoring(const Condition& condition, const Action& action)
    {
        Handle handle(getFreeIndex());
        if (!handle)
        {
            // failed - no space
            return handle;
        }

        auto& callback = m_callbacks[handle.m_value];
        callback.emplace(condition, action, *m_discovery);

        auto errorHandler = [&](auto) {
            // listener registration failed
            callback.reset();
            handle = Handle();
            --m_numCallbacks;
        };

        ++m_numCallbacks;

        if (m_numCallbacks == 1)
        {
            auto invoker = iox::popo::createNotificationCallback(invokeCallback, *this);
            m_listener.attachEvent(*m_discovery, iox::runtime::ServiceDiscoveryEvent::SERVICE_REGISTRY_CHANGED, invoker)
                .or_else(errorHandler);
        }

        return handle;
    }

    void stopMonitoring(Handle handle)
    {
        if (!handle)
        {
            return;
        }

        auto& callback = m_callbacks[handle.m_value];

        if (callback)
        {
            callback.reset();
            --m_numCallbacks;

            if (m_numCallbacks == 0)
            {
                m_listener.detachEvent(*m_discovery, iox::runtime::ServiceDiscoveryEvent::SERVICE_REGISTRY_CHANGED);
            }
        }
    }

    // special case for Autosar, instead of the container we use the whole Registry though but this could be changed
    // (do we want a past snapshot or the current state?)
    template <typename Handler>
    Handle startFindService(const string_t& service, const string_t& instance, const Handler& handler)
    {
        // we need to capture the search instance
        auto condition = [=](ServiceDiscovery& discovery) {
            auto result = discovery.findService(service, instance, iox::cxx::nullopt);
            return result.size() > 0;
        };

        // the handler operates on the current registry and not the search result
        return startMonitoring(condition, handler);
    }

    void stopFindService(Handle handle)
    {
        stopMonitoring(handle);
    }

  private:
    using condition_t = iox::cxx::function<bool(ServiceDiscovery&), 512U>;
    using action_t = iox::cxx::function<void(ServiceDiscovery&)>;
    class Callback
    {
      public:
        template <typename Condition, typename Action>
        Callback(Condition condition, Action action, ServiceDiscovery& discovery)
            : m_condition(condition)
            , m_action(action)
        {
            m_value = m_condition(discovery);
        }

        void operator()(ServiceDiscovery& discovery)
        {
            bool value = m_condition(discovery);
            if (value != m_value.value())
            {
                // we may miss a change though (!)
                // (back and forth without seeing the update since it is already replaced
                // by the most recent in the ServiceDiscovery)
                m_action(discovery);
            }

            m_value = value;
        }

      private:
        iox::cxx::optional<bool> m_value;
        condition_t m_condition;
        action_t m_action;
    };

    using callback_t = iox::cxx::optional<Callback>;
    ServiceDiscovery* m_discovery{nullptr};

    // no reason to use a pointer here, the same listener can attach to the registry
    // only once (per event, and we only have the change event)
    iox::popo::Listener m_listener;

    callback_t m_callbacks[MaxCallbacks];
    uint32_t m_numCallbacks{0};

    static void invokeCallback(ServiceDiscovery* discovery, AutosarDiscovery* self)
    {
        for (auto& callback : self->m_callbacks)
        {
            if (callback)
            {
                (*callback)(*discovery);
            }
        }
    }

    // can be made more efficient with a queue
    //(but the IndexQueue is not in the public interface)
    handle_t getFreeIndex()
    {
        for (handle_t handle = 0; handle < MaxCallbacks; ++handle)
        {
            auto& callback = m_callbacks[handle];
            if (!callback.has_value())
            {
                return handle;
            }
        }
        return NO_HANDLE;
    }
};

} // namespace discovery
