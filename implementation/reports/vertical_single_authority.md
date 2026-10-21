# Autoridad vertical única: fase 0

Fecha: 21 de octubre de 2026.
Base: rama `codex/initial-port`, HEAD `bfb708b4500efe15dba347da20af40f46a5e0fe0`.
Alcance: inventario, telemetría y reproducción del defecto. No se ha cambiado la física, no se ha instalado una DLL y no se ha conducido.

## 1. Resultado

La fase 0 queda cerrada con evidencia reproducible:

- inventario de todas las escrituras y lecturas de la autoridad vertical;
- telemetría con conmutador, desactivada por defecto;
- banco de integración `tools/validate_formula_vertical_authority.cpp` con **32 comprobaciones y 0 fallos**;
- reproducción del defecto de doble autoridad antes de cualquier corrección;
- reproducción de la fuga de `rideHeight` hacia la lectura aerodinámica;
- reproducción del clamp angular como escritura solo de vista.

El comportamiento físico no cambia con los diagnósticos desactivados. Las regresiones afectadas siguen pasando (§6).

## 2. Inventario de escrituras

Clasificación: **I** integración normal, **C** colisión, **L** límite/clamp, **T** transición explícita.

| # | Escritura | Archivo:línea | Qué modifica | Clase |
| --- | --- | --- | --- | --- |
| 1 | `SimCarVerticalPublish` | `car.cpp:1019-1119` | `DynGCg/DynGC` pos.z, ax, ay y velocidades; `wheel.pos.z`, `relPos.z`, `rideHeight`, `susp.x/v/a`, `susp.state`, `wheel.state`, fuerzas; `formula_vertical_state` | I |
| 2 | `SimCarUpdateSpeed` | `car.cpp:853-875` | integra `vel.x/y/az`; copia `vel.ax/ay/z` a `DynGC` | I |
| 3 | `SimCarUpdatePos` | `car.cpp:1546-1613` | integra x/y/az; copia pos a `DynGC`; **limita ax/ay a ±1 rad y anula sus velocidades** | I + L |
| 4 | `SimCarUpdate` | `car.cpp:1796-1813` | llama a `SimCarUpdatePos` y después a `SimCarCollideZ` | I + C |
| 5 | `SimCarCollideZ` | `collide.cpp:29-140` | `wheel.rideHeight = packers`; corrige `DynGCg.vel.x/y/z`; suma `dz` a `DynGCg.pos.z` | C |
| 6 | `SimCarConfig` | `car.cpp:301, 328` | `formula_vertical_solver_initialized = 0`; `memset` del estado | T |
| 7 | `SimReConfig` (cambio de reglaje) | `simu.cpp:263-298` | copia `DynGCg/DynGC` desde `carElt->_DynGC`; reinicia flag vertical; publica `pub.DynGCg` y `posMat` con roll/pitch 0 | T |
| 8 | `RemoveCar` (pull-up/side/down) | `simu.cpp:450-453` | `_pos_Z`, `_roll`, `_pitch` directamente sobre `pub.DynGC` | T |
| 9 | `SimWheelReConfig` | `wheel.cpp:234` | `relPos.z = radio - x0` | T |
| 10 | `SimAxleUpdate` | `axle.cpp:115-131` | `arbSusp.x`, `heaveSusp.x/v` | I (entrada del solver) |
| 11 | `SimCarUpdateWheelPos` | `car.cpp:875-910` | `wheel.pos.x/y/z`, `wheel.bodyVel` desde `DynGC/DynGCg` | I (preparación) |

`SimCarUpdateWheelPos` se ejecuta antes de `SimAeroUpdate` y de la actualización vertical (`simu.cpp:685`); escribe una vista de rueda con la pose del paso anterior que después se sobrescribe al publicar. `SimAxleUpdate` no se invoca desde la ruta acoplada actual; el recorrido de suspensión lo produce el solver. `susp.cpp:105-107` conserva un tope legacy sobre `susp.x` que tampoco forma parte de la ruta acoplada.

## 3. Inventario de lecturas

| Lector | Archivo:línea | Qué lee | Observación |
| --- | --- | --- | --- |
| Aerodinámica | `aero.cpp:245, 297-305` | `wheel.rideHeight` para sustentación, altura, rake y balanceo del fondo | Se ejecuta **antes** de la publicación vertical del paso siguiente |
| Vista pública | `simu.cpp:739-742, 872-875` | `DynGC`, `DynGCg`; `_roll`, `_pitch`, `_pos_Z` son macros de `pub.DynGC` | Depende de lo que dejó la colisión |
| Momentos laterales | `car.cpp:714-717` | `statGC.z + rideHeight` | Se ejecuta después de la publicación, no ve la fuga de la colisión |
| Velocidad de contacto | `car.cpp:1136-1138` | `DynGC.vel.x/y`, estado vertical | Usa estado y vistas horizontales |
| Activación de colisión | `collide.cpp:51` | `SIM_SUSP_COMP`, `SIM_WH_INAIR` | Flags publicados por el solver |
| Diagnóstico | `car.cpp:1869+` | estado y vistas | Solo lectura |

