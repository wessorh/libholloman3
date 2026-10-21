/* scale_sse41.c — 128-bit SSE4.1 scaling kernels via compiler intrinsics.
 * Runs on AVX1-only CPUs. Compiles on x86-64 with gcc/clang/MSVC. */

#include <smmintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BITS 14
#define ROUND_OFFSET (1 << (BITS - 1))

static inline uint32_t load_u32(const void *p) {
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static inline void store_u32(void *p, uint32_t v) {
    memcpy(p, &v, sizeof(v));
}

void holloman3_vscale_sse41(uint8_t *dst, const uint8_t *src,
                            const int32_t *coeffs, size_t size) {
    const __m128i round = _mm_set1_epi32(ROUND_OFFSET);
    const __m128i max255 = _mm_set1_epi32(255);
    const __m128i zero = _mm_setzero_si128();

    for (size_t y = 0; y < 4; y++) {
        uint8_t *dst_row = dst + y * size;
        const int32_t *coeff_row = coeffs + y * size;
        for (size_t x = 0; x < size; x += 4) {
            __m128i acc = zero;
            const uint8_t *src_ptr = src + x;
            const int32_t *coeff_ptr = coeff_row;
            for (size_t i = 0; i < size; i++) {
                __m128i pixels = _mm_cvtepu8_epi32(
                    _mm_cvtsi32_si128((int)load_u32(src_ptr)));
                __m128i coeff = _mm_set1_epi32(*coeff_ptr);
                acc = _mm_add_epi32(acc, _mm_mullo_epi32(pixels, coeff));
                src_ptr += size;
                coeff_ptr++;
            }
            acc = _mm_add_epi32(acc, round);
            acc = _mm_srai_epi32(acc, BITS);
            acc = _mm_max_epi32(acc, zero);
            acc = _mm_min_epi32(acc, max255);

            __m128i packed = _mm_packus_epi32(acc, acc);
            packed = _mm_packus_epi16(packed, packed);
            store_u32(dst_row + x, (uint32_t)_mm_cvtsi128_si32(packed));
        }
    }
}

void holloman3_hscale_sse41(uint8_t *dst, const uint8_t *src,
                            const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++) {
        uint8_t *dst_row = dst + y * 4;
        const uint8_t *src_row = src + y * size;
        for (size_t x = 0; x < 4; x++) {
            const int32_t *c = coeffs + x * size;
            __m128i acc = _mm_setzero_si128();
            for (size_t i = 0; i < size; i += 4) {
                __m128i pixels = _mm_cvtepu8_epi32(
                    _mm_cvtsi32_si128((int)load_u32(src_row + i)));
                __m128i coeffs4 = _mm_loadu_si128((const __m128i *)(c + i));
                acc = _mm_add_epi32(acc, _mm_mullo_epi32(pixels, coeffs4));
            }

            __m128i sum = _mm_hadd_epi32(acc, acc);
            sum = _mm_hadd_epi32(sum, sum);
            int pix = _mm_cvtsi128_si32(sum);

            pix = (pix + ROUND_OFFSET) >> BITS;
            if (pix < 0) pix = 0;
            if (pix > 255) pix = 255;
            dst_row[x] = (uint8_t)pix;
        }
    }
}
