# Ultima prueba: 2026-10-23 02:21:22

Evidencia preservada: runtime.log completo (13.37 MB), 3902 resumenes en samples.csv, 12 ventanas CSV/JSON (358.66 MB), events.json, metrics.json y t3_fr.csv. No se cambio codigo ni configuracion durante este analisis.

## Suspension

El tiempo fisico llega a 78.020 s. No hay MECHANICAL_STEP_REJECTED ni RUSTFAIL. El rechazo de la sesion anterior no se reprodujo; no equivale a demostrar que su causa este corregida.

En T3, FR pierde apoyo de forma continua en las muestras de 29.840 a 31.240 s y recupera a 31.260 s: unos 1.42 s, con incertidumbre de muestreo de unos 20 ms por borde. No llega al tope de extension: los flags de topes son cero. Su compresion se mantiene alrededor de 36-40 mm y cambia poco.

A 30.000 s: muelle +1103.7 N, amortiguador +4.4 N, barra -1341.3 N, suma -233.2 N; separacion 16.7 mm, sin impulso normal. La barra tira de FR hacia arriba mas de lo que el muelle la empuja hacia abajo; la suma no es la aceleracion neta porque faltan gravedad y terminos de marco movil.

En las muestras descargadas entre 29.8 y 31.3 s: muelle medio +1120.1 N, barra -1238.5 N, amortiguador -19.25 N y suma -137.6 N. La barra domina este balance; no se observa al amortiguador como unica retencion. Esto apunta a revisar el equilibrio y el acoplamiento de la barra con el recorrido y la cinematica, sin probar todavia si la rigidez, su Jacobiano o su signo fisico son incorrectos.

Ademas, FR deja de detectar suelo en 43 de 72 muestras seleccionadas. Su separacion pasa a NaN y ray_mask a cero. El codigo consulta desde el anclaje y limita la longitud a spring_length+tire_radius; al faltar hit no sabe cuanto mas abajo sigue la pista (shortfall=0 en este caso no mide esa distancia). Investigar el alcance respecto al recorrido efectivo de extension y distinguir falta de alcance de ausencia real de superficie. No se puede afirmar solo con esto que la rueda ya deberia estar tocando suelo.

La prueba utiliza el perfil base al crear el coche: rebound frontal 1.12, trasero 1.35; resting_ratio 0.271429/0.336615. No hay transaccion de garaje aplicada en esta sesion. Por tanto no valida el Reglaje 1 guardado con rebound 0.2.

La ventana de T3 contiene iteraciones de cuerpo de media 9.98 y maximo 13, sin subdivisiones; no muestra un agotamiento de iteraciones como explicacion del problema de apoyo.

## FPS

El juego informa media 48.7 FPS, minimo 0.8, maximo 78.3. Hay fuerte evidencia de tirones producidos por la instrumentacion: 12 volcados de 28.7-30.5 MB (358.66 MB en total). service_window invoca write_window desde la ruta de paso aceptado; trace_dump_window formatea todo el CSV y hace std::fs::write sincronamente, manteniendo el mutex del anillo. No existe escritor de fondo.

Pares de resumenes consecutivos con solo 20 ms de avance fisico:
- t=70.340 -> 70.360: 744 ms de reloj de pared; RUSTTRACE entre ellos.
- t=73.740 -> 73.760: 1035 ms; RUSTTRACE entre ellos.
- t=75.000 -> 75.020: 688 ms; RUSTTRACE entre ellos.
- Otros volcados coinciden con huecos de unos 225-357 ms.

Estos huecos son observados entre mensajes, no medidas exactas de duracion del volcado ni del tiempo GPU. No hay perfil CPU/GPU por frame. La pausa inicial de 3.219 s queda separada porque puede corresponder a inicio/carga y no hay RUSTTRACE para atribuirla al volcado.

El detector vuelve a armarse tras cada ventana aunque continue la misma perdida de apoyo: en el tramo final se generan capturas cada ~1.25 s, lo que multiplica las pausas. El registro de resumen a 50 Hz añade 13.37 MB de texto durante la sesion.

Las 12 ventanas declaran history_truncated=true: los 20480 registros solo conservan aproximadamente 1.3-1.9 s al escribirlas, con ~0.32-0.88 s previos al disparo en lugar de los 3 s solicitados. La instrumentacion informa correctamente de la perdida, pero su capacidad es insuficiente para esta densidad de iteraciones.

## Siguiente correccion recomendada

Primero desacoplar formato/escritura del hilo de simulacion mediante una cola acotada y escritor de fondo; disparar una sola vez por transicion de apoyo a perdida, rearmando tras recuperacion estable; preservar los ticks aceptados en un historial separado de las iteraciones detalladas y registrar coste/cola/bytes/tiempo de paso. Despues repetir T3 con el mismo reglaje efectivo, y contrastar el equilibrio ARB/muelle y el alcance real de rayos. No ajustar solo amortiguacion ni rigidez para ocultar el sintoma.