## 4. Reproducción

Banco: `tools/validate_formula_vertical_authority.cpp`.
Escenario controlado: fondo plano, `body_position[0] = 0.010 m`, extensión igual al packer en las cuatro ruedas, velocidad vertical `-3 m/s`. Los amortiguadores y topes quedan en su configuración del XML del Williams. Los 10 mm se eligen para que el packer delantero (25 mm) y el trasero (15 mm) superen la holgura publicada en las cuatro ruedas; es un caso de compresión máxima/penetración controlada, no una altura de marcha normal. El fixture fija contacto y carga normal en los diagnósticos de equilibrio para que la rama `SIM_SUSP_COMP` sea alcanzable.

Las tolerancias de las vistas son `1e-6` porque `DynGCg`/`DynGC` usan `tdble` (float); las magnitudes del estado autoritativo se comparan con tolerancia `1e-12`.

El banco llama a las funciones de producción en el orden relevante:

```
SimCarVerticalPublish -> SimCarUpdatePos -> SimCarCollideZ
```

También vuelve a publicar el mismo estado para comprobar qué ocurre en el paso siguiente.

### 4.1 Estado publicado

```
body=0.010000  vel=-3.000000
dyn_gc_z=0.254999995  dyn_gc_vz=-3.000000
ride=0.010000,0.010000,0.010000,0.010000
packers=0.025000,0.025000,0.015000,0.015000
flags=1,1,1,1   (SIM_SUSP_COMP)
```

### 4.2 Tras `SimCarCollideZ`

```
body=0.010000  vel=-3.000000      (formula_vertical_state sin cambios)
dyn_gc_z=0.254999995              (DynGC congelado antes de la colisión)
dyn_gc_g_z=0.269999981            (+0.0149999857 = 0.025 - 0.010)
dyn_gc_vz=-3.000000               (DynGC congelado)
dyn_gc_g_vz=0.000000              (corrección de velocidad)
ride=0.025000,0.025000,0.015000,0.015000   (= packers)
```

Comprobaciones que pasan: el estado autoritativo no cambia, `DynGCg.pos.z` sube por el solape del packer, `DynGC.pos.z` y `DynGC.vel.z` quedan obsoletos, las cuatro alturas de fondo se sobrescriben con el packer y `DynGC`/`DynGCg` quedan verticalmente inconsistentes.

### 4.3 Paso siguiente

Al volver a publicar el mismo estado:

```
dyn_gc_g_z=0.254999995   (la elevación de la colisión se descarta)
dyn_gc_g_vz=-3.000000    (la corrección de velocidad se descarta)
ride=0.010000,...        (la altura de fondo se restaura)
```

Esto confirma que la colisión no es un segundo integrador persistente: es un parche de un solo paso que se revierte en la siguiente publicación. El efecto medible es una discontinuidad de vista y de entrada aerodinámica, no un desplazamiento acumulado.

### 4.4 Clamp angular

Con `body_position[1] = 1.2`, `body_velocity[1] = 0.5`, `body_position[2] = -1.2`, `body_velocity[2] = -0.5`:

- `SimCarVerticalPublish` expone `1.2 / -1.2` y `0.5 / -0.5`;
- `SimCarUpdatePos` deja la vista en `1.0 / -1.0` y anula las velocidades;
- el estado autoritativo conserva `1.2 / -1.2` y `0.5 / -0.5`;
- la siguiente publicación vuelve a exponer `1.2 / -1.2` y `0.5 / -0.5`.

El clamp es, por tanto, una escritura exclusiva de vista que dura hasta la publicación siguiente.

## 5. Fuga de `rideHeight` hacia aerodinámica

`SimCarCollideZ` ejecuta `wheel->rideHeight = wheel->susp.spring.packers` dentro de la rama `SIM_SUSP_COMP`, incluso cuando `dz == 0`. En el escenario reproducido el valor publicado era `0.010 m` y la colisión lo dejó en `0.025 / 0.015 m`.

Orden real del fotograma:

