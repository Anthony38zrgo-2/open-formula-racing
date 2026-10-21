# Williams: setup y dirección del 21 de octubre de 2026 (segunda revisión)

Solicitud: neumáticos a 80 °C al entrar en pista, reparto de frenada 64 % delantero, 25 % menos de freno motor en coast, diferencial 10 % en aceleración y 30 % en retención, ride trasero a 45 mm y suavizado de dirección dependiente de la velocidad.

## Cambios aplicados

| Parámetro | Antes | Ahora |
| --- | ---: | ---: |
| Temperatura inicial de neumáticos | 60 °C | **80 °C** |
| Reparto de frenada | 59 % del. | **64 % del.** |
| Freno motor, coeficiente lineal | 0,04 | **0,03** |
| Freno motor, coeficiente constante | 0,07 | **0,0525** |
| Diferencial en aceleración | 0,30 | **0,10** |
| Diferencial en retención | 0,50 | **0,30** |
| Ride trasero | 60 mm | **45 mm** |
| Recorrido trasero / packers | 150 / 30 mm | **135 / 15 mm** |
| Respuesta de dirección por velocidad | no existía | **0,012 por m/s** |

El reparto de frenada ahora tiene rango ajustable en el XML: `min = 0.55`, `max = 0.70`. Sin esos atributos, el menú de setup lo trataba como valor fijo.

## Efectos y notas

- **Neumáticos a 80 °C:** se ajustaron tanto el campo heredado de cada rueda como la temperatura inicial del modelo acoplado (353,15 K). El modelo usa 100 kPa como presión de referencia a 25 °C; al arrancar a 80 °C la presión de gas será aproximadamente 238,5 kPa absolutos (≈ 137,1 kPa manométricos), no 100 kPa. Es el comportamiento físico del gas; si se quisieran 100 kPa reales a 80 °C habría que bajar la referencia en frío.
- **Reparto 64 %:** desplaza par de freno al eje delantero y reduce el riesgo de bloqueo trasero, a costa de algo más de subviraje en frenada. El rango 0,55–0,70 queda disponible para ajustar en pista. Con 64 % y 80 °C, el umbral teórico de bloqueo trasero en estático sube a ~121 % del pedal (no bloquea a pedal completo); con freno motor en 5.ª baja a ~102 %, y el cruce con el delantero se mueve a ~1,6–1,7 g. En marchas cortas el freno motor todavía puede adelantar el bloqueo.
- **Freno motor −25 %:** se redujeron ambos coeficientes del `Engine` al 75 % (0,04 → 0,03 y 0,07 → 0,0525). Es un cambio de datos, limitado al Williams; no afecta a otros coches ni al código compartido. El freno motor en 5.ª baja de ~279 a ~209 N·m por rueda.
- **Diferencial 10/30:** menos acoplamiento entre ruedas traseras en aceleración y retención. Se conservan la precarga de 10 N·m, el par de bloqueo de 90 N·m y `restoring coast lock = yes`.
- **Ride trasero 45 mm:** se bajó la referencia 15 mm respecto a los 60 mm anteriores. Para conservar la ventana de 30 mm de compresión y 90 mm de extensión, el recorrido trasero pasó a 135 mm y los packers a 15 mm. El coche queda con el eje trasero 5 mm más bajo que el delantero (rake negativo); conviene vigilar el bottoming en pianos y frenadas fuertes.
- **Dirección:** se añadió `steering speed response = 0.012` por m/s. El factor `1 + 0.012·|v|` alarga la respuesta y reduce las velocidades de aplicación, inversión y centrado. A 300 km/h el factor es 2,0: la respuesta pasa de 0,16 a 0,32 s y la aplicación de 1,5 a 0,75 por segundo. El teclado sigue siendo instantáneo por debajo de 5 m/s; por encima usa el mismo filtro con objetivo completo.

Tiempo hasta el 95 % del objetivo con el nuevo filtro (mando):

| Velocidad | Objetivo | Tiempo |
| ---: | ---: | ---: |
| 0 km/h | 1,000 | 0,758 s |
| 100 km/h | 0,667 | 0,802 s |
| 200 km/h | 0,500 | 0,894 s |
| 300 km/h | 0,400 | 1,010 s |

Ahora la dirección tarda más a alta velocidad, en lugar de alcanzar antes el objetivo reducido como ocurría con el filtro anterior.

## Archivos modificados

- `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- `tools/prepare_williams_vehicle.py` — nuevas opciones `--front-brake-bias`, `--power-slip-bias`, `--engine-brake-linear-coefficient`, `--engine-brake-constant-coefficient` y `--steering-speed-response`, con los nuevos valores por defecto.
- `src/libs/robottools/formula_gear_controls.cpp` — respuesta y ritmo de dirección dependientes de la velocidad.
- `implementation/candidates/williams-nineteen-ninety-seven/williams_generation_report.json`
- `distribution/distribution_manifest.json`
- `implementation/migration_status.json`

## Verificación

- XML candidato y distribuido idénticos: SHA-256 `0B3AB7A103EF18A4DDDD56081B67B3AC1BA93C499883B2B5D671F9EB46F8CA1C`.
- `tools/prepare_williams_vehicle.py` compila (`py_compile`).
- `simuv5` y `librobottools` se recompilaron; DLLs instaladas:
  - `simuv5.dll`: `F72F5624…736FF0C6`
  - `librobottools.dll`: `E051DAC9…FBFBB4DB`
- No se revalidó en pista. Las pruebas de la Etapa B, térmica, frenada y aerodinámica se hicieron con el setup anterior; este cambio de temperatura, reparto, freno motor, diferencial y altura altera el equilibrio y requiere prueba humana.
