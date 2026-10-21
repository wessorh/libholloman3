//go:build amd64

package libholloman3

import (
	"testing"
	"unsafe"
)

func benchVScale(b *testing.B, size int, fn func(dst, src []byte, coeffs []int32, size int)) {
	src := make([]byte, size*size)
	coeffs := make([]int32, 4*size)
	for i := range src {
		src[i] = byte((i*7 + 13) & 0xFF)
	}
	for i := range coeffs {
		coeffs[i] = int32((i*31+5)%4096 - 2048)
	}
	dst := make([]byte, size*4)
	b.SetBytes(int64(size * size))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		fn(dst, src, coeffs, size)
	}
}

func benchAll(b *testing.B, size int) {
	b.Run("scalar", func(b *testing.B) {
		benchVScale(b, size, func(dst, src []byte, c []int32, n int) {
			vScaleScalar(dst, src, c, []int16{0, 0, 0, 0}, n, n, 4, n, n)
		})
	})
	b.Run("sse41", func(b *testing.B) {
		benchVScale(b, size, func(dst, src []byte, c []int32, n int) {
			vScaleSSE41(unsafe.Pointer(&dst[0]), unsafe.Pointer(&src[0]), unsafe.Pointer(&c[0]), n)
		})
	})
	b.Run("avx2", func(b *testing.B) {
		benchVScale(b, size, func(dst, src []byte, c []int32, n int) {
			vScaleAVX2(unsafe.Pointer(&dst[0]), unsafe.Pointer(&src[0]), unsafe.Pointer(&c[0]), n)
		})
	})
}

func BenchmarkVScale256(b *testing.B)  { benchAll(b, 256) }
func BenchmarkVScale512(b *testing.B)  { benchAll(b, 512) }
func BenchmarkVScale1024(b *testing.B) { benchAll(b, 1024) }
func BenchmarkVScale2048(b *testing.B) { benchAll(b, 2048) }
