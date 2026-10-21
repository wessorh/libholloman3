// 128-bit SSE4.1 kernels for the holloman3 Lanczos downsampling (amd64 only).
//
// These run on AVX1-only CPUs (Sandy/Ivy Bridge, Bulldozer) that lack the
// 256-bit integer ops used by lanczos_amd64.s. Results are byte-identical.

#include "textflag.h"

// func vScaleSSE41(dst, src, coeffs unsafe.Pointer, size int)
//
// Vertical scaling pass specialized for the fingerprint pipeline (same
// contract as vScaleAVX2): height=4, offsets=0, width=taps=srcPitch=dstPitch=size.
// Processes 4 columns per iteration (128-bit). size is a power of two >= 8.
TEXT ·vScaleSSE41(SB), NOSPLIT, $0-32
	MOVQ dst+0(FP), R12
	MOVQ src+8(FP), R13
	MOVQ coeffs+16(FP), R14
	MOVQ size+24(FP), R15

	// Constants.
	PXOR X5, X5             // X5 = 0
	MOVL $255, AX
	MOVQ AX, X7
	PSHUFD $0, X7, X7       // X7 = 255 in all lanes
	MOVL $8192, AX
	MOVQ AX, X6
	PSHUFD $0, X6, X6       // X6 = 8192 in all lanes

	XORL R8, R8             // y = 0
yloop:
	CMPL R8, $4
	JGE ydone

	// SI = dstRow = dst + y*size.
	MOVQ R8, R9
	IMULQ R15, R9
	ADDQ R12, R9
	MOVQ R9, SI

	// R9 = coeffBase = coeffs + y*size*2.
	MOVQ R8, R9
	IMULQ R15, R9
	SHLQ $2, R9
	ADDQ R14, R9

	XORL R10, R10           // x = 0
xloop:
	CMPQ R10, R15
	JGE xdone

	MOVQ R13, DX            // srcPtr = src + x
	ADDQ R10, DX
	MOVQ R9, DI             // coeffPtr = coeffBase

	PXOR X0, X0             // acc = 0
	MOVQ R15, R11           // i = size (down-counter)
tap:
	PMOVZXBD (DX), X1       // 4 pixels -> 4 int32
	MOVL (DI), AX           // load coeff[i] (int32)
	MOVL AX, X2             // move to XMM
	PSHUFD $0, X2, X2       // broadcast coeff to 4 lanes
	PMULLD X2, X1           // product
	PADDD X1, X0            // acc += product
	ADDQ R15, DX            // srcPtr += size
	ADDQ $4, DI             // coeffPtr += 4
	DECQ R11
	JNZ tap

	PADDD X6, X0            // acc += 8192
	PSRAL $14, X0           // acc >>= 14 (arithmetic)
	PMAXSD X5, X0           // max(0, acc)
	PMINSD X7, X0           // min(255, acc)

	PACKUSDW X0, X0         // 4 int32 -> 8 int16 (low 4 valid)
	PACKUSWB X0, X0         // 8 int16 -> 16 uint8 (low 4 valid)
	MOVL X0, AX             // low 32 bits = 4 bytes
	MOVL AX, (SI)(R10*1)    // store 4 bytes to dstRow[x]

	ADDQ $4, R10            // x += 4
	JMP xloop
xdone:
	INCQ R8
	JMP yloop
ydone:
	RET

// func hScaleSSE41(dst, src, coeffs unsafe.Pointer, size int)
//
// Horizontal scaling pass (same contract as hScaleAVX2): height=4, width=4,
// offsets=0, taps=srcPitch=size, dstPitch=4. Processes 4 taps per iteration.
// size is a power of two >= 8.
TEXT ·hScaleSSE41(SB), NOSPLIT, $0-32
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

	PXOR X0, X0          // acc = 0
	MOVQ R15, R11        // i = size
htap:
	PMOVZXBD (DX), X1    // 4 pixels -> 4 int32
	MOVOU (CX), X2       // 4 int32 coeffs (128-bit unaligned)
	PMULLD X2, X1        // product
	PADDD X1, X0         // acc += product
	ADDQ $4, DX          // srcPtr += 4
	ADDQ $16, CX         // coeffPtr += 16 (4 int32)
	SUBQ $4, R11
	JNZ htap

	PHADDD X0, X0        // horizontal sum (4 int32 -> 1)
	PHADDD X0, X0
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
