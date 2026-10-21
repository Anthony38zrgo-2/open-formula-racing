# Williams: setup del 21 de octubre de 2026

> Actualización posterior: los valores de este documento fueron reemplazados por los de `williams_setup_2026-10-21b.md` (80 °C, reparto 64 %, freno motor −25 %, diferencial 10/30, ride trasero 45 mm y dirección dependiente de la velocidad).

Solicitud: elevar el ride trasero a 60 mm, temperatura inicial de 60 °C en las cuatro ruedas, convergencia positiva de 0,2° en las ruedas traseras, presión de neumáticos a 100 kPa, reducción del 20 % en la presión de frenos, diferencial en retención a 0,50, camber delantero a −3,0° y trasero a −1,0°.

## Cambios aplicados

| Parámetro | Antes | Ahora |
| --- | --- | --- |
| Ride height delantero | 50 mm | 50 mm (sin cambio) |
| Ride height trasero | 50 mm | **60 mm** |
| Recorrido de suspensión trasero | 140 mm | 150 mm |
| Packers traseros | 20 mm | 30 mm |
| Temperatura inicial, neumático heredado | 30 °C | **60 °C** |
| Temperatura inicial, modelo acoplado | 25 °C (298,15 K) | **60 °C (333,15 K)** |
| Convergencia trasera | 0° | **+0,2° derecha / −0,2° izquierda** |
| Presión en frío, modelo acoplado | 130 kPa (manómetro) | **100 kPa (manómetro)** |
| Presión absoluta por rueda | 1.103.161 Pa | **201.325 Pa** |
| Presión máxima de frenos | 25 MPa | **20 MPa** |
| Bloqueo en retención | 0,70 | **0,50** |
| Camber delantero | −4,0° | **−3,0°** |
| Camber trasero | −1,5° | **−1,0°** |

El bloqueo en aceleración del diferencial se conserva en 0,30 y la precarga en 10 N·m.

## Interpretación de los cambios

- **Ride trasero:** se elevó la referencia de los muelles traseros de 50 a 60 mm. Para conservar la ventana de recorrido diseñada (30 mm de compresión y 90 mm de extensión), el `suspension course` pasó de 0,14 a 0,15 m y los `packers` de 0,02 a 0,03 m. El eje delantero y la altura del centro de gravedad del XML no se modificaron; el coche queda con 10 mm más de rake trasero.
- **Temperatura inicial:** se ajustaron tanto el campo heredado de cada rueda (30 → 60 °C) como la temperatura inicial del modelo acoplado (298,15 → 333,15 K). Coincide con la temperatura de referencia en frío del desgaste (333,15 K), por lo que el multiplicador de desgaste por frío arranca en 1,0.
- **Convergencia trasera:** el XML usa un ángulo por rueda en radianes. Siguiendo la convención de los coches nativos (por ejemplo, `sc-cavallo-360`: derecha +0,1°, izquierda −0,1°), la convergencia simétrica se aplicó como **+0,2° en la rueda derecha y −0,2° en la izquierda**, es decir, 0,2° por rueda y 0,4° total. Si la intención era 0,2° total, deben usarse ±0,1°.
- **Presión de neumáticos:** 100 kPa manométricos. El campo `pressure` de cada rueda es absoluto en simuv5, por eso se guardó 201.325 Pa (100.000 + 101.325). Las referencias de presión caliente (160/150 kPa) no se cambiaron.
- **Frenos:** se redujo la presión máxima del sistema de 25 a 20 MPa sin cambiar el área de pistón. La presión hidráulica y el par máximo de frenado a pedal completo quedan aproximadamente un 20 % por debajo del valor anterior.
- **Camber:** los valores negativos se conservan; solo cambió su magnitud.

## Archivos modificados

- `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- `tools/prepare_williams_vehicle.py` — nuevas opciones `--rear-ride-height`, `--tire-initial-temperature`, `--tire-pressure-kpa-gauge`, `--rear-toe-degrees`, `--front-camber-degrees`, `--rear-camber-degrees`, `--brake-max-pressure` y `--coast-slip-bias`, con los nuevos valores como predeterminados.
- `implementation/candidates/williams-nineteen-ninety-seven/williams_generation_report.json`
- `distribution/distribution_manifest.json`
- `implementation/migration_status.json`

## Verificación y límites

- El XML candidato y el distribuido son idénticos: SHA-256 `BED31AADA49A33314BC5EAB00A9C4771807FCA69B56C20ABB2A9824243F52363`.
- El XML parsea correctamente y `tools/prepare_williams_vehicle.py` compila (`py_compile`).
- Es un cambio de datos: no se recompiló ni reinstaló `simuv5.dll` como parte de este cambio de setup.
- **No revalidado.** Las pruebas de la Etapa B (geometría, coche neutro, conducción diagnóstica), las comparaciones térmicas, la aerodinámica, el diferencial y los frenos se ejecutaron con el setup anterior. El cambio de altura, presión, temperatura, convergencia, camber y par de frenado altera la carga estática, el contacto, la térmica y el equilibrio, por lo que esas evidencias no se trasladan automáticamente al setup actual.
- La aceptación humana de alta velocidad registrada el 21 de octubre de 2026 corresponde al setup anterior. Con este setup siguen sin revalidarse la conducción sostenida a alta velocidad, pianos, pendientes, pits, lluvia y vuelta completa.

## Nota posterior: experimento de gravedad

Se probó temporalmente `G = 18.0f` y se revirtió a `9.80665f`; la DLL distribuida se reconstruyó e instaló con el valor normal (`simuv5.dll` SHA-256 `93D10953…214B630E`). El experimento no está validado y no cambia los valores de setup de este documento. El G-meter, los diagnósticos de simuv5 y los pilotos de IA siguen asumiendo 9,81 m/s².
