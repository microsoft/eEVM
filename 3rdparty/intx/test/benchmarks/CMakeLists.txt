# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

hunter_add_package(benchmark)
find_package(benchmark CONFIG REQUIRED)

find_package(GMP REQUIRED)

add_executable(intx-bench
    bench_builtins.cpp
    bench_div.cpp
    bench_int128.cpp
    benchmarks.cpp
    utils.cpp
)
target_link_libraries(intx-bench PRIVATE intx intx::experimental benchmark::benchmark GMP::gmp)
target_include_directories(intx-bench PRIVATE ${intx_internal_include_dir})
set_target_properties(intx-bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