1. `SimAeroUpdate` (`simu.cpp:691`) lee `rideHeight`.
2. `SimCarVerticalCoupledUpdate` (`simu.cpp:700`) publica `rideHeight`.
3. `SimCarUpdate` (`simu.cpp:709`) ejecuta la colisión y sobrescribe `rideHeight`.

Por tanto, la aerodinámica del paso siguiente consume el valor del packer durante un fotograma. El banco demuestra la sobrescritura; la traza de producción (`vertical authority diagnostic = 1`) permite medir la lectura exacta en `aero.cpp`.

## 6. Comandos y resultados

Build observado: `C:/Users/bill/open-formula-build/engine`, CMake 4.4.4, Ninja, GCC 16.2.0, Release, `CMAKE_DISABLE_FIND_PACKAGE_SOLID=TRUE`.

```
cmake -S D:/open-formula -B C:/Users/bill/open-formula-build/engine -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_SOLID=TRUE
ninja simuv5
ninja validate_formula_vertical_authority
```

Ejecución del banco (con las DLL del build y del toolchain en `PATH`):

```
C:/Users/bill/open-formula-build/engine/validate_formula_vertical_authority.exe \
  implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml
```

Artefactos compilados en esta fase (no instalados):

| Artefacto | SHA-256 |
| --- | --- |
| `validate_formula_vertical_authority.exe` | `D46BE539C7A5B8B94A7D69DEE5B93DFCE5BEABBFB3AFB46E90916E3BE6E44B08` |
| `simuv5.dll` (build, no distribución) | `2AC126A94EF3E691A90DBE851A31FBB644A9EC79B43099A3ACB0E230D2CA1C2A` |

Los conmutadores de diagnóstico se añadieron también al XML local del candidato (`Formula Controls`, líneas 507-508), que está ignorado por Git (`.gitignore:6`). El código funciona sin esas claves porque su valor por defecto es 0; para una traza hay que activarlas en el XML que use la distribución.

Evidencia original: `D:/open-formula-migration-evidence/vertical-authority-phase0-20261021` (`phase0-vertical-authority.log`, `hashes.txt`).

Regresiones ejecutadas (código de salida):

| Banco | Salida |
| --- | --- |
| `validate_formula_tire_vertical_system` | 0 (`checks=208 failures=0`) |
| `validate_formula_tire_geometry` | 0 (`checks=144 failures=0`) |
| `validate_formula_tire_contact` | 0 |
| `validate_formula_tire_forces` | 0 |
| `validate_formula_tire_energy` | 0 |
| `validate_formula_tire_thermal` | 0 |
| `validate_formula_tire_selector` | 0 |
| `validate_formula_aerodynamics` (con XML) | 0 |
| `validate_williams_chassis` (con XML) | 0 |
| `validate_formula_vertical_authority` (nuevo) | 0 (`checks=32 failures=0`) |

No se reutiliza el total histórico de 504 como evidencia de esta versión.

## 7. Telemetría añadida

Conmutadores en `Formula Controls`, ambos con valor 0 por defecto:

- `vertical authority diagnostic`: registra activaciones de la colisión, `rideHeight` antes/después, `dz`, `dotProd`, clamp angular y resumen por paso cuando algo cambia.
- `vertical authority every step`: añade publicación y lectura aerodinámica en cada paso. Es verboso; usar solo en ventanas cortas.

Líneas emitidas:

- `Formula vertical authority publish`
- `Formula vertical authority step`
- `Formula vertical authority collide wheel`
- `Formula vertical authority collide summary`
- `Formula vertical authority clamp`
- `Formula vertical authority aero read`

La telemetría no modifica ninguna magnitud física.

## 8. Límites de la evidencia

- El banco reproduce la interacción publicación → posición → colisión con funciones reales, pero no ejecuta `SimCarUpdateForces` ni `SimCarVerticalCoupledUpdate`. El solver acoplado sigue cubierto por el banco de 208 comprobaciones.
- El escenario es sintético y de laboratorio. No demuestra cuántas veces se activa la ruta en Fuji ni cuánto contribuye a los rebotes observados.
- La lectura aerodinámica se ha verificado por orden de llamada y por la sobrescritura de `rideHeight`; falta la traza instrumentada en conducción real.
- No se ha instalado ninguna DLL ni se ha conducido. La puerta D sigue pendiente.

## 9. Siguiente fase

La fase 1 debe definir la geometría del fondo desde los datos del vehículo y contrastarla con el modelo visual, con puntos explícitos y Jacobianos. Antes de tocar el solver conviene decidir la fuente de esos puntos y el tratamiento de la aproximación si los datos no bastan.
