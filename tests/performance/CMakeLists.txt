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
  transpose
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

