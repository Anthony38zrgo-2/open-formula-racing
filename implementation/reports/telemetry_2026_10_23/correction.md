# Correccion de la revision de telemetria

Se corrigieron los cinco hallazgos y los faltantes de persistencia/cinematica de independent_review.md. Cambios de instrumentacion; se conserva el modelo de fuerzas y las restricciones del solver.

- Separacion positiva = rueda elevada: q - limite de suelo al extremo del intervalo.
- La perdida sostenida de apoyo usa ground_binding y carga > 1e-6 N; no la visibilidad de pista. Una rueda que ve suelo pero no soporta carga activa el evento.
- CSV normal_force_n es la reaccion que consume el neumatico. element_force_sum_n es otra columna. contact_impulse_ns se convierte a la normal real igual que en el estado fisico; contact_dt_s permite comprobar J/dt incluso cuando un tick del host contiene varios intervalos.
- El ultimo candidato se conserva en cada iteracion fallida, tambien cuando se agotan las 32 sin error de eje. Se escribe kind=4. Tras rollback el volcado incluye kind=5 del estado aceptado restaurado, incluso si falla el primer paso. La ruta de estado no finito tambien registra su candidato.
- Cada transaccion/subdivision establece tiempo de extremo y duracion reales, manteniendo la secuencia del host. kind=3 registra ambos ejes resueltos de cada ensayo que supera las restricciones de eje; kind=2 registra el cierre cuerpo/esquinas. No se confunden estos ensayos con ticks aceptados kind=1.
- El anillo conserva 20480 registros para acotar memoria. La ventana solicitada sigue siendo 3 s previos/1 posterior, pero se informa oldest_retained_time_s, overwritten_records e history_truncated. No se garantiza detalle completo de cuatro segundos bajo cualquier numero de iteraciones. En rechazo no existe evolucion posterior: la captura es inmediata.
- CSV automatico persiste punto, normal, mascara de rayos, demanda predicha y cinematica del anclaje. El resumen RUSTSUSP tambien persiste punto/normal/mascara.
- anchor_p/v/a corresponde al anclaje fijo del chasis al inicio del intervalo. solve_frame_accel representa por separado la aceleracion movil usada por el solver (incluye el termino de Coriolis). Compresion/velocidad son del extremo final. Los niveles temporales quedan explicitos; no se cambio la aceleracion que alimenta las fuerzas.
- Los numeros no finitos se guardan como null en los metadatos JSON; el CSV conserva NaN para indicar dato indefinido.

## Verificacion

correction_tests.log: 44 pruebas pasan (6 acoplamiento + 15 reglaje + 16 contacto + 7 telemetria). La prueba de agotamiento usa la ruta de produccion con una estimacion inicial horizontal finita deliberadamente grande; no introduce un override de iteraciones. Verifica rechazo code=3, candidato kind=4, filas de eje y tiempo/dt reales.

correction_telemetry_tests.log: repeticion de las 7 pruebas de telemetria sobre el ultimo fuente, tras incorporar contact_dt_s.

correction_dll_validation.log y telemetry_validation/telemetry_validation.json: 47 comprobaciones sobre la DLL y malla reales; incluyen rueda sin apoyo que todavia ve pista (separacion positiva), reaccion normal distinta de suma de elementos, persistencia de rayos, estado restaurado y dt de subpasos de un tick de 10 ms.

La suite general --lib no compila por fixtures heredados ausentes (F1 2030 y automatic_transmission_tuning.json, mas una llamada a un constructor retirado). Las pruebas nuevas de diagnostico tambien se compilan en un harness de integracion para no depender de esos fixtures. No se alteraron esos fixtures para enmascarar el problema.

Instalacion y hashes: installation.json. Se sustituyen solo vehicle_physics_engine.dll y rustphysics.dll, con respaldo y rollback, verificando perfiles/reglajes/configuracion por hash. No se arranco el juego ni se condujo T3; aceptacion de la maniobra pendiente del usuario. No se hizo commit ni push.
