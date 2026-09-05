#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <expected>
#include <format>
#include <fstream>
#include <limits>
#include <print>
#include <random>
#include <string>
#include <string_view>
#include <vector>

inline constexpr int kDefaultMatrixSize = 1024;
inline constexpr int kSweepRepetitions = 3;

inline constexpr std::array<int, 8> kSweepBlockSizes{8,  16, 24, 32,
                                                     48, 64, 96, 128};

using Matrix = std::vector<float>;

struct TilingOptions {
  int matrix_size;
  int block_size;
  bool sweep;
};

struct CacheLevel {
  int level;
  std::string type;
  double bytes;
};

inline std::expected<int, std::string>
parse_positive_int(std::string_view argument, std::string_view what) {
  int value = 0;
  const auto result = std::from_chars(argument.data(),
                                      argument.data() + argument.size(), value);

  if (result.ec != std::errc{} ||
      result.ptr != argument.data() + argument.size() || value <= 0) {
    return std::unexpected{std::format("{} must be a positive integer.", what)};
  }

  return value;
}

// Usage: [matrix-size] [block-size | sweep]
inline std::expected<TilingOptions, std::string>
parse_tiling_options(int argc, char **argv) {
  if (argc > 3) {
    return std::unexpected{std::format(
        "Usage: {} [positive-matrix-size] [block-size|sweep]", argv[0])};
  }

  int matrix_size = kDefaultMatrixSize;

  if (argc >= 2) {
    const auto parsed = parse_positive_int(argv[1], "Matrix size");
    if (!parsed) {
      return std::unexpected{parsed.error()};
    }
    matrix_size = *parsed;
  }

  if (argc <= 2) {
    return TilingOptions{matrix_size, 0, true};
  }

  const std::string_view mode{argv[2]};
  if (mode == "sweep") {
    return TilingOptions{matrix_size, 0, true};
  }

  const auto block_size = parse_positive_int(mode, "Block size");
  if (!block_size) {
    return std::unexpected{block_size.error()};
  }

  return TilingOptions{matrix_size, *block_size, false};
}

inline std::string read_small_file(const std::string &path) {
  std::ifstream file{path};

  if (!file) {
    return {};
  }

  std::string content((std::istreambuf_iterator<char>{file}),
                      std::istreambuf_iterator<char>{});

  while (!content.empty() &&
         (content.back() == '\n' || content.back() == '\r' ||
          content.back() == ' ' || content.back() == '\t')) {
    content.pop_back();
  }

  return content;
}

inline double parse_cache_kib(const std::string &text) {
  std::string_view value{text};

  if (!value.empty() && (value.back() == 'K' || value.back() == 'k')) {
    value.remove_suffix(1);
  }

  double kib = 0.0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), kib);
  if (result.ec != std::errc{}) {
    return 0.0;
  }

  return kib;
}

// Reads /sys/devices/system/cpu/cpu0/cache/index*/ on Linux; empty elsewhere.
inline std::vector<CacheLevel> read_cache_hierarchy() {
  std::vector<CacheLevel> caches;

  for (int index = 0; index < 16; ++index) {
    const auto base =
        std::format("/sys/devices/system/cpu/cpu0/cache/index{}", index);
    const auto level_text = read_small_file(base + "/level");

    if (level_text.empty()) {
      break;
    }

    caches.push_back(CacheLevel{
        std::atoi(level_text.c_str()), read_small_file(base + "/type"),
        parse_cache_kib(read_small_file(base + "/size")) * 1024.0});
  }

  return caches;
}

inline const CacheLevel *find_data_cache(const std::vector<CacheLevel> &caches,
                                         int level) {
  for (const auto &cache : caches) {
    if (cache.level == level && cache.type != "Instruction") {
      return &cache;
    }
  }

  return nullptr;
}

inline std::string format_bytes(double bytes) {
  if (bytes >= 1024.0 * 1024.0) {
    return std::format("{:.2f} MiB", bytes / 1024.0 / 1024.0);
  }
  if (bytes >= 1024.0) {
    return std::format("{:.1f} KiB", bytes / 1024.0);
  }
  return std::format("{:.0f} B", bytes);
}

