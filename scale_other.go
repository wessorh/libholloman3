//go:build !amd64

package libholloman3

// HasAVX2 reports whether the CPU supports AVX2 (always false on non-amd64).
func HasAVX2() bool { return false }

// VScale performs the vertical Lanczos-3 scaling pass using the scalar kernel.
func VScale(dst, src []byte, coeffs []int32, size int) {
	vScaleScalar(dst, src, coeffs, []int16{0, 0, 0, 0}, size, size, 4, size, size)
}

// HScale performs the horizontal Lanczos-3 scaling pass using the scalar kernel.
func HScale(dst, src []byte, coeffs []int32, size int) {
	hScaleScalar(dst, src, coeffs, []int16{0, 0, 0, 0}, size, 4, 4, 4, size)
}
