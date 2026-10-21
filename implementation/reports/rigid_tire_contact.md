# Neumático rígido y contacto unilateral en simuv5

Actualización posterior: esta candidata presentó lanzamiento y NaN en conducción. Sus resultados de arranque no acreditan estabilidad. La auditoría está en rigid_tire_flight_audit.md y la reparación instalada se documenta en rigid_tire_flight_fix.md. Los hashes y límites que siguen describen la versión anterior.

Fecha: 21 de octubre de 2026.
Base: rama `codex/initial-port`, HEAD `bfb708b4500efe15dba347da20af40f46a5e0fe0`.
Alcance: eliminar la elasticidad y amortiguación vertical del neumático, conservar la suspensión y resolver el contacto rueda-suelo como restricción unilateral dentro del solver acoplado de siete grados de libertad.

## 1. Resultado

El cambio está implementado, compilado, validado en bancos e instalado en la distribución local:

- la fuerza radial `N = k·δ + c·δ̇` ha desaparecido del solver y de la configuración;
- el contacto de rueda se resuelve con una restricción geométrica `gap = H - L - G >= 0`;
- la carga normal publicada al grip y a la térmica es `N_step = ΣJ_s/dt`, acumulada de los subpasos aceptados;
- la configuración usa esquema 2 y rechaza XML antiguos o mezclados;
- la DLL y el XML instalados tienen hashes registrados y respaldo;
- el launcher alcanza la carrera, carga el esquema 2, inicializa el equilibrio y conduce sin fallo del paso vertical.

La aceptación humana de conducción sigue pendiente. No se declara cerrada la puerta D.

## 2. Formulación

Para cada rueda:

```
H_i = body_height - x_i·pitch + y_i·roll
gap_i = H_i - L_i - G_i
J_i = [1, y_i, -x_i, 0, ..., -1 en la extensión i]
```

- `gap_i >= 0`, multiplicador `λ_i >= 0`, restitución normal nula.
- El contacto se añade al active set cuando el hueco se cierra o se cruza; se retira cuando la reacción deja de ser admisible.
- La masa no suspendida, el muelle, el amortiguador, la barra antibalanceo y el tercer elemento siguen en el mismo sistema.
- La geometría del terreno usa altura y velocidad coherentes en el mismo subpaso.
- Los contactos del fondo y los topes de suspensión comparten el bloque de restricciones.

La base independiente de restricciones se selecciona con Gram-Schmidt modificado con pivoteo. Las reacciones publicadas se reconstruyen con una solución de mínima norma sujeta a signos; si la reconstrucción no es admisible se conserva la solución de la base para que el active set pueda revisar el conjunto. No se añade epsilon a la masa ni al KKT.

El solver exterior subdivide el intervalo de forma adaptativa hasta profundidad 6. Si no converge, conserva el último estado aceptado, marca `formula_vertical_failed` y deja de publicar estados parciales.

## 3. Configuración

Esquema 2 de `Formula Tires/* Contact`:

| Campo nuevo | Función |
| --- | --- |
| `empirical load compliance` (m/N) | estimación algebraica de deformación desde carga; no es un resorte |
| `empirical deformation limit` (m) | límite de la estimación empírica |
| `rolling radius load deflection factor` | radio efectivo de rodadura |
| `contact patch load gain` | sensibilidad de huella a carga |
| `contact patch empirical deformation gain` | sensibilidad de huella a la estimación empírica |

Se eliminan `radial stiffness`, `radial damping ratio`, `maximum deflection`, `partial contact stiffness floor` y los nombres antiguos de radio/huella. La herramienta `tools/migrate_formula_tire_configuration.py` migra de forma idempotente y `tools/prepare_williams_vehicle.py` genera directamente esquema 2. El lector rechaza esquema 1 y cualquier mezcla de campos antiguos y nuevos.

La deformación geométrica del contacto es cero. La estimación empírica solo alimenta huella, radio de rodadura y volumen térmico; nunca vuelve al centro de rueda ni al solver vertical.

## 4. Archivos

- `src/modules/simu/simuv5/tire_vertical_solver.cpp/.h`
- `src/modules/simu/simuv5/car.cpp`, `carstruct.h`, `simu.cpp`
- `src/modules/simu/simuv5/tire_model.cpp`
- `src/modules/simu/simuv5/tire_model_configuration.cpp/.h`
- `src/modules/simu/simuv5/tire_model_state.h`
- `src/modules/simu/simuv5/tire_contact_model.cpp/.h`
- `src/modules/simu/simuv5/tire_thermal_model.cpp/.h`
- `src/modules/simu/simuv5/tire_vertical_geometry.cpp/.h`
- `tools/prepare_williams_vehicle.py`
- `tools/migrate_formula_tire_configuration.py` (nuevo)
- `tools/validate_formula_tire_vertical_system.cpp`
- `tools/validate_formula_tire_contact.cpp`
- `tools/validate_formula_tire_thermal.cpp`
- `tools/validate_formula_tire_energy.cpp`
- `tools/validate_formula_tire_selector.cpp`
- `tools/validate_formula_vertical_authority.cpp`
- `implementation/migration_status.json`
- `distribution/distribution_manifest.json`