// The B and C tiles must stay cache-resident across the k loop; A is streamed.
inline double tile_working_set_bytes(int block) {
  return 2.0 * static_cast<double>(block) * block * sizeof(float);
}

inline std::string classify_working_set(double bytes, const CacheLevel *l1d,
                                        const CacheLevel *l2) {
  if (l1d != nullptr && bytes <= l1d->bytes) {
    return "fits in L1d";
  }
  if (l2 != nullptr && bytes <= l2->bytes) {
    return "fits in L2, exceeds L1d";
  }
  if (l2 != nullptr) {
    return "exceeds L2";
  }
  return "unknown cache sizes";
}

template <typename Gemm>
double measure_best_time(Matrix &A, Matrix &B, Matrix &C, int N, int block,
                         int repetitions, Gemm gemm) {
  std::ranges::fill(C, 0.0f);
  gemm(A, B, C, N, block); // Warm-up

  double best = std::numeric_limits<double>::max();

  for (int repetition = 0; repetition < repetitions; ++repetition) {
    std::ranges::fill(C, 0.0f);

    const auto start = std::chrono::steady_clock::now();
    gemm(A, B, C, N, block);
    const auto end = std::chrono::steady_clock::now();

    best = std::min(best, std::chrono::duration<double>(end - start).count());
  }

  return best;
}

template <typename Gemm>
int run_tiling_benchmark(int argc, char **argv, Gemm gemm) {
  const auto options = parse_tiling_options(argc, argv);

  if (!options) {
    std::println(stderr, "{}", options.error());
    return EXIT_FAILURE;
  }

  const int N = options->matrix_size;
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

  const auto gflops = [&](double seconds) {
    return 2.0 * N * N * N / seconds / 1e9;
  };

  if (!options->sweep) {
    const double seconds =
        measure_best_time(A, B, C, N, options->block_size, 1, gemm);

    std::println("Implementation = TILED");
    std::println("N              = {}", N);
    std::println("Block size     = {}", options->block_size);
    std::println("Time           = {:.6f} s", seconds);
    std::println("GFLOPS         = {:.2f}", gflops(seconds));
    std::println("C[0]           = {:.6f}", C[0]);

    return 0;
  }

  const auto caches = read_cache_hierarchy();
  const CacheLevel *l1d = find_data_cache(caches, 1);
  const CacheLevel *l2 = find_data_cache(caches, 2);

  std::println(stderr, "Cache hierarchy (cpu0):");
  for (const auto &cache : caches) {
    std::println(stderr, "  L{} {:<11} = {}", cache.level, cache.type,
                 format_bytes(cache.bytes));
  }
  std::println(stderr, "");

  std::println("block_size,gflops");

  int best_block = 0;
  double best_gflops = 0.0;

  std::println(stderr, "Block   Working set   Best time   GFLOPS");

  for (const int block : kSweepBlockSizes) {
    const double seconds =
        measure_best_time(A, B, C, N, block, kSweepRepetitions, gemm);
    const double flops = gflops(seconds);

    std::println("{},{}", block, std::format("{:.3f}", flops));
    std::println(stderr, "{:>5}   {:>11}   {:>9.6f}   {:>6.2f}", block,
                 format_bytes(tile_working_set_bytes(block)), seconds, flops);

    if (flops > best_gflops) {
      best_gflops = flops;
      best_block = block;
    }
  }

  const double best_bytes = tile_working_set_bytes(best_block);

  std::println(stderr, "");
  std::println(stderr, "Best block size  = {}", best_block);
  std::println(stderr, "Peak             = {:.2f} GFLOPS", best_gflops);
  std::println(stderr, "Working set      = {} (2 x {}^2 x {} B)",
               format_bytes(best_bytes), best_block, sizeof(float));
  if (l1d != nullptr) {
    std::println(stderr, "L1d cache        = {} -> {}",
                 format_bytes(l1d->bytes),
                 classify_working_set(best_bytes, l1d, l2));
  } else if (l2 != nullptr) {
    std::println(stderr, "L2 cache         = {} -> {}", format_bytes(l2->bytes),
                 classify_working_set(best_bytes, l1d, l2));
  }

  return 0;
}
