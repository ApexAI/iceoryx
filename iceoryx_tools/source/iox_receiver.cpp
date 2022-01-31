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

#include "iceoryx_hoofs/cxx/convert.hpp"
#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iceoryx_hoofs/cxx/string.hpp"
#include "iceoryx_hoofs/cxx/vector.hpp"
#include "iceoryx_hoofs/posix_wrapper/signal_watcher.hpp"
#include "iceoryx_posh/popo/untyped_subscriber.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

#include <cstring>
#include <iomanip>
#include <iostream>

using namespace iox;

enum class ArgumentType
{
    SWITCH,
    REQUIRED_VALUE,
    OPTIONAL_VALUE
};

class CommandLineOptions
{
  public:
    static constexpr uint64_t MAX_NUMBER_OF_ARGUMENTS = 16;
    static constexpr uint64_t MAX_OPTION_NAME_LENGTH = 32;
    static constexpr uint64_t MAX_OPTION_VALUE_LENGTH = 128;
    static constexpr uint64_t MAX_BINARY_NAME_LENGTH = 1024;

    using name_t = cxx::string<MAX_OPTION_NAME_LENGTH>;
    using value_t = cxx::string<MAX_OPTION_VALUE_LENGTH>;
    using binaryName_t = cxx::string<MAX_BINARY_NAME_LENGTH>;

    enum class Result
    {
        NO_SUCH_VALUE,
        UNABLE_TO_CONVERT_VALUE
    };

    template <typename T>
    cxx::expected<T, Result> get(const name_t& optionName) const noexcept;
    bool has(const name_t& switchName) const noexcept;
    const binaryName_t& binaryName() const noexcept;

    friend class CommandLineParser;

  private:
    struct argument_t
    {
        char shortId;
        name_t id;
        value_t value;
    };

    binaryName_t m_binaryName;
    cxx::vector<argument_t, MAX_NUMBER_OF_ARGUMENTS> m_arguments;
};


class CommandLineParser
{
  public:
    static constexpr uint64_t MAX_DESCRIPTION_LENGTH = 1024;
    static constexpr uint64_t OPTION_OUTPUT_WIDTH = 45;
    static constexpr char NO_SHORT_OPTION = '\0';

    using description_t = cxx::string<MAX_DESCRIPTION_LENGTH>;

    struct entry_t
    {
        char shortOption = NO_SHORT_OPTION;
        CommandLineOptions::name_t longOption;
        description_t description;
        ArgumentType type = ArgumentType::SWITCH;
    };

    CommandLineParser() noexcept;

    CommandLineParser&& addOption(const entry_t& option) && noexcept;
    CommandLineOptions parse(int argc, char* argv[]) && noexcept;

  private:
    cxx::optional<entry_t> getOption(const CommandLineOptions::name_t& name) const noexcept;
    bool areAllRequiredValuesPresent(const CommandLineOptions& options) const noexcept;
    void printHelpAndExit(const char* binaryName) const noexcept;

  private:
    cxx::vector<entry_t, CommandLineOptions::MAX_NUMBER_OF_ARGUMENTS> m_availableOptions;
};


CommandLineParser::CommandLineParser() noexcept
{
    std::move(*this).addOption({'h', "help", "Display help.", ArgumentType::SWITCH});
}

