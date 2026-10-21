# libholloman3

SIMD scaling kernels for the **holloman3** perceptual fingerprint's Lanczos-3
downsampling, exposed to **Go, C, and Rust** from a single, minimally-duplicated
native code base.

The library provides the two hot, byte-identical passes — the vertical
(`vscale`) and horizontal (`hscale`) separable Lanczos-3 downsampling — across
four backends, validated on Linux, Windows, and aarch64.

---

## How it works

holloman3 is a perceptual fingerprint: it maps a byte buffer onto a 2D grid via
a **Hilbert curve** (a space-filling curve that preserves spatial locality),
downsamples that grid to 4×4 with a separable **Lanczos-3** filter, rotates the
result 90°, and emits a 16-byte digest.

The downsampling is done in two separable passes over the `size × size` grid:

1. **`vscale` (vertical)** — `size×size → size×4`: each of the 4 output rows is
   a weighted sum of the input rows.
2. **`hscale` (horizontal)** — `size×4 → 4×4`: each of the 4×4 output pixels is
   a weighted sum of the intermediate row.

**This library supplies only these two passes.** The caller is responsible for
the surrounding pipeline: mapping the input onto the grid (Hilbert curve),
generating the Lanczos-3 coefficients (see below), and rotating the 4×4 result.

### Fixed-point math

The Lanczos-3 kernel is evaluated once per grid size and quantized to `int32`
coefficients with 14-bit fixed-point scale (sum of coefficients ≈ 16384). Each
output pixel is:

```
sum = Σ input[tap] × coeff[tap]
pixel = clamp((sum + 8192) >> 14, 0, 255)
```

The 14-bit scale, the `+8192` rounding bias, and the arithmetic (not logical)
right shift are what make every backend **byte-identical**.

---

## Backends

| backend | Go | C / Rust |
|---|---|---|
| AVX2 (256-bit, x86-64) | Plan 9 assembly | compiler intrinsics |
| SSE4.1 (128-bit, x86-64) | Plan 9 assembly | compiler intrinsics |
| NEON (aarch64) | scalar fallback | compiler intrinsics |
| scalar (portable) | Go | C |

The C implementation is written in **compiler intrinsics** (`immintrin.h`,
`smmintrin.h`, `arm_neon.h`), so the *same* sources build on **Linux, Windows
(MSVC/clang-cl/MinGW), and aarch64** — no per-OS or per-arch assembly
duplication. Rust consumes the C library via FFI. Go uses its own **Plan 9
assembly** (Go's assembler cannot consume C intrinsics), so the kernel algorithm
exists in exactly two dialects — one C-intrinsics source per backend (shared by
C and Rust) and one Go Plan 9 source per backend.

Runtime dispatch: on x86-64, `holloman3_has_avx2()` (CPUID) selects AVX2 vs
SSE4.1; `size < 8` and non-SIMD targets use the scalar fallback.

---

## Usage

### C

```c
#include "holloman3.h"

uint8_t  dst[16];            /* or size*4 for vscale */
uint8_t  *src = ...;         /* size*size grid */
int32_t  *coeffs = ...;      /* 4*size coefficients */

holloman3_vscale(dst, src, coeffs, size);  /* vertical pass  */
holloman3_hscale(dst, src, coeffs, size);  /* horizontal pass */

if (holloman3_has_avx2()) { /* AVX2 is available */ }
```

Link with `-lholloman3` (shared) or `libholloman3.a` (static). On Windows define
`HOLLOMAN3_STATIC` when linking statically, or link the DLL import library.

### Rust

```rust
use libholloman3_sys::{vscale, hscale, has_avx2};

vscale(&mut dst, &src, &coeffs, size);
hscale(&mut dst, &src, &coeffs, size);
let _ = has_avx2();
```

`cargo build` runs `build.rs`, which invokes `make` and links the static library.

### Go

```go
import "libholloman3"

libholloman3.VScale(dst, src, coeffs, size)  // coeffs []int32
libholloman3.HScale(dst, src, coeffs, size)
libholloman3.HasAVX2()
```

---

## Throughput

Measured on an Intel Xeon Silver 4510 (Sapphire Rapids), `make bench`,
`vscale` throughput in MB/s of input grid processed:

| size (grid) | AVX2 | SSE4.1 | scalar |
|---|---|---|---|
| 256² | 1393 | 738 | 317 |
| 512² | 2452 | 1204 | 307 |
| 1024² | 2765 | 1315 | 287 |
| 2048² | 611 | 312 | 78 |
| 4096² | 623 | 315 | 77 |

Observations:

- **AVX2 is ~2× SSE4.1 and ~9× scalar** for cache-resident sizes (≤ 1024²),
  peaking at ~2.8 GB/s.
- **Throughput collapses at ≥ 2048²** (4 MB grid): the working set exceeds the
  CPU cache and the strided column access becomes DRAM-latency-bound. This is a
  property of the algorithm's memory access, not the kernels.
- The scalar reference also drops at ≥ 2048² for the same reason.

The Go Plan 9 kernels track the same trend (slightly slower than the C compiler
output at equivalent sizes); the C intrinsics benefit from the compiler's
scheduling and use the fast memory-broadcast form of `vpbroadcastd`.

---

## Build & test

```bash
make              # build libholloman3.a / libholloman3.so (auto-detects arch)
make test         # build + run the C99 test suite (validates SIMD vs scalar)
make bench        # throughput benchmark
make gotest       # run the Go test suite
make cross-neon   # cross-compile aarch64, validate NEON under qemu-user
make cross-windows # cross-compile Windows x86-64, validate under wine
```

Cross-validation matrix (all byte-identical):

| target | backends | via |
|---|---|---|
| `make test` (Linux) | AVX2 + SSE4.1 + scalar | native |
| `make cross-neon` (aarch64) | NEON | qemu-aarch64 |
| `make cross-windows` (Win x64) | AVX2 + SSE4.1 | wine |
| `make gotest` | Go Plan 9 | native |
| `cargo test` (rust/) | Rust FFI | native |

---

## API

```c
int  holloman3_has_avx2(void);
void holloman3_vscale(uint8_t *dst, const uint8_t *src, const int32_t *coeffs, size_t size);
void holloman3_hscale(uint8_t *dst, const uint8_t *src, const int32_t *coeffs, size_t size);
```

- **`vscale`** — `dst` is `size*4` bytes (pitch `size`), `src` is `size*size`
  bytes (pitch `size`), `coeffs` is `4*size` `int32` (`coeffs[y*size + i]`).
- **`hscale`** — `dst` is 16 bytes (4×4, pitch 4), `src` is `size*4` bytes
  (pitch `size`), `coeffs` is `4*size` `int32` (`coeffs[x*size + i]`).
- `size` is a power of two (the `2^order` grid dimension). All backends produce
  byte-identical results.

See `include/holloman3.h` for the full contracts.

---

## Layout

```
include/holloman3.h      C API
src/scale.c              dispatch + scalar fallback
src/arch.c               CPUID (x86)
src/scale_avx2.c         AVX2 intrinsics
src/scale_sse41.c        SSE4.1 intrinsics
src/scale_neon.c         NEON intrinsics
tests/test_scale.c       C99 test suite
bench/bench.c            throughput benchmark
rust/                    Rust FFI crate
*.go, *.s                Go package (Plan 9 assembly)
```
