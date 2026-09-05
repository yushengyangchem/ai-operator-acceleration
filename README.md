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

Run one implementation, optionally specifying the matrix size:

```bash
just run ijk 1024
just run ikj 1024
```

Run all six loop orders:

```bash
just benchmark 1024
```

Compare cache references and cache misses for the IJK and IKJ implementations:

```bash
just perf 1024 5
```

Run the blocked (tiled) GEMM with one block size, sweep block sizes from 8 to
128, and plot GFLOPS per block size to find the optimum for your machine (the
sweep writes `build/tiling_sweep_<N>.csv` and `.svg`):

```bash
just tiling 1024 64
just tiling-sweep 1024
just tiling-perf 1024 32 128
```

Run `just` without arguments to list all available recipes. The default matrix
size is `1024`, and the default number of `perf` repetitions is `5`.

## License

This project is licensed under the [MIT License](LICENSE).
