set(_srcs
    data_item_reference.cpp
    data_requirements_check.cpp
    data_item_store.cpp
    monitor.cpp
    resilience.cpp
    scheduler.cpp
    this_work_item.cpp
    treeture.cpp
    get_num_localities.cpp
    components/monitor_component.cpp
    components/resilience_component.cpp
    components/scheduler_component.cpp
    components/scheduler_network.cpp
    components/util/graph_colouring.cpp
    detail/work_item_impl_base.cpp
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

if(CPUFREQ_FOUND)
    target_link_libraries(allscale_component ${CPUFREQ_LIBRARIES})
    target_include_directories(allscale_component PUBLIC ${CPUFREQ_INCLUDE_DIRS})
    target_compile_definitions(allscale_component PUBLIC ALLSCALE_HAVE_CPUFREQ)
endif()

if(EXTRAE_DIR)
    target_link_libraries(allscale_component ${EXTRAE_DIR}/lib/libpttrace.so)
endif()
