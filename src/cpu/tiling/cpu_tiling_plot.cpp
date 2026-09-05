// Reads a "block_size,gflops" CSV and renders a block-size vs GFLOPS SVG chart.
//
// Usage: cpu_tiling_plot [input.csv|-] [output.svg|-]
//                        [--l1d bytes] [--l2 bytes] [--title text]
//                        [--detect-caches|--no-detect-caches]

#include "tiling_benchmark.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Sample {
  double block_size;
  double gflops;
};

struct PlotOptions {
  std::string input;
  std::string output;
  std::string title = "Tiled GEMM: block size vs GFLOPS";
  double l1d_bytes = 0.0;
  double l2_bytes = 0.0;
  bool detect_caches = true;
};

struct CacheBoundary {
  double block;
  std::string label;
  const char *color;
};

std::string escape_xml(std::string_view text) {
  std::string escaped;

  for (const char c : text) {
    switch (c) {
    case '&':
      escaped += "&amp;";
      break;
    case '<':
      escaped += "&lt;";
      break;
    case '>':
      escaped += "&gt;";
      break;
    default:
      escaped += c;
      break;
    }
  }

  return escaped;
}

std::expected<double, std::string> parse_double(std::string_view text) {
  double value = 0.0;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);

  if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    return std::unexpected{std::format("invalid number: '{}'", text)};
  }

  return value;
}

std::vector<Sample> load_samples(std::istream &input) {
  std::vector<Sample> samples;
  std::string line;

  while (std::getline(input, line)) {
    const auto comma = line.find(',');

    if (comma == std::string::npos) {
      continue; // Skip the header row and blank lines.
    }

    const auto block = parse_double(std::string_view{line}.substr(0, comma));
    const auto gflops = parse_double(std::string_view{line}.substr(comma + 1));

    if (block && gflops && *block > 0.0 && *gflops > 0.0) {
      samples.push_back(Sample{*block, *gflops});
    }
  }

  std::ranges::sort(samples, {}, &Sample::block_size);
  return samples;
}

double nice_step(double range) {
  const double raw = range / 5.0;
  const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
  const double normalized = raw / magnitude;

  double step = 10.0;
  if (normalized < 1.5) {
    step = 1.0;
  } else if (normalized < 3.0) {
    step = 2.0;
  } else if (normalized < 7.0) {
    step = 5.0;
  }

  return step * magnitude;
}

