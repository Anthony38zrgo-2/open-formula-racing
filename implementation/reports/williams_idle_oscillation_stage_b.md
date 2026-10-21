# Oscilación en reposo del Williams: Etapa B (parcial)

Fecha: 20 de octubre de 2026. Rama: `codex/initial-port`. Base: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`.

Este informe documenta el intento de resolver el contrato vertical completo. **La Etapa B no se declara cerrada**: el solver implícito aislado es estable, pero su integración en la geometría de rueda puntual de simuv5 no produjo apoyo sostenido en el coche completo. La producción conserva la referencia estable de la Etapa A.

## 1. Contrato documentado antes de tocar ecuaciones

Coordenadas y signos de simuv5:

- `x` avance, `y` izquierda, `z` vertical.
- `wheel->pos.z` es la referencia de altura de rueda del chasis.
- `susp.x` es recorrido de suspensión escalado por `bellcrank`; su valor sin escalar es `susp.x / bellcrank`.
- `susp.v` es velocidad de compresión positiva.
- `SimSuspForceAt` devuelve fuerza en coordenadas de rueda, incluyendo `bellcrank`.
- La fuerza de suspensión se aplica al chasis; la fuerza del neumático es la reacción de contacto.
- Unidades: m, s, kg, rad, N, N·m, J, W, Pa absolutos, K.

Estados implicados:

- `susp.x`, `susp.v`, `susp.a`.
- `wheel->formula_tire_model.contact.deflection` y velocidad.
- `wheel->formula_tire_model.force` (slip y memoria).
- `wheel->formula_tire_model.thermal` y `wear`.
- `wheel->mass` como inercia no suspendida virtual.

Orden de actualización auditado:

1. `SimCarUpdateWheelPos`.
2. `SimWheelUpdateRide`.
3. `SimAxleUpdate`.
4. `SimWheelUpdateForce`.
5. `SimTransmissionUpdate`.
6. `SimWheelUpdateRotation`.
7. `SimCarUpdate`.

Las dos rutas de `simu.cpp` (`SimUpdate` y `SimUpdateSingleCar`) pasan por el mismo `SimWheelUpdateForce`.

## 2. Solver implícito propuesto

Se creó `tire_vertical_model.cpp/.h` como solver puro de una masa no suspendida virtual:

- Estado: compresión de neumático `d` y velocidad `v`.
- Fuerza de neumático: `F_t = k_t*d + c_t*v`, con recorte a cero.
- Fuerza de suspensión: callback evaluado en `travel = total + d` y `-v`.
- Ecuación de momento: `m*(v_new-v) - dt*(F_s - F_t) = 0`.
- Resolución implícita por bisección con límites de compresión y recorrido.
- Impulso de límite y estado de clamp registrados.
- `elapsed_seconds = 0` no modifica el estado.

El banco `tools/validate_formula_tire_vertical.cpp` valida:

- equilibrio estático
- fuerza de neumático y chasis
- perturbación de compresión
- energía no creciente
- convergencia de velocidad y posición
- tiempo cero
- contacto perdido

Resultado: **10/10 PASS**.

## 3. Por qué no se promovió al coche completo

La integración en `SimWheelUpdateForce` y `SimWheelUpdateRide` se probó con mandos neutros y telemetría a 500 Hz. La respuesta del coche completo fue inaceptable:

- cargas delanteras nulas durante la mayor parte de la ventana;
- deflexión y velocidad de suspensión sin apoyo sostenido;
- el coche no quedó apoyado en el equilibrio estático;
- la variante se descartó antes de instalar.

La causa pendiente es geométrica: simuv5 representa la rueda como punto de contacto y no tiene una altura de centro de rueda independiente. El solver aislado presupone una relación `travel/compression/radio` que no coincide con `wheel->pos.z`, `susp.x` y la altura de la malla de Fuji. Ajustar solo el signo o el offset no basta; hace falta definir el grado de libertad y el origen de coordenadas del centro de rueda.

No se ocultó el fallo con amortiguadores, barras, mu ni con un filtro de primer orden.

## 4. Estado de producción

La producción instalada conserva:

- selector corregido de la Etapa A;
- integrador de contacto de `8531ccb`;
- inicialización de deflexión en equilibrio;
- telemetría de fuerza y par;
- configuración `transient coupled` a 130 kPa manométricos.

El solver `tire_vertical_model` **no está enlazado en simuv5**; queda como módulo experimental y banco aislado. No debe promoverse como solución final.

## 5. Pruebas ejecutadas

- `validate_formula_tire_selector`: 6/6 PASS.
- `validate_formula_tire_forces`: 20/20 PASS.
- `validate_formula_tire_contact`: 12/12 PASS.
- `validate_formula_tire_thermal`: 24/24 PASS.
- `validate_formula_tire_energy`: 9/9 PASS.
- `validate_formula_tire_vertical`: 10/10 PASS (solver aislado).

Comparación neutral de la Etapa A, que sigue siendo la referencia de producción:

| Modelo efectivo | Balanceo RMS | Carga delantera | Pasos con carga < 1 N |
| --- | ---: | ---: | ---: |
| Legacy real | 0.000411° | 1282.9–1386.5 N | 0 % |
| Acoplado real | 0.000123° | 1270.3–1404.9 N | 0 % |

## 6. Archivos

- `src/modules/simu/simuv5/tire_vertical_model.cpp/.h`: solver experimental, fuera de simuv5.
- `tools/validate_formula_tire_vertical.cpp`: banco aislado.
- `src/modules/simu/simuv5/tire_model.cpp`: selector corregido y reset de equilibrio.
- `src/modules/simu/simuv5/tire_contact_model.cpp/.h`: integrador de `8531ccb` restaurado.
- `src/modules/simu/simuv5/wheel.cpp`: ruta vertical de `8531ccb` restaurada.
- `src/modules/simu/simuv5/car.cpp`: telemetría.
- `implementation/reports/williams_idle_oscillation_stage_a.md`: referencia estable.
- `implementation/reports/williams_idle_oscillation_stage_b.md`: este informe.

## 7. Pendiente para cerrar la Etapa B

1. Definir el grado de libertad de centro de rueda y el origen de altura de contacto en simuv5.
2. Resolver el acoplamiento suspensión–carcasa con una formulación geométrica que no dependa de una altura de carretera desplazada.
3. Añadir el banco acoplado rueda–suspensión–chasis con masa suspendida/no suspendida, perturbación de balanceo y contacto unilateral.
4. Repetir la comparación neutral a 500 Hz y exigir apoyo sostenido, sin cargas nulas ni picos de 5–10 kN.
5. Validar frenada hasta detenerse, aceleración, retención, cambios, marcha atrás, pianos y conducción sostenida.
6. Cerrar la aceptación humana.

No se hizo commit ni push.

## 8. Puerta 1 geométrica

El 20 de octubre de 2026 se añadió el adaptador puro `tire_vertical_geometry.cpp/.h` y el banco `validate_formula_tire_geometry`. La Puerta 1 queda validada con **144/144 PASS**, usando los valores cargados del XML del Williams y los `staticPos` recentrados al GC. El adaptador define `C = H + R - L`, `d = L - (H - G)`, `susp.x = b*L` y `susp.v = -b*L_velocity`, con la velocidad de `H` derivada de la misma transformación de `SimCarUpdateWheelPos`.

La producción sigue en la referencia de Etapa A y el adaptador todavía no está llamado desde `wheel.cpp`. El solver conjunto de siete grados de libertad y la evaluación pura de la suspensión nativa siguen pendientes. Detalle completo en [williams_idle_oscillation_stage_b_gate1.md](williams_idle_oscillation_stage_b_gate1.md).

## 9. Puerta 2: bloque conjunto de siete grados de libertad

El 20 de octubre de 2026 se añadió `tire_vertical_solver.cpp/.h` y el banco `validate_formula_tire_vertical_system`. La Puerta 2 queda validada en banco con **208/208 PASS**:

- cuarto de coche con equilibrio, perturbaciones, impulso, contacto perdido/recuperado, topes y 500/1000 Hz;
- cuatro ruedas con heave, roll y pitch de ambos signos, barras y heave con bellcrank distinto de uno, energía, topes y 500/1000 Hz;
- leyes nativas de muelle y amortiguador comparadas contra `SimSuspUpdate`;
- matriz conjunta y matriz suspendida definidas positivas; con `Ldot = 0` se recupera `M_reference`;
- Jacobiano analítico contra diferencias finitas con error máximo `1.63e-6`.

El solver todavía **no está conectado** a `wheel.cpp`, `car.cpp` ni a la ruta de `SimUpdate`. La producción sigue en Etapa A; la integración en el coche completo y Fuji es la Puerta 3. Detalle completo en [williams_idle_oscillation_stage_b_gate2.md](williams_idle_oscillation_stage_b_gate2.md).

## 10. Puerta 3: integración neutral en el coche completo

El 20 de octubre de 2026 se conectó el solver a `SimUpdate`/`SimUpdateSingleCar` mediante el selector temporal `Formula Controls / vertical integration`, con `reference` como valor predeterminado. La ruta nueva:

- resuelve el bloque conjunto y publica pose del chasis, `H/G/C/L/d`, `N/S`, `susp.x/v/a`, `rideHeight` y centro visual;
- actualiza el neumático acoplado sin ejecutar el oscilador de carcasa;
- no ejecuta la integración vertical legacy de rueda ni de chasis.

En Williams/Fuji con mandos neutros, 500 Hz y 12.07 s de física, la ruta nueva dio roll RMS `0.0°`, cargas delanteras `1336.590–1336.771 N`, traseras `1667.002–1667.191 N` y `0 %` de pasos con carga menor de 1 N. La referencia de producción dio roll RMS `0.0001399°` y cargas delanteras `1271.1–1394.3 N`. La ruta `reference` del árbol actual reprodujo la referencia.

Se inyectaron los momentos externos de roll/pitch por aero, alas, suelo, fuerzas horizontales del neumático y pares giroscópicos. La secuencia diagnóstica forzada validó aceleración, cambio 1→2, frenada hasta cero, marcha atrás y una dirección suave: velocidad `-4.099` a `+6.573 m/s`, marchas `-1, 0, 1, 2`, `0 %` de pasos sin carga, pitch `-0.01045` a `0.01408 rad` y roll `-0.000378` a `0.000387 rad`. La misma secuencia con la referencia alcanzó picos de carga de `3.59 kN` y perdió contacto en una fracción pequeña de pasos.

Quedan pendientes curvas de alta velocidad, curbs, pendientes, boxes/reinicio, conducción sostenida y aceptación humana. La distribución quedó enrutada a la ruta nueva para prueba manual; la DLL/XML/manifiesto de Etapa A están respaldados y `restore_stage_a_distribution.ps1` revierte la instalación. Detalle completo en [williams_idle_oscillation_stage_b_gate3.md](williams_idle_oscillation_stage_b_gate3.md).
