# Corrección del bloqueo por reactivación de contactos

Fecha: 21 de octubre de 2026. Repositorio: D:/open-formula. Base Git: bfb708b4500efe15dba347da20af40f46a5e0fe0, conservando los cambios locales anteriores. Sin commit ni push.

## Resultado y evidencia observada

Se reprodujo y corrigió un ciclo de liberación/reactivación de contactos que agotaba las 24 iteraciones del active set. La DLL corregida está instalada. La validación nativa es correcta; falta repetir conducción gráfica con esta DLL.

En la sesión detenida, runtime.log registró a 00:01:29.197:

    Coupled vertical step failed (final) at offset=0.000343750 duration=3.12500015e-05 residual=3.2155492e-10 iterations=24 reason=4
    Vehicle physics step rejected; complete last valid car state preserved (car=0)

El residual pequeño no implicaba éxito: el conjunto de contactos no se estabilizó. La protección conservó el último estado válido y detuvo la física del coche; la interfaz continuó recibiendo entradas. La sesión terminó normalmente a 00:01:46.755. Este bloqueo no fue una nueva propagación de NaN.

El registro anterior no incluía todos los estados de los contactos. Se reprodujo el mecanismo y la misma firma de fallo, pero no se afirma haber reconstruido exactamente la trayectoria ni el contacto concreto de esa sesión.

## Causa y cambio

El solver admite hasta 1e-7 m de solape inicial como redondeo y conserva ese pequeño offset en el residuo de contacto:

    constraint_gap = gap_end - min(gap_previous, 0)

Sin embargo, la comprobación de contactos inactivos volvía a activarlos si el gap ANTERIOR era inferior a -1e-9 m. Un contacto podía liberarse correctamente porque su reacción era negativa y reactivarse inmediatamente por ese solape histórico, incluso mientras se separaba del suelo. El proceso se repetía hasta agotar 24 iteraciones.

Ahora la comprobación de factibilidad usa el mismo constraint_gap que resuelve la restricción. Se aplica a ruedas y puntos del fondo. Los contactos que realmente se cierran siguen activándose; los que se separan pueden liberarse.

No se elevaron límites de iteración ni se relajaron tolerancias. No se modificaron muelles, amortiguadores, XML, grip o aerodinámica. Se mantienen los rechazos de geometría inicial incompatible, NaN/Inf y el estado transaccional ante fallos irrecuperables.

Archivos de código modificados en esta corrección:

- src/modules/simu/simuv5/tire_vertical_solver.cpp: factibilidad coherente para reactivar contactos de rueda y fondo.
- tools/validate_rigid_contact_safety.cpp: regresiones de separación y soporte con solapes numéricos, distintas alturas absolutas y pasos temporales.
- src/modules/simu/simuv5/car.cpp: ante un fallo definitivo registra con precisión de double posición, estado vertical, fuerzas externas, terreno y restricciones activas, para permitir investigar otro caso sin perder su estado inicial.

## Reproducción antes y después

Banco mínimo: masa total de 600 kg, cuatro masas no suspendidas de 13 kg, un punto del fondo, ruedas sin contacto, gravedad nula y fuerza ascendente de 6000 N. El estado inicial tiene el pequeño gap indicado.

| Gap inicial | Paso | Antes | Después |
| --- | --- | --- | --- |
| -1e-8 m | 2 ms | Fallo, razón 4, 24 iteraciones | Éxito, 2 iteraciones |
| -1e-8 m | 31.25 microsegundos | Fallo, razón 4, 24 iteraciones | Éxito, 2 iteraciones |
| -5e-8 m | 2 ms | Fallo, razón 4, 24 iteraciones | Éxito, 2 iteraciones |
| -5e-8 m | 31.25 microsegundos | Fallo, razón 4, 24 iteraciones | Éxito, 2 iteraciones |

Los controles con gap 0 y -1e-10 m pasan tanto antes como después. La velocidad ascendente corregida es 0.021897810219 m/s a 2 ms y 0.000342153284672 m/s a 31.25 microsegundos, igual que en el control sin solape. No se crea un impulso de despenetración.

Los registros probe-before.txt y probe-after.txt, así como el fuente anterior y sus ejecutables, están en el directorio de evidencia indicado abajo.

## Validación

Compilación Release CMake/Ninja/UCRT64 y once bancos ejecutados con salida 0:

| Banco | Resultado |
| --- | --- |
| validate_rigid_contact_safety | 624 comprobaciones, 0 fallos |
| validate_formula_tire_vertical_system | 208 comprobaciones, 0 fallos |
| validate_formula_vertical_authority | 71 comprobaciones, 0 fallos |
| validate_formula_tire_geometry | salida 0 |
| validate_formula_tire_contact | salida 0 |
| validate_formula_tire_forces | salida 0 |
| validate_formula_tire_energy | salida 0 |
| validate_formula_tire_thermal | salida 0 |
| validate_formula_tire_selector | salida 0 |
| validate_formula_aerodynamics | salida 0 |
| validate_williams_chassis | salida 0 |

Son 903 comprobaciones explícitas en los tres bancos principales y ocho ejecutables adicionales de regresión correctos.

Las nuevas regresiones cubren ruedas y fondo, fuerzas de separación débiles y fuertes, velocidad inicial de separación, reacción de soporte ante fuerza descendente, alturas absolutas 0 y 33.21259947 m, gaps 0, -1e-10, -1e-8 y -5e-8 m, y pasos de 2, 1, 0.5 ms y 31.25 microsegundos. Se conservan las pruebas anteriores de catapulta, finitud, pasividad, contactos redundantes y rechazo transaccional.

La repetición de 10 segundos de irregularidad prescrita de 2 mm a 8 Hz mediante el adaptador de carretera y actualización vertical de producción pasa sin fallos. Máximos: velocidad vertical 0.052163869 m/s y carga normal 3714.10498 N. Es una prueba controlada del adaptador, no conducción completa en Fuji.

## Instalación y pendientes

- DLL: distribution/lib/games/speed-dreams-2/modules/simu/simuv5.dll, 446995 bytes.
- SHA-256 instalado y compilado: 1F2EB09716BCB3977D149D800EC07D300F16421990FB6162DBFCFE332FF28EB9.
- DLL anterior respaldada: 759F8BE64E0ADCA9281C4AD81C4FA2E438237921B0214CA89E53314A340AD86D.
- XML del Williams sin cambios: EDEEB8F3F3F448EDC1E33F35FB431C6AFE6344D3E9E75AB3C0B82BB82343F2AD.
- Manifiesto de distribución: 358C69AA275AE343F6C8B7549A822DA22A87750E29BC3402D79C81D78C8C9A08. Solo cambió su entrada de simuv5.dll.
- Respaldo, sesión observada y resultados: D:/open-formula-migration-evidence/rigid-contact-active-set-20261021.

Se verificó que el juego estaba cerrado antes de sustituir la DLL. Para revertir esta instalación, con el juego cerrado copiar simuv5.before.dll del respaldo a la ruta instalada y restaurar distribution_manifest.json desde ese mismo respaldo. Esa versión anterior contiene el bloqueo reproducido.

Falta repetir la maniobra de conducción que detuvo el vehículo, además de las puertas humanas ya declaradas de Fuji, pianos, pendientes y alta velocidad. No se autoconcede esa aceptación ni se afirma haber eliminado toda posible causa futura de fallo del solver. Si reaparece un fallo, conservar el primer diagnóstico ampliado que emite esta versión.
