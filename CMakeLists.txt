##
##  CMakeLists.txt
##  Controller
##
##  Created by FireWolf on 2/21/22.
##

#
# MARK: - Basics
#

cmake_minimum_required(VERSION 3.10)
include(.cmake/Colorful.cmake)

# Set the compiler toolchain if not invoked by CLion
if (DEFINED ENV{CLION_IDE})
    message(STATUS "${BoldYellow}CMakeLists.txt is used by CLion. Will use the toolchain set by CLion.${ColorReset}")
else()
    message(STATUS "${BoldYellow}CMakeLists.txt is used by CMake. Will use the toolchain set by the user.${ColorReset}")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/CurrentToolchain.cmake)
endif()

# Project Definition
project(Controller)

# Set the language standard to `c++20`
set(CMAKE_CXX_STANDARD 20)

# Set the compiler flags
set(CMAKE_C_FLAGS                   "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG             "${CMAKE_C_FLAGS_DEBUG} -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE           "${CMAKE_C_FLAGS_RELEASE}")

set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE}")

message(STATUS "C Compiler Flags: ${CMAKE_C_FLAGS}")
message(STATUS "C Compiler Flags (DEBUG): ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "C Compiler Flags (RELEASE): ${CMAKE_C_FLAGS_RELEASE}")

message(STATUS "C++ Compiler Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ Compiler Flags (DEBUG): ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "C++ Compiler Flags (RELEASE): ${CMAKE_CXX_FLAGS_RELEASE}")

#
# MARK: - Dependencies
#

find_package(fmt REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#
# MARK: - Targets
#

# Target Definition
set(TARGET        "Controller")

# Target: Controller
file(GLOB_RECURSE SOURCE_FILES Controller/*.cpp Controller/.c)
add_executable(${TARGET} ${SOURCE_FILES})
target_link_libraries(${TARGET} PRIVATE fmt::fmt-header-only)
target_link_libraries(${TARGET} PRIVATE Threads::Threads)
