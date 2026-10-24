# E3/E4 — Integración del contacto, regresiones y comparación controlada

**Fecha:** 24 de octubre de 2026
**Binario Rust:** `rust/target/release/vehicle_physics_engine.dll`
SHA-256 `1C0DE8C7CDEBD2825A7F41BAC56F113A866BC63BD0F3C98B5350ABA7FA2B219F`
**Binario de referencia (HEAD, sin cambios):** `4AEDAB2346809729F32ED63373256D2D22457E75D4BD3C11B52C951BC5AFAFFB`
**Entorno:** Windows, Rust 1.95.0 (`cargo 1.95.0`), MSVC 19.44.35227 (x64).
**Sin toolchain del juego:** no hay plib/SDL/OSG en esta máquina, así que `rustphysics.dll`
(módulo C++) **no se ha podido compilar** (ver §7).

---

## 1. Qué se validó y con qué

| Nivel | Qué cubre | Cómo se ejecuta |
| --- | --- | --- |
| Unidad Rust | hash/identidad del mundo, registro de sesión, geometría de consulta | `cargo test -p vehicle_physics_engine --lib collision_world` |
| Integración Rust (heredada) | solver vertical, cargas, energía, contacto estructural | `cargo test -p vehicle_physics_engine --test structural_contact` (y 4 más) |
| Integración Rust (nueva, Puerta C) | centro de rueda, huecos, peralte, droop, simetría, convergencia, ciclo de vida | `cargo test -p vehicle_physics_engine --test wheel_centre_contact` |
| C++ puro (MSVC) | transformaciones de marco, actitud y COM↔origen | `frame_transform_test.exe`, `adapter_geometry_check.exe` |
| A/B de binario | misma escena con física anterior y nueva | `cargo run --release --example ab_metrics` en dos checkouts |

## 2. Comandos exactos y resultados

```
# 1. Biblioteca y pruebas nuevas de la Puerta C
cargo test -p vehicle_physics_engine --test wheel_centre_contact
    → ok. 14 passed; 0 failed

# 2. Suites heredadas (las que sí compilaban en HEAD)
cargo test -p vehicle_physics_engine --test structural_contact   → ok. 16 passed; 0 failed
cargo test -p vehicle_physics_engine --test body_coupling        → ok.  6 passed; 0 failed
cargo test -p vehicle_physics_engine --test setup_authority      → ok. 15 passed; 0 failed
cargo test -p vehicle_physics_engine --test suspension_35_40     → ok.  5 passed; 0 failed
cargo test -p vehicle_physics_engine --test vertical_telemetry   → ok.  9 passed; 0 failed

# 3. Unidad (el objetivo `--lib` NO compilaba en HEAD; ver §3)
cargo test -p vehicle_physics_engine --lib
    → 155 passed; 27 failed   (el mismo conjunto de 27 fallos que HEAD)

# 4. Transformaciones C++ (MSVC, sin cabeceras del juego)
cl /nologo /std:c++17 /W4 /EHsc /I src\modules\simu\rustphysics ^
   implementation\reports\collision_unification_2026_10_24\_tools\frame_transform_test.cpp
frame_transform_test.exe      → PASS frame_transform: 4 groups, 0 failures
cl ... adapter_geometry_check.cpp
adapter_geometry_check.exe    → PASS adapter_geometry: 3 groups, 0 failures

# 5. A/B de binario
cargo run --release -p vehicle_physics_engine --example ab_metrics   (checkout con cambios)
cargo run --release -p vehicle_physics_engine --example ab_metrics   (worktree HEAD en D:\ofp-baseline)
```

Hashes de los arneses:
`frame_transform_test.exe` `A639626AE205037116E667D1168FF84257FC38875D8D1A2F52113EC6F3F9FD85`,
`adapter_geometry_check.exe` `9E9F40D2C04A39E543BF729F162F24D4F9D82B4CDE3256D8A6E831CDDE0AE16E`.