std::string make_svg(const std::vector<Sample> &samples,
                     const std::string &title,
                     const std::vector<CacheBoundary> &boundaries) {
  constexpr double kWidth = 960;
  constexpr double kHeight = 620;
  constexpr double kLeft = 96;
  constexpr double kRight = 48;
  constexpr double kTop = 72;
  constexpr double kBottom = 76;
  constexpr double kPlotX0 = kLeft;
  constexpr double kPlotX1 = kWidth - kRight;
  constexpr double kPlotY0 = kTop;
  constexpr double kPlotY1 = kHeight - kBottom;

  const double max_gflops =
      std::ranges::max(samples, {}, &Sample::gflops).gflops;
  const double step = nice_step(max_gflops);
  const double y_max =
      std::max(step * std::ceil(max_gflops / step + 1e-9), step);
  const double lx_min = std::log2(samples.front().block_size);
  const double lx_max = std::log2(samples.back().block_size);

  const auto x_of = [&](double block) {
    return kPlotX0 + (std::log2(block) - lx_min) / (lx_max - lx_min) *
                         (kPlotX1 - kPlotX0);
  };
  const auto y_of = [&](double gflops) {
    return kPlotY1 - gflops / y_max * (kPlotY1 - kPlotY0);
  };

  std::vector<std::string> parts;

  parts.push_back(std::format(
      R"(<svg xmlns="http://www.w3.org/2000/svg" width="{:.0f}" height="{:.0f})"
      R"(" viewBox="0 0 {:.0f} {:.0f}" font-family="monospace">)",
      kWidth, kHeight, kWidth, kHeight));
  parts.push_back(
      std::format(R"(<rect width="{:.0f}" height="{:.0f}" fill="#ffffff"/>)",
                  kWidth, kHeight));
  parts.push_back(std::format(
      R"(<text x="{:.0f}" y="36" text-anchor="middle" font-size="17" )"
      R"(font-weight="600" fill="#111827">{}</text>)",
      kWidth / 2, escape_xml(title)));
  parts.push_back(std::format(
      R"(<rect x="{:.1f}" y="{:.1f}" width="{:.1f}" height="{:.1f}" )"
      R"(fill="#ffffff" stroke="#d1d5db"/>)",
      kPlotX0, kPlotY0, kPlotX1 - kPlotX0, kPlotY1 - kPlotY0));

  const int y_decimals = step >= 1.0 ? 0 : (step >= 0.1 ? 1 : 2);

  for (double g = 0.0; g <= y_max + 1e-9; g += step) {
    const double y = y_of(g);
    const auto label = std::vformat(
        y_decimals == 0 ? "{:.0f}" : (y_decimals == 1 ? "{:.1f}" : "{:.2f}"),
        std::make_format_args(g));
    parts.push_back(
        std::format(R"(<line x1="{:.1f}" y1="{:.1f}" x2="{:.1f}" y2="{:.1f}" )"
                    R"(stroke="#e5e7eb" stroke-width="1"/>)",
                    kPlotX0, y, kPlotX1, y));
    parts.push_back(std::format(
        R"(<text x="{:.1f}" y="{:.1f}" text-anchor="end" font-size="12" )"
        R"(fill="#6b7280">{}</text>)",
        kPlotX0 - 8, y + 4, label));
  }

  for (const auto &sample : samples) {
    const double x = x_of(sample.block_size);
    parts.push_back(
        std::format(R"(<line x1="{:.1f}" y1="{:.1f}" x2="{:.1f}" y2="{:.1f}" )"
                    R"(stroke="#f3f4f6" stroke-width="1"/>)",
                    x, kPlotY0, x, kPlotY1));
    parts.push_back(std::format(
        R"(<text x="{:.1f}" y="{:.1f}" text-anchor="middle" font-size="12" )"
        R"(fill="#6b7280">{:.0f}</text>)",
        x, kPlotY1 + 24, sample.block_size));
  }

  for (const auto &boundary : boundaries) {
    if (std::log2(boundary.block) < lx_min ||
        std::log2(boundary.block) > lx_max) {
      continue;
    }

    const double x = x_of(boundary.block);
    parts.push_back(std::format(
        R"(<line x1="{:.1f}" y1="{:.1f}" x2="{:.1f}" y2="{:.1f}" )"
        R"(stroke="{}" stroke-width="1.5" stroke-dasharray="6 4"/>)",
        x, kPlotY0, x, kPlotY1, boundary.color));

    const bool near_right_edge = x > kPlotX1 - 220;
    parts.push_back(std::format(
        R"(<text x="{:.1f}" y="{:.1f}" text-anchor="{}" font-size="12" )"
        R"(fill="{}">{}</text>)",
        near_right_edge ? x - 6 : x + 6, kPlotY0 + 16,
        near_right_edge ? "end" : "start", boundary.color,
        escape_xml(boundary.label)));
  }

  std::string points;
  for (const auto &sample : samples) {
    points += std::format("{:.1f},{:.1f} ", x_of(sample.block_size),
                          y_of(sample.gflops));
  }
  parts.push_back(std::format(
      R"(<polyline points="{}" fill="none" stroke="#2563eb" stroke-width="2"/>)",
      points));

  for (const auto &sample : samples) {
    parts.push_back(
        std::format(R"(<circle cx="{:.1f}" cy="{:.1f}" r="4" fill="#2563eb"/>)",
                    x_of(sample.block_size), y_of(sample.gflops)));
  }

  const auto best = std::ranges::max_element(samples, {}, &Sample::gflops);
  const double best_x = x_of(best->block_size);
  const double best_y = y_of(best->gflops);
  const bool label_below_point = best_y - 16 < kPlotY0 + 14;
  parts.push_back(std::format(
      R"(<circle cx="{:.1f}" cy="{:.1f}" r="6" fill="none" stroke="#dc2626" )"
      R"(stroke-width="2"/>)",
      best_x, best_y));
  parts.push_back(std::format(
      R"(<text x="{:.1f}" y="{:.1f}" text-anchor="middle" font-size="13" )"
      R"(font-weight="600" fill="#dc2626">best: bs={:.0f}, {:.2f} GFLOPS</text>)",
      std::clamp(best_x, kPlotX0 + 90, kPlotX1 - 90),
      label_below_point ? best_y + 28 : best_y - 16, best->block_size,
      best->gflops));

  parts.push_back(std::format(
      R"(<text x="{:.1f}" y="{:.1f}" text-anchor="middle" font-size="13" )"
      R"(fill="#374151">GFLOPS</text>)",
      26.0, (kPlotY0 + kPlotY1) / 2));
  parts.push_back(std::format(
      R"(<text x="{:.1f}" y="{:.1f}" text-anchor="middle" font-size="13" )"
      R"(fill="#374151">Block size</text>)",
      (kPlotX0 + kPlotX1) / 2, kHeight - 18));
  parts.push_back(std::format(
      R"(<text x="{:.1f}" y="{:.1f}" text-anchor="end" font-size="12" )"
      R"(font-style="italic" fill="#6b7280">B+C tile working set = 2 x bs^2 x 4 B</text>)",
      kPlotX1 - 8, kPlotY1 - 10));

  parts.push_back("</svg>");

  std::string svg;
  for (const auto &part : parts) {
    svg += part;
    svg += '\n';
  }

  return svg;
}

