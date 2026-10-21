/* scale_avx2.c — AVX2 (256-bit) scaling kernels via compiler intrinsics.
 * Compiles on x86-64 with gcc/clang/MSVC (Linux and Windows). */

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

#define BITS 14
#define ROUND_OFFSET (1 << (BITS - 1))

void holloman3_vscale_avx2(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size) {
    const __m256i round = _mm256_set1_epi32(ROUND_OFFSET);
    const __m256i max255 = _mm256_set1_epi32(255);
    const __m256i zero = _mm256_setzero_si256();

    for (size_t y = 0; y < 4; y++) {
        uint8_t *dst_row = dst + y * size;
        const int32_t *coeff_row = coeffs + y * size;
        for (size_t x = 0; x < size; x += 8) {
            __m256i acc = zero;
            const uint8_t *src_ptr = src + x;
            const int32_t *coeff_ptr = coeff_row;
            for (size_t i = 0; i < size; i++) {
                __m256i pixels = _mm256_cvtepu8_epi32(
                    _mm_loadl_epi64((const __m128i *)src_ptr));
                __m256i coeff = _mm256_set1_epi32(*coeff_ptr);
                acc = _mm256_add_epi32(acc, _mm256_mullo_epi32(pixels, coeff));
                src_ptr += size;
                coeff_ptr++;
            }
            acc = _mm256_add_epi32(acc, round);
            acc = _mm256_srai_epi32(acc, BITS);
            acc = _mm256_max_epi32(acc, zero);
            acc = _mm256_min_epi32(acc, max255);

            __m128i lo = _mm256_castsi256_si128(acc);
            __m128i hi = _mm256_extracti128_si256(acc, 1);
            __m128i packed = _mm_packus_epi32(lo, hi);
            packed = _mm_packus_epi16(packed, packed);
            _mm_storel_epi64((__m128i *)(dst_row + x), packed);
        }
    }
}

void holloman3_hscale_avx2(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++) {
        uint8_t *dst_row = dst + y * 4;
        const uint8_t *src_row = src + y * size;
        for (size_t x = 0; x < 4; x++) {
            const int32_t *c = coeffs + x * size;
            __m256i acc = _mm256_setzero_si256();
            for (size_t i = 0; i < size; i += 8) {
                __m256i pixels = _mm256_cvtepu8_epi32(
                    _mm_loadl_epi64((const __m128i *)(src_row + i)));
                __m256i coeffs8 = _mm256_loadu_si256((const __m256i *)(c + i));
                acc = _mm256_add_epi32(acc, _mm256_mullo_epi32(pixels, coeffs8));
            }

            __m128i lo = _mm256_castsi256_si128(acc);
            __m128i hi = _mm256_extracti128_si256(acc, 1);
            __m128i sum = _mm_add_epi32(lo, hi);
            sum = _mm_hadd_epi32(sum, sum);
            sum = _mm_hadd_epi32(sum, sum);
            int pix = _mm_cvtsi128_si32(sum);

            pix = (pix + ROUND_OFFSET) >> BITS;
            if (pix < 0) pix = 0;
            if (pix > 255) pix = 255;
            dst_row[x] = (uint8_t)pix;
        }
    }
}
