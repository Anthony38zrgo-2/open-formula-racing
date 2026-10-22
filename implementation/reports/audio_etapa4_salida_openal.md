# Etapa 4 — Cargador y salida OpenAL en `snddefault` (cerrada, revisión sin compilador)

Fecha: 2026-10-22. Primera activación: Williams + `rustphysics`, vía OpenAL existente.

## Qué se hizo

- **Cargador** `src/modules/sound/snddefault/ofp_audio_loader.{h,cpp}`: patrón `rustphysics` (LoadLibrary/GetProcAddress), verifica ABI v1 + firma de layout + 44100 Hz, fail-closed con causa única registrada. Rust nunca abre dispositivos.
- **Stream** `src/modules/sound/snddefault/ofp_v10_stream.{h,cpp}` (un coche = un handle + una fuente AL + 4 buffers):
  - `tryActivate`: puerta Williams (nombre) + primer paso válido del anillo (puerta física) + creación con perfil explícito; solo éxito con V10 confirmado (sin mezcla legacy silenciosa).
  - `update` (hilo de refresco, dueño del contexto; el contexto nunca se mueve): drena pasos en orden, `push_step`, top-up con ≥2 buffers en cola y ≤3 bloques por refresco (4×1024 @44100 ≈ 93 ms máx). Contadores de underruns/drops/renders con log.
  - Cockpit (<3 m): estéreo relativo al listener. Exterior: mono (`render_mono`, downmix verificado) en fuente posicional. Pitch base 1 siempre (el sampler lo resuelve); Doppler exterior una sola vez con la fórmula radial legada auditada (`SoundSource::update`; Doppler OpenAL desactivado, LOD/synth Rust apagados). Ganancia 1.0 por fuente: volumen/mute global aplican una sola vez vía listener + pausa.
  - Pausa/mute/discapacitado: fuente parada, sin render (el mixer se congela; el suavizado empalma al volver; el overrun del anillo se cuenta).
- **Exclusión de efectos antiguos**: `CarSoundData::v10_active` silencia motor, axle/turbo, cambios, backfire y beds commons en `update()` (las colas de máximos los excluyen solas); `OpenalSoundInterface` para el motor legacy del coche (devuelve la fuente al pool) en vez de poner su volumen a cero. N↔1 conserva su capa commons dentro del mixer; el sampler posee los eventos (sin doble envío marcha-diferencia/fase). Motor detenido: sin ralentí residual; superficies solo vía mixer si rueda.
- **Presupuesto verificado en log real** (ronda 81, `distribution/user-settings/runtime.log` previo): backend OpenAL con 256 fuentes y 1024+ buffers — 1 fuente + 4 buffers por coche V10 (tope 4) cabe con holgura.
- **`grsound.cpp`**: init del loader + streams tras el pool compartido (solo OpenAL; PLIB/discapacitado intactos), refresco con activación perezosa y tope de 4 streams, mute propaga pausa, shutdown destruye antes que el dispositivo. Teardown V10 idempotente (ronda 17): re-init entre carreras sin shutdown intermedio no fuga handles/streams/fuentes, y un cambio de modo a DISABLED también libera. Reversión explícita: cualquier fallo deja el mix legacy intacto.
- **Muestra legacy de Williams retenida a propósito** (ronda 28): `grInitSound` sigue creando su `engine sample` aunque el V10 vaya a activarse — si el backend falla en cerrado, el coche nunca queda sin motor; con V10 activo su fuente estática queda parada en el pool (coste: una fuente estática por coche V10, despreciable frente al presupuesto probado).
- **Contrato**: `ofp_audio_abi.h` movida a `src/interfaces/` como definición única (los includes `"ofp_audio_abi.h"` siguen resolviendo por el include path de interfaces); `ofp_audio_presentation.h` sin duplicados + `static_assert(552)`; `tCarElt` con anillo `audio_steps[8]` + contadores (fuera de `formulaHud`); `rustphysics.cpp` publica en el anillo y renueva sesión; CMake de ambos módulos actualizado.

## Decisión de transporte (registrada antes de activar por defecto)

