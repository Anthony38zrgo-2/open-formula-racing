# Williams: configuración de monoplazas y diagnóstico de frenada

Fecha de revisión: 21 de octubre de 2026. Repositorio: `D:/open-formula`, HEAD `a68ee9e`.

Investigación del código, XML distribuido, última telemetría y referencias originales de Speed Dreams. No se cambian código, setup, binarios, instrucciones.md ni memorias. Se preservan los cambios que ya estaban presentes. No se realizó una nueva conducción, compilación, commit ni push.

## Resultado

Sí hay un error concreto de conversión y varios contratos/configuraciones que requieren revisión. Pero el registro disponible **no muestra que la inestabilidad comience por bloquear el eje trasero**: primero aumenta el deslizamiento de frenada delantero; después aparece saturación lateral trasera y, en la segunda frenada, bloqueo trasero tardío con una rueda descargada.

Los hallazgos principales son:

1. **Error confirmado: toe delantero con el mismo signo en las dos ruedas.** Produce una orientación común de −0,1°, en lugar de un ajuste simétrico por rueda.
2. **Desajuste de soporte aerodinámico muy plausible:** muelles traseros efectivos de 19,6 kN/m, sin tercer muelle y con solo 30 mm de compresión. En recta rápida la suspensión trasera ya trabaja a 1–4 mm del tope. Las referencias IndyCar con muelles de rueda blandos incluyen muelles de heave mucho más rígidos.
3. **Frenada actual muy adelantada:** 65 % de presión delante y menor fricción de pastillas detrás equivalen a aproximadamente 66,6 % del par hidráulico delante. El registro confirma bloqueo delantero previo. Seguir adelantando el reparto no está justificado por esta tanda.
4. **El consejo de bajar siempre la retención del diferencial necesita corregirse:** menos acoplamiento puede aumentar la rotación en entrada. No es una cura general del nerviosismo.
5. **Interferencia confirmada en el código de controles:** la ruta Formula sobrescribe el acelerador después del golpe de gas automático del piloto humano. Su contribución al problema requiere una medición específica de las reducciones.

La causa dominante del movimiento completo del coche todavía necesita una prueba controlada: los datos no permiten atribuir toda la descarga de ruedas a un fallo del solver, al fondo, al terreno o a un único ajuste.

## 1. La referencia IndyCar existe

Speed Dreams incluye las familias Monoposto America: MPA1, MPA11 y MPA12. La lista del proyecto identifica MPA11 con IndyCar 2011; las notas oficiales de 2.3.0 confirman MPA12 y sus coches Murasama, Deckard y Spirit. No se ha identificado un CART histórico concreto como el que recuerda el usuario.

Fuentes primarias:

- [Lista de coches del proyecto](https://sourceforge.net/p/speed-dreams/wiki/ListofCars/).
- [Notas oficiales de Speed Dreams 2.3.0](https://speed-dreams.net/en/blog/release-2-3-0).
- [XML original MPA11 Murasama](https://svn.code.sf.net/p/speed-dreams/code/trunk/data/cars/models/mpa11-murasama/mpa11-murasama.xml).
- [XML original MPA12 Murasama](https://svn.code.sf.net/p/speed-dreams/code/trunk/data/cars/models/mpa12-murasama/mpa12-murasama.xml).
- [XML original MP1 Bavaria FW27, inspirado en el Williams de 2005](https://svn.code.sf.net/p/speed-dreams/code/trunk/data/cars/models/mp1-bavaria-fw27/mp1-bavaria-fw27.xml).

Los tres XML de coche se descargaron y parsearon desde el SVN público del proyecto. La herramienta web no pudo abrirlos, pero la descarga HTTP directa devolvió los documentos XML completos. Copias y hashes se conservan en `C:/Users/bill/open-formula-openwheel-audit-20261021/`.

El submódulo local `speed-dreams-data`, revisión `8bb0beb8d1ec77ae02349468ada9f40db298a41d`, contiene categorías MPA y MP1, pero en este checkout solo tiene instalado el modelo sc-cavallo-360. Las categorías son límites/plantillas: **no deben confundirse sus valores con los del coche efectivo**. Por ejemplo, MPA11.xml declara 18 kN/m detrás; el coche Murasama consultado declara 22 kN/m y un tercer muelle de 285 kN/m.

## 2. Qué significa configurar correctamente un open wheel aquí

No basta con convertir medidas y copiar números de otro simulador. Es necesario cerrar estas relaciones:

- Geometría y signos: batalla, vías, centro de gravedad, altura de referencia, signos por lado de camber/toe.
- Tasas efectivas a rueda: si los datos representan el elemento de suspensión, aplicar `k_rueda = k_elemento × motion_ratio²`; no volver a aplicar esa relación mediante bellcrank. Comprobar además la equivalencia del movimiento no lineal y de la precarga cuando corresponda.
- Soporte vertical: peso y carga aerodinámica deben caber en los recorridos útiles. Las barras resisten diferencias izquierda/derecha; no sostienen una compresión simétrica del eje. Un tercer muelle permite sostener carga aerodinámica sin endurecer tanto los muelles de rueda en balanceo.
- Frenos: calcular par por rueda y contrastarlo con radio efectivo, carga normal, grip combinado y freno motor. La presión máxima aislada no mide la potencia de frenado.
- Transmisión: diferenciar freno motor sostenido, sincronización en reducción y acoplamiento del diferencial.
- Neumático: comprobar fuerza longitudinal/lateral, sensibilidad a carga, temperaturas, presión y recuperación después del deslizamiento en el modelo realmente ejecutado.
- Reglaje final: validar frenada recta antes de entrada en curva; después pianos, pendientes y velocidad sostenida. Variar una familia de parámetros cada vez.

Esto coincide con el método del proyecto, que separa suspensión, alas, barras y frenos: [BaselineSetupMethod](https://sourceforge.net/p/speed-dreams/wiki/BaselineSetupMethod/). La guía [36GPSetup](https://sourceforge.net/p/speed-dreams/wiki/36GPSetup/) es útil para interpretar los campos, pero está escrita para coches sin aerodinámica y versiones anteriores. Sus cifras y afirmaciones sobre parámetros sin uso no se trasladan directamente a este fork.

Además, este simuv5 local ya usa neumáticos transitorios propios, contacto vertical rígido y solver acoplado. Los XML originales no traen `Formula Tires` schema 2. Sirven como referencia mecánica, no como coches listos para ejecutarse sin migración ni como calibración intercambiable del grip.

## 3. Error confirmado: toe delantero sin simetría

Archivo distribuido: `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`.

| Rueda | Toe actual |
| --- | ---: |
| Delantera derecha, línea 274 | −0,1° |
| Delantera izquierda, línea 302 | −0,1° |
| Trasera derecha, línea 330 | +0,3° |
| Trasera izquierda, línea 358 | −0,3° |

`tools/prepare_williams_vehicle.py:424` copia el toe delantero sin aplicar un signo por lado; solo lo hace para el trasero. `tire_model.cpp:153` calcula directamente `waz = wheel->steer + wheel->staticPos.az`, sin invertir automáticamente el toe izquierdo. Por tanto, ambos delanteros apuntan hacia el mismo lado cuando el rack está centrado.

Esto confirma una asimetría de configuración; no demuestra que por sí sola explique la zaga nerviosa. Introduce una demanda lateral con dirección centrada y debe corregirse antes de calibrar el balance. Definir el objetivo de convergencia/divergencia y generar signos opuestos. Si se conserva una divergencia de 0,1° por rueda, la convención actual implica derecha −0,1° e izquierda +0,1°. Una prueba diagnóstica con ambos a cero también permite aislarlo.

El toe trasero sí es convergencia simétrica. No encontré un toe trasero invertido que explique el síntoma.

## 4. Suspensión: falta margen en heave detrás

Valores por rueda, convertidos a unidades SI. La tabla usa los XML de coche, no las categorías:

| Dato | Williams actual | MPA11 Murasama | MPA12 Murasama | MP1 Bavaria FW27 |
| --- | ---: | ---: | ---: | ---: |
| Masa XML, kg | 605 | 796 | 717 | 605 |
| Reparto estático delantero | 44,5 % | 41,5 % | 43 % | 40,2 % |
| Muelle delantero, kN/m | 103,04 | 42 | 320 | 185 |
| Muelle trasero, kN/m | 19,60 | 22 | 90 | 205 |
| Tercer muelle delantero, kN/m | 0 | 325 | 275 | 0 |
| Tercer muelle trasero, kN/m | 0 | 285 | 290 | 0 |
| Barra delantera, kN/m | 100 | 35 | 85 | 9,5 |
| Barra trasera, kN/m | 65 | 16 | 22 | 5,5 |
| Rigidez total trasera en heave, kN/m, con bellcrank 1 | 39,20 | 329 | 470 | 410 |

La última fila es `2 × muelle_de_rueda + tercer_muelle`; no representa la rigidez en balanceo. No implica que se deban copiar estas cifras al Williams.

El generador aplica motion ratio delantero 0,875923 y trasero 0,370497, medidos y documentados en `tools/williams_suspension_reference.json`. El cuadrado del trasero reduce 142,8 kN/m en el elemento a 19,60 kN/m en rueda. **La operación es mecánicamente correcta bajo la aproximación de ratio constante**. El problema es que esa equivalencia local, con precarga nativa y sin progresividad/linkage completos, no garantiza una plataforma aerodinámica adecuada.

El XML Williams omite Front/Rear Heave Spring. `car.cpp:256-266` inicializa esos muelles a cero cuando faltan; `car.cpp:684-703` les da precarga cero. El solver sí admite heave (`car.cpp:1012` y `tire_vertical_solver.cpp:296`), pero el Williams no lo configura.

En compresión simétrica la diferencia izquierda/derecha es cero y la barra no añade soporte. Subir la barra no remedia esta carencia.

Con ride trasero de 50 mm y packers de 20 mm hay 30 mm de compresión. Cada muelle trasero solo añade unos **588 N** sobre su soporte estático antes del tope, unos **1.176 N por eje**. Es una cuenta cuasiestática de muelles, sin amortiguación, transferencia de carga, contactos de fondo ni distribución real de momentos aerodinámicos. Delante la reserva equivalente es unos 2.576 N por rueda.

Evidencia de la tanda: a t=6,998 s y 240,5 km/h, sin freno, las extensiones traseras son 21,637 y 21,186 mm. A t=15,848 s, una alcanza 20,000 mm. En 87 muestras de más de 180 km/h, freno <1 % y rack <0,006 rad, ambas traseras se mantienen entre 20 y 23,991 mm. Esa selección incluye aceleración y lift-off; no es un ensayo de velocidad constante.

La reserva mínima y los topes están demostrados. Que el fondo descargue ruedas, que cambie bruscamente el balance aerodinámico o que exista una inestabilidad numérica son hipótesis que necesitan registrar fuerzas/contactos del fondo y más resolución temporal.

El [manual oficial iRacing Dallara IR18, pp. 17 y 21](https://s100.iracing.com/wp-content/uploads/2026/01/Dallara-IR18-V3.pdf) explica el uso del tercer elemento para soportar cargas verticales aerodinámicas con muelles de rueda blandos. Es una referencia funcional, no una especificación histórica del FW19 ni una equivalencia de parámetros entre motores.

## 5. Frenos: presión no es reparto de fuerza

La ruta normal de `brake.cpp:206-209` aplica la fracción delantera a la presión y su complemento detrás. `brake.cpp:51-60` calcula:

`T_rueda = P_max × pedal × fracción_eje × radio_disco × área_pistón × mu_pastilla`.

Con el XML activo, a pedal completo:

| Dato | Delante, por rueda | Detrás, por rueda |
| --- | ---: | ---: |
| Presión máxima de línea tras reparto | 13 MPa | 7 MPa |
| Diámetro de disco | 278 mm | 278 mm |
| Área de pistón | 25,5795 cm² | 25,5795 cm² |
| Mu de pastilla | 0,45 | 0,42 |
| Par hidráulico | 2.080 N·m | 1.045,33 N·m |

La telemetría confirma estos pares exactamente. La fracción delantera del par es 66,55 %. Con radios efectivos aproximados 0,305/0,314 m, la fracción de fuerza hidráulica delante ronda 67,2 %. El freno motor añade fuerza detrás y modifica el reparto total.

Los MPA consultados usan repartos de presión 56 % y 61 %, pero también otras áreas, fricciones, masas, neumáticos y relaciones. No se debe copiar el porcentaje ni comparar solo MPa.

Al cerrarse gas, `engine.cpp:212-268` usa aproximadamente `T_motor = −0,015 × omega −0,026 × T_pico`. Con pico de 390 N·m, a 13.000 rpm son unos −30,56 N·m. Es menor que en las revisiones anteriores y sigue multiplicándose por las relaciones de transmisión. La reacción de sincronización en una reducción es un efecto distinto del freno motor sostenido.

No hay inversión del eje en el reparto ni ABS/ESP/TCL habilitados en Features. Que un freno individual tenga `enable abs=1` no activa ABS si la feature global está deshabilitada.

## 6. Qué muestra realmente la última conducción

Se analizó `distribution/user-settings/runtime.log`, encabezado `2026/10/21 21:16:14`, 524 grupos de diagnóstico, máximo de velocidad longitudinal 275,56 km/h. Los tiempos siguientes son de simulación, no del reloj ni del prefijo del log. El registro incluye muestras t=0 antes de conducir y un reset del garaje; se usan las ventanas posteriores de conducción. La cadencia útil observada es aproximadamente 50 ms.

Ruedas: 0 delantera derecha, 1 delantera izquierda, 2 trasera derecha, 3 trasera izquierda. `slip_ratio` es adimensional: −1 corresponde aproximadamente a rueda parada respecto al avance. En cambio `Formula wheel/wheel_speed` es `spinVel × radio_geométrico`, expresado en m/s, **no rad/s**; `Formula coupled force/spin` sí es rad/s.

| Tiempo | Velocidad | Evidencia |
| --- | ---: | --- |
| 7,598 s | 209,8 km/h | Deslizamiento FR −0,209; FL −0,062; traseras −0,060/−0,040. El delantero supera el pico longitudinal nominal 0,12 primero. |
| 8,448 s | 159,9 km/h | Sin freno: ángulos de deriva traseros 6,6°/6,2°, cercanos al pico lateral nominal de 6,59° antes de considerar camber/migración combinada; slips longitudinales traseros 0,001/0,049. Hay pérdida lateral, no bloqueo trasero. |
| 8,548 s | 157,7 km/h | FL slip −0,971; traseras −0,018/−0,072. En toda la ventana 7,2–10 s no se registra una trasera por debajo de −0,95. |
| 15,898 s | 258,0 km/h | En 5ª: FL slip −0,307 y traseras −0,025/−0,027. |
| 16,098 s | 236,2 km/h | En 5ª: FL slip −1,000; traseras −0,027/−0,061. |
| 17,498 s | 109,0 km/h | Todavía en 5ª: delanteras −1,000/−1,000; RL −1,000 con carga normal cero. El primer bloqueo trasero casi completo muestreado llega 1,4 s después del delantero. |

La segunda frenada pierde estabilidad **sin reducir inicialmente de marcha**. Por tanto, las reducciones no explican por sí solas el inicio de ese episodio. No se registra aumento de damage en esta tanda; el accidente del informe previo pertenece a otra evidencia.

Los estados de rueda muestran oscilaciones/descargas fuertes, por ejemplo FL pasa por N=0 a 15,998 s; FR también pierde apoyo en muestras posteriores. Sin posición global de cada muestra, estado completo del terreno, reacciones del fondo y una repetición plana, no puede afirmarse que toda esa variación sea físicamente incorrecta o causada por el solver.

## 7. Diferencial: corregir la interpretación anterior

El Williams usa 1.5 WAY LSD, precarga 10 N·m, umbral 90 N·m, bias power 0,10 y coast 0,15, con `restoring coast lock=yes`.

`differential.cpp:340-365` usa esos coeficientes como amplitud de sesgo de par dependiente de entrada y diferencia de velocidades. No son un porcentaje de bloqueo cinemático ni rampas en grados. El signo restaurador de retención está activado.

La afirmación anterior «menos coast evita que una rueda arrastre a la otra y por tanto estabiliza» es incompleta. El acoplamiento transfiere par entre semiejes; no crea par neto de frenada. Puede ayudar a la rueda descargada y resistir la diferencia de velocidades que favorece la rotación. El [manual oficial IR18, p. 25](https://s100.iracing.com/wp-content/uploads/2026/01/Dallara-IR18-V3.pdf) describe mayor estabilidad/subviraje de entrada al aumentar el bloqueo en retención. Su ley no es idéntica a la local: comprobarlo con un A/B del coche completo.

Probar coast 0,15 frente a 0,30 con idénticos frenos, geometría y maniobra es defendible. Concluir de antemano que 0,15 es más seguro, o volver arbitrariamente a 0,70, no lo es.

## 8. Controles: el golpe de gas automático puede ser borrado

`rthumandriver.cpp:1651-1660` añade acelerador para el ajuste automático de rpm cuando se cumplen las condiciones de autoClutch. `HumanDriver::drive_mt`, desde línea 1901, llama primero a `common_drive`, después a `applyFormulaPedalControls`.

`formula_gear_controls.cpp:94-109` reconstruye el acelerador desde teclado/RT y escribe `car->_accelCmd` sin preservar el aumento anterior. El Williams habilita esta ruta con `simultaneous pedal controls=yes`. La interferencia de orden está confirmada por lectura del código. El log muestra acelerador cero alrededor de reducciones, pero su cadencia no permite excluir pulsos breves entre muestras ni medir el impulso de sincronización.

Hay que verificar/reparar esta composición de comandos antes de justificar otros recortes del freno motor por reducciones. No se afirma que explique la segunda frenada: el bloqueo delantero empezó mientras seguía en 5ª.

El pedal Formula tiene respuesta de aplicación directa, con zona muerta de 5 % y suavizado solo al soltar; el teclado pide 100 %. Esto afecta a la modulación. Las preferencias genéricas de sensibilidad del joystick no describen por sí solas la ruta efectiva RT/LT.

## 9. Orden recomendado de corrección y comprobación

1. Corregir únicamente la simetría del toe delantero en el generador y XML candidato. Verificar que con rack cero y velocidad recta los ángulos base sean los previstos. Es una corrección de conversión, no una afinación de grip.
2. Resolver el soporte vertical para la carga aero vigente: estudiar un tercer elemento o tasas efectivas que mantengan recorrido útil, conservando la calibración aerodinámica durante la comparación. No copiar sin más los 285 kN/m del MPA11. Auditar precarga, reparto estático, amortiguación de heave, topes y fuerzas de fondo en el solver existente.
3. En plano, frenar desde varias velocidades con una marcha fija y presión progresiva. Medir pedal al inicio de pérdida de adherencia en cada rueda. Evaluar una presión total menor para aumentar margen de modulación antes de seguir adelantando el reparto. El objetivo no es imponer que 100 % de pedal jamás bloquee a cualquier velocidad.
4. Repetir la prueba con transmisión desacoplada mediante un mecanismo verificado y registrar el estado del embrague. Separar después reducciones con/sin ajuste de rpm; revisar la composición del golpe de gas Formula y el impulso de sincronización.
5. Comparar coast 0,15/0,30 con todo lo demás fijo; después entrada en curva, lift-off, curva mantenida y salida. Decidir por velocidad de guiñada/deriva y apoyo, no por un porcentaje importado de otro motor.
6. Solo después afinar barras, toe trasero, camber, alas y térmica. Mantener fases separadas y guardar cada candidato con su manifiesto/hash.

Registrar a paso físico o resolución suficiente: tiempo, posición, velocidad local, guiñada/rate, rack y pedal, marcha, rpm, throttle, clutch_transfer, N por rueda, spin, slip_ratio/angle, fuerzas/capacidades, extensión/velocidad, topes, normales de terreno, reacciones/gaps del fondo, aero y temperaturas. Una salida 0 de bancos aislados no equivale a estabilidad de frenada del coche completo.

## Evidencia reproducible

Directorio externo: `C:/Users/bill/open-formula-openwheel-audit-20261021/`. Contiene el log copiado, XML Williams observado, los tres XML originales descargados e inventario Git previo a crear este informe. Las referencias son snapshots de investigación; no se instalaron.

SHA-256:

- Log: `8c4dce6692c0375f7b5c9a7f14177b57c365c164a629b0a44d668f4e6304b74a`.
- Williams distribuido: `d8cb7fd41863f0045251b963745944b2892ba296cdc0ba0bfc0027822a91e7a6`.
- MPA11 Murasama: `2877725f977a41755a256cec9880f6f3486df27d72dff2c11c533b57a1026966`.
- MPA12 Murasama: `c172fc66d9c4ee1f355b0551d37d78bc15daa3ad4c56edb69f6afe4046e1bddc`.
- MP1 Bavaria FW27: `d075f82f1e66eba8fad17f3f291f24fc7308301413ef5a88a24ec152d3403c6b`.

Los registros `Formula dynamics` se agrupan con las cuatro líneas `Formula wheel`, `Formula coupled force` y `Formula coupled tire` que les siguen. Los umbrales −0,15 y −0,95 son criterios de clasificación de esta revisión, no parámetros nuevos del simulador. El script y sus resultados se guardan junto a la evidencia para repetir la extracción.
