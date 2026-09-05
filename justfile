set shell := ["bash", "-cu"]

loop_orders := "ijk ikj jik jki kij kji"

# List available recipes.
default:
    @just --list

# Configure the CMake build directory and refresh the clangd compile-database link.
configure:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    ln -sfn build/compile_commands.json compile_commands.json

# Configure and compile all GEMM implementations in Release mode.
build: configure
    cmake --build build --parallel

# Remove compiled artifacts while keeping the CMake configuration.
clean:
    @if [[ -d build ]]; then cmake --build build --target clean; else echo "Nothing to clean."; fi

# Remove the complete CMake build directory, including its configuration cache.
distclean:
    @if [[ -d build ]]; then cmake -E remove_directory build; fi
    rm -f compile_commands.json

# Build and run the smoke tests.
test: build
    ctest --test-dir build --output-on-failure

# Run one loop order. Usage: just run ikj 1024
run order="ijk" n="1024": build
    ./build/bin/cpu_gemm_{{ order }} "{{ n }}"

# Run all six loop orders. Usage: just benchmark 1024
benchmark n="1024": build
    #!/usr/bin/env bash
    set -euo pipefail

    for order in {{ loop_orders }}; do
        echo
        echo "--- ${order^^} ---"
        "./build/bin/cpu_gemm_${order}" "{{ n }}"
    done

# Compare cache behavior of IJK and IKJ. Usage: just perf 1024 5
perf n="1024" repetitions="5": build
    perf stat -r "{{ repetitions }}" -e cache-references,cache-misses \
        ./build/bin/cpu_gemm_ijk "{{ n }}"
    perf stat -r "{{ repetitions }}" -e cache-references,cache-misses \
        ./build/bin/cpu_gemm_ikj "{{ n }}"

# Run the tiled GEMM with one block size. Usage: just tiling 1024 64
tiling n="1024" block="64": build
    ./build/bin/cpu_gemm_tiled "{{ n }}" "{{ block }}"

# Sweep block sizes 8..128 and write the CSV result plus the SVG plot
# into the build directory. Usage: just tiling-sweep 1024
tiling-sweep n="1024": build
    #!/usr/bin/env bash
    set -euo pipefail

    csv="build/tiling_sweep_{{ n }}.csv"
    svg="build/tiling_sweep_{{ n }}.svg"

    ./build/bin/cpu_gemm_tiled "{{ n }}" sweep > "${csv}"
    ./build/bin/cpu_tiling_plot "${csv}" "${svg}" \
        --title "Tiled GEMM (N={{ n }}): block size vs GFLOPS"

    echo "Wrote ${csv} and ${svg}"

# Plot an existing sweep CSV. Usage: just tiling-plot build/tiling_sweep_1024.csv
tiling-plot csv: build
    ./build/bin/cpu_tiling_plot "{{ csv }}" "{{ without_extension(csv) }}.svg"

# Compare cache behavior of two block sizes. Usage: just tiling-perf 1024 32 128
tiling-perf n="1024" small="32" large="128": build
    perf stat -r 3 -e cache-references,cache-misses \
        ./build/bin/cpu_gemm_tiled "{{ n }}" "{{ small }}"
    perf stat -r 3 -e cache-references,cache-misses \
        ./build/bin/cpu_gemm_tiled "{{ n }}" "{{ large }}"
