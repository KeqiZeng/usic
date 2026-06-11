use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    println!("cargo:rerun-if-changed=macos/usic-macos-media/main.swift");

    if env::var("CARGO_CFG_TARGET_OS").as_deref() != Ok("macos") {
        return;
    }

    let Some(target_dir) = target_dir() else {
        println!("cargo:warning=failed to resolve Cargo target directory for Swift helper");
        return;
    };

    let source = PathBuf::from("macos/usic-macos-media/main.swift");
    let output = target_dir.join("usic-macos-media");
    let status = Command::new("swiftc")
        .arg(&source)
        .arg("-framework")
        .arg("Foundation")
        .arg("-framework")
        .arg("MediaPlayer")
        .arg("-o")
        .arg(&output)
        .status();

    match status {
        Ok(status) if status.success() => {}
        Ok(status) => {
            println!("cargo:warning=swiftc failed to build usic-macos-media: {status}");
        }
        Err(err) => {
            println!("cargo:warning=failed to run swiftc for usic-macos-media: {err}");
        }
    }
}

fn target_dir() -> Option<PathBuf> {
    let out_dir = PathBuf::from(env::var_os("OUT_DIR")?);
    out_dir.ancestors().nth(3).map(PathBuf::from)
}
