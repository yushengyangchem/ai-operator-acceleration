#include "gemm_benchmark.hpp"

#include <cstddef>

// JIK: same dot-product form as IJK -- an FP reduction that only vectorizes
// under -ffast-math (reassociation), plus a stride-N read of B. Row pointers
// tidy up the loads but cannot fix either blocker; this order stays scalar.
void gemm_jik(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N) {
  const float *bp = B.data();

  for (std::size_t j = 0; j < N; ++j) {
    for (std::size_t i = 0; i < N; ++i) {
      const float *arow = A.data() + i * N;
      float sum = 0.0f;

      for (std::size_t k = 0; k < N; ++k) {
        sum += arow[k] * bp[k * N + j];
      }

      C[i * N + j] = sum;
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("JIK", false, argc, argv, gemm_jik);
}
