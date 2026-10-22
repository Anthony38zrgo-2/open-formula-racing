# Encargo: incorporar el HUD de Formula90s en Open Formula

Fecha: 22 de octubre de 2026.

## 1. Repositorios locales y límites

- **Destino de la implementación: `D:\open-formula`.** Proyecto C++/OpenSceneGraph basado en Speed Dreams, con física Rust. Todos los cambios de este encargo se realizan aquí.
- **Referencia visual y funcional: `D:\Formula90s`.** Proyecto Godot. Consultar sus escenas, scripts, configuraciones y recursos en modo lectura. No modificar este repositorio.
- No modificar memorias. No hacer commit ni push durante este encargo.
- Leer los `AGENTS.md` aplicables antes de editar. Revisar `git status`, diffs y archivos sin seguimiento; preservar cualquier trabajo previo o concurrente.
- El archivo `D:\open-formula\instrucciones.md` corresponde a otro encargo histórico. No reemplazarlo ni ejecutar automáticamente sus tareas de física como parte de esta migración.

El usuario ya solicitó esta migración y definió el alcance. Resolver las decisiones rutinarias de implementación dentro de estos límites sin volver a pedir elecciones ya acordadas.

## 2. Objetivo y alcance autorizado

Reproducir en Open Formula únicamente estos elementos de la UI de Formula90s:

1. HUD de tacómetro y velocímetro.
2. Indicador de gasolina: nivel y cantidad restante.
3. Temperaturas de motor: agua y aceite.
4. Temperaturas de los cuatro conjuntos de rueda: banda interior/centro/exterior, carcasa, gas y llanta cuando el modelo disponga de esas magnitudes.
5. Temperaturas de los discos de freno de las cuatro ruedas.
6. Minimapa: trazado, salida/meta y posición del coche.
7. Tiempos: número de vuelta, tiempo actual, última vuelta y mejor vuelta, usando la autoridad de carrera del destino.
8. Indicador de giro del volante en grados, con signo o indicación izquierda/derecha.
9. Indicadores de pedales de acelerador y freno.

Conservar la distribución relativa del original y adaptar las dimensiones para evitar choques con otros componentes de la UI del destino. Todos los textos y números de los componentes migrados deben usar **Libron**, Regular/Bold según corresponda. El usuario la denominó «lebrons»; el recurso identificado en el destino es Libron.

No incorporar otros paneles de Formula90s: configuración de conducción, tuning, PIT STOP, ayudas, clasificación, rivales u otros widgets ajenos a la lista. No rediseñar menús ni garage ni añadir controles al coche.

## 3. Estado previo que debe preservarse

En la sesión anterior se corrigió el signo de dirección en:

`D:\open-formula\src\modules\simu\rustphysics\rustphysics.cpp`

La asignación debe seguir siendo `input.steering = (double)ctrl->steer;`, sin negación adicional. Se compiló `rustphysics` y se instaló la DLL en la distribución; la comprobación conduciendo seguía pendiente. Inspeccionar el estado actual antes de actuar: esta corrección puede estar todavía sin commit.

No tocar constantes de dirección, suspensión, neumáticos, motor o frenos para conseguir que el HUD parezca correcto. La migración puede ampliar la publicación de telemetría y metadatos, pero no debe cambiar el comportamiento físico.

## 4. Referencia de Formula90s

Leer las versiones actuales de estos archivos, sin asumir que las notas históricas describen el checkout presente:

