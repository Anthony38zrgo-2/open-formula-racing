# Oscilación en reposo del Williams: Etapa B, Puerta 2

Fecha: 20 de octubre de 2026. Rama: `codex/initial-port`. HEAD: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`.

Este informe cubre la Puerta 2: bloque vertical conjunto de siete grados de libertad y bancos de cuarto de coche y cuatro ruedas con las leyes nativas de suspensión, barras y heave. **La Etapa B no se declara cerrada**: el solver está validado en banco, pero no está conectado a `wheel.cpp`/`car.cpp` ni probado en el coche completo. La producción sigue en la referencia de Etapa A.

## 1. Resultado

- Solver puro creado en `src/modules/simu/simuv5/tire_vertical_solver.cpp/.h`.
- Banco `tools/validate_formula_tire_vertical_system.cpp`: **208/208 PASS**.
- Cuarto de coche: equilibrio, perturbaciones positiva y negativa, impulso de chasis, pérdida/recuperación de contacto, topes inferior y superior, tiempo cero y comparación 500/1000 Hz.
- Cuatro ruedas: equilibrio cargado del Williams, heave/roll/pitch de ambos signos, impulso de chasis, pérdida/recuperación de contacto, topes, heave dinámico, barras y heave con bellcrank distinto de uno, y comparación 500/1000 Hz con la misma excitación física.
- Jacobiano analítico comprobado contra diferencias finitas: error máximo `1.63e-6` en el caso cargado y `1.82e-7` con heave.
- Los bancos existentes siguen pasando: selector 6/6, contacto 13/13, fuerzas 21/21, térmica 25/25, energía 27/27, vertical aislado 10/10 y chasis 6/6.
- DLL y XML de producción sin cambios: SHA-256 `B14FB36D...9927` y `66FD0A8D...A426`.

## 2. Modelo

Coordenadas:

    q = [q0, roll, pitch, L0, L1, L2, L3]
    J_i = [1, wheel_y_i, -wheel_x_i]

`q0` es la altura de referencia del chasis en el GC. La aproximación de ángulos pequeños usada por el motor es:

    H_i = q0 - wheel_x_i * pitch + wheel_y_i * roll
    Hdot_i = v0 - wheel_x_i * pitch_dot + wheel_y_i * roll_dot
    d_i = L_i - (H_i - G_i)
    ddot_i = Gdot_i + Ldot_i - Hdot_i
    C_i = H_i + R_i - L_i

La aproximación se validó contra el adaptador geométrico exacto de la Puerta 1 a `roll = pitch = ±0.005 rad`; las diferencias de `H`, `d` y `C` quedan por debajo de `1e-6 m`.

Matriz de masa:

    M_reference = diag(masa_total, I_roll, I_pitch)
    M = [ M_reference    -J^T D ]
        [ -D J             D   ]

La matriz de masa suspendida `M_reference - J^T D J` se exige definida positiva y se comprueba con Cholesky. La matriz conjunta también se comprueba. Con `Ldot = 0` la energía cinética recupera exactamente `M_reference`; el banco lo verifica con una diferencia menor que `1e-9 J`.

Fuerzas generalizadas:

    Q_body = sum_i J_i^T N_i + gravedad + fuerzas externas
    Q_Li = S_i - N_i + m_i g + fuerzas externas
    N_i = max(0, k_ti d_i + c_ti ddot_i), solo con contacto geométrico
    S_i = S_suspensión_i + S_barra_i + S_heave_i

No se suma `S` otra vez al cuerpo: la transferencia se realiza mediante el acoplamiento de masa. El equilibrio de cada rueda cumple `N = S + m g`.

## 3. Ley nativa y barras/heave

La ley de muelle y amortiguador se implementó como evaluación pura, sin mutar `susp.x/v/a`, fuerza previa, daños ni estados. El banco la compara con `SimSuspUpdate` en una malla de posiciones y velocidades; el error máximo es menor que `2e-3 N`. La derivada del potencial del muelle coincide con la fuerza nativa dentro de `1e-6 N`.

Convención de trabajo para barras y heave, con `b` explícito:

    delta_barra = b_left L_left - b_right L_right
    U_barra = 1/2 K_barra delta_barra^2
    Q_L = -dU/dL

    x_heave = 1/2 (b_left L_left + b_right L_right)
    U_heave = b_heave U_muelle_heave(x_heave)
    Q_L = -dU/dL

Con `b = 1` estas expresiones coinciden con `SimAxleUpdate`. Con `b` distinto de uno, la pareja `axleFz` legacy no incluye el Jacobiano del bellcrank y no es conservativa en `L`; el solver nuevo usa la convención trabajo-gradiente anterior. El banco comprueba las fuerzas y la derivada numérica del potencial con `b` delantero `0.8/0.75`, trasero `0.37/0.37` y heave `0.9`. El caso cargado del Williams tiene `b = 1` y heave deshabilitado, por lo que no cambia respecto a la convención legacy.

## 4. Resolución numérica

- Euler implícito con posición y velocidad del mismo extremo temporal.
- Newton conjunto 7x7 con Jacobiano analítico y búsqueda de línea con reducción del residuo.
- Contacto, tramos de amortiguador y recorte del muelle se seleccionan en cada iteración según el estado actual.
- Topes inferior y superior mediante conjuntos activos con multiplicador, impulso y pérdida de energía contabilizada.
- Si Newton no converge, o si un límite o el signo del multiplicador no se satisface, el paso devuelve fallo y **no** escribe un estado parcial.
- `elapsed_seconds <= 0` se rechaza antes de cualquier proyección y deja el estado intacto.
- El cuarto de coche usa el mismo criterio en 2x2. El bloque de cuatro ruedas usa las cuatro extensiones y heave/roll/pitch.

## 5. Valores cargados y equilibrio del Williams

Cargados desde `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml` y la configuración nativa de `SimAxleConfig`/`SimSuspConfig`:

- Masa total con combustible: `612.6 kg`.
- `I_roll = 132.05845866498 kg.m2`; `I_pitch = 680.31329653572 kg.m2`.
- Ruedas: `x = +1.603950013, +1.603950013, -1.286049973, -1.286049973 m`.
- `y = -0.834986985, +0.834986985, -0.800000012, +0.800000012 m`.
- Masas no suspendidas: `13, 13, 16, 16 kg`.
- Muelle delantero: `K = -103040.421875 N/m`, `F0 = 1320.548584 N`, `x0 = 0.050000001 m`, límites `0.025..0.135 m`, `b = 1`.
- Muelle trasero: `K = -19601.890625 N/m`, `F0 = 1646.976440 N`, `x0 = 0.050000001 m`, límites `0.020..0.140 m`, `b = 1`.
- Barras: `100000 N/m` delantera y `80000 N/m` trasera.
- Heave: deshabilitado en el Williams cargado.
- Neumático delantero: `k = 165068.578125 N/m`, `c = 1318.397526 N.s/m`.
- Neumático trasero: `k = 205872.046875 N/m`, `c = 1633.432455 N.s/m`.

Los valores de neumático salen de `TireModelConfigurationRead` y de la fórmula nativa de amortiguación radial `2 * ratio * sqrt(k * m)`.

Equilibrio conjunto obtenido:

| Magnitud | Valor |
| --- | ---: |
| Altura de referencia del chasis | 0.046236941 m |
| Roll | 0.000000000 rad |
| Pitch | 0.002032956 rad |
| Extensión delantera | 0.051076678 m |
| Extensión trasera | 0.056951920 m |
| Carga delantera, cada rueda | 1337.137313 N |
| Carga trasera, cada rueda | 1667.665686 N |
| Fuerza de suspensión delantera | 1209.607313 N |
| Fuerza de suspensión trasera | 1510.705686 N |

La suma de cargas normales es `6009.606 N`, igual al peso total. El pitch pequeño es el equilibrio físico de precarga y rigideces, no un error de convergencia. Cada rueda cumple `N_i = S_i + m_i g`.

## 6. Cobertura y resultados de la Puerta 2

- Ley nativa de suspensión y potencial: PASS.
- Cuarto de coche: equilibrio, perturbación positiva y negativa, impulso de chasis, pérdida/recuperación de contacto, tope inferior, tope superior, tiempo cero y 500/1000 Hz: PASS.
- Cuatro ruedas: equilibrio, matrices de masa, recuperación de `M_reference` con `Ldot = 0`, geometría de ángulos pequeños, Jacobiano contra diferencias finitas: PASS.
- Perturbaciones de heave, roll y pitch de ambos signos: error final `<= 4.1e-10 m/rad`, residuo máximo `<= 1.0e-7 N`, pérdida numérica entre `1.06` y `3.94 J`, sin pérdida de contacto después del transitorio.
- Impulso de chasis: PASS, sin creación de energía y con contacto sostenido.
- Comparación 500/1000 Hz: diferencia de roll `-1.34e-6`, de heave `1.80e-6`, máxima de extensión `4.04e-6`.
- Badén físico de `10 mm` y `0.2 s` (independiente de la frecuencia de integración): diferencia de heave `8.03e-5 m`; pérdida numérica `1.853 J` a 500 Hz y `0.984 J` a 1000 Hz; sin creación de energía.
- Topes: impulso inferior `14.4741 N.s` y pérdida `14.4741 J`; impulso superior `-17.0891 N.s` y pérdida `92.1537 J`.
- Pérdida/recuperación de contacto en cuatro ruedas: PASS, conservando la dinámica de extensión y recuperando el apoyo.
- Heave dinámico: PASS, con Jacobiano contra diferencias finitas (`1.82e-7`).
- Barras y heave con `b` distinto de uno: fuerzas y derivadas del potencial comprobadas; deltas delantero `-0.00625 m` y trasero `-0.00185 m`; fuerzas de heave `1136.25, 1065.23, 686.19, 686.19 N`.
- Tiempo cero: rechazado sin modificar el estado.

La energía del banco incluye energía cinética de la matriz conjunta, potenciales de muelle, neumático, barra, heave y gravedad, trabajo de carretera, pérdidas de amortiguador, pérdida de amortiguación del neumático, pérdida de topes y pérdida numérica de integración. Para base fija y sin trabajo externo no se observa crecimiento sostenido; el residuo de balance se registra como pérdida numérica positiva y acotada.

## 7. Compilación, regresión y producción

Compilación:

    cmake --build C:/Users/bill/open-formula-build/engine --target simuv5 validate_formula_tire_vertical_system

Ejecución:

    validate_formula_tire_vertical_system.exe <williams-nineteen-ninety-seven.xml>

Resultado: `checks=208 failures=0`.

Regresión existente, toda con salida 0:

- `validate_formula_tire_geometry`: 144/144.
- `validate_formula_tire_selector`: 6/6.
- `validate_formula_tire_contact`: 13/13.
- `validate_formula_tire_forces`: 21/21.
- `validate_formula_tire_thermal`: 25/25.
- `validate_formula_tire_energy`: 27/27.
- `validate_formula_tire_vertical`: 10/10.
- `validate_williams_chassis`: 6/6.

Hashes:

- DLL instalada: `B14FB36D0286DCE4FA6F0A53531C214BB911DE56224AD84F4A5658353DBC9927`.
- XML instalado: `66FD0A8D05AF00C291C4647D0DDEC3A401B1D51423871AE49BA68B3CBCFDA426`.
- DLL compilada en el build: `654444240EF62241808C6D07D85110B74D7D70F0065024A2804205059D52E7C1`.
- No se copió la DLL experimental a `distribution` ni se instaló.

## 8. Límites y siguiente puerta

- El solver no está llamado desde `SimWheelUpdateRide`, `SimWheelUpdateForce`, `SimCarUpdateWheelPos`, `SimAxleUpdate` ni `SimCarUpdate`. La producción sigue en la ruta de Etapa A.
- No incluye fuerzas laterales/longitudinales, aero, momentos de cabeceo/balanceo por contacto, transmisión ni térmica. La Puerta 3 debe definir el orden de esas fuerzas y usar estados provisionales sin avanzar memorias durante Newton.
- El suelo del banco es altura/velocidad por rueda; no incluye normal local de Fuji, malla en coordenadas horizontales ni trabajo de contacto inclinado.
- La aproximación de ángulos pequeños está validada a `±0.005 rad`; el coche completo debe comprobar los ángulos reales y, si crecen, pasar al Jacobiano exacto.
- La inercia de heave no está soportada; la inercia de las esquinas se admite en la diagonal de masa. El Williams no usa inertance.
- No se ha validado el coche completo ni la aceptación humana. No se declara cerrada la Etapa B.

## 9. Evidencias

En `D:/open-formula-migration-evidence/vertical-stage-b-gate2-20261020/`:

- `coupled-solver-bench.log`: salida completa 208/208.
- `geometry-bench.log` y `energy-bench.log`: regresión de las puertas anteriores.
- `source-hashes-after.json`, `git-status-after.txt` y `git-diff-after.patch`.
- `gate2-validation-summary.txt`: resumen de resultados y hashes.

No se hizo commit ni push.