## 3. Hallazgo previo: la suite unitaria estaba muerta en HEAD

El objetivo `cargo test --lib` **no compilaba** en HEAD `b045d4d`:

```
error: couldn't read .../data/vehicles/f1_2030/f1_2030_v10_geometric.json
error: couldn't read .../data/race_intelligence/automatic_transmission_tuning.json
error[E0599]: no function named `f1_2030_geometric_canonical`
```

Son restos del port: módulos `#[cfg(test)]` que referencian fixtures F1 2030 que el repositorio ya no
incluye. Ningún informe histórico podía respaldarse en esas pruebas porque no se ejecutaban.

**Reparación mínima aplicada** (sin cambio de física): los cuatro módulos/pruebas afectados pasan a
`#[cfg(all(test, feature = "retired-f1-2030-fixtures"))]` con la feature declarada y documentada en
`Cargo.toml`. Habilitarla sin los documentos es un error de compilación deliberado, nunca un salto
silencioso. Es el único cambio de esta entrega fuera del alcance de colisiones y está aislado.

**Medición base frente a medición con los cambios** (mismo árbol `D:\ofp-baseline` en HEAD, con la
misma reparación de arnés):

| | HEAD | Con los cambios |
| --- | --- | --- |
| `--lib` aprobadas | 149 | 155 (+6 nuevas de `collision_world`) |
| `--lib` fallidas | 27 | 27 |
| Conjunto de fallos | — | **idéntico** (comparado nombre a nombre) |
| Integración heredada | 51 aprobadas | 51 aprobadas |

Los 27 fallos son previos y ajenos: fixtures ausentes (`game/data/vehicles/f1_1997/...`,
`data/vehicles/f1_2030/...`, `game/data/vehicles/f1_2026_2008/...`), rutas con un `../` de más, y
aserciones de versión de ABI que esperan 6 cuando el HEAD ya publica 7. No se tocaron.

## 4. A/B: misma escena, física anterior y nueva

Escena determinista (`examples/ab_metrics.rs`), 240 Hz, suelo plano:

| Tramo | Entrada |
| --- | --- |
| 0–1 s | freno 1, embrague 1, punto muerto (asentamiento) |
| 1–3 s | acelerador 0,55, embrague 0, marchas 2→3 |
| 3–5 s | freno 0,55, dirección −0,30, marcha 3 (transferencia longitudinal + lateral) |

Resultado (ficheros completos en `ab_baseline.csv`, `ab_candidate.csv` y
`collision_refactor_comparison.csv`):

* 25 muestras, 8 magnitudes por muestra → **200 valores comparados, idénticos bit a bit**.
* La única diferencia entre ambos ficheros es la línea de comentario con la identidad del mundo
  (`hash=baseline` frente a `hash=0cd4647bf688aa73`).
* Valores representativos: altura del COM 0,539680 m en ambos en t = 1,004 s; carga total 6006,113 N
  en ambos; en t = 3,254 s (frenada + giro a izquierda) FL 1449,138 N / FR 1646,586 N /
  RL 1366,089 N / RR 1542,939 N en ambos; guiñada −0,228778 rad/s en ambos.

Interpretación: **en terreno plano el nuevo contrato reproduce exactamente el anterior**, que es lo
que la derivación algebraica predice (§2.5 del informe de migración). El cambio no altera el
comportamiento asentado; cambia lo que no podía representar (origen móvil, peralte real,
penetración, mundo obsoleto).

## 5. Contacto: una sola aplicación, instrumentada

* `OfpSnapshot.body_contact_impulse_ns[32]` publica el impulso de cada contacto de host y
  `floor_impulse_ns[2]` el de cada sonda de fondo. `ContactReport.barrier_impulse_ns` suma los de
  barrera del paso aceptado.
