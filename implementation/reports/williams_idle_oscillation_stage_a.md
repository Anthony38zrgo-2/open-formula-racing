# Oscilación en reposo del Williams: Etapa A

Fecha: 20 de octubre de 2026. Rama: `codex/initial-port`. Base: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`.

Este informe cubre la Etapa A del encargo de corrección: selector efectivo y recuperación de una referencia estable. No declara resuelto el contrato vertical definitivo ni la aceptación de conducción.

## 1. Causa corregida: selector legacy

`TireModelConfigurationRead` devolvía `true` para una configuración legacy válida, aunque dejaba `configuration.enabled = 0`. `SimTireModelConfigure` interpretaba ese retorno como la orden de activar el modelo acoplado.

Efecto: un XML con `tire model = legacy` ejecutaba el modelo nuevo con valores predeterminados. La comparación anterior entre legacy y acoplado no era válida.

Corrección en `tire_model.cpp`: el modelo se habilita solo cuando la lectura es válida **y** `configuration.enabled` está activo. El registro de ejecución publica ahora el modelo efectivo y el estado de la configuración.

## 2. Recuperación del contacto de referencia

Se retiró el filtro de primer orden sin commitear y se restauró `tire_contact_model.cpp` desde `8531ccb`. Se conservaron:

- Inicialización de la deflexión en equilibrio estático en `SimTireModelResetWheel`.
- Radio efectivo inicial coherente.
- Telemetría adicional de `car.cpp`.
- Selector corregido.

El XML candidato y el instalado quedan en `transient coupled` con telemetría desactivada.

## 3. Comparación neutral a 500 Hz

Condiciones: Williams/Fuji, dirección, acelerador, freno y freno de mano a cero, embrague desacoplado, marcha neutra. Se excluyeron los primeros 2 s. La DLL diagnóstica con mandos forzados se mantiene fuera de la distribución.

| Modelo efectivo | Balanceo RMS | Carga delantera derecha | Carga delantera izquierda | Pasos con carga < 1 N |
| --- | ---: | ---: | ---: | ---: |
| Legacy real | 0.000411° | 1282.9–1386.5 N | 1285.2–1386.5 N | 0 % |
| Acoplado real | 0.000123° | 1270.3–1384.5 N | 1271.1–1404.9 N | 0 % |

El acoplado real queda en el mismo orden que la referencia de la auditoría y no reproduce el patrón de cargas nulas ni los picos de 5–10 kN del filtro de primer orden. La deflexión delantera se mantiene en 8.59–8.61 mm y la velocidad de suspensión RMS en aproximadamente 0.206 m/s.

## 4. Pruebas

- `validate_formula_tire_selector`: 6/6 PASS.
  - selector ausente → legacy
  - legado → legacy
  - acoplado → acoplado
  - esquema inválido → deshabilitado
  - presión incompleta → deshabilitado
  - fixtures escritos correctamente
- `validate_formula_tire_contact`: 12/12 PASS.
- `validate_formula_tire_forces`: 20/20 PASS.
- `validate_formula_tire_thermal`: 24/24 PASS.
- `validate_formula_tire_energy`: 9/9 PASS.
- Arranque con XML legacy: registra `legacy (configuration_valid=1 enabled=0)` y no ejecuta resets acoplados.
- Arranque con XML acoplado: registra `transient coupled (configuration_valid=1 enabled=1)` y carga la configuración del vehículo.

## 5. Archivos

- `src/modules/simu/simuv5/tire_model.cpp`: selector efectivo.
- `src/modules/simu/simuv5/tire_contact_model.cpp`: restaurado desde `8531ccb`.
- `src/modules/simu/simuv5/car.cpp`: telemetría de fuerza, giro, par y potencia.
- `tools/validate_formula_tire_selector.cpp`: banco nuevo de selector.
- `CMakeLists.txt`: registro del banco.
- `implementation/reports/williams_idle_oscillation_stage_a.md`: este informe.

## 6. Evidencia externa

- `D:/open-formula-migration-evidence/oscillation-audit-20261020/simuv5-stage-a-neutral-diagnostic.dll`.
- `runtime-stage-a-legacy-neutral.log`.
- `runtime-stage-a-coupled-neutral.log`.
- `run_stage_a_comparison.ps1`.
- Copia previa a la intervención en `stage-a-before/` y `stage-a-before.patch`.

La DLL diagnóstica no es producción y no debe promoverse a la distribución.

## 7. Entrega

- `simuv5.dll` compilada: SHA-256 `76552EA9CF70576C7ED5DE809DF390C12632CD630D6D045C6F69DABECBD05A1F`.
- XML candidato/instalado: `66FD0A8D05AF00C291C4647D0DDEC3A401B1D51423871AE49BA68B3CBCFDA426`.
- Configuración instalada: `transient coupled`, telemetría desactivada.

## 8. Límites

La Etapa A recupera una referencia estable en reposo y corrige la selección. No cierra la Etapa B:

- El contrato de masa no suspendida, deformación, amortiguación y trabajo del sistema rueda–suspensión–carcasa sigue pendiente de formulación definitiva y balance energético.
- El banco de convergencia de contacto aún debe corregirse para aplicar una excitación física común y comprobar trayectoria, fase y trabajo.
- Falta el banco acoplado rueda–suspensión–carcasa con perturbación de balanceo y contacto unilateral.
- La conducción sostenida, frenada hasta detenerse, pianos, cambios y aceptación humana siguen pendientes.
