# Etapa 3 — Telemetría física (cerrada, validada)

Fecha: 2026-10-22. Williams/Fuji, backend `rustphysics`.

## Qué se hizo

- **ABI física v5 → v6** (offsets v5 preservados, todo lo nuevo al final):
  - `rust/vehicle-physics-engine/src/open_formula_ffi.rs`: `OFP_ABI_VERSION=6`; `OfpVehicleSpecs` += `max_torque_nm`, `front_torque_split`; `OfpSnapshot` += `clutch_torque_nm`, `tc_cut_ratio_smoothed`, `is_rev_limited`, `shift_timer_s`, `step_dt_s`, `sequence`. Firma `ofp_abi_signature` extendida con los nuevos offsets.
  - `rust/vehicle-physics-engine/src/open_formula.rs`: `sequence` + `last_step_dt` por simulación (se reinician en `reset`); cada paso aceptado avanza el reloj; el snapshot lee el `PowertrainState` existente (par de embrague, TC suavizado, limitador, temporizador de cambio); `vehicle_specs` publica `max_torque` y `front_torque_split`. Sin decisiones acústicas en el integrador.
  - `src/modules/simu/rustphysics/rust_abi.h` espejo v6; `rust_library.cpp` verifica la nueva firma (rechaza mezclas de cabeceras/binarios).
- **Contrato de presentación** `src/interfaces/ofp_audio_presentation.h` (nuevo, listado en `src/interfaces/CMakeLists.txt`):
  - `OfpAudioStepV1` crudo (versión/tamaño, coche, generación de sesión, secuencia, `sim_time`, `dt` real + estado mecánico + 4 ruedas en orden Rust FL,FR,RL,RR + superficies físicas 0..7 + suelo/carrocería). Conversión m/s→km/h exactamente una vez, en el wrapper.
  - Ayudas inline: `OfpAudioTranslateSurface` (Road→asphalt, Grass→grass, Dirt/Gravel/Sand→sand, Curb→rumble/piano, Wall/Metal→asphalt), `OfpAudioOutputShaftHz` (media de motrices × final / 2π, nunca solo RPM), `OfpAudioRingState` + `OfpAudioDrainSteps` sobre el anillo acotado de `tCarElt` (8×552 B, orden y conteo de pérdidas; probado con el ejecutable `tools/check_audio_drain.cpp`).
  - `rustphysics.cpp`: `BuildAudioStep` (mapeo puro snapshot+specs), `publishAudioStep` en cada paso aceptado (tras `publishCar`), `resetAudioSession` en creación/reset (los paquetes viejos no resucitan ventanas de corte). `formulaHud` intacto; `snddefault` nunca toca privados del módulo (drenará la cola en Etapa 4).
- **Adaptador en el wrapper** (`rust/open-formula-audio`: `OfpAudioStepV1` espejo + `ofp_audio_push_step`, ABI sin cambio de versión por ser aditivo):
  - Carga entregada desde par de embrague/referencia con magnitud de retención y TC una sola vez en par positivo (referencia `audio_telemetry.rs` del donante, sin importar el core); signo de transmisión por par transmitido, no por número de marcha; embrague/corte/limitador/TC desde el estado físico real.
  - Derivadas con `dt` (τ 20 ms, `dt` inválido resetea sin picos); fases corte→blip (~120 ms solo en reducciones)→recuperación (40 ms) con los discriminantes de `ShiftPhase` (0/1/2/3/4/5), distinguiendo corte físico de recuperación acústica; el adaptador no aplica par ni blips a la física.
  - Auditoría de consumo (ronda 21): el sampler bifurca cortes en 1/3 y las fases 2/4/5 entran por su rama de recuperación acústica con las mismas condiciones de marcha/carga que el core donante — los enteros emitidos son significativos, no adorno.
  - Frecuencia del eje de salida por contrato del donante (verificado orden de ruedas y diferencial); superficies explícitas con cama dominante por contacto; roce con onset solo en flanco (un apoyo sostenido no dispara impactos por tick).
  - Cabecera C `ofp_audio_abi.h` con el espejo `OfpAudioStepV1` + `ofp_audio_push_step`.

