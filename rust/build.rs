//! Build script: compiles the C library (via make) and links it statically.

use std::process::Command;

fn main() {
    let out = Command::new("make")
        .arg("-C")
        .arg("..")
        .arg("libholloman3.a")
        .output()
        .expect("failed to run make");

    if !out.status.success() {
        panic!(
            "make libholloman3.a failed:\n{}",
            String::from_utf8_lossy(&out.stderr)
        );
    }

    println!("cargo:rustc-link-search=native=..");
    println!("cargo:rustc-link-lib=static=holloman3");
    println!("cargo:rerun-if-changed=../src");
    println!("cargo:rerun-if-changed=../include/holloman3.h");
}
