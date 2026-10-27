# Rally Rusia Stage 1 + Ford Escort RS2000 — colisión y térmica

**Fecha:** 2026-10-27
**Ámbito:** `russia-stage1` (visual `.acc`), clasificador `ac3d_track.rs`, campo de muros
`wall_field.rs` y perfil `rust/data/vehicles/ford-escort-rs2000`.
**Pregunta 1:** ¿todo el tramo 1 del rally ruso tiene colisión al salirse de la pista?
**Pregunta 2:** ¿hay que adaptar la ventana óptima de temperatura de los neumáticos del Ford para rally?

---

## 1. Tramo 1 de Rusia: qué había y qué se ha corregido

### 1.1 Respuesta corta

**No. Antes de este cambio el tramo no tenía colisión fuera de la pista.** El `.acc` exporta
todos los materiales con el prefijo `tree_visual_`, y el parser ignoraba **cualquier** objeto cuyo
nombre contuviera `tree`. Eso eliminaba:

- los **troncos de árbol** (`tree_visual_russian_tree_trunks`, 8.571 triángulos);
- los **graneros** (`haybarn`), **postes** (`pylon`), **señales** (`sign_*`), **rocas**
  (`stone_*`) y **tiendas** (`tent*`), 16.796 triángulos sólidos en total;
- el **terreno exterior** (`tree_visual_terrain_grass_1`, 28.345 triángulos), que era el único
  suelo más allá de la franja de hierba que acompaña a la carretera.

Solo colisionaban los 20 objetos `wall_solid_*` (3.635 triángulos: vallas, casas, algunos
edificios) y el suelo de la carretera + franja interior. Además, el XML del circuito define
`Left/Right Side`, `Border` y `Barrier` con **anchura 0**, así que no hay barreras XML: toda la
colisión depende del modelo visual.

### 1.2 Cifras del modelo

| Categoría de objetos del `.acc` | Objetos | Triángulos | Antes | Ahora |
|---|---:|---:|---|---|
| `grass_terrain_*` | 11 | 71.819 | suelo interior | suelo |
| `road_gravel_*` | 5 | 43.195 | suelo | suelo |
| `tree_visual_terrain_grass_1` | 1 | 28.345 | **ignorado** | **suelo exterior** |
| `tree_visual_*` sólidos (troncos, graneros, postes, señales, rocas, tiendas) | 23 | 16.796 | **ignorados** | **obstáculos** |
| `tree_visual_*` visuales (copas/forest cards, arbustos, público, pancartas, tendidos, zanjas, erosión) | 21 | 56.360 | ignorados | se quedan visuales (correcto) |
| `wall_solid_*` (vallas, casas, edificios, rocas) | 20 | 3.635 | obstáculos | obstáculos |

El mundo de colisión pasó de **52.660** triángulos transitables y **64.020** de obstáculo a
**81.002** transitables y **76.309** de obstáculo. El campo de muros pasó de 49.090 a **60.896**
segmentos (42.684 de un lado, 18.212 de dos lados).

### 1.3 Cobertura real de suelo fuera de la pista

Muestreo sobre la trazada real (`staging_rally/integration/route_segments.json`, 983 segmentos),
lanzando un rayo hacia abajo desde 120 m por encima de la carretera en cada desplazamiento
lateral. "Sin suelo" = no hay ningún triángulo transitable bajo ese punto.

| Desplazamiento lateral | Antes (sin suelo) | Ahora (sin suelo) |
|---:|---:|---:|
| 0 m (centro) | 0,0 % | 0,0 % |
| 3 m (borde) | 0,4 % | 0,0 % |
| 5 m | **16,2 %** | **2,6 %** |
| 8 m | **93,1 %** | **7,5 %** |
| 12 m | **92,6 %** | **0,6 %** |
| 20 m | **63,0 %** | **14,6 %** |
| 30 m | **62,5 %** | **27,4 %** |
| 50 m | **87,1 %** | **32,4 %** |

Lectura:

- Hasta 12 m el suelo es prácticamente continuo (99,4 % de aciertos). Los huecos de 8 m se
  concentran en **puentes y desfiladeros** (el rayo cae al cauce, no al terreno).
- A 20–50 m siguen existiendo huecos reales: son el **borde del escenario visual**. Ahí están las
  tarjetas de bosque (que siguen siendo visuales para no crear muros invisibles) y los troncos,
  que ahora **sí** colisionan. Entre tronco y tronco puede quedar un hueco por el que caer.
- Si se quiere garantizar que **nunca** se caiga en ningún punto, el siguiente paso es un límite
  de recuperación/barrera generado a ~20–25 m de la trazada, o ampliar el terreno. No se ha
  añadido un muro invisible por decisión de diseño; se puede hacer si se pide.

### 1.4 Rendimiento: el campo de muros tardaba 40 s en cargar

