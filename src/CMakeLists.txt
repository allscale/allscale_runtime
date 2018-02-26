set(_srcs
    data_item_reference.cpp
    data_requirements_check.cpp
    monitor.cpp
    resilience.cpp
    scheduler.cpp
    this_work_item.cpp
    treeture.cpp
    components/monitor_component.cpp
    components/resilience_component.cpp
    components/scheduler_component.cpp
    components/scheduler_network.cpp
    components/util/graph_colouring.cpp
    detail/work_item_impl_base.cpp
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
    target_compile_definitions(allscale_component PUBLIC ALLSCALE_HAVE_CPUFREQ)
endif()
