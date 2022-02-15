// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
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

#include "discovery_autosar.hpp"
#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include <iostream>

constexpr char APP_NAME[] = "autosar-find-service";

using namespace discovery;

iox::capro::IdString_t service{"Radar"};
iox::capro::IdString_t instance{"FrontLeft"};
iox::capro::IdString_t event{"Counter"};

int main()
{
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);

    auto condition = [&](iox::runtime::ServiceDiscovery& discovery) -> bool {
        auto result = discovery.findService(service, instance, event);

        if (result.size() > 0)
        {
            std::cout << "Condition: <Radar Frontleft Counter> available" << std::endl;
            return true;
        }

        std::cout << "Condition: <Radar Frontleft Counter> unavailable" << std::endl;
        return false;
    };

    auto handler = [&](iox::runtime::ServiceDiscovery& discovery) -> void {
        (void)discovery;
        std::cout << "Availabilty of <Radar Frontleft Counter> changed" << std::endl;
    };

    // we can monitor two conditions
    AutosarDiscovery<2> autosarDiscovery;

    // try to register three search conditions with actions
    auto handle1 = autosarDiscovery.startMonitoring(condition, handler);
    auto handle2 = autosarDiscovery.startFindService(service, instance, handler);
    auto handle3 = autosarDiscovery.startMonitoring(condition, handler);

    if (!handle1)
    {
        std::cout << "could not register search condition monitoring" << std::endl;
    }

    if (!handle2)
    {
        std::cout << "could not register service/instance montoring" << std::endl;
    }

    if (!handle3)
    {
        std::cout << "could not register search condition monitoring a second time" << std::endl;
    }
    else
    {
        autosarDiscovery.stopMonitoring(handle3);
    }

    while (!iox::posix::hasTerminationRequested())
        ;

    autosarDiscovery.stopMonitoring(handle1);
    // autosarDiscovery.stopMonitoring(handle2);
    autosarDiscovery.stopFindService(handle2);

    return (EXIT_SUCCESS);
}
