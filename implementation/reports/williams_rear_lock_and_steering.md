# Williams: bloqueo trasero en frenada y suavizado de dirección

> Actualización: este informe describe el setup anterior (60 °C, 59 % de reparto, freno motor original). Los cambios aplicados después están en `williams_setup_2026-10-21b.md`: 80 °C, 64 % de reparto, freno motor −25 %, diferencial 10/30, ride trasero 45 mm y dirección dependiente de la velocidad.

Fecha: 21 de octubre de 2026. La gravedad experimental se revirtió a `9.80665f` y `simuv5.dll` se reconstruyó e instaló antes de este análisis.

## 1. Bloqueo del tren trasero

### 1.1 Reparto de frenada actual

El XML usa `front-rear brake repartition = 0.59`; el código lo aplica correctamente:

- `src/modules/simu/simuv5/brake.cpp:206-209`: front = `ctrl * coeff * rep`, rear = `ctrl * coeff * (1 - rep)`.
- `brake.cpp:51`: `coeff = radio_disco * área_pistón * mu`.
- `brake.cpp:60`: `Tq = coeff * presión`.

Con los valores actuales:

| Dato | Delantero | Trasero |
| --- | ---: | ---: |
| Presión de línea a pedal completo | 11,8 MPa | 8,2 MPa |
| Reparto | 59 % | 41 % |
| Par por rueda a pedal completo | 1.888 N·m | 1.312 N·m |
| Fuerza de frenado por rueda | 6.053 N | 4.075 N |
| Carga estática por rueda | 1.337 N | 1.667 N |
| Capacidad de neumático a 60 °C (μ ≈ 2,34) | 3.124 N | 3.896 N |
| Umbral de pedal para bloquear en estático | 52 % | 96 % |

La conclusión inicial es que el trasero no está “mal cableado”: a temperatura inicial de 60 °C, el par de freno trasero a pedal completo supera en ~5 % la capacidad del neumático trasero en estático; el umbral de bloqueo es ≈96 % del pedal. El delantero también está sobrefrenado (194 % de su capacidad), por lo que a poco pedal bloquea primero el delantero.

### 1.2 Transferencia de carga

Al frenar, la carga se desplaza hacia delante. Con masa total ≈ 612,6 kg, altura de CG ≈ 0,295 m y batalla 2,89 m:

| Desaceleración | Carga del. / rueda | Carga tras. / rueda | Umbral pedal del. | Umbral pedal tras. | Bloquea primero |
| ---: | ---: | ---: | ---: | ---: | --- |
| 0 g | 1.337 N | 1.667 N | 52 % | 96 % | Delantero |
| 1,0 g | 1.643 N | 1.361 N | 62 % | 79 % | Delantero |
| 1,5 g | 1.797 N | 1.207 N | 68 % | 71 % | Delantero |
| 1,75 g | 1.873 N | 1.131 N | 70 % | 67 % | **Trasero** |
| 2,0 g | 1.950 N | 1.054 N | 73 % | 63 % | **Trasero** |

Es decir, por encima de aproximadamente **1,5–1,6 g** el eje trasero tiene menos margen que el delantero. El coche puede superar ese valor con carga aerodinámica, así que el bloqueo trasero es esperable en frenadas fuertes, aunque el reparto sea 59/41.

### 1.3 Freno motor y diferencial

El freno motor añade par negativo al eje trasero cuando el embrague está acoplado (`engine.cpp:212-268`). A cero acelerador:

- `EngBrkK = brakeLinCoeff * rads` (0,04 × rad/s).
- `Tq = -EngBrkK - brakeCoeff` (27,3 N·m).
- A 5.ª velocidad y ~13.800 rpm, el par negativo es ≈ 85 N·m en el motor; con relación 1,441 × 4,65 y rendimiento 0,98, unos **−279 N·m por rueda trasera**.
- En 2.ª velocidad puede superar **−400 N·m por rueda**.

