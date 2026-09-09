#include "gemm_benchmark.hpp"

#include <cstddef>

// IKJ: the inner j loop streams C and B rows contiguously.
// Caveat: writing the loads as A[i * N + k] / B[k * N + j] (indices built
// from two loop variables) fails GCC's data-reference analysis, and the whole
// kernel degrades to scalar code (-fopt-info-vec-missed: "data ref analysis
// failed"). Hoisting row pointers gives the analyzer plain crow[j] / brow[j]
// accesses and the j loop auto-vectorizes. GCC still emits a runtime alias
// check because C and B might overlap (both float *); __restrict pointers
// would remove it at the price of an aliasing promise.
void gemm_ikj(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N) {
  for (std::size_t i = 0; i < N; ++i) {
    float *crow = C.data() + i * N;

    for (std::size_t k = 0; k < N; ++k) {
      const float *brow = B.data() + k * N;
      const float aik = A[i * N + k];

      for (std::size_t j = 0; j < N; ++j) {
        crow[j] += aik * brow[j];
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("IKJ", true, argc, argv, gemm_ikj);
}
