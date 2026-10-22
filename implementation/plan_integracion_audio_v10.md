# Integración del banco V10 y commons en Open Formula

Fecha: 22 de octubre de 2026. Estado: evaluación y propuesta; no se ha implementado código de producción, copiado bancos ni compilado un candidato.

## Recomendación

Reutilizar `vehicle-audio-engine` de Formula90s mediante una biblioteca Rust con ABI C propia para Open Formula, consumida desde `snddefault` en C++. Conservar OpenAL como salida. El crate resuelve la reproducción, las mezclas y los eventos; C++ conecta simulación, cámara, ciclo de vida y transporte PCM.

Es la ruta con menos duplicación de comportamiento. Importar únicamente los WAV al reproductor actual perdería las capas ON/OFF, curvas de presencia, eventos temporizados y transmisión. Importar Godot, `formula90-core` o `game-sim` completos incorporaría coordinación y dependencias ajenas al port.

Primera activación: Williams/Fuji con backend `rustphysics`. La finalización incluye motor completo y commons; su activación por etapas sirve para verificar cada grupo. Los demás coches conservan el audio existente hasta tener perfil y telemetría compatibles.

## Evidencia de partida

- Destino: `D:/open-formula`, rama `codex/initial-port`, HEAD `9c9164869c8210a558abb5e9aa5e760e5eacf5dd`. Hay informes/capturas sin seguimiento anteriores a esta tarea; preservarlos.
- Origen: `D:/Formula90s`, HEAD `f3efcf246c2a2abf62477037ed0243f2f9a93a9b`. Tiene cambios locales, incluidos `game/crates/vehicle-audio-engine/src/grand_prix_sampler.rs`, un test de `formula90-core` y binarios. HEAD solo no identifica el código de audio que se está evaluando.
- SHA-256 actual de ese `grand_prix_sampler.rs`: `1441064fe5b1868e4794417b97c8d403e047480181730956f465a0089c8480b2`.
- `tools/prepare_williams_vehicle.py:510` copia actualmente solo `engine_powered_maximum_loop.wav`; el XML selecciona una muestra y `rpm scale`. No importa el banco completo.
- `rust/data/vehicles/williams-fw19-hockenheim/audio.json` ya selecciona `grand_prix_sampler` y la ruta V10 correcta, pero esa configuración no demuestra consumo por `snddefault`.
- `rust/Cargo.toml` contiene únicamente `vehicle-physics-engine`. El patrón DLL Rust + cargador C++ versionado ya existe en `src/modules/simu/rustphysics/`.
- `grsound.cpp` crea el backend, muestras por coche y efectos compartidos; `grRefreshSound` actualiza desde `tCarElt` y cámara. `OpenalSoundInterface.cpp` posee o reutiliza el contexto OpenAL y aplica listener/volumen global.
- La ABI física propia es v5. `OfpSnapshot` expone RPM, par motor, embrague, ruedas y contactos, pero no todo el estado mecánico que consume el audio de Formula90s.

Validación realizada en esta evaluación: los hashes de los 18 WAV V10 y los 12 WAV commons coinciden con sus manifests. Los 18 WAV V10 son PCM entero, mono, 44100 Hz y 16 bits; sus frames coinciden con el manifest. Los 30 WAV ocupan 19 272 750 bytes. Esa cifra es almacenamiento, no memoria del mixer: la decodificación a flotante y los niveles filtrados de transmisión consumen más.

No se ha probado carga mediante Rust, compilación, rendimiento, salida OpenAL ni escucha en el juego.

## Inventario que debe conservarse

El manifest V10 declara seis entradas de reproducción principal: ralentí y cinco capas con acelerador; además tres capas de retención, ocho muestras de eventos y una muestra continua de transmisión. Son 18 WAV, no el banco histórico de 13 assets.

| Grupo | Muestra | Cobertura declarada, RPM |
| --- | --- | --- |
| Ralentí | engine_idle_loop.wav | 4500–5600 |
| ON bajo | engine_powered_low_loop.wav | 4800–11500 |
| ON medio | engine_mid_on_loop.wav | 9500–13000 |
| ON medio alto | engine_powered_medium_high_loop.wav | 12000–16000 |
| ON alto | engine_powered_high_loop.wav | 14500–17500 |
| ON máximo | engine_powered_maximum_loop.wav | 16500–18000 |
| OFF bajo | engine_low_off_loop.wav | 4500–11500 |
| OFF medio | engine_mid_off_loop.wav | 9500–16000 |
| OFF máximo | engine_off_maximum_loop.wav | 14000–18000 |
| Eventos | upshift_event, downshift_event, backfire_burst_3 a 7, limiter_cut_loop | Política de eventos del manifest |
| Transmisión | transmission_whine_loop.wav | Eje de salida/carga, no una banda adicional de RPM |

