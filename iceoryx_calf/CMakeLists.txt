# Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

cmake_minimum_required(VERSION 3.5)

set(IOX_VERSION_STRING "1.0.0")


#include("${CMAKE_CURRENT_LIST_DIR}/cmake/IceoryxVersion.cmake")

#parse_version(${IOX_VERSION_STRING})
project(iceoryx_calf VERSION ${IOX_VERSION_STRING})
#adjust_version()

find_package(iceoryx_hoofs REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/IceoryxPackageHelper.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/IceoryxPlatform.cmake")

if(CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME MATCHES Darwin)
    option(BUILD_SHARED_LIBS "Create shared libraries by default" ON)
endif()

if(CLANG_TIDY)
    find_program(
        CLANG_TIDY_EXE
        NAMES "clang-tidy"
    )

    if(CLANG_TIDY_EXE)
        set(PERFORM_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else(CLANG_TIDY_EXE)
        message(WARNING "clang-tidy activated but unable to find clang-tidy executable")
    endif(CLANG_TIDY_EXE)
endif(CLANG_TIDY)

########## find_package in source tree ##########
#
set(${PROJECT_NAME}_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake
    CACHE FILEPATH
    "${PROJECT_NAME}Config.cmake to make find_package(${PROJECT_NAME}) work in source tree!"
    FORCE
)

#
########## set util internal target, needed by tests in components ##########
#
# TODO: we need something like internal public for internal build dependencies
# instead of making the whole source folder public for internal dependency resolution

if(BUILD_TEST AND NOT GTest_FOUND)
    find_package(GTest CONFIG REQUIRED)
endif(BUILD_TEST AND NOT GTest_FOUND)

if(GTest_FOUND) # only GTest_FOUND, just in case someone want's to use iceoryx_calf_testing without also building the tests

    setup_package_name_and_create_files(
        NAME ${PROJECT_NAME}_testing
        NAMESPACE iceoryx_calf_testing
        PROJECT_PREFIX ${PREFIX}
    )

    add_library(iceoryx_calf_testing
        STATIC
        testing/mocks/time_mock.cpp
        testing/timing_test.cpp
        testing/compile_test.cpp
    )

    add_library(iceoryx_calf_testing::iceoryx_calf_testing ALIAS iceoryx_calf_testing)

    set_target_properties(iceoryx_calf_testing PROPERTIES VERSION ${PROJECT_VERSION})
    if(PERFORM_CLANG_TIDY)
        set_target_properties(
            iceoryx_calf_testing PROPERTIES CXX_CLANG_TIDY "${PERFORM_CLANG_TIDY}"
        )
    endif(PERFORM_CLANG_TIDY)
    set_target_properties(iceoryx_calf_testing PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD ${ICEORYX_CXX_STANDARD}
        POSITION_INDEPENDENT_CODE ON
    )

    if(TEST_WITH_ADDITIONAL_USER)
        target_compile_definitions(iceoryx_calf_testing PUBLIC -DTEST_WITH_ADDITIONAL_USER)
    endif(TEST_WITH_ADDITIONAL_USER)

    target_include_directories(iceoryx_calf_testing
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/testing/include>
        $<INSTALL_INTERFACE:include/${PREFIX}>
    )

    target_link_libraries(iceoryx_calf_testing PRIVATE
        ${CODE_COVERAGE_LIBS}
        iceoryx_calf
        GTest::gtest
        GTest::gmock
        ${CMAKE_DL_LIBS}
    )

    target_compile_options(iceoryx_calf_testing PRIVATE ${ICEORYX_WARNINGS})

    if(LINUX)
        target_link_libraries(iceoryx_calf_testing PRIVATE rt)
    endif()

    setup_install_directories_and_export_package(
        TARGETS iceoryx_calf_testing
        INCLUDE_DIRECTORY testing/include/
    )

    #
    ########## find_package in source tree ##########
    #
    set(${PROJECT_NAME}_testing_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake
    CACHE FILEPATH
    "${PROJECT_NAME}_testingConfig.cmake to make find_package(${PROJECT_NAME}_testing) work in source tree!"
    FORCE
    )

endif(GTest_FOUND)

setup_package_name_and_create_files(
    NAME ${PROJECT_NAME}
    NAMESPACE iceoryx_calf
    PROJECT_PREFIX ${PREFIX}
)

#
########## build iceoryx util lib ##########
#

add_library(iceoryx_calf
    source/concurrent/active_object.cpp
)

add_library(iceoryx_calf::iceoryx_calf ALIAS iceoryx_calf)
set_target_properties(iceoryx_calf PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD ${ICEORYX_CXX_STANDARD}
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_compile_options(iceoryx_calf PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS})

target_include_directories(iceoryx_calf
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PREFIX}>
)



#
########## exporting library ##########
#

#TODO add support for multiple include directories to setup_install_directories_and_export_package
install(
    DIRECTORY ${ICEORYX_PLATFORM}/include/
    DESTINATION include/${PREFIX}
    COMPONENT dev
)
setup_install_directories_and_export_package(
    TARGETS iceoryx_calf
    INCLUDE_DIRECTORY include/
)

# header
install(
    FILES
        cmake/IceoryxPackageHelper.cmake
        cmake/IceoryxPlatform.cmake
        cmake/IceoryxVersion.cmake
    DESTINATION ${DESTINATION_CONFIGDIR}
)

if(BUILD_TEST)
    add_subdirectory(test)
endif(BUILD_TEST)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
  DESTINATION share/doc/iceoryx_calf
  COMPONENT dev)
