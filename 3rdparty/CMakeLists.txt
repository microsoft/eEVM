cmake_policy(SET CMP0076 NEW)

add_library(intx INTERFACE)
add_library(intx::intx ALIAS intx)

target_sources(intx INTERFACE
  intx/include/intx/intx.hpp
  intx/lib/intx/div.cpp
  intx/lib/intx/div.hpp
)

target_include_directories(intx INTERFACE
  intx/include/
)