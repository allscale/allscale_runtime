
add_hpx_executable(
  uts
  SOURCES
    rng/alfg.cpp
    rng/brg_sha1.cpp
    uts.cpp
  COMPONENT_DEPENDENCIES allscale
)
