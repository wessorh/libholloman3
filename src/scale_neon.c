/* scale_neon.c — ARM64 NEON scaling kernels via compiler intrinsics.
 * Compiled with arm_neon.h (aarch64). Untested on real hardware. */

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>

#define BITS 14
#define ROUND_OFFSET (1 << (BITS - 1))

void holloman3_vscale_neon(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size) {
    const int32x4_t round = vdupq_n_s32(ROUND_OFFSET);
    const int32x4_t max255 = vdupq_n_s32(255);
    const int32x4_t zero = vdupq_n_s32(0);

    for (size_t y = 0; y < 4; y++) {
        uint8_t *dst_row = dst + y * size;
        const int32_t *coeff_row = coeffs + y * size;
        for (size_t x = 0; x < size; x += 8) {
            int32x4_t acc_lo = zero;
            int32x4_t acc_hi = zero;
            const uint8_t *src_ptr = src + x;
            const int32_t *coeff_ptr = coeff_row;
            for (size_t i = 0; i < size; i++) {
                uint8x8_t pixels_u8 = vld1_u8(src_ptr);
                uint16x8_t pixels_u16 = vmovl_u8(pixels_u8);
                uint32x4_t pixels_lo = vmovl_u16(vget_low_u16(pixels_u16));
                uint32x4_t pixels_hi = vmovl_u16(vget_high_u16(pixels_u16));
                int32x4_t coeff = vdupq_n_s32(*coeff_ptr);
                acc_lo = vaddq_s32(acc_lo, vmulq_s32((int32x4_t)pixels_lo, coeff));
                acc_hi = vaddq_s32(acc_hi, vmulq_s32((int32x4_t)pixels_hi, coeff));
                src_ptr += size;
                coeff_ptr++;
            }

            acc_lo = vmaxq_s32(vminq_s32(vshrq_n_s32(vaddq_s32(acc_lo, round), BITS), max255), zero);
            acc_hi = vmaxq_s32(vminq_s32(vshrq_n_s32(vaddq_s32(acc_hi, round), BITS), max255), zero);

            int16x8_t packed16 = vcombine_s16(vqmovn_s32(acc_lo), vqmovn_s32(acc_hi));
            uint8x8_t packed8 = vqmovun_s16(packed16);
            vst1_u8(dst_row + x, packed8);
        }
    }
}

void holloman3_hscale_neon(uint8_t *dst, const uint8_t *src,
                           const int32_t *coeffs, size_t size) {
    for (size_t y = 0; y < 4; y++) {
        uint8_t *dst_row = dst + y * 4;
        const uint8_t *src_row = src + y * size;
        for (size_t x = 0; x < 4; x++) {
            const int32_t *c = coeffs + x * size;
            int32x4_t acc = vdupq_n_s32(0);
            for (size_t i = 0; i < size; i += 4) {
                uint8x8_t pixels_u8 = vld1_u8(src_row + i);
                uint16x8_t pixels_u16 = vmovl_u8(pixels_u8);
                uint32x4_t pixels = vmovl_u16(vget_low_u16(pixels_u16));
                int32x4_t coeffs4 = vld1q_s32(c + i);
                acc = vaddq_s32(acc, vmulq_s32((int32x4_t)pixels, coeffs4));
            }

            int pix = vaddvq_s32(acc);
            pix = (pix + ROUND_OFFSET) >> BITS;
            if (pix < 0) pix = 0;
            if (pix > 255) pix = 255;
            dst_row[x] = (uint8_t)pix;
        }
    }
}