Preservar bytes, referencias acústicas, curvas `presence_curve`, crossfades, ganancias calibradas y grupos. El medio no contribuye desde 13000 RPM. No sustituir las referencias acústicas por los límites de cobertura.

Commons contiene cuatro impactos `impact_hit_1` a `4`, barrera, cono, roce metálico, contacto N↔1, hierba, piano, arena y `tyre_scrub`: 12 WAV. Mantener separado `bank_manifest.json` de commons y `manifest.json` del V10. No importar archivos Godot `.import` ni convertirlos a recursos del motor.

Conservar también `source_inventory.json`, atribuciones y un registro del origen. Revisar las condiciones declaradas antes de una distribución pública: ambos crates Rust indican `LicenseRef-Proprietary` y el código anfitrión conserva GPL-2.0-or-later. Esta evaluación técnica no establece autorización de redistribución.

## Reutilización y dependencias reales

Origen principal: `D:/Formula90s/game/crates/vehicle-audio-engine`. Es Rust sin dependencia de Godot y ya ofrece `rlib` y `cdylib`. Usa serde, serde_json, thiserror, sha2 y el crate vecino `v10-engine-synth`.

Para la primera importación, incluir también `v10-engine-synth` sin activarlo como fuente continua. Así no se mezcla el port con un refactor grande de dependencias. Una extracción futura mediante features sería otro trabajo, después de verificar paridad del sampler.

No hace falta `f90_audio_dsp.dll` para la ruta `grand_prix_sampler`; mantener desactivadas las capas GF509 y DSP C++ del donante. Existe, sin embargo, un `include_str!("../../../BUILD_SOURCE")` en `dsp_runtime.rs`: copiar el crate a `rust/vehicle-audio-engine` sin adaptar esa ruta rompe la compilación incluso si el DSP no se activa. Adaptar su contrato de procedencia al destino y evitar una dependencia artificial del archivo de otro repositorio. Revisar también `build.rs`, cuyo seguimiento de `.git/HEAD` asume la estructura del donante y no cubre por sí solo cambios del ref de la rama ni fuentes sucias.

Copiar/resolver explícitamente `sounds/sound_mixer_config.json`. El mixer lo busca dos niveles por encima de commons. Omitirlo permite defaults y puede cambiar el resultado aunque estén todos los WAV. Registrar su hash y el perfil acústico efectivo.

Congelar una instantánea importada con revisión de origen, hashes de fuentes, manifests y diff local incluido. No usar dependencias Cargo absolutas hacia `D:/Formula90s` ni copiar sus DLL ya compiladas. Ajustar los tests que resuelven fixtures mediante rutas de la estructura antigua.

## Adaptación obligatoria del ralentí a 3500 RPM

Williams declara 3500 RPM de ralentí y 17500 RPM máximas. El banco declara 4500–18000 RPM. La integración debe adaptarse para aceptar y reproducir correctamente el ralentí de 3500 RPM, conservando ese valor en la física y en el perfil del coche. Es un requisito de implementación y aceptación.

El cambio local existente en Formula90s elimina el rechazo de mínimos inferiores a la cobertura, mantiene validación superior y añade tests para 3500 RPM. La reproducción ya limita las RPM acústicas al rango del banco, por lo que a 3500 RPM físicas utiliza 4500 RPM acústicas. No extiende la afinación del sample hasta 3500.

Adaptación requerida: ampliar la cobertura efectiva del sampler hasta 3500 RPM, tanto en la validación de inicialización como en la selección de capas, cálculo de playback rate, límites de reproducción y gate del motor. A 3500 RPM físicas, la reproducción debe seguir esas RPM; mantener el clamp acústico a 4500 RPM no satisface el requisito.

Evaluar primero la reutilización del loop de ralentí con su referencia acústica real: 3500 / 4531.6387 ≈ 0.7723 de playback rate. Validar afinación, timbre y continuidad antes de ampliar sus metadatos de cobertura y límites de reproducción. Revisar también la mezcla de retención por debajo de 4500 RPM para que no haya huecos o saltos al soltar el acelerador. Si la muestra no admite esa extensión con calidad suficiente, preparar una variante específica de ralentí y actualizar de forma reproducible sus metadatos y procedencia.

