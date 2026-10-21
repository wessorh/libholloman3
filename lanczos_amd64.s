// AVX2 kernels for the holloman3 Lanczos downsampling (amd64 only).

#include "textflag.h"

DATA roundv<>+0(SB)/4, $8192
GLOBL roundv<>(SB), RODATA, $4

DATA max255v<>+0(SB)/4, $255
GLOBL max255v<>(SB), RODATA, $4

// func vScaleAVX2(dst, src, coeffs unsafe.Pointer, size int)
//
// Vertical scaling pass specialized for the fingerprint pipeline:
//   height = 4, offsets = 0, width = taps = srcPitch = dstPitch = size.
// size is a power of two >= 8, so it is a multiple of 8 (no scalar remainder).
//
// dst    = size*4 bytes (row-major, pitch = size)
// src    = size*size bytes (row-major, pitch = size)
// coeffs = [4][size]int16
//
// ABI (amd64 register ABI): dst=AX, src=BX, coeffs=CX, size=DI.
TEXT ·vScaleAVX2(SB), NOSPLIT, $0-32
	MOVQ dst+0(FP), R12
	MOVQ src+8(FP), R13
	MOVQ coeffs+16(FP), R14
	MOVQ size+24(FP), R15

	VPBROADCASTD roundv<>(SB), Y6   // Y6 = 8192
	VPBROADCASTD max255v<>(SB), Y7  // Y7 = 255
	VPXOR Y5, Y5, Y5                // Y5 = 0

	XORL R8, R8          // y = 0
yloop:
	CMPL R8, $4
	JGE ydone

	// R9 = dstRow = dst + y*size
	MOVQ R8, R9
	IMULQ R15, R9
	ADDQ R12, R9
	MOVQ R9, SI          // SI = dstRow

	// R9 = coeffBase = coeffs + y*size*2
	MOVQ R8, R9
	IMULQ R15, R9
	SHLQ $2, R9
	ADDQ R14, R9         // R9 = coeffBase

	XORL R10, R10        // x = 0
xloop:
	CMPQ R10, R15
	JGE xdone

	MOVQ R13, DX         // srcPtr = src
	ADDQ R10, DX         // srcPtr = src + x
	MOVQ R9, DI          // coeffPtr = coeffBase

	VPXOR Y0, Y0, Y0    // acc = 0
	MOVQ R15, R11       // i = size (down-counter)
tap:
	VMOVQ (DX), X1          // load 8 pixels (64-bit)
	VPMOVZXBD X1, Y1        // zero-extend from register (fast; memory form is microcoded)
	VPBROADCASTD (DI), Y2   // broadcast coeff[i] (int32) directly from memory
	VPMULLD Y1, Y2, Y3      // product
	VPADDD Y3, Y0, Y0       // acc += product
	ADDQ R15, DX            // srcPtr += size
	ADDQ $4, DI             // coeffPtr += 4
	DECQ R11
	JNZ tap

	VPADDD Y6, Y0, Y0    // acc += 8192
	VPSRAD $14, Y0, Y0   // acc >>= 14 (arithmetic)
	VPMAXSD Y5, Y0, Y0   // max(0, acc)
	VPMINSD Y7, Y0, Y0   // min(255, acc)

	VEXTRACTI128 $1, Y0, X1   // X1 = lanes 4-7
	VPACKUSDW X1, X0, X0      // X0 = 8 int16 (lanes 0-7)
	VPACKUSWB X0, X0, X0      // X0 = 16 uint8 (low 8 = result)
	MOVQ X0, AX               // AX = 8 bytes
	MOVQ AX, (SI)(R10*1)      // store to dstRow[x]

	ADDQ $8, R10
	JMP xloop
xdone:
	INCQ R8
	JMP yloop
ydone:
	RET

// func hScaleAVX2(dst, src, coeffs unsafe.Pointer, size int)
//
// Horizontal scaling pass specialized for the fingerprint pipeline:
//   height = 4, width = 4, offsets = 0, taps = srcPitch = size, dstPitch = 4.
// size is a power of two >= 8.
//
// dst    = 4*4 = 16 bytes (row-major, pitch = 4)
// src    = size*4 bytes (row-major, pitch = size)  [the vertical intermediate]
// coeffs = [4][size]int32
TEXT ·hScaleAVX2(SB), NOSPLIT, $0-32
	MOVQ dst+0(FP), R12
	MOVQ src+8(FP), R13
	MOVQ coeffs+16(FP), R14
	MOVQ size+24(FP), R15

	XORL R8, R8          // y = 0
hyloop:
	CMPL R8, $4
	JGE hydone

	// SI = srcRow = src + y*size.
	MOVQ R8, R9
	IMULQ R15, R9
	ADDQ R13, R9
	MOVQ R9, SI

	// DI = dstRow = dst + y*4.
	MOVQ R8, R9
	SHLQ $2, R9
	ADDQ R12, R9
	MOVQ R9, DI

	XORL R10, R10        // x = 0
hxloop:
	CMPL R10, $4
	JGE hxdone

	// CX = coeffCol = coeffs + x*size*4 (int32).
	MOVQ R10, R9
	IMULQ R15, R9
	SHLQ $2, R9
	ADDQ R14, R9
	MOVQ R9, CX

	MOVQ SI, DX          // DX = srcPtr

	VPXOR Y0, Y0, Y0     // acc = 0
	MOVQ R15, R11        // i = size
htap:
	VMOVQ (DX), X1       // 8 pixels
	VPMOVZXBD X1, Y1     // zero-extend to 8 int32
	VMOVDQU (CX), Y2     // 8 int32 coeffs
	VPMULLD Y1, Y2, Y3   // product
	VPADDD Y3, Y0, Y0    // acc += product
	ADDQ $8, DX          // srcPtr += 8
	ADDQ $32, CX         // coeffPtr += 32 (8 int32)
	SUBQ $8, R11
	JNZ htap

	// Horizontal sum: Y0 (8 int32) -> AX (1 int32).
	VEXTRACTI128 $1, Y0, X1
	VPADDD X1, X0, X0
	VPHADDD X0, X0, X1
	VPHADDD X1, X1, X0
	MOVL X0, AX

	// Clamp: u8((AX + 8192) >> 14).
	ADDL $8192, AX
	SARL $14, AX
	TESTL AX, AX
	JGE hnotneg
	XORL AX, AX
hnotneg:
	CMPL AX, $255
	JLE hstore
	MOVL $255, AX
hstore:
	MOVB AL, (DI)(R10*1)

	INCQ R10
	JMP hxloop
hxdone:
	INCQ R8
	JMP hyloop
hydone:
	RET