CommandLineOptions CommandLineParser::parse(int argc, char* argv[]) && noexcept
{
    CommandLineOptions options;
    for (uint64_t i = 0U; i < static_cast<uint64_t>(argc); ++i)
    {
        if (i == 0)
        {
            if (strnlen(argv[i], CommandLineOptions::MAX_BINARY_NAME_LENGTH + 1)
                > CommandLineOptions::MAX_BINARY_NAME_LENGTH)
            {
                std::cerr << "The \"" << argv[i] << "\" binary path is too long" << std::endl;
                printHelpAndExit(argv[0]);
            }
            options.m_binaryName.unsafe_assign(argv[i]);
        }
        else
        {
            if (argv[i][0] != '-')
            {
                std::cerr << "Every option has to start with \"-\" but \"" << argv[i] << "\" does not." << std::endl;
                printHelpAndExit(argv[0]);
            }

            uint64_t argIdentifierLength = strnlen(argv[i], CommandLineOptions::MAX_OPTION_NAME_LENGTH + 1);

            if (argIdentifierLength == 1 || (argIdentifierLength == 2 && argv[i][1] == '-'))
            {
                std::cerr << "Empty option names are forbidden" << std::endl;
                printHelpAndExit(argv[0]);
            }
            else if (argIdentifierLength > 2 && argv[i][1] != '-')
            {
                std::cerr << "Only one letter allowed when using a short option name. This \"" << argv[i]
                          << "\" is not valid." << std::endl;
                printHelpAndExit(argv[0]);
            }
            else if (argIdentifierLength > 2 && argv[i][2] == '-')
            {
                std::cerr << "A long option name should start after \"--\". This \"" << argv[i] << "\" is not valid."
                          << std::endl;
                printHelpAndExit(argv[0]);
            }
            else if (argIdentifierLength > CommandLineOptions::MAX_OPTION_NAME_LENGTH)
            {
                std::cerr << "\"" << argv[i] << "\" is longer then the maximum supported size of "
                          << CommandLineOptions::MAX_OPTION_NAME_LENGTH << " for option names." << std::endl;
                printHelpAndExit(argv[0]);
            }

            uint64_t optionNameStart = (argv[i][1] == '-') ? 2 : 1;
            auto optionEntry =
                getOption(CommandLineOptions::name_t(cxx::TruncateToCapacity, argv[i] + optionNameStart));

            if (!optionEntry)
            {
                std::cerr << "Unknown option \"" << argv[i] << "\"" << std::endl;
                printHelpAndExit(argv[0]);
            }

            options.m_arguments.emplace_back();
            options.m_arguments.back().id.unsafe_assign(optionEntry->longOption);
            options.m_arguments.back().shortId = optionEntry->shortOption;

            // parse value of the option name
            if (i + 1 < static_cast<uint64_t>(argc) && argv[i + 1][0] != '-')
            {
                if (optionEntry->type == ArgumentType::SWITCH)
                {
                    std::cerr << "The parameter \"" << argv[i] << "\" is a switch. You cannot set a value here."
                              << std::endl;
                    printHelpAndExit(argv[0]);
                }

                if (strnlen(argv[i + 1], CommandLineOptions::MAX_OPTION_VALUE_LENGTH + 1)
                    > CommandLineOptions::MAX_OPTION_VALUE_LENGTH)
                {
                    std::cerr << "\"" << argv[i + 1] << "\" is longer then the maximum supported size of "
                              << CommandLineOptions::MAX_OPTION_VALUE_LENGTH << " for option values." << std::endl;
                    printHelpAndExit(argv[0]);
                }
                options.m_arguments.back().value.unsafe_assign(argv[i + 1]);
                ++i;
            }
        }
    }

    if (areAllRequiredValuesPresent(options))
    {
        if (options.has("help"))
        {
            printHelpAndExit(argv[0]);
        }
        return options;
    }

    printHelpAndExit(argv[0]);
    return options;
}

cxx::optional<CommandLineParser::entry_t>
CommandLineParser::getOption(const CommandLineOptions::name_t& name) const noexcept
{
    const auto nameSize = name.size();
    for (const auto& r : m_availableOptions)
    {
        if (name == r.longOption || (nameSize == 1 && name.c_str()[0] == r.shortOption))
        {
            return r;
        }
    }
    return cxx::nullopt;
}

bool CommandLineParser::areAllRequiredValuesPresent(const CommandLineOptions& options) const noexcept
{
    bool allPresent = true;
    for (const auto& r : m_availableOptions)
    {
        if (r.type == ArgumentType::REQUIRED_VALUE)
        {
            bool isValuePresent = false;
            for (const auto& o : options.m_arguments)
            {
                if (o.id == r.longOption || (o.id.size() == 1 && o.id.c_str()[0] == r.shortOption))
                {
                    isValuePresent = true;
                    break;
                }
            }
            if (!isValuePresent)
            {
                std::cout << "Required option \"";

                if (r.shortOption != NO_SHORT_OPTION)
                {
                    std::cout << "-" << r.shortOption;
                }
                if (r.shortOption != NO_SHORT_OPTION && !r.longOption.empty())
                {
                    std::cout << ", ";
                }
                if (!r.longOption.empty())
                {
                    std::cout << "--" << r.longOption;
                }

                std::cout << "\" is unset!" << std::endl;
                allPresent = false;
            }
        }
    }
    return allPresent;
}

const CommandLineOptions::binaryName_t& CommandLineOptions::binaryName() const noexcept
{
    return m_binaryName;
}

bool CommandLineOptions::has(const name_t& switchName) const noexcept
{
    for (const auto& a : m_arguments)
    {
        if (a.value.empty() && (a.id == switchName || (switchName.size() == 1 && a.shortId == switchName.c_str()[0])))
        {
            return true;
        }
    }
    return false;
}

