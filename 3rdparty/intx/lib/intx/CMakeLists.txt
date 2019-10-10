# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

set(include_dir ${PROJECT_SOURCE_DIR}/include)

add_library(int128 INTERFACE)
add_library(intx::int128 ALIAS int128)
target_sources(int128 INTERFACE $<BUILD_INTERFACE:${include_dir}/intx/int128.hpp>)
target_include_directories(int128 INTERFACE $<BUILD_INTERFACE:${include_dir}>$<INSTALL_INTERFACE:include>)


add_library(intx STATIC
    ${include_dir}/intx/intx.hpp
    div.cpp
    div.hpp
)
add_library(intx::intx ALIAS intx)
target_include_directories(intx PUBLIC $<BUILD_INTERFACE:${include_dir}>$<INSTALL_INTERFACE:include>)
target_link_libraries(intx PUBLIC intx::int128)

install(
    TARGETS intx int128
    EXPORT intxTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    DIRECTORY
    ${include_dir}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
