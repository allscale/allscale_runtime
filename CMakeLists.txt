project(allscale)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(HPX REQUIRED)
find_package(CPUFREQ)

if (ALLSCALE_SHARED_MEMORY_ONLY)
  add_definitions(-DALLSCALE_SHARED_MEMORY_ONLY)
endif()

if (ALLSCALE_REALTIME_VIZ)
add_definitions(-DREALTIME_VIZ)
endif()

if (ALLSCALE_WITH_PAPI)
add_definitions(-DHAVE_PAPI)
endif()

if(NOT ALLSCALE_API_DIR)
  message(FATAL_ERROR "ALLSCALE_API_DIR CMake variable not set. Please set it to point to the directory where you checked out https://github.com/allscale/allscale_api/")
endif()

include_directories(${CMAKE_SOURCE_DIR} ${ALLSCALE_API_DIR}/code/utils/include ${ALLSCALE_API_DIR}/code/api/include)

add_subdirectory(src)
add_subdirectory(examples)

if(ALLSCALE_WITH_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()
