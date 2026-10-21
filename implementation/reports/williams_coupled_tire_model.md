# Modelo acoplado de neumáticos Williams FW19

Fecha: 20 de octubre de 2026.

## 1. Alcance y resultado

Se añadió un modelo nativo seleccionable por vehículo dentro de simuv5 y se activó únicamente en el Williams/Fuji. El resto de vehículos conserva el camino legacy. La implementación cubre fuerzas puras y combinadas, carcasa radial en serie con la suspensión, presión absoluta, red térmica de cinco masas, desgaste por zona y telemetría. No se modificó `D:/Formula90s`, no se añadió Godot ni una DLL Rust y no se cambiaron aero, diferencial, geometría, masas, suspensión ni controles para compensar el neumático.

La selección se persiste en `Formula Controls/tire model = transient coupled`. La configuración física vive en `Formula Tires`, con bloques `Front/Rear Force`, `Front/Rear Contact`, `Front/Rear Thermal` y `Front/Rear Wear`. El esquema se versiona con `schema version = 1`.

## 2. Procedencia y contratos

La referencia de ecuaciones fue la revisión local de Formula90s ya documentada en el handoff: `tire.rs`, `wheel_mechanics.rs`, `tire_thermals.rs`, `tire_wear.rs`, el esquema de configuración y el perfil `hockenheim/tires.json`. Se trasladaron contratos y ecuaciones, no una dependencia binaria. La equivalencia numérica con Rust no se declara: la integración, los signos de simuv5, la resistencia a la rodadura existente y el contacto con la malla de Fuji son propios de este destino.

Convenciones internas: metros, segundos, kilogramos, radianes, rad/s, N, N·m, J, W, Pa absolutos y K. La frontera de configuración usa Pa para presión y K para temperatura absoluta; la interfaz muestra presión manométrica.

Presión adoptada para las pruebas: **130 kPa manométricos en las cuatro ruedas**, con `ambient pressure = 101325 Pa`, `reference temperature = 298.15 K` y presión absoluta inicial de **231325 Pa**. Las referencias calientes son 160/160/150/150 kPa manométricos delante/detrás. El valor legacy de 160 psi del XML no se reutiliza para el modelo nuevo ni se reinterpreta en silencio; queda como caso extremo de robustez en los bancos. La presión legacy del neumático (`1103161.16691 Pa`) se conserva en las secciones de rueda para no alterar el camino legacy.

## 3. Archivos

Módulos nuevos:

- `src/modules/simu/simuv5/tire_model_configuration.h/.cpp`: esquema, validación y conversión de frontera.
- `src/modules/simu/simuv5/tire_model_state.h`: estado POD de tamaño fijo por rueda.
- `src/modules/simu/simuv5/tire_force_model.h/.cpp`: curvas puras, presupuesto combinado, carga, camber, relajación, migración de picos y par autoalineante.
- `src/modules/simu/simuv5/tire_contact_model.h/.cpp`: carcasa radial, amortiguación, radio efectivo, parche y sensibilidad a carga.
- `src/modules/simu/simuv5/tire_thermal_model.h/.cpp`: red de banda/carcasa/gas, presión por gas ideal y modificadores mecánicos.
- `src/modules/simu/simuv5/tire_wear_model.h/.cpp`: desgaste por zona, multiplicadores térmicos y grip.
- `src/modules/simu/simuv5/tire_model.cpp`: configuración, reset, publicación y actualización por rueda.

Integración:

- `wheel.h`, `wheel.cpp`: estado, selector, reset, camino nuevo y uso de la deflexión de carcasa en la altura efectiva de la suspensión.
- `carstruct.h`, `sim.h`, `car.cpp`: configuración por coche, telemetría de diagnóstico y orden de configuración.
- `simu.cpp`: reset de neumático nuevo en boxes.
- `CMakeLists.txt` y `src/modules/simu/simuv5/CMakeLists.txt`: fuentes y bancos nuevos.
- `tools/prepare_williams_vehicle.py`: generación persistente del selector y de todos los parámetros.
- `src/modules/graphic/ssggraph/grboard.cpp` y `src/modules/graphic/osggraph/Viewer/OsgHUD.cpp`: presión mostrada como manométrica.
- `tools/validate_formula_tire_forces.cpp`, `validate_formula_tire_contact.cpp`, `validate_formula_tire_thermal.cpp`: bancos nuevos.
- `tools/validate_formula_tire_energy.cpp`: se conserva como regresión legacy; solo se enlazaron los módulos nuevos para resolver símbolos.

