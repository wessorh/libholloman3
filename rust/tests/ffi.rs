//! Integration test: validates the Rust FFI against a scalar reference.

use libholloman3_sys::{has_avx2, hscale, vscale};

fn clamp(v: i32) -> u8 {
    if v < 0 {
        0
    } else if v > 255 {
        255
    } else {
        v as u8
    }
}

fn vscale_ref(dst: &mut [u8], src: &[u8], coeffs: &[i32], size: usize) {
    for y in 0..4 {
        for x in 0..size {
            let mut acc = 0i32;
            for i in 0..size {
                acc += src[i * size + x] as i32 * coeffs[y * size + i];
            }
            dst[y * size + x] = clamp((acc + 8192) >> 14);
        }
    }
}

fn hscale_ref(dst: &mut [u8], src: &[u8], coeffs: &[i32], size: usize) {
    for y in 0..4 {
        for x in 0..4 {
            let mut acc = 0i32;
            for i in 0..size {
                acc += src[y * size + i] as i32 * coeffs[x * size + i];
            }
            dst[y * 4 + x] = clamp((acc + 8192) >> 14);
        }
    }
}

#[test]
fn scales_match_scalar() {
    for &size in &[8usize, 16, 64, 128, 512] {
        let src: Vec<u8> = (0..size * size)
            .map(|i| ((i * 7 + 13) & 0xFF) as u8)
            .collect();
        let coeffs: Vec<i32> = (0..4 * size)
            .map(|i| ((i as i32) * 31 + 5) % 4096 - 2048)
            .collect();

        let mut got = vec![0u8; size * 4];
        let mut want = vec![0u8; size * 4];

        vscale(&mut got, &src, &coeffs, size);
        vscale_ref(&mut want, &src, &coeffs, size);
        assert_eq!(&got[..size * 4], &want[..size * 4], "vscale size={}", size);

        hscale(&mut got, &src, &coeffs, size);
        hscale_ref(&mut want, &src, &coeffs, size);
        assert_eq!(&got[..16], &want[..16], "hscale size={}", size);
    }
}

#[test]
fn reports_avx2() {
    // Just ensure the function is callable and returns a boolean (0 or 1).
    let b = has_avx2();
    assert!(b == false || b == true);
    let _ = b;
}
