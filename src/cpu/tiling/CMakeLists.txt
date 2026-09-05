add_executable(
  cpu_gemm_tiled
  cpu_gemm_tiled.cpp
  tiling_benchmark.hpp
)
target_compile_features(cpu_gemm_tiled PRIVATE cxx_std_23)

add_executable(
  cpu_tiling_plot
  cpu_tiling_plot.cpp
  tiling_benchmark.hpp
)
target_compile_features(cpu_tiling_plot PRIVATE cxx_std_23)

foreach(target cpu_gemm_tiled cpu_tiling_plot)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

if(BUILD_TESTING)
  add_test(NAME cpu_gemm_tiled_smoke COMMAND cpu_gemm_tiled 64 16)
  add_test(NAME cpu_gemm_tiled_edge_tiles COMMAND cpu_gemm_tiled 100 33)
  add_test(NAME cpu_gemm_tiled_sweep_smoke COMMAND cpu_gemm_tiled 96 sweep)
  add_test(NAME cpu_tiling_plot_smoke
    COMMAND cpu_tiling_plot ${CMAKE_CURRENT_SOURCE_DIR}/example_sweep.csv
            ${CMAKE_CURRENT_BINARY_DIR}/example_sweep.svg)
  add_test(NAME cpu_gemm_tiled_invalid_size COMMAND cpu_gemm_tiled 0)
  add_test(NAME cpu_gemm_tiled_invalid_block COMMAND cpu_gemm_tiled 64 0)
  set_tests_properties(cpu_gemm_tiled_invalid_size PROPERTIES WILL_FAIL TRUE)
  set_tests_properties(cpu_gemm_tiled_invalid_block PROPERTIES WILL_FAIL TRUE)
endif()
