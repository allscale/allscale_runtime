project(allscale CXX)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

option(ALLSCALE_WITH_EXAMPLES "Build AllScale Runtime Examples" ON)

find_package(HPX REQUIRED)
find_package(CPUFREQ)

if (ALLSCALE_SHARED_MEMORY_ONLY)
  add_definitions(-DALLSCALE_SHARED_MEMORY_ONLY)
endif()

if (ALLSCALE_DEBUG_OUTPUT)
    add_definitions(-DDEBUG_)
endif()

if (ALLSCALE_REALTIME_VIZ)
add_definitions(-DREALTIME_VIZ)
endif()

if (ALLSCALE_WITH_PAPI)
add_definitions(-DHAVE_PAPI)
endif()

if (ALLSCALE_WITH_CRAY_PM)
add_definitions(-DCRAY_COUNTERS)
endif()

if (ALLSCALE_WITH_POWER_ESTIMATE)
add_definitions(-DPOWER_ESTIMATE)
endif()

if (ALLSCALE_READ_VOLTAGE)
add_definitions(-DREAD_VOLTAGE_FILE)
endif()

if (ALLSCALE_WITH_EXTRAE)
if(NOT EXTRAE_DIR)
  message(FATAL_ERROR "EXTRAE_DIR Cmake variable not set. Please set it to point to the directory where EXTRAE is installed")
else()
add_definitions(-DHAVE_EXTRAE)
include_directories(${EXTRAE_DIR}/include)
endif()

endif()

if(NOT ALLSCALE_API_DIR)
  message(FATAL_ERROR "ALLSCALE_API_DIR CMake variable not set. Please set it to point to the directory where you checked out https://github.com/allscale/allscale_api/")
endif()

add_definitions(-DALLSCALE_WITH_HPX)

include_directories(${CMAKE_SOURCE_DIR} ${ALLSCALE_API_DIR}/code/utils/include ${ALLSCALE_API_DIR}/code/api/include)

add_subdirectory(src)

if(ALLSCALE_WITH_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ALLSCALE_WITH_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()
