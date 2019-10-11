if (${CMAKE_VERSION} VERSION_GREATER "3.13")
  cmake_policy(SET CMP0076 NEW)
endif()

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