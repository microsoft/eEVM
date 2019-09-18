# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

# This example shows how to use intx CMake package.

cmake_minimum_required(VERSION 3.5)

project(use_evmc_in_cmake LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(intx CONFIG REQUIRED)

add_executable(use_int128 use_int128.cpp)
target_link_libraries(use_int128 PRIVATE intx::int128)

add_executable(use_intx use_intx.cpp)
target_link_libraries(use_intx PRIVATE intx::intx)
