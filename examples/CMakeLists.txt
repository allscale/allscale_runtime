
add_hpx_executable(
  fibonacci
  SOURCES fibonacci.cpp
  COMPONENT_DEPENDENCIES allscale
)

add_subdirectory(pfor)
add_subdirectory(uts)
add_subdirectory(resilience_examples)
