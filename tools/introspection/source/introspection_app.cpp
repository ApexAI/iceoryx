// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_introspection/introspection_app.hpp"
#include "iceoryx_hoofs/internal/units/duration.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/roudi/introspection_types.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_versions.hpp"

#include <chrono>
#include <iomanip>
#include <poll.h>
#include <thread>

using namespace iox::client::introspection;
using namespace iox::units::duration_literals;

namespace iox
{
namespace client
{
namespace introspection
{
void snafu()
{
    return;
}

IntrospectionApp::IntrospectionApp() noexcept
{
    iox::runtime::PoshRuntime::initRuntime(iox::roudi::INTROSPECTION_APP_NAME);

    using namespace iox::roudi;

    popo::SubscriberOptions subscriberOptions;
    subscriberOptions.queueCapacity = 1U;
    subscriberOptions.historyRequest = 1U;

    iox::popo::Subscriber<MemPoolIntrospectionInfoContainer> memPoolSubscriber(IntrospectionMempoolService,
                                                                               subscriberOptions);

    cxx::optional<popo::Sample<const MemPoolIntrospectionInfoContainer>> memPoolSample;

    int i = 10000;
    while (i > 0) // when this is commented out -> no maybe-uninitialized warning for memPoolSample
    {
        --i;

        // snafu();

        memPoolSubscriber.take().and_then([&](auto& sample) { memPoolSample = sample; });
    }
}

} // namespace introspection
} // namespace client
} // namespace iox
