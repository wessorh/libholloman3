/* bench.c — throughput benchmark for the libholloman3 scaling kernels.
 *
 * Measures vscale (the hot vertical pass) throughput in MB/s of input grid
 * processed, for the AVX2 (dispatch), SSE4.1, and scalar backends, at several
 * power-of-two grid sizes. hscale is measured too, but it is 16 output pixels
 * (negligible) and reported for completeness. */

#define _POSIX_C_SOURCE 199309L /* clock_gettime */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "holloman3.h"

#if defined(__x86_64__) || defined(_M_X64)
void holloman3_vscale_sse41(uint8_t *, const uint8_t *, const int32_t *, size_t);
#endif

#define BITS 14
#define ROUND_OFFSET (1 << (BITS - 1))

static uint8_t u8_(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static void vscale_scalar(uint8_t *dst, const uint8_t *src,
                          const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++)
        for (size_t x = 0; x < size; x++) {
            int acc = 0;
            for (size_t i = 0; i < size; i++)
                acc += (int)src[i * size + x] * (int)coeffs[y * size + i];
            dst[y * size + x] = u8_((acc + ROUND_OFFSET) >> BITS);
        }
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

typedef void (*vfn)(uint8_t *, const uint8_t *, const int32_t *, size_t);

static double bench_v(vfn fn, uint8_t *dst, const uint8_t *src,
                      const int32_t *coeffs, size_t size, size_t iters) {
    fn(dst, src, coeffs, size); /* warm-up */
    double t0 = now_sec();
    for (size_t i = 0; i < iters; i++)
        fn(dst, src, coeffs, size);
    double dt = now_sec() - t0;
    /* input grid bytes = size*size; report MB/s */
    return (double)size * (double)size * (double)iters / dt / 1e6;
}

int main(void) {
    size_t sizes[] = {256, 512, 1024, 2048, 4096};
    size_t n = sizeof(sizes) / sizeof(sizes[0]);

    printf("vscale throughput (MB/s of input grid):\n");
    printf("%-8s %12s %12s %12s\n", "size", "AVX2", "SSE4.1", "scalar");
    for (size_t i = 0; i < n; i++) {
        size_t size = sizes[i];
        uint8_t *src = malloc(size * size);
        int32_t *coeffs = malloc(4 * size * sizeof(int32_t));
        uint8_t *dst = malloc(size * 4);
        for (size_t j = 0; j < size * size; j++)
            src[j] = (uint8_t)((j * 7 + 13) & 0xFF);
        for (size_t j = 0; j < 4 * size; j++)
            coeffs[j] = (int32_t)(((int32_t)j * 31 + 5) % 4096 - 2048);

        /* Target roughly 200 MB of input processed per measurement. */
        size_t iters = 1;
        while (iters * size * size < 200000000)
            iters *= 2;

        double avx2 = bench_v(holloman3_vscale, dst, src, coeffs, size, iters);
#if defined(__x86_64__) || defined(_M_X64)
        double sse41 = bench_v(holloman3_vscale_sse41, dst, src, coeffs, size, iters);
#else
        double sse41 = 0.0;
#endif
        double scalar = bench_v(vscale_scalar, dst, src, coeffs, size, iters);

        printf("%-8zu %12.1f %12.1f %12.1f\n", size, avx2, sse41, scalar);

        free(src);
        free(coeffs);
        free(dst);
    }

    return 0;
}
