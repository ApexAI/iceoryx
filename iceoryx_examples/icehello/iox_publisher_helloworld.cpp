#include "iceoryx_dust/cxx/std_string_support.hpp"
#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"
#include "iceoryx_hoofs/posix_wrapper/posix_call.hpp"
#include "iox/into.hpp"

#include <cstdlib>
#include <iostream>
#include <sys/resource.h>
#include <thread>
#include <vector>

using namespace iox::posix;

iox::posix::SharedMemory::Name_t generateName()
{
    static uint64_t counter = 0;

    return iox::into<iox::lossy<iox::posix::SharedMemory::Name_t>>(std::string("shm_test_")
                                                                   + std::to_string(counter++));
}

int main(int argc, char* argv[])
{
    // sysctl -w vm.max_map_count=655300

    if (argc <= 2)
    {
        std::cerr << "provide the number of files and the shared memory object size\n";
        return -1;
    }

    uint64_t number_of_files = std::stoi(argv[1]);
    uint64_t shared_memory_size = std::stoi(argv[2]);
    std::cout << "test system with " << number_of_files << " shared memory objects of size" << shared_memory_size
              << std::endl;

    struct rlimit limit;
    posixCall(getrlimit)(RLIMIT_NOFILE, &limit)
        .failureReturnValue(-1)
        .evaluate()
        .expect("Failed to acquire file handle limit.");

    std::cout << "file limit: " << limit.rlim_cur << " / " << limit.rlim_max << std::endl;

    limit.rlim_cur = number_of_files;
    limit.rlim_max = number_of_files;

    posixCall(setrlimit)(RLIMIT_NOFILE, &limit)
        .failureReturnValue(-1)
        .evaluate()
        .expect("Failed to acquire file handle limit.");

    std::cout << "new file limit: " << limit.rlim_cur << " / " << limit.rlim_max << std::endl;


    std::vector<iox::posix::SharedMemoryObject> shms;

    // add 10 to take care of the internal implicit file usages
    for (uint64_t i = 0; i + 10 < number_of_files; ++i)
    {
        auto shm = iox::posix::SharedMemoryObjectBuilder()
                       .memorySizeInBytes(shared_memory_size)
                       .accessMode(iox::posix::AccessMode::READ_WRITE)
                       .openMode(iox::posix::OpenMode::OPEN_OR_CREATE)
                       .permissions(iox::perms::owner_all)
                       .name(generateName())
                       .create();

        if (shm.has_error())
        {
            std::cerr << "Failed to create shm_test_" << i << std::endl;
            return -1;
        }
        else
        {
            std::cout << "Create and map shared memory: shm_test_" << i << std::endl;
        }

        shms.emplace_back(std::move(shm.value()));
    }

    std::cout << "Created and opened " << number_of_files << " successfully\n";

    return 0;
}
