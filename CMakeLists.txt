cmake_minimum_required(VERSION 3.10)

project(evm)

set(CMAKE_CXX_STANDARD 17)

set(Boost_ADDITIONAL_VERSIONS "1.67" "1.67.0")
find_package(Boost 1.60.0 REQUIRED)

if(MSVC)
  # Set Windows compiler options.
  add_compile_options(/W3 /std:c++latest)
else()
  # Set Linux compiler options
  add_compile_options(-Wall -Werror)

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    separate_arguments(COVERAGE_FLAGS UNIX_COMMAND "--coverage -fprofile-arcs -ftest-coverage")
    set(PLATFORM_SPECIFIC_TEST_LIBS "gcov")
  else()
    separate_arguments(COVERAGE_FLAGS UNIX_COMMAND "-fprofile-instr-generate -fcoverage-mapping")
    set(PLATFORM_SPECIFIC_TEST_LIBS "-fprofile-instr-generate")
  endif()
endif()

file(GLOB KECCAK_SOURCES
  3rdparty/keccak/*.c
)

option(RECORD_TRACE "Record a detailed trace of EVM execution during test runs" OFF)
if(RECORD_TRACE)
  add_definitions(-DRECORD_TRACE)
endif(RECORD_TRACE)

add_library(evm STATIC
  evm/account.cpp
  evm/disassembler.cpp
  evm/rlp.cpp
  evm/simpleglobalstate.cpp
  evm/simplestorage.cpp
  evm/stack.cpp
  evm/transaction.cpp
  evm/util.cpp
  evm/processor.cpp
  ${KECCAK_SOURCES}
)
target_include_directories(evm PRIVATE
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

add_executable(evm_tests
  tests/main.cpp
  tests/harness.cpp
)
target_include_directories(evm_tests PRIVATE
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(evm_tests PRIVATE ${COVERAGE_FLAGS})
target_link_libraries(evm_tests evm ${PLATFORM_SPECIFIC_TEST_LIBS})

enable_testing()
add_test(
  NAME evm_tests
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit_test_wrapper.sh evm_tests
)

if(NOT ENV{TEST_DIR}) 
  set_tests_properties(evm_tests PROPERTIES ENVIRONMENT TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/test_cases)
endif()

function(add_sample name)
  add_executable(${name}
    samples/${name}/main.cpp
  )
  target_include_directories(${name} PRIVATE
    ${Boost_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(${name} evm)
endfunction()

add_sample(hello_world)
add_sample(sum)
add_sample(erc20)