* `body_contact_count` y `floor_valid_mask` distinguen «contacto detectado» de «impulso aplicado».
* Presupuesto de consultas por paso aceptado: **12 de rueda + 2 de fondo**, constante. Publicado en
  `wheel_ray_queries/hits` y `floor_ray_queries/hits`; con 240 Hz son 3360 consultas de rueda por
  segundo y vehículo (medido, no estimado).
* Estructura sin doble aplicación (auditoría §6.7 del informe de arquitectura): barrera → fondo →
  ruedas, cada una con su bucle, y el rechazo restaura el clon completo.
* Reposición de la energía: el informe de contacto publica `kinetic_energy_before_j` /
  `kinetic_energy_after_j` por paso; la prueba de caída comprueba que la energía cinética máxima no
  supera la energía potencial de la caída (+5 %).

## 6. Pruebas de la Puerta D (alcance real)

| Requisito | Estado | Evidencia |
| --- | --- | --- |
| Colisionadores estáticos sin integrador propio | **cumplido** | `CollisionWorld` inmutable, sin masa ni estado; sólo `raycast` |
| `apply_body_impulse` conserva signos, masas efectivas y pares | auditado, sin cambios de signo | masa efectiva por diferencia de aceleraciones (`body_accelerations(0)` vs `(unit)`), impulso sólo si `v_n < 0` |
| Resolución secuencial barrera → fondo → ruedas; ¿hace falta acoplada? | **medido, no reescrito** | no hay doble aplicación; el acoplamiento simultáneo queda como riesgo declarado |
| *Ground effect* ≠ impacto sólido | **cumplido** | el fondo aerodinámico vive en `aero.rs` (fuerzas); el contacto de fondo es una restricción unilateral aparte |
| No reparar penetración con `penetración/dt` | **cumplido en ruedas y barreras**; el fondo no corrige posición (declarado) | la demanda es una compresión objetivo; el impulso sólo cancela velocidad de cierre |
| Rechazo: estado aceptado completo + diagnóstico | **cumplido** | `*self = accepted_before_step`, `FailureDiagnostics` con `demand_m`, `limit_m`, `excess_m`, rueda, eje, iteración |
| Sin NaN/Inf al host | **cumplido** | validación de estado finito antes de aceptar; la prueba de hueco/rechazo comprueba finitud del snapshot |
| Impulsos laterales = carrocería | **no cumplido, declarado** | `collectBarrierContacts` usa puntos de rueda; no hay volumen de chasis |
| Sin Rapier3D | **cumplido** | sólo Parry3D como proveedor de consultas |

Escenarios sintéticos ejecutados: suelo plano (4 ruedas), frenada recta, frenada + giro, una rueda sin
apoyo, peralte 10°, caída de 3 cm, cambio de circuito en el mismo proceso, liberación del mundo,
240/500/1000 Hz. **Melbourne T3 no se ha podido ejecutar** (requiere el juego con el adaptador
recompilado); no se declara resuelto.

## 7. Compatibilidad y ciclo de vida

| Aspecto | Estado |
| --- | --- |
| ABI de estructuras | **sin cambios** (v7, 3480 bytes); sólo funciones nuevas |
| Carga de la biblioteca antigua por el host nuevo | funciona: los símbolos nuevos se resuelven opcionalmente |
| Carga de la biblioteca nueva por el host antiguo | funciona: el host llama a `ofp_load_track_ground_mesh` (una pista por sesión) y `ofp_create` exige mundo, que ese host ya instala |
| Varias sesiones en un proceso | soportado: instalar sustituye y avanza generación; el vehículo antiguo falla explícitamente |
| Varios vehículos | soportado: comparten el `Arc<CollisionWorld>` |
| Reversión | copia de seguridad de los cinco ficheros fuente y de la DLL original en `implementation/reports/collision_unification_2026_10_24/_baseline/` |

## 8. Estado de aceptación (estados separados)

