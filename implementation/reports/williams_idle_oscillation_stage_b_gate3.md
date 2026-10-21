# Oscilación en reposo del Williams: Etapa B, Puerta 3

Fecha: 20 de octubre de 2026. Rama: `codex/initial-port`. HEAD: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`.

Este informe cubre la integración del solver vertical conjunto en el coche completo y la validación neutral en Williams/Fuji. **La Etapa B no se declara cerrada**: el reposo está validado y la referencia no cambia, pero la conducción, los momentos de balanceo/cabeceo por fuerzas laterales/aero y la aceptación humana siguen pendientes. La referencia de Etapa A se conserva como respaldo; la distribución actual selecciona la ruta nueva para prueba manual, con los límites de aceptación indicados en este informe.

## 1. Resultado

- Ruta nueva explícita y temporal: `Formula Controls / vertical integration = coupled seven degree of freedom`.
- Valor predeterminado: `reference`. Ningún coche cambia de ruta por defecto.
- `SimUpdate` y `SimUpdateSingleCar` centralizan la integración vertical nueva una vez por paso.
- La ruta nueva no ejecuta `SimWheelUpdateRide`, `SimAxleUpdate` ni la parte vertical de `SimWheelUpdateForce`.
- Prueba neutral completa en Williams/Fuji con la ruta nueva: **estable**, sin pérdida de contacto y sin el patrón de cargas nulas.
- Momentos externos de roll/pitch inyectados en las fuerzas generalizadas: aero, alas, suelo, fuerzas horizontales de neumático y pares giroscópicos.
- Secuencia de conducción diagnóstica validada: aceleración, cambio 1→2, frenada hasta cero, marcha atrás y una fase breve de dirección.
- Regresión de la referencia: la DLL de producción y la ruta `reference` del árbol actual conservan el comportamiento de Etapa A.
- Todos los bancos existentes siguen pasando.
- DLL y XML de producción restaurados tras el ensayo: SHA-256 `B14FB36D...9927` y `66FD0A8D...A426`.
- La distribución quedó enrutada a la ruta nueva para prueba manual, con telemetría y controles normales.
- La DLL/XML/manifiesto de Etapa A se conservan respaldados y hay un script de reversión.

## 2. Integración

### 2.1 Selector

En `SimCarConfig` se lee, en `Formula Controls`:

    vertical integration = reference | coupled seven degree of freedom

El valor predeterminado es `reference`. La ruta nueva exige el neumático `transient coupled`; si no está habilitado, se registra un aviso y el vehículo permanece en `reference`. El selector es independiente del selector de neumáticos.

### 2.2 Orden por paso

En la ruta nueva:

1. `SimCarUpdateWheelPos`, frenos, aero y alas como en la referencia.
2. `SimWheelUpdateRoad` muestrea altura, malla, normal, fricción y resistencia de rodadura sin tocar `susp.x/v/a`.
3. `SimCarVerticalCoupledUpdate` construye la configuración desde `tCar` y resuelve el bloque conjunto.
4. Se publican pose/velocidades del chasis, `H`, `G`, `C`, `L`, `Ldot`, `d`, `ddot`, `N`, `S`, `susp.x/v/a`, `rideHeight`, `relPos.z` y estado de contacto.
5. `SimTireModelUpdateWheelWithVerticalState` actualiza la fuerza lateral/longitudinal, térmica y desgaste una vez por paso, usando la `N` y la deformación del solver.
6. `SimTransmissionUpdate`, `SimWheelUpdateRotation` y `SimCarUpdate` mantienen su orden.

`SimCarUpdateSpeed` y `SimCarUpdatePos` no integran `z`, `roll` ni `pitch` en la ruta nueva: esa integración la realiza el solver. La traslación horizontal, la guiñada, la transmisión, los frenos y el giro de rueda siguen su ruta habitual.

### 2.3 Inicialización

La primera llamada resuelve un equilibrio conjunto con gravedad, precarga nativa, contacto y geometría cargada mediante `TireVerticalSolverInitializeEquilibrium`. No se inicializa la velocidad nueva desde `susp.v`. El equilibrio se publica una vez y se registra en el log.

### 2.4 Subpasos y captura

El solver se ejecuta en subpasos de `vertical substep seconds` (0.002 s en el ensayo de 500 Hz). La telemetría vertical se registra por subpaso con `diagnostic vertical every step = 1`. La telemetría normal sigue desactivada por defecto.

### 2.5 Consumidores de fuerza

- `wheel->forces.z` es la fuerza normal `N` para el neumático, térmica, desgaste, HUD y telemetría.
- `wheel->susp.force` conserva la fuerza de suspensión `S` y las reacciones internas.
- El chasis no recibe `N` y `S` dos veces: la integración vertical de `SimCarUpdateSpeed/Pos` está desactivada en la ruta nueva.
- `TireContactModelUpdate` se llama con `elapsed_seconds = 0` y con la deformación/velocidad del solver, por lo que no integra el oscilador de carcasa. La deformación tiene una sola fuente.
- `wheel->axleFz` y `axleFz3rd` se publican a cero en la ruta nueva; las barras y el heave ya están incluidos en `S`. La telemetría de barras queda pendiente de desglose.

### 2.6 Momentos externos

La configuración del solver recibe tres fuerzas generalizadas externas:

    Q_ext[0] = suma(ala.z + aero.lift) + aero.floor_downforce
    Q_ext[1] = suma(neumatico.y * altura_contacto + wheel.torques.x)
    Q_ext[2] = -suma(neumatico.x * altura_contacto)
             + suma(wheel.torques.y)
             - suma(ala.z + aero.lift) * ala.x
             - suma(ala.x * ala.z)
             - aero.floor_downforce * (floor.x - GC.x)
             - aero.floor_drag * (floor.z - GC.z)

No incluyen gravedad, `N` ni `S`, porque el solver ya los aporta. Las fuerzas de aero/alas/suelo son del paso actual.

La ruta nueva evalúa la velocidad en el mismo punto de contacto que los momentos:

    h = q0 + statGC.z - G
    v_contacto_x = vx - yaw_rate*y - pitch_rate*h
    v_contacto_y = vy + yaw_rate*x + roll_rate*h

con `q0` la altura de referencia del chasis en el GC. La ruta `reference` conserva la velocidad anterior; la corrección se aplicó solo a la ruta nueva.

El acoplamiento horizontal ya no usa las fuerzas cometidas del cuadro anterior. Por cuadro:

1. Se calcula la velocidad de contacto y se evalúan fuerzas provisionales con una **copia** de `TireForceState`, la `N` anterior y el tiempo actual. No se comete ni se avanza térmica, desgaste o contacto.
2. Se resuelve un primer bloque vertical y se obtiene un estado predictor.
3. Con ese estado se recalcula la velocidad de contacto y una segunda fuerza provisional con la nueva `N`.
4. Se vuelve a resolver el bloque vertical desde el mismo estado anterior con el `Q_ext` corregido.
5. Se publica el estado final y se actualiza el neumático real una sola vez.

No se usa ninguna fuerza del cuadro anterior en `Q_ext`; queda un error de una iteración de punto fijo, no un retardo de cuadro. La memoria real del neumático se comete una vez.

## 3. Ensayo neutral Williams/Fuji

Condiciones:

- Williams FW19, Fuji, salida actual.
- Dirección, acelerador y frenos a cero; embrague desacoplado; marcha neutra. El juego mantiene el freno de prestart, por lo que el coche permanece inmóvil.
- Neumático `transient coupled`, presión de prueba 130 kPa manométricos.
- Misma masa/combustible, temperatura, desgaste, superficie y reglaje entre las tres variantes.
- Telemetría diagnóstica; 500 Hz verticales; se excluyen los primeros 2 s de física.
- La ruta nueva cubre 12.07 s de física; la referencia de producción cubre 132.8 s a 20 Hz y la referencia diagnóstica 12.34 s.

### 3.1 Ruta nueva acoplada

| Métrica | Valor |
| --- | ---: |
| Roll RMS | 0.0° (máximo medido en ventana corta `3.02e-5°`) |
| Carga delantera | 1336.590–1336.771 N |
| Carga trasera | 1667.002–1667.191 N |
| Fracción de pasos con carga < 1 N | 0 % |
| Contacto | 100 % |
| Residuo máximo del solver | `1.0e-7` |
| Iteraciones máximas de Newton | 2 |
| Restricciones activas | 0 |
| Energía mecánica | 200100.15–200100.41 J, sin crecimiento |
| `L` delantera / trasera | 0.0510766 / 0.0569490 m |
| `d` delantera / trasera | 0.0086007 / 0.0082138 m |
| `H` delantera / trasera | 33.253829 / 33.260072 m |
| `C` delantera / trasera | 33.514701 / 33.525139 m |

La carga media delantera es `1336.6 N` y la trasera `1667.1 N`, coherentes con el peso total. No aparece el patrón de aproximadamente un tercio de pasos descargados ni picos de 5–10 kN.

La referencia de Etapa A ya había medido a 500 Hz un rango delantero de `1270.3–1404.9 N` y un roll RMS de `0.000123°`. La ruta nueva, medida también a 500 Hz, queda en `1336.59–1336.77 N` y roll prácticamente nulo en la ventana de 10 s. Es una mejora fuerte del reposo, no una validación de conducción.

### 3.2 Referencia de producción (Etapa A)

| Métrica | Valor |
| --- | ---: |
| Roll RMS | 0.0001399° |
| Carga delantera | 1271.1–1394.3 N |
| Carga trasera | 1649.3–1679.9 N |
| Fracción de pasos con carga < 1 N | 0 % |

### 3.3 Referencia del árbol actual con selector `reference`

| Métrica | Valor |
| --- | ---: |
| Roll RMS | 0.0001177° |
| Carga delantera | 1271.5–1383.7 N |
| Carga trasera | 1649.8–1680.0 N |
| Fracción de pasos con carga < 1 N | 0 % |
| Líneas `Formula vertical step` | 0 |

La ruta `reference` del árbol actual reproduce la referencia de producción. La ruta nueva solo se ejecuta cuando el selector está activo.

## 4. Secuencia de conducción diagnóstica

Se añadieron mandos forzados solo en el XML diagnóstico. La secuencia fue:

| Tiempo de física | Mando |
| --- | --- |
| 0–1 s | Freno 1, neutro, embrague desacoplado |
| 1–3 s | Marcha 1, embrague 0, acelerador 0.12; dirección 0.05 entre 2.0 y 2.5 s |
| 3–4 s | Marcha 2, acelerador 0.12 |
| 4–6.5 s | Freno 1, embrague desacoplado |
| 6.5–7 s | Neutro, freno |
| 7–9.5 s | Marcha atrás, embrague 0, acelerador 0.10 |
| 9.5–10.5 s | Freno 1, embrague desacoplado |
| >10.5 s | Neutro, freno |

La dirección se aplica con la limitación de velocidad de `SimSteerUpdate`; el valor efectivo máximo fue `0.0157`, no `0.05`.

Resultados de la ruta nueva:

| Métrica | Valor |
| --- | ---: |
| Velocidad | `-4.099` a `+6.573 m/s` |
| Marchas observadas | `-1, 0, 1, 2` |
| Carga delantera | 177.8–2541.6 N |
| Carga trasera | 1044.7–2416.9 N |
| Fracción de pasos con carga < 1 N | 0 % |
| Contacto | 100 % |
| Roll | `-0.000378` a `0.000387 rad` |
| Pitch | `-0.01045` a `0.01408 rad` |
| Residuo máximo | `1.0e-7` |
| Iteraciones máximas | 2 |
| Restricciones activas | 0 |
| Daño | 0 |
| Fallos de paso | 0 |

Diferencia de carga izquierda-derecha en la fase de dirección: `-142.8` a `+139.8 N`.

La referencia con la misma secuencia:

| Métrica | Valor |
| --- | ---: |
| Velocidad | `-4.153` a `+6.560 m/s` |
| Marchas | `-1, 0, 1, 2` |
| Carga delantera | 0–3592 N |
| Carga trasera | 0–2470 N |
| Fracción de pasos con carga < 1 N | 0.52 % delantera; 0.13 % trasera |
| Roll | `-0.000388` a `0.000417 rad` |
| Pitch | `-0.01283` a `0.01691 rad` |
| Diferencia izquierda-derecha | `-166.7` a `+145.7 N` |

La ruta nueva reproduce la secuencia de velocidad, marchas, frenada y marcha atrás, mantiene el contacto en todos los pasos y limita los picos de carga (2.54 kN delante) frente a la referencia (3.59 kN). El pitch responde a aceleración y frenada, y el balanceo responde a la dirección, lo que confirma que los momentos externos llegan al bloque vertical.

### 4.1 Baches y pendiente

Se añadió una excitación diagnóstica de carretera, desactivada en la distribución, para reproducir baches con fase distinta por rueda. No sustituye a la última curva real de Fuji.

El primer ensayo, `0.02 m` a `6 Hz` con pendiente sintética del `5 %`, resultó demasiado severo: ambos acoplamientos perdieron contacto y volcaron (roll de hasta `±1.29 rad`, cargas de hasta `50 kN`). No se usa como criterio de aceptación.

Ensayo moderado, `0.005 m` a `5 Hz`, sin pendiente sintética:

| Variante | Roll RMS | Rango de roll | Pasos con carga < 1 N | Pico delantero |
| --- | ---: | ---: | ---: | ---: |
| Provisional simple, sin corrector | 5.81° | ±0.71 rad | 2.24 % / 3.61 % | 4.04 kN |
| Fuerza del cuadro anterior | 3.55° | ±0.21 rad | 1.55 % / 2.02 % | 3.46 kN |
| Provisional + una corrección | 3.80° | ±0.35 rad | 1.26 % / 1.69 % | 3.29 kN |

La corrección reduce claramente el caso provisional simple: menos roll, menos pasos descargados y menos pico de carga. Frente al acoplamiento anterior, reduce pasos descargados y picos, pero el roll RMS/rango queda algo mayor. El ensayo sintético sigue perdiendo contacto, así que **no se declara resuelto el rebote en baches**. La distribución lleva el corrector para que se pruebe en la última curva real.

## 5. Geometría y consumidores

En la ruta nueva se comprobó, a 500 Hz:

- `C = H + R - L` con `R` geométrico y `L` del solver.
- `d = L - (H - G)`, publicado como `max(d, 0)` para el neumático.
- `H` publicado en `wheel->pos.z`; `relPos.z = R - L`; `rideHeight = H - G`.
- `N` publicado en `wheel->forces.z`; `S` en `wheel->susp.force`.
- `susp.x = b*L`, `susp.v = -b*Ldot`, `susp.a = b*Laccel`.
- Contacto y estado de vuelo coherentes con `N` y `d`.
- La deformación del neumático usada por fuerza/térmica/desgaste es la del solver; no se ejecuta el oscilador de `TireContactModelUpdate`.

## 6. Límites

- **Conducción completa pendiente**: se validaron aceleración, cambio 1→2, frenada hasta cero, marcha atrás y una dirección suave. Quedan retención sostenida, cambios múltiples, pendientes, pianos, boxes/reinicio, curvas de alta velocidad y conducción sostenida.
- **Momentos externos**: se inyectan aero, alas, suelo, fuerzas horizontales del neumático y pares giroscópicos. El acoplamiento usa predictor más una corrección; no hay fuerza cometida del cuadro anterior, pero queda el error de una iteración de punto fijo. Falta validarlo a alta velocidad y con aero plenamente activo.
- **Baches**: el ensayo sintético moderado sigue perdiendo contacto y el severo vuelca; el corrector mejora al provisional simple, pero no se declara resuelto el rebote. La última curva real de Fuji sigue pendiente de prueba manual.
- **Aero**: la sustentación vertical y su momento de cabeceo ya forman parte de `Q_ext`, pero a velocidad no se ha comparado contra la referencia.
- **Suelo**: se usan alturas y velocidades por rueda; no se ha validado la normal local de Fuji, pianos, pendientes, curbs ni el trabajo de contacto inclinado. El overlay de baches es solo diagnóstico.
- **Ángulos pequeños**: la geometría del solver es linealizada; la publicación visual usa la misma linealización. La aproximación está validada a `±0.005 rad`, no en grandes inclinaciones.
- **Barras/heave**: la fuerza total `S` incluye barras y heave, pero la telemetría no desglosa `axleFz`/`axleFz3rd` en la ruta nueva.
- **Aceptación humana**: pendiente. Las pruebas automatizadas neutral y de conducción diagnóstica no la sustituyen.
- **Producción**: la distribución está enrutada a la ruta nueva solo para prueba manual. No es una aceptación; la DLL/XML/manifiesto de Etapa A están respaldados y se restauran con `restore_stage_a_distribution.ps1`.

## 7. Regresión y artefactos

Bancos existentes, todos con salida 0:

- `validate_formula_tire_geometry`: 144/144.
- `validate_formula_tire_vertical_system`: 208/208.
- `validate_formula_tire_selector`: 6/6.
- `validate_formula_tire_contact`: 13/13.
- `validate_formula_tire_forces`: 21/21.
- `validate_formula_tire_thermal`: 25/25.
- `validate_formula_tire_energy`: 27/27.
- `validate_formula_tire_vertical`: 10/10.
- `validate_williams_chassis`: 6/6.

Hashes:

- DLL de Etapa A respaldada: `B14FB36D0286DCE4FA6F0A53531C214BB911DE56224AD84F4A5658353DBC9927`.
- XML de Etapa A respaldado: `66FD0A8D05AF00C291C4647D0DDEC3A401B1D51423871AE49BA68B3CBCFDA426`.
- DLL diagnóstica neutral histórica: `BBAA5A6820C1BDFADD9614327F9C538C5284A86549ADFEA88E0CC7693E6F757B`.
- DLL diagnóstica de conducción histórica: `D72DBFDDFBCF1C1B7078712A9A0200EFB11D0E275F931B916386B88540D09E38`.
- DLL final con velocidad de contacto y predictor-corrector: `1BF9905B7D96101D44ACE50A0E680F09A9BE6F4FFED7B7FF2C7455B76EE896FB`.
- La distribución usa actualmente `1BF9905B...96FB` para la prueba manual, con respaldo de Etapa A.

## 8. Promoción para prueba

La distribución quedó enrutada a la ruta nueva:

    distribution/OpenFormula.exe
    Formula Controls / vertical integration = coupled seven degree of freedom

- DLL de distribución: `1BF9905B7D96101D44ACE50A0E680F09A9BE6F4FFED7B7FF2C7455B76EE896FB`.
- XML de distribución: `25149E890634A095A95D16DC09AA5EA8E0B7F3D70B239F93052697F3C68E6D6B`.
- Manifiesto de distribución: `8B270AD2DECB11463DE10A73C916255EA205FF9A73040E0C482FDB24EB275841`.
- Telemetría normal: desactivada.
- Controles diagnósticos: desactivados.
- Log vertical por paso: desactivado.

La comprobación de arranque `runtime-promotion-check.log` registra `SimuV5 vertical integration: coupled seven degree of freedom` y el equilibrio conjunto. El coche arranca con los controles normales del jugador.

Respaldo de Etapa A en `promotion-before/`:

- DLL: `B14FB36D0286DCE4FA6F0A53531C214BB911DE56224AD84F4A5658353DBC9927`.
- XML: `66FD0A8D05AF00C291C4647D0DDEC3A401B1D51423871AE49BA68B3CBCFDA426`.
- Manifiesto: copia íntegra anterior a la promoción.
- Reversión: `restore_stage_a_distribution.ps1`.

Esta promoción es para prueba manual, no una aceptación de conducción. Si el coche pierde apoyo, oscila o empeora frente a Etapa A, hay que restaurar el respaldo y registrar la medición.

## 9. Evidencias

En `D:/open-formula-migration-evidence/vertical-stage-b-gate3-20261020/`:

- `runtime-gate3-coupled-neutral-500hz-long.log`: ruta nueva, 500 Hz, 12.07 s.
- `runtime-gate3-coupled-neutral-production-reference.log`: DLL de producción, Etapa A.
- `runtime-gate3-coupled-neutral-diagnostic-reference.log`: árbol actual con selector `reference`.
- `runtime-gate3-coupled-neutral-driving-v3.log`: secuencia forzada, ruta nueva.
- `runtime-gate3-coupled-neutral-driving-reference-v3.log`: misma secuencia, selector `reference`.
- `runtime-gate3-coupled-neutral-final-neutral.log`, `runtime-gate3-coupled-neutral-final-driving.log`: código final.
- `runtime-gate3-coupled-neutral-bump3-provisional.log`, `runtime-gate3-coupled-neutral-bump3-previous.log`, `runtime-gate3-coupled-neutral-bump4-provisional.log`: ensayos de baches.
- `runtime-promotion-check-provisional.log`: arranque con la DLL final.
- `williams-gate3-diagnostic-500hz-long.xml`, `williams-gate3-diagnostic-diagnostic-reference.xml`.
- `williams-gate3-diagnostic-driving-v3.xml`, `williams-gate3-diagnostic-driving-reference-v3.xml`.
- `simuv5-gate3-500hz-long.dll`, `simuv5-gate3-diagnostic-reference.dll`, `simuv5-gate3-production-reference.dll`.
- `simuv5-gate3-driving-v3.dll`, `simuv5-gate3-driving-reference-v3.dll`.
- `gate3-validation-summary.txt`, `promotion-summary.txt`, `source-hashes-after.json`, `git-status-after.txt`, `git-diff-after.patch`.
- `promotion-before/` y `promotion-after/`: DLL/XML/manifiesto antes y después de la promoción.
- `run_gate3_neutral.ps1`: ensayo reproducible con respaldo y restauración.

No se hizo commit ni push.
