// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2023 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_CONCEPTS_SHARED_MEMORY_BUMP_ALLOCATOR_HPP
#define IOX_CONCEPTS_SHARED_MEMORY_BUMP_ALLOCATOR_HPP

#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iox/memory.hpp"
#include "shm_pointer.hpp"

#include <cstdint>

namespace iox
{
namespace cal
{
// **** Allocator not yet thread-safe!!
// The ShmBumpAllocator is not final. It is just a minimal implementation needed to have a working shared memory concept
// implementation. One of the next steps is to implement a shared memory allocator concept and provide e.g. a pool
// allocator and a bump allocator (for which this ShmBumpAllocator implementation can be reused). Note that they must be
// thread-safe.

enum class ShmBumpAllocatorError : uint8_t
{
    OUT_OF_MEMORY,
    REQUESTED_ZERO_SIZED_MEMORY
};

/// @brief A bump allocator for shared memory provided in the ctor arguments
class ShmBumpAllocator final
{
  public:
    struct ShmBumpAllocatorConfig
    {
    };
    using Configuration = ShmBumpAllocatorConfig;

    /// @brief c'tor
    /// @param[in] startAddress of the memory this allocator manages
    /// @param[in] length of the memory this allocator manages
    ShmBumpAllocator(void* const startAddress, const uint64_t length) noexcept
        // AXIVION Next Construct AutosarC++19_03-A5.2.4, AutosarC++19_03-M5.2.9 : required for low level memory management
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        : m_startAddress(reinterpret_cast<uint64_t>(startAddress))
        , m_length(length)
    {
    }

    ShmBumpAllocator(const ShmBumpAllocator&) = delete;
    ShmBumpAllocator(ShmBumpAllocator&&) noexcept = default;
    ShmBumpAllocator& operator=(const ShmBumpAllocator&) noexcept = delete;
    ShmBumpAllocator& operator=(ShmBumpAllocator&&) noexcept = default;
    ~ShmBumpAllocator() noexcept = default;

    /// @brief allocates on the memory supplied with the ctor
    /// @param[in] size of the memory to allocate, must be greater than 0
    /// @param[in] alignment of the memory to allocate
    /// @return an expected containing a distance to the memory if allocation was successful, otherwise
    /// ShmBumpAllocatorError
    // NOLINTJUSTIFICATION allocation interface requires size and alignment as integral types
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    expected<PtrDistance_t, ShmBumpAllocatorError> allocate(const uint64_t size, const uint64_t alignment) noexcept
    {
        if (size == 0)
        {
            IOX_LOG(WARN) << "Cannot allocate memory of size 0.";
            return error<ShmBumpAllocatorError>(ShmBumpAllocatorError::REQUESTED_ZERO_SIZED_MEMORY);
        }

        const uint64_t currentAddress{m_startAddress + m_currentPosition};
        uint64_t alignedPosition{align(currentAddress, alignment)};

        alignedPosition -= m_startAddress;

        const uint64_t nextPosition{alignedPosition + size};
        if (m_length >= nextPosition)
        {
            m_currentPosition = nextPosition;
        }
        else
        {
            IOX_LOG(WARN) << "Trying to allocate additional " << size << " bytes in the memory of capacity " << m_length
                          << " when there are already " << alignedPosition << " aligned bytes in use.\n Only "
                          << m_length - alignedPosition << " bytes left.";
            return error<ShmBumpAllocatorError>(ShmBumpAllocatorError::OUT_OF_MEMORY);
        }

        return success<uint64_t>(alignedPosition);
    }

    /// @brief mark the memory as unused
    void deallocate() noexcept
    {
        m_currentPosition = 0;
    }

  private:
    uint64_t m_startAddress{0U};
    uint64_t m_length{0U};
    uint64_t m_currentPosition{0U};
};
} // namespace cal
} // namespace iox

#endif

