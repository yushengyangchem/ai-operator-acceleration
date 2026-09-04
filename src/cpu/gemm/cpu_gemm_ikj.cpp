#include "gemm_benchmark.hpp"

void gemm_ikj(const Matrix &A, const Matrix &B, Matrix &C, int N) {
  for (int i = 0; i < N; ++i) {
    for (int k = 0; k < N; ++k) {
      float aik = A[i * N + k];

      for (int j = 0; j < N; ++j) {
        C[i * N + j] += aik * B[k * N + j];
      }
    }
  }
}

int main(int argc, char **argv) {
  return run_gemm_benchmark("IKJ", true, argc, argv, gemm_ikj);
}