Conservar intacto el banco donante: cualquier cambio de muestras o metadatos se realiza en una variante local identificada para Open Formula, con hashes y herramienta de preparación actualizados. No basta con reducir un límite del manifest. La transición 3500→4500→5600 debe ser continua, el motor detenido debe terminar en silencio y debe conservarse la validación del límite superior del banco.

## Arquitectura y contratos

Flujo propuesto:

`pasos físicos aceptados → paquetes de audio ordenados → cola acotada → propietario del mixer Rust → PCM → buffers OpenAL → dispositivo existente`

La cámara alimenta posición, velocidad, orientación y modo de escucha por un canal de estado independiente. No gobierna los eventos mecánicos.

### Frontera física

Crear un contrato de presentación de audio en `src/interfaces/`, publicado por `rustphysics` en cada paso aceptado. No sobrecargar `formulaHud`, no consultar estructuras privadas del módulo desde `snddefault` y no añadir decisiones acústicas al integrador físico.

El paquete necesita versión/tamaño, coche y generación de sesión, secuencia, tiempo de simulación y duración real del paso. El estado continuo puede agruparse; las ventanas de corte y transiciones no pueden reemplazarse por el último estado del frame gráfico. Una cola acotada por coche debe preservar orden y contabilizar pérdidas. Al desbordar o detectar datos antiguos: descartar de forma controlada, cerrar ventanas activas y registrar diagnóstico, evitando un limitador atascado.

| Dato de audio | Fuente/contrato |
| --- | --- |
| RPM y límites | `OfpSnapshot.engine_rpm`, `OfpVehicleSpecs.idle_rpm/max_rpm`; ya son RPM. `_enginerpm` de Speed Dreams usa rad/s y requiere otra conversión si se usa como fallback. |
| Acelerador, velocidad, marcha actual/objetivo | Snapshot físico; m/s→km/h una vez. |
| Carga entregada | Par de embrague y par de referencia del perfil; conservar magnitud de retención y aplicar TC una sola vez, siguiendo `audio_telemetry.rs`. No sustituir carga por pedal. |
| Signo de transmisión | Par transmitido por el embrague; no deducirlo del número de marcha. |
| Embrague, corte de cambio, limitador y TC | Estado real de `PowertrainState`, no comparaciones acústicas de RPM o pulsaciones del mando. |
| Frecuencia del eje de salida | Media de velocidad angular de ruedas motrices × relación final / 2π, siguiendo el contrato del donante; verificar orden de ruedas y diferencial. |
| Superficie y neumáticos | Contacto por rueda, velocidad de deslizamiento, carga, fricción y referencia de carga para `set_tire_scrub_state`; traducir superficies explícitamente. |
| Fondo y carrocería | Impulsos/contactos, velocidad tangencial e inicio/fin de roce. Un apoyo sostenido no debe disparar impactos cada tick. |

Extender el snapshot físico o añadir una consulta específica para par de embrague, estado de cambio/corte y TC, leyendo el estado ya existente. Toda modificación de layout actual exige incrementar ABI, actualizar cabeceras/firmas y reconstruir consumidores. La captura de cortes debe hacerse a la frecuencia en que se resuelven, no después de perder subpasos.

Extraer el pequeño adaptador de `formula90-core/src/audio_telemetry.rs` como referencia probada: derivadas con dt, carga, recuperación acústica y mapeo de fases. No importar el core completo. Distinguir corte físico de recuperación acústica; el adaptador no aplica par ni blips a la física.

### Frontera Rust/C++

Añadir `rust/open-formula-audio` como `cdylib`, dependiente del crate importado. Su handle opaco posee el mixer. La ABI C v3 existente sirve de referencia, pero no basta: `vehicle_audio_create` solo carga commons y deja la fuente continua en Legacy; no expone la activación del V10, telemetría con dt, estado mecánico adicional ni controles completos de roce/neumáticos.

Operaciones necesarias en la nueva ABI versionada:

1. Consultar versión, firma de layout y procedencia; devolver errores legibles por handle o durante creación.
2. Crear desde raíz de assets y perfil explícitos: cargar commons, mixer config y V10 mediante `enable_grand_prix_sampler`; comprobar rango físico y devolver fuente/hash efectivos.
3. Consumir paquetes temporizados completos; llamar a `set_grand_prix_mechanical_state`, `set_telemetry_timed`, `set_scrape_state` y `set_tire_scrub_state`.
4. Generar bloques PCM con tamaño máximo y frecuencia conocidos; consultar diagnósticos sin asignaciones en la ruta crítica.
5. Reiniciar generación/sesión y destruir recursos de forma ordenada.

