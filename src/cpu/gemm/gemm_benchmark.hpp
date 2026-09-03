#pragma once

#include <charconv>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <string_view>

inline constexpr int kDefaultMatrixSize = 1024;

inline std::optional<int> parse_matrix_size(int argc, char **argv) {
  if (argc == 1) {
    return kDefaultMatrixSize;
  }

  if (argc != 2) {
    std::fprintf(stderr, "Usage: %s [positive-matrix-size]\n", argv[0]);
    return std::nullopt;
  }

  const std::string_view argument(argv[1]);
  int parsed_size = 0;
  const auto result = std::from_chars(
      argument.data(), argument.data() + argument.size(), parsed_size);

  if (result.ec != std::errc{} ||
      result.ptr != argument.data() + argument.size() || parsed_size <= 0) {
    std::fprintf(stderr, "Matrix size must be a positive integer.\n");
    return std::nullopt;
  }

  return parsed_size;
}

inline std::size_t matrix_element_count(int matrix_size) {
  const auto size = static_cast<std::size_t>(matrix_size);
  return size * size;
}
