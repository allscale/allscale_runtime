set(_srcs
    data_item_reference.cpp
    data_requirements_check.cpp
    data_item_store.cpp
    data_item/index_service.cpp
    data_item/get_ownership_json.cpp
    dashboard.cpp
    hierarchy.cpp
    monitor.cpp
    resilience.cpp
    scheduler.cpp
    schedule_policy.cpp
    task_id.cpp
    task_times.cpp
    this_work_item.cpp
    treeture.cpp
    tuner.cpp
    optimizer.cpp
    get_num_localities.cpp
    get_num_numa_nodes.cpp
    components/internodeoptimizer.cpp
    components/monitor_component.cpp
    components/resilience_component.cpp
    components/scheduler_component.cpp
    components/util/graph_colouring.cpp
    components/localoptimizer.cpp
    components/nmsimplex_bbincr.cpp
)

if(CPUFREQ_FOUND)
    set(_srcs ${_srcs}
        components/util/hardware_reconf.cpp
    )
else()
    MESSAGE(STATUS "Didnt find CPUFREQ, will compile without hardware reconf support")
endif()

add_hpx_component(allscale SOURCES ${_srcs} COMPONENT_DEPENDENCIES iostreams)

target_compile_options(allscale_component PUBLIC -Wno-error=parentheses)
target_compile_options(allscale_component PUBLIC -Wno-error=return-type)
target_compile_options(allscale_component PUBLIC -Wno-error=sign-compare)

if(CPUFREQ_FOUND)
    target_link_libraries(allscale_component ${CPUFREQ_LIBRARIES})
    target_include_directories(allscale_component PUBLIC ${CPUFREQ_INCLUDE_DIRS})
    target_compile_definitions(allscale_component PUBLIC ALLSCALE_HAVE_CPUFREQ)
endif()

if(EXTRAE_DIR)
    target_link_libraries(allscale_component ${EXTRAE_DIR}/lib/libpttrace.so)
endif()
