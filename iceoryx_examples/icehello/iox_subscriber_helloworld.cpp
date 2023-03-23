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

const Name_t validName{"valid_name"};
const uint64_t sizeGreaterZero{1};
const access_rights all{perms::all};

using Implementations =
    Types<SharedMemory<posix::SharedMemoryObject, BumpAllocator>, SharedMemory<ProcessLocal, DummyAllocator>>;

TYPED_TEST_SUITE(SharedMemory_test, Implementations, );

TYPED_TEST(SharedMemory_test, CreationWorksWithAppropriateParameters)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    EXPECT_FALSE(mem.has_error());
}

// can be removed once FileName is implemented and used in shared memory concept
TYPED_TEST(SharedMemory_test, CreationFailsWithEmptyName)
{
    using Type = typename TestFixture::SharedMemoryType;
    const Name_t emptyName("");
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(emptyName);
    ASSERT_TRUE(mem.has_error());
    EXPECT_EQ(mem.get_error(), SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);
}

TYPED_TEST(SharedMemory_test, CreationFailsWhenMemorySizeIsZero)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(0).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem.has_error());
    EXPECT_EQ(mem.get_error(), SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);
}

TYPED_TEST(SharedMemory_test, NameIsSetToPassedValidName)
{
    using Type = typename TestFixture::SharedMemoryType;
    const Name_t name("some_name");
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(name);
    ASSERT_FALSE(mem.has_error());
    EXPECT_THAT(mem->getName(), Eq(name));
}

TYPED_TEST(SharedMemory_test, MemorySizeIsAtLeastThePassedValidSize)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{1234};
    auto mem = SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());
    EXPECT_THAT(mem->getSizeInBytes(), Ge(MEMORY_SIZE));
}

TYPED_TEST(SharedMemory_test, GetStartAdressDoesNotReturnNullptrAfterSuccessfulCreation)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());
    EXPECT_THAT(mem->getStartAddress(), Ne(nullptr));
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

    EXPECT_FALSE(openedMem.has_error());
}

TYPED_TEST(SharedMemory_test, OpenFailsWithInappropriateAccessMode)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator()
                   .memorySizeInBytes(sizeGreaterZero)
                   .permissions(perms::others_read)
                   .create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(sizeGreaterZero)
                         .accessMode(posix::AccessMode::READ_WRITE)
                         .open<Type>(validName);
    ASSERT_TRUE(openedMem.has_error());

    // change error code?
    EXPECT_EQ(openedMem.get_error(), SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);
}

TYPED_TEST(SharedMemory_test, OpenFailsWhenRequiredSizeIsGreaterThanSharedMemorySize)
{
    // implement when check is added to SharedMemoryObject
}

TYPED_TEST(SharedMemory_test, OpenFailsWhenNameDoesNotMatch)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator()
                   .memorySizeInBytes(sizeGreaterZero)
                   .permissions(perms::others_read)
                   .create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto openedMem = SharedMemoryOpener()
                         .requiredMemorySize(sizeGreaterZero)
                         .accessMode(posix::AccessMode::READ_ONLY)
                         .open<Type>("other_name");
    ASSERT_TRUE(openedMem.has_error());

    // change error code?
    EXPECT_EQ(openedMem.get_error(), SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);
}

TYPED_TEST(SharedMemory_test, AllocateDoesNotReturnNullptrWithAppropriateParameters)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator()
                   .memorySizeInBytes(sizeGreaterZero)
                   .permissions(perms::others_read)
                   .create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto result = mem->allocate(sizeGreaterZero, 1);
    ASSERT_FALSE(result.has_error());
    EXPECT_THAT(result.value(), Ne(nullptr));
}

TYPED_TEST(SharedMemory_test, AllocateFailsWhenPassedSizeIsTooLarge)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator()
                   .memorySizeInBytes(sizeGreaterZero)
                   .permissions(perms::others_read)
                   .create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto result = mem->allocate(sizeGreaterZero + 1, 1);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), SharedMemoryError::SHARED_MEMORY_ALLOCATION_ERROR);
}

TYPED_TEST(SharedMemory_test, AllocateFailsWhenPassedSizeIsZero)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator()
                   .memorySizeInBytes(sizeGreaterZero)
                   .permissions(perms::others_read)
                   .create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto result = mem->allocate(0, 1);
    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.get_error(), SharedMemoryError::SHARED_MEMORY_ALLOCATION_ERROR);
}

TYPED_TEST(SharedMemory_test, AllocationIsCorrectlyAligned)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    auto mem = SharedMemoryCreator()
                   .memorySizeInBytes(4 * MEMORY_SIZE)
                   .permissions(perms::others_read)
                   .create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto allocation_result = mem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());
    auto p = reinterpret_cast<uintptr_t>(allocation_result.value());
    EXPECT_THAT(p % MEMORY_ALIGNMENT, Eq(0));

    allocation_result = mem->allocate(2 * MEMORY_SIZE, 2 * MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());
    p = reinterpret_cast<uintptr_t>(allocation_result.value());
    EXPECT_THAT(p % (2 * MEMORY_ALIGNMENT), Eq(0));
}

