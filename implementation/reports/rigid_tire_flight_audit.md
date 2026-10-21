# Auditoría del lanzamiento y NaN con contacto rígido

Actualización: los defectos descritos aquí corresponden a la versión auditada. La reparación, sus pruebas y la nueva DLL instalada se documentan en rigid_tire_flight_fix.md; se conserva esta evidencia del fallo original.

Fecha: 21 de octubre de 2026. Base Git: bfb708b4500efe15dba347da20af40f46a5e0fe0, con cambios locales del contacto rígido y autoridad vertical.

Esta revisión no cambia fuentes de producción, reglajes, DLL ni XML instalados. Se inspeccionó el registro del fallo y se compiló un reproducer aislado contra una copia exacta del solver actual. No se ejecutó el juego.

## Evidencia conservada

Directorio: D:/open-formula-migration-evidence/rigid-tire-flight-audit-20261021.

- runtime.log: copia del registro de la sesión que termina con matrices NaN.
- tire_vertical_solver.cpp/.h, car.cpp y simu.cpp: copias de los fuentes auditados.
- probe.cpp, probe.exe y probe-results.txt: reproducción autocontenida y resultados.
- SHA-256 del solver copiado y actual: 972AEFCCC107F5A919A8B0E7B085BCEF10DEFA18E43942DCC547D854B3C5A51C.
- SHA-256 de car.cpp copiado y actual: A2026175D2D74851B5A4029FD47773431BFC3CC7FB8E0134385F29F64CBAEEBC.
- DLL instalada: 733D44685F3F3BB04359D4746722E9941A75B320813E0F254A14F016F8C2B15D, coincidente con la candidata registrada en rigid_tire_contact.md.

El log contiene en su línea 609:

```text
00:00:46.934 ... Coupled vertical step failed (predictor)
at offset=0.001000000 duration=3.12500015e-05
residual=7.11068981e-05 iterations=60
```

Desde la línea 613 aparecen matrices de render con NaN. El registro no contiene el estado vertical, huecos y multiplicadores inmediatamente anteriores. Confirma fallo numérico seguido de NaN; no permite identificar por sí solo qué punto de contacto inició el lanzamiento ni probar la causa exacta del cierre del proceso.

## P0: recuperar penetración mediante velocidad física produce una catapulta

TireVerticalConstraintResidual impone gap_final=0 usando q_final=q_anterior+h*v_final. Para una penetración ya existente p, eso obliga a recuperar aproximadamente p/h como velocidad de separación. Es una corrección geométrica convertida en movimiento físico, sin una condición de pasividad que limite su trabajo.

La subdivisión adaptativa reintenta el intervalo penetrado con h menor: no reduce necesariamente el defecto geométrico inicial. Por tanto puede aumentar la velocidad como 1/h y la fuerza como 1/h².

Reproducción con masa total 600 kg, cuatro masas no suspendidas de 13 kg, suelo inmóvil, gravedad nula, sin fuerza externa, velocidad inicial cero, suspensión sin precarga y contacto central del fondo. El único defecto inicial es un solape de 10 mm:

| Caso | Resultado | Velocidad vertical final | Reacción | Energía inicial → final |
| --- | --- | ---: | ---: | ---: |
| Sin solape, h=2 ms | Aceptado | 0 m/s | 0 N | 0 → 0 J |
| Solape 10 mm, h=2 ms | Aceptado | 5 m/s | 1 370 000 N | 0 → 6 850 J |
| Mismo solape, h=31.25 µs | Aceptado | 320 m/s | 5 611 520 000 N | 0 → 28 057 600 J |

El último intervalo coincide en duración con el mínimo que alcanzó la sesión del log, pero el solape sintético NO es una medición del solape que tenía el coche real. Esto demuestra un mecanismo capaz de reproducir el síntoma, no la atribución completa de esa sesión.

