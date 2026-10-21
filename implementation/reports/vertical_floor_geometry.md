# Geometría del fondo: auditoría y propuesta

Fecha: 21 de octubre de 2026.
Base: rama `codex/initial-port`, HEAD `bfb708b4500efe15dba347da20af40f46a5e0fe0`.
Alcance: auditar la geometría disponible y proponer puntos físicos de contacto. No se ha modificado el solver ni la física.

## 1. Resultado

La geometría del fondo se puede reconstruir con confianza a partir de la malla visual instalada y contrastarla con los datos del vehículo:

- la escala y los ejes coinciden con el XML dentro de 1 µm;
- el origen vertical queda verificado: el vértice más bajo está a 24.899 mm sobre el suelo en la pose nominal, exactamente el valor de la auditoría de chasis;
- el fondo tiene una tabla central a −25.1 mm, un tramo trasero a −21.0 mm, un suelo lateral a −3.0 mm, un borde trasero a 0.0 mm y un alerón delantero a −6.04 mm, todos relativos al plano de referencia;
- se proponen 12 puntos de fondo en tres niveles (8/10/12) y 2 puntos de alerón delantero como grupo separado;
- la propuesta pasa 100 comprobaciones geométricas, 0 fallos.

La propuesta está en `implementation/floor_contact_points.json`. Este documento explica procedencia, verificación, cobertura y límites.

## 2. Fuentes y procedencia

| Fuente | Ruta | Dato |
| --- | --- | --- |
| Malla visual instalada | `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.acc` | 48.216 vértices, 63.659 triángulos; SHA-256 `96E42B84D184F0338360FA6DE43C4F188D7953367AA876C5AD54260748D6B84C` |
| GLB de origen | `D:\Formula90s\...\williams_fw19_hockenheim_1997_chassis.glb` | SHA-256 `71cb5cf969f5f093551c6a49af3689111c7e0691e07a9e39581081d066fc6238` |
| Informe de generación | `implementation/candidates/williams-nineteen-ninety-seven/williams_generation_report.json` | transformación, offsets, recuentos |
| XML del vehículo | `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml` | dimensiones, GC, ejes, reglajes; SHA-256 `E6E4A89411790393BC0AD2C6C9F870F59DE1B236ACBBED5D0C1A47EB962A6C69` |
| Auditoría de chasis | `implementation/reports/williams_chassis_suspension_audit.md` | mínimo visual de 24.899 mm; referencia vertical |
| Herramienta | `tools/inspect_vehicle_floor.py` | lectura de objetos, cotas y envolvente inferior |

El GLB de origen no está en este repositorio; el `.acc` es la malla instalada y la que se audita. El directorio de candidatos está ignorado por Git, pero el informe de generación conserva la ruta y el hash del origen.

## 3. Escala, ejes y origen

### 3.1 Dimensiones

| Magnitud | XML | Malla ACC | Diferencia |
| --- | --- | --- | --- |
| Longitud de la malla completa | 4.444270 m | 4.444269 m (`x`: −2.304584…2.139685) | 0.000001 m |
| Anchura de la malla completa | 1.661375 m | 1.661374 m (`z`: −0.830687…0.830687) | 0.000001 m |
| Altura de la malla completa | 1.154363 m | 1.154363 m (`y`: −0.025101…1.129262) | 0.000000 m |

La malla completa incluye los brazos de suspensión, el alerón delantero y el trasero. La anchura de 1.661375 m es la envolvente del conjunto, no la del carenado: el cuerpo principal mide 1.402 m (`z` ±0.701) y el suelo lateral llega a ±0.64 m.

La coincidencia fija los ejes sin ambigüedad:

- `acc_x` = longitudinal, positivo hacia delante;
- `acc_y` = vertical, positivo hacia arriba;
- `acc_z` = lateral.

La orientación longitudinal se confirma con los nombres de los objetos: `GEO_BODYWORK_NOSE_CONE` ocupa `x` positivo (+1.345…+2.140) y `GEO_REAR_WING_*` ocupa `x` negativo (−2.304…−1.952).

### 3.2 Origen y referencia vertical

`acc_y = 0` es el plano de referencia del chasis. El renderer coloca la malla en `body_position[0]` (la altura publicada del plano de referencia), de modo que:

`altura_mundo = body_position[0] + acc_y` (con roll = pitch = 0).

Con `body_position[0] = 0.050 m` nominal, el vértice más bajo (`acc_y = −0.025101`) queda a **24.899 mm** sobre el suelo, que coincide con la auditoría de chasis. La cadena de conversión del generador es:

`acc_y = source_y + 0.48865`; el renderer añade la holgura de referencia (0.05), y la validación de exportación comprueba `acc_y + 0.05 = source_y + 0.53865`.

### 3.3 Origen longitudinal y centro de gravedad

El XML sitúa los ejes en `x = +1.146992` (delantero) y `x = −1.743008` (trasero), relativos al origen de la carrocería. El centro de gravedad del solver está en:

`statGC.x = 1.146992 × 0.445 + (−1.743008) × 0.555 = −0.456958055 m`.

Por tanto:

`x_solver = x_acc + 0.456958055`.

Comprobación: eje delantero `1.146992 → 1.603950`; trasero `−1.743008 → −1.286050`; batalla `2.890000 m`. Coincide con la geometría nativa.

### 3.4 Signo lateral

Los objetos con nombre resuelven la lateralidad:

- `GEO_BODYWORK_SIDEPOD_SURFACE_PANEL_LEFT`: `z` medio −0.2626 m;
- `GEO_BODYWORK_SIDEPOD_SURFACE_PANEL_RIGHT`: `z` medio +0.2626 m.

Por tanto, `acc_z` positivo es la **derecha**. El solver usa `y` positivo a la izquierda:

`y_solver = −acc_z`.

Un punto del lado izquierdo tiene `acc_z < 0` y `y_solver > 0`.

## 4. Auditoría de la superficie inferior

La malla no tiene un objeto "floor" separado; la superficie inferior forma parte de `GEO_BODYWORK_MAIN_BODY_AND_COCKPIT_SHELL`. Se muestreó por interpolación de triángulos, no por vértices sueltos, porque la malla tiene polígonos grandes y vértices dispersos.

| Superficie | `acc_y` | `acc_x` | `acc_z` | Altura al suelo con H = 0.05 |
| --- | --- | --- | --- | --- |
| Tabla central (plank) | −25.10 mm | −1.30…+0.90 | ±0.10…±0.30 | 24.90 mm |
| Borde de tabla en `x = −0.80` | −24.15 mm | −0.80 | ±0.28 | 25.85 mm |
| Tabla trasera (rear-most) | −21.00 mm | −1.80 | ±0.10 | 29.00 mm |
| Suelo lateral | −2.97…−3.00 mm | −1.30…−0.20 | ±0.40…±0.60 | 47.0 mm |
| Borde trasero del suelo | 0.00 mm | −1.40 | ±0.50 | 50.0 mm |
| Alerón delantero (endplates) | −6.04 mm | +1.51…+1.99 | ±0.64…±0.67 | 43.96 mm |
| Deflector lateral del sidepod | +0.62 mm en el borde trasero; +63 mm en `x = 0.5` | −0.31…+0.82 | ±0.14…±0.64 | 50.6…113 mm |
| Caja de cambios / motor | +129 mm | −2.30…−1.77 | ±0.06 | 179 mm |

Observaciones:

- La tabla central es la superficie más baja y el primer contacto en suelo plano.
- El tramo trasero de la tabla está 4 mm más alto que la tabla central, pero queda detrás del eje trasero; con morro arriba su brazo es mayor y contacta antes que el resto del fondo.
- El suelo lateral está 22 mm por encima de la tabla; entra en contacto con balanceo.
- El borde trasero del suelo está 25 mm por encima de la tabla; entra en contacto con cabeceo de morro arriba.
- El alerón delantero está 19 mm por encima de la tabla, pero su brazo longitudinal es mayor; con cabeceo de morro abajo contacta casi al mismo tiempo que la tabla delantera.
- El deflector del sidepod no es la superficie lateral más baja; su borde inferior está a 50.6 mm y se eleva hacia delante. No se propone como punto primario.
- La caja de cambios está muy por encima del fondo; no es un punto de contacto del fondo.

## 5. Propuesta de puntos

Coordenadas en el marco del solver (origen en el GC, `x` adelante, `y` izquierda, `z` arriba). `gap` es el hueco nominal con `H = 0.05 m`, roll = pitch = 0.

### Nivel 1: mínimo viable (8 puntos)

