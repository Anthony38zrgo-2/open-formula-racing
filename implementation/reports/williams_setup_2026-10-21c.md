# Williams: setup de frenada y estabilidad del 21 de octubre de 2026 (tercera revisión)

Solicitud: aplicar el paquete recomendado tras el análisis del registro de la última sesión, donde el impacto contra el muro ocurrió 17 ms después de una reducción 3ª→2ª y con el coche cruzado.

## Cambios aplicados

| Parámetro | Antes | Ahora |
| --- | ---: | ---: |
| Freno motor, coeficiente lineal | 0,03 | **0,015** |
| Freno motor, coeficiente constante | 0,0525 | **0,026** |
| Diferencial en retención | 0,30 | **0,15** |
| Reparto de frenada | 64 % del. | **65 % del.** |
| Coeficiente de fricción del freno trasero | 0,45 | **0,42** |
| Convergencia trasera | 0,2° por rueda | **0,3° por rueda** (0,6° total) |
| Respuesta de dirección por velocidad | 0,012 | **0,020** por m/s |
| Ride trasero | 45 mm | **50 mm** |
| Recorrido trasero / packers | 135 / 15 mm | **140 / 20 mm** |
| Barra estabilizadora trasera | 80.000 N/m | **65.000 N/m** |
| Telemetría diagnóstica | 0 | **1** |

El ride delantero se mantiene en 50 mm. El coche queda a nivel, sin el rake negativo de 5 mm de la revisión anterior. El generador recalcula automáticamente `suspension course` y `packers` a partir del nuevo ride.

## Justificación

- **Freno motor:** el registro muestra el impacto inmediatamente después de la reducción 3ª→2ª. A 1,5 g el freno hidráulico trasero al 70 % de pedal ronda 2,5 kN por rueda; el freno motor anterior añadía aproximadamente 0,9–1,6 kN por rueda en 3ª–5ª. La reducción a la mitad devuelve el conjunto por debajo de la capacidad trasera en la mayor parte del rango.
- **Diferencial en retención:** bajar de 0,30 a 0,15 reduce el acoplamiento entre las ruedas traseras; si una empieza a bloquear, no arrastra a la otra.
- **Reparto y fricción trasera:** el reparto ya estaba adelantado; 0,65 y un `mu` trasero de 0,42 recortan el par hidráulico trasero sin desplazar todavía más carga al eje delantero.
- **Convergencia y dirección:** más convergencia trasera y una respuesta más lenta a alta velocidad mejoran la estabilidad en recta y reducen la agresividad de las entradas.
- **Ride trasero:** subir a 50 mm elimina el rake negativo y acerca el fondo a su ventana de funcionamiento, además de aumentar la holgura.
- **Barra trasera:** 65.000 N/m suaviza el eje trasero y aumenta su agarre relativo. Es un cambio de balance; si el coche queda subvirador, debe revertirse a 80.000 N/m.
- **Telemetría:** se activa para la próxima tanda y poder confirmar el bloqueo con `wheel_speed`, `load`, `brake_torque` y `utilization`. Debe volver a 0 después de la prueba.

## Archivos modificados

- `tools/prepare_williams_vehicle.py`: nuevos valores por defecto y opciones `--rear-brake-mu` y `--diagnostic-telemetry`.
- `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- `implementation/candidates/williams-nineteen-ninety-seven/williams_generation_report.json`
- `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`
- `distribution/share/games/speed-dreams-2/cars/models/williams-nineteen-ninety-seven/williams_generation_report.json`
- `distribution/distribution_manifest.json`
- `implementation/migration_status.json`
- `tools/validate_formula_differential.cpp`: la comprobación de retención ya no depende de los valores concretos de la revisión anterior; verifica la dirección del acoplamiento con 0, 0,15 y 0,30.

## Verificación

- XML candidato y distribuido idénticos: SHA-256 `D8CB7FD41863F0045251B963745944B2892BA296CDC0BA0BFC0027822A91E7A6`.
- Informe de generación distribuido: SHA-256 `AE738B8DEA629933889C9431990E161E0D11B02095353939E8DB4C7E001AFD4E`.
- Manifiesto de distribución: SHA-256 `8322CB453B3B7A9A63D355717F6B6CCEE0F25F762904D087616753ED14B65FB8`.
- `python -m py_compile tools/prepare_williams_vehicle.py`: correcto.
- `prepare_williams_vehicle.py` regeneró el vehículo; los recursos gráficos y de audio quedaron idénticos al candidato anterior (solo cambiaron XML e informe de generación).
- Bancos nativos ejecutados con el XML distribuido: `validate_williams_chassis`, `validate_formula_aerodynamics`, `validate_formula_tire_geometry`, `validate_formula_tire_contact`, `validate_formula_tire_forces`, `validate_formula_tire_energy`, `validate_formula_tire_thermal`, `validate_formula_tire_selector`, `validate_formula_tire_vertical_system`, `validate_formula_vertical_authority`, `validate_rigid_contact_safety` y `validate_formula_differential`: salida 0.
- El banco de diferencial se reconstruyó tras adaptar su expectativa: con 0,15 la diferencia de velocidad es 9,90305; sin retención 10; con 0,30 9,88612; potencia 9,92204. PASS.

## Límites y pendientes

- Es un cambio de datos; no se recompiló ni reinstaló `simuv5.dll`.
- No se ha conducido con este setup. La validación de frenada, estabilidad, pianos, rasantes y alta velocidad sigue pendiente.
- La barra trasera a 65.000 N/m es un cambio de balance condicional. Si aparece subviraje, revertir a 80.000 N/m antes de tocar otros parámetros.
- La telemetría está activada en el XML distribuido; hay que volver a poner `diagnostic telemetry = 0` cuando termine la sesión de medida.
- El análisis del accidente se basa en el registro de eventos, no en `wheel_speed`, porque la sesión anterior no tenía telemetría de ruedas.
