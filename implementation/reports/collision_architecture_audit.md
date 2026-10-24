# Auditoría de arquitectura de colisiones — Open Formula / Speed Dreams ↔ Rust

**Fecha:** 24 de octubre de 2026
**Alcance:** `src/modules/simu/rustphysics/*` (host) y `rust/vehicle-physics-engine` (autoridad física).
**Base medida:** rama `codex/initial-port`, HEAD `b045d4d`, árbol de trabajo con los cambios de esta
intervención. Estado de partida: `git status` limpio salvo informes sin seguimiento; no había cambios
del usuario pendientes de confirmar dentro de `rust/`, `src/` ni `distribution/`.

> Este informe es la **Puerta A**. No se implementó ningún cambio de contacto antes de establecer
> quién carga, quién consulta y quién aplica. Todas las afirmaciones llevan ruta y línea del código
> inspeccionado o una prueba ejecutada.

---

## 1. Recorrido real de la geometría: del circuito al estado dinámico

```
circuito XML (tTrack::params)
  └─ Header / ground collision mesh            ← src/modules/simu/rustphysics/rustphysics.cpp:248-257 (antes)
       └─ GfDataDir() + ruta                    →  share/games/speed-dreams-2/tracks/circuit/<id>/<id>_ground_collisions.txt
            └─ ofp_load_track_ground_mesh(path)  ← open_formula_ffi.rs:521
                 └─ set_track_ground_mesh()      ← open_formula.rs:221 (antes) → OnceLock<GroundWorld>
                      └─ GroundWorld::load       ← open_formula.rs:76  (Parry3D TriMesh + materiales)
                           └─ sample_wheel_rays / sample_floor  (12 + 2 rayos por paso)
```

### 1.1 Origen y naturaleza del documento `FORMULA_GROUND_1`

* El documento **no lo genera Speed Dreams**. Lo produce `tools/prepare_fuji_object_collisions.py`
  a partir de la **escena visual** (`convert_binary_scene.BinaryScene`, es decir el `.acc`),
  aplicando una matriz de alineación externa (`tools/*alignment*.json`) y filtrando por categoría de
  material (`metadata/surfaces.json`, mediana de `dry_grip`).
* Por tanto la geometría que consulta Rust es una **copia derivada de la malla visual**, no la
  representación física de `tTrack`. La física de referencia de Speed Dreams para el suelo es el
  modelo de segmentos (`tTrackSeg`, con `vertex[4]`, peralte, rasante y pianos), que es lo que usan
  `RtTrackGlobal2Local` / `RtTrackSideNormalG` y el solver `simuv5`.
* El mismo documento lo consume **también** `simuv5` con su propio sampler
  (`src/modules/simu/simuv5/track_ground_mesh.cpp`, cargado en `simuv5/simu.cpp:764`). Los dos
  motores son mutuamente excluyentes (sólo se carga un `IPhysicsEngine`), así que no compiten en
  ejecución, pero **hay dos lectores del mismo archivo con dos interpretaciones distintas**.
* Consecuencia para la Fase B: existe una fuente física autoritativa del host (segmentos `tTrack`)
  que **hoy no se exporta**, y un archivo de transferencia derivado de la geometría visual. La
  unificación elegida en esta entrega (ver `collision_world_unification.md`) conserva el documento
  como fuente de la sesión pero lo dota de **identidad verificable y ciclo de vida**, y deja
  documentado el exportador desde `tTrack` como trabajo pendiente (no se puede validar sin compilar
  el host; ver §7 del informe de validación).

### 1.2 Formato y escala

`FORMULA_GROUND_1 <n>` seguido de 11 números por triángulo: `friction rolling x1 y1 z1 x2 y2 z2 x3 y3 z3`
en coordenadas **Speed Dreams** (X adelante, Y izquierda, Z arriba). La conversión a Rust
(X derecha, Y arriba, −Z adelante) se hace **una sola vez** al importar
(`open_formula.rs:124-131` antes; ahora `collision_world.rs::build`). Fuji declara 8091 triángulos
(1.099.112 bytes). Los materiales se clasifican por `rolling_resistance` en
`classify_surface` (heurística de presentación; los valores explícitos de fricción y resistencia
viajan por triángulo y llegan al modelo de neumático desde la corrección de seguimiento del 24 de
octubre; la primera versión auditada los descartaba al construir `RaycastHit`).

### 1.3 Escala y transformación

No hay escala: las coordenadas se usan en metros tal como se exportan. La transformación es
únicamente el cambio de base rígido `(x,y,z)_SD → (−y, z, −x)_Rust`, idéntico para posiciones y
direcciones. No hay offset vertical ni corrección de pendiente al importar.

---

## 2. Matriz de autoridades

