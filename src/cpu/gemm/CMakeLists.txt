set(GEMM_LOOP_ORDERS ijk ikj jik jki kij kji)

foreach(loop_order IN LISTS GEMM_LOOP_ORDERS)
  set(target_name cpu_gemm_${loop_order})

  add_executable(
    ${target_name}
    cpu_gemm_${loop_order}.cpp
    gemm_benchmark.hpp
  )
  target_compile_features(${target_name} PRIVATE cxx_std_23)

  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  if(GEMM_USE_AVX2)
    if(MSVC)
      target_compile_options(${target_name} PRIVATE /arch:AVX2)
    else()
      target_compile_options(${target_name} PRIVATE -mavx2 -mfma)
    endif()
  endif()

  if(BUILD_TESTING)
    add_test(NAME ${target_name}_smoke COMMAND ${target_name} 32)
    add_test(NAME ${target_name}_invalid_size COMMAND ${target_name} 0)
    set_tests_properties(${target_name}_invalid_size PROPERTIES WILL_FAIL TRUE)
  endif()
endforeach()
