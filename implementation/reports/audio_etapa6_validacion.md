# Etapa 6 — Validación integrada (protocolo + pre-validación; pendiente juego y escucha)

Fecha: 2026-10-22. Estado: implementación Etapas 1–5 completa; sin compilador C++/CMake en este entorno (hay Visual Studio instalado pero fuera del PATH y sin `cmake`/`ninja`/`make`), por lo que la compilación del juego, la sesión Williams/Fuji y la escucha humana quedan como trabajo dirigido por este protocolo. Nada de lo anterior se declara terminado.

## Pre-validación técnica ya superada (offline, rondas 1–7)

- Puerta única: `python tools/check_audio_gates.py` → `AUDIO_GATES_OK` (suite+paridad, sonda v6, afinación, bancos, orden ABI, ambas DLL).
- `cargo test -p open_formula_audio --offline`: 26/26 lib + 2/2 paridad física→paquete.
- Sonda física `ofp_audio_v6_probe`: `OFP_AUDIO_V6_PROBE_OK` (seq/dt v6, 60 Hz y 20 Hz).
- Harness `ofp_v10_replay`: 16/16 segmentos audibles, picos 0.43–0.71, `dropped 0`.
- Paridad donante (`audio_paridad_donante.json`): 82 816 líneas, 0 diffs; GrandPrix 58/58 en verde en ambos lados.
- Helper `prepare_audio_banks.py --self-test` OK; copia 34/34 verificada; fallos (faltante/corrupto) → exit 1.
- `cargo check` del workspace y `cargo build --release -p open_formula_audio` (DLL 1,65 MB) OK.
- C++ verificado con MSVC (`cl /Zs`, exit 0 en todos los TUs) + contrato loader↔DLL 22/22/22.
- Afinación del ralentí (`tools/check_idle_3500.py`, evidencia `audio_etapa2/idle_3500_tuning.txt`): fundamental 292.00 Hz (esperado 291.67), ratio 0.7735 vs 0.7723, armónicos x2/x3 presentes, pérdida en mono 0.00 dB, pico 0.446 sin clipping.
- Render en release (`audio_etapa2/render_bench.txt`, mixer aislado, 1 coche): p50 ~338 µs, p95 ~533 µs, p99 ~585 µs, max ~697 µs frente a deadline 23 220 µs (margen ~38×; ~4 coches caben con holgura). Memoria decodificada, asignaciones en render, fuentes OpenAL y coste multicoches en juego deben medirse en sesión (ver abajo).

## Estado del build del juego (ronda 8: intentado y bloqueado con lista exacta)

- Toolchain local: MSVC 19.44 + Windows SDK 10.0.26100 + CMake 4.4.4 (pip) + Ninja 1.13.2 — todos funcionales (los TUs de Etapa 4 se verificaron con ellos).
- `cmake -S src -B build -G Ninja` llega a la resolución de dependencias y para en la primera ausente: **SDL2**. Puertas `FATAL_ERROR` restantes en `cmake/thirdpartydeps.cmake`: SQLITE3, plib, OSG, tinygltf, SDL2_mixer, OpenGL, OpenAL, OGG, VORBIS (+ resto del árbol). Todas se construyen vía `packaging/3rdParty-devel` (descarga+build, CI orientado a MinGW) — fuera del alcance de estas rondas.
- Además: `src/CMakeLists.txt` carece de `cmake_minimum_required` (CMake 4.x lo exige; con 3.x era advertencia). Quien configure deberá añadir `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` o el mínimo al proyecto.
- Prerrequisito exacto de la sesión: stack 3rdParty construido + configure + build + install, o una máquina con el juego ya compilado.

## Matriz de cobertura (offline verificado vs pendiente en juego)

| Aceptación (GOAL) | Offline | En juego |
|---|---|---|
| 18+12 assets, hashes/frames/formatos, config, fuente | ✅ helper + tests | carga UCRT64 en juego |
| Contrato loader↔DLL + DLL real por ctypes (31 checks) | ✅ `check_audio_dll.py` | carga en juego |
| Rutas espacios/no ASCII, ausente, corrupto, ABI incorrecta | ✅ tests ABI | — |
| Ralentí 3500 afinado (292 Hz, ratio 0.7735) y audible | ✅ `check_idle_3500.py` | escucha |
| Transición 3500→5600, ON/OFF, retención sin huecos bajo 4500, salida medio a 13000, 17500 máx, silencio a motor 0 | ✅ tests + harness | escucha |
| Downmix mono exacto (gemelos deterministas, muestra a muestra) + 0.00 dB en idle | ✅ test + análisis | — |
| Caja N↔1/±, reversa, retención, embrague abierto/cerrado | ✅ harness (16 segmentos) + tests | escucha |
| Limitador multi-corte por frame y audible solo en ventana física, TC una sola vez | ✅ tests (gemelos) + harness | escucha |
| Commons: hierba/piano/arena, scrub por rueda (superficie de la que patina), 4 impactos, barrera/cono, roce sin repetición | ✅ harness + tests | escucha |
| Flanco de impacto→barrera una vez + cooldown (propiedad única en wrapper) | ✅ test gemelos | umbral 50 N·s en escucha |
| Orden/sesión, datos antiguos, pausa/reanudar, cámaras, reinicios | ✅ tests | sesión |
| Cola/anillo lleno (overrun contado), rebase de sesión, slot corrupto | ✅ `tools/check_audio_drain.cpp` (ejecutable MSVC) | — |
| p50/p95/p99 mixer aislado (~38×) | ✅ bench release | p95/p99 en juego + memoria + fuentes + multicoches |
| Paridad donante (82 816 líneas, 0 diffs; GrandPrix 58/58) | ✅ | stems en sesión con tolerancia |
| Mute/volumen, distancias, dobles voces, clipping | parcial (diseño) | escucha + medición |
| Escucha cockpit/exterior | — | **requiere humano** |

