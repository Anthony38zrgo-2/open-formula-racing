# Melbourne T3: FR descargada y reglajes de extensión sin efecto

**Actualización posterior:** la corrección ya está instalada y validada técnicamente. Consultar [completion.md](completion.md) y `installation.json`. Este documento conserva el diagnóstico de la versión anterior; su estado de «sin cambios de producción» corresponde al momento de aquella investigación.

Investigación: 22 de octubre de 2026, America/Bogota. Solo diagnóstico; no se modificaron código de producción, perfiles instalados, base de reglajes ni DLL. No hay commit ni push de esta investigación.

## Conclusión

Hay dos fallos confirmados de conexión entre los controles del garaje y la suspensión simplificada activa. Explican por qué editar la extensión y la estabilizadora no cambia la respuesta. La estabilizadora efectiva, bastante mayor que la solicitada, es una candidata fuerte para la descarga sostenida de la rueda interior. No se ha demostrado todavía qué proporción de la duración concreta de T3 corresponde a barra, amortiguador, transferencia de carga o terreno.

No parece una rueda congelada en un tope de extensión: durante el episodio su compresión cambia y está lejos del límite nominal de droop.

## Base verificada

- HEAD del repositorio: `5f5f04d50f9f4a921280bb95c3e01a478a6a64e9`.
- El runtime carga `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll`, con suspensión simplificada y contacto rígido. La investigación se hizo sobre esa DLL, no sobre simuv5.
- SHA-256 instalado: `28756170fcb0e92f3bd9e27aa9a2c3034bfc9d1a2383964ebdc7eadfd1e3ae5b`; coincide con `rust/target/release/vehicle_physics_engine.dll`.
- Proveniencia publicada por la DLL: `abi=6 source_commit=f3efcf246c2a2abf62477037ed0243f2f9a93a9b build=9c91648`.
- Fuente de reglaje: `distribution/user-settings/open_formula.db`, abierta en modo de solo lectura. Único registro: id 1, **Reglaje 1**, Melbourne 1997, Williams, backend `rustphysics`.
- Última modificación del registro: 22/10/2026 20:10:56 en America/Bogota.
- La prueba offline valida la firma completa del ABI antes de acceder a sus estructuras.

## 1. Extensión, amortiguación, compresión y topes no gobiernan los elementos activos

Valores guardados relevantes:

| Control | Delante | Detrás |
| --- | ---: | ---: |
| Extensión / `rebound_damp_multiplier` | 2,02 | 1,45 |
| Amortiguación / `damping_ratio` | 0,80 | 0,80 |
| Compresión / `bump_damp_multiplier` | 1,10 | 0,90 |
| Topes / `bump_stop_multiplier` | 0,15 | 0,25 |
| Estabilizadora geométrica / `geometry_physical.*_arb.ratio` | 0,20 | 0,20 |

El garaje presenta «Extensión» y escribe `suspension.front.rebound_damp_multiplier` (`formula_garage_menu.cpp:683`; descriptores en `open_formula.rs:257`). `apply_setup_json` acepta y conserva el valor y reconstruye los datos preparados (`open_formula.rs:1145`). La conversión del documento mantiene esos campos separados de `geometry_physical` (`vehicle_configuration/legacy_profile.rs:227`).

Pero la ruta efectiva entra por `SuspensionSystem::step_with_contact_kinematics` y `solve_simplified_geometric_axle` (`suspension.rs:617`, `simplified_axle_integration.rs:309`). `PreparedAxleCorner::element_forces` utiliza directamente `geometry.axle(...).damper_bump_Ns_per_m` y `damper_rebound_Ns_per_m` mediante `geometric_damper_element`, sin aplicar esos multiplicadores ni el damping ratio. Los multiplicadores del garaje aparecen en `suspension_element_forces` (`suspension.rs:1643`), que es otra ruta.

Coeficientes que realmente consume el modelo geométrico instalado:

| Elemento | Delante | Detrás |
| --- | ---: | ---: |
| Amortiguador en compresión | 6970 N s/m | 14000 N s/m |
| Amortiguador en extensión | 11475 N s/m | 22000 N s/m |
| Multiplicador de topes geométrico | 2,6 | 2,8 |

