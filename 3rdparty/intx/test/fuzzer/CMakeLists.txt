# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

find_package(GMP REQUIRED)

string(REPLACE fuzzer-no-link fuzzer CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

add_executable(intx-fuzzer fuzz_intx.cpp)
target_link_libraries(intx-fuzzer PRIVATE intx GMP::gmp)
set_target_properties(intx-fuzzer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
