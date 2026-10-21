//go:build amd64

package libholloman3

import "unsafe"

//go:noescape
func vScaleAVX2(dst, src, coeffs unsafe.Pointer, size int)

//go:noescape
func vScaleSSE41(dst, src, coeffs unsafe.Pointer, size int)

//go:noescape
func hScaleAVX2(dst, src, coeffs unsafe.Pointer, size int)

//go:noescape
func hScaleSSE41(dst, src, coeffs unsafe.Pointer, size int)

//go:noescape
func hasAVX2() bool

// HasAVX2 reports whether the CPU supports AVX2 (with OSXSAVE enabled).
func HasAVX2() bool { return hasAVX2() }

// VScale performs the vertical Lanczos-3 scaling pass specialized for the
// fingerprint pipeline: height=4, offsets=0, width=taps=srcPitch=dstPitch=size.
// size is a power of two; size < 8 falls back to scalar. AVX2 is used when
// available, SSE4.1 otherwise. Byte-identical to the scalar kernel.
func VScale(dst, src []byte, coeffs []int32, size int) {
	if size >= 8 {
		d := unsafe.Pointer(&dst[0])
		s := unsafe.Pointer(&src[0])
		c := unsafe.Pointer(&coeffs[0])
		if hasAVX2() {
			vScaleAVX2(d, s, c, size)
		} else {
			vScaleSSE41(d, s, c, size)
		}
		return
	}
	vScaleScalar(dst, src, coeffs, []int16{0, 0, 0, 0}, size, size, 4, size, size)
}

// HScale performs the horizontal Lanczos-3 scaling pass (height=4, width=4,
// offsets=0, taps=srcPitch=size, dstPitch=4). size < 8 falls back to scalar.
func HScale(dst, src []byte, coeffs []int32, size int) {
	if size >= 8 {
		d := unsafe.Pointer(&dst[0])
		s := unsafe.Pointer(&src[0])
		c := unsafe.Pointer(&coeffs[0])
		if hasAVX2() {
			hScaleAVX2(d, s, c, size)
		} else {
			hScaleSSE41(d, s, c, size)
		}
		return
	}
	hScaleScalar(dst, src, coeffs, []int16{0, 0, 0, 0}, size, 4, 4, 4, size)
}