## Validación (esta ronda)

- `cargo check` del workspace OK (física + audio).
- Sonda `cargo run -p vehicle_physics_engine --example ofp_audio_v6_probe`: `seq=2 dt=0.0100` a 60 Hz, `seq=3 dt=0.0500 hz=20` a 20 Hz, campos v6 finitos (`OFP_AUDIO_V6_PROBE_OK`).
- `cargo test -p open_formula_audio --offline`: **14/14 lib** (incl. `step_layout_locked` 552 bytes, TC una sola vez + retención, eje trasero motriz, corte→blip→recuperación, orden/sesión) + **1/1 paridad** `physics_step_parity` (10 pasos a 60 Hz + 1 a 50 ms: paquetes idénticos al snapshot, `last_sequence=11`, `dropped=0`, render audible y `rendered_rpm` junto al RPM físico).
- Tests unitarios nuevos en `open_formula.rs` documentan v6 (no ejecutables aquí: la suite `--lib` del crate físico ya estaba rota antes de esta tarea por rutas donante `../../../data` y renombres — `powertrain.rs`, `suspension_loads.rs`, `simplified_axle_integration.rs`, `legacy_profile/tests.rs` — archivos no tocados; la sonda y la paridad cubren lo añadido).
- Layout v6 bloqueado en tres lados (ronda 14): la sonda fija `specs=400`, `snapshot=2216`, `wheels@328`, cola v6 `2168..2208` (la cola empieza donde terminaba v5: offsets v5 intactos); `rust_abi.h` repite tamaños/offsets con `static_assert` (verificado con MSVC); la firma mezcla los mismos 15+3 campos en ambos lados y en el mismo orden.
- DLL física release verificada por ctypes (ronda 54): 22 símbolos `ofp_*` exportados, `ofp_abi_version()==6`, firma estable `0xeebdad2292b2359d`, procedencia `abi=6 source_commit=f3efcf2… build=9c91648…`.

## Puerta de salida Etapa 3

- [x] Paquetes idénticos al estado físico (paridad: mismos `sequence`/`sim_time`/`dt`, RPM, par, caja, ruedas, suelo) + vuelta con subida 1→2 pedida por cable (ventana de corte vista en snapshots, caja en segunda, 130 paquetes sin pérdidas, mezcla finita y audible).
- [x] Sin pérdida de cortes a FPS bajos (cada paso aceptado lleva su `dt`; limitador/`shift_timer` viajan en el paquete, no en el último frame gráfico; 20 Hz verificado).
  - Límite honesto de granularidad (ronda 22): el integrador resuelve **un paso por frame** (`updateSituation`→`stepCar`→`step`), así que el chatter del limitador dentro de un paso colapsa a su nivel final — exactamente igual que el donante (que también lee un `is_rev_limited` booleano por tick). El wrapper sí preserva N cortes por frame gráfico cuando llegan como N paquetes (harness `06_limiter_windows` con 5×2 ms); si la física subdivide algún día, ningún cambio del contrato hace falta.
- [x] ABI verificada (versión 6 + firma de layout en ambos lados; C++ rechaza mezclas).
- [x] ABI verificada (versión 6 + firma de layout en ambos lados; C++ rechaza mezclas).
- Pendiente: drenaje desde `snddefault` con servicio OpenAL (Etapa 4).

## Archivos responsables

- `rust/vehicle-physics-engine/src/open_formula_ffi.rs`, `src/open_formula.rs`, `examples/ofp_audio_v6_probe.rs`
- `src/modules/simu/rustphysics/rust_abi.h`, `rust_library.cpp`, `rustphysics.cpp`
- `src/interfaces/ofp_audio_presentation.h`, `src/interfaces/CMakeLists.txt`
- `rust/open-formula-audio/src/lib.rs` (`OfpAudioStepV1`, `ofp_audio_push_step`), `Cargo.toml` (dev-dep física), `tests/physics_step_parity.rs`
- `src/modules/sound/snddefault/ofp_audio_abi.h` (espejo del paso)