| ID | Papel | `x` | `y` | `z` | `gap` (mm) | Origen |
| --- | --- | --- | --- | --- | --- | --- |
| F1 | Delantero centro | 1.306958 | 0.000000 | −0.02510 | 24.90 | main body, `acc_x 0.85` |
| F2 | Delantero izquierdo | 1.306958 | 0.100000 | −0.02510 | 24.90 | main body, `acc_z −0.10` |
| F3 | Delantero derecho | 1.306958 | −0.100000 | −0.02510 | 24.90 | main body, `acc_z +0.10` |
| M1 | Central | 0.456958 | 0.000000 | −0.02510 | 24.90 | main body, `acc_x 0.00` |
| R1 | Trasero centro | −0.343042 | 0.000000 | −0.02510 | 24.90 | main body, `acc_x −0.80` |
| R2 | Suelo trasero izquierdo | −0.943042 | 0.500000 | 0.00000 | 50.00 | main body, `acc_x −1.40` |
| R3 | Suelo trasero derecho | −0.943042 | −0.500000 | 0.00000 | 50.00 | main body, `acc_z +0.50` |
| T1 | Tabla trasera centro | −1.343042 | 0.000000 | −0.02100 | 29.00 | main body, `acc_x −1.80` |

### Nivel 2: recomendado (10 puntos)

Añade el suelo lateral central:

| ID | Papel | `x` | `y` | `z` | `gap` (mm) | Origen |
| --- | --- | --- | --- | --- | --- | --- |
| L1 | Suelo lateral izquierdo | 0.056958 | 0.550000 | −0.00297 | 47.03 | main body, `acc_x −0.40`, `acc_z −0.55` |
| L2 | Suelo lateral derecho | 0.056958 | −0.550000 | −0.00297 | 47.03 | main body, `acc_z +0.55` |

### Nivel 3: cobertura completa (12 puntos de fondo)

Añade los bordes de la tabla trasera:

| ID | Papel | `x` | `y` | `z` | `gap` (mm) | Origen |
| --- | --- | --- | --- | --- | --- | --- |
| T2 | Tabla trasera izquierda | −1.343042 | 0.100000 | −0.02100 | 29.00 | main body, `acc_x −1.80`, `acc_z −0.10` |
| T3 | Tabla trasera derecha | −1.343042 | −0.100000 | −0.02100 | 29.00 | main body, `acc_z +0.10` |

### Grupo separado: alerón delantero

| ID | Papel | `x` | `y` | `z` | `gap` (mm) | Origen |
| --- | --- | --- | --- | --- | --- | --- |
| W1 | Endplate izquierdo | 2.206958 | 0.655000 | −0.00604 | 43.96 | `GEO_FRONT_WING_ENDPLATE_LEFT` |
| W2 | Endplate derecho | 2.206958 | −0.655000 | −0.00604 | 43.96 | `GEO_FRONT_WING_ENDPLATE_RIGHT` |

El alerón delantero **no es el fondo del chasis**. Se propone como grupo aparte, desactivado por defecto hasta definir su evento/daño; no debe generar daño de rueda ni mezclarse con los contactos del fondo.

## 6. Cobertura y orden de contacto

Cálculo cuasiestático a `H = 0.05 m`: ángulo que cierra el hueco de cada punto, sin dinámica ni movimiento del suelo.

| Movimiento | Primer contacto | Ángulo | Segundo contacto |
| --- | --- | --- | --- |
| Balanceo izquierdo | Alerón delantero W1 | 3.85° | Suelo lateral L1, 4.89°; suelo trasero R2, 5.73° |
| Balanceo derecho | Alerón delantero W2 | 3.85° | Suelo lateral L2, 4.89°; suelo trasero R3, 5.73° |
| Cabeceo morro abajo | Tabla delantera F1 | 1.09° | Alerón delantero W1/W2, 1.14°; central M1, 3.12° |
| Cabeceo morro arriba | Tabla trasera T1 | 1.24° | Suelo trasero R2/R3, 3.04°; trasero R1, 4.16° |

Sin el grupo del alerón delantero, en los niveles 2–3 el primer contacto lateral es el suelo lateral L1/L2 a 4.89°; en el nivel 1 (sin L1/L2) el primer apoyo lateral es R2/R3 a 5.73°. La tabla delantera sigue siendo el primer contacto en cabeceo de morro abajo, y la tabla trasera T1 en cabeceo de morro arriba.

Estos ángulos no son una predicción dinámica: solo verifican que el conjunto cubre los cuatro apoyos y que el orden es físicamente admisible.

## 7. Contrato para el solver

Para cada punto `i`:

```
g_i = H + z_i - x_i*pitch + y_i*roll - ground(x_world, y_world)
J_i = [1, y_i, -x_i]          (H, roll, pitch)
v_n,i = Hdot + y_i*rolldot - x_i*pitchdot - ground_velocity
```

