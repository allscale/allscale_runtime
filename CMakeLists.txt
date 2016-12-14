
project(allscale)

cmake_minimum_required(VERSION 2.8)

set(HPX_ROOT_DIR /cfs/klemming/nobackup/x/xaguilar/AllScale/hpx/*)
find_package(HPX REQUIRED)

if (ALLSCALE_SHARED_MEMORY_ONLY)
  add_definitions(-DALLSCALE_SHARED_MEMORY_ONLY)
endif()

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(src)
add_subdirectory(examples)


if(ALLSCALE_WITH_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()


