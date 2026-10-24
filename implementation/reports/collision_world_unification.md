# E1 — Un único mundo físico estático por sesión

**Entrega:** E1 (Puerta B)
**Archivos:** `rust/vehicle-physics-engine/src/collision_world.rs` (nuevo),
`src/open_formula.rs`, `src/open_formula_ffi.rs`, `src/lib.rs`,
`src/modules/simu/rustphysics/{rustphysics.cpp,rustphysics.h,rust_library.h,rust_library.cpp,frame_transform.h}` (nuevo).
**Estado:** implementado y validado en Rust; el adaptador C++ está modificado pero **no compilado**
en este entorno (ver §7 de `contact_integration_validation.md`).

---

## 1. Qué se sustituyó

| Antes | Después |
| --- | --- |
| `static TRACK_GROUND: OnceLock<GroundWorld>` (`open_formula.rs:218`) | registro de sesión `Mutex<Option<RegistryState>>` con generación e historial de instalaciones (`collision_world.rs`) |
| `set_track_ground_mesh()` que **no sustituía** un mundo existente | `install_track_ground_mesh()` que **siempre** sustituye e invalida (generación +1) |
| mundo vacío silencioso para ruta `""` | error explícito; el mundo se retira y la creación de vehículos falla |
| sin identidad | `WorldIdentity { circuit_id, revision, source, source_path, geometry_sha256, triangle_count, aabb_min, aabb_max }` |
| `GroundWorld` público con `f32`-free pero sin hash ni AABB | `CollisionWorld` inmutable: `TriMesh` de Parry3D + materiales por triángulo + identidad |
| sin descarga | `uninstall_world()`; `RustPhysics::shutdown` llama a `ofp_release_track_ground_mesh` |
| — | `WorldStatus`: `Absent` / error explícito / `Loaded` |

## 2. Diseño de propiedad y vida útil

```
install_track_ground_mesh(path, circuit_id, revision)
        │  carga, valida y calcula identidad
        ▼
  collision_world::install_world(CollisionWorld)  → generation += 1 ; current = Arc<CollisionWorld>
        │
        ├─ OpenFormulaSimulation::create*  →  captura (generation, Arc) en `world` / `world_generation`
        │        · varios vehículos comparten el mismo Arc (sin copia geométrica)
        │        · un `Arc` vivo no puede quedar colgando: la sesión anterior lo mantiene si existe
        │
        └─ nueva sesión → install_world() sustituye y sube la generación
                 │
                 └─ vehículo antiguo: `step()` → Err("the collision world ... was replaced or released")
                    (fallo explícito, estado congelado en el último paso aceptado, diagnóstico)
```

* **Ninguna simulación conserva referencias colgantes**: la propiedad es compartida (`Arc`), y la
  obsolescencia se detecta por generación, no por puntero.
* **Un circuito nuevo invalida el anterior**: `install_world` retira el `Arc` del registro y avanza
  la generación; `uninstall_world` hace lo mismo al cerrar sesión.
* **Varios vehículos**: cada uno captura el mismo `Arc`; no hay una segunda copia de triángulos.

## 3. Identidad verificable

* `geometry_sha256`: SHA-256 sobre `signature`, número de triángulos, `friction` y
  `rolling_resistance` (bits IEEE-754 exactos) y las nueve coordenadas **ya en el marco Rust**, de
  modo que el hash describe lo que la física consume, no el texto del archivo.
* `circuit_id`: el host pasa `Header/name` del circuito (`rustphysics.cpp::trackGroundSource`); si
  falta, se usa el nombre del archivo.
* `revision`: `Header/version` del circuito (Fuji = 4). Permite distinguir dos exportaciones del
  mismo nombre.
* `triangle_count` y `AABB` en coordenadas físicas.
* Publicación: `ofp_world_identity_json()` (nueva función, sin cambiar el diseño de `OfpSnapshot`) y
  la línea de log `RUSTWORLD reason=... generation=... installs=... identity={...}`.

Reproducción: `the_same_circuit_document_hashes_identically_and_a_different_one_does_not`
(mismo archivo → mismo hash; un vértice movido 1 cm → hash distinto; revisión distinta → identidad
distinta pero geometría coincidente).

## 4. Mundo disponible ≠ malla vacía

| Situación | Antes | Ahora |
| --- | --- | --- |
| Circuito sin `ground collision mesh` | mundo vacío, caída silenciosa | `ofp_load_track_ground_mesh` → error; no se instala mundo |
| Documento ilegible / firma errónea | error en el log, mundo anterior conservado | error, mundo anterior **retirado**, `ofp_world_last_error` con el motivo |
| Documento con `0` triángulos | mundo vacío aceptado | rechazado: «an empty world is not a world» |
| Vehículo creado sin mundo (ruta host) | se creaba y caía | `ofp_create` falla con mensaje explícito |

`OpenFormulaSimulation::create_with_world(..., require_world = true)` es la ruta del host;
`create()` (sin exigencia) sigue disponible para pruebas unitarias y para el contrato heredado.

## 5. Materiales por triángulo

Cada `CollisionWorld` conserva `TriangleMaterial { friction, rolling_resistance }` por triángulo y lo
publica en `GroundHit { friction, rolling_resistance, triangle }`. `classify_surface` sigue
existiendo como **etiqueta de presentación** (tipo de superficie para HUD/audio), pero la física
consume los valores explícitos y el índice de triángulo, no la etiqueta.

