# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

add_library(experimental STATIC div.hpp div.cpp)
target_include_directories(experimental PUBLIC ${PROJECT_SOURCE_DIR}/test)
target_link_libraries(experimental PUBLIC intx)
add_library(intx::experimental ALIAS experimental)