La clasificación nueva añadía 28.000 triángulos de terreno y 17.000 de props. El cálculo de
orientación de muros era O(segmentos × muestras de suelo): pasó de 40,4 s a **83,6 s** para Rusia.
Se sustituyó la búsqueda de la muestra de suelo más cercana por una **rejilla espacial de 10 m**
(la ventana de 20 m cubre el radio útil máximo de 15 m; el resultado es idéntico).

| Circuito | Antes | Con la clasificación nueva (fuerza bruta) | Con la rejilla |
|---|---:|---:|---:|
| Rusia Stage 1 | 40,44 s | 83,59 s | **1,42 s** |
| Melbourne 1997 | 2,96 s | — | **0,070 s** |
| Fuji 1976/77 | 4,14 s | — | **0,111 s** |

La caché `.ofpcache` del tramo ruso se regeneró con el nuevo clasificador
(`OFP_TRACK_CACHE6`, 14,45 MB) y tarda 0,21 s en cargar.

### 1.5 Qué queda visual (a propósito)

`tree_visual_russian_tree_plane` (43.896 triángulos de copas), `forest_mixed_*`, `road_forest`,
`bush_4`, `audience_*`, `sponsors`, `string_red`, `checkpoint`, `power_line_1/cord`,
`ditch_*`, `sand_erosion_*`. Convertir esos planos en sólidos crearía paredes invisibles en
medio del bosque. Los **troncos** ya están; el coche choca con el árbol, no con su copa.

### 1.6 Cambios de código

- `rust/vehicle-physics-engine/src/ac3d_track.rs`
  - `is_hidden_rally_terrain`: `tree_visual_terrain_grass_*` â†’ suelo.
  - `is_solid_rally_prop`: `tree_visual_*` + `trunk|haybarn|pylon|stone|sign|tent` → obstáculo.
  - El chequeo de ignorados se salta para esos dos roles.
  - `CACHE_MAGIC` → `OFP_TRACK_CACHE6` para invalidar las cachés con la clasificación vieja.
  - Test `rally_visual_tags_route_ground_and_solid_props`.
- `rust/vehicle-physics-engine/src/wall_field.rs`
  - Orientación de muros con rejilla espacial de muestras transitables.

### 1.7 Validación

- `cargo test --lib ac3d_track`: 3/3 (incluye el test nuevo y Melbourne real).
- `cargo test --test wall_field_tests`: 11/11.
- Validación nativa del tramo: `TRACK_VALID` con `triangle_count=81002`,
  `walls: segments=60896 one_sided=42684 two_sided=18212`; 600 pasos, `rejected=0`; el coche queda apoyado al final (4/4) y al inicio 2/4 por la pendiente del spawn.
- La cobertura de la tabla se reproduce con `cargo run --example stage_route_coverage`.

---

## 2. Ford Escort RS2000: ventana térmica de rally

### 2.1 Situación inicial

`rust/data/vehicles/ford-escort-rs2000/tires.json` **no tenía bloque `thermal`**. El cargador
aplica entonces `TireThermalConfig::default()`:

| Parámetro | Valor por defecto |
|---|---:|
| `optimal_tread_temperature_c` | 95 °C |
| `optimal_tread_window_c` | 12 °C → ventana 89–101 °C |
| `overheat_tread_temperature_c` | 120 °C |
| `optimal_carcass_temperature_c` | 80 °C |
| `initial_temperature_c` | 25 °C |

Esa es la ventana de un **slick de circuito**, no la de un neumático de grava. El propio
`port_validation.json` declaraba "thermal parameters" como dominio **estimado**, no validado.

### 2.2 Lo que ocurre en grava

Banco `ford_thermal_probe` (perfil Ford, plano de grava, 40 s de slalom a ~15 m/s; el coche parte
a 25 °C):

| t | Presión media | Banda de rodadura | Carcasa | Grip scale con 95 °C | Grip scale con 75 °C |
|---:|---:|---:|---:|---:|---:|
| 0 s | 180,0 kPa | 25,0 °C | 25,0 °C | 0,703 | 0,716 |
| 20 s | 182,5 | 37,5 / 38,4 | 25,4 | 0,761 | 0,811 |
| 40 s | 182,7 | 51,3 / 52,3 | 26,9 | **0,825** | **0,910** |

La banda de rodadura es la medida en cada run (95 °C / 75 °C); cada columna de grip corresponde a su propia temperatura, con menos de 1 °C de diferencia.

Con la ventana de 95 °C el neumático **nunca entra en la zona óptima** (89–101 °C) en una etapa
de grava; se queda en 0,70–0,83 de grip. Además, la presión de referencia del propio Ford
(180 kPa en frío → 210 kPa en caliente, referencia 25 °C) implica por gas ideal una temperatura
de gas de ~75 °C:

