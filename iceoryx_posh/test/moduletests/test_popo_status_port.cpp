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

#include "iceoryx_posh/internal/popo/ports/status_port.hpp"

#include "test.hpp"


namespace
{
using namespace ::testing;
using namespace iox::popo;

class StatusPort_test : public Test
{
  public:
  protected:
    StatusPort_test()
    {
    }

    ~StatusPort_test()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    using TestDataType = uint32_t;

    /// @todo move to integration tests?
    // Transaction<TestDataType> ackTransactions[1];

    StatusPortData<TestDataType> acknowledgedTransactions;
    StatusPortReader<TestDataType> sut1{acknowledgedTransactions};
    StatusPortWriter<TestDataType> sut2{acknowledgedTransactions};
};


TEST_F(StatusPort_test, InitialStateIsEmpty)
{
}

TEST_F(StatusPort_test, SendOneChunkSequentiallyIsSucessfully)
{
    constexpr uint32_t VALUE{42};
    sut2.store([](auto& valueToStore) { valueToStore = VALUE; });

    uint32_t receivedValue{0};
    sut1.take([&receivedValue](const auto& valueToTake) { receivedValue = valueToTake; });

    EXPECT_THAT(VALUE, Eq(receivedValue));
}

// TEST_F(StatusPort_test, SendOneChunkConcurrentlyIsSucessfully)
// {
//     constexpr uint32_t VALUE{42};
//     sut2.storeChunk([](auto& valueToStore) { valueToStore = VALUE; });

//     uint32_t receivedValue{0};
//     sut1.takeChunk([&receivedValue](auto& valueToTake) { receivedValue = valueToTake; });

//     EXPECT_THAT(VALUE, Eq(receivedValue));
// }

} // namespace
