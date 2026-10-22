# Etapa 5 — Preparación/instalación (cerrada, verificada)

Fecha: 2026-10-22. Destino instalado: `<datos>/sounds/` (`banks/commons`, `banks/v10-v2-bank`, `sound_mixer_config.json`), resuelto desde el directorio de datos del motor, nunca desde el cwd.

## Qué se hizo

- **Helper** `tools/prepare_audio_banks.py` (`--source` explícito, `--dest`, `--receipt`, `--self-test`):
  - Copia únicamente assets declarados (18 WAV V10 + 12 commons + 2 manifests + mixer config + `source_inventory.json` = 34 archivos; sin `.import`/`.gdignore`, sin remasterizar).
  - Verifica SHA-256 de cada WAV contra su manifest (incluidos frames del manifest V10), formato PCM mono 44100/16, los 3 manifests/config contra hashes fijados y `source_inventory.json` contra el hash que el propio manifest V10 declara para él.
  - Falla ante faltantes/incompatibles (probado: exit 1 en ambas clases) y escribe `audio_install_receipt.json` con hashes de assets y del código (`git HEAD`).
  - Verificado de extremo a extremo: 34 archivos, 19 355 355 bytes, receipt con `code_head 9c9164869c82`.
- **Corrección de procedencia**: el hash del manifest V10 en `audio_v10_provenance.json` tenía un error de transcripción (`...2cbbe38...`); fijado al valor medido (`...2cbee38...`, verificado por programa contra el donante).
- **CMake Cargo/instalación** (`src/modules/simu/rustphysics/CMakeLists.txt`): el workspace se compila una sola vez en el módulo de física (miembros`vehicle-physics-engine`, `vehicle-audio-engine`, `v10-engine-synth`, `open_formula_audio`); BYPRODUCTS incluye ambas DLL; `open_formula_audio.dll` se instala en `modules/sound` (donde el loader la resuelve). `snddefault/CMakeLists.txt` documenta que no debe invocar Cargo (sin builds simultáneos redundantes).
- **DLL verificada**: `cargo build --offline --release -p open_formula_audio` → `target/release/open_formula_audio.dll` (1,65 MB).
- **Coherencia UCRT64, mitad estática (ronda 26, `dumpbin /IMPORTS`)**: el juego es MinGW-w64 UCRT64 (`speed-dreams-2.exe` → `libgcc_s_seh`/`libstdc++-6`/`api-ms-win-crt-*`); `open_formula_audio.dll` y `vehicle_physics_engine.dll` solo importan `KERNEL32`/`ntdll`/`api-ms-win-core-synch`, **cero** `msvcrt`/`ucrtbase`/`VCRUNTIME` — sin conflicto de CRT posible. Falta la mitad dinámica: carga real en el proceso del juego (Etapa 6).
- **Selección de backend por coche**: puerta Williams (nombre) + primer paso válido del anillo (puerta física) + V10 confirmado, con tope de 4 streams; el resto conserva el audio existente. Fallo = legacy intacto (reversión explícita). `tools/prepare_williams_vehicle.py` conserva su sección legacy de un WAV como fallback hasta la activación por coche.
- **Fallback legacy verificado instalado** (ronda 66): `engine_powered_maximum_loop.wav` en el modelo instalado coincide con el hash fijado `9bec0ea0…` y su `rpm scale` del XML; la reversión tiene su asset.

## Puerta de salida Etapa 5

- [x] Distribución autónoma: helper + receipt con hashes de assets/código; DLL correcta construida.
- [x] Perfil instalado conserva ganancias motor/caja/coast/limitador y relaciones físicas (ronda 76, verificado por programa: las 8 ganancias del stream coinciden con `audio.json` y ratios/final/idle/max con `powertrain.json`).
- [x] Raíz instalada `data/sounds` con ambos bancos y config; resolución desde datos del motor (`GfDataDir()`/`GfLocalDir()` + sufijo, nunca cwd; verificado en `tgf.cpp:makeRunTimeDirPath` que normaliza con `/` final, como el resto del código).
- Pendiente: ejecución del helper contra el árbol instalado + carga UCRT64 en juego (Etapa 6).

## Archivos responsables

- `tools/prepare_audio_banks.py`
- `src/modules/simu/rustphysics/CMakeLists.txt`, `src/modules/sound/snddefault/CMakeLists.txt`
- `implementation/audio_v10_provenance.json` (hash corregido)
