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

#include "iceoryx_hoofs/posix_wrapper/condition.hpp"
#include "iceoryx_hoofs/testing/watch_dog.hpp"
#include "test.hpp"

#include <atomic>
#include <thread>
#include <vector>

namespace
{
using namespace ::testing;
using namespace iox::posix;
using namespace iox::cxx;

class Condition_test : public Test
{
  public:
    void SetUp() override
    {
        m_watchdog.watchAndActOnFailure([] { std::terminate(); });
    }

    const iox::units::Duration m_fatalTimeout = 2_s;
    Watchdog m_watchdog{m_fatalTimeout};
    std::chrono::milliseconds m_waitingTime = std::chrono::milliseconds(50);
    std::chrono::milliseconds m_shortWaitingTime = std::chrono::milliseconds(5);
    iox::units::Duration m_waitForTimeout = iox::units::Duration(m_waitingTime) * 2.0;
    static constexpr const uint64_t NUMBER_OF_CONCURRENT_WAITS = 4;
};

using sutAction_t = std::function<void(Condition&)>;

void waitBlocks(Condition_test& test,
                const ConditionScope scope,
                const sutAction_t& waitCall,
                const sutAction_t& notifyCall)
{
    Condition sut(scope);

    std::atomic_bool hasFinished{false};
    std::atomic_bool isThreadRunning{false};

    std::thread t{[&] {
        isThreadRunning = true;
        waitCall(sut);
        hasFinished = true;
    }};

    while (!isThreadRunning.load())
    {
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(test.m_waitingTime);
    EXPECT_FALSE(hasFinished);

    notifyCall(sut);
    t.join();
}

void waitBlocksForEveryScopeValue(Condition_test& test, const sutAction_t& waitCall, const sutAction_t& notifyCall)
{
    waitBlocks(test, ConditionScope::INTER_PROCESS, waitCall, notifyCall);
    waitBlocks(test, ConditionScope::SINGLE_PROCESS, waitCall, notifyCall);
}

TEST_F(Condition_test, waitBlocksUntilNotifyOne)
{
    waitBlocksForEveryScopeValue(
        *this, [](auto& sut) { sut.wait(); }, [](auto& sut) { sut.notifyOne(); });
}

TEST_F(Condition_test, waitForBlocksUntilNotifyOneAndReturnsTrue)
{
    waitBlocksForEveryScopeValue(
        *this, [&](auto& sut) { EXPECT_TRUE(sut.waitFor(m_waitForTimeout)); }, [&](auto& sut) { sut.notifyOne(); });
}

TEST_F(Condition_test, waitBlocksUntilNotifyAll)
{
    waitBlocksForEveryScopeValue(
        *this, [](auto& sut) { sut.wait(); }, [](auto& sut) { sut.notifyAll(); });
}

TEST_F(Condition_test, waitForBlocksUntilNotifyAllAndReturnsTrue)
{
    waitBlocksForEveryScopeValue(
        *this, [&](auto& sut) { EXPECT_TRUE(sut.waitFor(m_waitForTimeout)); }, [&](auto& sut) { sut.notifyAll(); });
}

TEST_F(Condition_test, waitForBlocksUntilTimeoutAndReturnsFalse)
{
    waitBlocksForEveryScopeValue(
        *this,
        [&](auto& sut) { EXPECT_FALSE(sut.waitFor(m_waitForTimeout)); },
        [&](auto&) { std::this_thread::sleep_for(std::chrono::milliseconds(m_waitForTimeout.toMilliseconds())); });
}

void notifyOneNotifiesCorrectly(Condition_test& test,
                                const ConditionScope scope,
                                const sutAction_t& waitCall,
                                const uint64_t numberOfNotifies)
{
    Condition sut(scope);

    std::atomic_uint64_t numberOfFinishedThreads{0U};
    std::atomic_uint64_t numberOfRunningThreads{0U};

    std::vector<std::thread> threads;

    for (uint64_t i = 0U; i < Condition_test::NUMBER_OF_CONCURRENT_WAITS; ++i)
        threads.emplace_back([&] {
            ++numberOfRunningThreads;
            waitCall(sut);
            ++numberOfFinishedThreads;
        });

    while (numberOfRunningThreads.load() != Condition_test::NUMBER_OF_CONCURRENT_WAITS)
    {
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(test.m_shortWaitingTime);
    EXPECT_THAT(numberOfFinishedThreads.load(), Eq(0U));
    for (uint64_t i = 0U; i < numberOfNotifies; ++i)
    {
        sut.notifyOne();
    }

    std::this_thread::sleep_for(test.m_waitingTime);
    EXPECT_THAT(numberOfFinishedThreads.load(), Eq(numberOfNotifies));
    sut.notifyAll();

    for (auto& t : threads)
    {
        t.join();
    }
}

void notifyOneNotifiesCorrectlyForEveryScopeValue(Condition_test& test,
                                                  const sutAction_t& waitCall,
                                                  const uint64_t numberOfNotifies)
{
    notifyOneNotifiesCorrectly(test, ConditionScope::INTER_PROCESS, waitCall, numberOfNotifies);
    notifyOneNotifiesCorrectly(test, ConditionScope::SINGLE_PROCESS, waitCall, numberOfNotifies);
}

TEST_F(Condition_test, singleNotifyOneNotifiesOneWait)
{
    notifyOneNotifiesCorrectlyForEveryScopeValue(
        *this, [](auto& sut) { sut.wait(); }, 1);
}

TEST_F(Condition_test, multipleNotifyOneNotifiesMultipleWait)
{
    notifyOneNotifiesCorrectlyForEveryScopeValue(
        *this, [](auto& sut) { sut.wait(); }, 3);
}

TEST_F(Condition_test, singleNotifyOneNotifiesOneWaitFor)
{
    notifyOneNotifiesCorrectlyForEveryScopeValue(
        *this, [&](auto& sut) { sut.waitFor(m_waitForTimeout); }, 1);
}

TEST_F(Condition_test, multipleNotifyOneNotifiesMultipleWaitFor)
{
    notifyOneNotifiesCorrectlyForEveryScopeValue(
        *this, [&](auto& sut) { sut.waitFor(m_waitForTimeout); }, 3);
}


} // namespace
