
add_hpx_executable(
  fibonacci
  SOURCES fibonacci.cpp
  COMPONENT_DEPENDENCIES allscale
)
add_subdirectory(uts)
