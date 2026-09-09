#include "gemm_benchmark.hpp"

#include <cstddef>

// IJK: dot-product form. The k loop is a floating-point reduction, and
// vectorizing it would reassociate the additions -- GCC only does that under
// -ffast-math, which we avoid to keep results reproducible. B is also read
// column-wise (stride N). This order therefore stays scalar and cache-hostile
// no matter how the indices are written.
void gemm_ijk(const Matrix &A, const Matrix &B, Matrix &C, std::size_t N) {
  const float *bp = B.data();

  for (std::size_t i = 0; i < N; ++i) {
    const float *arow = A.data() + i * N;

    for (std::size_t j = 0; j < N; ++j) {
      float sum = 0.0f;

      for (std::size_t k = 0; k < N; ++k) {
        sum += arow[k] * bp[k * N + j];
      }

      C[i * N + j] = sum;
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("IJK", false, argc, argv, gemm_ijk);
}