| Responsabilidad | Quién la ejerce (código) | Verificación |
| --- | --- | --- |
| Cargar el circuito y decidir qué malla existe | Speed Dreams (`rustphysics.cpp::initialize`, `trackGroundSource`) | `initialize` lee `Header/ground collision mesh` y llama al cargador; ninguna otra ruta crea geometría de pista |
| Construir el índice geométrico de consulta | Rust (`CollisionWorld`, Parry3D `TriMesh`) | estructura inmutable, sin masa ni integrador (`collision_world.rs`) |
| Integrar posición, velocidad y orientación | Rust (`VehicleSimulator`, `simulation.rs`) | el host sólo copia el snapshot a `tCarElt` (`publishCar`) |
| Aplicar impulsos de barrera | Rust (`apply_body_impulse`, una vez por contacto y paso) | `collectBarrierContacts` sólo **detecta** y envía geometría |
| Aplicar contacto de fondo | Rust (`apply_body_impulse`, unilateral) | `sample_floor` sólo mide dos holguras |
| Aplicar carga de rueda | Rust (solver vertical por eje) | el host no calcula fuerzas |
| Detección de barreras laterales | Speed Dreams (`collectBarrierContacts`, basada en ruedas) | **no** es una malla del chasis (ver §5) |
| Detección de contacto del chasis con el suelo | Nadie (no existe) | **limitación declarada** |
| Colocación/recolocación | Rust (`create`/`reset`/`set_pose`) | `setCar` traduce, no integra |

**No hay doble respuesta**: `SimCarCollide*` existe únicamente en `simuv5` y `simureplay`
(`collide.cpp`, `simu.cpp:707`); el módulo `rustphysics` implementa `IPhysicsEngine` y nunca las
llama. La única vía de impulso del host es el vector `OfpBodyContact` que Rust resuelve.

---

## 3. Subsistemas activos con `rustphysics`

| Subsistema | Estado | Evidencia |
| --- | --- | --- |
| `simuv4` / `simuv5` | **inactivos** (no cargados) | `standardgame.cpp:311-353` carga un solo módulo de física; la configuración del perfil usa `rustphysics` |
| Colisiones del host (`DtObject`, `SimCarCollideXYScene`) | **inactivas** | sólo referenciadas desde `simuv5/*` |
| Barreras laterales | **detección host → impulso Rust** | `collectBarrierContacts` + `ofp_step(contacts)` |
| Fondo | **detección Rust → impulso Rust** | `sample_floor` + `apply_body_impulse` |
| Ruedas | **detección y respuesta Rust** | `sample_wheel_rays` + solver vertical |
| Daños por barrera | host, a partir del impulso publicado | `applyBarrierDamage` acumula `damage·|J|²` |
| Publicación en `CarElt` | host, sólo lectura del snapshot | `publishCar` no escribe ningún campo que Rust vuelva a leer salvo `ctrl` |

Única realimentación host→Rust: `tCarCtrl` (mandos), `Situation` (prestart), `reconfigureCar`
(pit), `setCar` (red, sin llamador activo: `racenetwork.cpp:79` está comentado) y los contactos.

---

## 4. Convenios de ejes y orden de ruedas

* Ruedas: Speed Dreams publica `FR, FL, RR, RL`; Rust trabaja `FL, FR, RL, RR`. Tablas únicas
  `SD_TO_RUST_WHEEL = {1,0,3,2}` y `RUST_TO_SD_WHEEL = {1,0,3,2}` (`rustphysics.cpp:37-38`), usadas
  en publicación, diagnósticos, audio y HUD.
* Mundos: SD `(X adelante, Y izquierda, Z arriba)` ↔ Rust `(X derecha, Y arriba, −Z adelante)`.
  La conversión vive ahora en `src/modules/simu/rustphysics/frame_transform.h` y está verificada por
  el arnés MSVC `frame_transform_test.cpp` (involución, ortonormalidad, mano derecha).
* Actitud: `publishCar` deriva `yaw = atan2(fwd.y, fwd.x)`, `pitch = −asin(fwd.z)`,
  `roll = atan2(left.z, up.z)` (`eulerFromBasis`). `publishCar` publica las **tres** columnas
  (`basis_x/y/z`) y el yaw/pitch/roll derivados; `setCar` (antes) sólo reconstruía yaw.

---

## 5. Inventario de fuentes de colisión

| Fuente | Geometría | ¿Genera impulso? | ¿Quién lo aplica? | Comparte malla |
| --- | --- | --- | --- | --- |
| Ruedas (12 rayos) | `CollisionWorld` (documento de pista) | Sí (restricción unilateral vertical) | Rust, solver por eje | Sí (misma malla) |
| Fondo (2 sondas) | mismo `CollisionWorld` | Sí (impulso unilateral por sonda) | Rust `apply_body_impulse` | Sí |
| Barreras | **ninguna malla**: recta lateral del `tTrackSeg` más cercano al punto de rueda | Sí (impulso normal) | Rust | No |
| Chasis (volumen) | **inexistente** | No | — | — |
| Pianillos / escapatorias | mismo `CollisionWorld` (material por triángulo) | Sí, vía ruedas | Rust | Sí |
| Pista | mismo `CollisionWorld` | Sí, vía ruedas y fondo | Rust | Sí |

