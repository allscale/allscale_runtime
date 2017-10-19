set(tests
  #this_work_item
#  1d_stencil
  scalar_read_write
  grid_init
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
    add_test(NAME ${test}_test_exe COMMAND $<TARGET_FILE:${test}_test_exe> -Ihpx.stacks.use_guard_pages=0 --hpx:threads=1)
  else()
    add_test(NAME ${test}_test_exe COMMAND $<TARGET_FILE:${test}_test_exe> -Ihpx.stacks.use_guard_pages=0)
  endif()

endforeach()

