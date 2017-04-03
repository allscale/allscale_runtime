
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

add_hpx_executable(
  fine_grained_with_data_items
  SOURCES
    fine_grained_with_data_items.cpp
  COMPONENT_DEPENDENCIES allscale
)
