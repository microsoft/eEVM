# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

include(GoogleTest)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

add_executable(intx-unittests
    test_builtins.cpp
    test_cases.hpp
    test_div.cpp
    test_int128.cpp
    test_intx.cpp
    test_intx_api.cpp
)
target_link_libraries(intx-unittests PRIVATE intx intx::experimental GTest::gtest GTest::main)
target_include_directories(intx-unittests PRIVATE ${intx_internal_include_dir})
set_target_properties(intx-unittests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)

gtest_add_tests(TARGET intx-unittests TEST_PREFIX ${PROJECT_NAME}/unittests/)