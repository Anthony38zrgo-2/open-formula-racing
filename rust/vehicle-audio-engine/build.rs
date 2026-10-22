use std::process::Command;

fn main() {
    // Open Formula provenance stamp: resolve the destination repository HEAD.
    // Kept compatible with the donor `FORMULA90_BUILD_SHA` env consumed by
    // `src/ffi.rs` (`vehicle_audio_build_sha`) and `src/dsp_runtime.rs`.
    //
    // Destination layout is `<root>/rust/vehicle-audio-engine/build.rs`, so the
    // repository metadata lives two levels up, not three as in the donor
    // (`game/crates/vehicle-audio-engine`).
    //
    // Donor gap closed here: watching `.git/HEAD` alone misses branch-ref
    // advances and dirty sources. This script follows the ref that HEAD points
    // to (no hardcoded branch name) and records whether tracked sources under
    // `rust/` differ from the stamp commit.
    let output = Command::new("git")
        .args(["rev-parse", "HEAD"])
        .output()
        .expect("git must be available to stamp open-formula audio BUILD");
    assert!(
        output.status.success(),
        "cannot resolve git HEAD for open-formula audio BUILD"
    );
    let sha = String::from_utf8(output.stdout).expect("git HEAD must be UTF-8");
    let sha = sha.trim();
    assert_eq!(sha.len(), 40, "open-formula audio BUILD must be a full git SHA");
    println!("cargo:rustc-env=FORMULA90_BUILD_SHA={sha}");
    println!("cargo:rustc-env=OPEN_FORMULA_BUILD_SHA={sha}");
    println!("cargo:rustc-env=OPEN_FORMULA_AUDIO_PROVENANCE=open-formula-import");
    println!("cargo:rerun-if-env-changed=FORMULA90_FORCE_BUILD_SHA_REFRESH");
    println!("cargo:rerun-if-env-changed=OPEN_FORMULA_FORCE_BUILD_SHA_REFRESH");
    println!("cargo:rerun-if-changed=../../.git/HEAD");

    // Follow the branch ref (or stay on HEAD when detached) so ref advances
    // re-stamp without encoding any branch name here.
    let head = std::fs::read_to_string("../../.git/HEAD").unwrap_or_default();
    let head = head.trim();
    let dirty = if let Some(reference) = head.strip_prefix("ref: ") {
        println!("cargo:rerun-if-changed=../../{reference}");
        sources_differ()
    } else {
        // Detached HEAD: the SHA itself is the whole identity.
        sources_differ()
    };
    println!("cargo:rustc-env=OPEN_FORMULA_AUDIO_DIRTY={}", u8::from(dirty));
    if dirty {
        println!("cargo:warning=open-formula audio stamped from dirty sources (see git status)");
    }
}

/// True when tracked files under `rust/` differ from HEAD (untracked staging
/// such as `sounds/` banks is ignored: banks are hashed separately by
/// `tools/prepare_audio_banks.py`).
///
/// NOTE: cargo runs this script with the package directory as CWD
/// (`<root>/rust/vehicle-audio-engine`), so git must be pointed at the
/// repository root explicitly; a bare `-- rust/` pathspec would match nothing.
fn sources_differ() -> bool {
    let output = Command::new("git")
        .args(["-C", "../..", "status", "--porcelain", "--", "rust/"])
        .output();
    let Ok(output) = output else {
        return true;
    };
    if !output.status.success() {
        return true;
    }
    let text = String::from_utf8_lossy(&output.stdout);
    text.lines().any(|line| !line.starts_with("??"))
}
