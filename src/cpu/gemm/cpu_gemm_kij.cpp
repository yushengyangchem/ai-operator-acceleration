#include "gemm_benchmark.hpp"

#include <cstddef>

// KIJ: like IKJ, the inner j loop is a contiguous AXPY over C and B rows.
// The same vectorization caveat applies: A[i * N + k]-style indices defeat
// GCC's data-reference analysis, while hoisted row pointers let the j loop
// auto-vectorize (with a runtime C/B alias check).
void gemm_kij(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N) {
  const float *ap = A.data();

  for (std::size_t k = 0; k < N; ++k) {
    const float *brow = B.data() + k * N;

    for (std::size_t i = 0; i < N; ++i) {
      float *crow = C.data() + i * N;
      const float aik = ap[i * N + k];

      for (std::size_t j = 0; j < N; ++j) {
        crow[j] += aik * brow[j];
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("KIJ", true, argc, argv, gemm_kij);
}