`collectBarrierContacts` convierte el **punto de contacto de rueda** (o el buje si la rueda está en
el aire) a coordenadas locales de pista, comprueba `toLeft/toRight` y construye un contacto con la
normal de la barrera y una penetración `−toSide`. Es una detección **basada en ruedas**, no una
malla del chasis: no detecta impactos de morro, pontón o alerón contra un muro. **Limitación
declarada**, no ocultada (ver §7 del informe de validación).

---

## 6. Problemas demostrados (con mecanismo y reproducción)

### 6.1 Mundo de colisión obsoleto al cambiar de circuito en el mismo proceso — **P0**

* Código: `open_formula.rs:218-237` (antes) `static TRACK_GROUND: OnceLock<GroundWorld>` y
  `set_track_ground_mesh()` que **retorna el recuento anterior si ya existe un mundo**, sin cargar
  nada. Complementariamente, `RustPhysics::shutdown()` ponía `g_track_ready = false`, de modo que la
  carrera siguiente volvía a llamar al cargador, que respondía «éxito» con la pista antigua.
* Efecto: `Fuji → Melbourne → Fuji` dentro de un proceso mantiene la geometría de Fuji en las
  consultas de rueda y de fondo, mientras el log del host imprime la ruta de Melbourne y su número
  de triángulos antiguo. Las alturas, normales y materiales son los del circuito anterior.
* Reproducción: prueba `a_new_circuit_invalidates_the_previous_vehicle_geometry`
  (`tests/wheel_centre_contact.rs`). En la versión anterior la instalación de la segunda pista
  devolvía el recuento de la primera; ahora se sustituye el mundo y el vehículo antiguo **rechaza
  el paso** con un mensaje explícito.
* Corregido en E1.

### 6.2 Malla vacía indistinguible de «sin suelo» — **P0**

* `set_track_ground_mesh("")` instalaba `GroundWorld::empty()` (`mesh: None`), cuyas consultas
  devuelven siempre `None`: el vehículo cae indefinidamente sin diagnóstico. Además, ese mundo vacío
  **bloqueaba** cualquier carga posterior por el mismo `OnceLock`.
* Corregido en E1: un documento vacío o ilegible es un error; si se declara pero falla la carga, se
  retira el mundo y la creación de vehículos se rechaza.

### 6.3 Raycasts de rueda anclados al chasis — **P0 de contrato**

* `open_formula.rs:686-700` (antes): origen = `wheel_anchor_local()` transformado al mundo;
  longitud = `spring_length + tire_radius`. `suspension.rs:829-830`: `target = (max_ray_length −
  weighted_distance).max(0)`.
* Consecuencia: la compresión solicitada es una magnitud **independiente del estado** de la
  suspensión. Una rueda que penetra 3 cm y un chasis que está 3 cm más alto producen la misma
  demanda; la penetración real no entra en la restricción hasta que el anclaje se hunde. El origen
  no se mueve con la masa no suspendida, así que cambiar la compresión no cambia la lectura.
* Corregido en E2 (origen = centro físico, nueva conversión hueco→demanda).

### 6.4 `setCar(publishCar(state))` no es invertible — **P1 (latente)**

* `publishCar` publica en `DynGCg.pos` el **centro de gravedad**
  (`origin + basis·com_local`, `rustphysics.cpp:266-280`).
* `setCar` reenvía ese mismo valor como **origen del cuerpo** (`sdToRust(dynGCG.pos)` →
  `ofp_set_pose`) y reconstruye la base sólo con el yaw, descartando pitch y roll
  (`rustphysics.cpp:1318-1334`).
* Error: `|com_local|` ≈ 0,230 m de desplazamiento, más la pérdida de actitud. No hay llamador
  activo (`racenetwork.cpp:79` comentado), por eso es latente, no observable en carrera.
* Corregido en E1 con `ofp::frame::origin_from_com` + base completa; el arnés MSVC mide
  `setCar(publishCar(state))` con error < 1e-9 m en 4 actitudes.

### 6.5 Desplazamiento de aparición con yaw — **P1**

* `createCar` (antes) restaba `(cg.x·cos(yaw), cg.y, cg.z·cos(yaw))` al origen: faltaban los
  términos `sin(yaw)`. Error de hasta `|cg_horizontal|·sin(yaw)` (2,75 cm medidos a yaw = 0,7 rad con el centro de masas del perfil).
