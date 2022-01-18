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

    /// @todo move to integration tests?
    StatusPortReader<uint32_t, uint32_t> sut;
    StatusPortWriter<uint32_t, uint32_t> sut2;
};


TEST_F(StatusPort_test, InitialStateIsEmpty)
{

}


} // namespace