Archivos: tire_vertical_solver.cpp (TireVerticalConstraintResidual, líneas 785-808; resolución Newton) y car.cpp (SimCarSolveVerticalFrame, líneas 1355-1373).

Corrección requerida: distinguir impacto alcanzado desde un estado admisible de penetración preexistente/entrada geométrica incoherente. La recuperación de solape debe tratarse en el estado autoritativo sin convertir automáticamente toda la corrección en velocidad física o trabajo positivo. Si no existe una recuperación admisible, rechazar/diagnosticar el caso. No resolverlo con topes arbitrarios a N ni ajustando muelles. Validar también la coherencia temporal de las muestras de terreno antes de atribuir el solape a la geometría.

## P0: el solver acepta NaN como un estado convergido

El reproducer introduce NaN únicamente en body_position[0] y mantiene el resto finito. TireVerticalSolverStep devuelve:

```text
nan_state ok=1 H=nan residual=0
```

Las comparaciones de hueco con NaN no activan ni rechazan restricciones; con fuerzas finitas independientes de esa posición, el residual dinámico puede ser cero. No existe una validación completa de finitud del estado/entradas/salida antes de aceptar y publicar.

Archivos: tire_vertical_solver.cpp, TireVerticalSolverStep (línea 1378 en adelante), comprobaciones finales 1740-1808; car.cpp, publicación del resultado.

Corrección requerida: comprobar finitud de configuración, entradas, estado previo, iterados, multiplicadores, residuos, salida y conversiones a float; un valor no finito nunca puede producir converged=1. Conservar el último estado completo válido y comunicar el fallo a la actualización completa del vehículo.

## P0: el fallo vertical no detiene la evolución del resto del vehículo

SimCarSolveVerticalFrame pone formula_vertical_failed=1 y retorna false; SimCarVerticalCoupledUpdate retorna y en los pasos siguientes sale al encontrar ese flag. Sin embargo, las dos rutas de simu.cpp continúan con SimTransmissionUpdate, SimWheelUpdateRotation y SimCarUpdate después de llamar a SimCarVerticalCoupledUpdate.

Así quedan congelados el estado vertical y sus fuerzas, mientras avanza el movimiento horizontal. Si las cargas o velocidades retenidas son enormes, sus efectos sobre grip, transmisión, posición y matrices pueden seguir propagándose. El flag no implementa la parada coherente que exigía el handoff.

Corrección requerida: propagar el estado de fallo al bucle completo; suspender de manera coherente la integración del vehículo afectado y evitar todas las publicaciones no finitas. Probar ambas rutas, los efectos del predictor sobre bodyVel y la restauración atómica de las vistas. No basta con retornar desde el subsistema vertical.

## P1: el contacto del fondo acepta reacciones de tracción y oculta su impulso

En la rama TIRE_VERTICAL_CONSTRAINT_CHASSIS del active set, valid se fija a 1 y gap se descarta (líneas 1570-1578). La comprobación final excluye los contactos de chasis del control de signo (líneas 1759-1766). Algunos contactos fuera de la base también quedan excluidos de la prueba de penetración final.

Reproducción: un punto del fondo toca el suelo, gravedad nula y fuerza exterior ascendente de 6 000 N. El contacto debería liberarse. El solver devuelve éxito y mantiene el chasis pegado al suelo mediante lambda=-6 000 N e impulso=-12 N·s. El impulso publicado específicamente como contacto del fondo es cero, aunque el impulso negativo figura en el conjunto de restricciones y se utilizó para resolver el movimiento.

Esto viola unilateralidad y hace que parte de los diagnósticos no represente la reacción aplicada. No es una limitación aceptable para conducción; el informe anterior la describía como trabajo pendiente, pero el resultado instalado no cumple el contrato físico pedido.

Corrección requerida: liberar contactos con signo inadmisible, comprobar todas las desigualdades, gestionar redundancias sin descartar contactos físicos y rechazar conjuntos incompatibles en lugar de convertir un contacto en una sujeción bilateral.

## Por qué las validaciones existentes no detectaron el problema

