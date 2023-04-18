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
#pragma once

#include "iceoryx_hoofs/cxx/expected.hpp"
#include "iox/memory.hpp"
#include "shared_memory_concept.hpp"

#include <cstdint>

namespace iox
{
namespace cal
{
enum class ShmBumpAllocatorError : uint8_t
{
    OUT_OF_MEMORY,
    REQUESTED_ZERO_SIZED_MEMORY
};

/// @brief A bump allocator for the memory provided in the ctor arguments
class ShmBumpAllocator final
{
  public:
    /// @brief c'tor
    /// @param[in] startAddress of the memory this allocator manages
    /// @param[in] length of the memory this allocator manages
    ShmBumpAllocator(void* const startAddress, const uint64_t length) noexcept;

    ShmBumpAllocator(const ShmBumpAllocator&) = delete;
    ShmBumpAllocator(ShmBumpAllocator&&) noexcept = default;
    ShmBumpAllocator& operator=(const ShmBumpAllocator&) noexcept = delete;
    ShmBumpAllocator& operator=(ShmBumpAllocator&&) noexcept = default;
    ~ShmBumpAllocator() noexcept = default;

    /// @brief allocates on the memory supplied with the ctor
    /// @param[in] size of the memory to allocate, must be greater than 0
    /// @param[in] alignment of the memory to allocate
    /// @return an expected containing a pointer to the memory if allocation was successful, otherwise
    /// ShmBumpAllocatorError
    expected<PtrDistance_t, ShmBumpAllocatorError> allocate(const uint64_t size, const uint64_t alignment) noexcept;

    /// @brief mark the memory as unused
    void deallocate() noexcept;

  private:
    uint64_t m_startAddress{0U};
    uint64_t m_length{0U};
    uint64_t m_currentPosition{0U};
};

ShmBumpAllocator::ShmBumpAllocator(void* const startAddress, const uint64_t length) noexcept
    // AXIVION Next Construct AutosarC++19_03-A5.2.4, AutosarC++19_03-M5.2.9 : required for low level memory management
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    : m_startAddress(reinterpret_cast<uint64_t>(startAddress))
    , m_length(length)
{
}

// NOLINTJUSTIFICATION allocation interface requires size and alignment as integral types
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
expected<PtrDistance_t, ShmBumpAllocatorError> ShmBumpAllocator::allocate(const uint64_t size,
                                                                          const uint64_t alignment) noexcept
{
    if (size == 0)
    {
        IOX_LOG(WARN) << "Cannot allocate memory of size 0.";
        return error<ShmBumpAllocatorError>(ShmBumpAllocatorError::REQUESTED_ZERO_SIZED_MEMORY);
    }

    const uint64_t currentAddress{m_startAddress + m_currentPosition};
    uint64_t alignedPosition{align(currentAddress, alignment)};

    alignedPosition -= m_startAddress;

    void* allocation{nullptr};

    const uint64_t nextPosition{alignedPosition + size};
    if (m_length >= nextPosition)
    {
        // AXIVION Next Construct AutosarC++19_03-A5.2.4 : required for low level memory management
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
        allocation = reinterpret_cast<void*>(alignedPosition);
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

void ShmBumpAllocator::deallocate() noexcept
{
    m_currentPosition = 0;
}
} // namespace cal
} // namespace iox
