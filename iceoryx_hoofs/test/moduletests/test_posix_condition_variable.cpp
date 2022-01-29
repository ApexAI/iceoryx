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

#include "iceoryx_hoofs/posix_wrapper/condition_variable.hpp"
#include "iceoryx_hoofs/testing/watch_dog.hpp"
#include "test.hpp"

namespace
{
using namespace ::testing;
using namespace iox::posix;
using namespace iox::cxx;

class TestClass
{
  public:
    TestClass(const int a, const int b)
        : a{a}
        , b{b}
    {
    }

    int a = 0;
    int b = 0;
};

class ConditionVariable_test : public Test
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

    iox::cxx::optional<ConditionVariable<TestClass>> sutStorage;
};

TEST_F(ConditionVariable_test, UnsetPredicateLeadsToErrorInBuilder)
{
    auto result = ConditionVariableBuilder<TestClass>().scope(ConditionScope::SINGLE_PROCESS).create(sutStorage, 1, 2);

    ASSERT_THAT(result.has_error(), Eq(true));
    EXPECT_THAT(result.get_error(), Eq(ConditionVariableError::PREDICATE_IS_NOT_SET));
}

} // namespace