Eso equivale a sumar entre 850 y 1.300 N de fuerza de frenado por rueda trasera. Con ese aporte, el umbral de bloqueo trasero puede bajar a aproximadamente **0,7–1,0 g** según marcha y rpm; en marchas cortas el freno motor pesa más que el propio reparto de frenada.

El diferencial 1.5-way con `coast max slip bias = 0.50` y `restoring coast lock = yes` acopla las dos ruedas traseras en retención. No crea el par de freno, pero cuando una rueda empieza a bloquearse arrastra a la otra y hace que el bloqueo sea más estable y difícil de recuperar. El auto-clutch del piloto humano (`rthumandriver.cpp:1646-1659`) da un golpe de gas en los cambios para reducir el pico, pero no elimina el freno motor sostenido.

### 1.4 Papel del setup actual

- **Temperatura inicial 60 °C:** el modelo acoplado usa una ventana óptima de 89–101 °C (95 °C ± 6 °C). A 60 °C el multiplicador térmico es ≈ 0,866, es decir, ~13 % menos de μ que en óptimo. Eso reduce el margen trasero justo cuando el freno ya está cerca del límite. El campo heredado `optimal temperature = 80 °C` no es el que usa esta ruta.
- **Presión 100 kPa:** el modelo la interpreta como presión de referencia en frío a 25 °C; a 60 °C la presión de arranque real es ≈ 224.958 Pa absolutos (≈ 123,6 kPa manométricos). No es la causa principal, pero cambia la rigidez y el calentamiento.
- **Ride trasero 60 mm:** aumenta ligeramente el rake y la altura del CG, lo que incrementa un poco la transferencia de carga. Efecto secundario frente al reparto de frenada.
- **Camber trasero −1,0° y convergencia +0,2°:** cambios pequeños de contacto y arrastre; no explican por sí solos el bloqueo.
- **Sin ABS:** el coche depende por completo de la modulación del pedal.

### 1.5 Hallazgo de configuración importante

El reparto de frenada **no se puede ajustar en el menú de setup** porque el XML no declara `min`/`max`:

```xml
<attnum name="front-rear brake repartition" val="0.59" />
```

`GfParmGetNumWithLimits` (`src/libs/tgf/params.cpp:855-867`) usa `min = max = val` cuando faltan esos atributos. Lo mismo ocurre con ride height, camber, toe, presión y presión máxima de frenos. Si se quiere probar otro reparto desde el juego, hay que añadir rangos, por ejemplo:

```xml
<attnum name="front-rear brake repartition" val="0.59" min="0.55" max="0.67" />
```

### 1.6 Recomendaciones para el bloqueo trasero

1. **Confirmar con telemetría:** activar `Formula Controls/diagnostic telemetry = 1` y registrar `Formula wheel` (wheel_speed, load) y `Formula wheel power` (brake_torque) en una frenada fuerte. Si la rueda trasera cae a ~0 rad/s mientras la delantera sigue girando, es bloqueo confirmado.
2. **Separar freno motor:** repetir la frenada con el embrague desacoplado. Si el bloqueo desaparece o mejora mucho, el problema principal es el freno motor/downshifts, no el reparto.
3. **Probar reparto más adelantado:** 0,61–0,64. Con 0,63 el umbral de bloqueo trasero sube y se acerca al del delantero; el coste es más subviraje al frenar.
4. **Probar retención del diferencial más baja:** 0,30–0,40 para reducir el acoplamiento entre ruedas traseras.
5. **Añadir rangos `min`/`max`** al reparto y a la presión de frenos para poder ajustarlos sin editar XML.
6. **No interpretar el cambio de temperatura/presión como cura:** a 60 °C el trasero está en el límite por diseño; si se quiere margen, subir la temperatura inicial o aligerar el freno trasero.

## 2. Suavizado de dirección según velocidad

### 2.1 Qué se aplica