## 5. Comandos y resultados

Build: `C:/Users/bill/open-formula-build/engine`, Ninja, GCC 16.2.0, Release.

```
cmake --build C:/Users/bill/open-formula-build/engine --target simuv5
cmake --build C:/Users/bill/open-formula-build/engine
```

Bancos ejecutados con el XML candidato:

| Banco | Resultado |
| --- | --- |
| `validate_formula_tire_vertical_system` | 208 comprobaciones, 0 fallos |
| `validate_formula_vertical_authority` | 56 comprobaciones, 0 fallos |
| `validate_formula_tire_geometry` | salida 0 |
| `validate_formula_tire_contact` | salida 0 |
| `validate_formula_tire_forces` | salida 0 |
| `validate_formula_tire_energy` | salida 0 |
| `validate_formula_tire_thermal` | salida 0 |
| `validate_formula_tire_selector` | salida 0 |
| `validate_formula_aerodynamics` | salida 0 |
| `validate_williams_chassis` | salida 0 |

Los bancos cubren equilibrio de quarter-car y coche completo, perturbaciones de heave/roll/pitch, vuelo, impacto, topes, contacto del fondo, recuperación de contacto, comparación de frecuencias, tiempo cero como no-op, balance energético con trabajo de contacto firmado, migración de configuración y equilibrio estático con contacto rígido.

## 6. Artefactos e instalación

| Artefacto | SHA-256 |
| --- | --- |
| `simuv5.dll` instalada | `733D44685F3F3BB04359D4746722E9941A75B320813E0F254A14F016F8C2B15D` |
| XML instalado | `EDEEB8F3F3F448EDC1E33F35FB431C6AFE6344D3E9E75AB3C0B82BB82343F2AD` |
| Manifiesto de distribución | `9FB9FCDC8E5519EC8A867BD9E3104D25600DA05F690D53ADF0403AF268C29C5F` |
| DLL anterior | `9CBD34ADEFC01EF6B5D15BE68CC26B1A10466C0061E66CDFF5B70A0BBA66ECDD` |

Instalación:

- DLL: `distribution/lib/games/speed-dreams-2/modules/simu/simuv5.dll`
- XML: `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- Respaldo: `D:/open-formula-migration-evidence/rigid-tire-20261021`
- Ejecutable: `distribution/OpenFormula.exe`

Reversión: copiar `simuv5.before.dll` y `williams.before.xml` desde el directorio de respaldo a sus rutas originales.

## 7. Prueba de arranque

`OpenFormula.exe` arrancó, cargó el módulo nuevo y alcanzó la carrera. El log `distribution/user-settings/runtime.log` confirma:

- `Coupled tire model enabled: schema=2`;
- `SimuV5 tire model: transient coupled (configuration_valid=1 enabled=1)`;
- `Coupled vertical equilibrium: body=33.264184023 roll=0.000000000 pitch=0.000321734 L=0.051067269,... N=1337.612351,...`;
- cambios de marcha y conducción sin `Coupled vertical step failed`.

No se ha realizado una medición instrumentada de alta velocidad ni una aceptación humana en esta versión.

## 8. Límites y pendientes

- **Soporte del fondo**: el solver conserva una base de hasta tres filas independientes para el chasis. Los puntos adicionales habilitados se publican como inactivos y pueden presentar penetración geométrica en casos sintéticos severos; el LCP/polígono completo sigue pendiente. Es la misma limitación declarada en `vertical_floor_geometry.md`, ahora conviviendo con el contacto rígido de rueda.
- **Reacción de chasis**: una fila de chasis con multiplicador negativo se mantiene en el conjunto para no ciclar; no se publica como activa. La revisión completa de signos y el reparto físico entre contactos equivalentes requiere el LCP pendiente.
- **Térmica**: se retiró el término de histéresis por velocidad radial. El balance energético sigue siendo coherente con las fuentes restantes, pero no es equivalente al modelo elástico anterior; una recalibración de calor de rodadura es otro alcance.
- **Conducción**: pianos, pendientes, boxes, alta velocidad sostenida y aceptación humana quedan pendientes.
- **Daños del fondo**: el mapeo de impulsos aceptados a eventos/daños heredados sigue parcialmente pendiente.

## 9. Evidencia

- Bancos: salidas de los ejecutables en `C:/Users/bill/open-formula-build/engine`.
- Hashes y respaldos: `D:/open-formula-migration-evidence/rigid-tire-20261021`.
- Log de arranque: `D:/open-formula/distribution/user-settings/runtime.log`.
- Estado: `implementation/migration_status.json`, bloque `rigid_tire_contact`.
