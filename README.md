# libholloman3

SIMD scaling kernels for the holloman3 perceptual fingerprint's Lanczos-3
downsampling, exposed to **Go, C, and Rust** with as little duplication as
possible.

Two hot, byte-identical passes are provided — the vertical (`vscale`) and
horizontal (`hscale`) separable Lanczos-3 downsampling passes — across four
backends:

| backend | Go | C / Rust |
|---|---|---|
| AVX2 (256-bit) | Plan 9 assembly | compiler intrinsics |
| SSE4.1 (128-bit) | Plan 9 assembly | compiler intrinsics |
| NEON (ARM64) | (scalar fallback) | compiler intrinsics |
| scalar | Go | C |

## Layering

- **C library** (`include/holloman3.h`, `src/*.c`) — the canonical native
  implementation, written with **compiler intrinsics** (`immintrin.h`,
  `smmintrin.h`, `arm_neon.h`). The same sources build on **Linux, Windows
  (MSVC/clang-cl), and macOS** for x86-64, and on **aarch64** for NEON — no
  per-OS or per-arch assembly duplication. Rust consumes this via FFI.
- **Go package** (`scale*.go`, `*.s`) — the same kernels in **Plan 9 assembly**
  (Go's assembler dialect), since Go does not consume C intrinsics directly.
  The scalar fallback is shared conceptually (byte-identical) with the C one.

The kernel *algorithm* is duplicated in exactly two dialects (C intrinsics and
Go Plan 9 assembly) — unavoidable given Go's separate assembler — but each
dialect is a single source covering all its targets.

## Build & test

```bash
make              # build libholloman3.a / libholloman3.so (auto-detects arch)
make test         # build + run the C99 test suite (tests/test_scale.c)
make gotest       # run the Go test suite
make cross-neon   # cross-compile aarch64, validate NEON under qemu-user
make cross-windows # cross-compile Windows x86-64, validate under wine
```

Windows (x86-64): compile `src/scale.c src/arch.c src/scale_avx2.c
src/scale_sse41.c` with MSVC or clang-cl (`/O2 /Iinclude`).

Rust (requires a Rust toolchain; `cargo build` runs `make` via `build.rs`):

```bash
cd rust && cargo build
```

## API

```c
int  holloman3_has_avx2(void);
void holloman3_vscale(uint8_t *dst, const uint8_t *src, const int32_t *coeffs, size_t size);
void holloman3_hscale(uint8_t *dst, const uint8_t *src, const int32_t *coeffs, size_t size);
```

See `include/holloman3.h` for the exact buffer contracts.