## 4. Decisiones físicas

Fuerzas: curvas puras independientes con pico lateral 0,115 rad, pico longitudinal 0,12, meseta 0,88 y caída progresiva. Presupuesto único con proyección elíptica C1 sobre demanda normalizada; la frenada consume presupuesto y no multiplica mu. Sensibilidad a carga `(Fz/Fz_ref)^(0,92-1)` con referencia por rueda; a carga cero la fuerza es cero. La fricción de compuesto (2,7) y el multiplicador de superficie se separan; la superficie de Fuji aporta 1,0 en asfalto. Camber se espeja por lado para que un camber simétrico cancele el empuje lateral en recta. La relajación es por distancia con tiempo mínimo y recuperación asimétrica; puede desactivarse por configuración. La migración de picos se puede aislar y está activada en el perfil de pruebas.

Contacto: rigidez radial derivada de la carga estática y 8 mm de deflexión de referencia; amortiguación con razón 0,45 y escala `sqrt(k)` al cambiar presión; deflexión no negativa, límite físico y contador de clamp. La deflexión se introduce como compliance en serie con la suspensión: la altura efectiva de carretera es `Zroad - deflexión`, de modo que la carcasa comprimida reduce la compresión de suspensión en lugar de realimentarla. El radio efectivo resta una fracción de la deflexión y el parche crece con carga/deflexión. La fuerza normal que alimenta fricción, calor y desgaste es la reacción de suspensión aceptada en el subpaso.

Térmica: cinco masas (banda interior/central/exterior, carcasa, gas), capacidades positivas y conductancias entre banda, carcasa, gas, pista y aire. Cada arista resta y suma la misma potencia; el residual de balance se registra. El calor de deslizamiento procede del trabajo disipado de las fuerzas de slip, no del par motor ni del empuje de camber. La presión usa gas ideal con referencia fija, corrección pequeña de volumen por deflexión y clamps diagnosticados. El desgaste es no negativo, monotónico con el trabajo, sin recuperación y con evento de pinchazo persistente.

## 5. Pruebas ejecutadas

Bancos nuevos:

- `validate_formula_tire_forces`: barridos lateral/longitudinal, pico, caída, presupuesto combinado, utilización ≤1, carga 0/100/1200/3200/6000/12000 N, camber simétrico, reversa, relajación con/sin, convergencia y migración de picos. Resultado: 20/20 PASS.
- `validate_formula_tire_contact`: equilibrio estático, carga cero, límite de deflexión, 120/240/480/500/1000 Hz, convergencia 500-1000 Hz y rigidez por presión. Resultado: 12/12 PASS.
- `validate_formula_tire_thermal`: equilibrio aislado, residual de energía, calentamiento, gas ideal, conversión de frontera gauge/absoluta, 130 kPa, 160 psi como caso extremo, desgaste, pinchazo y reset. Resultado: 24/24 PASS.
- `validate_formula_tire_energy`: regresión legacy. Resultado: 9/9 PASS.

Regresiones existentes con el XML candidato: `validate_formula_aerodynamics` PASS, `validate_formula_differential` PASS, `validate_williams_chassis` PASS.

Arranque nativo: `distribution/OpenFormula.exe` cargó el Williams/Fuji con el modelo acoplado. La telemetría de diagnóstico observó, tras 30 s de sesión, temperaturas de banda de 307-323 K (34-50 °C), presiones manométricas de 132-134 kPa, deflexiones de 6-14 mm, cobertura 1,0, utilización combinada 0,23-0,97, cero clamps de presión/temperatura/contacto y un residual térmico absoluto del orden de 10⁻² J. El contador térmico avanzó una vez por rueda y paso de 2 ms.

