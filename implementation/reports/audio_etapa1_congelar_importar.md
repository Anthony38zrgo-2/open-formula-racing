# Etapa 1 — Congelar e importar (cerrada parcial, validada)

Fecha: 2026-10-22. Rama: `codex/initial-port`. HEAD destino: `9c9164869c8210a558abb5e9aa5e760e5eacf5dd`.
Origen: `D:/Formula90s` HEAD `f3efcf246c2a2abf62477037ed0243f2f9a93a9b` con diff local en `grand_prix_sampler.rs` incluido.

## Qué se hizo

- Copiados íntegros `game/crates/vehicle-audio-engine` → `rust/vehicle-audio-engine` y `game/crates/v10-engine-synth` → `rust/v10-engine-synth`.
  SHA-256 de `grand_prix_sampler.rs` tras copiar: `1441064fe5b1868e4794417b97c8d403e047480181730956f465a0089c8480b2` (coincide con el donante evaluado).
- `rust/Cargo.toml`: miembros `vehicle-physics-engine`, `vehicle-audio-engine`, `v10-engine-synth`. `Cargo.lock` actualizado. `cargo check -p vehicle_audio_engine --offline` OK.
- `build.rs`: sello con HEAD de Open Formula, expone `FORMULA90_BUILD_SHA` (compat) + `OPEN_FORMULA_BUILD_SHA` + `OPEN_FORMULA_AUDIO_DIRTY`, observa `../../.git/HEAD` y el ref dinámico al que apunta (sin rama fija), y avisa con `cargo:warning` si hay fuentes tracked sucias bajo `rust/` (ronda 10; el aviso dispara hoy porque el árbol lleva el trabajo de Etapas).
- `dsp_runtime.rs`: sustituido `include_str!("../../../BUILD_SOURCE")` por `match option_env!(OPEN_FORMULA_BUILD_SHA/FORMULA90_BUILD_SHA)` con fallback de ceros. Ruta DSP sigue desactivada para `grand_prix_sampler`.
- `.gitignore`: ignora `/sounds/`, `/.acl-repair-out/` y targets de los nuevos crates. Los WAV stageados no se versionan; la procedencia se registra por hashes.
- Procedencia: [implementation/audio_v10_provenance.json](../audio_v10_provenance.json) (revisiones, hashes de manifests, mixer config, perfil Williams, variante local, licencias).

## Adaptación 3500 RPM (variante local `OFP-V10-3500`, banco intacto)

Banco donante: cobertura `[4500, 18000]`, referencia ralentí `4531.638723634397` RPM. Williams exige `3500` RPM con seguimiento real (`3500/4531.6387 ≈ 0.7723`), no clamp a `4500`.

Cambios solo en `rust/vehicle-audio-engine/src/grand_prix_sampler.rs`:

- Nuevas constantes `OPEN_FORMULA_MINIMUM_RPM=3500`, `OPEN_FORMULA_IDLE_REFERENCE_RPM`, `OPEN_FORMULA_IDLE_EXTENDED_RATE_MIN=0.76`, `OPEN_FORMULA_IDLE_EXTENSION_VARIANT="OFP-V10-3500"`.
- `GrandPrixSampler` guarda `effective_minimum_revolutions_per_minute` + rango requerido. `new()` admite mínimo bajo cobertura si `>=1200` y `>=3500`, conserva rechazo superior.
- Render: clamp inferior usa el mínimo efectivo; pesos usan `open_formula_powered_engine_loop_weights()` (bajo `4500` solo el ralentí pesa `1.0`, equivalente a extender su `presence_curve` a `[3500:1, 4800:1, 5600:0]`); el loop de ralentí admite rate `>=0.76` sin contar `clamped_rate_samples`.
- `idle_transition ((rendered-4500)/1500)` se conserva: en `3500` vale `0` (motor pleno, sin coast), continuo con `4500`. Motor a `0` sigue en silencio por el gate (`>=1200`).
- Accesores `effective_minimum_revolutions_per_minute()`, `idle_extension_variant()`, `required_range_revolutions_per_minute()`.

## Validación Etapa 1

Staging local (no versionado, hashes verificados, sin `.import`): `sounds/banks/v10-v2-bank`, `sounds/banks/commons`, `sounds/sound_mixer_config.json`.
Así `CARGO_MANIFEST_DIR/../../sounds/...` resuelve en el destino igual que en el donante (`game/sounds/...`).

- Nuevas pruebas puras (sin banco): `open_formula_idle_rate_math_matches_reference`, `open_formula_idle_extension_variant_identifier_is_stable` — OK.
- Nuevas pruebas con banco: `open_formula_idle_3500_tracks_real_rpm_without_4500_clamp` (rendered≈3500, rate≈0.7723, `clamped_rate_samples==0`, audible), `open_formula_idle_transition_3500_to_5600_stays_continuous` (energía finita sin saltos), `open_formula_upper_limit_still_rejects_above_bank_coverage` — OK (5/5 con las donantes `williams_idle_*` y `unsupported_upper_*`).
- Suite completa `--lib`: **291 passed, 16 failed** (ronda 22; antes 289 por dos tests OFP añadidos después: retención bajo 4500 y silencio a motor 0). Los 16 son rutas desactivadas por diseño (`f90_audio_dsp.dll`, `audio/v10_gf509`, `audio/v10_f2002_sampled`, `sounds/banks/v8-bank`), presentes en el donante pero excluidas aquí según la recomendación. Ningún fallo en `grand_prix_sampler`, `grand_prix_sample_bank` (V10/commons), `ffi`, `tire_scrub`, `state`, `telemetry` ni `synth`.

## Puerta de salida Etapa 1

- [x] Dependencias resueltas localmente (`cargo check --offline` OK).
- [x] Ralentí `3500` admitido en init, capas, playback rate, límites y gate, con tests.
- [x] Carga reproducible sin Formula90s instalado una vez stageados los bancos (hashes en procedencia); el helper de copia con verificación queda para Etapa 5.
- [x] Integridad del import (ronda 16, por contenido SHA-256): `vehicle-audio-engine` idéntico en archivos (0 añadidos/eliminados) con exactamente 3 adaptaciones intencionadas (`build.rs`, `src/dsp_runtime.rs`, `src/grand_prix_sampler.rs`); `v10-engine-synth` pristine (0 cambios). Donante anclado en `f3efcf2` con el mismo diff local (`1441064f…`, 25+/1-); su deriva posterior (dir `cim_2008_ferrari`, un `.import`) queda fuera de la superficie congelada.
- Pendiente para Etapa 2: harness de replay + WAV con perfil Williams, config de mezcla y ambos bancos; paridad contra el mixer donante congelado (stems/eventos con tolerancia registrada).

## Archivos responsables (esta etapa)

- `rust/Cargo.toml`, `rust/Cargo.lock`, `.gitignore`
- `rust/vehicle-audio-engine/` (import + `build.rs`, `src/dsp_runtime.rs`, `src/grand_prix_sampler.rs`)
- `rust/v10-engine-synth/` (import sin activar como fuente continua)
- `implementation/audio_v10_provenance.json`, este informe
- Staging local no versionado: `sounds/banks/v10-v2-bank`, `sounds/banks/commons`, `sounds/sound_mixer_config.json`
