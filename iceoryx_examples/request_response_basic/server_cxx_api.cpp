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
#include "iceoryx_posh/popo/server.hpp"
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
    constexpr char APP_NAME[] = "iox-cpp-request-response-server";
    iox::runtime::PoshRuntime::initRuntime(APP_NAME);
    //! [initialize runtime]

    //! [create server]
    iox::popo::Server<AddRequest, AddResponse> server({"Example", "Request-Response", "Add"});
    //! [create server]

    //! [process requests in a loop]
    while (keepRunning)
    {
        //! [take request]
        server.getRequest()
            .and_then([&](auto& requestHeader) {
                std::cout << "Got Request!" << std::endl;
                server.releaseRequest(requestHeader);
            })
            .or_else([](auto& error) {
                std::cout << "No Request! Return value = " << static_cast<uint64_t>(error) << std::endl;
            });
        //! [take request]

        //! [send response]
        server.allocateResponse()
            .and_then([&](auto& responseHeader) {
                std::cout << "Send Response!" << std::endl;
                server.sendResponse(responseHeader);
            })
            .or_else([](auto& error) {
                std::cout << "Could not allocate Response! Return value = " << static_cast<uint64_t>(error)
                          << std::endl;
            });
        //! [send response]

        constexpr std::chrono::milliseconds SLEEP_TIME{1000U};
        std::this_thread::sleep_for(SLEEP_TIME);
    }
    //! [process requests in a loop]

    return EXIT_SUCCESS;
}