## 6. Límites y riesgos abiertos

- No se declara equivalencia con el modelo Rust ni fidelidad histórica del Goodyear de 1997.
- La presión de 160 psi no se probó en conducción; solo se verificó robustez, diagnóstico y ausencia de NaN. No es una referencia de calibración.
- La temperatura de pista usa el fallback declarado cuando `Ttrack` no es válido en simuv5; no se inventa una temperatura de pista a partir del aire.
- No se añadió calor de freno a la carcasa/gas en esta fase. La fuente no está definida en el contrato actual.
- El desgaste y la térmica se validaron en banco y en arranque corto, no en vueltas completas ni con lluvia.
- El contacto es una compliance radial equivalente con masa virtual; no sustituye un multibody de masa no suspendida.
- La aceptación humana de conducción, vuelcos, vibración, mando físico y vueltas completas en Fuji sigue pendiente.
- El modelo nuevo usa una única ley de compuesto; los índices de compuesto siguen publicándose para compatibilidad, pero no hay curvas específicas por compuesto.
- El HUD muestra presión manométrica; la interfaz `_tyreCurrentPressure` conserva Pa absolutos para no romper consumidores existentes.

## 7. Estado por fase

> Actualización posterior (Etapa A): se corrigió el selector `legacy` y se restauró el integrador de contacto de `8531ccb` como referencia estable en reposo. La comparación neutral a 500 Hz y el estado efectivo están en `implementation/reports/williams_idle_oscillation_stage_a.md`. La Etapa B del contrato vertical sigue abierta; el intento y sus límites están en `implementation/reports/williams_idle_oscillation_stage_b.md`.

| Fase | Estado | Evidencia |
| --- | --- | --- |
| TIRE-01 | Completada | contratos, unidades, selector, presión explícita, reset y mapa de archivos |
| TIRE-02 | Completada | `validate_formula_tire_forces` |
| TIRE-03 | Completada con compliance equivalente | `validate_formula_tire_contact`, deflexión en serie y límites |
| TIRE-04 | Completada | relajación, recuperación, trail y par de reacción |
| TIRE-05 | Completada en banco y arranque | `validate_formula_tire_thermal`, presión, desgaste y residual |
| TIRE-06 | Completada | selector por coche, dos rutas, reset/boxes, HUD y generador |
| TIRE-07 | Parcial | calibración 130 kPa y contraste de ecuaciones; no se ejecutó el núcleo Rust ni se declara equivalencia |
| TIRE-08 | Implementación y entrega técnica completas | build, DLL instalada, manifiesto, arranque y telemetría; aceptación humana de conducción pendiente |

## 8. Reproducción
Generación persistente:

```text
python tools/prepare_williams_vehicle.py --source-root D:/Formula90s \
  --data-root D:/open-formula/speed-dreams-data/data \
  --destination D:/open-formula/implementation/candidates/williams-nineteen-ninety-seven \
  --report D:/open-formula/implementation/candidates/williams-nineteen-ninety-seven/williams_generation_report.json
```

Build y pruebas:

```text
cmake --build C:/Users/bill/open-formula-build/engine --target simuv5 osggraph ssggraph
cmake --build C:/Users/bill/open-formula-build/engine --target validate_formula_tire_forces \
  validate_formula_tire_contact validate_formula_tire_thermal validate_formula_tire_energy
```

Distribución:

```text
python tools/assemble_portable_distribution.py --project D:/open-formula \
  --compiler-prefix C:/Users/bill/open-formula-build-tools/msys64/ucrt64
```

Hashes de la entrega:

- `simuv5.dll` compilada e instalada: `8821A29120AF2FFD53A56205832E069D9AA2B566562D0A561D16EADFD010CF6B`.
- XML candidato: `66FD0A8D05AF00C291C4647D0DDEC3A401B1D51423871AE49BA68B3CBCFDA426`.
- Manifiesto portable: `distribution/distribution_manifest.json`, 4619 archivos y cero dependencias sin resolver.
