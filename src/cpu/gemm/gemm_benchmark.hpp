#pragma once

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <expected>
#include <format>
#include <print>
#include <random>
#include <string>
#include <string_view>
#include <vector>

inline constexpr int kDefaultMatrixSize = 1024;

using Matrix = std::vector<float>;

inline std::expected<int, std::string> parse_matrix_size(int argc,
                                                         char **argv) {
  if (argc == 1) {
    return kDefaultMatrixSize;
  }

  if (argc != 2) {
    return std::unexpected{
        std::format("Usage: {} [positive-matrix-size]", argv[0])};
  }

  const std::string_view argument{argv[1]};
  int parsed = 0;
  const auto result = std::from_chars(
      argument.data(), argument.data() + argument.size(), parsed);

  if (result.ec != std::errc{} ||
      result.ptr != argument.data() + argument.size() || parsed <= 0) {
    return std::unexpected{"Matrix size must be a positive integer."};
  }

  return parsed;
}

template <typename Gemm>
int run_gemm_benchmark(std::string_view implementation, bool accumulates,
                       int argc, char **argv, Gemm gemm) {
  const auto matrix_size = parse_matrix_size(argc, argv);

  if (!matrix_size) {
    std::println(stderr, "{}", matrix_size.error());
    return EXIT_FAILURE;
  }

  const int N = *matrix_size;
  const auto element_count = static_cast<std::size_t>(N) * N;
  Matrix A(element_count);
  Matrix B(element_count);
  Matrix C(element_count, 0.0f);

  std::mt19937 rng(42);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  for (auto &x : A) {
    x = dist(rng);
  }
  for (auto &x : B) {
    x = dist(rng);
  }

  const auto run_once = [&] {
    if (accumulates) {
      std::ranges::fill(C, 0.0f);
    }
    gemm(A, B, C, N);
  };

  run_once(); // Warm-up

  const auto start = std::chrono::steady_clock::now();
  run_once();
  const auto end = std::chrono::steady_clock::now();

  const double seconds = std::chrono::duration<double>(end - start).count();
  const double gflops = 2.0 * N * N * N / seconds / 1e9;

  std::println("Implementation = {}", implementation);
  std::println("N              = {}", N);
  std::println("Time           = {:.6f} s", seconds);
  std::println("GFLOPS         = {:.2f}", gflops);
  std::println("C[0]           = {:.6f}", C[0]);

  return 0;
}
