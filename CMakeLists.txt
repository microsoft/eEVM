cmake_minimum_required(VERSION 3.10)

project(eevm)

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

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Options 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
option(RECORD_TRACE "Record a detailed trace of EVM execution during test runs" OFF)
if(RECORD_TRACE)
  add_definitions(-DRECORD_TRACE)
endif(RECORD_TRACE)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Common variables 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
set(EEVM_INCLUDE_DIRS
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(EEVM_CORE_SRCS
  src/disassembler.cpp
  src/processor.cpp
  src/stack.cpp
  src/transaction.cpp
  src/util.cpp
)

set(EEVM_SIMPLE_SRCS
  src/simple/simpleaccount.cpp
  src/simple/simpleglobalstate.cpp
  src/simple/simplestorage.cpp
)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Libraries
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
add_library(eevm STATIC
  ${EEVM_CORE_SRCS}
  ${KECCAK_SOURCES}
)
target_include_directories(eevm PRIVATE
  ${EEVM_INCLUDE_DIRS}
)


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Executables
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
add_executable(eevm_tests
  tests/main.cpp
  tests/harness.cpp
  tests/rlp.cpp
  ${EEVM_SIMPLE_SRCS}
)
target_include_directories(eevm_tests PRIVATE
  ${EEVM_INCLUDE_DIRS}
)
target_compile_options(eevm_tests PRIVATE ${COVERAGE_FLAGS})
target_link_libraries(eevm_tests eevm ${PLATFORM_SPECIFIC_TEST_LIBS})


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Tests
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
enable_testing()

add_test(
  NAME eevm_tests
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit_test_wrapper.sh eevm_tests
)

if(NOT ENV{TEST_DIR}) 
  set_tests_properties(eevm_tests
    PROPERTIES
      ENVIRONMENT TEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/test_cases
  )
endif()


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Samples
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
function(add_sample name)
  add_executable(${name}
    samples/${name}/main.cpp
    ${EEVM_SIMPLE_SRCS}
  )
  target_include_directories(${name} PRIVATE
    ${EEVM_INCLUDE_DIRS}
  )
  target_link_libraries(${name} eevm)
endfunction()

add_sample(hello_world)
add_sample(sum)
add_sample(erc20)