## Protocolo de sesión (Williams/Fuji, `rustphysics`)

Tripleta verificada presente en el árbol instalado (ronda 65): sesión `williams-fuji-practice.xml` (Practice, 999 vueltas), pista `fuji-seventy-six-seventy-seven`, coche `williams-nineteen-ninety-seven` (piloto humano) — su `_carName` contiene «williams» (puerta V10) y conserva la sección `Sound` legacy como fallback. Parrilla de 1 solo coche (ronda 67): primera activación sin contención multi-V10 (tope 4). Backend ya seleccionado (ronda 80): `distribution/user-settings/config/raceengine.xml:454` fija `simu=rustphysics` (el corredor usa ese `-lc`); hay un `runtime.log` previo con `Rust physics: loaded …vehicle_physics_engine.dll`.

1. Instalar: construir el juego, instalar `open_formula_audio.dll` en `modules/sound` (CMake Etapa 5) y ejecutar `tools/prepare_audio_banks.py --source <checkout-con-bancos> --dest <datos>/sounds`; guardar `audio_install_receipt.json` en `implementation/reports/`.
2. Sesión instrumentada: `pwsh -File tools/run_rust_integration.ps1 -RequireAudio` (conduce solo con presencia humana: el script enfoca ventanas y envía teclas; jamás desatendido). El gate evalúa `audio_gate.txt`: `loaded` + `car N activated` = PASS; cualquier fallo duro = FAIL.
2. Verificar carga desde UCRT64: arrancar, leer el log — exigir `Open Formula audio: loaded …`, `car N activated (bank …, mixer …)`, versión física v6 y firma ABI sin rechazos. (Mitad estática ya probada en Etapa 5: el juego es MinGW UCRT64 y las DLL Rust no importan ningún CRT en conflicto; aquí falta el `LoadLibrary` real en el proceso del juego.)
3. Sesión: Williams en solitario (la parrilla del XML trae 1 piloto; sin rivales en la primera activación), `rustphysics`, 10 vueltas de prueba: salida, aceleración/retención por vuelta, 2 N↔1 en boxes, 1 tanda sobre la hierba lateral (20 m) y 1 roce contra el muro. Superficies de Fuji 76-77 verificadas en su XML (ronda 78): asfalto, hierba y muro — no hay piano ni arena, así que esos beds solo se cubren offline (harness); en sesión mandan hierba (bed+scrub) y muro (barrera+roce, sin bed por diseño).
4. Capturar por vuelta: log con `steps dropped/underruns/renders`, picos/RMS del bus (medir fuera del juego si hay grabación), y los WAV offline de referencia para comparar stems con tolerancia registrada (no equiparar mixes con distinta config).

## Aceptación (mapeo a la lista del GOAL)

- Carga/errores: 18+12 assets, hashes/frames/formatos, config efectiva, fuente, rutas con espacios/no ASCII, ausente, corrupto, ABI incorrecta → cubierto offline salvo carga UCRT64 (aquí).
- RPM 3500 sin clamp (fundamental 292 Hz y ratio 0.7735 medidos), transiciones ON/OFF, salida del medio a 13000, 17500 máx, silencio a motor 0 → offline OK salvo escucha del ralentí (requiere humano).
- Caja, limitador multi-corte por frame, TC una sola vez, commons (4 impactos, barrera/cono, roce sin repetición), tiempo/hilos (pausa/reanudación, cámaras, reinicios) → parte offline OK; resto en sesión.
- Rendimiento: p50/p95/p99 de mixer aislado en release ya medidos con margen ~38×; memoria, fuentes OpenAL y multicoches → medir en sesión (instrumentar `render_calls`/`underruns` del log + picos del mixer).
- Escucha: cockpit y exterior, capas sin clicks, balance caja/commons, mute/volumen, distancias, sin dobles voces, pico/RMS y clipping → **requiere humano**; la verificación de hashes no la sustituye.

## Criterio de terminado

Solo cuando la sesión Williams/Fuji complete el protocolo con pruebas técnicas y escucha humana registradas en `implementation/reports/`. Hasta entonces el objetivo sigue activo.