- `D:\Formula90s\game\scenes\runtime\world_hud_compositor.gd`
- `D:\Formula90s\game\scenes\runtime\world_hud_compositor.tscn`
- `D:\Formula90s\game\scenes\ui\debug_hud.tscn`
- `D:\Formula90s\game\scripts\hud\arcade_race_hud.gd`
- `D:\Formula90s\game\features\hud\config\hud_config.json`
- `D:\Formula90s\game\features\retro_hud\scripts\retro_hud_display.gd`
- `D:\Formula90s\game\features\retro_hud\scripts\competition_hud_adapter.gd`
- `D:\Formula90s\game\features\retro_hud\scenes\retro_hud_display.tscn`
- `D:\Formula90s\game\features\retro_hud\config\retro_hud.json`
- `D:\Formula90s\game\features\retro_hud\assets\`
- `D:\Formula90s\game\scripts\hud\engine_temperature_panel.gd`
- `D:\Formula90s\game\scripts\hud\tire_status_panel.gd`
- `D:\Formula90s\game\scripts\hud\lap_timing_panel.gd`
- `D:\Formula90s\game\scripts\hud\driving_controls_panel.gd`
- `D:\Formula90s\game\scripts\track\track_minimap_controller.gd`
- `D:\Formula90s\game\scenes\ui\fuji76_77_hud_map.tres`

La distribución observada es:

- Tacómetro/velocidad/combustible: abajo a la derecha.
- Cuatro ruedas y frenos: sobre el tacómetro.
- Agua/aceite: encima del panel de ruedas.
- Tiempos: arriba a la izquierda.
- Minimapa: debajo de los tiempos.
- Volante, acelerador y freno: abajo al centro.

El original usa D-DIN en varios paneles y una fuente de fallback en los controles. No trasladar esas fuentes: el requisito del destino es Libron en todo el HUD migrado. El volante original muestra porcentaje; el destino debe mostrar grados de volante. No confundir grados de volante con grados de orientación de las ruedas.

## 5. Arquitectura del destino y propietarios

Verificar primero que la configuración activa siga seleccionando `rustphysics` y `osggraph`. En la revisión estaban definidos en `D:\open-formula\distribution\user-settings\config\raceengine.xml`.

Archivos de entrada:

- `D:\open-formula\src\modules\graphic\osggraph\Viewer\OsgHUD.cpp` y `.h`: integración del HUD. Extraer componentes propios para evitar concentrar toda la implementación en este archivo.
- `D:\open-formula\src\modules\graphic\osggraph\CMakeLists.txt`: registrar nuevos fuentes si corresponde.
- `D:\open-formula\src\modules\simu\rustphysics\rustphysics.cpp`: adaptar/publicar datos del coche.
- `D:\open-formula\src\modules\simu\rustphysics\rust_abi.h`: contrato C++ del snapshot Rust.
- `D:\open-formula\src\modules\simu\rustphysics\rust_library.cpp` y `.h`: carga y validación de ABI.
- `D:\open-formula\rust\vehicle-physics-engine\src\open_formula.rs`: construir el snapshot desde el estado físico autoritativo.
- `D:\open-formula\rust\vehicle-physics-engine\src\open_formula_ffi.rs`: contrato Rust de la ABI.
- `D:\open-formula\src\interfaces\car.h`: estado compartido y tiempos publicados por el motor de carrera.
- `D:\open-formula\speed-dreams-data\data\config\osghudconfig.xml`: configuración y convivencia con indicadores anteriores.
- `D:\open-formula\src\libs\tgfclient\guifont.cpp` y selección de fuente en `OsgHUD.cpp`: referencias de integración de Libron.

Confirmar la ubicación física y el empaquetado de `Libron-Regular.ttf` y `Libron-Bold.ttf`. Verificar si `speed-dreams-data` es un submódulo y reportar por separado cualquier cambio dentro de él. No tratar la distribución generada como fuente canónica.

La implementación será nativa en C++/OpenSceneGraph. Reutilizar recursos y diseño compatibles; no añadir un runtime Godot ni pretender cargar escenas `.tscn` o scripts GDScript en el destino.

## 6. Contrato de datos: completar antes de dibujar

Crear un inventario campo por campo: dato solicitado, propietario físico, unidad de origen, conversión, validez y consumidor visual. Mantener un snapshot de presentación coherente del mismo coche y paso aceptado. No integrar una segunda simulación en la UI.

Hallazgos de la revisión, pendientes de reconfirmar en el checkout:

- La ABI actual expone temperatura central de banda y carcasa, presión, desgaste, fuerzas y dirección; no transporta todas las temperaturas necesarias para este encargo.
- El adaptador copia la temperatura central a las tres zonas de banda. Eso no representa tres medidas independientes: publicar las zonas reales cuando existan.
- El sistema térmico de frenos Rust dispone de temperaturas de disco y llanta. El adaptador actual fuerza `brakeTemp` a cero; no usar ese cero como temperatura física.
- Rust contiene estado térmico de agua y aceite; localizar y publicar las magnitudes y los umbrales del perfil activo.
- `engine_rpm_rad_s` contiene RPM en el snapshot actual pese al nombre. La publicación a `tCarElt` convierte a rad/s. Auditar este contrato y evitar doble conversión.
- La UI legacy consume `temp_opt`, `slipNorm` y `slipOpt`, campos que el adaptador actual no completa. Sustituir los indicadores afectados o reparar su contrato donde deban seguir activos; impedir divisiones por cero y valores NaN.

Requisitos del contrato:

1. Si se amplía la ABI, actualizar simultáneamente Rust/C++, versión, firma de layout y comprobaciones del cargador. Reconstruir e instalar ambos binarios compatibles.
2. Mantener la correspondencia de ruedas: Rust FL/FR/RL/RR frente a Speed Dreams FR/FL/RR/RL. Centralizar el mapeo.
3. Mostrar temperaturas en °C; distinguir presión absoluta y manométrica. Usar umbrales físicos del perfil cuando existan, sin duplicar constantes de física en el HUD.
4. Conservar las unidades de combustible del diseño original cuando corresponda y etiquetarlas explícitamente. Si se convierten kg a litros, usar la densidad real del perfil.
5. Los límites de RPM y las alertas deben depender del coche activo, no de los valores del F1 2030 de referencia.
6. Mostrar grados de volante a partir de la dirección efectiva y la relación/rango configurados. Si el modelo no publica ángulo físico del volante, documentar la conversión como derivada; no presentar directamente el ángulo de rueda como ángulo de volante. Verificar centro, ambos topes y signo.
7. Leer pedales del contrato de controles autoritativo elegido y documentar si representan entrada aplicada o intervención de ayudas. No mezclar valores de distintas etapas.
8. Leer vueltas y tiempos del motor de carrera de Open Formula. No trasladar el controlador Godot ni crear otro cronómetro de carrera.
9. Transformar las coordenadas del minimapa de forma coherente con la pista del destino. Validar orientación, meta y posición en varios puntos del trazado; no asumir que el recurso Godot coincide directamente con las coordenadas Speed Dreams.
10. Los datos ausentes o inválidos se muestran como `--`, con validez explícita; nunca se inventan temperaturas, valores óptimos, estimaciones de consumo o tiempos.

## 7. Layout y convivencia

Implementar una configuración propia de layout: anclajes, márgenes, escala, tamaños mínimos de texto y separación de bloques. Medir textos con Libron; cambiar de fuente modifica sus anchos.

Conservar posiciones relativas del original, adaptando las dimensiones al viewport efectivo. Calcular rectángulos visuales de los grupos y reservar zonas para elementos actuales que permanezcan: mensajes de carrera, banderas, avisos y acceso a boxes/garage. Ocultar únicamente los widgets anteriores que sean reemplazados por esta migración para evitar duplicados.

En espacios reducidos, reducir escalas y separaciones de forma controlada sin solapar grupos ni volver ilegible el texto. Mantener volante/pedales abajo al centro, sin invadir el tacómetro. El grupo derecho completo debe caber en altura. Recalcular al cambiar resolución, tamaño de ventana y modo de pantalla. Usar el área visible real, no coordenadas fijas de una captura.

Todos los textos, números, unidades, símbolos de grados, porcentajes y tiempos deben renderizarse con Libron. Verificar los glifos necesarios y evitar fallbacks silenciosos. Conservar formas, colores y recursos del original en lo compatible con este requisito.

## 8. Orden de implementación

1. Preflight de ambos repositorios y captura/inventario del HUD original y del destino. Registrar procedencia de los recursos trasladados.
2. Inventario de datos y extensión de telemetría/ABI; verificar unidades, validez y mapeo de ruedas.
3. Componentes visuales nativos con Libron y configuración de layout.
4. Integración del bloque derecho: tacómetro, velocidad, combustible, ruedas/frenos y motor.
5. Tiempos y minimapa a la izquierda; volante en grados y pedales abajo al centro.
6. Desactivar indicadores duplicados y validar convivencia con la UI existente.
7. Compilar, instalar y comprobar procedencia de los binarios utilizados.
8. Validación funcional y visual; corregir defectos dentro del alcance y entregar informe de resultados.

## 9. Compilación y validación

Rutas observadas para el destino; verificarlas antes de usarlas:

- Build C++: `C:\Users\bill\open-formula-build\engine`.
- Toolchain UCRT64: `C:\Users\bill\open-formula-build-tools\msys64\ucrt64\bin`.
- Workspace Rust: `D:\open-formula\rust`.
- Distribución: `D:\open-formula\distribution`.
- Ejecutable: `D:\open-formula\distribution\bin\speed-dreams-2.exe`.
- Módulos instalados: `D:\open-formula\distribution\lib\games\speed-dreams-2\modules\simu`.

Conservar la configuración existente de CMake, incluida `CMAKE_DISABLE_FIND_PACKAGE_SOLID=TRUE` si sigue aplicando. No reconfigurar o reconstruir dominios ajenos sin necesidad.

Validar de forma proporcional al cambio:

- Compilación de los módulos afectados y compatibilidad ABI/layout.
- Pruebas focalizadas de conversiones, orden de ruedas, datos inválidos y layout cuando cubran errores reales.
- Comparación de los valores del HUD con el snapshot/log del mismo coche: ralentí, aceleración, frenada y giro en ambos sentidos.
- Frenada sostenida y evolución de temperatura por rueda; comprobar que disco, llanta, carcasa y banda no se intercambian.
- Paso por meta y cierre de vuelta: última/mejor/vuelta actual coinciden con la carrera.
- Minimapa: salida/meta y varios puntos de pista coinciden con el coche.
- Reset, reposicionamiento, entrada/salida de boxes y cambio de coche cuando esté disponible: no quedan datos ni referencias del estado anterior.
- Capturas comparativas a 1280×720, 1920×1080, 2560×1440 y al menos un formato ultrapanorámico; comprobar también redimensionado.
- Revisar Libron en todo el HUD, recortes, colisiones, valores largos y ausencia de widgets duplicados.

No declarar validación conduciendo a partir de una compilación, captura estática o prueba aislada. Si se usa Computer Use y el usuario lo detiene con Escape, parar la interacción y dejar explícita la validación pendiente hasta que la autorice de nuevo.

## 10. Entrega y aceptación

Entregar un informe en `D:\open-formula\implementation\reports\hud_formula90s_migration.md` con:

- Archivos creados/modificados y propósito de cada uno.
- Recursos de Formula90s utilizados y sus rutas de origen.
- Tabla final de datos, unidades y propietario autoritativo.
- Versionado ABI y binarios instalados, con hashes/procedencia.
- Capturas del resultado y resoluciones comprobadas.
- Comprobaciones realizadas, fallos pendientes y limitaciones reales.

El encargo se acepta cuando todos los elementos solicitados están presentes, conservan su distribución relativa, usan Libron, muestran datos coherentes con el coche activo y no se superponen con la UI existente. Distinguir entrega técnica de revisión visual humana. No afirmar aceptación humana si el usuario aún no la dio.

No modificar `D:\Formula90s`, no alterar físicas para ajustar la presentación y no hacer commit ni push.
