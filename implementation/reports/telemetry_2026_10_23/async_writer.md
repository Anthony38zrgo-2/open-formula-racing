# Escritura de trazas fuera del hilo de simulacion

Se crea el hilo dedicado ofp-telemetry-writer al activar la captura. El planificador del sistema operativo asigna su CPU; no se fija afinidad a un nucleo supuestamente libre.

La ruta de simulacion reserva uno de dos puestos y copia los registros de la ventana bajo el mutex del anillo (vector preasignado). Despues libera ese mutex. El escritor trabaja solo con su copia: crea directorios, formatea el CSV mediante BufWriter, escribe CSV/JSON e informa RUSTTRACE al terminar. No accede al estado fisico ni al anillo durante el formato o la escritura.

Como maximo hay dos ventanas en vuelo, incluyendo la que se esta escribiendo. Si no queda sitio, try_send/reserva no esperan: se descarta la ventana y aparece RUSTTRACE_DROPPED. Los metadatos incluyen dropped_windows, writer_thread, capture_ms y format_write_ms. La copia sigue teniendo coste en el hilo principal; estos numeros permiten medirlo separado del disco.

La perdida de apoyo dispara una vez por rueda y episodio. La rueda debe recuperar apoyo estable durante 250 ms antes de rearmarse. Los topes tambien se disparan por transicion, no por cada tick en el tope. No se reinician los temporizadores al terminar un volcado.

El resumen del host baja de 50 a 25 Hz: diagnostic interval seconds = 0.04 en la configuracion activa, y 0.04 como fallback C++. Se mantienen los registros internos por subpaso e iteracion para diagnosticar T3.

Se exportan ofp_trace_flush (herramientas/pruebas) y ofp_trace_shutdown. El host llama a shutdown al cerrar la sesion, antes de entrar en la descarga de modulos; el cargador tambien lo comprueba antes de FreeLibrary: la cola se drena y se hace join del hilo mientras el codigo de la DLL todavia esta cargado. Esos bloqueos solo ocurren en exportacion explicita o limites de sesion, no en el tick. ABI de estructuras permanece en version 7.

El volcado explicito ofp_trace_dump sigue siendo sincrono para herramientas, pero tampoco mantiene el mutex del anillo durante formato/IO. Los volcados automaticos de apoyo, topes y rechazo usan exclusivamente el escritor.

Pruebas: async_tests.log (9 pruebas). Incluyen escritor detenido deliberadamente, dos ventanas en cola, tercera descartada sin bloquear, registro de nuevas muestras con escritor detenido, snapshot inmutable, nombre del hilo que escribe, drenaje/join y rearme tras recuperacion estable. La prueba de agotamiento del solver sigue conservando el candidato rechazado.

Verificacion de DLL/malla: async_dll_validation.log y telemetry_validation/telemetry_validation.json. Instalacion, hashes, respaldo y archivos protegidos: installation.json. La reproduccion manual de T3 y los FPS reales quedan pendientes del usuario. No se cambiaron fuerzas, reglajes del coche ni controles.

Banco sin graficos (async_benchmark/metrics.json): 800 pasos de 2 ms con captura activa y una ventana de 10820 filas. Mediana de paso 1.15 ms, p99 4.57 ms, maximo 12.11 ms; copia 9.87 ms y formato/escritura 225.82 ms en ofp-telemetry-writer. No son FPS del juego, ni una garantia para toda maniobra o carga del equipo.
