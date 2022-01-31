# Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

set(IOX_VERSION_STRING "1.90.0")

project(iceoryx_tools VERSION ${IOX_VERSION_STRING})

find_package(iceoryx_posh REQUIRED)
find_package(iceoryx_hoofs REQUIRED)

include(IceoryxPackageHelper)
include(IceoryxPlatform)

if(CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME MATCHES Darwin)
    option(BUILD_SHARED_LIBS "Create shared libraries by default" ON)
endif()

#
########## build apps ##########
#
add_executable(iox-receiver
    source/iox_receiver.cpp
)
target_link_libraries(iox-receiver
    iceoryx_posh::iceoryx_posh
)

target_compile_options(iox-receiver PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

add_executable(iox-sender
    source/iox_sender.cpp
)
target_link_libraries(iox-sender
    iceoryx_posh::iceoryx_posh
)

target_compile_options(iox-sender PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})


set_target_properties(iox-receiver iox-sender
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
)
