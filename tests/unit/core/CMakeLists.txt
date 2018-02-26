set(tests
  #this_work_item
#  1d_stencil
  scalar_read_write
  scalar_read_only
  grid_init
  grid_stencil
  grid_init_overlap
  overlapping_async.insieme.seq
  #overlapping_pfor.insieme.seq
  serialization
  treeture
  work_item)

foreach(test ${tests})
    set(sources ${test}.cpp)
    # add example executable
    add_hpx_executable(
        ${test}_test
        SOURCES ${sources}
        COMPONENT_DEPENDENCIES allscale
    )
  if (${test} STREQUAL "overlapping_async.insieme.seq")
    add_test(NAME ${test}_test_exe COMMAND $<TARGET_FILE:${test}_test_exe> --hpx:threads=1)
  else()
    add_test(NAME ${test}_test_exe COMMAND $<TARGET_FILE:${test}_test_exe> --hpx:threads=4)
  endif()
endforeach()

add_test(NAME distributed.grid_init_test COMMAND ${HPX_PREFIX}/bin/hpxrun.py --verbose -l2 -t4 $<TARGET_FILE:grid_init_test_exe> -- --hpx:ignore-batch-env --hpx:bind=none)
