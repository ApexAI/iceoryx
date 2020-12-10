// Copyright (c) 2020 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_posh/popo/reactal.hpp"

#include "test.hpp"

using namespace iox;
using namespace iox::popo;
using namespace ::testing;

class Reactal_test : public Test
{
  public:
    virtual void SetUp()
    {
        internal::CaptureStderr();
    }
    virtual void TearDown()
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }

    Reactal<1> m_sut;
};


TEST_F(Reactal_test, Simple)
{
    Reactal<2> reactal;
    auto delegator = reactal.acquireCallbackDelegator();
    auto callback = [] { printf("hello world\n"); };
    delegator.delegateCall(callback);
}