El control del piloto humano pasa por `applyFormulaSteeringControls` (`src/libs/robottools/formula_gear_controls.cpp:112-150`):

```cpp
if (!keyboard_left && !keyboard_right)
    steering_target /= 1.0 + speed_sensitivity * std::abs(car->_speed_x);
```

Con `steering speed sensitivity = 0.018`:

| Velocidad | Factor | Comando máximo |
| ---: | ---: | ---: |
| 0 km/h | 1,00 | 100 % |
| 100 km/h | 1,50 | 66,7 % |
| 200 km/h | 2,00 | 50,0 % |
| 300 km/h | 2,50 | 40,0 % |

Por tanto, **sí hay una reducción de ganancia dependiente de la velocidad**, pero solo para el mando (Xbox/stick). El teclado queda fuera:

```cpp
if (keyboard_left || keyboard_right)
    state.previous_steering = steering_target;   // sin filtro ni escala
```

Si se usan las flechas del teclado, no se aplica ni la reducción por velocidad ni el suavizado.

### 2.2 El ritmo no depende de la velocidad

Los límites son fijos:

- aplicación: 1,5 por segundo
- inversión: 2 por segundo
- centrado: 3 por segundo
- respuesta exponencial: 0,16 s

La velocidad reduce el **objetivo**, no la **velocidad de movimiento**. Como el objetivo es más pequeño, el filtro lo alcanza antes. Simulación del filtro actual para un stick a fondo:

| Velocidad | Objetivo | Tiempo hasta 95 % |
| ---: | ---: | ---: |
| 0 km/h | 1,000 | 0,758 s |
| 100 km/h | 0,667 | 0,602 s |
| 200 km/h | 0,500 | 0,536 s |
| 300 km/h | 0,400 | 0,506 s |

Es decir: a 300 km/h el volante llega a su posición reducida **más rápido** que a baja velocidad. La dirección está limitada en amplitud, pero no se vuelve más lenta ni más “pesada” con la velocidad. Eso explica la sensación de que a alta velocidad sigue siendo fácil moverla.

Además, `steer.cpp:78-80` aplica un límite de velocidad de giro constante (`max steer speed = 8 rad/s`) y `steer lock = 0.314 rad`; ninguno depende de la velocidad.

### 2.3 Recomendaciones para la dirección

1. **Si se usa teclado:** aplicar también la escala de velocidad y el filtro al teclado, o al menos la escala. Hoy el teclado es instantáneo y a fondo.
2. **Aumentar la sensibilidad:** subir `steering speed sensitivity` de 0,018 a 0,03–0,04 reduce más el comando a alta velocidad (0,30–0,25 a 300 km/h).
3. **Hacer dependiente el ritmo:** escalar `application_rate` o `response_seconds` con la velocidad, por ejemplo `response_seconds * (1 + k * |v|)`, para que el volante tarde más en moverse a alta velocidad. Eso es lo que produce la sensación de “más pesado”, no solo un menor ángulo.
4. **Mantener el límite de rack** (`max steer speed`) como protección, pero no es el factor dominante porque el filtro ya es más lento.

## 3. Resumen

- El reparto 59/41 se aplica correctamente; no hay una inversión ni un error de eje.
- El trasero supera ligeramente la capacidad del neumático a 60 °C a pedal completo incluso en estático; la transferencia de carga y el freno motor lo llevan a bloquear primero por encima de ~1,5–1,6 g sin retención, y a ~0,7–1,0 g con retención en marchas cortas.
- El reparto de frenada está fijado en el XML: sin `min`/`max`, el menú de setup no permite ajustarlo.
- La dirección sí tiene reducción de ganancia por velocidad para el mando, pero no para el teclado; el ritmo es fijo y a alta velocidad el objetivo reducido se alcanza más rápido.
- Ningún cambio de código o setup se aplicó en esta investigación; solo se revirtió la gravedad y se reconstruyó/instaló la DLL normal.
