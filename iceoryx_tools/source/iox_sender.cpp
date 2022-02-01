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

#include "iceoryx_hoofs/cxx/command_line_parser.hpp"
#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/popo/untyped_publisher.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

using namespace iox;
void print(const void* const memory, const uint64_t length, const uint64_t counter) noexcept
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << std::put_time(std::localtime(&now), "%F %T : ") << "[ send ]{" << counter << "} ";

    for (uint64_t i = 0; i < length; ++i)
    {
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<uint16_t>(static_cast<const uint8_t*>(memory)[i]) << std::dec << " ";
    }
    std::cout << std::endl;
}

bool sendDataReceivedFromPipe(iox::popo::UntypedPublisher& publisher, uint64_t& counter, const uint64_t chunkSize)
{
    bool stopPublish = false;
    publisher.loan(chunkSize).and_then([&](auto& sample) {
        char* data = static_cast<char*>(sample);
        for (uint64_t i = 0; i < chunkSize && std::cin.good(); ++i)
        {
            data[i] = static_cast<char>(std::cin.get());
            if (data[i] == EOF)
            {
                for (uint64_t k = i + 1; k < chunkSize; ++k)
                {
                    data[i] = '\0';
                }
                stopPublish = true;
            }
        }
        print(data, chunkSize, counter++);
        publisher.publish(sample);
    });
    return stopPublish;
}

int main(int argc, char* argv[])
{
    iox::log::LogManager::GetLogManager().SetDefaultLogLevel(iox::log::LogLevel::kError);

    auto options =
        cxx::CommandLineParser()
            .addOption({'s', "service", "Name of the service to publish to.", cxx::ArgumentType::REQUIRED_VALUE})
            .addOption({'i', "instance", "Name of the instance to publish to.", cxx::ArgumentType::REQUIRED_VALUE})
            .addOption({'e', "event", "Mame of the event to publish to.", cxx::ArgumentType::REQUIRED_VALUE})
            .addOption({'r', "runtime", "Name used to register at RouDi.", cxx::ArgumentType::OPTIONAL_VALUE})
            .addOption({'p', "pipe", "Read VALUE bytes from stdin and send it.", cxx::ArgumentType::OPTIONAL_VALUE})
            .parse(argc, argv);

    capro::IdString_t service(cxx::TruncateToCapacity, options.get<capro::IdString_t>("service").value());
    capro::IdString_t instance(cxx::TruncateToCapacity, options.get<capro::IdString_t>("instance").value());
    capro::IdString_t event(cxx::TruncateToCapacity, options.get<capro::IdString_t>("event").value());

    auto maybeRuntime = options.get<RuntimeName_t>("runtime");
    RuntimeName_t runtime(cxx::TruncateToCapacity, (maybeRuntime) ? maybeRuntime.value() : "GenericReceiver");

    auto maybePipe = options.get<uint64_t>("pipe");

    std::cout << "\n  application  :  " << runtime << std::endl;
    std::cout << "  service      :  " << service << ", " << instance << ", " << event << "\n" << std::endl;

    iox::runtime::PoshRuntime::initRuntime(runtime);
    iox::popo::UntypedPublisher publisher({service, instance, event});

    uint64_t counter = 0;
    bool stopPublish = false;
    while (!iox::posix::hasTerminationRequested())
    {
        maybePipe.and_then(
            [&](auto& pipeSize) { stopPublish = sendDataReceivedFromPipe(publisher, counter, *maybePipe); });

        if (stopPublish)
        {
            break;
        }
    }
}
