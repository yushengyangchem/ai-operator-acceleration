# AI Operator Acceleration

A hands-on project for learning AI operator optimization from first principles.

## Requirements

- A C++23-compatible compiler
- CMake 3.16 or newer

The included `shell.nix` provides the required development tools. Enter the
environment with `nix-shell`, or use `direnv allow` when direnv is installed.

## Usage

Build all six GEMM implementations in Release mode:

```bash
just build
```

Remove compiled artifacts while keeping the CMake configuration, or remove the
entire build directory for a clean reconfiguration:

```bash
just clean
just distclean
```

Run the smoke tests:

```bash
just test
```

Run one implementation, optionally specifying the matrix size (and block size
or `sweep` for the tiled GEMM):

```bash
just run cpu_gemm_ijk 1024
just run cpu_gemm_ikj 1024
just run cpu_gemm_tiled 1024 64
```

Run all six loop orders:

```bash
just benchmark-orders 1024
```

Compare cache references and cache misses for the IJK and IKJ implementations:

```bash
just perf cpu_gemm_ijk 1024 5
just perf cpu_gemm_ikj 1024 5
```

Sweep block sizes from 8 to 128 and plot GFLOPS per block size to find the
optimum for your machine (the sweep writes `build/tiling_sweep_<N>.csv` and
`.svg`), then compare cache behavior of two block sizes:

```bash
just tiling-sweep 1024
just perf cpu_gemm_tiled 1024 5 32
just perf cpu_gemm_tiled 1024 5 128
```

Run `just` without arguments to list all available recipes. The default matrix
size is `1024`, and the default number of `perf` repetitions is `5`.

## License

This project is licensed under the [MIT License](LICENSE).
