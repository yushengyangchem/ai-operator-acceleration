#include "tiling_benchmark.hpp"

#include <algorithm>

// Blocked GEMM in IKJ form: the inner j loop streams contiguous C and B rows.
// C must be zero-initialized because blocks accumulate in place.
void gemm_tiled(const Matrix &A, const Matrix &B, Matrix &C, int N, int block) {
  for (int ii = 0; ii < N; ii += block) {
    const int i_end = std::min(ii + block, N);

    for (int jj = 0; jj < N; jj += block) {
      const int j_end = std::min(jj + block, N);

      for (int kk = 0; kk < N; kk += block) {
        const int k_end = std::min(kk + block, N);

        for (int i = ii; i < i_end; ++i) {
          for (int k = kk; k < k_end; ++k) {
            const float aik = A[i * N + k];

            for (int j = jj; j < j_end; ++j) {
              C[i * N + j] += aik * B[k * N + j];
            }
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_tiling_benchmark(argc, argv, gemm_tiled);
}