Los valores del amortiguador están definidos en su coordenada; el solver los lleva a rueda mediante la relación de movimiento. No deben interpretarse directamente como amortiguación a rueda.

Los topes también leen `geometry_physical.*.bump_stop_mult` (`suspension.rs:1563`), separado de `suspension.*.bump_stop_multiplier` editado por el garaje.

**Distinción física:** extensión/rebote es resistencia al extender el amortiguador, no recorrido adicional ni fuerza que expulsa la rueda. Si el multiplicador estuviera conectado, aumentarlo frenaría el retorno. Referencia primaria: [Penske, What Are Adjustable Shocks?](https://www.penskeshocks.com/blog/what-are-adjustable-shocks?hs_amp=true).

## 2. La estabilizadora ignora el ratio guardado

El descriptor permite cambiar `suspension.geometry_physical.front_arb.ratio` y su equivalente trasero. Sin embargo, `simplified_antiroll_stiffness` toma `configuration.suspension.front_arb_ratio` / `rear_arb_ratio` (`simplified_axle_integration.rs:294`). Después `effective_antiroll_configuration` sustituye expresamente el ratio de `LegacyRatio` por ese otro valor (`simplified_suspension.rs:24`).

El perfil instalado fija:

- `suspension.front.arb_ratio = 1,00`.
- `suspension.rear.arb_ratio = 0,80`.

Estos campos no son los editados por los descriptores actuales. Frente al 0,20 guardado, la barra efectiva tiene un ratio **5 veces mayor delante y 4 veces mayor detrás**. Esto compara el ratio que multiplica la rigidez de referencia; no significa multiplicar la transferencia total de carga del coche por esos factores.

La fuerza usa la diferencia de recorridos entre ruedas. En un giro a derecha, FL comprimida y FR menos comprimida generan una contribución de barra que carga FL y descarga FR. Esta conexión equivocada es coherente con el síntoma y hace ineficaz intentar corregirlo desde el control visible de barra.

## 3. Evidencia del registro de conducción

Orden confirmado del registro Rust: **FL, FR, RL, RR** (`rustphysics.cpp:646`). No confundir con el orden público de Speed Dreams FR, FL, RR, RL.

En el giro a derecha alrededor del metro 1100 del circuito, compatible con T3:

| Tiempo físico (s) | Velocidad (km/h) | N FL | N FR | Compresión FL | Compresión FR |
| --- | ---: | ---: | ---: | ---: | ---: |
| 36,174 | 86,5 | 4207,4 N | 0 N | 56,9 mm | 42,7 mm |
| 36,424 | 83,3 | 3542,9 N | 0 N | 55,5 mm | 42,3 mm |
| 36,674 | 80,3 | 3351,6 N | 0 N | 56,5 mm | 43,7 mm |
| 36,924 | 77,2 | 3208,3 N | 0 N | 54,5 mm | 42,2 mm |
| 37,174 | 75,6 | 2490,2 N | 0 N | 49,1 mm | 39,0 mm |
| 37,424 | 73,9 | 1533,7 N | 1350,9 N | 47,0 mm | 46,4 mm |

Son cinco muestras consecutivas con FR sin apoyo, separadas por aproximadamente 0,25 s, cubriendo un segundo. No permiten afirmar ausencia ininterrumpida de contacto entre muestras ni medir exactamente el instante de despegue/retorno.

Durante esas cinco muestras:

- FR cambia de compresión: la coordenada no está congelada.
- En el inicio y final del grupo hay 12/12 rayos con impacto, aunque FR no genera reacción normal. Detectar suelo y soportar carga son hechos distintos; no es un simple fallo permanente de detección del terreno.
- No hay impulso de fondo: `floor_impulse=(0,0)` en las muestras. No se demuestra que no exista algún evento entre ellas.
- No hay pasos rechazados en las muestras.
- La referencia delantera es 95 mm y el droop configurado 85 mm: límite nominal cercano a 10 mm de compresión, antes de la pequeña tolerancia de sobrepaso y la intersección cinemática. FR permanece alrededor de 39–44 mm, lejos del límite nominal. No se midió aquí el límite geométrico efectivo exacto.
- Las traseras sí sostienen carga. El registro no indica que todo el coche dependa exclusivamente de FL.

Hay episodios parecidos cerca de 22–23 s y 56–57 s. La secuencia posterior de caída de todo el coche bajo el mapa se conserva en `results.json`, pero no se utiliza como evidencia de esta descarga en curva.

El registro contiene el guardado de id 1 al final de la sesión. El reglaje actual documenta los valores guardados; no constituye una captura de todos los valores vivos en cada instante anterior.

## 4. Comprobación ejecutable con la DLL instalada

Ejecutar desde la raíz: `python implementation/reports/melbourne_t3_extension/diagnose.py`.

Banco: caída sobre una superficie con 30 mm de diferencia entre lados, 480 pasos a 480 Hz, sin dirección, punto muerto y embrague desacoplado. Todos los ensayos parten del mismo reglaje guardado. Las transacciones editables se aceptan y se releen para comprobar el valor solicitado. Se comparan posición del chasis y, por rueda, compresión, velocidad, fuerza de muelle, amortiguador y reacción normal.

| Cambio aislado | Resultado numérico respecto al reglaje guardado |
| --- | --- |
| Extensión delantera a 0,20 | Trayectoria idéntica, diferencia cero |
| Extensión delantera a 3,00 | Trayectoria idéntica, diferencia cero |
| Amortiguación delantera a 0,05 | Trayectoria idéntica, diferencia cero |
| Compresión delantera a 0,20 | Trayectoria idéntica, diferencia cero |
| Topes delanteros a 4,00 | Trayectoria idéntica, diferencia cero |
| Barra geométrica del garaje a 0,00 | Trayectoria idéntica, diferencia cero |
| Barra geométrica del garaje a 2,00 | Trayectoria idéntica, diferencia cero |
| Coeficiente geométrico real de rebote reducido a la mitad | Respuesta distinta; diferencia máxima de compresión FR 36,26 mm |
| Ratio real `suspension.front.arb_ratio` a 0,20 | Respuesta distinta; diferencia máxima de compresión FR 5,90 mm |

Los dos últimos son controles positivos en documentos candidatos guardados exclusivamente dentro de esta carpeta. Confirman que el banco excita esos elementos. No se instalaron ni se ofrecen como reglajes validados. Las diferencias máximas incluyen el transitorio de caída; no predicen directamente el beneficio en T3.

Archivos: `diagnose.py`, `results.json`, `runtime_excerpt.log`, terreno sintético y tres documentos candidatos del banco.

## 5. Corrección que corresponde investigar e implementar después

1. Definir una sola autoridad para los parámetros ajustables: conectar los descriptores con los coeficientes y ratios que consume la ruta simplificada, o traducir explícitamente los multiplicadores con una referencia estable. Evitar multiplicar repetidamente el perfil al aplicar varias veces un reglaje.
2. Resolver la compatibilidad del reglaje SQLite existente y del hash de perfil base. No eliminar ni reinterpretar silenciosamente «Reglaje 1».
3. Verificar respuesta física, no solo lectura del JSON: cada control debe alterar su elemento correspondiente, sin cambiar los demás. Cubrir ambas barras, ambos ejes, compresión, rebote, amortiguación y topes.
4. Comparar en condiciones idénticas la barra actual con la solicitada realmente aplicada. Registrar por paso compresión/velocidad, hueco al suelo, fuerzas de muelle/amortiguador/barra/tope, límites e impulsos aceptados.
5. Repetir T3 y curvas equivalentes para medir tiempo de descarga y recuperación. No añadir un temporizador para forzar apoyo: la recuperación depende del balance de fuerzas, la geometría, la transferencia lateral y el terreno.

**Estado:** fallos de conexión confirmados por código y por la DLL instalada; mecanismo de barra coherente con la descarga; causalidad completa y mejora real de conducción pendientes de comparación instrumentada. Producción y reglaje del usuario conservados.
