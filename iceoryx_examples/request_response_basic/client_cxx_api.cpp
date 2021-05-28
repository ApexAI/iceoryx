// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_hoofs/posix_wrapper/signal_handler.hpp"
#include "iceoryx_posh/popo/client.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
//! [iceoryx includes]

#include <atomic>
#include <iostream>

//! [signal handling]
std::atomic<bool> keepRunning{true};

static void sigHandler(int sig IOX_MAYBE_UNUSED)
{
    // caught SIGINT or SIGTERM, now exit gracefully
    keepRunning = false;
}
//! [signal handling]

int main()
{
    //! [register sigHandler]
    auto signalIntGuard = iox::posix::registerSignalHandler(iox::posix::Signal::INT, sigHandler);
    auto signalTermGuard = iox::posix::registerSignalHandler(iox::posix::Signal::TERM, sigHandler);
    //! [register sigHandler]

    //! [initialize runtime]
    constexpr char APP_NAME[] = "iox-cpp-request-response-client";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [create client]
    iox::popo::Client<AddRequest, AddResponse> client({"Example", "Request-Response", "Add"});
    //! [create client]

    //! [send requests in a loop]
    uint64_t fibonacciLast = 0;
    uint64_t fibonacciCurrent = 1;
    while (keepRunning)
    {
        //! [send request]
        client.allocateRequest()
            .and_then([&](auto& requestHeader) {
                auto request = static_cast<AddRequest*>(requestHeader->getUserPayload());
                request->augend = fibonacciLast;
                request->addend = fibonacciCurrent;
                std::cout << "Send Request: " << fibonacciLast << " + " << fibonacciCurrent << std::endl;
                client.sendRequest(requestHeader);
            })
            .or_else([](auto& error) {
                std::cout << "Could not allocate Request! Return value = " << static_cast<uint64_t>(error) << std::endl;
            });
        //! [send request]

        constexpr std::chrono::milliseconds DELAY_TIME{50U};
        std::this_thread::sleep_for(DELAY_TIME);

        //! [take response]
        client.getResponse().and_then([&](auto& responseHeader) {
            auto response = static_cast<const AddResponse*>(responseHeader->getUserPayload());
            fibonacciLast = fibonacciCurrent;
            fibonacciCurrent = response->sum;
            client.releaseResponse(responseHeader);
            std::cout << "Got Response : " << fibonacciCurrent << std::endl;
        });
        //! [take response]

        constexpr std::chrono::milliseconds SLEEP_TIME{950U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    //! [send requests in a loop]

    return EXIT_SUCCESS;
}
