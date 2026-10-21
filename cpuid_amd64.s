// CPUID feature detection for the amd64 SIMD dispatch.

#include "textflag.h"

// func hasAVX2() bool
//
// Reports whether the CPU supports AVX2 with the OS having enabled XSAVE state
// (OSXSAVE) and XCR0 YMM. Mirrors the C arch_detect.c AVX2 check.
TEXT ·hasAVX2(SB), NOSPLIT, $0-1
	// CPUID leaf 1: OSXSAVE (ECX bit 27) and AVX (ECX bit 28).
	MOVL $1, AX
	CPUID
	MOVL CX, DX
	ANDL $0x18000000, DX
	CMPL DX, $0x18000000
	JNE no

	// XGETBV (ECX=0) -> EDX:EAX = XCR0; require SSE (bit 1) and YMM (bit 2).
	XORL CX, CX
	BYTE $0x0f; BYTE $0x01; BYTE $0xd0
	ANDL $0x6, AX
	CMPL AX, $0x6
	JNE no

	// CPUID leaf 7 subleaf 0: AVX2 (EBX bit 5).
	MOVL $7, AX
	XORL CX, CX
	CPUID
	ANDL $0x20, BX
	CMPL BX, $0x20
	JNE no

	MOVB $1, ret+0(FP)
	RET
no:
	MOVB $0, ret+0(FP)
	RET
