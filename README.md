# AI Operator Acceleration

A hands-on project for learning AI operator optimization from first principles.

## Requirements

- A C++23-compatible compiler
- CMake 3.16 or newer

The included `flake.nix` provides the required development tools. Enter the
environment with `nix develop` (CPU only) or `nix develop .#cuda`, or use
`direnv allow` when direnv is installed.

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

Sweep block sizes from 8 to 512 and plot GFLOPS per block size to find the
optimum for your machine (the sweep writes `build/tiling_sweep_<N>.csv` and
`.svg`), then compare cache behavior of two block sizes:

```bash
just tiling-sweep 1024
just perf cpu_gemm_tiled 1024 5 32
just perf cpu_gemm_tiled 1024 5 128
```

Run `just` without arguments to list all available recipes. The default matrix
size is `1024`, and the default number of `perf` repetitions is `5`.

## Vectorization notes

All kernels are portable by default (baseline SSE2, built into `build/`). The
AVX2+FMA variants live in a separate tree (`build-avx2/`) so both can coexist:

```bash
just benchmark-orders-avx2 1024
just tiling-sweep-avx2 1024
```

Why this matters and what to watch out for:

- **Index form blocks auto-vectorization.** Writing the inner-loop loads as
  `A[i * N + k]` (an index built from two loop variables) fails GCC's
  data-reference analysis, and the whole kernel compiles to scalar code
  (`g++ -fopt-info-vec-missed` reports `data ref analysis failed`). Hoisting
  row pointers (`crow[j] += aik * brow[j]`) lets the j loop auto-vectorize;
  measured on an Intel N100 this alone is worth ~2x, and AVX2+FMA ~3.5x.
- **Aliasing versioning.** Even with row pointers, GCC emits a runtime
  C/B alias check because both are `float *`; `__restrict` pointers would
  remove it.
- **Dot-product orders (IJK, JIK) cannot vectorize as written.** The k loop
  is a floating-point reduction: vectorizing reorders the additions, which is
  only allowed under `-ffast-math` (we avoid it to keep results
  reproducible), and B is read with stride N anyway.
- **Column orders (JKI, KJI) are layout-bound.** The inner i loop strides N
  through memory: one cache line per element, nothing contiguous to load into
  a vector register.
- **FMA changes low-order bits.** `-ffp-contract=fast` is GCC's C++ default,
  so vectorized kernels fuse multiply-add and `C[0]` shifts slightly
  (e.g. `-6.985047` -> `-6.985044`); this is rounding, not an error.
- **Tiling only pays after vectorization.** On scalar kernels every loop
  order hits the same compute ceiling (~3 GFLOPS here), so blocking adds
  overhead without reducing memory traffic that matters. The block-size
  sweep becomes meaningful once the kernel is fast enough to be
  memory-bound.
- **Nix devshell caveat.** `NIX_ENFORCE_NO_NATIVE` silently strips
  `-march=native`; that is why `configure-avx2` passes explicit
  `-mavx2 -mfma` instead.

## License

This project is licensed under the [MIT License](LICENSE).