* Corregido en E1; medido por `adapter_geometry_check.cpp`.

### 6.6 Dirección publicada sin signo — **P1 de telemetría**

* `snapshot.steering_angle_rad` usaba el **máximo no firmado** de los cuatro ángulos de rueda
  (`open_formula.rs:1078-1083`): un giro a la izquierda y uno a la derecha publican el mismo valor.
* Corregido en E2: media firmada del eje delantero.

### 6.7 Secuencia de impulsos dentro del paso — **P1, verificado, no reescrito**

* `OpenFormulaSimulation::step` aplica, en este orden: (1) impulsos de barrera, (2) impulsos de
  fondo, (3) resolución de suspensión/ruedas. Los dos primeros modifican `linear_velocity`,
  `angular_velocity` y las velocidades no suspendidas **antes** de que el solver vertical lea el
  estado.
* Verificación: no hay doble aplicación. Cada contacto del vector del host se aplica una vez
  (`for contact in contacts`), cada sonda de fondo una vez (`for index in 0..2`), y el solver
  vertical no vuelve a aplicar ni barrera ni fondo. El rechazo de un paso restaura el clon aceptado
  completo (`*self = accepted_before_step`), incluidos los impulsos, por lo que no quedan
  aplicaciones huérfanas.
* Riesgo residual: la secuencia es **secuencial, no acoplada**; con contacto simultáneo rueda/fondo
  el fondo se resuelve con la velocidad anterior a la carga de rueda. Se instrumenta (ver
  `contact_integration_validation.md`) pero no se convierte en solver acoplado en esta entrega.

### 6.8 Contacto de fondo sin corrección posicional — **P2**

* El impulso de fondo sólo actúa si la velocidad normal de cierre es negativa; con penetración y
  cuerpo separándose el impulso es 0 y la penetración **permanece** (no hay corrección de posición).
  Es coherente con la prohibición de `penetración/dt`, pero debe declararse: el fondo puede quedar
  solapado sin fuerza.
* Documentado; sin cambio de comportamiento en esta entrega.

### 6.9 Geometría refrescada sólo al inicio del paso — **P1**

* `step()` muestrea los 12+2 rayos una vez y los subpasos internos reutilizan esas muestras
  (`simulation.rs:857-859` antes). Con `dt = 1/240` y 4 subpasos, la geometría puede quedar 4 ms
  atrás.
* Mitigado en E2 con un predictor de altura de anclaje por subpaso y medido con la prueba de
  convergencia en `dt` (240/500/1000 Hz). El remuestreo completo por subpaso queda como trabajo
  pendiente declarado.

### 6.10 `ray_distance` publicado como «compresión» por rayo — **P2**

* `ray_compressions_m` se derivaba de `max_ray_length − distance` y se publicaba como
  `OfpWheelState.ray_distance`. Con el nuevo contrato se publica la **demanda por rayo**
  (`compression_at_sample − gap_i`), documentada en `suspension.rs::ray_demand`.

---

## 7. Diferencias frente al contrato Godot/Rust (`ffi.rs`)

| Aspecto | Godot/Rust (`f1_94_physics_*`) | Speed Dreams/Rust |
| --- | --- | --- |
| Quién integra | Godot (`RigidBody3D`) | Rust (`VehicleSimulator`) |
| Quién muestrea el terreno | Host (Godot) envía `TriRaycastSample` | Rust consulta `CollisionWorld` |
| Origen de los rayos | El que el host decida; el contrato documentado es el anclaje de chasis | Centro físico de rueda |
| Interpretación de distancias | `spring + radius − distance` (heredada) | `compression_at_sample − (d·cosθ − r)` |
| Autoridad de pose | Host | Rust |

El adaptador antiguo declara explícitamente su contrato con
`WheelRayContext::from_chassis_anchor()` (en `FfiTriRaycastSample::into` y en el mundo acoplado), de
modo que la ruta heredada conserva su significado y no puede confundirse con la nueva por accidente.

---

## 8. Conclusión de la Puerta A

1. La fuente de cada posición está identificada: pista → documento → `CollisionWorld` → rayos;
   chasis → `VehicleSimulator` → snapshot → `CarElt`.
2. La autoridad de cada reacción es única: **Rust**, con detección de barreras delegada al host y
   respuesta siempre en Rust.
3. La geometría física por circuito **no era única ni estable** (OnceLock + archivo derivado de la
   malla visual). La entrega E1 la convierte en un objeto de sesión con identidad verificable y
   ciclo de vida explícito.
4. El origen de los rayos y la conversión a compresión no cumplían el contrato exigido. E2 los
   sustituye y mide.
5. Quedan tres limitaciones declaradas: barreras sin volumen de chasis, ausencia de exportador
   desde `tTrack`, y resolución secuencial (no acoplada) de barrera → fondo → ruedas.
