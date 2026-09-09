#include "tiling_benchmark.hpp"

#include <algorithm>
#include <cstddef>

// Blocked GEMM in IKJ form: blocks keep the B and C tiles cache-resident
// across the k loop; C must be zero-initialized because blocks accumulate
// in place.
// Caveats:
//  - Same as gemm_ikj: A[i * N + k]-style indices defeat GCC's data-reference
//    analysis, so the original version compiled to scalar code; row pointers
//    let each j loop auto-vectorize.
//  - Tiling only pays once the kernel is fast enough to be memory-bound. On
//    scalar code (~3 GFLOPS here) plain and tiled sit at the same compute
//    ceiling and the block size barely matters.
void gemm_tiled(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N,
                std::size_t block) {
  const float *ap = A.data();
  const float *bp = B.data();
  float *cp = C.data();

  for (std::size_t ii = 0; ii < N; ii += block) {
    const std::size_t i_end = std::min(ii + block, N);

    for (std::size_t jj = 0; jj < N; jj += block) {
      const std::size_t j_end = std::min(jj + block, N);

      for (std::size_t kk = 0; kk < N; kk += block) {
        const std::size_t k_end = std::min(kk + block, N);

        for (std::size_t i = ii; i < i_end; ++i) {
          float *crow = cp + i * N;

          for (std::size_t k = kk; k < k_end; ++k) {
            const float aik = ap[i * N + k];
            const float *brow = bp + k * N;

            for (std::size_t j = jj; j < j_end; ++j) {
              crow[j] += aik * brow[j];
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