Además, la normal devuelta **siempre se opone a la dirección de consulta**: un triángulo con el
sentido invertido en la malla exportada no puede reportar una normal descendente y tirar del
vehículo hacia el suelo (probado en `raycast_reports_distance_point_normal_and_material`). El
material se resuelve plegando la codificación de parry (`Face(i + n)` = cara trasera del triángulo
`i`), así que una malla sin winding garantizado conserva materiales correctos.

## 6. COM / origen: contrato de pose corregido

* `publishCar`: `DynGCg.pos = COM = origin + basis · com_local` (una sola definición,
  `ofp::frame::com_from_origin`).
* `setCar`: `origin = COM − basis · com_local`, con la base completa construida desde
  `pos.ax/ay/az` (`ofp::frame::basis_from_sd_euler`), no sólo yaw.
* `createCar`: el desplazamiento de aparición usa la base de spawn completa; antes se perdían los
  términos `sin(yaw)`.
* `OfpWheelState.hub_position` publica ahora **el mismo centro físico** que consultan los rayos
  (`suspension::wheel_centre_local`), de modo que presentación, contacto y diagnóstico no pueden
  divergir.

Verificación:

| Prueba | Resultado |
| --- | --- |
| `frame_transform_test.exe` (MSVC): involución SD↔Rust, Euler→base→Euler, ortonormalidad, mano derecha, COM↔origen con pitch/roll | 4 grupos, 0 fallos |
| `adapter_geometry_check.exe` (MSVC): `setCar(publishCar(state))` en 4 actitudes | error de origen < 1e-9 m; base preservada < 1e-12 |
| `adapter_geometry_check.exe`: desplazamiento de spawn con yaw 0,7 rad frente a la fórmula anterior | diferencia > 1e-3 m (el defecto era real) |
| `frame_transform_test.exe`: COM ≠ origen | desplazamiento 0,230 m (medido) |
| `the_wheel_centre_follows_the_unsprung_state_and_the_hub_offsets` (Rust) | 1 cm de compresión = 1 cm de centro; `hub_position` == centro publicado (< 1e-9 m) |

## 7. ABI y compatibilidad

* **`OfpSnapshot` y el resto de estructuras no cambian** (tamaños y offsets de ABI v7 intactos:
  3480 bytes, `wheel_diagnostics` en 2240, `failure` en 3392). No se requiere recompilar el host
  para que la biblioteca cargue.
* Se **añaden funciones** (ABI por símbolos, no por estructura):
  `ofp_load_track_ground_mesh_v2`, `ofp_release_track_ground_mesh`, `ofp_world_generation`,
  `ofp_world_install_count`, `ofp_world_identity_json`, `ofp_world_last_error`,
  `ofp_contact_report`.
* `rust_library.cpp` las resuelve **opcionalmente** y el adaptador cae a
  `ofp_load_track_ground_mesh` si `_v2` no existe, de modo que una biblioteca anterior sigue
  cargando.
* `ofp_load_track_ground_mesh` conserva su firma y su significado (carga y devuelve triángulos), pero
  cambia el comportamiento: **sustituye** en lugar de reutilizar.

## 8. Lo que NO se hizo (y por qué)

1. **Exportar la geometría física de `tTrack`** en lugar del documento derivado de la malla visual.
   Es la opción preferida por el encargo, pero exige compilar y ejecutar el host para verificar
   paridad (triángulos, curbs, rasantes) y este entorno no tiene toolchain del juego. Se deja como
   trabajo pendiente con interfaz preparada: `CollisionWorld::from_host_export(circuit_id, revision,
   triangles, materials)` acepta triángulos en el marco Rust **sin pasar por archivo**, y
   `install_world` no distingue el origen (`source = "host_export"` / `"transfer_file"`).
2. **Sustituir el archivo de transferencia por una consulta FFI por rayo.** Descartado por el propio
   encargo (coste y vida útil); no se implementó ninguna llamada por rayo.
3. **Rapier3D.** No se introdujo; Parry3D sigue siendo sólo proveedor de consultas.

## 9. Pruebas de la Puerta B

| # | Requisito | Prueba | Resultado |
| --- | --- | --- | --- |
| 1 | Cargar, descargar y cambiar de circuito en el mismo proceso con IDs y hashes correctos | `a_new_circuit_invalidates_the_previous_vehicle_geometry`, `releasing_the_world_also_invalidates_live_vehicles`, `the_same_circuit_document_hashes_identically_and_a_different_one_does_not` | PASS |
| 2 | AABB, triángulos, normales, alturas y materiales coinciden con la fuente para puntos de control | `transfer_file_round_trip_keeps_the_frame_mapping`, `raycast_reports_distance_point_normal_and_material`, `tilted_ground_reports_the_perpendicular_gap_not_the_slant_distance` | PASS (la malla visual es la fuente actual; el contraste contra `tTrack` queda pendiente por falta de toolchain) |
| 3 | `setCar(publishCar(state))` preserva pose y COM con pitch/roll ≠ 0 | `adapter_geometry_check.exe` (4 actitudes) | PASS (< 1e-9 m) |
| 4 | Sin doble integración del chasis ni dos fuentes capaces de aplicar el mismo impulso | auditoría §2/§6.7 + `barrier_impulse_ns` y `body_contact_impulse_ns[]` instrumentados | PASS estructural |
| 5 | Tasa de consultas compatible, con medición | contadores `wheel_ray_queries/hits`, `floor_ray_queries/hits` publicados en el snapshot y en el informe de contacto | 12 + 2 consultas por paso aceptado, constante (ver §5 de `contact_integration_validation.md`) |
