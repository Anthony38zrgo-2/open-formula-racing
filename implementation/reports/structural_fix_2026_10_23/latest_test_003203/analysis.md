# Ultima prueba: Melbourne, 2026-10-23 00:32:03

Resultado: el problema no queda resuelto y la prueba termina con un paso mecanico rechazado.

Se preservan runtime.log completo, diagnostics.csv (155 muestras, 142 tiempos distintos), saved_setups.json y evidence.json. La DLL instalada coincide con el candidato ba4dc5a73277bb13b4f86f9fb54790ae9cb3d99530bf423ca3f2dc813edbead7. El log confirma una transaccion de reglaje aplicada (9 campos); no identifica los valores de esos campos. La copia SQLite refleja los reglajes guardados al revisar, no demuestra por si sola el estado activo de cada instante.

Orden de ruedas: FL, FR, RL, RR. travel se interpreta como coordenada de compresion, no altura libre de la rueda sobre el suelo. reach en este log contiene en realidad steering_angle_rad (rustphysics.cpp); no usarlo como distancia de alcance.

| Tiempo fisico s | Cargas N FL/FR/RL/RR | Observacion |
|---|---|---|
|32.674|3981 / 736.8 / 2156.3 / 1375.4|Cuatro contactos|
|32.924|4387.5 / 0 / 3044 / 312.3|FR pierde contacto|
|34.174|2981.9 / 0 / 2611.4 / 889.8|FR sigue sin contacto|
|34.424|1099.9 / 1524.2 / 2687.6 / 0|FR recupera apoyo; RR lo pierde|
|34.674|1284.7 / 827.2 / 3159.2 / 298.2|Cuatro contactos|
|35.330|0 / 0 / 0 / 33562.8|Ultimo estado aceptado; solo RR|

La perdida continua de FR esta muestreada entre 32.924 y 34.174 s. Con intervalo de 0.25 s, su duracion queda acotada aproximadamente entre 1.25 y 1.75 s. La FR varia su coordenada de compresion: 79.1 mm al primer cero, 75.9 mm a 33.174 s, 67.4 mm a 33.924 s y 71.6 mm a 34.174 s. Hay movimiento de suspension, pero no recuperacion rapida del apoyo. El log no incluye holgura rueda-suelo, velocidad axial, limite droop ni desglose de fuerzas para establecer por que no alcanza el suelo.

A 35.174 s hay fuerte deslizamiento lateral (vf=9.151, vl=14.851 m/s). En el ultimo estado vf=-0.857 y vl=14.879 m/s; compresion RR=185 mm y carga RR=33.56 kN. Esta carga puntual no equivale a apoyo estable.

Error exacto: MECHANICAL_STEP_REJECTED time=35.32999999999723 reason=Ground constraint is incompatible with suspension bump limit.

El guard de simplified_axle_integration.rs rechaza cuando la compresion exigida por el contacto, adelantada al extremo del intervalo, supera el maximo permitido. Confirma incompatibilidad detectada entre restricciones, pero el log no identifica rueda, exceso ni si proviene de geometria, prediccion del cuerpo o tratamiento numerico del contacto. No demuestra por si solo una causa unica para la perdida previa de FR.

rustphysics.cpp marca car.failed y no vuelve a integrar; tampoco refresca el snapshot al fallar. Por eso se repite time=35.330 y rejected=0: el contador mostrado pertenece al ultimo snapshot aceptado y no invalida el error registrado.

Siguiente investigacion: reproducir esta trayectoria y registrar rueda, limite de suelo actual/predicho, limites droop/bump, velocidad de compresion, aceleracion de anclaje, fuerzas de muelle/amortiguador/ARB, impulsos y residual. Revisar la compatibilidad conjunta de cuerpo, contacto y tope durante el solve, y por separado la recuperacion de FR antes del fallo. Los tests de banco y spawn anteriores no cubrieron esta maniobra. No se modifico codigo de produccion durante esta revision.
