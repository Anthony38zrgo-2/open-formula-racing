# Oscilación del Williams detenido: diagnóstico de simuv5

Fecha: 20 de octubre de 2026. Repositorio: `D:/open-formula`. Rama: `codex/initial-port`. HEAD: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`.

La oscilación grande reproducida procede del acoplamiento entre la carcasa de primer orden que está sin commitear y la suspensión de simuv5. Además, un error del selector impide que `tire model = legacy` active realmente el neumático legacy. Por tanto, la comparación anterior que atribuía el mismo problema a ambos modelos no era válida.

Esta auditoría no aplica una corrección a producción. Los tres archivos que ya estaban modificados se conservan byte por byte. Las bibliotecas y XML utilizados en las pruebas se restauraron al terminar. Las variantes experimentales, fuentes, órdenes de compilación y registros están fuera del repositorio, en [oscillation-audit-20261020](D:/open-formula-migration-evidence/oscillation-audit-20261020).

## Estado inicial y preservación

Los únicos cambios previos eran `src/modules/simu/simuv5/car.cpp`, `tire_contact_model.cpp` y `tire_model.cpp`. Se guardaron sus copias y `source-before.patch` antes de realizar las pruebas. No se cambió de rama, se reseteó el árbol ni se creó un commit.

El candidato usa `transient coupled` y telemetría desactivada; el XML instalado usa `legacy` y telemetría activada. La comparación del XML completo encontró solamente esas dos diferencias. La biblioteca instalada contenía el cambio de contacto a primer orden y la inicialización en equilibrio.

SHA-256 de la DLL original y restaurada: `2CAC8FAE43C8354673E2D03723A62B16103E9044F06195377568EBD7FF640FD8`.

SHA-256 del XML instalado original y restaurado: `CFC8C53C3ABB3B447119E565EF99C37B5F4A6E10D0243E5D406F77D2B0C5A155`.

## 1. Error confirmado del selector

En [tire_model_configuration.cpp:324](D:/open-formula/src/modules/simu/simuv5/tire_model_configuration.cpp:324), `TireModelConfigurationRead` devuelve `true` cuando `legacy` es una configuración válida, dejando `configuration.enabled = 0` y terminando antes de leer los parámetros del neumático acoplado.

En [tire_model.cpp:129](D:/open-formula/src/modules/simu/simuv5/tire_model.cpp:129), `SimTireModelConfigure` interpreta ese valor de retorno como la orden de activar el modelo. Confunde «configuración leída correctamente» con «modelo acoplado seleccionado».

Consecuencia verificada en ejecución: con el XML instalado en `legacy`, el registro anuncia `SimuV5 tire model: transient coupled`. Se ejecuta el neumático nuevo con valores predeterminados, sin haber leído su configuración específica del coche. Esto también afecta a otros vehículos que lleguen a esta función con el selector ausente o en `legacy`.

Las bibliotecas de comparación corrigen solamente esta selección mediante la condición «lectura válida y `configuration.enabled`», además de las modificaciones diagnósticas descritas abajo. La prueba adicional `runtime-installed-selector-bug-neutral.log` conserva el error para reproducir el estado instalado.

## 2. Reproducción y aislamiento de la carcasa–suspensión

Se utilizó el lanzador `distribution/OpenFormula.exe`, el Williams y el punto de salida actual de Fuji. En copias externas de `simu.cpp` se fijaron dirección, acelerador, freno y freno de mano a cero, embrague desacoplado y marcha neutra antes de actualizar la física. Se verificaron marcha y mandos en la telemetría. Esto evita atribuir entradas del mando o cambios de marcha a una oscilación espontánea.

Las sesiones iniciales con entradas libres no se utilizaron para comparar el reposo: contenían cambios de marcha y desplazamiento. Los resultados siguientes excluyen los primeros dos segundos de simulación. Cada sesión controlada cubre aproximadamente diez segundos de física. Las últimas cuatro comparaciones registran datos cada 2 ms, a 500 Hz.

| Variante, telemetría a 500 Hz | Balanceo RMS, grados | Carga delantera derecha, N | Carga delantera izquierda, N | Fracción de pasos con carga menor de 1 N, derecha / izquierda |
| --- | ---: | ---: | ---: | ---: |
| Estado instalado: XML `legacy`, selector erróneo y carcasa actual | 0.027030 | 0–5328 | 0–5375 | 33.6 % / 31.2 % |
| Acoplado explícito, carcasa actual de primer orden | 0.030619 | 0–9970 | 0–10383 | 34.0 % / 31.4 % |
| Contacto y reset del commit `8531ccb`, con selector corregido | 0.000115 | 1271–1393 | 1270–1394 | 0 % / 0 % |
| Carcasa actual, suprimiendo solamente su realimentación a la altura de contacto | 0.000123 | 1304–1371 | 1288–1361 | 0 % / 0 % |

El integrador y reset del commit reducen el balanceo RMS unas 266 veces respecto al caso acoplado actual. Suprimir solamente la realimentación geométrica lo reduce unas 250 veces. El cambio de inicialización en equilibrio tampoco explica el resultado: otra prueba recupera únicamente el integrador anterior y mantiene el reset actual; su balanceo RMS es `0.000002131 rad`, igualmente pequeño.

La carga nula no equivale siempre a que la rueda esté geométricamente en el aire: los indicadores de contacto siguen activos durante casi todos los pasos. Hay recorte de fuerzas normales negativas y grandes picos de apoyo. La carga delantera contiene componentes fuertes alrededor de 84–91 Hz; el balanceo del chasis tiene componentes más lentas. El registro original de 20 Hz no puede resolver esa vibración rápida de las cargas.

[Gráfico de las comparaciones a 500 Hz](D:/open-formula-migration-evidence/oscillation-audit-20261020/contact_suspension_comparison.png).

La ruta aislada es:

1. [wheel.cpp:526](D:/open-formula/src/modules/simu/simuv5/wheel.cpp:526) resta la deformación de carcasa del paso anterior a la altura de carretera utilizada por la suspensión.
2. Esa altura cambia el límite de extensión. [wheel.cpp:579](D:/open-formula/src/modules/simu/simuv5/wheel.cpp:579) obtiene la velocidad de suspensión mediante diferencias de posición divididas por el paso temporal.
3. `SimSuspUpdate` calcula muelle y amortiguador con ese estado. `SimWheelUpdateForce` calcula la nueva carga normal y actualiza la velocidad de la masa de rueda; las fuerzas normales negativas se recortan.
4. [tire_model.cpp:165](D:/open-formula/src/modules/simu/simuv5/tire_model.cpp:165) entrega esa carga a la carcasa. El filtro de [tire_contact_model.cpp:67](D:/open-formula/src/modules/simu/simuv5/tire_contact_model.cpp:67) actualiza la deformación que vuelve a entrar en la suspensión en el siguiente paso.

El filtro aislado puede converger con carga impuesta y, aun así, producir una oscilación al cerrar este circuito con la suspensión y su amortiguador. La eliminación de la masa virtual no demuestra por sí sola que el sistema acoplado sea estable. La prueba de una sola modificación en la altura de contacto localiza la ruta que sostiene la oscilación, sin demostrar todavía un balance energético completo ni validar una solución definitiva de contacto.

## 3. Comparaciones que descartan causas necesarias en esta reproducción

Estas comparaciones tienen muestreo de telemetría a 20 Hz; sus rangos no deben mezclarse con los máximos a 500 Hz de la tabla anterior.

| Variante controlada | Balanceo RMS, radianes | Momento de balanceo por cargas normales, N·m |
| --- | ---: | ---: |
| Legacy real, selector corregido | 0.000007366 | −76 a +75 |
| Acoplado actual | 0.000536868 | −5458 a +5360 |
| Acoplado actual, empuje de camber a cero en ambos ejes | 0.000539934 | −5596 a +4858 |
| Acoplado actual, ambas barras estabilizadoras a cero | 0.000689752 | −5489 a +5267 |
| Acoplado actual, añadiendo velocidad de contacto por roll y pitch | 0.000449229 | −5444 a +5763 |

Quitar el camber thrust o las barras no elimina el problema. Añadir la velocidad de contacto debida a roll/pitch tampoco lo resuelve. No están justificados nuevos cambios de rigidez de barras, agarre o bloqueo del diferencial para corregir esta reproducción.

En neutro, el momento giroscópico de balanceo sumado fue cero en los casos acoplados. Además, la misma transmisión permanece en las variantes que eliminan la oscilación grande. No hay evidencia de que un impulso sin reacción del diferencial sea la causa necesaria de este balanceo. Esto no valida todos los transitorios de transmisión ni descarta otros problemas al circular.

El legacy real conserva una oscilación pequeña en este ensayo, pero no reproduce la descarga repetida ni los picos grandes del cambio actual. No se puede concluir que este defecto grande ya existiera en Speed Dreams original.

## 4. Contratos adicionales que requieren revisión

### Velocidad y momento no usan exactamente el mismo punto

[car.cpp:852](D:/open-formula/src/modules/simu/simuv5/car.cpp:852) incluye traslación y guiñada en la velocidad que recibe el neumático, pero omite las componentes de roll/pitch a la altura del contacto. Las fuerzas laterales y longitudinales sí generan momentos utilizando esa altura en `SimCarUpdateForces`.

La misma aproximación existe en el ancestro upstream local `d8d9c179`; no la introduce este neumático. Puede omitir amortiguación y dificultar el balance de potencia. Su corrección aislada no suprime la oscilación grande actual, como muestra la tabla.

### Corrección de reposo heredada

[car.cpp:724](D:/open-formula/src/modules/simu/simuv5/car.cpp:724), con `FEAT_SLOWGRIP` y velocidad menor de 0.1 m/s, modifica la fuerza lateral total y el momento de guiñada para detener el coche. No recalcula del mismo modo el momento de balanceo que ya producen las fuerzas individuales.

Esto explica por qué la telemetría puede mostrar velocidad y aceleración lateral nulas mientras persiste balanceo. Es un comportamiento heredado que debe incluirse en una auditoría de potencia; no prueba por sí solo el origen de la inestabilidad actual.

### Relajación de fuerza y energía no contabilizada

El banco nativo externo `probe_force_contracts.cpp` reproduce una inversión de velocidad lateral de +0.01 a −0.01 m/s con carga de 1500 N, sin camber ni giro. Tras la inversión, la fuerza relajada conserva temporalmente su signo anterior: `Fy = −1250.94 N`. Entrega aproximadamente `+12.51 W` al movimiento, mientras la potencia publicada como calor es cero porque el resultado negativo de disipación se recorta.

Una fuerza transitoria puede devolver energía almacenada; esta medición no demuestra creación neta sostenida. Sí demuestra que hace falta contabilizar ese almacenamiento o verificar pasividad mecánica, en vez de interpretar automáticamente el calor no negativo como conservación de energía. Este hallazgo es adicional: la prueba de aislamiento vertical ya elimina la oscilación grande manteniendo el modelo de fuerza.

### El ensayo de convergencia de contacto no aplica una excitación válida

En [validate_formula_tire_contact.cpp:59](D:/open-formula/tools/validate_formula_tire_contact.cpp:59), la frecuencia de integración se reutiliza como frecuencia de la sinusoide de carga. Con `time = step / frequency`, su argumento es `2π × step`: la supuesta sinusoide se muestrea siempre prácticamente a cero. Los ensayos comparan el equilibrio bajo unos 3000 N, no la convergencia dinámica frente a una excitación común.

Los bancos existentes de contacto y fuerza pasan, pero ese resultado no valida este acoplamiento. También hay que revisar el caso `elapsed_seconds = 0`: el filtro actual utiliza una fracción de actualización igual a uno y puede cambiar la deformación sin transcurso de tiempo.

## Orden propuesto de corrección y aceptación

1. Corregir el selector en `tire_model.cpp` y cubrir explícitamente selector ausente, `legacy`, `transient coupled` y configuración inválida. Verificar el modelo efectivo en ejecución, no solamente el XML.
2. No promover el filtro de primer orden actual como solución a la vibración. El integrador del commit sirve como referencia comparativa para el reposo, pero estas pruebas no certifican su arquitectura de masas ni su comportamiento en pista.
3. Resolver conjuntamente el contrato vertical de `wheel.cpp`, `tire_contact_model.cpp` y `tire_model.cpp`: coordenadas, masa no suspendida, deformación elástica, amortiguación, trabajo de contacto y orden temporal. Si se elimina un estado de masa, demostrar estabilidad y balance de trabajo del sistema completo. Suprimir la realimentación a la altura fue una prueba de aislamiento; no es una propuesta de eliminar la compliance del coche final.
4. Sustituir el ensayo de convergencia por una carga física común a todos los pasos temporales. Comparar trayectorias, fase y trabajo, incluyendo contacto/despegue y tiempo cero. Añadir un ensayo acoplado de rueda–suspensión, porque el banco con carga impuesta no contiene el circuito que falla.
5. Repetir reposo con mandos controlados, perturbación pequeña de balanceo, inversión lateral y frenado hasta detenerse. Registrar fuerzas, velocidades en el punto de contacto, energía cinética, elástica y disipada. Verificar que la perturbación decae y que desaparece el patrón de cargas cero/picos bajo apoyo sostenido.
6. Después comprobar aceleración, retención, diferencial, cambios de marcha y paso por pianos, manteniendo iguales neumáticos, temperaturas y reglaje entre comparaciones. Hace falta conducción sostenida para aceptar el comportamiento final; estas sesiones cortas en reposo no la sustituyen.

## Entregables y estado final

El directorio de evidencias contiene registros `runtime-*-neutral.log`, resúmenes `runtime-summary.json` y `fast-summary.json`, el gráfico, el banco de fuerzas y las fuentes de cada biblioteca diagnóstica. `build_isolation_variants.py`, `build_final_comparisons.py` y los scripts de ejecución documentan las sustituciones aisladas. Las bibliotecas experimentales no están instaladas al terminar.

El código de producción conserva exactamente las tres modificaciones previas. Se comprobó por SHA-256 frente a las copias iniciales, además de restaurar DLL y XML. El único archivo añadido al repositorio por esta revisión es este informe. No se modificaron diferencial, reglaje, neumáticos, texturas ni instrucciones de implementación pendientes.
