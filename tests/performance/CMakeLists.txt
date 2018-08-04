set(tests
  fibonacci-dump-trg
  fibonacci-manual
  heat_stencil-dump-trg
  matrix-dump-trg
  matrix-manual
  pi-dump-trg
  quicksort-dump-trg
  vector_add-dump-trg
  #data_item_get
  #data_item_get_invariant
  stream
  #tpc.allscalecc.latency_n29
  #tpc.allscalecc.latency_n30
  #tpc.allscalecc.latency_n32
  #tpc.allscalecc.latency_n34
  #tpc.allscalecc.latency_n36
  #tpc.allscalecc.throughput_2_n29
  #tpc.allscalecc.throughput_2_n30
  #tpc.allscalecc.throughput_2_n32
  #tpc.allscalecc.throughput_2_n34
  #tpc.allscalecc.throughput_2_n36
  #tpc.allscalecc.throughput_n29
  #tpc.allscalecc.throughput_n30
  #tpc.allscalecc.throughput_n32
  #tpc.allscalecc.throughput_n34
  #tpc.allscalecc.throughput_n36
  transpose
  stencil
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

