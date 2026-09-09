#include "gemm_benchmark.hpp"

#include <cstddef>

// JKI: the inner i loop walks down columns of C and A with stride N -- every
// iteration touches a different cache line, and there is no contiguous run
// to fill a vector register. No index rewrite can fix the layout; contrast
// with IKJ, whose inner loop is contiguous in both C and B.
void gemm_jki(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N) {
  const float *ap = A.data();
  float *cp = C.data();

  for (std::size_t j = 0; j < N; ++j) {
    for (std::size_t k = 0; k < N; ++k) {
      const float bkj = B[k * N + j];

      for (std::size_t i = 0; i < N; ++i) {
        cp[i * N + j] += ap[i * N + k] * bkj;
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("JKI", true, argc, argv, gemm_jki);
}
