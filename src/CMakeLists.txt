
add_hpx_component(
  allscale
  SOURCES
    monitor.cpp
    scheduler.cpp
    this_work_item.cpp
    treeture.cpp
    detail/work_item_impl_base.cpp
    components/scheduler_component.cpp
    components/util/graph_colouring.cpp
    components/monitor_component.cpp
)
