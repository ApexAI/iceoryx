#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "iceoryx_hoofs/posix_wrapper/types.hpp"
#include "iox/filesystem.hpp"
#include "posix_shared_memory.hpp"
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

using Implementations = Types<SharedMemory<posix::SharedMemoryObject, BumpAllocator>>;

TYPED_TEST_SUITE(SharedMemory_test, Implementations, );

TYPED_TEST(SharedMemory_test, CreationWorksWithAppropriateParameters)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    EXPECT_FALSE(mem.has_error());
}

TYPED_TEST(SharedMemory_test, CreationFailsWithEmptyName)
{
    using Type = typename TestFixture::SharedMemoryType;
    const Name_t emptyName("");
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(emptyName);
    ASSERT_TRUE(mem.has_error());
    EXPECT_EQ(mem.get_error(), SharedMemoryError::SHARED_MEMORY_CREATION_FAILED);
}

TYPED_TEST(SharedMemory_test, CreationFailsWithInvalidName)
{
    // implement when semantic string is available
}

TYPED_TEST(SharedMemory_test, CreationFailsWhenMemorySizeIsZero)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(0).permissions(all).create<Type>(validName);
    ASSERT_TRUE(mem.has_error());

    // change error code?
    EXPECT_EQ(mem.get_error(), SharedMemoryError::MAPPING_SHARED_MEMORY_FAILED);
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
    constexpr uint64_t SIZE{1234};
    auto mem = SharedMemoryCreator().memorySizeInBytes(SIZE).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    EXPECT_THAT(mem->getSizeInBytes(), Ge(SIZE));
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

TYPED_TEST(SharedMemory_test, OpenFailsWhenRequiredSizeIsZero)
{
    using Type = typename TestFixture::SharedMemoryType;
    auto mem = SharedMemoryCreator().memorySizeInBytes(sizeGreaterZero).permissions(all).create<Type>(validName);
    ASSERT_FALSE(mem.has_error());

    auto openedMem =
        SharedMemoryOpener().requiredMemorySize(0).accessMode(posix::AccessMode::READ_ONLY).open<Type>(validName);
    ASSERT_TRUE(openedMem.has_error());

    // change error code?
    EXPECT_EQ(openedMem.get_error(), SharedMemoryError::MAPPING_SHARED_MEMORY_FAILED);
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

TYPED_TEST(SharedMemory_test, AllocateDoesReturnNotReturnNullptrWithAppropriateParameters)
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

// creator: permissions = passed parameter?
// creator: user = passed parameter?
// creator: group = passed parameter?
// creator: accessMode = passed parameter?
// creator: does not work when wrong combination of identity and acces parameters are passed? to what extend?

// shared memory: move, destruction works?

// configuration tests?

// posix shared memory: error enum translation

} // namespace

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

