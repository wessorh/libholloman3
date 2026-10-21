//go:build amd64

package libholloman3

import (
	"testing"
	"unsafe"
)

// TestSIMDvScaleMatchesScalar verifies the AVX2 and SSE4.1 vertical kernels are
// byte-identical to the scalar reference for a range of power-of-two sizes.
func TestSIMDvScaleMatchesScalar(t *testing.T) {
	for _, size := range []int{8, 16, 64, 128, 512} {
		src := make([]byte, size*size)
		coeffs := make([]int32, 4*size)
		for i := range src {
			src[i] = byte((i*7 + 13) & 0xFF)
		}
		for i := range coeffs {
			coeffs[i] = int32((i*31+5)%4096 - 2048)
		}

		scalar := make([]byte, size*4)
		vScaleScalar(scalar, src, coeffs, []int16{0, 0, 0, 0}, size, size, 4, size, size)

		avx2 := make([]byte, size*4)
		vScaleAVX2(unsafe.Pointer(&avx2[0]), unsafe.Pointer(&src[0]), unsafe.Pointer(&coeffs[0]), size)

		sse41 := make([]byte, size*4)
		vScaleSSE41(unsafe.Pointer(&sse41[0]), unsafe.Pointer(&src[0]), unsafe.Pointer(&coeffs[0]), size)

		for i := range scalar {
			if avx2[i] != scalar[i] {
				t.Fatalf("size=%d byte %d: avx2=%d scalar=%d", size, i, avx2[i], scalar[i])
			}
			if sse41[i] != scalar[i] {
				t.Fatalf("size=%d byte %d: sse41=%d scalar=%d", size, i, sse41[i], scalar[i])
			}
		}
	}
}

// TestSIMDhScaleMatchesScalar verifies the AVX2 and SSE4.1 horizontal kernels
// are byte-identical to the scalar reference.
func TestSIMDhScaleMatchesScalar(t *testing.T) {
	for _, size := range []int{8, 16, 64, 128, 512} {
		src := make([]byte, size*4)
		coeffs := make([]int32, 4*size)
		for i := range src {
			src[i] = byte((i*7 + 13) & 0xFF)
		}
		for i := range coeffs {
			coeffs[i] = int32((i*31+5)%4096 - 2048)
		}

		scalar := make([]byte, 16)
		hScaleScalar(scalar, src, coeffs, []int16{0, 0, 0, 0}, size, 4, 4, 4, size)

		avx2 := make([]byte, 16)
		hScaleAVX2(unsafe.Pointer(&avx2[0]), unsafe.Pointer(&src[0]), unsafe.Pointer(&coeffs[0]), size)

		sse41 := make([]byte, 16)
		hScaleSSE41(unsafe.Pointer(&sse41[0]), unsafe.Pointer(&src[0]), unsafe.Pointer(&coeffs[0]), size)

		for i := 0; i < 16; i++ {
			if avx2[i] != scalar[i] {
				t.Fatalf("size=%d byte %d: avx2=%d scalar=%d", size, i, avx2[i], scalar[i])
			}
			if sse41[i] != scalar[i] {
				t.Fatalf("size=%d byte %d: sse41=%d scalar=%d", size, i, sse41[i], scalar[i])
			}
		}
	}
}