- En tools/validate_formula_tire_vertical_system.cpp, el cálculo de numerical_loss_joules resta también constraint_work, incluyendo trabajo positivo de contacto. En el reproducer, el suelo realiza cero trabajo, pero chassis_contact_work_joules vale exactamente 6 850 J o 28 057 600 J. Restarlo como si justificase la ganancia permite que una catapulta pase una comprobación de balance. Medir trabajo no demuestra que ese trabajo sea físicamente admisible.
- En tools/validate_formula_vertical_authority.cpp, CheckClose considera fallo solo cuando fabs(actual-expected)>tolerance. Esa condición es falsa con NaN; la comprobación puede imprimir PASS para una magnitud no finita.
- El mismo banco comprueba que algunos impactos eleven la posición y produzcan impulso, sin exigir en todos ellos un balance mecánico pasivo ni una velocidad de recuperación acotada por el problema físico.

Corrección requerida: pruebas de pasividad con suelo inmóvil y sin fuentes externas, rechazo explícito de NaN/Inf, separación entre trabajo del terreno y artefacto de recuperación, y repetición de penetración a varios h. No añadir la energía creada a un término contable para hacer pasar el balance.

## Otro incumplimiento: tamaño de subpaso configurado ignorado

SimCarSolveVerticalFrame recibe substep_count y substep_seconds, calcula frame_duration y encola únicamente un intervalo de toda esa duración (líneas 1311 y 1327-1329). Solo subdivide después de fallar. No respeta como máximo el subpaso vertical solicitado, por lo que variar vertical substep seconds puede no cambiar el resultado si el paso completo converge. Esto invalida comparaciones de frecuencia que supongan esa configuración efectiva.

## Orden de reparación y pruebas que faltan

1. Propagar fallo/NaN a toda la integración y preservar un estado válido completo; esto contiene el problema, pero no corrige el lanzamiento.
2. Corregir la recuperación de penetración y el contrato temporal del terreno. Exigir ausencia de creación de energía en el reproducer estático a 2 ms, 1 ms, 0.5 ms y 31.25 µs. Declarar si el caso se recupera sin energía artificial o se rechaza de forma explícita.
3. Restaurar unilateralidad y factibilidad del fondo, reconstrucción de reacciones coherente y diagnósticos fieles al movimiento resuelto.
4. Respetar el subpaso configurado y corregir las comprobaciones de los bancos. Añadir casos de NaN/Inf en cada grupo de entradas, fuerza ascendente con contacto que debe liberarse, redundancias/permutaciones y fallo inducido en predictor/final.
5. Ejecutar integración con muestreo real de pista, entrada/salida de superficie y cambio de rasante; registrar el primer estado/hueco/impulso anómalo. No atribuir el fallo concreto de Fuji a un contacto específico sin esa traza.
6. Solo tras superar estas puertas, generar otra candidata para conducción. La DLL instalada no queda validada por los resultados históricos del banco ni por alcanzar RACE.

## Reproducción

El probe incluye directamente la copia del solver, sin mocks ni cambios en sus ecuaciones. Se compiló con el compilador UCRT64 existente y enlace estático:

```powershell
$env:PATH = 'C:/Users/bill/open-formula-build-tools/msys64/ucrt64/bin;' + $env:PATH
& C:/Users/bill/open-formula-build-tools/msys64/ucrt64/bin/c++.exe -std=c++17 -O0 -static D:/open-formula-migration-evidence/rigid-tire-flight-audit-20261021/probe.cpp -o D:/open-formula-migration-evidence/rigid-tire-flight-audit-20261021/probe.exe
& D:/open-formula-migration-evidence/rigid-tire-flight-audit-20261021/probe.exe
```

No se ha reparado ni reinstalado producción en esta revisión. El defecto reproducido y las faltas de contención están confirmados; el contacto que inició la sesión real y la causa exacta del cierre del proceso siguen sin una traza suficiente.
