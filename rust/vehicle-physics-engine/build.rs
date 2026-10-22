use std::fs;
use std::path::PathBuf;
use std::process::Command;

use serde_json::{json, Map, Value};

const EMBEDDED_PROFILE_DIRECTORY: &str = "../data/vehicles/williams-fw19-hockenheim";
const EMBEDDED_DOCUMENT_FILE_NAME: &str = "resolved_williams_fw19_hockenheim.json";
const LEGACY_ASSEMBLY_SCHEMA_VERSION: u32 = 3;

fn main() {
    emit_provenance();
    generate_embedded_geometric_profile();
}

/// The destination workspace is not guaranteed to expose the source checkout, so the
/// provenance of the copied physics code is declared in `provenance.json` and embedded
/// as a compile-time constant. A short destination Git hash is attached when available.
fn emit_provenance() {
    let crate_directory =
        PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    let provenance_path = crate_directory.join("provenance.json");
    println!("cargo:rerun-if-changed={}", provenance_path.display());
    let provenance_text = fs::read_to_string(&provenance_path)
        .unwrap_or_else(|error| panic!("cannot read {}: {error}", provenance_path.display()));
    let provenance: Value = serde_json::from_str(&provenance_text)
        .unwrap_or_else(|error| panic!("cannot parse {}: {error}", provenance_path.display()));
    let source_commit = provenance["source_commit"]
        .as_str()
        .expect("provenance source_commit must be a string");
    assert_eq!(
        source_commit.len(),
        40,
        "physics source provenance must be a full commit identifier"
    );
    println!("cargo:rustc-env=PHYSICS_SOURCE_COMMIT={source_commit}");

    let destination_hash = Command::new("git")
        .args(["rev-parse", "--short", "HEAD"])
        .current_dir(&crate_directory)
        .output()
        .ok()
        .filter(|output| output.status.success())
        .and_then(|output| String::from_utf8(output.stdout).ok())
        .map(|text| text.trim().to_string())
        .filter(|text| !text.is_empty())
        .unwrap_or_else(|| "unknown".to_string());
    println!("cargo:rustc-env=GIT_HASH={destination_hash}");
    println!("cargo:rustc-env=COUPLED_WORLD_SOURCE_IDENTIFIER={source_commit}");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src");
}

/// Resolves the adapted Williams profile (manifest + domain documents) into the single
/// legacy assembly document the runtime consumes. The profile therefore ships with the
/// crate instead of referring to a source checkout or to the retired F1 2030 default.
fn generate_embedded_geometric_profile() {
    let crate_directory =
        PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    let profile_directory = crate_directory.join(EMBEDDED_PROFILE_DIRECTORY);
    let manifest_path = profile_directory.join("vehicle.json");
    println!("cargo:rerun-if-changed={}", manifest_path.display());

    let manifest_text = fs::read_to_string(&manifest_path)
        .unwrap_or_else(|error| panic!("cannot read {}: {error}", manifest_path.display()));
    let manifest: Value = serde_json::from_str(&manifest_text)
        .unwrap_or_else(|error| panic!("cannot parse {}: {error}", manifest_path.display()));
    let schema_version = manifest["schema_version"]
        .as_u64()
        .expect("manifest schema_version must be an integer");
    assert_eq!(
        schema_version, 1,
        "unsupported vehicle profile manifest schema version"
    );

    let domain_references = manifest["domains"]
        .as_object()
        .expect("manifest domains must be an object");

    let mut merged_document = Map::new();
    merged_document.insert(
        "schema_version".to_string(),
        json!(LEGACY_ASSEMBLY_SCHEMA_VERSION),
    );
    merged_document.insert(
        "vehicle_id".to_string(),
        manifest["vehicle_identifier"].clone(),
    );
    merged_document.insert(
        "profile_name".to_string(),
        manifest["profile_name"].clone(),
    );
    merged_document.insert("units".to_string(), manifest["units"].clone());
    merged_document.insert(
        "coordinate_contract".to_string(),
        manifest["coordinate_contract"].clone(),
    );

    let section_map: [(&str, &[&str]); 11] = [
        ("chassis", &["chassis", "geometry", "physical_mass_inventory"]),
        ("fuel", &["fuel"]),
        ("steering", &["steering"]),
        ("powertrain", &["powertrain"]),
        ("suspension", &["suspension"]),
        ("tires", &["tires"]),
        ("brakes", &["brakes"]),
        ("aerodynamics", &["aero"]),
        ("driving_assistance", &["aids"]),
        ("underfloor_contact", &["underfloor_contact"]),
        ("coupled_world", &["coupled_world"]),
    ];

    for (domain_name, section_names) in section_map {
        let reference = domain_references
            .get(domain_name)
            .and_then(Value::as_str)
            .unwrap_or_else(|| panic!("manifest domains.{domain_name} is required"));
        let document_path = profile_directory.join(reference);
        println!("cargo:rerun-if-changed={}", document_path.display());
        let document_text = fs::read_to_string(&document_path)
            .unwrap_or_else(|error| panic!("cannot read {}: {error}", document_path.display()));
        let document: Value = serde_json::from_str(&document_text)
            .unwrap_or_else(|error| panic!("cannot parse {}: {error}", document_path.display()));
        for section_name in section_names {
            if let Some(section_value) = document.get(*section_name) {
                if !section_value.is_null() {
                    merged_document.insert(section_name.to_string(), section_value.clone());
                }
            }
        }
    }

    let output_directory = PathBuf::from(std::env::var("OUT_DIR").expect("OUT_DIR"));
    let output_path = output_directory.join(EMBEDDED_DOCUMENT_FILE_NAME);
    let output_text = serde_json::to_string(&Value::Object(merged_document))
        .expect("resolved vehicle configuration must serialize");
    fs::write(&output_path, output_text).expect("resolved vehicle configuration must be written");
}
