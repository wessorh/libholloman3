/* test_scale.c — C99 test suite for the libholloman3 SIMD scaling kernels.
 *
 * Validates holloman3_vscale / holloman3_hscale against a portable scalar
 * reference for several power-of-two sizes, byte-for-byte. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "holloman3.h"

#define BITS 14
#define ROUND_OFFSET (1 << (BITS - 1))

static uint8_t u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static void vscale_ref(uint8_t *dst, const uint8_t *src,
                       const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++)
        for (size_t x = 0; x < size; x++) {
            int acc = 0;
            for (size_t i = 0; i < size; i++)
                acc += (int)src[i * size + x] * (int)coeffs[y * size + i];
            dst[y * size + x] = u8((acc + ROUND_OFFSET) >> BITS);
        }
}

static void hscale_ref(uint8_t *dst, const uint8_t *src,
                       const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++)
        for (size_t x = 0; x < 4; x++) {
            int acc = 0;
            for (size_t i = 0; i < size; i++)
                acc += (int)src[y * size + i] * (int)coeffs[x * size + i];
            dst[y * 4 + x] = u8((acc + ROUND_OFFSET) >> BITS);
        }
}

static int test_size(size_t size) {
    uint8_t *src = malloc(size * size);
    int32_t *coeffs = malloc(4 * size * sizeof(int32_t));
    uint8_t *got = malloc(size * 4);
    uint8_t *want = malloc(size * 4);
    if (!src || !coeffs || !got || !want) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < size * size; i++)
        src[i] = (uint8_t)((i * 7 + 13) & 0xFF);
    for (size_t i = 0; i < 4 * size; i++)
        coeffs[i] = (int32_t)((i * 31 + 5) % 4096 - 2048);

    holloman3_vscale(got, src, coeffs, size);
    vscale_ref(want, src, coeffs, size);
    if (memcmp(got, want, size * 4) != 0) {
        printf("vscale size=%zu MISMATCH\n", size);
        return 1;
    }

    holloman3_hscale(got, src, coeffs, size);
    hscale_ref(want, src, coeffs, size);
    if (memcmp(got, want, 16) != 0) {
        printf("hscale size=%zu MISMATCH\n", size);
        return 1;
    }

    printf("size=%zu OK\n", size);
    free(src);
    free(coeffs);
    free(got);
    free(want);
    return 0;
}

int main(void) {
    int failures = 0;
    size_t sizes[] = {8, 16, 64, 128, 512, 1024};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++)
        failures += test_size(sizes[i]);

    printf("has_avx2=%d\n", holloman3_has_avx2());

    if (failures) {
        printf("FAIL: %d size(s) mismatched\n", failures);
        return 1;
    }
    printf("PASS\n");
    return 0;
}
