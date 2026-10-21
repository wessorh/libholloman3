/* arch.c — runtime CPU feature detection (x86 AVX2). */

#include "holloman3.h"

#if defined(__x86_64__) || defined(_M_X64)

#if defined(_MSC_VER)
#  include <intrin.h>
static void cpuid_(int leaf, int subleaf, unsigned int out[4]) {
    __cpuidex((int *)out, leaf, subleaf);
}
static unsigned long long xgetbv_(unsigned int xcr) {
    return _xgetbv(xcr);
}
#else
#  include <cpuid.h>
static void cpuid_(int leaf, int subleaf, unsigned int out[4]) {
    __cpuid_count(leaf, subleaf, out[0], out[1], out[2], out[3]);
}
static unsigned long long xgetbv_(unsigned int xcr) {
    unsigned int eax, edx;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
    return ((unsigned long long)edx << 32) | eax;
}
#endif

int holloman3_has_avx2(void) {
    unsigned int r[4];

    /* CPUID leaf 1: OSXSAVE (ECX bit 27) and AVX (ECX bit 28). */
    cpuid_(1, 0, r);
    if (!(r[2] & (1u << 27)) || !(r[2] & (1u << 28)))
        return 0;

    /* XCR0: require SSE (bit 1) and YMM (bit 2). */
    if ((xgetbv_(0) & 0x6u) != 0x6u)
        return 0;

    /* CPUID leaf 7 subleaf 0: AVX2 (EBX bit 5). */
    cpuid_(7, 0, r);
    return (r[1] & (1u << 5)) != 0;
}

#else

int holloman3_has_avx2(void) {
    return 0;
}

#endif
