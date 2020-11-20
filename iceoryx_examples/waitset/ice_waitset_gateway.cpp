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

#include "iceoryx_posh/popo/modern_api/untyped_subscriber.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "topic_data.hpp"

#include <chrono>
#include <csignal>
#include <iostream>

iox::popo::UserTrigger shutdownGuard;
using Subscriber = iox::popo::UntypedSubscriber;

static void sigHandler(int f_sig [[gnu::unused]])
{
    shutdownGuard.trigger();
}

void subscriberCallback(iox::popo::UntypedSubscriber* const subscriber)
{
    subscriber->take().and_then([&](iox::popo::Sample<const void>& sample) {
        std::cout << "subscriber: " << std::hex << subscriber << " length: " << std::dec
                  << sample.getHeader()->m_info.m_payloadSize << " ptr: " << std::hex << sample.getHeader()->payload()
                  << std::endl;
    });
}

void receiving()
{
    iox::runtime::PoshRuntime::getInstance("/iox-ex-subscriber-waitset");
    iox::popo::WaitSet waitset;

    iox::cxx::vector<iox::popo::UntypedSubscriber, 4> subscriberVector;

    for (auto i = 0; i < 2; ++i)
    {
        subscriberVector.emplace_back(iox::capro::ServiceDescription{"Radar", "FrontLeft", "Counter"});
        auto& subscriber = subscriberVector.back();

        subscriber.subscribe();
        subscriber.attachToWaitset(waitset, iox::popo::SubscriberEvent::HAS_NEW_SAMPLES, 1, subscriberCallback);
    }


    shutdownGuard.attachToWaitset(waitset);

    while (true)
    {
        auto triggerVector = waitset.wait();

        for (auto& trigger : triggerVector)
        {
            if (trigger.doesOriginateFrom(&shutdownGuard))
            {
                return;
            }
            else
            {
                trigger();
            }
        }

        std::cout << std::endl;
    }
}

int main()
{
    signal(SIGINT, sigHandler);

    std::thread rx(receiving);
    rx.join();

    return (EXIT_SUCCESS);
}