- Impulso normal `λ_i >= 0`, sin componente tangencial (sin tracción).
- Fuerza generalizada: `Q = Σ J_i^T λ_i = [Σλ_i, Σ y_i λ_i, −Σ x_i λ_i]`.
- El punto de aplicación es el propio punto; no se usa una elevación escalar global.
- La identidad de la restricción debe ser `(point_id, kind)`, no `(wheel, kind)`.
- Capacidad mínima: 12 puntos de fondo + 2 de alerón + 4 límites de rueda + topes. La capacidad actual del solver es 4; debe ampliarse antes de conectar la geometría.
- Contacto vertical inicial. En pendiente, el hueco vertical no es la distancia normal; hay que medir el límite y decidir si se usa la normal del terreno por punto en una fase posterior.
- El grupo del alerón delantero debe tener su propio evento; no reutilizar el daño de rueda del fondo.

## 8. Límites

- La malla es visual, no CAD. No hay espesor de tabla, desgaste ni bloques de patín separados; la superficie inferior central se usa como tabla.
- La malla tiene polígonos grandes y vértices dispersos; las alturas se obtuvieron por interpolación de triángulos. La incertidumbre declarada es ±0.5 mm.
- Las zonas curvas (sidepod, difusor) no tienen normal vertical; el contacto vertical es una aproximación. La tabla y el suelo lateral son casi horizontales (`normal_y ≈ −1` en la malla).
- El signo lateral se resolvió con los paneles izquierdo/derecho. Si el modelo se invirtiera, las etiquetas izquierda/derecha se intercambiarían, pero el conjunto simétrico no cambia.
- `H = 0.05 m` es la holgura nominal publicada; en runtime debe usarse la posición de equilibrio del solver. Los `z_i` son independientes de `H`.
- No se ha validado dinámicamente ni en conducción. La cobertura de §6 es geométrica.

## 9. Reproducción

```
python tools/inspect_vehicle_floor.py \
  implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.acc \
  --json <salida.json>
```

La herramienta imprime:

- cotas por objeto (main body, sidepods, alerón delantero, alerón trasero, suspensiones);
- la envolvente inferior por estaciones longitudinales;
- el ajuste del plano del fondo;
- el signo lateral resuelto por los paneles.

### Validación de la propuesta

```
python tools/validate_floor_contact_points.py \
  --mesh implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.acc \
  --xml implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml \
  --points implementation/floor_contact_points.json
```

Comprueba 100 condiciones: cotas de carrocería, centro de gravedad, mínimo nominal de 24.899 mm, altura de cada punto contra el triángulo de la malla, mapeo ACC→solver, hueco nominal, simetría de los pares y recuento de niveles. Resultado: **100 comprobaciones, 0 fallos**.

Evidencia original: `D:/open-formula-migration-evidence/vertical-floor-20261021` (`floor_audit.json`, `floor_audit.log`, `validation.log`, `hashes.txt`).

## 10. Decisiones aplicadas

1. **Nivel 2**: el XML habilita los 10 puntos de fondo de los niveles 1–2. Los puntos de nivel 3 (T2/T3) y el alerón delantero quedan escritos pero desactivados.
2. **Alerón delantero**: incluido como grupo separado, desactivado. No participa en el contacto ni en el daño de rueda.
3. **Soporte de datos**: `tools/apply_floor_contacts.py` genera la sección `Formula Floor` desde `implementation/floor_contact_points.json`; el XML se verifica contra el JSON después de escribirlo.
4. **Tabla**: la superficie de la malla a −25.1 mm se toma como tabla real; no se añade espesor ni desgaste.

## 11. Implementación del contacto

### 11.1 Datos

- `Formula Floor` en el XML: 14 puntos, 10 de fondo habilitados, 4 deshabilitados (T2/T3 y W1/W2).
- `tools/prepare_williams_vehicle.py` llama a `apply_floor_contacts` después de escribir el XML, de modo que la sección sobrevive a una regeneración del vehículo.
- `car.cpp` carga la sección en `tCar` y `SimCarBuildVerticalConfiguration` la copia al solver.
- El adaptador muestrea el terreno en la posición mundial de cada punto (`SimCarChassisContactWorldPosition`, `SimCarSampleGroundHeightAt`) y pasa altura y velocidad al solver.
- `TireVerticalSolverStepInput` incorpora `chassis_ground_height_meters` y `chassis_ground_height_velocity_meters_per_second`.

### 11.2 Ley de contacto

