/* holloman3.h — public C API for the libholloman3 SIMD scaling kernels.
 *
 * These kernels implement the two hot, byte-identical scaling passes of the
 * holloman3 perceptual fingerprint's Lanczos-3 downsampling. The functions are
 * specialized for the fingerprint pipeline:
 *
 *   vscale: height = 4, offsets = 0, width = taps = src_pitch = dst_pitch = size
 *   hscale: height = 4, width = 4, offsets = 0, taps = src_pitch = size,
 *           dst_pitch = 4
 *
 * `size` is a power of two (the 2^order grid dimension). Results are
 * byte-identical across the AVX2, SSE4.1, NEON, and scalar backends.
 *
 * Consumable from C, C++, and Rust (via FFI). */
#ifndef HOLLOMAN3_H
#define HOLLOMAN3_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  if defined(HOLLOMAN3_BUILD)
#    define HOLLOMAN3_API __declspec(dllexport)
#  else
#    define HOLLOMAN3_API __declspec(dllimport)
#  endif
#else
#  define HOLLOMAN3_API __attribute__((visibility("default")))
#endif

/* Reports whether the CPU supports AVX2 (with OSXSAVE enabled). Always 0 on
 * non-x86-64 targets. */
HOLLOMAN3_API int holloman3_has_avx2(void);

/* Vertical Lanczos-3 scaling pass.
 *   dst    size*4 bytes (row-major, pitch = size)
 *   src    size*size bytes (row-major, pitch = size)
 *   coeffs 4*size int32 (coeffs[y*size + i]) */
HOLLOMAN3_API void holloman3_vscale(uint8_t *dst, const uint8_t *src,
                                    const int32_t *coeffs, size_t size);

/* Horizontal Lanczos-3 scaling pass.
 *   dst    16 bytes (4x4, row-major, pitch = 4)
 *   src    size*4 bytes (row-major, pitch = size)
 *   coeffs 4*size int32 (coeffs[x*size + i]) */
HOLLOMAN3_API void holloman3_hscale(uint8_t *dst, const uint8_t *src,
                                    const int32_t *coeffs, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* HOLLOMAN3_H */
