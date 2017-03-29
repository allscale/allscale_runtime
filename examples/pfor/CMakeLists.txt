
add_hpx_executable(
  single
  SOURCES
    single.cpp
  COMPONENT_DEPENDENCIES allscale
)

add_hpx_executable(
  chain
  SOURCES
    chain.cpp
  COMPONENT_DEPENDENCIES allscale
)

add_hpx_executable(
  fine_grained
  SOURCES
    fine_grained.cpp
  COMPONENT_DEPENDENCIES allscale
)
