set(tests
  fibonacci-dump-trg
  fibonacci-manual
  heat_stencil-dump-trg
  matrix-dump-trg
  matrix-manual
  pi-dump-trg
  quicksort-dump-trg
  vector_add-dump-trg
  data_item_get
  data_item_get_invariant
  stream
  two_point_correlation.allscale
  two_point_correlation
  two_point_correlation.allscale.n29
  two_point_correlation.allscale.n32
  two_point_correlation.allscale.n34
  transpose
  stencil
)

foreach(test ${tests})
    set(sources ${test}.cpp)
    # add example executable
    add_hpx_executable(
        ${test}_test
        SOURCES ${sources}
        COMPONENT_DEPENDENCIES allscale
    )
endforeach()

