# E2 — Raycasts anclados al centro físico de las cuatro ruedas

**Entrega:** E2 (Puerta C)
**Archivos:** `rust/vehicle-physics-engine/src/{types.rs,open_formula.rs,suspension.rs,simulation.rs,simplified_axle_integration.rs}`, `src/modules/simu/rustphysics/frame_transform.h`.
**Estado:** migración inicial y correcciones de seguimiento validadas (16 pruebas de puerta + 51 pruebas de integración heredadas).

---

## 1. Contrato anterior y por qué había que cambiarlo

```
origen  = wheel_anchor_local() transformado al mundo      (punto de anclaje del chasis, inmóvil)
longitud = spring_length + tire_radius
demanda  = (spring_length + tire_radius) − distancia_promedio
```

Consecuencias medidas:

1. La demanda **no depende del estado de la suspensión**. Si la rueda penetra 3 cm, la lectura no
   cambia en absoluto: el rayo nace en el chasis.
2. Mover la compresión no mueve el origen: el contrato no puede representar una rueda que sube.
3. En pendiente/peralte la distancia es oblicua y se usa como si fuera perpendicular
   (`1/cosθ` de error, hasta un 2 % con 10° de peralte).
4. `spring_length + radius − distance` aplicado a una distancia **desde el centro** es dimensional e
   interpretativamente incorrecto: no se puede reutilizar.

## 2. Contrato nuevo

### 2.1 Origen

```
centro_local = anchor_local − UP·(spring_length − q) + (hub_lateral, 0, hub_long)
centro_world = transform · centro_local
```

* Función pura única: `suspension::wheel_centre_local(config, wheel, q, hub_lateral, hub_long)`.
* `q` es la compresión de la masa no suspendida del estado aceptado, no un snapshot gráfico.
* Los desplazamientos laterales/longitudinales del buje (`hub_lateral_m`, `hub_long_m`) provienen de
  la cinemática geométrica cuando existe; **no se supone** que el centro sea puramente vertical.
  Aproximación declarada: la dirección de la extensión se toma sobre el eje de la suspensión
  (`basis.y`), que es también el eje de la coordenada `q` del integrador; la cinemática resuelve
  `q` y publica su trayectoria lateral/longitudinal, que aquí se suma en coordenadas de cuerpo.
* Publicación coherente: `OfpWheelState.hub_position` usa **esta misma función**, así que la
  presentación, los rayos y el diagnóstico comparten una definición.

### 2.2 Dirección

Se conserva el eje de suspensión (`−basis.y`), documentado en `types::WheelRayContext`:

* En pendiente/peralte la distancia se convierte a **hueco perpendicular** con
  `gap = d·(axis·n) − r`, que es exactamente la distancia normal al plano de contacto. No introduce
  falsas distancias ni depende del ángulo de la pendiente.
* Una superficie cuya normal es casi perpendicular al eje (un muro) **no es plano de apoyo** y se
  descarta (`axis·n < 0,15`), de modo que una barrera vertical no puede convertirse en suelo.
* Se comparó con la vertical del mundo: el eje de suspensión es el que conserva la geometría en
  peralte y rasante, porque es el que define la coordenada que integra el solver. La vertical del
  mundo sólo coincide con él en terreno plano.

### 2.3 Longitud y cobertura de recorrido

```
longitud = tire_radius + q + 0.005 m
```

Igualdad algebraica con el alcance absoluto anterior:

```
alcance_desde_anclaje = (spring_length − q) + longitud = spring_length + tire_radius + 0.005
```

Por tanto **el recorrido completo droop↔bump queda cubierto**: una rueda en droop máximo ve
exactamente lo que veía antes, y ninguna superficie dentro de la envolvente mecánica se declara
«sin suelo». La prueba `ray_reach_covers_the_whole_droop_to_bump_travel` verifica la igualdad para
`q = 0, 25 %, 50 %, 75 %, 100 %` del recorrido en las cuatro ruedas.

El margen de 5 mm es una tolerancia numérica (bordes de triángulo y redondeo), no un suplemento de
alcance físico; está declarado como constante `WHEEL_RAY_MARGIN_M` con su justificación.

### 2.4 Separación de banda y simetría

* Origen de cada rayo: `centro ± rueda_derecha_world · span`, con `span = ancho ·
  tri_ray_spacing_ratio`. El eje transversal incluye camber, toe y dirección resueltos de la rueda;
  la dirección del rayo conserva el eje de suspensión.