Usar tamaños y tipos C fijos, offsets verificados, strings UTF-8 y memoria liberada por su propietario. Un solo hilo toca el handle mutable. Capturar errores/pánicos en la frontera; ninguna excepción cruza la ABI.

Un fallo de inicialización no debe anunciar el V10 como activo ni activar silenciosamente una mezcla Legacy sobre él. Mantener el modo antiguo seleccionable para reversión explícita; registrar la causa del fallo una vez y distinguir disponibilidad de commons y motor.

### Transporte OpenAL

Reutilizar el dispositivo/contexto de `OpenalSoundInterface`. Rust no abre dispositivos. Renderizar fuera del hilo de simulación, con buffers preasignados y colas acotadas. Todas las llamadas OpenAL se hacen desde el propietario autorizado de su contexto; no trasladar el contexto al worker mientras el backend existente lo usa.

Prototipo de menor alcance: worker del mixer produce PCM y el propietario actual del contexto recicla buffers procesados con `alSourceUnqueueBuffers`/`alSourceQueueBuffers`. Punto de partida medible: cuatro buffers de 1024 frames a 44100 Hz, unos 93 ms de cola máxima; ajustar después de medir latencia y frames lentos. El worker PCM no evita que una parada prolongada del hilo que alimenta OpenAL agote esa cola. Ensayar ese límite antes de cerrar la implementación.

Si los bloqueos habituales del render hacen fallar la prueba, la siguiente fase necesaria es trasladar el servicio OpenAL a un hilo propietario con comandos para el backend existente; no aumentar indefinidamente el buffer ni usar un contexto compartido concurrentemente sin contrato. Registrar esta decisión de transporte antes de activar por defecto.

Cockpit del jugador: stream estéreo relativo al listener. Cámaras exteriores/rivales: salida mono espacializable obtenida del mixer, inicialmente mediante downmix, con posición/velocidad del coche. Verificar cancelación al convertir a mono y no prometer equivalencia estéreo exterior. OpenAL 1.1 no aplica espacialización 3D a buffers multicanal. No alimentar una fuente estéreo esperando posicionamiento 3D.

El pitch del motor ya lo resuelve el sampler. La fuente PCM usa pitch base 1; cualquier Doppler exterior se aplica una sola vez. Auditar el Doppler manual de `car_src` existente y la distancia/LOD de Rust para evitar doble pitch o atenuación. Reflejar volumen global, mute, pausa y cambio de cámara en ambas rutas sin multiplicar la ganancia dos veces. Respetar el presupuesto de fuentes del pool.

### Exclusión de efectos antiguos

Al activar el nuevo backend para un coche, desactivar sus fuentes antiguas de motor, axle/turbo, cambios, backfire y los efectos commons sustituidos. Los bucles globales que eligen el máximo entre coches deben excluirlo; poner su volumen de motor a cero no basta.

En particular, no enviar un cambio simultáneamente desde diferencia de marcha y desde fase física: el sampler posee esos eventos. N↔1 conserva la capa commons prevista por el mixer. El motor detenido no puede dejar sonando ralentí, mientras las superficies pueden seguir si el coche se mueve.

## Plan por etapas y archivos responsables

| Etapa | Trabajo y archivos previstos | Puerta de salida |
| --- | --- | --- |
| 1. Congelar e importar | `rust/vehicle-audio-engine/`, `rust/v10-engine-synth/`, `rust/Cargo.toml`, lockfile; registro de procedencia en `implementation/`. Adaptar build stamp/fixtures, identificar el diff local y completar la adaptación real a 3500 RPM descrita arriba. | Dependencias resueltas localmente, ralentí de 3500 RPM admitido y carga reproducible sin Formula90s instalado. |
| 2. Contrato y render offline | `rust/open-formula-audio/`, cabecera ABI en `src/modules/sound/snddefault/`; harness de replay y WAV. Usar el perfil Williams, config de mezcla y ambos bancos. | V10 activo confirmado, todos los grupos audibles y reloj de eventos correcto. |
| 3. Telemetría | `rust/vehicle-physics-engine/src/open_formula*.rs`, `src/modules/simu/rustphysics/rust_abi.h`, cargador y publicación C++; nuevo contrato bajo `src/interfaces/`. | Paquetes iguales al estado físico y sin pérdida de cortes a FPS bajos; ABI verificada. |
| 4. Cargador y salida | Nuevos adaptador/cargador/stream C++ en `snddefault`; modificar `grsound.cpp`, `OpenalSoundInterface.*`, `CarSoundData.*` y su CMake. | Reproducción estable, reversión explícita y efectos sin duplicación. Resolver aquí si se requiere servicio OpenAL independiente. |
| 5. Preparación/instalación | `tools/prepare_williams_vehicle.py` o helper específico de bancos, CMake Cargo/instalación y selección de backend por coche. | Distribución autónoma con hashes de assets/código y DLL correcta. |
| 6. Validación integrada | Replays y evidencia en `implementation/reports/`; sesión Williams/Fuji con cockpit y exterior. | Pruebas técnicas y escucha humana completas antes de declarar terminado. |

