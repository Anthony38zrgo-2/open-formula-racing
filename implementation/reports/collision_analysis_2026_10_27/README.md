# Análisis del sistema de colisiones — objetos verticales

**Fecha:** 2026-10-27
**Ámbito:** `rust/vehicle-physics-engine` (motor `vehicle_physics_engine.dll`) y su uso desde
`src/modules/simu/rustphysics`.
**Síntoma reportado:** al pasar de forma tangencial junto a ciertos objetos verticales, el coche
sale despedido de forma desproporcionada y puede acabar en geometría sin colisión. Los objetos
parecen atraer y repeler.

---

## 1. Resultado

La causa no es una "fuerza" del objeto: es un error geométrico del campo de muros 2.5D.

Cada triángulo de obstáculo se convertía en un **segmento de muro**, pero la consulta de contacto
medía la penetración contra el **plano infinito** de la cara, no contra el segmento finito:

```rust
// antes: wall_field.rs
let gap = (P - A) · n;          // distancia al plano, sin acotar al segmento
if u >= -0.10/len && u <= 1 + 0.10/len { ... }
```

`u` solo acota la proyección tangencial. Un vértice del coche podía estar **metros por detrás del
plano** de una cara de un poste —lejos del poste en la dirección normal— y aun así contar como
penetración. Las cajas del coche son grandes (ala delantera de 1,60 m, suelo de 2,56 m) y los
segmentos de un poste son cortos (0,3 m): al pasar con un poco de guiñada, una esquina del ala o
del suelo entraba en la ventana tangencial del poste y el sistema leía penetraciones de 1 a 1,6 m
**sin que el coche tocara nada**. Ese gap negativo alimentaba el solver de impulsos y, sobre todo,
el *split impulse*, que desplazaba el coche varios centímetros por paso sin cambiar su velocidad.

La corrección convierte la consulta en una consulta **segmento finito ↔ polígono** (punto más
cercano en el segmento, normal radial en los extremos, lado según el propio polígono, contacto en
la superficie del muro), limita el desplazamiento total por paso y elimina el pestillo de 2 s que
elegía el lado del muro desde el centro del coche.

Verificación rápida: el mismo paso tangencial que antes reportaba `-1,03 m` en el suelo con
`0,20 m` de separación real ahora reporta `+0,061 m` como contacto más profundo, y **ningún**
contacto negativo en todo el barrido.

---

## 2. Recorrido real de una colisión

| Fuente | Quién la calcula | Cuándo se usa |
| --- | --- | --- |
| Rayos de rueda (12) y sondas de suelo (2) | `OpenFormulaSimulation::sample_wheel_rays` / `sample_floor` | Siempre; resuelven apoyo y suelo |
| Contactos de barrera del XML del circuito | `collectBarrierContacts` (`rustphysics.cpp:694`) | Solo si **no** hay mundo visual instalado (`rustphysics.cpp:798`) |
| Muros 2.5D del modelo visual `.acc` | `WallField` + `resolve_obstacle_wall_contacts` (`open_formula.rs:1195`) | Siempre que el mundo visual tenga obstáculos |

El mundo visual sustituye a las barreras XML, no se suma a ellas: el host no envía los contactos
de barrera cuando `g_visualWorldInstalled` es verdadero.

### 2.1 De triángulo AC3D a segmento de muro

`WallField::build` (`wall_field.rs:489`):

1. Por cada triángulo de obstáculo calcula la normal 3D, toma su componente horizontal
   `n2 = normalize(nx, nz)` y proyecta los tres vértices sobre la tangente. El segmento va del
   vértice de mínima proyección al de máxima.
2. Fusiona segmentos colineales (normal casi igual, plano a â‰¤ 4 cm, hueco â‰¤ 10 cm), hasta 60 m.
3. Decide la orientación "hacia la pista" con 4 sondas a 0,6/1,5/3/6 m y la distancia mínima a
   muestras de triángulos transitables. Si ambos lados puntúan igual → `two_sided = true`.

Para un objeto libre sobre asfalto o hierba transitable, **las cuatro caras acaban `two_sided`**:
el campo no distingue un poste cerrado de una valla abierta.

### 2.2 De segmento a contacto (antes)

`query_poly_with_candidates` recorría los vértices del polígono del coche (5 cajas + 4
rectángulos de rueda) y, para cada uno, calculaba `gap = (P - A)·n` si su proyección tangencial
caía en `[−0,10/len, 1+0,10/len]`. Guardaba el vértice más profundo como punto de contacto y
usaba la normal de la cara. Después:

- `gap > 0` → restricción especulativa: el solver solo limita la velocidad de aproximación.
- `gap ≤ 0` → el impulso normal anula la velocidad de aproximación; el *split impulse* traslada
  el coche a lo largo de `n` hasta `(penetración − 3 mm)·0,8`, con tope por muro.

---

## 3. Causas raíz

### 3.1 Plano infinito en lugar de segmento finito (causa principal)

La distancia se medía al plano de la cara, no al segmento. Ejemplo real del banco de pruebas
(poste de 0,30 × 0,30 × 3 m, coche con las cinco cajas del solver, 8° de guiñada, 1,02 m de
separación lateral):

| Caja | Gap reportado | Separación real (SAT) | Error |
| --- | ---: | ---: | ---: |
| Suelo | **âˆ’1,028 m** | 0,201 m | 1,229 m |
| Ala delantera | **âˆ’0,989 m** | 0,328 m | 1,317 m |
| Ala trasera (guiñada +4°) | **−1,627 m** | 1,339 m | 2,966 m |

La separación real se midió con SAT 2D entre el polígono de la caja y el cuadrado del poste. El
coche estaba **fuera** del poste en todos los casos; el sistema leía penetraciones de hasta 1,6 m.
El motivo geométrico es que un vértice del ala/suelo, al cruzar el plano de la cara, queda "detrás"
aunque esté a un metro del poste en la dirección tangencial; el margen de 10 cm de `u` permitía
entrar a esquinas que están justo fuera del extremo del segmento.

Además, la normal era la de la cara y el punto era el vértice del coche, no el punto del muro.
Con esquinas alejadas, el brazo de palanca respecto al centro de masas era grande y aparecía un
par (guiñada/cabeceo) sin contacto real.

### 3.2 El lado del muro se elegía con el centro del coche, con pestillo de 2 s

Para `two_sided`:

```rust
let side_sign = (car_pos - a) · n;
let locked = side_memory.get_or_set(wall_index, side_sign, time); // nunca actualizaba el signo
if locked < 0 { n = -n; }
```

El centro del coche puede estar en un lado de la línea del muro mientras la esquina que toca está
en el otro. Peor: `get_or_set` **solo actualizaba la marca de tiempo**, nunca el signo; tras cruzar
el plano, el muro seguía empujando en la dirección vieja durante 2 s. Ese es el comportamiento de
"atracción" (gap negativo detrás del plano → el *split impulse* arrastra hacia el plano) y de
"repulsión" (una vez cruzado, el signo obsoleto empuja al otro lado).

### 3.3 Split impulse por muro, sin presupuesto global

```rust
let max_push_per_step = (7.5 * dt).clamp(0.005, 0.040); // por muro
for (normal, max_pen) in wall_groups { origin += normal * push; }
```

Con `dt = 1/100` son hasta **40 mm por muro**; un poste con 4 caras son 160 mm/paso y un
contacto con hasta 16 muros puede desplazar el coche **0,5–0,64 m en un solo paso**, sin cambiar
su velocidad. A 60 m/s eso no es un empujón: es un teletransporte a través de la geometría. El
comentario del código decía "15 mm por subpaso de 2 ms", pero el paso real del host es mayor.

### 3.4 Margen especulativo usado como holgura vertical

La comprobación de altura usaba `pred` (hasta 0,40 m) también en Y:

```rust
if poly.y_max < seg.y_min - pred || poly.y_min > seg.y_max + pred { continue; }
```

Un bordillo de 10 cm podía "tocar" un ala que está 40 cm por encima. Contribuye a empujones
laterales invisibles en pianos y bordillos.

### 3.5 Clasificación: cualquier triángulo vertical no clasificado es un muro

`ac3d_track.rs:422`:

```rust
} else if ny_abs < 0.40 && height_span >= 0.35 {
    // Unclassified vertical surface near track: obstacle
}
```

Las listas de nombres (`is_ignored_element`, `is_obstacle_element`) son heurísticas. En los
circuitos del proyecto esto convierte en colisión multitud de objetos decorativos o de atrezo:

| Circuito | Triángulos transitables | Triángulos obstáculo | Segmentos | 1 lado | 2 lados | Segmentos < 1 m | Build |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Melbourne 1997 | 17.456 | 18.391 | 8.575 | 4.611 | 3.964 | 1.952 | 2,96 s |
| Fuji 1976/77 | 12.275 | 29.875 | 24.269 | 17.901 | 6.368 | 9.275 | 4,14 s |
| Russia Stage 1 | 52.660 | 64.020 | 49.090 | 43.423 | 5.667 | 12.078 | 40,4 s |
| Jarama | 0 (nombres sin palabra clave) | — | — | — | — | — | falla la carga |