* El signo se decide **una vez por lado**: `+span` para ruedas izquierdas, `−span` para derechas, de
  modo que el rayo interior siempre apunta hacia la línea central. Las tres posiciones se publican
  con el eje de rueda registrado en `WheelRayContext`.

### 2.5 Conversión hueco → compresión (nueva, en `suspension.rs`)

```
gap_i   = d_i·(axis·n_i) − r            (∞ si no hay impacto o si es un muro)
gap     = Σ w_i·gap_i / Σ w_i           (w = 1:2:1, sólo rayos con apoyo)
demanda = compression_at_sample − gap     (clamp ≥ 0)
```

Derivación (terreno plano, `cosθ = 1`): la rueda toca cuando `spring − q + r = h_anchor − H`, luego
`q_touch = spring + H + r − h_anchor`; el hueco es `gap = q − q_touch` y por tanto
`q_touch = q − gap`. La nueva fórmula es **la misma magnitud** que la anterior en plano
(comprobado numéricamente: la serie A/B es idéntica bit a bit), y sólo difiere donde debe: el origen
es el centro, la pendiente entra por la normal y la penetración entra en la restricción.

`gap > 0` significa rueda flotando; `gap < 0`, penetración; `gap = 0`, contacto exacto.

### 2.6 Complementariedad `gap ≥ 0`, `N ≥ 0`, `gap·N = 0`

Se añadió al estado por rueda `ground_gap_m` y la restricción unilateral se **activa sólo** cuando
`ground_gap_m ≤ CONTACT_GAP_TOLERANCE_M`:

* `simplified_axle_integration.rs`: `contact = if contact_fraction > 0 && gap ≤ tol`.
* `suspension.rs::ground_contact_constraint` (ruta heredada): misma condición.

Sin esta puerta, una rueda suspendida en el aire recibía una reacción de 284 N en la primera prueba
de la Puerta C (el límite geométrico actuaba como suelo aunque la rueda estuviera 3 cm por encima).
Es exactamente el fallo que la condición de complementariedad prohíbe.

`CONTACT_GAP_TOLERANCE_M = 0,5 mm`, justificado en el código: el cuerpo cae 0,09 mm por paso a
240 Hz, así que una histéresis de 0,5 mm (5× el error de muestreo, 1,25 % del recorrido de 40 mm y
0,16 % del radio de 0,31 m) impide el chattering del estado discreto sin permitir una carga falsa
mayor que esa misma ventana.

### 2.7 Predictor por subpaso

`simulation::predicted_contact_samples` desplaza el contexto de compresión al paso actual:

```
compression_efectiva = compression_at_sample + (ancla_muestreada − ancla_actual)·up
```

Se recalcula desde la muestra en cada intento (nunca se acumula) y sólo cambia el **objetivo** de la
restricción, no un impulso. Así un cuerpo que se desplaza verticalmente durante el intervalo no se
resuelve contra la pose de un instante que ya no existe.

## 3. Contrato de datos publicado

`TriRaycastSample` incorpora `context: WheelRayContext` con:

| Campo | Significado |
| --- | --- |
| `from_wheel_centre` | `true` = distancias desde el centro físico; `false` = contrato heredado de anclaje |
| `centre_world` | centro físico muestreado |
| `anchor_world` | punto de anclaje muestreado (para el predictor y la trazabilidad) |
| `up` | eje de suspensión muestreado |
| `compression_m` | `q` en el instante del muestreo (corregido por el predictor) |
| `radius_m` | radio físico del neumático (nunca el radio efectivo de rodadura) |
| `span_m` | semiseparación de banda |

`RaycastHit` conserva `distance/point/normal/surface`; la semántica de `distance` queda declarada por
el contexto. No se cambió ninguna estructura expuesta a C++ (`OfpSnapshot` sigue en ABI v7).

## 4. Tres radios que no se confunden

| Magnitud | Dónde vive | Uso |
| --- | --- | --- |
| centro geométrico | `wheel_centre_local` / `context.centre_world` | origen de rayos, `hub_position` |
| radio físico | `config.tires.*_tire_radius` → `context.radius_m` | conversión hueco↔contacto, altura del centro |
| radio efectivo de rodadura | modelo de neumático (`tire.rs`) | cinemática de giro, nunca corrección de penetración |

`radius_m` nunca se usa para corregir penetración: la corrección se expresa como compresión y la
resuelve el solver vertical.

