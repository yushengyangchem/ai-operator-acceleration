#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

#include "gemm_benchmark.hpp"

using Matrix = std::vector<float>;

void gemm_ijk(const Matrix &A, const Matrix &B, Matrix &C, int N) {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      float sum = 0.0f;

      for (int k = 0; k < N; ++k) {
        sum += A[i * N + k] * B[k * N + j];
      }

      C[i * N + j] = sum;
    }
  }
}

int main(int argc, char **argv) {
  int N = 1024;

  if (!parse_matrix_size(argc, argv, N)) {
    return EXIT_FAILURE;
  }

  const auto element_count = matrix_element_count(N);
  Matrix A(element_count);
  Matrix B(element_count);
  Matrix C(element_count);

  std::mt19937 rng(42);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  for (auto &x : A) {
    x = dist(rng);
  }

  for (auto &x : B) {
    x = dist(rng);
  }

  // Warm-up
  gemm_ijk(A, B, C, N);

  // Benchmark
  auto start = std::chrono::steady_clock::now();

  gemm_ijk(A, B, C, N);

  auto end = std::chrono::steady_clock::now();

  double seconds = std::chrono::duration<double>(end - start).count();

  double flops = 2.0 * N * N * N;
  double gflops = flops / seconds / 1e9;

  std::printf("Implementation = IJK\n");
  std::printf("N              = %d\n", N);
  std::printf("Time           = %.6f s\n", seconds);
  std::printf("GFLOPS         = %.2f\n", gflops);
  std::printf("C[0]           = %.6f\n", C[0]);

  return 0;
}