El análisis por objeto (`tools/scan_track_objects.py` → `tools/vertical_objects.csv`) encuentra
4.262 objetos con triángulos de obstáculo y 1.393 objetos verticales esbeltos (altos y con huella
< 1,5 m) en los cuatro circuitos. Muchos son postes, vallas, torretas y señales legítimas; otros
son detalles que no deberían ser sólidos. El problema reportado aparece precisamente en esos
objetos cortos y verticales, porque el error del plano infinito crece con la relación entre el
tamaño del coche y el del objeto.

**Nota de rendimiento:** el `build` es O(segmentos × muestras transitables) por la fase de
orientación. Melbourne tarda 3 s, Fuji 4 s y Russia Stage 1 **40 s**; es un problema aparte, pero
afecta a la carga de la sesión.

---

## 4. Por qué el coche "va a geometría sin colisión"

La suma de los tres efectos explica el síntoma completo:

1. Un paso tangencial genera un gap negativo falso en una esquina del ala o del suelo.
2. El solver anula la velocidad normal en ese punto (con un brazo de palanca grande â†’ giro) y el
   *split impulse* desplaza el coche lateralmente.
3. Como el desplazamiento se aplica **sin cambiar la velocidad**, el coche conserva su velocidad
   y aparece "despedido" en una dirección que no corresponde a ninguna superficie.
4. Si el desplazamiento lo saca al otro lado de la valla/barrera, la normal `two_sided` con
   pestillo de 2 s sigue empujando hacia el lado viejo: el objeto parece atraerlo. Si lo saca al
   exterior de un muro de un solo lado, el gap negativo lo arrastra a través del muro.
5. Al otro lado no hay triángulos transitables (nía hierba clasificada ni suelo), los rayos de
   rueda fallan y el coche cae o queda varado: "geometría que no tiene colisión".

---

## 5. Corrección aplicada

### 5.1 Consulta de contacto finita (`wall_field.rs`)

`query_poly_with_candidates` (`wall_field.rs:718`) ahora, para cada segmento candidato:

1. **Rechazo AABB** segmento vs polígono (con `pred`) antes de tocar vértices.
2. **Holgura vertical propia** de 3 cm (`VERTICAL_TOLERANCE_M`), no el margen especulativo.
3. **Lado del muro** a partir del **centroide del polígono consultado**; `WallSideMemory` queda
   solo como histéresis cuando el centroide está exactamente sobre el plano y **actualiza el
   signo** cuando la geometría es inequívoca (`wall_field.rs:441`).
4. **Recorrido de vértices contra el segmento finito**:
   - `Q` = punto más cercano del segmento (proyección acotada a `[0, len]`).
   - Si la proyección es interior, `gap` es la distancia con signo respecto a la normal efectiva.
   - Si la proyección cae en un extremo, `gap` es la **distancia euclídea al extremo** y la normal
     es **radial** (el poste tiene esquina redondeada, no un plano infinito).
   - El punto de contacto es `Q` sobre el muro, no el vértice del coche.
5. **Extremo del muro dentro del cuerpo** (`wall_field.rs:836`): el coche cubre el poste; se le
   empuja hacia fuera por su arista más cercana (dirección `-normal exterior de la arista`).
6. **Extremo del muro fuera del cuerpo**: contacto especulativo contra la **arista** del cuerpo
   más cercana (`closest_polygon_edge`), para no perder una esquina que se acerca sin que ningún
   vértice del coche esté cerca.
7. **Muro de un solo lado**: solo recupera un cuerpo que esté como máximo 0,5 m por detrás
   (`BACK_RECOVERY_BAND_M`). Un cuerpo más atrás está fuera de ese muro y no se arrastra a través
   de él.
8. Se conservan como máximo dos contactos por segmento (el más profundo y otro separado ≥ 10 cm
   para el par).

### 5.2 Split impulse con presupuesto global (`open_formula.rs:1451`)

Se siguen agrupando las penetraciones por índice de muro, pero la traslación total del paso se
acota a `(7,5·dt).clamp(0,004, 0,020)` — **20 mm como máximo para todos los muros juntos**, frente
a 40 mm por muro. Se aplica un único vector de traslación. Esto elimina el teletransporte y deja
la recuperación en varios pasos, sin aportar energía.

