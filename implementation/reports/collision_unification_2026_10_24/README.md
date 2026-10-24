# Entrega — Unificación de colisiones y contacto de ruedas (2026-10-24)

Índice de artefactos de esta intervención. Todo el material es reproducible con los comandos
registrados en cada informe.

## Informes

| Fichero | Contenido |
| --- | --- |
| `implementation/reports/collision_architecture_audit.md` | **Fase A / E0.** Recorrido real de la geometría, matriz de autoridades, subsistemas activos, convenios, inventario de colisiones, defectos demostrados con ruta de código y reproducción. |
| `implementation/reports/collision_world_unification.md` | **E1 / Puerta B.** Mundo de colisión por sesión, identidad y hash, ciclo de vida, materiales por triángulo, contrato COM/origen, ABI. |
| `implementation/reports/wheel_center_contact_migration.md` | **E2 / Puerta C.** Raycasts desde el centro físico, nueva conversión hueco→compresión, complementariedad, predictor por subpaso, resultados de los seis criterios. |
| `implementation/reports/contact_integration_validation.md` | **E3/E4.** Comandos exactos, PASS/FAIL, A/B, hashes, riesgos residuales y estado de aceptación separado. |
| `implementation/reports/collision_refactor_comparison.csv` | Comparación cuantitativa: puertas + 200 valores de la serie A/B con tolerancia y estado. |

## Evidencia ejecutable

| Fichero | Qué demuestra |
| --- | --- |
| `_tools/frame_transform_test.cpp` / `.exe` | Involución SD↔Rust, Euler→base→Euler, ortonormalidad, mano derecha, COM↔origen. 4 grupos, 0 fallos. |
| `_tools/adapter_geometry_check.cpp` / `.exe` | Las expresiones exactas de `publishCar`/`setCar`/`createCar` con tipos simulados: ida y vuelta exacta, desplazamiento de spawn corregido. 3 grupos, 0 fallos. |
| `_tools/ab_metrics_baseline.rs` | Copia del runner A/B compilada en el worktree de HEAD (`D:\ofp-baseline`). |
| `ab_baseline.csv`, `ab_candidate.csv` | Series temporales de la misma escena con la física anterior y la nueva. |
| `baseline_lib_tests.txt` | Salida íntegra de `cargo test --lib` en HEAD con la reparación de arnés (149/27). |

## Reversión

Copia byte a byte de los ficheros fuente originales y de la DLL instalada en `_baseline/`:

| Fichero | Restaura en |
| --- | --- |
| `_baseline/open_formula.rs.bak` | `rust/vehicle-physics-engine/src/open_formula.rs` |
| `_baseline/open_formula_ffi.rs.bak` | `rust/vehicle-physics-engine/src/open_formula_ffi.rs` |
| `_baseline/suspension.rs.bak` | `rust/vehicle-physics-engine/src/suspension.rs` |
| `_baseline/types.rs.bak` | `rust/vehicle-physics-engine/src/types.rs` |
| `_baseline/rustphysics.cpp.bak` | `src/modules/simu/rustphysics/rustphysics.cpp` |
| `_baseline/vehicle_physics_engine.dll.bak` | DLL de HEAD, SHA-256 `4AEDAB23…AFAFFB` |

Reversión completa (árbol de trabajo, sin commits ni push):

```
git checkout -- rust/vehicle-physics-engine/src/rustphysics.cpp      # no aplica: ver nota
copy implementation\reports\collision_unification_2026_10_24\_baseline\*.bak <destino>
git checkout -- rust/vehicle-physics-engine/src            # restaura todo el crate salvo ficheros nuevos
```

Los ficheros **nuevos** (`collision_world.rs`, `wheel_centre_contact.rs`, `frame_transform.h`,
`ab_metrics.rs`, los informes) se pueden borrar sin afectar al resto. El worktree de referencia
`D:\ofp-baseline` es temporal y ya no es necesario para operar el proyecto.

## Instalación del candidato (no realizada)

No se instaló nada. La instalación coherente exige **dos** artefactos:

1. `rust/target/release/vehicle_physics_engine.dll`
   (`6D38B079295050FB8048BDD187199BAF37789632E9336A1BE684DAF7A15EB4A7`)
   → `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll`
2. `rustphysics.dll` recompilado con los cambios de `src/modules/simu/rustphysics/`
   (`frame_transform.h`, `rustphysics.cpp`, `rust_library.{h,cpp}`)
   → `distribution/lib/games/speed-dreams-2/modules/simu/rustphysics.dll`

El segundo no se pudo construir aquí (faltan plib/SDL/OSG). Instalar sólo el primero deja el
contrato nuevo de ruedas gobernado por un adaptador antiguo: funciona, pero no es el candidato que
se validó, por eso no se instaló.

Procedimiento cuando el host esté cerrado y exista el módulo recompilado:

```
copy /y rust\target\release\vehicle_physics_engine.dll ^
        distribution\lib\games\speed-dreams-2\modules\simu\vehicle_physics_engine.dll
copy /y <build>\rustphysics.dll ^
        distribution\lib\games\speed-dreams-2\modules\simu\rustphysics.dll
```

Verificación mínima tras instalar: en el log de la sesión deben aparecer
`RUSTWORLD reason=initialize ... identity={"circuit_id":"...","geometry_sha256":"..."}` y
`Rust physics: ground world ... circuit=... revision=...`; con `diagnostic telemetry = 1`,
`RUSTCONTACT` incluye `world_generation` y las doce geometrías de rueda.