Proponer la raíz instalada `distribution/data/sounds/`, con `banks/commons`, `banks/v10-v2-bank` y `sound_mixer_config.json`. Resolverla desde el directorio de datos del motor; nunca desde el cwd. El perfil instalado apunta a esa raíz y conserva sus ganancias de motor/caja/coast/limitador, junto a relaciones de transmisión físicas.

Añadir la biblioteca de audio al build Cargo con paquete explícito. El CMake de física hoy ejecuta `cargo build --offline --release` para todo el workspace: al agregar miembros puede compilar bibliotecas de audio incidentalmente. Ajustar objetivos y dependencias para evitar builds simultáneos redundantes y asegurar instalación de cada DLL. Elegir target/runtime Windows coherentes y verificar carga desde UCRT64; la ABI C reduce el acoplamiento C++, pero no reemplaza esa prueba.

No regenerar ni remasterizar muestras para efectuar el port. El helper copia únicamente los assets declarados, comprueba hashes y conserva manifests. Las pruebas de preparación fallan ante archivos faltantes o incompatibles.

## Aceptación

- Carga y errores: 18 assets V10 + 12 commons, hashes/frames/formatos, configuración efectiva, fuente seleccionada, rutas con espacios/no ASCII, archivo ausente, manifest corrupto y ABI incorrecta.
- Paridad offline: mismo perfil y secuencia contra el mixer donante congelado; comparar stems y eventos con tolerancia numérica registrada. No equiparar simplemente dos mixes con configuración distinta.
- RPM: inicialización con ralentí configurado a 3500, reproducción estable y afinada a 3500 sin clamp a 4500, transición continua 3500→4500→5600 con acelerador y en retención, todos los solapamientos ON/OFF, salida del medio a 13000 y 17500 máximas; motor a cero termina en silencio. Verificar playback rate, energía de salida y rechazo de máximos fuera de cobertura, además de escucha del ralentí.
- Caja: N→1 y 1→N una vez por transición, subidas con carga y flutter, reducciones, marcha atrás, embrague abierto/cerrado y retención. Eje de salida no se calcula únicamente con RPM del motor.
- Limitador: reproducción exclusivamente durante ventanas físicas, incluidos múltiples cortes dentro de un frame gráfico. TC refleja el corte real y no se descuenta dos veces.
- Commons: hierba/piano/arena, derrape y scrub por rueda, cuatro impactos, barrera/cono y roce sostenido del fondo; sin repetición por contacto mantenido ni falsos impactos al iniciar sesión.
- Tiempo/hilos: secuencia equivalente a distintos FPS, cola llena, datos antiguos, underruns, frames lentos, pausa/reanudación, cambio de cámara y salida/reinicio repetidos. Informar profundidad y latencia reales; no prometer cero underruns sin medir.
- Rendimiento: tiempos p95/p99 de render por bloque frente a su deadline, memoria decodificada, asignaciones del camino de render, fuentes OpenAL y coste con varios coches. Carga y preparación fuera de la ruta crítica.
- Escucha: cockpit y exterior, aceleración/retención, cruce de capas sin clicks, balance caja/commons, mute/volumen, distancias y ausencia de dobles voces. Medir pico/RMS y clipping. La verificación de hashes no sustituye esta prueba.

## Resultado de la evaluación

La propuesta de importar el crate y conectar C++ es viable y preferible a reconstruir el sampler. El trabajo principal está en exponer su configuración completa, transportar telemetría física temporal, instalar también la mezcla y gobernar OpenAL y las fuentes antiguas. Debe tratarse como una integración de audio completa por etapas, no como una copia del banco ni como una nueva dependencia del runtime Godot.

Fuentes técnicas primarias: [Rust Reference: linkage](https://doc.rust-lang.org/reference/linkage.html), [Rustonomicon: FFI](https://doc.rust-lang.org/nomicon/ffi.html), [OpenAL 1.1 Specification](https://www.openal.org/documentation/openal-1.1-specification.pdf) y [OpenAL Programmer's Guide](https://www.openal.org/documentation/OpenAL_Programmers_Guide.pdf). La evidencia específica del proyecto procede de los archivos locales citados y del diff inspeccionado.