### 5.3 Histéresis de lado sin pestillo

`WallSideMemory::get_or_set` actualiza el signo cuando `|current_sign| â‰¥ 0,02`; el pestillo de 2 s
ya no puede mantener el muro empujando hacia el lado del que el coche ya salió. Los tests
`two_sided_normal_follows_the_body_across_a_clear_crossing` y
`two_sided_wall_side_memory` fijan el comportamiento.

### 5.4 No se ha tocado

- Los rayos de rueda y el modelo de suspensión/contacto.
- La cinemática del coche, la masa, el solver de fricción y la respuesta de impulsos.
- La clasificación de triángulos (sigue siendo automática por nombre/normal/altura).
- El host C++ (`rustphysics.cpp`); el ABI v7 no cambia.

---

## 6. Verificación

### 6.1 Banco de contacto (`examples/wall_contact_probe.rs`)

Poste de 0,30 m, cinco cajas del solver, barrido de separaciones 1,02–1,25 m y guiñadas −8…+8°:

| Caso | Antes: gap mínimo | Separación real | Después: gap mínimo |
| --- | ---: | ---: | ---: |
| 1,02 m / −8° / suelo | −1,028 m | 0,201 m | **+0,061 m** |
| 1,02 m / −8° / ala delantera | −0,989 m | 0,328 m | **+0,353 m** |
| 1,02 m / +4° / ala trasera | −1,627 m | 1,339 m | **+0,373 m** |
| 1,25 m / −8° / ala delantera | −0,989 m | 0,129 m | **+0,362 m** |
| 1,25 m / +8° / ala trasera | −1,565 m | 1,259 m | **+0,360 m** |

En el barrido completo posterior a la corrección no aparece **ningún** `gap` negativo. Donde el
coche está realmente solapado, el gap negativo coincide con el solape real (test de solape de
5 cm).

### 6.2 Tests

```
cargo test --test wall_field_tests        # 11/11, también en --release
cargo test --test wheel_centre_contact    # 16/16
cargo test --test contact_ray_count       # 5/5
cargo test --lib                          # 182 pasan; 16 fallos preexistentes de entorno
```

Los 16 fallos de `--lib` no tienen relación con este cambio: faltan rutas `game/data/...` en este
árbol y hay un test de equilibrio estático que ya fallaba en la línea base. El informe
`collision_unification_2026_10_24` registraba 27 fallos en esa misma línea base.

Tests nuevos en `tests/wall_field_tests.rs`:

| Test | Qué fija |
| --- | --- |
| `short_post_does_not_report_penetration_for_a_clear_tangential_pass` | El caso reportado: 3 poses claras, ningún gap negativo |
| `a_real_overlap_uses_the_wall_surface_point_and_the_wall_normal` | El solape real usa el punto del muro y su normal |
| `contacts_beyond_a_segment_end_use_a_radial_normal` | Esquina del poste â†’ normal radial, no normal de cara |
| `one_sided_wall_does_not_drag_a_body_from_behind_it` | Un muro no arrastra desde metros atrás; solo recupera el túnel somero |
| `low_wall_does_not_collide_with_a_high_body_box` | El margen horizontal no eleva un bordillo bajo hasta el ala |
| `two_sided_normal_follows_the_body_across_a_clear_crossing` | El lado sigue al cuerpo al cruzar; sin pestillo |
| `a_wall_end_under_the_body_is_pushed_out_through_the_nearest_body_edge` | Poste cubierto por el cuerpo → empuje hacia la arista más cercana |

### 6.3 Instalación

Se compiló el motor y se instaló la DLL (el juego no estaba en ejecución):

```
cargo build --release --manifest-path rust/vehicle-physics-engine/Cargo.toml --lib
# copia de rust/target/release/vehicle_physics_engine.dll
#   -> distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll
```

| Artefacto | SHA-256 |
| --- | --- |
| DLL instalada (corregida) | `FA74DF7A4B36D898CA59EE8427A4C18D47F27149F7586C88592BB5AF750C3FD6` |
| DLL instalada antes de la corrección (copia en `_baseline/`) | `92FFF88B8C4062AB94CF4E5438D9436E2F541AF866C3171E8AD0CF9BD3E53EFC` |
| DLL de la línea base de la intervención anterior (restaurada) | `4AEDAB2346809729F32ED63373256D2D22457E75D4BD3C11B52C951BC5AFAFFB` |

