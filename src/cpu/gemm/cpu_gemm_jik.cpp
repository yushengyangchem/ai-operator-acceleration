#include "gemm_benchmark.hpp"

void gemm_jik(const Matrix &A, const Matrix &B, Matrix &C, int N) {
  for (int j = 0; j < N; ++j) {
    for (int i = 0; i < N; ++i) {
      float sum = 0.0f;

      for (int k = 0; k < N; ++k) {
        sum += A[i * N + k] * B[k * N + j];
      }

      C[i * N + j] = sum;
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("JIK", false, argc, argv, gemm_jik);
}
