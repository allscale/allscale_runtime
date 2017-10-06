
if(CPUFREQ_FOUND)
    add_hpx_component(
      allscale
      SOURCES
        data_requirements_check.cpp
        monitor.cpp
        resilience.cpp
        scheduler.cpp
        this_work_item.cpp
        treeture.cpp
        detail/work_item_impl_base.cpp
        components/scheduler_component.cpp
        components/scheduler_network.cpp
        components/util/graph_colouring.cpp
        components/util/hardware_reconf.cpp
        components/monitor_component.cpp
        components/resilience_component.cpp
    )
    target_link_libraries(allscale_component ${CPUFREQ_LIBRARIES})
    target_compile_definitions(allscale_component PUBLIC ALLSCALE_HAVE_CPUFREQ)

elseif(NOT CPUFREQ_FOUND)
    MESSAGE(STATUS "didnt find it compile without hardwarereconf")
     add_hpx_component(
      allscale
      SOURCES
        monitor.cpp
        resilience.cpp
        scheduler.cpp
        this_work_item.cpp
        treeture.cpp
        detail/work_item_impl_base.cpp
        components/scheduler_component.cpp
        components/scheduler_network.cpp
        components/util/graph_colouring.cpp
        components/monitor_component.cpp
        components/resilience_component.cpp
    )
    target_link_libraries(allscale_component )
endif()