Comprobado con `LoadLibrary` + `GetProcAddress`: la DLL carga y exporta `ofp_abi_version = 7`,
`ofp_abi_signature = 0xA91CDEBA9D3FE775`, `ofp_step` y `ofp_load_track_visual_world`.

**Reversión:** copiar `_baseline/vehicle_physics_engine_pre_fix_2026_10_27.dll` sobre
`distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` y reiniciar el
juego. También está la DLL corregida en `_baseline/vehicle_physics_engine_fixed_2026_10_27.dll`.

---

## 7. Riesgos residuales y siguientes pasos

1. **Solape profundo con un sólido cerrado.** La consulta sigue siendo por cara. Para un cuerpo
   muy metido dentro de un poste/edificio, la cara opuesta puede sumar una penetración
   sobreestimada (el caso sintético de una caja de 0,8 m que atraviesa un poste de 0,3 m da
   âˆ’0,55 m desde la cara lejana). El presupuesto global de 20 mm/paso evita el teletransporte,
   pero la dirección no es el MTV verdadero. La solución completa es agrupar segmentos en
   **polígonos sólidos** (loops cerrados) y resolver polígono↔polígono por SAT (o usar los
   contact manifolds de Parry). Es el siguiente paso natural si se quiere robustez total en
   impactos frontales a postes.
2. **Distancia arista↔arista.** Se añadió extremo↔arista, pero no arista↔arista; una arista
   paralela del coche puede estar más cerca que cualquier vértice. Afecta al margen especulativo
   (llegada tarde), no al falso empuje ya corregido.
3. **Clasificación/atrezo.** Conviene filtrar por espacio (solo obstáculos a ≤ X m de superficie
   transitable y con altura dentro del rango del coche) y/o por lista blanca de objetos sólidos.
   No se ha cambiado para no alterar el contenido de colisión de los circuitos.
4. **Rendimiento de carga.** `WallField::build` tarda 3–40 s por la orientación O(n·m). Un
   `HashMap`/grid de muestras transitables por celda lo bajaría a milisegundos.
5. **Barreras XML sin mundo visual.** Si un `.acc` no produce ningún triángulo de obstáculo pero
   sí transitables, `g_visualWorldInstalled` queda activo y el host deja de enviar los contactos
   de barrera XML. Sería prudente que el host los enviara también cuando
   `track_obstacle_triangles() == 0` (o que el campo incorporara las barreras del XML).
6. **Objetos por encima del coche.** El filtro vertical de 3 cm evita que banderas/paneles altos
   toquen el ala; aun así, la altura del contacto se toma del centro de la caja del cuerpo, no de
   la suspensión real en ese instante.

---

## 8. Cómo reproducir el análisis

Desde la raíz del repositorio, con `cargo` en `C:\Users\bill\.cargo\bin\cargo.exe`:

```powershell
cargo test --manifest-path rust/vehicle-physics-engine/Cargo.toml --test wall_field_tests
cargo run  --manifest-path rust/vehicle-physics-engine/Cargo.toml --example wall_contact_probe
cargo run  --release --manifest-path rust/vehicle-physics-engine/Cargo.toml `
  --example wall_field_stats -- `
  distribution/share/games/speed-dreams-2/tracks/circuit/melbourne-nineteen-ninety-seven/melbourne-nineteen-ninety-seven.acc
```

`wall_contact_probe` imprime, para cada pose, el gap reportado, la separación real por SAT y el
"fantasma" (diferencia). `wall_field_stats` imprime el recuento de segmentos, lados y tiempos de
carga. `tools/scan_track_objects.py` reproduce la clasificación por nombre/normal/altura y genera
`tools/vertical_objects.csv`.

---

## 9. Archivos tocados

| Archivo | Cambio |
| --- | --- |
| `rust/vehicle-physics-engine/src/wall_field.rs` | Consulta finita, normal radial, lado por centroide, holgura vertical, recuperación acotada, contacto en el muro |
| `rust/vehicle-physics-engine/src/open_formula.rs` | Presupuesto global del split impulse (20 mm/paso) |
| `rust/vehicle-physics-engine/tests/wall_field_tests.rs` | 7 tests de regresión nuevos |
| `rust/vehicle-physics-engine/examples/wall_contact_probe.rs` | Banco de medida antes/después |
| `rust/vehicle-physics-engine/examples/wall_field_stats.rs` | Estadísticas del campo real |
| `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` | DLL recompilada e instalada |

