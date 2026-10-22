# Etapa 2 — Contrato y render offline (cerrada, validada)

Fecha: 2026-10-22. Williams/Fuji, backend `rustphysics` como primera activación (este harness es previo a la telemetría física de Etapa 3).

## Qué se hizo

- Nuevo crate `rust/open-formula-audio` (`cdylib` + `rlib`, depende del crate importado):
  - Handle opaco posee el mixer (`VehicleAudioEngine`). Un solo hilo toca el handle mutable. Pánicos capturados; nada cruza la ABI.
  - `OFP_AUDIO_ABI_VERSION=1`, `SAMPLE_RATE=44100`, `MAX_BLOCK=4096`. Firma de layout y procedencia estáticas.
  - `ofp_audio_create(assets_root, params)`: carga commons + `sound_mixer_config.json` vía `VehicleAudioEngine::new`, activa V10 con `enable_grand_prix_sampler` y comprueba rango físico (idle 3500 / max 17500). Fallo V10 con `fallback_to_legacy==0` falla cerrado (null + `last_create_error`); con fallback explícito conserva legacy con `v10_active==0`.
  - `ofp_audio_push_packet`: consume paquetes temporizados completos (versión/tamaño, coche, generación de sesión, secuencia, `sim_time`, `dt`) y distribuye en orden a `set_grand_prix_mechanical_state` + `set_telemetry_timed` + `set_scrape_state` + `set_tire_scrub_state`. Secuencia antigua o sesión distinta se descarta, cuenta `dropped_packets` y no ingiere (evita limitador atascado).
  - `ofp_audio_render` (estéreo) / `ofp_audio_render_mono` (downmix `(L+R)*0.5` para exterior), `ofp_audio_diagnostics` sin asignar en éxito, `ofp_audio_trigger` (commons one-shots con ruteo single-owner al sampler), `ofp_audio_reset` / `reset_session`, consultas `bank_id` / `bank_sha256` / `mixer_hash` / `last_error`.
  - Tests de layout bloquean offsets/tamaños (`CreateParams:136`, `Packet:224`, `Diagnostics:112`).
- Cabecera C versionada `src/modules/sound/snddefault/ofp_audio_abi.h` para el cargador de Etapa 4.
- Harness `rust/open-formula-audio/examples/ofp_v10_replay.rs`: perfil Williams (gains de `audio.json`, caja `4.65/21/46`, idle 3500/max 17500), bloques de 1024 frames (referencia 4×1024 ≈ 93 ms de cola), 15 segmentos + `full_mix.wav` + `replay_manifest.json` en `implementation/reports/audio_etapa2/`.

## Validación (esta ronda)

- `cargo test -p open_formula_audio --offline --lib`: **10/10 OK** (layouts, V10 activo + procedencia, versiones/tamaños, orden/sesión, overflow render, mixer hash + bank SHA, trigger inválido, rutas con espacios/no ASCII sin pánico, manifest corrupto fail-closed/fallback explícito).
- Harness: `V10 active`, 15/15 segmentos audibles sin silencio ni clipping:
  - `01_idle_3500` peak 0.446 rendered 3500 gate 1.0 (sigue 3500, no 4500).
  - `02_on_ramp` 3500→17500, `03_off_retention` 14000, `04_N↔1`, `05_upshift`, `05b_downshift`, `06_limiter` (5×2 ms dentro de un frame gráfico), `07_grass`/`08_piano`/`09_arena` + scrub por rueda, `10_hit1`/`11_hit2`/`12_barrier`/`13_cone`, `14_scrape` sostenido. Picos 0.43–0.66, `dropped 0`, eventos aceptados 0→5 según segmento.
  - WAV estéreo PCM16 44100 Hz verificados (`wave` params), `full_mix.wav` 4 579 372 bytes.
- Reloj de eventos: `dt` real del paso preserva ventanas de corte a FPS bajos (limiter 5×0.002 s); secuencia monótona por coche/sesión; datos antiguos descartados con diagnóstico.

## Puerta de salida Etapa 2

- [x] V10 activo confirmado (`v10_active==1`, `continuous_source==1`, bank/mixer hashes).
- [x] Todos los grupos audibles (ON/idle/5 capas, 3 OFF, up/downshift, backfire, limiter, transmisión continua, 4 hits + barrera/cono + N↔1 + hierba/piano/arena + scrub + roce).
- [x] Reloj de eventos correcto (paquetes temporizados, sin reemplazo por último frame gráfico).
- Pendiente paridad fina contra mixer donante congelado por stems (Etapa 6, tolerancia numérica registrada) y escucha humana cockpit/exterior.

## Archivos responsables

- `rust/open-formula-audio/Cargo.toml`, `src/lib.rs`, `examples/ofp_v10_replay.rs`
- `src/modules/sound/snddefault/ofp_audio_abi.h`
- `rust/Cargo.toml` (miembro añadido)
- Evidencia: `implementation/reports/audio_etapa2/*.wav`, `replay_manifest.json`