## 5. Resultados de la Puerta C

| # | Requisito | Prueba | Resultado |
| --- | --- | --- | --- |
| 1 | Suelo plano y cuatro ruedas: centros, huecos y normales coherentes | `flat_ground_reports_centre_origin_gaps_and_normals` | PASS — centro a `r ± 1 cm`, hueco `±1 cm`, `N` total = peso |
| 2 | Droop, bump e intermedios sin pérdida falsa ni inversión de signo | `ray_reach_covers_the_whole_droop_to_bump_travel`, `a_wheel_hanging_at_droop_reports_no_false_ground` | PASS — alcance idéntico al anterior en 5 puntos de recorrido |
| 3 | Peralte, rasante y borde: normales sin saltos; hueco perpendicular | `tilted_ground_reports_the_perpendicular_gap_not_the_slant_distance` | PASS — la prueba compara con distancia firmada proyectada sobre la normal unitaria; tolerancia 1 mm a 10° |
| 4 | Elevación progresiva: pérdida de apoyo, `N=0` en vuelo, recuperación sin impulso energético | `a_wheel_hanging_at_droop_reports_no_false_ground` | PASS — `N=0` en vuelo; energía de impacto ≤ energía potencial de la caída (+5 %) |
| 5 | Simetría izquierda/derecha y correspondencia visual/estado | `left_and_right_corners_are_symmetric_on_flat_ground`, `the_wheel_centre_follows_the_unsprung_state_and_the_hub_offsets` | PASS |
| 6 | Disminuir `dt` converge, no genera energía | `halving_the_step_converges_the_ride_height_and_the_loads` (240/500/1000 Hz) | PASS — altura a 1000 Hz ±2,5 mm; error 500 Hz ≤ error 240 Hz |

## 6. Límites declarados

1. **Remuestreo por subpaso**: la geometría se muestrea una vez por paso y se corrige con el
   predictor de altura. La componente horizontal (entrar en un piano) puede quedar hasta 4 ms
   retrasada a 240 Hz. Medido por la prueba de convergencia; no se implementó el remuestreo completo
   por subpaso porque exige mover la consulta del mundo dentro del simulador (cambio de mayor
   superficie, sin ganancia demostrada en estas pruebas).
2. **Ponderación 1:2:1**: se conserva. En bordes de piano la fracción de contacto (0, 0,25, 0,5,
   0,75, 1) se publica y el modelo de neumático escala la rigidez con ella; no se cambió por una
   consulta de forma. Las tres muestras de diagnóstico se mantienen.
3. **Filtro de «muro»**: una superficie con `axis·n < 0,15` se descarta como apoyo. Un piano muy
   inclinado (> 81°) se trataría como muro; ningún circuito del proyecto lo tiene.

## 7. Correcciones de seguimiento — 24 de octubre de 2026

La revisión encontró dos huecos en la migración inicial y una tolerancia demasiado amplia en la
prueba de peralte:

* `CollisionWorld` ya guardaba fricción y resistencia a la rodadura por triángulo, pero se perdían
  al construir `RaycastHit`. Ahora ambos coeficientes acompañan cada impacto, se ponderan con los
  mismos rayos de apoyo 1:2:1 y multiplican los coeficientes de superficie configurados. El ABI C
  no cambia; los campos internos serializados nuevos tienen un valor neutro por compatibilidad.
  Se rechazan coeficientes negativos o no finitos al crear el mundo.
* Los orígenes interior/centro/exterior ahora siguen el eje transversal de la rueda con su toe,
  camber y ángulo de dirección. El mismo eje queda en el contexto muestreado y en el informe por
  rayo, para que el solver y la telemetría describan la misma consulta.
* La prueba de peralte calcula la distancia firmada a la superficie usando la normal unitaria y
  bajó la tolerancia de 6 mm a 1 mm.
* Nuevas pruebas extremo a extremo: `per_triangle_material_coefficients_reach_the_live_wheel_model`
  y `wheel_contact_strip_follows_the_solved_steer_and_camber`. Puerta C actual: 16 aprobadas.

Sigue pendiente remuestrear la geometría dentro de los subpasos cuando hay desplazamiento horizontal.
El mundo sigue cargándose del documento derivado de la malla visual; exportar directamente los
segmentos físicos `tTrack` requiere compilar y validar el adaptador del juego. La barrera aún se
detecta con los puntos de rueda y no con un volumen de chasis. No se ha ejecutado Melbourne T3.
