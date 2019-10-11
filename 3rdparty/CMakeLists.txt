add_library(intx INTERFACE)
add_library(intx::intx ALIAS intx)

target_sources(intx INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/intx/include/intx/intx.hpp
  ${CMAKE_CURRENT_LIST_DIR}/intx/lib/intx/div.cpp
  ${CMAKE_CURRENT_LIST_DIR}/intx/lib/intx/div.hpp
)

target_include_directories(intx INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/intx/include/
)