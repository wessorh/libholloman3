// Package libholloman3 provides the SIMD scaling kernels for the holloman3
// perceptual fingerprint's Lanczos-3 downsampling, implemented as Plan 9
// assembly (AVX2 and SSE4.1 on amd64) with a portable scalar fallback. Tools
// that implement holloman3 fingerprinting link against this package for the
// two hot, byte-identical scaling passes.
package libholloman3

const (
	bits        = 14
	roundOffset = 1 << (bits - 1) // 8192
)

// u8 clamps a signed value into the uint8 range.
func u8(v int) uint8 {
	if v < 0 {
		return 0
	}
	if v > 255 {
		return 255
	}
	return uint8(v)
}

// vScaleScalar is the portable vertical scaling pass (matches v_scale_avx2's
// scalar loop).
func vScaleScalar(dst, src []byte, coeffs []int32, offsets []int16, taps, width, height, dstPitch, srcPitch int) {
	for y := 0; y < height; y++ {
		dstRow := dst[y*dstPitch:]
		c := coeffs[y*taps : (y+1)*taps]
		offset := int(offsets[y])
		for x := 0; x < width; x++ {
			acc := 0
			for i := 0; i < taps; i++ {
				acc += int(src[(offset+i)*srcPitch+x]) * int(c[i])
			}
			dstRow[x] = u8((acc + roundOffset) >> bits)
		}
	}
}

// hScaleScalar is the portable horizontal scaling pass.
func hScaleScalar(dst, src []byte, coeffs []int32, offsets []int16, taps, width, height, dstPitch, srcPitch int) {
	for y := 0; y < height; y++ {
		srcRow := src[y*srcPitch:]
		dstRow := dst[y*dstPitch:]
		for x := 0; x < width; x++ {
			offset := int(offsets[x])
			c := coeffs[x*taps : (x+1)*taps]
			acc := 0
			for i := 0; i < taps; i++ {
				acc += int(srcRow[offset+i]) * int(c[i])
			}
			dstRow[x] = u8((acc + roundOffset) >> bits)
		}
	}
}
