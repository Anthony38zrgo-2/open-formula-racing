# Revision independiente de instrumentacion

Resultado: implementacion parcial, no aceptada como completa. Se ejecuto de nuevo verify_telemetry.py: 41 comprobaciones pasan. Hash de DLL candidata e instalada coincide (91dc8c9c...), rustphysics instalado coincide con README (56957773...). No se arranco el juego ni se modifico codigo de produccion.

## Hallazgos

1. P1 - Separacion con signo invertido. simplified_axle_integration.rs:69 calcula ground_limit - compression. La restriccion del integrador es compression >= ground_limit; por tanto una rueda factible separada del suelo produce un valor NEGATIVO aunque el contrato documenta positivo = en el aire. Corregir a compression - ground_limit y validar una rueda elevada cuyo rayo todavia encuentre suelo. Las pruebas actuales solo comprueban la separacion cercana a cero.

2. P1 - El detector de perdida de apoyo usa visibilidad del suelo. vertical_diagnostics.rs:560 resetea el temporizador siempre que ground_detected es true. Una rueda sin reaccion que todavia ve la pista nunca dispara contact_loss, precisamente el caso de FR que se quiere estudiar. Usar el estado de apoyo/ground_binding con tolerancia y validar perdida sostenida con rayos positivos. La prueba actual eleva el coche hasta que los rayos no encuentran suelo, otro caso.

3. P2 - Carga normal falsa en CSV. vertical_diagnostics.rs:508 copia element_force_sum_n a normal_force_n. En el volcado regenerado FL_normal_force_n=1180.467 N, igual a la suma de elementos; FL_contact_impulse_ns=5.531523 con paso 1/240 s implica ~1327.57 N de reaccion normal. No son la misma magnitud: incluyen balances distintos. Registrar la carga normal que realmente consume el neumatico y mantener la suma en otra columna.

4. P1 - Rechazo por agotamiento de iteraciones pierde su candidato. simulation.rs: for 0..32 solo asigna failing al encontrar error mecanico o de integracion. Si termina normalmente sin converger, failing queda None y record_failure no se ejecuta (requiere Some(candidate)). El volcado de ese fallo carece de kind=4 y de la esquina del candidato; no lo cubre la prueba de suelo incompatible. Mantener el ultimo candidato aun cuando solo falle el cierre del cuerpo y probar explicitamente esa ruta.

5. P2 - La traza no cumple la granularidad/ventana garantizadas. begin_step se llama una vez por paso del host (open_formula.rs:837), todos los subpasos y subdivisiones heredan el mismo tiempo y dt; no existen llamadas a kind::SUBSTEP ni kind::RESTORED. El anillo de 20480 registros tampoco garantiza cuatro segundos con 32 iteraciones por paso: a 480 Hz harian falta al menos 63360 registros contando un tick por paso, sin contar reintentos. Actualizar contexto por intervalo, guardar dt real, estados aceptados/restaurados y subpasos, y declarar/medir perdida de historia si se limita la memoria.

## Otros faltantes

Punto/normal y mascara de rayos se publican en snapshot pero no se escriben en RUSTSUSP ni en el CSV automatico, por lo que no quedan persistidos para revisar una sesion posterior. El registro llamado anchor_p usa anchor - up*(length-initial_compression), es decir posicion del centro virtual de rueda al inicio, no el anclaje fijo al chasis; documentar o separar ambas cinematicas. La posicion/velocidad son del inicio y la compresion es final: etiquetar niveles temporales.

## Lo que si funciona

ABI 7, firma y hashes; desglose por esquina/eje; reglaje efectivo; numeros concretos del rechazo suelo/tope; refresco de snapshot y contador al fallar; congelacion identificada; steer_rad renombrado; resumen configurado a 0.02 s y trace activado en configuracion distribuida.

Las pruebas existentes validan disponibilidad, estatica, ausencia total de suelo y rechazo geometrico inmediato. No validan los hallazgos anteriores ni la maniobra real de T3.