```
T_hot = (25 + 273.15) * (210 + 101.325) / (180 + 101.325) = 347.9 K ≈ 74,6 °C
```

Ese dato respalda una ventana de rally alrededor de **75 °C**, no de 95 °C.

### 2.3 Ventana aplicada

Se añadió a `tires.json` (fuente y copia instalada en `distribution/.../cars/rust/...`):

```json
"thermal": {
  "optimal_tread_temperature_c": 75.0,
  "optimal_tread_window_c": 20.0,
  "overheat_tread_temperature_c": 110.0,
  "optimal_carcass_temperature_c": 65.0
}
```

Resultado:

- ventana óptima **65–85 °C** (20 °C de ancho, el doble que el slick);
- `grip_scale` 1,0 en 65–85 °C; 0,91 a 52 °C; 0,94 al empezar el sobrecalentamiento (110 °C);
- el coche gana ~10 % de grip en la zona donde realmente trabaja en grava; en el banco pasa de
  14,8 a 16,8 m/s con el mismo input;
- la carcasa óptima baja de 80 a 65 °C (solo afecta a rigidez vertical ±12 %, no al grip);
- el HUD usa los valores del perfil (`specs.tread_optimal_temperature_c`, `..._window_c`,
  `..._overheat_temperature_c` en `rustphysics.cpp`), así que la banda y los colores cambian sin
  tocar la configuración del HUD.

### 2.4 Lo que sigue siendo estimado

- Las **capacidades y conductancias** del modelo siguen siendo las genéricas:
  `tread_zone_heat_capacity=4200`, `carcass_heat_capacity=11000`,
  `tread_to_carcass=45`, `tread_to_air=25`, `carcass_to_air=12`, `road_conductance=55`,
  `lateral=18`. Son plausibles para un neumático de grava pesado, pero no están validadas.
- El **ambiente y la pista** siguen usando `ambient_fallback_c=25` y `track_fallback_c=35` porque
  el motor no recibe temperatura de sesión: `simulation.rs:1173` usa
  `TireEnvironment::fallback(tire_thermal)`. Para una etapa rusa con nieve eso es optimista; si se
  quiere modelar frío real hay que bajar esos dos valores (o llevar la temperatura de sesión al
  motor), y volver a calibrar la ventana.
- La ventana de 75 °C es una **calibración de rally**, no una medida de un neumático concreto;
  conviene validarla con telemetría de una vuelta real (HUD: banda de rodadura, carcasa, presión)
  y ajustar si la etapa es más fría o más rápida.

### 2.5 Cambios de datos

- `rust/data/vehicles/ford-escort-rs2000/tires.json` (bloque `thermal`).
- `distribution/share/games/speed-dreams-2/cars/rust/ford-escort-rs2000/tires.json` (idéntico).
- `port_validation.json` (ambas copias): nota `rally_thermal_window`.
- SHA-256 de `tires.json`: `FCF1B01486CF45091CD96EFC1250F7890B26277F222D832095DA903BC23B9D79`.

---

## 3. Instalación y reversión

Se recompiló e instaló el motor:

| Artefacto | SHA-256 |
|---|---|
| `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` (rally fix) | `11AF706E0C5F1DC9C92207911D73F19E520604F43F3E246E9143DD4924834761` |
| Copia de la DLL anterior (fix de colisión de la vuelta 1) | `_baseline/vehicle_physics_engine_turn1_fixed.dll` |
| Copia de la DLL nueva | `_baseline/vehicle_physics_engine_rally_fixed.dll` |
| Caché nueva del tramo | `_baseline/russia-stage1.acc.ofpcache` |

El juego **no estaba en ejecución**; hay que reiniciarlo para cargar la DLL, la caché nueva y el
perfil del Ford. La DLL carga y reporta ABI 7 (`ofp_abi_signature=0xA91CDEBA9D3FE775`).

**Reversión de colisiones:** copiar `_baseline/vehicle_physics_engine_turn1_fixed.dll` sobre la
DLL instalada. **Reversión térmica:** eliminar el bloque `thermal` de los dos `tires.json` (vuelve
a 95/12/120/80).

---

## 4. Comandos de reproducción

```powershell
# Cobertura del tramo (antes/después depende de la clasificación compilada)
cargo run --manifest-path rust/vehicle-physics-engine/Cargo.toml --example stage_route_coverage

# Tiempo y segmentos del campo de muros
cargo run --release --manifest-path rust/vehicle-physics-engine/Cargo.toml `
  --example wall_field_stats -- `
  distribution/share/games/speed-dreams-2/tracks/circuit/russia-stage1/russia-stage1.acc

# Térmica del Ford
cargo run --manifest-path rust/vehicle-physics-engine/Cargo.toml `
  --example ford_thermal_probe -- 40 3

# Tests
cargo test --lib ac3d_track
cargo test --test wall_field_tests
```