Top-up síncrono en el refresco; **no** se crea un hilo propietario OpenAL en esta etapa. Una parada prolongada del hilo que alimenta agota los ~93 ms: el contador de underruns lo demuestra. Solo si la Etapa 6 mide underruns habituales se traslada el servicio OpenAL a un hilo propietario con comandos; nunca más buffer indefinido ni contexto compartido concurrente.

Cotas de latencia informadas (ronda 28, sin promesas): anillo de 8 pasos ≈ 133 ms de edad máxima a 60 Hz + cola AL de 93 ms ⇒ peor caso evento→oído ~226 ms si todo se atasca al máximo; caso típico (1 paso en anillo + 2–3 buffers) ≈ 50–90 ms. La Etapa 6 debe medir las reales en sesión.

## Validación (ronda 4: verificación sintáctica con MSVC real)

- Suite Rust intacta: `open_formula_audio` **14/14 lib + 1/1 paridad**; `cargo check` del workspace OK.
- C++ verificado con el compilador MSVC 19.44 (`cl /Zs`, sin enlazar) contra las cabeceras reales del proyecto (stubs solo para terceros: OpenAL/PLIB/`config.h`, en `%TEMP%/ofp-stubs`):
  - `ofp_audio_abi.h` + `ofp_audio_presentation.h` (incl. `static_assert(552)`), `ofp_audio_loader.h/.cpp`, `ofp_v10_stream.h/.cpp`, `grsound.cpp`, `CarSoundData.cpp`, `OpenalSoundInterface.cpp`, `rustphysics.cpp`, `rust_library.cpp` → **exit 0 en todos**.
  - La verificación cazó y corrigió un bug real que habría roto la compilación: la macro `OFP_RESOLVE` confundía el nombre del símbolo DLL con el miembro (`decltype` resolvía al tipo función declarado). Reescrita al patrón tabla de `rust_library.cpp`.
  - Avisos restantes: ninguno propio (un C4244 preexistente en `rustphysics.cpp:805`; los `C4273/C4530/C4834` solo aparecen sin las defines reales `SNDDEFAULT_DLL`/`TRACE_OUT`/EH).
- Contrato loader↔DLL probado en ambos sentidos (ronda 5): `dumpbin /EXPORTS` confirma los 22 símbolos en `open_formula_audio.dll`; la firma embebida coincide byte a byte con la esperada por el loader; cabecera↔loader↔Rust 22/22/22 sin faltantes. Tamaños C++ bloqueados con `static_assert` (136/224/112/552); orden de campos verificado idéntico con la puerta permanente `tools/check_audio_abi_order.py`: 4 structs de audio (22+36+21+34) + 6 de física v6 (3+29+7+8+38+48), `ABI_ORDER_OK`.
- Auditoría negativa (ronda 15, por búsqueda): `alcOpenDevice`/`alcCreateContext`/`alcMakeContextCurrent` solo existen en el `OpenalSoundInterface.cpp` preexistente — ningún archivo nuevo abre dispositivos ni mueve contextos. `OFP_ABI_VERSION` solo se define en `rust_abi.h` y se comprueba en `rust_library.cpp` (sin consumidores v5 rezagados). El anillo `audio_steps`/`audio_step_write`/`audio_session` tiene exactamente un productor (física: crea→publica→renueva) y un consumidor (stream: puerta→bind→drena), con el contrato en `car.h`.
- Puerta de salida: reproducción estable **no verificada en juego** (sin CMake/ninja ni terceros en este entorno; el enlace completo y la prueba en UCRT64 quedan para Etapa 6); reversión explícita y no-duplicación implementadas y verificadas sintácticamente; servicio independiente decidido (no requerido por ahora).

## Archivos responsables

- `src/modules/sound/snddefault/ofp_audio_loader.{h,cpp}`, `ofp_v10_stream.{h,cpp}`, `grsound.cpp`, `CarSoundData.{h,cpp}`, `OpenalSoundInterface.{h,cpp}`, `CMakeLists.txt`
- `src/interfaces/ofp_audio_abi.h` (movida), `ofp_audio_presentation.h`, `car.h`, `CMakeLists.txt`
- `src/modules/simu/rustphysics/rustphysics.cpp` (anillo en `tCarElt`)
- DLL `open_formula_audio.dll` e instalación de bancos: Etapa 5.
