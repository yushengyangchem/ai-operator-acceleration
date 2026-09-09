#include "gemm_benchmark.hpp"

#include <cstddef>

// KJI: like JKI, the inner i loop strides N down columns of C and A -- one
// cache line per element and nothing contiguous for the vectorizer. This
// order stays slow regardless of how the kernel is written.
void gemm_kji(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N) {
  const float *ap = A.data();
  float *cp = C.data();

  for (std::size_t k = 0; k < N; ++k) {
    for (std::size_t j = 0; j < N; ++j) {
      const float bkj = B[k * N + j];

      for (std::size_t i = 0; i < N; ++i) {
        cp[i * N + j] += ap[i * N + k] * bkj;
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("KJI", true, argc, argv, gemm_kji);
}
