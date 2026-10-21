# libholloman3 — SIMD scaling kernels for the holloman3 fingerprint.
#
# Targets:
#   make            build the static + shared C library
#   make test       build and run the C99 test suite
#   make gotest     run the Go test suite
#   make clean      remove build artifacts
#
# The C library uses compiler intrinsics, so the same sources build on
# x86-64 (AVX2 + SSE4.1) and aarch64 (NEON) with gcc/clang/MSVC — no per-OS or
# per-arch assembly duplication.

CC       ?= cc
AR       ?= ar
CFLAGS   ?= -O3 -Wall -Wextra -std=c99
INCLUDE   = -Iinclude
ARCH     := $(shell uname -m)

CORE_SRCS = src/scale.c src/arch.c

ifeq ($(ARCH),x86_64)
  SIMD_SRCS  = src/scale_avx2.c src/scale_sse41.c
  SIMD_CFLAGS = -mavx2 -msse4.1
else ifeq ($(ARCH),aarch64)
  SIMD_SRCS  = src/scale_neon.c
  SIMD_CFLAGS =
else ifeq ($(ARCH),arm64)
  SIMD_SRCS  = src/scale_neon.c
  SIMD_CFLAGS =
else
  SIMD_SRCS  =
  SIMD_CFLAGS =
endif

SRCS = $(CORE_SRCS) $(SIMD_SRCS)
OBJS = $(SRCS:.c=.o)

.PHONY: all test gotest cross-neon cross-windows clean

# Cross-compile for aarch64 and validate the NEON backend under qemu-user.
# Requires: gcc-aarch64-linux-gnu, qemu-user.
cross-neon:
	aarch64-linux-gnu-gcc -O3 -Wall -Wextra -std=c99 $(INCLUDE) -c -o /tmp/h3_scale_neon.o src/scale_neon.c
	aarch64-linux-gnu-gcc -O3 -Wall -Wextra -std=c99 $(INCLUDE) -c -o /tmp/h3_scale.o src/scale.c
	aarch64-linux-gnu-gcc -O3 -Wall -Wextra -std=c99 $(INCLUDE) -c -o /tmp/h3_arch.o src/arch.c
	aarch64-linux-gnu-gcc -static -O3 $(INCLUDE) -o /tmp/h3_test_scale_aarch64 \
		/tmp/h3_scale_neon.o /tmp/h3_scale.o /tmp/h3_arch.o tests/test_scale.c -lm
	qemu-aarch64 /tmp/h3_test_scale_aarch64

# Cross-compile for Windows x86-64 (static) and validate under wine.
# Requires: gcc-mingw-w64-x86-64, wine.
MINGW = x86_64-w64-mingw32-gcc
cross-windows:
	$(MINGW) -O3 -Wall -Wextra -std=c99 -mavx2 -msse4.1 -DHOLLOMAN3_STATIC $(INCLUDE) -c -o /tmp/h3_win_scale.o src/scale.c
	$(MINGW) -O3 -Wall -Wextra -std=c99 -mavx2 -msse4.1 -DHOLLOMAN3_STATIC $(INCLUDE) -c -o /tmp/h3_win_arch.o src/arch.c
	$(MINGW) -O3 -Wall -Wextra -std=c99 -mavx2 -msse4.1 -DHOLLOMAN3_STATIC $(INCLUDE) -c -o /tmp/h3_win_avx2.o src/scale_avx2.c
	$(MINGW) -O3 -Wall -Wextra -std=c99 -mavx2 -msse4.1 -DHOLLOMAN3_STATIC $(INCLUDE) -c -o /tmp/h3_win_sse41.o src/scale_sse41.c
	$(MINGW) -O3 -std=c99 -mavx2 -msse4.1 -DHOLLOMAN3_STATIC $(INCLUDE) -o /tmp/h3_test_scale.exe \
		/tmp/h3_win_scale.o /tmp/h3_win_arch.o /tmp/h3_win_avx2.o /tmp/h3_win_sse41.o tests/test_scale.c -lm
	wine /tmp/h3_test_scale.exe

all: libholloman3.a libholloman3.so

%.o: %.c
	$(CC) $(CFLAGS) $(SIMD_CFLAGS) $(INCLUDE) -fPIC -c -o $@ $<

libholloman3.a: $(OBJS)
	$(AR) rcs $@ $^

libholloman3.so: $(OBJS)
	$(CC) -shared -o $@ $^

test: tests/test_scale
	./tests/test_scale

tests/test_scale: tests/test_scale.c libholloman3.a
	$(CC) $(CFLAGS) $(SIMD_CFLAGS) $(INCLUDE) -o $@ tests/test_scale.c libholloman3.a -lm

gotest:
	go test ./...

clean:
	rm -f $(OBJS) libholloman3.a libholloman3.so tests/test_scale
