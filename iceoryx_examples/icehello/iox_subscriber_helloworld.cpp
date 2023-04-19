#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iceoryx_hoofs/testing/testing_logger.hpp"
#include "iox/filesystem.hpp"
#include "posix_shared_memory.hpp"
#include "process_local.hpp"
#include "shared_memory_concept.hpp"

namespace
{
using namespace iox::cal;
using namespace iox;
using namespace ::testing;

template <typename T>
class SharedMemory_test : public Test
{
  public:
    using SharedMemoryType = T;

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

const auto validName = FileName::create("valid_name").expect("Name is not a valid file name.");
const uint64_t sizeGreaterZero{1};
const access_rights all{perms::owner_all};

using Implementations =
    Types<SharedMemory<posix::SharedMemoryObject, ShmBumpAllocator>, SharedMemory<ProcessLocal, ShmBumpAllocator>>;

TYPED_TEST_SUITE(SharedMemory_test, Implementations, );

TYPED_TEST(SharedMemory_test, CreationWorksWithAppropriateParameters)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    EXPECT_FALSE(mem.has_error());
}

TYPED_TEST(SharedMemory_test, CreationFailsWhenMemorySizeIsZero)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(0).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem.has_error());
    EXPECT_EQ(mem.get_error(), SharedMemoryCreationError::REQUESTED_ZERO_SIZED_MEMORY);
}

TYPED_TEST(SharedMemory_test, NameIsSetToPassedValidName)
{
    using Type = typename TestFixture::SharedMemoryType;
    const auto name = FileName::create("some_name").expect("Name is not a valid file name.");
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(name);
    ASSERT_FALSE(mem.has_error());
    EXPECT_THAT(name, Eq(mem->getName()));
}

TYPED_TEST(SharedMemory_test, MemorySizeIsAtLeastThePassedValidSize)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{1234};
    auto mem = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());
    EXPECT_THAT(mem->getSizeInBytes(), Ge(MEMORY_SIZE));
}

TYPED_TEST(SharedMemory_test, OpenWorksWithAppropriateParameters)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(sizeGreaterZero)
                         .accessMode(posix::AccessMode::READ_ONLY)
                         .open<Type>(validName);

    ASSERT_FALSE(openedMem.has_error());
    EXPECT_THAT(openedMem->getSizeInBytes(), Ge(sizeGreaterZero));
}

TYPED_TEST(SharedMemory_test, OpenFailsWithInappropriateAccessMode)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem =
        SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(perms::owner_read).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(sizeGreaterZero)
                         .accessMode(posix::AccessMode::READ_WRITE)
                         .open<Type>(validName);
    ASSERT_TRUE(openedMem.has_error());

    // change error code in SharedMemoryObject?
    // EXPECT_EQ(openedMem.get_error(), SharedMemoryOpenError::PERMISSION_DENIED);
}

TYPED_TEST(SharedMemory_test, OpenFailsWhenRequiredSizeIsGreaterThanSharedMemorySize)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(sizeGreaterZero + 1)
                         .accessMode(posix::AccessMode::READ_WRITE)
                         .open<Type>(validName);
    ASSERT_TRUE(openedMem.has_error());
    EXPECT_EQ(openedMem.get_error(), SharedMemoryOpenError::REQUESTED_SIZE_EXCEEDS_ACTUAL_SIZE);
}

TYPED_TEST(SharedMemory_test, OpenFailsWhenNameDoesNotMatch)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    const auto other_name = FileName::create("other_name").expect("Name is not a valid file name.");
    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(sizeGreaterZero)
                         .accessMode(posix::AccessMode::READ_ONLY)
                         .open<Type>(other_name);
    ASSERT_TRUE(openedMem.has_error());

    // change error code in SharedMemoryObject?
    // EXPECT_EQ(openedMem.get_error(), SharedMemoryOpenError::SHARED_MEMORY_DOES_NOT_EXIST);
}

