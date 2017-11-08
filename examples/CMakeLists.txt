
add_hpx_executable(
  fibonacci
  SOURCES fibonacci.cpp
  COMPONENT_DEPENDENCIES allscale
)

add_hpx_executable(
  stencil_1d_pfor
  SOURCES stencil_1d_pfor.cpp
  COMPONENT_DEPENDENCIES allscale
)

add_hpx_executable(
  stencil_2d_pfor
  SOURCES stencil_2d_pfor.cpp
  COMPONENT_DEPENDENCIES allscale
)
add_hpx_executable(
  ipic3d_allscalecc
  SOURCES ipic3d_allscalecc.cpp
  COMPONENT_DEPENDENCIES allscale
)

add_subdirectory(pfor)
add_subdirectory(uts)
