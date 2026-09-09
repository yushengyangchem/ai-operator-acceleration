set shell := ["bash", "-cu"]

gemm_orders := "ijk ikj jik jki kij kji"

# List available recipes.
default:
    @just --list

# Configure the CMake build directory and refresh the clangd compile-database link.
configure:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    ln -sfn build/compile_commands.json compile_commands.json

# Like configure, but compile GEMM kernels with AVX2+FMA (cached in build/;
# revert with: cmake -S . -B build -DGEMM_USE_AVX2=OFF). Explicit ISA flags
# instead of -march=native because the Nix devshell strips native flags.
configure-avx2:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGEMM_USE_AVX2=ON
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

# Run one built benchmark binary.
# Usage: just run cpu_gemm_ikj [n]; just run cpu_gemm_tiled [n] [block|sweep]
run target n="1024" *args: build
    ./build/bin/{{ target }} "{{ n }}" {{ args }}

# Run all six loop orders. Usage: just benchmark-orders 1024
benchmark-orders n="1024": build
    #!/usr/bin/env bash
    set -euo pipefail

    for order in {{ gemm_orders }}; do
        echo
        echo "--- ${order^^} ---"
        "./build/bin/cpu_gemm_${order}" "{{ n }}"
    done

# Profile cache behavior of one built benchmark binary with perf.
# Usage: just perf cpu_gemm_ijk [n] [repetitions]; just perf cpu_gemm_tiled [n] [repetitions] [block]
perf target n="1024" repetitions="5" *args: build
    perf stat -r "{{ repetitions }}" -e cache-references,cache-misses \
        ./build/bin/{{ target }} "{{ n }}" {{ args }}

# Sweep block sizes 8..512 and write the CSV result plus the SVG plot
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