TYPED_TEST(SharedMemory_test, WriteAndReadShmWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(uint64_t)};

    auto mem = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto* data_ptr = reinterpret_cast<uint64_t*>(mem->getStartAddress());
    for (uint64_t i = 0; i < MEMORY_SIZE; i++)
    {
        data_ptr[i] = i * 2 + 1;
    }

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(MEMORY_SIZE)
                         .accessMode(posix::AccessMode::READ_ONLY)
                         .open<Type>(validName);
    ASSERT_FALSE(openedMem.has_error());

    data_ptr = reinterpret_cast<uint64_t*>(openedMem->getStartAddress());
    for (uint64_t i = 0; i < MEMORY_SIZE; i++)
    {
        EXPECT_THAT(data_ptr[i], Eq(i * 2 + 1));
    }
}

// correct allocation will probably be tested separately for every allocator implementation so that the following tests
// can be removed
// BEGIN allocation tests (remove later)
TYPED_TEST(SharedMemory_test, AllocateDoesNotReturnNullptrWithAppropriateParameters)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto result = mem->allocate(sizeGreaterZero, 1);
    ASSERT_FALSE(result.has_error());
    EXPECT_THAT(result->mapped_ptr(), Ne(nullptr));
}

TYPED_TEST(SharedMemory_test, AllocateFailsWhenPassedSizeIsTooLarge)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto result = mem->allocate(sizeGreaterZero * 1000, 1);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), SharedMemoryAllocationError::OUT_OF_MEMORY);
}

TYPED_TEST(SharedMemory_test, AllocateFailsWhenPassedSizeIsZero)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto result = mem->allocate(0, 1);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), SharedMemoryAllocationError::REQUESTED_ZERO_SIZED_MEMORY);
}

TYPED_TEST(SharedMemory_test, AllocationIsCorrectlyAligned)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    auto mem = SharedMemoryCreator().memorySizeInBytes(4 * MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto allocation_result = mem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());
    auto p = reinterpret_cast<uintptr_t>(allocation_result->mapped_ptr());
    EXPECT_THAT(p % MEMORY_ALIGNMENT, Eq(0));

    allocation_result = mem->allocate(2 * MEMORY_SIZE, 2 * MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());
    p = reinterpret_cast<uintptr_t>(allocation_result->mapped_ptr());
    EXPECT_THAT(p % (2 * MEMORY_ALIGNMENT), Eq(0));
}

TYPED_TEST(SharedMemory_test, OverAllocationFails)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    auto mem = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto allocation_result = mem->allocate(mem->getSizeInBytes(), MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());
    auto p = reinterpret_cast<uintptr_t>(allocation_result->mapped_ptr());
    EXPECT_THAT(p % MEMORY_ALIGNMENT, Eq(0));

    allocation_result = mem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_TRUE(allocation_result.has_error());
    EXPECT_EQ(allocation_result.get_error(), SharedMemoryAllocationError::OUT_OF_MEMORY);
}

TYPED_TEST(SharedMemory_test, AllocateMemoryAndStoreDataWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    auto mem = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto allocation_result = mem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());

    int* data = static_cast<int*>(allocation_result->mapped_ptr());
    *data = std::numeric_limits<int>::min();
    EXPECT_THAT(*data, Eq(std::numeric_limits<int>::min()));
}

TYPED_TEST(SharedMemory_test, MemoryCanBeAllocatedByCreatorAndOpener)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    auto mem = SharedMemoryCreator().memorySizeInBytes(2 * MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto allocation_result = mem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(mem.has_error());
    int* data = static_cast<int*>(allocation_result->mapped_ptr());
    *data = std::numeric_limits<int>::min();
    EXPECT_THAT(*data, Eq(std::numeric_limits<int>::min()));

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(2 * MEMORY_SIZE)
                         .accessMode(posix::AccessMode::READ_WRITE)
                         .open<Type>(validName);
    ASSERT_FALSE(openedMem.has_error());

    allocation_result = openedMem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());
    EXPECT_THAT(allocation_result->mapped_ptr(), Ne(nullptr));
    EXPECT_THAT(allocation_result->distance(), Ne(0));
    data = static_cast<int*>(allocation_result->mapped_ptr());
    *data = std::numeric_limits<int>::max();
    EXPECT_THAT(*data, Eq(std::numeric_limits<int>::max()));
}
// END allocation tests