- Contacto rígido unilateral, sin tracción, con impulso normal `λ >= 0`.
- Restricción de posición `g_i = H + z_i - x_i*pitch + y_i*roll - ground`; Jacobiano `[1, y_i, -x_i]`.
- Restitución objetivo 0 (inelástico): toda la energía normal se disipa en el impulso. No se añade rigidez ni amortiguación de contacto.
- Capacidad: 20 restricciones (4 límites de rueda + 16 contactos). Los diagnósticos publican hueco, impulso, contacto activo y pérdida de energía por punto.
- Se añadió una regularización numérica de `1e-9` en el bloque de restricciones para tratar la redundancia de un cuerpo rígido con más de tres apoyos coplanarios. No es una rigidez física; la penetración activa se acota a `1e-5 m` y la inactiva a `1e-4 m`.

### 11.3 Conjunto de apoyo

Un cuerpo rígido sobre un plano queda determinado por tres apoyos independientes. El solver selecciona hasta **tres filas de contacto independientes** por paso, priorizando el hueco más negativo y comprobando el rango del Jacobiano. Los demás puntos habilitados quedan inactivos y se publican como tales (`formula_floor_active = 0`).

Esto es una limitación explícita: los 10 puntos están en el XML, pero no todos portan carga simultáneamente. El reparto entre puntos redundantes de un contacto rígido es indeterminado; el solver elige una solución admisible de tres apoyos. Un LCP completo o una formulación de polígono de apoyo es el siguiente paso para repartir la carga entre todos los puntos.

### 11.4 Respuesta externa

Cuando `Formula Floor` está habilitado con al menos un contacto, `SimCarCollideZ` retorna sin ejecutar su respuesta vertical ni su escritura de `rideHeight`. La autoridad vertical queda en el solver.

**Pendiente:** los eventos y daños que producía `SimCarCollideZ` no están mapeados todavía a los impulsos aceptados. No se declara conservada esa puerta.

### 11.5 Resultados

| Banco | Resultado |
| --- | --- |
| `validate_formula_vertical_authority` | 56 comprobaciones, 0 fallos |
| `validate_formula_tire_vertical_system` | 208 comprobaciones, 0 fallos |
| `validate_formula_tire_geometry` | 144 comprobaciones, 0 fallos |
| contacto, fuerzas, energía, térmica, selector, aerodinámica, chasis | salida 0 |

El banco de integración cubre: un contacto, dos contactos simétricos, balanceo, cabeceo, caída libre de 50 pasos con los 10 puntos del nivel 2 configurados (hasta tres apoyos activos), huecos, impulsos, energía y capacidad. No cubre la lectura aerodinámica en conducción ni los eventos de daño.

### 11.6 Pendiente

- Mapear los impulsos aceptados a eventos y daños sin duplicar ni inventar daños de rueda.
- Retirar el clamp angular externo y definir el tratamiento fuera de dominio.
- Verificar y documentar que la lectura aerodinámica del paso anterior es íntegra. La fuga de `rideHeight` por `collide.cpp` ya no ocurre cuando `Formula Floor` está habilitado, porque la respuesta vertical externa no se ejecuta.
- Validar en conducción real, con DLL instalada y hashes registrados.
- Sustituir el conjunto de apoyo de tres puntos por un LCP/polígono completo si se exige reparto entre todos los contactos.

### 11.7 Evidencia

- Banco: `D:/open-formula-migration-evidence/vertical-contact-20261021/vertical_authority_bench.log` (56/0).
- Hashes: `simuv5.dll` `9CBD34ADEFC01EF6B5D15BE68CC26B1A10466C0061E66CDFF5B70A0BBA66ECDD`; banco `5D89A48174E0152BF62EDE0B1B901889C64B238600F53DC9152141EC82799371`; XML de candidato `E6E4A89411790393BC0AD2C6C9F870F59DE1B236ACBBED5D0C1A47EB962A6C69`; puntos `51068E2219D81AF61C6FB9119816747A5159722021D54F82199E87992D2592A2`.
- **DLL instalada para prueba manual** en `distribution/lib/games/speed-dreams-2/modules/simu/simuv5.dll`, hash `9CBD34AD…6ECDD`.
- XML instalado en `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`, hash `E6E4A894…A6C69`; 100 comprobaciones de puntos, 0 fallos.
- Copias de seguridad y manifiesto: `D:/open-formula-migration-evidence/vertical-contact-install-20261021`.
- Prueba de arranque: el juego alcanza el garaje; el log confirma `Formula Floor: 14 contacts, 10 enabled, section=1` y la inicialización del solver acoplado sin fallo de paso. La conducción no está validada.