std::expected<PlotOptions, std::string> parse_plot_options(int argc,
                                                           char **argv) {
  PlotOptions options;

  const auto usage = std::string{"Usage: cpu_tiling_plot [input.csv|-] "
                                 "[output.svg|-] [--l1d bytes] [--l2 bytes] "
                                 "[--title text] [--no-detect-caches]"};

  for (int i = 1; i < argc; ++i) {
    const std::string_view argument{argv[i]};

    if (argument == "--l1d" || argument == "--l2") {
      if (i + 1 >= argc) {
        return std::unexpected{std::format("{} requires a value.", argument)};
      }

      const auto bytes = parse_double(argv[++i]);
      if (!bytes || *bytes <= 0.0) {
        return std::unexpected{
            std::format("{} must be a positive number of bytes.", argument)};
      }

      if (argument == "--l1d") {
        options.l1d_bytes = *bytes;
      } else {
        options.l2_bytes = *bytes;
      }
    } else if (argument == "--title") {
      if (i + 1 >= argc) {
        return std::unexpected{"--title requires a value."};
      }
      options.title = argv[++i];
    } else if (argument == "--detect-caches") {
      options.detect_caches = true;
    } else if (argument == "--no-detect-caches") {
      options.detect_caches = false;
    } else if (argument.starts_with("--")) {
      return std::unexpected{
          std::format("Unknown option: {}\n{}", argument, usage)};
    } else if (options.input.empty()) {
      options.input = argument;
    } else if (options.output.empty()) {
      options.output = argument;
    } else {
      return std::unexpected{usage};
    }
  }

  return options;
}

} // namespace

int main(int argc, char **argv) {
  auto options = parse_plot_options(argc, argv);

  if (!options) {
    std::println(stderr, "{}", options.error());
    return EXIT_FAILURE;
  }

  if (options->detect_caches) {
    const auto caches = read_cache_hierarchy();
    const CacheLevel *l1d = find_data_cache(caches, 1);
    const CacheLevel *l2 = find_data_cache(caches, 2);

    if (l1d != nullptr && options->l1d_bytes == 0.0) {
      options->l1d_bytes = l1d->bytes;
    }
    if (l2 != nullptr && options->l2_bytes == 0.0) {
      options->l2_bytes = l2->bytes;
    }
  }

  std::vector<CacheBoundary> boundaries;
  if (options->l1d_bytes > 0.0) {
    boundaries.push_back(
        CacheBoundary{std::sqrt(options->l1d_bytes / 8.0),
                      std::format("L1d fit: 2 x bs^2 x 4B = {}",
                                  format_bytes(options->l1d_bytes)),
                      "#059669"});
  }
  if (options->l2_bytes > 0.0) {
    boundaries.push_back(
        CacheBoundary{std::sqrt(options->l2_bytes / 8.0),
                      std::format("L2 fit: 2 x bs^2 x 4B = {}",
                                  format_bytes(options->l2_bytes)),
                      "#d97706"});
  }

  std::ifstream input_file;
  std::istream *input = &std::cin;

  if (!options->input.empty() && options->input != "-") {
    input_file.open(options->input);

    if (!input_file) {
      std::println(stderr, "Cannot open input file: {}", options->input);
      return EXIT_FAILURE;
    }

    input = &input_file;
  }

  const auto samples = load_samples(*input);

  if (samples.size() < 2 ||
      samples.front().block_size == samples.back().block_size) {
    std::println(stderr,
                 "Need at least two rows with distinct block sizes in the "
                 "CSV input.");
    return EXIT_FAILURE;
  }

  const std::string svg = make_svg(samples, options->title, boundaries);

  if (options->output.empty() || options->output == "-") {
    std::cout << svg;
    return 0;
  }

  std::ofstream output_file{options->output};

  if (!output_file) {
    std::println(stderr, "Cannot open output file: {}", options->output);
    return EXIT_FAILURE;
  }

  output_file << svg;

  std::println(stderr, "Wrote {} ({} block sizes, best bs={:.0f}).",
               options->output, samples.size(),
               std::ranges::max(samples, {}, &Sample::gflops).block_size);

  return 0;
}
