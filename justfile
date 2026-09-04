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
