# Retirada del neumático legacy de simuv5

Fecha: 21 de octubre de 2026. Rama: `codex/initial-port`. HEAD de partida: `37f8058ef814e1864c59126f3c4fe38ebf0f29d5`.

El usuario autorizó retirar también el neumático original de Speed Dreams. simuv5 utiliza exclusivamente el neumático transient coupled y el solver vertical conjunto de siete grados de libertad. La aprobación humana de alta velocidad del modelo anterior a esta limpieza queda registrada; esta ejecución no repitió conducción instrumentada a alta velocidad ni certifica los reglajes modificados por otro trabajo simultáneo.

## Implementación

- `wheel.cpp`: retirados SimWheelUpdateRide, SimWheelUpdateForce y SimWheelUpdateTire, incluyendo fuerza Magic Formula original y evolución térmica/desgaste legacy. Retirada la relajación de giro exclusiva de la ruta antigua. Se conservan configuración mecánica, inercia, interfaz de reglajes, ayudas y metadatos de compatibilidad.
- `simu.cpp`, `car.cpp`, `carstruct.h`, `sim.h`: eliminada selección y ejecución del integrador reference en ambas rutas; posición/velocidad vertical, roll y pitch pertenecen al solver conjunto.
- `collide.cpp`: la colisión consulta carretera sin invocar el integrador retirado.
- `tire_model_configuration.cpp`, `tire_model.cpp`: ausencia de selector elige coupled; `legacy`, selectores desconocidos y configuración inválida se rechazan sin fallback. Los coches de simuv5 necesitan Formula Tires válido con convención y presiones explícitas; deben migrarse antes de utilizar este módulo. No se inventa una calibración para coches antiguos.
- `tire_contact_model.cpp/.h`: evaluación del contacto a partir de deformación/velocidad proporcionadas por el solver único; no existe una segunda masa de carcasa integrada.
- Eliminados el prototipo local descartado `tire_vertical_model.cpp/.h`, su banco `validate_formula_tire_vertical.cpp` y su target. El solver vigente `tire_vertical_solver` se conserva.
- `prepare_williams_vehicle.py`: el generador conserva explícitamente la integración coupled seven degree of freedom. Los cambios simultáneos de reglaje en este archivo se preservaron.
- Banco de energía adaptado al modelo térmico vigente. Detectó y corrigió una omisión contable: carcass_air_power ahora forma parte de external_energy. Cambian únicamente los acumuladores/residuos del balance, no las temperaturas ni fuerzas calculadas.

No se modificaron los plugins históricos simuv2/simuv4. Las leyes compartidas de suspensión y barra estabilizadora se conservan; el banco de chasis sigue comprobándolas, mientras el banco vertical prueba su integración vigente. Los informes de Etapas A/B conservan comparaciones históricas con legacy; no describen opciones todavía disponibles.

## Validación actual

| Banco | PASS |
| --- | ---: |
| Selector | 6 |
| Geometría | 144 |
| Sistema vertical | 208 |
| Contacto sin integración secundaria | 24 |
| Fuerzas | 20 |
| Térmica y desgaste | 24 |
| Energía térmica y tiempo cero | 72 |
| Chasis | 6 |
| Total | 504 |

Compilación de simuv5 y todos los bancos correcta. Ningún banco terminó con error. Las advertencias del compilador preexistentes no impidieron el enlace.

Arranque con `distribution/OpenFormula.exe`: llegó al estado RACE; registro confirma configuración coupled válida, solver único y equilibrio inicial con cargas 1336.681 / 1336.681 / 1667.096 / 1667.096 N. Es una prueba de arranque y apoyo inicial, no una medición de estabilidad sostenida. La sesión creada para la prueba fue cerrada. No se alteraron controles ni se activó telemetría para este arranque.

DLL instalada SHA-256: `8733DF1581C0F1B5FD6FB6554A072D19D65E5742A5512A07DDCCBD3EDC9101E9`.
XML candidato usado en el arranque SHA-256: `BED31AADA49A33314BC5EAB00A9C4771807FCA69B56C20ABB2A9824243F52363`. El reglaje simultáneo conserva 100 kPa manométricos y 60 °C iniciales; no se sustituyó por los valores anteriores.

Se actualizó la entrada de simuv5 en el manifiesto de distribución. Respaldo previo de fuentes, DLL y registros: `D:/open-formula-migration-evidence/retire-legacy-20261021`. Los archivos del prototipo descartado permanecen recuperables en ese respaldo.

Pendientes independientes: pruebas completas de pianos, pendientes y boxes; revalidación de reglajes posteriores. Sin commit ni push en esta retirada.
