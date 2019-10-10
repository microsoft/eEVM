# intx: extended precision integer library.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

set(install_dir ${CMAKE_CURRENT_BINARY_DIR}/install)
set(build_dir ${CMAKE_CURRENT_BINARY_DIR}/build)
set(use_build_dir ${CMAKE_CURRENT_BINARY_DIR}/use_cmake_package)

file(MAKE_DIRECTORY ${build_dir} ${use_build_dir})

add_test(
    NAME ${PROJECT_NAME}/cmake_package/configure
    COMMAND
        ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR}
        ${PROJECT_SOURCE_DIR} -DCMAKE_INSTALL_PREFIX=${install_dir}
        -DINTX_TESTING=OFF -DINTX_BENCHMARKING=OFF -DINTX_FUZZING=OFF
    WORKING_DIRECTORY ${build_dir}
)

add_test(
    NAME ${PROJECT_NAME}/cmake_package/install
    COMMAND ${CMAKE_COMMAND} --build ${build_dir} --target install
)
set_tests_properties(
    ${PROJECT_NAME}/cmake_package/install
    PROPERTIES DEPENDS ${PROJECT_NAME}/cmake_package/configure
)

if(CMAKE_BUILD_TYPE)
    set(build_type_arg -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
endif()

add_test(
    NAME ${PROJECT_NAME}/cmake_package/use_cmake_package/configure
    COMMAND
        ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR}
        ${PROJECT_SOURCE_DIR}/examples/use_cmake_package
        -DCMAKE_PREFIX_PATH=${install_dir} ${build_type_arg}
    WORKING_DIRECTORY ${use_build_dir}
)
set_tests_properties(
    ${PROJECT_NAME}/cmake_package/use_cmake_package/configure
    PROPERTIES DEPENDS ${PROJECT_NAME}/cmake_package/install
)

add_test(
    NAME ${PROJECT_NAME}/cmake_package/use_cmake_package/build
    COMMAND ${CMAKE_COMMAND} --build ${use_build_dir}
)
set_tests_properties(
    ${PROJECT_NAME}/cmake_package/use_cmake_package/build
    PROPERTIES DEPENDS ${PROJECT_NAME}/cmake_package/use_cmake_package/configure
)
