//! Rust FFI bindings to the libholloman3 C library.
//!
//! Thin, safe wrappers over the `holloman3_*` C functions (see
//! `../include/holloman3.h` for the exact contract). The C library is built and
//! linked by `build.rs`.

use std::os::raw::c_int;

extern "C" {
    fn holloman3_has_avx2() -> c_int;
    fn holloman3_vscale(dst: *mut u8, src: *const u8, coeffs: *const i32, size: usize);
    fn holloman3_hscale(dst: *mut u8, src: *const u8, coeffs: *const i32, size: usize);
}

/// Reports whether the CPU supports AVX2.
pub fn has_avx2() -> bool {
    unsafe { holloman3_has_avx2() != 0 }
}

/// Vertical Lanczos-3 scaling pass (height=4, offsets=0, taps=src_pitch=size).
pub fn vscale(dst: &mut [u8], src: &[u8], coeffs: &[i32], size: usize) {
    unsafe { holloman3_vscale(dst.as_mut_ptr(), src.as_ptr(), coeffs.as_ptr(), size) }
}

/// Horizontal Lanczos-3 scaling pass (height=4, width=4, dst_pitch=4).
pub fn hscale(dst: &mut [u8], src: &[u8], coeffs: &[i32], size: usize) {
    unsafe { holloman3_hscale(dst.as_mut_ptr(), src.as_ptr(), coeffs.as_ptr(), size) }
}
