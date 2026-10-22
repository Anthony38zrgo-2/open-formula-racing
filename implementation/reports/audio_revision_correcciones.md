# Correcciones de la revisión de audio V10

Fecha: 22 de octubre de 2026. Cambios locales sobre `codex/initial-port`, HEAD `9c9164869c8210a558abb5e9aa5e760e5eacf5dd`. El checkout ya contenía la implementación de audio y otros trabajos sin commit; se han conservado.

## Correcciones

1. `OpenalSoundInterface.*` y `grsound.cpp`: reservar hasta cuatro fuentes antes de llenar el pool antiguo, cuando existen bancos preparados. `OfpV10Stream::tryActivate` confirma también la disponibilidad de su fuente antes de anunciarse como activo y silenciar las voces anteriores. Si falta una fuente, la activación falla y conserva la ruta anterior.
2. `ofp_v10_stream.*`: lista persistente de buffers libres para encolar desde el primer bloque; retirar todos los procesados antes de calcular la reposición; conservar cada buffer recuperado y verificar los errores de cola. Reponer cuatro bloques de 1024 frames (aproximadamente 93 ms máximos a 44100 Hz). El contador de underruns ya observa la cola después de retirar los procesados.
3. `ofp_audio_presentation.h` y el consumidor: ampliar el anillo y el lote de consumo a 64 pasos, equivalentes a 128 ms a 500 Hz. Si se supera esa capacidad, contar la pérdida y reiniciar mixer/PCM antes de consumir los pasos conservados; ninguna ventana de la secuencia perdida permanece en el mixer.
4. `ofp_v10_stream.cpp` y `rust/open-formula-audio/src/lib.rs`: al cambiar la generación, vaciar PCM, reiniciar el handle Rust con la nueva sesión y consumir sus pasos desde secuencia nueva. Comprobar el resultado de cada envío. El reset también limpia derivadas, fases de recuperación y memoria de roces/impactos; un reset anterior al primer paquete conserva el handle sin vinculación parcial.

La compilación UCRT64 detectó además una conversión implícita de `GetProcAddress` a `void*` que MSVC aceptaba. Se corrigió mediante conversión explícita en `ofp_audio_loader.cpp`.

## Verificación ejecutada

| Comprobación | Resultado |
| --- | --- |
| `cargo test --offline -p open_formula_audio` | 26 tests unitarios y 2 de paridad física/audio correctos; prueba de reset ampliada para comprobar limpieza del adaptador. |
| `python tools/check_cxx_audio.py` | Sintaxis de los nueve grupos C++ y ejecutable de drenaje correctos. El drenaje conserva secuencias y flags del limitador de los 500 pasos de un segundo a 60, 30, 20 y 10 FPS, sin pérdidas. Conserva pruebas de sobrecarga y datos inválidos. |
| `python tools/check_audio_stream.py` | Ejecuta el stream y cargador C++ de producción con la DLL Rust y OpenAL de la distribución, dispositivo null: cola inicial, reciclaje, recuperación, cambio de generación, 25 pasos por refresco, sobrecarga de 65 pasos y rechazo de activación con fuentes agotadas correctos. |
| `python tools/check_audio_source_pool.py --build-dir C:/Users/bill/open-formula-build/engine` | Usa los objetos UCRT64 del backend real. Sin reserva el pool agota las fuentes; con reserva permite crear cuatro fuentes adicionales y la quinta falla como corresponde. |
| `python tools/check_idle_3500.py` | WAV de referencia conservado: fundamental 292 Hz frente a 291.67 esperados, pico 0.446; comprobación de afinación correcta. |
| CMake/UCRT64, targets `snddefault` y `rustphysics` | Compilación y enlace correctos en el árbol de build local; biblioteca Rust release también construida. |

La regresión de stream queda incorporada a `tools/check_audio_gates.py`. La prueba del pool requiere previamente los objetos nativos del build indicado. Los temporales y objetos de las pruebas nuevas se escriben fuera del checkout; el test de drenaje también coloca ahora su objeto en un temporal.

## Límites y entrega

No se han instalado DLL en `distribution`, ni ejecutado una carrera o validado escucha humana. Las pruebas OpenAL usan su dispositivo null: comprueban el transporte real sin emitir sonido, no la experiencia de escucha ni el rendimiento de una carrera.

La profundidad de 64 pasos conserva telemetría a 10 FPS, pero la cola PCM de 93 ms no garantiza reproducción sin underruns a ese framerate ni ante bloqueos largos. El servicio OpenAL independiente sigue sujeto a medición en juego.

Cambiar el tamaño del anillo cambia el layout de `tCarElt`. Antes de instalar o ejecutar el candidato se deben reconstruir todos los consumidores C++ de esa cabecera y desplegar el conjunto coherente; no mezclar módulos antiguos con los recién compilados. Quedan pendientes la sesión Williams/Fuji y la escucha cockpit/exterior, mute, cambios de cámara y reinicios.
