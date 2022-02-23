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

//! [iceoryx includes]
#include "request_and_response_types.hpp"

#include "iceoryx_hoofs/cxx/optional.hpp"
#include "iceoryx_hoofs/internal/concurrent/smart_lock.hpp"
#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/popo/client.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
//! [iceoryx includes]

#include <iostream>

iox::cxx::optional<iox::popo::WaitSet<>> waitset;

constexpr char APP_NAME[] = "iox-cpp-request-response-client-waitset";


struct ContextData
{
    uint64_t fibonacciLast = 0;
    uint64_t fibonacciCurrent = 1;
    int64_t requestSequenceId = 0;
    int64_t expectedResponseSequenceId = requestSequenceId;
};

iox::concurrent::smart_lock<ContextData> ctx;

int main()
{
    //! [initialize runtime]
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [create waitset]
    waitset.emplace();

    //! [create client]
    iox::popo::Client<AddRequest, AddResponse> client({"Example", "Request-Response", "Add"});
    //! [create client]

    // attach client to waitset
    waitset->attachState(client, iox::popo::ClientState::HAS_RESPONSE).or_else([](auto) {
        std::cerr << "failed to attach client" << std::endl;
        std::exit(EXIT_FAILURE);
    });
    //! [create waitset]

    //! [mainloop]
    while (!iox::posix::hasTerminationRequested())
    {
        //! [send request]
        client.loan()
            .and_then([&](auto& request) {
                auto guardedCtx = ctx.getScopeGuard();
                request.getRequestHeader().setSequenceId(guardedCtx->requestSequenceId);
                guardedCtx->expectedResponseSequenceId = guardedCtx->requestSequenceId;
                guardedCtx->requestSequenceId += 1;
                request->augend = guardedCtx->fibonacciLast;
                request->addend = guardedCtx->fibonacciCurrent;
                std::cout << APP_NAME << " Send Request: " << guardedCtx->fibonacciLast << " + "
                          << guardedCtx->fibonacciCurrent << std::endl;
                request.send();
            })
            .or_else([](auto& error) {
                std::cout << "Could not allocate Request! Return value = " << static_cast<uint64_t>(error) << std::endl;
            });
        //! [send request]


        // We block and wait for samples to arrive.
        auto notificationVector = waitset->wait();

        for (auto& notification : notificationVector)
        {
            if (notification->doesOriginateFrom(&client))
            {
                auto guardedCtx = ctx.getScopeGuard();
                //! [take response]
                while (client.take().and_then([&](const auto& response) {
                    auto receivedSequenceId = response.getResponseHeader().getSequenceId();
                    if (receivedSequenceId == guardedCtx->expectedResponseSequenceId)
                    {
                        guardedCtx->fibonacciLast = guardedCtx->fibonacciCurrent;
                        guardedCtx->fibonacciCurrent = response->sum;
                        std::cout << APP_NAME << " Got Response : " << guardedCtx->fibonacciCurrent << std::endl;
                    }
                    else
                    {
                        std::cout << "Got Response with outdated sequence ID! Expected = "
                                  << guardedCtx->expectedResponseSequenceId << "; Actual = " << receivedSequenceId
                                  << "! -> skip" << std::endl;
                    }
                }))
                {
                }
                //! [take response]
            }
        }
        constexpr std::chrono::milliseconds SLEEP_TIME{950U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    //! [mainloop]

    std::cout << "shutting down" << std::endl;

    waitset.reset();
    return (EXIT_SUCCESS);
}
