set(tests
  fibonacci-dump-trg
  heat_stencil-dump-trg
  matrix-dump-trg
  matrix-manual
  pi-dump-trg
  quicksort-dump-trg
  vector_add-dump-trg
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

