set(tests data_item data_item_manager_server this_work_item)

foreach(test ${tests})
    set(sources ${test}.cpp)
    # add example executable
    add_hpx_executable(
        ${test}_test
        SOURCES ${sources}
        COMPONENT_DEPENDENCIES allscale
    )
    add_test(NAME ${test}_test_exe COMMAND $<TARGET_FILE:${test}_test_exe>)
endforeach()

