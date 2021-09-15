// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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

#include "iceoryx_hoofs/internal/relocatable_pointer/relocatable_ptr.hpp"

#include "test.hpp"

#include <cstring>
#include <type_traits>

namespace
{
using namespace ::testing;
using namespace iox::rp;

template <uint32_t Size = 1024>
class Memory
{
  public:
    Memory()
    {
    }

    std::uint8_t* operator[](int i)
    {
        return &buf[i];
    }

    std::uint8_t buf[Size];
};

class RelocatableType
{
  public:
    RelocatableType(int value)
        : data(value)
        , rp(&this->data)
    {
    }

    RelocatableType(const RelocatableType&) = delete;
    RelocatableType& operator=(const RelocatableType&) = delete;

    int data;
    iox::rp::relocatable_ptr<int> rp;
};

using Data = Memory<1024>;

class Relocatable_ptr_test : public Test
{
  public:
    void SetUp() override
    {
        internal::CaptureStderr();
    }

    void TearDown() override
    {
        std::string output = internal::GetCapturedStderr();
        if (Test::HasFailure())
        {
            std::cout << output << std::endl;
        }
    }
};

TEST_F(Relocatable_ptr_test, defaultConstructionLeadsToNullpointer)
{
    iox::rp::relocatable_ptr<int> rp;
    EXPECT_EQ(rp.get(), nullptr);
}

TEST_F(Relocatable_ptr_test, nonNullPointerConstructionWorks)
{
    Data data;
    iox::rp::relocatable_ptr<Data> rp(&data);
    EXPECT_EQ(&data, rp.get());
}

TEST_F(Relocatable_ptr_test, copyCtorOfNullptrWorks)
{
    iox::rp::relocatable_ptr<Data> rp1;
    iox::rp::relocatable_ptr<Data> rp2(rp1);
    EXPECT_EQ(rp1.get(), nullptr);
    EXPECT_EQ(rp2.get(), nullptr);
}

TEST_F(Relocatable_ptr_test, moveCtorOfNullptrWorks)
{
    iox::rp::relocatable_ptr<Data> rp1;
    iox::rp::relocatable_ptr<Data> rp2(std::move(rp1));
    EXPECT_EQ(rp1.get(), nullptr);
    EXPECT_EQ(rp2.get(), nullptr);
}

TEST_F(Relocatable_ptr_test, copyAssignmentOfNullptrWorks)
{
    iox::rp::relocatable_ptr<Data> rp1;
    iox::rp::relocatable_ptr<Data> rp2;
    rp2 = rp1;
    EXPECT_EQ(rp1.get(), nullptr);
    EXPECT_EQ(rp2.get(), nullptr);
}

TEST_F(Relocatable_ptr_test, moveAssignmentOfNullptrWorks)
{
    iox::rp::relocatable_ptr<Data> rp1;
    iox::rp::relocatable_ptr<Data> rp2;
    rp2 = std::move(rp1);
    EXPECT_EQ(rp1.get(), nullptr);
    EXPECT_EQ(rp2.get(), nullptr);
}

TEST_F(Relocatable_ptr_test, copyCtorWorks)
{
    Data data;
    auto p = &data;
    iox::rp::relocatable_ptr<Data> rp1(p);
    iox::rp::relocatable_ptr<Data> rp2(rp1);
    EXPECT_EQ(rp1.get(), p);
    EXPECT_EQ(rp2.get(), p);
}

TEST_F(Relocatable_ptr_test, moveCtorWorks)
{
    Data data;
    auto p = &data;
    iox::rp::relocatable_ptr<Data> rp1(p);
    iox::rp::relocatable_ptr<Data> rp2(std::move(rp1));
    EXPECT_EQ(rp1.get(), nullptr);
    EXPECT_EQ(rp2.get(), p);
}

TEST_F(Relocatable_ptr_test, copyAssignmentWorks)
{
    Data data;
    auto p = &data;
    iox::rp::relocatable_ptr<Data> rp1(p);
    iox::rp::relocatable_ptr<Data> rp2;
    rp2 = rp1;
    EXPECT_EQ(rp1.get(), p);
    EXPECT_EQ(rp2.get(), p);
}

TEST_F(Relocatable_ptr_test, moveAssignmentWorks)
{
    Data data;
    auto p = &data;
    iox::rp::relocatable_ptr<Data> rp1(p);
    iox::rp::relocatable_ptr<Data> rp2;
    rp2 = std::move(rp1);
    EXPECT_EQ(rp1.get(), nullptr);
    EXPECT_EQ(rp2.get(), p);
}

// regular get is tested with the ctor
TEST_F(Relocatable_ptr_test, constGetWorks)
{
    Data data;
    const iox::rp::relocatable_ptr<Data> rp(&data);
    EXPECT_EQ(&data, rp.get());
}

TEST_F(Relocatable_ptr_test, conversionToRawPointerWorks)
{
    Data data;
    iox::rp::relocatable_ptr<Data> rp(&data);
    Data* p = rp;
    EXPECT_EQ(&data, p);
}

TEST_F(Relocatable_ptr_test, conversionToConstRawPointerWorks)
{
    Data data;
    const iox::rp::relocatable_ptr<Data> rp(&data);
    const Data* p = rp;
    EXPECT_EQ(&data, p);
}

TEST_F(Relocatable_ptr_test, dereferencingWorks)
{
    int x = 73;
    iox::rp::relocatable_ptr<int> rp(&x);
    EXPECT_EQ(*rp, x);
}

TEST_F(Relocatable_ptr_test, dereferencingConstWorks)
{
    int x = 73;
    const iox::rp::relocatable_ptr<int> rp(&x);
    EXPECT_EQ(*rp, x);
}

TEST_F(Relocatable_ptr_test, arrowOperatorWorks)
{
    Data data;
    iox::rp::relocatable_ptr<Data> rp(&data);
    EXPECT_EQ(&data, rp.operator->());
}

TEST_F(Relocatable_ptr_test, arrowOperatorConstWorks)
{
    Data data;
    const iox::rp::relocatable_ptr<Data> rp(&data);
    EXPECT_EQ(&data, rp.operator->());
}

TEST_F(Relocatable_ptr_test, nullptrIsEqualToNullptr)
{
    iox::rp::relocatable_ptr<Data> rp1;
    iox::rp::relocatable_ptr<Data> rp2;

    EXPECT_TRUE(operator==(rp1, rp2));
    EXPECT_FALSE(operator!=(rp1, rp2));
}

TEST_F(Relocatable_ptr_test, nullptrIsNotEqualToNonNullptr)
{
    Data data;
    iox::rp::relocatable_ptr<Data> rp1(&data);
    iox::rp::relocatable_ptr<Data> rp2;

    EXPECT_FALSE(operator==(rp1, rp2));
    EXPECT_FALSE(operator==(rp2, rp1));
    EXPECT_TRUE(operator!=(rp1, rp2));
    EXPECT_TRUE(operator!=(rp2, rp1));
}

TEST_F(Relocatable_ptr_test, equalNonNullptrComparisonWorks)
{
    Data data;
    iox::rp::relocatable_ptr<Data> rp1(&data);
    iox::rp::relocatable_ptr<Data> rp2(&data);

    EXPECT_TRUE(operator==(rp1, rp2));
    EXPECT_FALSE(operator!=(rp1, rp2));
}

TEST_F(Relocatable_ptr_test, nonEqualNonNullptrComparisonWorks)
{
    Data data1;
    Data data2;
    iox::rp::relocatable_ptr<Data> rp1(&data1);
    iox::rp::relocatable_ptr<Data> rp2(&data2);

    EXPECT_FALSE(operator==(rp1, rp2));
    EXPECT_TRUE(operator!=(rp1, rp2));
}

TEST_F(Relocatable_ptr_test, relocationWorks)
{
    using T = RelocatableType;
    using storage_t = std::aligned_storage<sizeof(T), alignof(T)>::type;
    storage_t sourceStorage, destStorage;

    void* sourcePtr = new (&sourceStorage) T(37);
    void* destPtr = &destStorage;
    T* source = reinterpret_cast<T*>(sourcePtr);
    T* dest = reinterpret_cast<T*>(destPtr);

    EXPECT_EQ(source->data, 37);
    EXPECT_EQ(*source->rp, 37);

    // sturcture is relocated by memcopy
    std::memcpy(destPtr, sourcePtr, sizeof(T));
    // memory original source is set to 0
    std::memset(sourcePtr, 0, sizeof(T));

    // reading this is leagl since it is a primitive type
    EXPECT_EQ(source->data, 0);
    EXPECT_EQ(dest->data, 37);

    // points to relocated data automatically
    EXPECT_EQ(*dest->rp, 37);
    dest->data = 73;

    EXPECT_EQ(source->data, 0);
    EXPECT_EQ(*dest->rp, 73);
}

// TODO: typed test with void version - not possible for most operations (use own file)
} // namespace
