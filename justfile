set shell := ["bash", "-cu"]

gemm_orders := "ijk ikj jik jki kij kji"

# List available recipes.
default:
    @just --list

# Configure the baseline build tree (build/) and refresh the clangd link.
# GEMM_USE_AVX2 is always passed explicitly so the tree can never inherit a
# stale cached value.
configure:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGEMM_USE_AVX2=OFF
    ln -sfn build/compile_commands.json compile_commands.json

# Configure the AVX2+FMA build tree (build-avx2/), separate from build/ so
# both variants coexist without clobbering each other's CMake cache.
# Explicit ISA flags instead of -march=native because the Nix devshell
# strips native flags.
configure-avx2:
    cmake -S . -B build-avx2 -DCMAKE_BUILD_TYPE=Release -DGEMM_USE_AVX2=ON
    ln -sfn build-avx2/compile_commands.json compile_commands.json

# Configure and compile all GEMM implementations (baseline) in Release mode.
build: configure
    cmake --build build --parallel

# Configure and compile all GEMM implementations (AVX2+FMA) in Release mode.
build-avx2: configure-avx2
    cmake --build build-avx2 --parallel

# Remove baseline artifacts while keeping the CMake configuration.
clean:
    @if [[ -d build ]]; then cmake --build build --target clean; else echo "Nothing to clean."; fi

# Remove AVX2 artifacts while keeping the CMake configuration.
clean-avx2:
    @if [[ -d build-avx2 ]]; then cmake --build build-avx2 --target clean; else echo "Nothing to clean."; fi

# Remove both build trees, including their configuration caches.
distclean:
    @if [[ -d build ]]; then cmake -E remove_directory build; fi
    @if [[ -d build-avx2 ]]; then cmake -E remove_directory build-avx2; fi
    rm -f compile_commands.json

# Build (baseline) and run the smoke tests.
test: build
    ctest --test-dir build --output-on-failure

# Build (AVX2+FMA) and run the smoke tests.
test-avx2: build-avx2
    ctest --test-dir build-avx2 --output-on-failure

# Run one benchmark binary from the baseline tree.
# Usage: just run cpu_gemm_ikj [n]; just run cpu_gemm_tiled [n] [block|sweep]
run target n="1024" *args: build
    ./build/bin/{{ target }} "{{ n }}" {{ args }}

# Run one benchmark binary from the AVX2+FMA tree.
# Usage: just run-avx2 cpu_gemm_ikj [n]; just run-avx2 cpu_gemm_tiled [n] [block|sweep]
run-avx2 target n="1024" *args: build-avx2
    ./build-avx2/bin/{{ target }} "{{ n }}" {{ args }}

# Run all six loop orders (baseline). Usage: just benchmark-orders 1024
benchmark-orders n="1024": build
    #!/usr/bin/env bash
    set -euo pipefail

    for order in {{ gemm_orders }}; do
        echo
        echo "--- ${order^^} ---"
        "./build/bin/cpu_gemm_${order}" "{{ n }}"
    done

# Run all six loop orders (AVX2+FMA). Usage: just benchmark-orders-avx2 1024
benchmark-orders-avx2 n="1024": build-avx2
    #!/usr/bin/env bash
    set -euo pipefail

    for order in {{ gemm_orders }}; do
        echo
        echo "--- ${order^^} ---"
        "./build-avx2/bin/cpu_gemm_${order}" "{{ n }}"
    done

# Profile cache behavior (perf, baseline tree).
# Usage: just perf cpu_gemm_ijk [n] [repetitions]; just perf cpu_gemm_tiled [n] [repetitions] [block]
perf target n="1024" repetitions="5" *args: build
    perf stat -r "{{ repetitions }}" -e cache-references,cache-misses \
        ./build/bin/{{ target }} "{{ n }}" {{ args }}

# Profile cache behavior (perf, AVX2+FMA tree).
# Usage: just perf-avx2 cpu_gemm_ijk [n] [repetitions]; just perf-avx2 cpu_gemm_tiled [n] [repetitions] [block]
perf-avx2 target n="1024" repetitions="5" *args: build-avx2
    perf stat -r "{{ repetitions }}" -e cache-references,cache-misses \
        ./build-avx2/bin/{{ target }} "{{ n }}" {{ args }}

# Sweep block sizes 8..512 (baseline) and write the CSV result plus the SVG
# plot into build/. Usage: just tiling-sweep 1024
tiling-sweep n="1024": build
    #!/usr/bin/env bash
    set -euo pipefail

    csv="build/tiling_sweep_{{ n }}.csv"
    svg="build/tiling_sweep_{{ n }}.svg"

    ./build/bin/cpu_gemm_tiled "{{ n }}" sweep > "${csv}"
    ./build/bin/cpu_tiling_plot "${csv}" "${svg}" \
        --title "Tiled GEMM (N={{ n }}): block size vs GFLOPS"

    echo "Wrote ${csv} and ${svg}"

# Sweep block sizes 8..512 (AVX2+FMA) and write the CSV result plus the SVG
# plot into build-avx2/. Usage: just tiling-sweep-avx2 1024
tiling-sweep-avx2 n="1024": build-avx2
    #!/usr/bin/env bash
    set -euo pipefail

    csv="build-avx2/tiling_sweep_{{ n }}.csv"
    svg="build-avx2/tiling_sweep_{{ n }}.svg"

    ./build-avx2/bin/cpu_gemm_tiled "{{ n }}" sweep > "${csv}"
    ./build-avx2/bin/cpu_tiling_plot "${csv}" "${svg}" \
        --title "Tiled GEMM (N={{ n }}, AVX2+FMA): block size vs GFLOPS"

    echo "Wrote ${csv} and ${svg}"

# Plot an existing sweep CSV. Usage: just tiling-plot build/tiling_sweep_1024.csv
tiling-plot csv: build
    ./build/bin/cpu_tiling_plot "{{ csv }}" "{{ without_extension(csv) }}.svg"
