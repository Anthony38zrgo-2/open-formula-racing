# Verificación del solver antes de publicación

Fecha: 20 de octubre de 2026. Base inspeccionada: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`, rama `codex/initial-port`.

La publicación incluye el selector corregido, la inicialización de contacto en equilibrio, el adaptador geométrico y el solver vertical conjunto de siete grados de libertad. Incluye su integración seleccionable en ambas rutas de simuv5, velocidad de contacto con roll/pitch, evaluación provisional de fuerzas sin avanzar sus memorias y corrección de momentos. `reference` sigue siendo el valor predeterminado del código; la integración nueva se selecciona por vehículo con `Formula Controls / vertical integration = coupled seven degree of freedom`.

El candidato/XML de distribución es generado e ignorado por Git. El generador mantiene `transient coupled` a 130 kPa manométricos; quien quiera reproducir la ruta vertical nueva debe seleccionar explícitamente el atributo anterior. No se publican DLL ni la distribución portable en este commit.

## Comprobación actual

Se compilaron `simuv5`, `legacymenu` y los bancos con el build local de Windows, sin cambiar de rama. Los siguientes ejecutables terminaron con código cero y ninguna línea FAIL; los bancos que consumen el coche recibieron el candidato Williams actual.

| Banco incluido en la publicación | Comprobaciones PASS |
| --- | ---: |
| validate_formula_tire_selector | 6 |
| validate_formula_tire_geometry | 144 |
| validate_formula_tire_vertical_system | 208 |
| validate_formula_tire_contact | 12 |
| validate_formula_tire_forces | 20 |
| validate_formula_tire_thermal | 24 |
| validate_formula_tire_energy | 27 |
| validate_williams_chassis | 6 |
| Total | 447 |

Los registros de esta ejecución están fuera del repositorio, en `D:/open-formula-migration-evidence/publication-ui-solver-20261020/`. Los informes de Etapas A/B y Puertas 1–3 contienen los ensayos anteriores y sus condiciones; sus cifras históricas no sustituyen la tabla de esta ejecución.

## Prototipo excluido

El primer experimento `tire_vertical_model.cpp/.h` y `validate_formula_tire_vertical.cpp` no forman parte de esta publicación ni se enlazan en simuv5. Su target local se conserva sin preparar en el árbol de trabajo. El ensayo original de diez comprobaciones pasa, pero la prueba adicional de perturbación negativa cuestiona su contabilidad de energía; no es el solver integrado ni una certificación física. El código publicado utiliza `tire_vertical_solver.cpp/.h` y el banco `validate_formula_tire_vertical_system`.

## Límites de aceptación

Esta publicación conserva el estado parcial documentado: existen validaciones de geometría, balance mecánico del banco, reposo y una secuencia diagnóstica de conducción. Quedan pendientes conducción sostenida a alta velocidad, la última curva real de Fuji, pianos, pendientes, boxes y aceptación humana. Los ensayos sintéticos de baches y la corrección de momentos no establecen por sí solos que la oscilación fuerte a alta velocidad haya desaparecido.

La pantalla de carga se publica separadamente: compilación correcta, fondo editado suministrado por el usuario y comprobación de igualdad de píxeles tras conversión JPG→PNG. La revisión visual interactiva se interrumpió por Escape y permanece pendiente.

## Aprobación posterior: 21 de octubre de 2026

El usuario aprobó explícitamente la validación a alta velocidad del modelo actual. Se registra como aceptación humana de ese aspecto; no representa una nueva ejecución instrumentada por el agente ni extiende automáticamente la aceptación a boxes, pianos, pendientes u otros casos no confirmados. Las limitaciones anteriores describen el estado al publicar el 20 de octubre. El alcance de retirada de las rutas anteriores se está aclarando antes de modificar código compartido.

## Retirada posterior: 21 de octubre de 2026

El usuario confirmó retirar también el neumático legacy original. La limpieza está implementada y la DLL instalada: simuv5 usa únicamente el modelo acoplado y su solver de siete grados de libertad. Los selectores, targets y cifras de publicación anteriores quedan como historial. Validación actual: 504 comprobaciones y arranque correcto; ver `simuv5_legacy_tire_retirement.md`. Los reglajes simultáneos se preservaron y requieren su propia revalidación.