| Estado | Resultado |
| --- | --- |
| **Implementación** | E1, E2 y la parte de E3 dentro del alcance están implementadas. E4 (Melbourne T3 y comparativa de conducción) **no** ejecutada. |
| **Validación técnica** | Rust: 155 unitarias (27 fallos previos idénticos a HEAD) + 51 de integración heredadas + 14 de Puerta C, todas en verde. C++ puro: 2 arneses MSVC en verde. **El módulo C++ del juego no se ha compilado en este entorno.** |
| **Instalación** | **Ninguna**. `distribution/lib/.../vehicle_physics_engine.dll` sigue siendo el binario de HEAD (`4AEDAB…`). Candidato disponible en `rust/target/release/`. |
| **Prueba humana de conducción** | **Pendiente**; corresponde al usuario tras instalar el candidato y el módulo recompilado. |

## 9. Riesgos residuales

1. **Adaptador C++ sin compilar.** Los cambios en `rustphysics.cpp`, `rust_library.{h,cpp}` y
   `frame_transform.h` están verificados por los dos arneses puros y por revisión, pero **no por un
   compilador con las cabeceras del juego**. Riesgo: error de compilación en el host. Mitigación:
   los cambios son locales, el encabezado nuevo no depende de nada del juego y las expresiones
   exactas están replicadas en `adapter_geometry_check.cpp`.
2. **Geometría de pista derivada de la malla visual.** No es la representación física de `tTrack`.
   El exportador desde segmentos queda pendiente.
3. **Barreras sin volumen de chasis.**
4. **Resolución secuencial** de barrera → fondo → ruedas: con contacto simultáneo, el fondo usa la
   velocidad previa a la carga de rueda.
5. **Remuestreo por subpaso** no implementado (predictor de altura sí).
6. **Los 27 fallos unitarios previos** siguen ahí; no se ocultan ni se han convertido en verdes.
7. **Problema de barra–muelle–balancín en Melbourne**: este trabajo no lo aborda. La migración
   geométrica no lo resuelve por sí sola.
8. **`setCar` sin llamador activo**: la corrección está probada en el arnés, no en carrera de red.

## 10. Correcciones de seguimiento — 24 de octubre de 2026

Se cerraron tres defectos observados en la revisión posterior:

* El coeficiente de fricción y la resistencia a la rodadura de cada triángulo llegan ahora al
  `RaycastHit` y se combinan por los rayos que apoyan. Se añadieron comprobaciones de material finito
  y no negativo. No cambió el ABI C v7.
* Los rayos transversales usan el eje de rueda calculado con camber, toe y dirección. El contexto y
  el reporte de contacto conservan ese eje para validar las posiciones de las tres consultas.
* La prueba de peralte compara una distancia perpendicular a la superficie: proyecta la separación
  sobre la normal unitaria y usa una tolerancia de 1 mm (antes tomaba diferencia vertical y admitía
  6 mm).

Validación posterior a estos cambios:

```
cargo test -p vehicle_physics_engine --test wheel_centre_contact
    → ok. 16 passed; 0 failed

cargo test -p vehicle_physics_engine --test structural_contact --test body_coupling --test setup_authority --test suspension_35_40 --test vertical_telemetry
    → ok. 51 passed; 0 failed

cargo test -p vehicle_physics_engine --test wheel_centre_contact --lib physical_material_coefficients_follow_the_supported_tricast_weights
    → ok. 1 passed; 0 failed
```

La suite unitaria completa posterior a estas correcciones: `cargo test -p vehicle_physics_engine
--lib` → **156 aprobadas y 27 fallidas** entre 183 pruebas. Los 27 nombres son el conjunto previo
documentado en §3; la prueba nueva de ponderación de materiales pasa.

No se pudo validar aquí la compilación del adaptador completo ni la conducción. La prueba de 16 casos
usa el mundo Rust y escenas sintéticas. El remuestreo dentro de subpasos, el exportador desde segmentos
físicos `tTrack`, la geometría de chasis contra barreras y Melbourne T3 siguen pendientes; E4 no se
acepta con estas pruebas.
