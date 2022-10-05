// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#include "iceoryx_hoofs/internal/posix_wrapper/shared_memory_object.hpp"

int main()
{
    auto shm = iox::posix::SharedMemoryObjectBuilder()
                   .permissions(iox::cxx::perms::owner_all)
                   .memorySizeInBytes(1024 * 1024 * 512)
                   .accessMode(iox::posix::AccessMode::READ_WRITE)
                   .openMode(iox::posix::OpenMode::PURGE_AND_CREATE)
                   .name("blubb")
                   .create();
}
