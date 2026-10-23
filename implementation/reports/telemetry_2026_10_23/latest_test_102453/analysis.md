# Diagnóstico de la sesión del 23 de octubre de 2026, finalizada a las 10:24:53

Williams, Melbourne. 6112 muestras, de 0 a 244.44 s, cada 40 ms. Evidencia preservada: runtime.log, parsed.json, events.csv y ticks aceptados de dos ventanas (FR_accepted.csv y FL_accepted.csv). No se modificó física ni configuración.

## Resultado

El mecanismo dominante de pérdida prolongada de apoyo delantero es la descarga por la barra estabilizadora: su fuerza sobre la esquina descargada cancela o supera al muelle. El amortiguador de extensión contribuye en transitorios, pero no domina la retención prolongada. Esto identifica el mecanismo observado; no demuestra por sí solo que la rigidez o la geometría sean incorrectas.

## Evidencia

- FR pierde apoyo en el resumen de 100.52 a 102.56 s: aproximadamente 2.04 s. Velocidad inicial 146.4 km/h y fuerte desaceleración posterior. Separación máxima medida mientras hay raycast: 55.1 mm; no es un máximo garantizado durante los misses.
- A 100.520 s, el CSV kind=1 confirma N=0, separación=0.204 mm, muelle=1797.783 N, amortiguador=-151.962 N, barra=-1694.126 N. A 100.518 s aún había 120.197 N de reacción. Los rayos siguen viendo suelo cuando se pierde apoyo.
- A 100.600 s: muelle=1699.0 N, amortiguador=-8.6 N, barra=-1837.5 N. FR permanece elevada 10.3 mm. La barra supera al muelle incluso cuando casi no actúa el amortiguador.
- FL pierde apoyo en el resumen de 125.60 a 127.00 s: aproximadamente 1.40 s y hasta 34.5 mm medidos. A 126.20 s: muelle=1307.6 N, amortiguador=-60.2 N, barra=-1562.4 N; N=0. El mecanismo también ocurre en el lado opuesto.
- En 44 episodios delanteros de duración muestreada >=0.24 s hay 873 muestras sin apoyo. Todas tienen topes (0,0); 864 tienen barra negativa y en 700 su magnitud supera al muelle. Promedios: muelle +1313.5 N, amortiguador -50.2 N, barra -1388.4 N. La suma no es la aceleración neta: faltan gravedad, inercia y términos del marco móvil.
- Tiempo aproximado sin apoyo, contando muestras con N<1 N y velocidad>5 m/s: FL 17.20 s, FR 32.04 s, RL 2.52 s, RR 5.80 s. Incluye episodios breves. Los bordes del resumen tienen resolución de 40 ms. Los CSV a 2 ms muestran contactos breves que puede omitir el resumen.

## Autoridad de la barra

RUSTSETUP registra ratios efectivos delantero 1.00 y trasero 0.80, sin otra transacción de reglaje en el log. RUSTAXLE registra 103040.4 N/m delante y 15681.5 N/m detrás. Una diferencia de recorrido de 15 mm genera unos 1546 N delante con la ley implementada.

El suspension.json instalado contiene front.arb_ratio=1.00 y geometry_physical.front_arb.ratio=0.15. simplified_suspension.rs:15 sustituye LegacyRatio por el ratio del eje; el snapshot efectivo confirma 1.00. El factor 6.67 respecto al 0.15 escrito no demuestra que 0.15 sea el ajuste correcto.

simplified_axle_integration.rs:473 obtiene una rigidez fija desde k_wheel_rest; la fuerza usa la diferencia de recorridos (línea 766). El muelle usa la cinemática de la esquina en el recorrido actual. Esta combinación merece una comprobación controlada antes de elegir otro reglaje. No se ha demostrado un error de signo o Jacobiano.

## Factores secundarios y límites

En 197 de las 873 muestras prolongadas seleccionadas no se detecta suelo. open_formula.rs:650 limita el rayo a spring_length+radius, desde el anclaje en dirección -body_up; no incorpora explícitamente toda la extensión permitida. Esto deja zonas sin medida y puede condicionar la recuperación, pero no demuestra que cada miss corresponda a suelo alcanzable. En FR la carga desaparece antes del miss.

Ninguno de los 873 registros seleccionados llega al tope duro de extensión o compresión. Aumentar recorrido o reducir solo rebound no ataca el mecanismo dominante observado.

El resumen y estos CSV no registran pedal de freno ni giro individual de ruedas. La desaceleración está medida, pero no permite atribuirla exclusivamente al freno ni concluir que un bloqueo inicia el problema. Además open_formula.rs:1078 calcula steering_angle_rad con fold(0.0, max), perdiendo los ángulos negativos. steer_rad=0 no prueba volante centrado. La clasificación exacta de cambios de dirección requiere corregir esa publicación o derivar la trayectoria con cautela.

Las ventanas seleccionadas declaran history_truncated=true; se analizaron los ticks aceptados presentes, sin asumir tres segundos completos de prehistoria. Una comparación algebraica de fuerzas no equivale a reproducir la trayectoria con otra barra.

## Siguiente comprobación

Reproducir las maniobras con entradas registradas y comparar controladamente la autoridad de la barra, conservando muelles, amortiguadores y grip; verificar rigidez de referencia y Jacobianos contra la cinemática actual. Después revisar alcance de rayos. No atribuir todavía todo el defecto a un valor de reglaje incorrecto.

## Procedencia

El log carga ABI 7, source_commit=f3efcf246c2a2abf62477037ed0243f2f9a93a9b, build=4a14467. Los hashes actuales coinciden con el recibo de instalación:

- vehicle_physics_engine.dll: dcc0b44437ee5746cffe2325fd6c71f01c61af97bf901658d197ad1916faee90
- rustphysics.dll: 00ec1ad1ca057b623bb69d189dff555e9464257d76c13086629a61c12087eddb

Solo se crearon artefactos de análisis. No hubo instalación, commit ni push.