TYPED_TEST(SharedMemory_test, SeveralCreateAndOpenCalls)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{2 * sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    // optional for lifetime regulation
    optional<expected<Type, SharedMemoryCreationError>> mem1 =
        SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem1.has_value());
    ASSERT_FALSE(mem1->has_error());
    EXPECT_THAT(validName, Eq(mem1->value().getName()));
    EXPECT_THAT(mem1->value().getSizeInBytes(), Ge(MEMORY_SIZE));

    // second create with same name fails
    auto mem2 = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem2.has_error());
    // change error code in SharedMemoryObject?
    // EXPECT_EQ(mem2.get_error(), SharedMemoryCreationError::SHARED_MEMORY_ALREADY_EXISTS);

    // two openers are valid
    auto mem3 = SharedMemoryOpener()
                    .requiredMemorySize(MEMORY_SIZE)
                    .accessMode(posix::AccessMode::READ_WRITE)
                    .open<Type>(validName);
    EXPECT_FALSE(mem3.has_error());
    auto allocation_mem3 = mem3->allocate(MEMORY_SIZE / 2, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_mem3.has_error());

    auto mem4 = SharedMemoryOpener()
                    .requiredMemorySize(MEMORY_SIZE)
                    .accessMode(posix::AccessMode::READ_WRITE)
                    .open<Type>(validName);
    EXPECT_FALSE(mem4.has_error());

    // creator goes out of scope; openers should still work on valid memory, further opens are not possible
    mem1.reset();

    // old opener still okay
    EXPECT_THAT(validName, Eq(mem4->getName()));
    auto allocation_mem4 = mem4->allocate(MEMORY_SIZE / 2, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_mem4.has_error());
    EXPECT_THAT(allocation_mem4->mapped_ptr(), Ne(nullptr));
    EXPECT_THAT(allocation_mem4->distance(), Ne(0));

    int* mem4_content = static_cast<int*>(allocation_mem4->mapped_ptr());
    *mem4_content = 1990;
    EXPECT_EQ(*mem4_content, 1990);

    // mem3 can write to memory
    int* mem3_content = static_cast<int*>(allocation_mem3->mapped_ptr());
    *mem3_content = 13;
    EXPECT_EQ(*mem3_content, 13);

    // mem4 still reads the value it has written
    EXPECT_EQ(*mem4_content, 1990);

    // further open fails
    auto mem5 = SharedMemoryOpener()
                    .requiredMemorySize(MEMORY_SIZE)
                    .accessMode(posix::AccessMode::READ_ONLY)
                    .open<Type>(validName);
    EXPECT_TRUE(mem5.has_error());
    // change error code in SharedMemoryObject?
    // EXPECT_EQ(mem5.get_error(), SharedMemoryOpenError::SHARED_MEMORY_DOES_NOT_EXIST);

    // create new shared memory with same name and write data
    auto mem6 = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem6.has_error());

    auto allocation_mem6 = mem6->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_mem6.has_error());
    int* mem6_content = static_cast<int*>(allocation_mem6->mapped_ptr());
    *mem6_content = 1984;
    EXPECT_EQ(*mem6_content, 1984);

    // mem3 and mem4 still read the old data
    EXPECT_EQ(*mem3_content, 13);
    EXPECT_EQ(*mem4_content, 1990);
}

TYPED_TEST(SharedMemory_test, SelfMoveAssignmentExcluded)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());
    const auto size = mem->getSizeInBytes();

    mem = std::move(mem);
    EXPECT_THAT(validName, Eq(mem->getName()));
    EXPECT_THAT(mem->getSizeInBytes(), Eq(size));
}

TYPED_TEST(SharedMemory_test, MoveAssignmentWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem1 = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem1.has_error());
    const auto size = mem1->getSizeInBytes();

    auto mem2 = std::move(mem1);
    EXPECT_THAT(validName, Eq(mem2->getName()));
    EXPECT_THAT(mem2->getSizeInBytes(), Eq(size));
}

TYPED_TEST(SharedMemory_test, MoveConstructorWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem1 = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem1.has_error());
    const auto size = mem1->getSizeInBytes();

    auto mem2{std::move(mem1)};
    EXPECT_THAT(validName, Eq(mem2->getName()));
    EXPECT_THAT(mem2->getSizeInBytes(), Eq(size));
}

} // namespace

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    iox::testing::TestingLogger::init();

    return RUN_ALL_TESTS();
}