TYPED_TEST(SharedMemory_test, AllocateMemoryAndStoreDataWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    constexpr uint64_t MEMORY_SIZE{sizeof(int)};
    constexpr uint64_t MEMORY_ALIGNMENT{alignof(int)};

    auto mem =
        SharedMemoryCreator().memorySizeInBytes(MEMORY_SIZE).permissions(perms::others_read).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto allocation_result = mem->allocate(MEMORY_SIZE, MEMORY_ALIGNMENT);
    ASSERT_FALSE(allocation_result.has_error());

    int* data = static_cast<int*>(allocation_result.value());
    *data = std::numeric_limits<int>::min();
    EXPECT_THAT(*data, Eq(std::numeric_limits<int>::min()));
}

TYPED_TEST(SharedMemory_test, SeveralCreateAndOpenCalls)
{
    using Type = typename TestFixture::SharedMemoryType;

    // optional for lifetime regulation
    optional<expected<Type, SharedMemoryError>> mem1 =
        SharedMemoryCreator().memorySizeInBytes(sizeof(int)).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem1.has_value());
    ASSERT_FALSE(mem1->has_error());
    EXPECT_THAT(mem1->value().getName(), Eq(validName));
    EXPECT_THAT(mem1->value().getSizeInBytes(), Ge(sizeof(int)));

    // second create with same name fails
    auto mem2 = SharedMemoryCreator().memorySizeInBytes(sizeof(int)).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem2.has_error());
    EXPECT_EQ(mem2.get_error(), SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);

    // two openers are valid
    auto mem3 = SharedMemoryOpener()
                    .requiredMemorySize(sizeof(int))
                    .accessMode(posix::AccessMode::READ_WRITE)
                    .open<Type>(validName);
    EXPECT_FALSE(mem3.has_error());
    auto allocation_mem3 = mem3->allocate(sizeof(int), alignof(int));
    ASSERT_FALSE(allocation_mem3.has_error());

    auto mem4 = SharedMemoryOpener()
                    .requiredMemorySize(sizeof(int))
                    .accessMode(posix::AccessMode::READ_WRITE)
                    .open<Type>(validName);
    EXPECT_FALSE(mem4.has_error());

    // creator goes out of scope; openers should still work on valid memory, further opens are not possible
    mem1.reset();
    // EXPECT_THAT(mem1->value().getName(), Eq(validName));

    // old opener still okay
    EXPECT_THAT(mem4->getName(), Eq(validName));
    auto allocation_mem4 = mem4->allocate(sizeof(int), alignof(int));
    ASSERT_FALSE(allocation_mem4.has_error());
    int* mem4_content = static_cast<int*>(allocation_mem4.value());
    *mem4_content = 1990;
    EXPECT_EQ(*mem4_content, 1990);

    // mem3 should read the same as mem4
    int* mem3_content = static_cast<int*>(allocation_mem3.value());
    EXPECT_EQ(*mem3_content, 1990);

    // mem3 can write to memory
    *mem3_content = 13;
    EXPECT_EQ(*mem3_content, 13);

    // mem4 reads what mem3 has written
    EXPECT_EQ(*mem4_content, 13);

    // further open fails
    auto mem5 = SharedMemoryOpener()
                    .requiredMemorySize(sizeof(int))
                    .accessMode(posix::AccessMode::READ_ONLY)
                    .open<Type>(validName);
    EXPECT_TRUE(mem5.has_error());
}

TYPED_TEST(SharedMemory_test, SelfMoveAssignmentExcluded)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());
    const auto size = mem->getSizeInBytes();

    mem = std::move(mem);
    EXPECT_THAT(mem->getName(), Eq(validName));
    EXPECT_THAT(mem->getSizeInBytes(), Eq(size));
}

TYPED_TEST(SharedMemory_test, MoveAssignmentWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem1 = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem1.has_error());
    const auto size = mem1->getSizeInBytes();

    auto mem2 = std::move(mem1);
    EXPECT_THAT(mem2->getName(), Eq(validName));
    EXPECT_THAT(mem2->getSizeInBytes(), Eq(size));
}

TYPED_TEST(SharedMemory_test, MoveConstructorWorks)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem1 = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem1.has_error());
    const auto size = mem1->getSizeInBytes();

    auto mem2{std::move(mem1)};
    EXPECT_THAT(mem2->getName(), Eq(validName));
    EXPECT_THAT(mem2->getSizeInBytes(), Eq(size));
}

// creator: permissions = passed parameter?
// creator: user = passed parameter?
// creator: group = passed parameter?
// creator: accessMode = passed parameter?
// creator: does not work when wrong combination of identity and acces parameters are passed? to what extend?

// configuration tests?

// posix shared memory: error enum translation

} // namespace

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    iox::testing::TestingLogger::init();

    return RUN_ALL_TESTS();
}

