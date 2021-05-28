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
#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/notification_callback.hpp"
#include "iceoryx_posh/popo/server.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
//! [iceoryx includes]

#include <atomic>
#include <iostream>

//! [signal handling]
std::atomic<bool> keepRunning{true};

iox::posix::Semaphore shutdownSemaphore =
    iox::posix::Semaphore::create(iox::posix::CreateUnnamedSingleProcessSemaphore, 0U).value();

static void sigHandler(int sig IOX_MAYBE_UNUSED)
{
    shutdownSemaphore.post().or_else([](auto) {
        std::cerr << "unable to call post on shutdownSemaphore - semaphore corrupt?" << std::endl;
        std::exit(EXIT_FAILURE);
    });
    // caught SIGINT or SIGTERM, now exit gracefully
    keepRunning = false;
}
//! [signal handling]

void onRequestReceived(iox::popo::Server<AddRequest, AddResponse>* server)
{
    //! [take request]
    while (server->getRequest().and_then([&](auto& requestHeader) {
        auto request = static_cast<const AddRequest*>(requestHeader->getUserPayload());
        std::cout << "Got Request: " << request->augend << " + " << request->addend << std::endl;

        //! [send response]
        server->allocateResponse(requestHeader)
            .and_then([&](auto& responseHeader) {
                auto response = static_cast<AddResponse*>(responseHeader->getUserPayload());
                response->sum = request->augend + request->addend;
                std::cout << "Send Response: " << response->sum << std::endl;
                server->sendResponse(responseHeader);
            })
            .or_else([](auto& error) {
                std::cout << "Could not allocate Response! Return value = " << static_cast<uint64_t>(error)
                          << std::endl;
            });
        //! [send response]

        server->releaseRequest(requestHeader);
    }))
    {
    }
    //! [take request]
}

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

    iox::popo::Listener listener;

    //! [create server]
    iox::popo::Server<AddRequest, AddResponse> server({"Example", "Request-Response", "Add"});
    //! [create server]

    listener
        .attachEvent(
            server, iox::popo::ServerEvent::REQUEST_RECEIVED, iox::popo::createNotificationCallback(onRequestReceived))
        .or_else([](auto) {
            std::cerr << "unable to attach server" << std::endl;
            std::exit(EXIT_FAILURE);
        });

    shutdownSemaphore.wait().or_else(
        [](auto) { std::cerr << "unable to call wait on shutdownSemaphore - semaphore corrupt?" << std::endl; });

    listener.detachEvent(server, iox::popo::ServerEvent::REQUEST_RECEIVED);

    return EXIT_SUCCESS;
}
