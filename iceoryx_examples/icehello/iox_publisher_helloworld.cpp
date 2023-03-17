#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iox/bump_allocator.hpp"
#include "shared_memory_concept.hpp"

// abstract concept ... (abstract interface class)
template <typename SMT, typename Allocator>
class SMC
{
  public:
    void func();
    int bar();

  private:
    SMT m_member;
};

using BumpAllocator = int;
using PoolAllocator = float;

// something
class SM1
{
  public:
    void func()
    {
    }

    void bla()
    {
    }
};

// less abstract concept depending only on allocator
// required since partial template specialization is not allowed, see
// template <typename Allocator>
// void SMC<SM1, Allocator>::func()
template <typename Allocator>
class SMC<SM1, Allocator>
{
  public:
    void func();
    int bar();

  private:
    SM1 m_member;
};

// implementation of SMC for SM1
template <typename Allocator>
void SMC<SM1, Allocator>::func()
{
    m_member.func();
    m_member.bla();
    m_member.func();
}

template <>
void SMC<SM1, BumpAllocator>::func()
{
    // empty if hoofs not ready, tests fail until implemented
    m_member.func();
    m_member.bla();
    m_member.func();
}


// something else
class SM2
{
  public:
    void func2(int a, int b)
    {
    }
};

// implementation of SMC for SM2
template <>
void SMC<SM2, PoolAllocator>::func()
{
    m_member.func2(1, 2);
}

template <>
int SMC<SM2, PoolAllocator>::bar()
{
    m_member.func2(1, 2);
    return 1234;
}

class PosixSharedMemory
{
};

template <>
void SMC<PosixSharedMemory, BumpAllocator>::func()
{
}

struct MarikaService
{
    using SharedMemory = SMC<PosixSharedMemory, BumpAllocator>;
};

struct ElfenService
{
    using SharedMemory = SMC<SM1, PoolAllocator>;
};

int main()
{
    SMC<SM1, BumpAllocator> test1;

    test1.func();
    // test1.bar();

    SMC<SM2, PoolAllocator> test2;
    test2.func();
    test2.bar();


    using SharedMemory = iox::cal::SharedMemory<iox::posix::SharedMemoryObject, iox::BumpAllocator>;
    auto mem = iox::cal::SharedMemoryCreator()
                   .memorySizeInBytes(1234)
                   .permissions(iox::perms::owner_all)
                   .create<SharedMemory>("shmem");

    // auto mem2 = iox::cal::SharedMemoryCreator()
    //.permissions(iox::perms::owner_write) //
    //.user("Bla") //
    //.group("blubb") //
    //.accessMode(READ_WRITE)
    //.memorySizeInBytes(1234)
    //.create<SharedMemory>("name");

    auto mem3 = iox::cal::SharedMemoryOpener()
                    .accessMode(iox::posix::AccessMode::READ_WRITE)
                    .requiredMemorySize(1234)
                    .open<SharedMemory>("shmem");

    if (!mem)
    {
        // handle creation error
    }
    auto name = mem->getName();
    std::cout << name.c_str() << std::endl;
    std::cout << mem->getSizeInBytes() << std::endl;
    std::cout << mem->getStartAddress() << std::endl;
    return 0;
}

// START: use case, additional config
struct EmptyConfiguration
{
};

struct Posix_SharedMemory // MemoryType
{
    using Configuration = EmptyConfiguration;
};

struct Gpu_SharedMemory // MemoryType
{
    using Configuration = std::string;
};

struct ZeroCopyService
{
    using SharedMemory = iox::cal::SharedMemory<iox::posix::SharedMemoryObject, BumpAllocator>;
    using Configuration = std::string;
};

struct HighPerformanceUnsafeService
{
    using SharedMemory = iox::cal::SharedMemory<int, BumpAllocator>;
    using Configuration = int;
};

template <typename ServiceType>
struct Publisher
{
    static iox::expected<Publisher, iox::cal::SharedMemoryError> createPublisher(iox::cal::Name_t& service_description)
    {
        success<typename ServiceType::SharedMemory>(
            iox::cal::SharedMemoryCreator().template create<ServiceType::SharedMemory>("data_segment"));
    }

    iox::cal::Name_t& getName()
    {
        return m_dataSegment.name();
    }
    typename ServiceType::SharedMemory m_dataSegment;
};

HighPerformanceUnsafeService::Configuration get_high_perf_config()
{
}

template <typename T>
struct IceoryxService
{
    static iox::expected<IceoryxService, iox::cal::SharedMemoryError> create(const std::string&)
    {
    }

    Publisher<ZeroCopyService>&& createPublisher()
    {
    }
    Publisher<ZeroCopyService>&& createSubscriber()
    {
    }
};


void another_main()
{
    using Service = ZeroCopyService;
    // using Service = HighPerformanceUnsafeService;
    using Config = Service::Configuration;

    auto service = IceoryxService<Service>::create("service name");

    auto publisher = service->createPublisher();
    auto subscriber = service->createSubscriber();

    // auto publisher = Publisher<Service>::createPublisher("my_service").expect("failed to create publisher");
}
// END

