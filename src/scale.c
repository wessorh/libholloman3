/* scale.c — dispatch + portable scalar fallback. */

#include "holloman3.h"

/* Forward declarations of the SIMD backends (each built conditionally). */
void holloman3_vscale_avx2(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size);
void holloman3_vscale_sse41(uint8_t *dst, const uint8_t *src,
                            const int32_t *coeffs, size_t size);
void holloman3_vscale_neon(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size);
void holloman3_hscale_avx2(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size);
void holloman3_hscale_sse41(uint8_t *dst, const uint8_t *src,
                            const int32_t *coeffs, size_t size);
void holloman3_hscale_neon(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size);

static uint8_t u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

#define BITS 14
#define ROUND_OFFSET (1 << (BITS - 1))

static void vscale_scalar(uint8_t *dst, const uint8_t *src,
                          const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++) {
        for (size_t x = 0; x < size; x++) {
            int acc = 0;
            for (size_t i = 0; i < size; i++)
                acc += (int)src[i * size + x] * (int)coeffs[y * size + i];
            dst[y * size + x] = u8((acc + ROUND_OFFSET) >> BITS);
        }
    }
}

static void hscale_scalar(uint8_t *dst, const uint8_t *src,
                          const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++) {
        for (size_t x = 0; x < 4; x++) {
            int acc = 0;
            for (size_t i = 0; i < size; i++)
                acc += (int)src[y * size + i] * (int)coeffs[x * size + i];
            dst[y * 4 + x] = u8((acc + ROUND_OFFSET) >> BITS);
        }
    }
}

void holloman3_vscale(uint8_t *dst, const uint8_t *src,
                      const int32_t *coeffs, size_t size) {
    if (size >= 8) {
#if defined(__x86_64__) || defined(_M_X64)
        if (holloman3_has_avx2())
            holloman3_vscale_avx2(dst, src, coeffs, size);
        else
            holloman3_vscale_sse41(dst, src, coeffs, size);
#elif defined(__aarch64__) || defined(_M_ARM64)
        holloman3_vscale_neon(dst, src, coeffs, size);
#else
        vscale_scalar(dst, src, coeffs, size);
#endif
        return;
    }
    vscale_scalar(dst, src, coeffs, size);
}

void holloman3_hscale(uint8_t *dst, const uint8_t *src,
                      const int32_t *coeffs, size_t size) {
    if (size >= 8) {
#if defined(__x86_64__) || defined(_M_X64)
        if (holloman3_has_avx2())
            holloman3_hscale_avx2(dst, src, coeffs, size);
        else
            holloman3_hscale_sse41(dst, src, coeffs, size);
#elif defined(__aarch64__) || defined(_M_ARM64)
        holloman3_hscale_neon(dst, src, coeffs, size);
#else
        hscale_scalar(dst, src, coeffs, size);
#endif
        return;
    }
    hscale_scalar(dst, src, coeffs, size);
}