template <typename T>
cxx::expected<T, CommandLineOptions::Result> CommandLineOptions::get(const name_t& optionName) const noexcept
{
    for (const auto& a : m_arguments)
    {
        if (a.id == optionName || (optionName.size() == 1 && a.shortId == optionName.c_str()[0]))
        {
            if (a.value.empty())
            {
                return cxx::error<Result>(Result::NO_SUCH_VALUE);
            }

            T value;
            if (!cxx::convert::fromString(a.value.c_str(), value))
            {
                std::cerr << "\"" << a.value.c_str() << "\" could not be converted to the requested type" << std::endl;
                return cxx::error<Result>(Result::UNABLE_TO_CONVERT_VALUE);
            }
            return cxx::success<T>(value);
        }
    }

    return cxx::error<Result>(Result::NO_SUCH_VALUE);
}

void CommandLineParser::printHelpAndExit(const char* binaryName) const noexcept
{
    std::cout << "\nUsage: " << binaryName << " [OPTIONS]\n" << std::endl;
    std::cout << "  Options:" << std::endl;
    for (const auto& a : m_availableOptions)
    {
        uint64_t outLength = 4U;
        std::cout << "    ";
        if (a.shortOption != NO_SHORT_OPTION)
        {
            std::cout << "-" << a.shortOption;
            outLength += 2;
        }

        if (a.shortOption != NO_SHORT_OPTION && !a.longOption.empty())
        {
            std::cout << ", ";
            outLength += 2;
        }

        if (!a.longOption.empty())
        {
            std::cout << "--" << a.longOption.c_str();
            outLength += 2 + a.longOption.size();
        }

        if (a.type == ArgumentType::REQUIRED_VALUE)
        {
            std::cout << " [REQUIRED_VALUE]";
            outLength += 17;
        }
        else if (a.type == ArgumentType::OPTIONAL_VALUE)
        {
            std::cout << " [OPTIONAL_VALUE]";
            outLength += 17;
        }

        uint64_t spacing = (outLength + 1 < OPTION_OUTPUT_WIDTH) ? OPTION_OUTPUT_WIDTH - outLength : 2;

        for (uint64_t i = 0; i < spacing; ++i)
        {
            std::cout << " ";
        }
        std::cout << a.description << std::endl;
    }
    std::cout << std::endl;
    std::exit(EXIT_FAILURE);
}

CommandLineParser&& CommandLineParser::addOption(const entry_t& option) && noexcept
{
    m_availableOptions.emplace_back(option);
    return std::move(*this);
}

void print(const void* const memory, const uint64_t length) noexcept
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << std::put_time(std::localtime(&now), "%F %T : ");

    for (uint64_t i = 0; i < length; ++i)
    {
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<uint16_t>(static_cast<const uint8_t*>(memory)[i]) << " ";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    iox::log::LogManager::GetLogManager().SetDefaultLogLevel(iox::log::LogLevel::kError);

    auto options =
        CommandLineParser()
            .addOption({'s', "service", "Name of the service to subscribe to.", ArgumentType::REQUIRED_VALUE})
            .addOption({'i', "instance", "Name of the instance to subscribe to.", ArgumentType::REQUIRED_VALUE})
            .addOption({'e', "event", "Mame of the event to subscribe to.", ArgumentType::REQUIRED_VALUE})
            .addOption({'r', "runtime", "Name used to register at RouDi.", ArgumentType::OPTIONAL_VALUE})
            .parse(argc, argv);

    capro::IdString_t service(cxx::TruncateToCapacity, options.get<capro::IdString_t>("service").value());
    capro::IdString_t instance(cxx::TruncateToCapacity, options.get<capro::IdString_t>("instance").value());
    capro::IdString_t event(cxx::TruncateToCapacity, options.get<capro::IdString_t>("event").value());

    auto maybeRuntime = options.get<RuntimeName_t>("runtime");
    RuntimeName_t runtime(cxx::TruncateToCapacity, (maybeRuntime) ? maybeRuntime.value() : "GenericReceiver");

    std::cout << "\n  application  :  " << runtime << std::endl;
    std::cout << "  service      :  " << service << ", " << instance << ", " << event << "\n" << std::endl;

    iox::runtime::PoshRuntime::initRuntime(runtime);
    iox::popo::UntypedSubscriber subscriber({service, instance, event});
    iox::popo::WaitSet<> waitset;
    waitset.attachEvent(subscriber, popo::SubscriberEvent::DATA_RECEIVED).or_else([](auto&) {
        std::cerr << "unable to attach subscriber to waitset" << std::endl;
        std::exit(EXIT_FAILURE);
    });

    while (!iox::posix::hasTerminationRequested())
    {
        waitset.wait();
        while (subscriber.hasData())
        {
            subscriber.take().and_then([](const void* payload) {
                uint64_t payloadSize = 8U;
                print(payload, payloadSize);
            });
        }
    }
}
