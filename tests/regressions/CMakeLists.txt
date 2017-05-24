set(tests
  empty_main
  empty_main.1
  insieme.out.0
  async_basic_basic.insieme.seq
  async_capture_mutable_reference_capture_mutable_reference.insieme.seq
  async_capture_reference_capture_reference.insieme.seq
  async_capture_value_capture_value.insieme.seq
  async_side_effects_side_effects.insieme.seq
  basic_basic.insieme.seq
  cmd_options_optimized_optimized.insieme.seq
  cmd_options_unoptimized_unoptimized.insieme.seq
  fib_eager_fib_eager.insieme.seq
  insieme-src-195b1d7e
  insieme-src-1a9e077a
  insieme-src-228d4e56
  insieme-src-30e0e74e
  insieme-src-35234a9a
  insieme-src-405ca12d
  insieme-src-52b53f54
  insieme-src-532a1abf
  insieme-src-5637e0f0
  insieme-src-5b0f1c59
  insieme-src-5ca530c2
  insieme-src-6f8b4b6b
  insieme-src-7707e4bb
  insieme-src-7934293f
  insieme-src-89e92259
  insieme-src-8dfa9193
  insieme-src-99b39329
  insieme-src-9bd34046
  insieme-src-9f068c2b
  insieme-src-9f9f472d
  insieme-src-a2667acd
  insieme-src-ac7c9bac
  insieme-src-b7f5d9b7
  insieme-src-bc4a2555
  insieme-src-bea7c4c4
  insieme-src-c8fe3705
  insieme-src-dd6f1989
  insieme-src-ec4d27c7
  pfor_2d_init_2d_init.insieme.seq
  pfor_basic_basic.insieme.seq
  pfor_empty_loop_empty_loop.insieme.seq
  pfor_vector_add_vector_add.insieme.seq
  pfor_vector_update_vector_update.insieme.seq
  tutorials_heat_stencil_01_basic_01_basic.insieme.seq
  tutorials_heat_stencil_02_data_structure_02_data_structure.insieme.seq
  tutorials_heat_stencil_04_pfor_collapsed_04_pfor_collapsed.insieme.seq
  tutorials_matrix_multiplication_00_c_style_00_c_style.insieme.seq
  tutorials_matrix_multiplication_01_basic_01_basic.insieme.seq
  tutorials_matrix_multiplication_02_data_structure_02_data_structure.insieme.seq
)

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

