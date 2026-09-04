#include "gemm_benchmark.hpp"

void gemm_kji(const Matrix &A, const Matrix &B, Matrix &C, int N) {
  for (int k = 0; k < N; ++k) {
    for (int j = 0; j < N; ++j) {
      float bkj = B[k * N + j];

      for (int i = 0; i < N; ++i) {
        C[i * N + j] += A[i * N + k] * bkj;
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("KJI", true, argc, argv, gemm_kji);
}
