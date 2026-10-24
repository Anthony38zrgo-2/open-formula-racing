# Plan de corrección estructural de suspensión y contacto

Base inspeccionada: `4a14467ca061982fca87a60212489bef4e018911`, D:/open-formula. Encargo: planificar a partir del hallazgo aportado por el usuario; no implementar todavía. Este archivo solo añade documentación. No modifica física, reglajes, DLL, instalación ni telemetría. Las referencias `archivo:línea` se verificaron sobre este commit; el detalle de la verificación está en §7.

## 1. Hallazgos contrastados y límites

| Hallazgo | Confirmación actual | Límite |
| --- | --- | --- |
| Rueda sin apoyo pierde su contribución de suspensión al cuerpo simplificado | `simulation.rs:608–677` termina en `continue` (676) sin aplicar `chassis_suspension_force`; la alternativa de reacciones internas exige `resolves_internal_link_forces()`, que solo devuelve true para Standard (`suspension.rs:526–528`). El segundo bucle también descarta la rueda sin apoyo (`simulation.rs:749–751`). | Es un defecto de ensamblaje en la ruta que aplica `chassis_suspension_force`; su impacto cuantitativo sobre T3 requiere reproducción. |
| El residuo de convergencia del eje omite el suelo | `simplified_axle_integration.rs:160–173` proyecta el residuo solo contra los topes de recorrido; la tolerancia de aceptación es `2e-9` (`190`) y el fallback no verificado ocupa `196–218`. Los solves de esquina sí reconocen el suelo (`vertical_suspension_integration.rs:139–151`, `188–198`). | El criterio puede rechazar una reacción válida y alcanzar el fallback; aún no se midió cuántas veces ocurre en la sesión. |
| Dinámica relativa incompleta y masa virtual | `vertical_suspension_integration.rs` usa la masa y fuerza de suspensión; `simulation.rs:1125–1126` integra el cuerpo con masa total y gravedad total. `wheel_mechanics.rs:69–77` declara masa virtual con multiplicador. | No agregar gravedad a las cuatro ruedas sin definir una distribución de masas y el intercambio de momento. |
| Alternancia brusca de cargas | El log congelado confirma N=(0;3856;0;6174,5) a 41,174 s y N=(4453,6;0;4604,9;0) a 41,424 s, con roll -0,0040/-0,0027 rad. | El muestreo de aproximadamente 0,25 s no demuestra un ciclo numérico continuo, ni excluye cambios de terreno/velocidad angular entre muestras. |

Una precisión: actualmente el soporte del cuerpo simplificado procede de `chassis_suspension_force`, no directamente de sumar otra vez la reacción normal del suelo (`simulation.rs:769–774`). En producción es además el único camino: `SuspensionBackend::production()` devuelve Simplified (`simplified_suspension.rs:28–32`) y `open_formula.rs:519` fija ese backend, por lo que `resolves_internal_link_forces()` es siempre falso y las ramas de wrench geométrico (`simulation.rs:634–675` y `779–820`) no se ejecutan. El soporte publicado es `basis.y * chassis_suspension_force` (`772`) y el momento usa `arm.cross(wheel_force)` con el brazo desde `effective_contact_point` (`752–753`, `823`, `828`). La corrección debe conservar esa separación y evitar introducir un soporte duplicado.

Otra precisión: que las dos contribuciones verticales de una barra ideal se compensen no significa que su par de balanceo deba ser cero. Se verifican fuerza neta y momento con sus brazos, y el balance interno del conjunto cuerpo/ruedas, no se anula artificialmente el momento de la barra sobre el cuerpo.

## 2. Decisión técnica previa: un solo contrato de masa y fuerzas

**Recomendación para una corrección física completa:** tratar las masas de rueda que se integren como masas físicas, contadas una sola vez, y resolver su acoplamiento con el cuerpo. Conservar los elementos simplificados/tabulados: no requiere un cuerpo rígido de aplicación por rueda, otro motor ni migrar automáticamente al backend Standard o al mundo acoplado alternativo.

Antes de codificar, entregar una tabla de masa, centro de masa e inercia: cuerpo suspendido, conjuntos móviles, combustible y partes ya incluidas en cada presupuesto. No basta con restar cuatro masas a un escalar: la distribución espacial y las inercias deben cerrar. El multiplicador de masa virtual no se convierte silenciosamente en masa física; definir su retirada/migración o función auxiliar sin duplicación.

Si se decide conservar la masa virtual como filtro, documentar un contrato aproximado distinto y sus balances. No presentarlo como una rueda física independiente ni mezclarlo con las ecuaciones de masas separadas. Esta decisión es una puerta de diseño; no se instala una suma de reacciones y gravedad por piezas antes de cerrarla.

Convención propuesta: `x` es compresión absoluta, `q=x-x_ref`, q positivo en compresión. La normal del suelo y el eje de suspensión se expresan en mundo. Derivar con cinemática del buje:

`M(Q) dv/dt + C(Q,v) = Q_gravedad + Q_elementos + J_contacto^T lambda + J_topes^T mu + Q_externas`.

Q incluye pose del cuerpo y cuatro recorridos; la orientación se integra con su representación actual. Evaluar la reutilización de las utilidades de masa/Jacobianos existentes en `suspension_coupled_solver.rs` y `coupled_vehicle_dynamics.rs`, pero verificar su compatibilidad antes de incorporarlas. No activar esas rutas ni mantener dos integraciones del cuerpo.

Con coordenadas relativas al cuerpo, la transferencia interna puede aparecer a través de términos cruzados de M y los Jacobianos. No añadir además una reacción explícita equivalente si ya está representada: elegir y demostrar una sola formulación.

## 3. Implementación por fases

### Fase A — Reproducción e instrumentación, antes de corregir

Crear bancos de integración compilables por separado, sin depender de los fixtures ajenos ausentes del target `--lib`. Mantener la suite de autoridad de reglajes existente.

Casos que deben reproducir el fallo actual:

1. Cuerpo y dos ruedas, una con apoyo y otra sin él, ambas con fuerzas de elementos no nulas. Mostrar qué fuerza e impulso de suspensión se calculan y cuáles recibe cada parte.
2. Eje apoyado con una carga constante conocida. Con F=1000 N y h=1/480 s, una reacción de 2,083333 N s debe ser válida, sin exigir residuo de momento bruto cero ni recurrir al fallback por ese motivo.
3. Compresión diferencial de barra, con y sin suelo bajo cada lado. Verificar fuerzas iguales/opuestas en el modelo paralelo y momento correcto sobre el cuerpo.
4. Cuerpo en caída libre, rueda descargada, y cuerpo con aceleración prescrita. Sirven para detectar gravedad duplicada, aceleración de anclaje omitida y confusión de velocidad mundial/relativa.

Traza técnica por subpaso aceptado: estado inicial/final de cuerpo y rueda, reglaje efectivo, q/qdot y límites, fuerzas de cada elemento, impulsos aceptados de suelo y topes, hueco/velocidad normal, brazo/momento, estado de restricciones, iteraciones, residuo normalizado y uso del fallback. Registrar por separado intentos rechazados, sin acumular su energía o impulsos.

**Salida:** reproducción determinista y contrato de masas/fuerzas cerrado. Todavía sin instalar una corrección parcial.

### Fase B — Contacto unilateral y convergencia conjunta del eje (P0)

En `simplified_axle_integration.rs` y `vertical_suspension_integration.rs`:

- Incluir suelo y topes en la misma condición de convergencia; no exigir momento libre cero en una coordenada ligada.
- Verificar factibilidad del hueco, J normal >= 0 y complementariedad. La reacción normal permite el residuo de cantidad de movimiento correspondiente; las esquinas libres sí deben cerrar su balance sin esa reacción.
- Resolver ambas ruedas con un único par final de velocidades y fuerzas de barra coherentes. Reevaluar los dos residuos con ese mismo par antes de aceptar; no publicar dos esquinas recalculadas con estados opuestos desfasados.
- Sustituir el fallback no verificado por una resolución conjunta con restricciones activas. El eje tiene pocas coordenadas: usar un método acotado/determinista, con detección de configuraciones inviables y validación final completa.
- Suelo y droop coincidentes, o suelo incompatible con bump, requieren política explícita. No recortar el límite del suelo al tope y aceptar penetración silenciosamente. Subdividir cuando el problema temporal lo permita; si sigue siendo inviable, fallo diagnosticable y conservación del último estado aceptado.
- Escalar las tolerancias según masa, impulso y precisión geométrica; no cambiar simplemente `2e-9` por un umbral grande. Documentar tolerancias absolutas/relativas por unidades y verificar convergencia frente a la referencia.

**Puerta B:** reposo, una rueda libre, dos apoyadas, contacto/topes simultáneos y permutación izquierda/derecha sin fallback injustificado. Balance de impulso y restricciones satisfechos para el estado publicado.

### Fase C — Transferencia al cuerpo y puntos de aplicación (P0/P1)

En `simulation.rs`, `suspension.rs` y `simplified_contact_kinematics.rs`, con el contrato de masa ya fijado:

- Separar el ensamblaje de suspensión interna del recorrido de neumáticos. Una rueda con N=0 conserva su conexión mecánica al cuerpo; la suspensión no depende de `is_grounded` para existir.
- Grip, fuerza tangencial y par autoalineante siguen requiriendo contacto. No basta con eliminar `continue`, pues podría reactivar otros consumidores que sí deben quedar a cero.
- No activar `resolves_internal_link_forces()` globalmente para Simplified como atajo: el método selecciona también otros caminos de soporte/cinemática y puede duplicar cargas.
- Separar geométricamente el contacto rueda/suelo de la transmisión interna. Obtener una fuerza y momento equivalentes por trabajo virtual/Jacobianos o un wrench de anclajes consistente; no inventar un único punto conveniente que cambie el par de cabeceo/balanceo.
- En vuelo, no leer un `effective_contact_point` borrado o antiguo para calcular el brazo de suspensión.
- Usar impulsos/fuerzas del intervalo aceptado para la transferencia mecánica, no muestras finales arbitrarias. Los límites de recorrido también intercambian momento con el cuerpo y requieren su reacción.

**Puerta C:** acción/reacción o su equivalente generalizado, conservación de momento global cuando no hay fuerzas externas, continuidad del wrench al separar/recuperar contacto y par de barra correcto. En un paso de impacto, distinguir apoyo durante el intervalo del flag de contacto al final.

### Fase D — Evolución relativa y cierre del modelo físico (P1)

Integrar cuerpo y recorridos bajo una autoridad. Incluir gravedad y aceleración de anclaje una sola vez; derivar los términos de rotación, Coriolis y cinemática del recorrido cuando correspondan, en vez de agregarlos como correcciones empíricas.

Si las fases B/C usan temporalmente el modelo virtual para aislar fallos, mantener esa variante solo en bancos hasta completar esta fase o demostrar su contrato aproximado. La aceptación final no se obtiene sumando resultados de modelos distintos.

**Puerta D:** caída libre del sistema sin aceleración relativa gravitatoria espuria; respuesta correcta a anclaje acelerado; reposo, roll/pitch, vuelo y aterrizaje con balance de energía y momento; masa/inercia totales conservadas; no hay una segunda integración vertical del cuerpo.

### Fase E — Geometría efectiva y regresión (P2)

Medir q mínimo/máximo real, precarga, desasentamiento del muelle, movimiento de balancín, relación de movimiento y derivada en el rango usado. El cambio de `resting_ratio` debe preservar una referencia coherente y no producir saltos del estado físico al aplicar un reglaje.

Repetir los diez controles del garaje y la idempotencia; verificar ambos ejes, simetría espejo, plano inclinado, pérdida de uno/tres rayos, contacto parcial y retorno a boxes/reset. Mantener neumático rígido y curvas tangenciales actuales.

Barrido temporal con los pasos admitidos por la API actual —por ejemplo 1/120, 1/240 y 1/480 s— y subdivisiones internas suficientes para medir tendencia. No afirmar convergencia usando un dt menor que el clamp efectivo del host. Comparar con referencia analítica o integración independiente de mayor precisión, no únicamente contra el algoritmo corregido.

No exigir identidad de trayectoria con la física anterior: precisamente se corrigen fuerzas y dinámica. Conservar identidad solo donde el cambio no debe actuar, y justificar variaciones en apoyo, energía y movimiento del cuerpo.

### Fase F — Candidato e instalación; conducción reservada al usuario

Solo tras superar B–E: compilar, comprobar ABI, hashes y procedencia, respaldar externamente e instalar candidato local sin sobrescribir reglajes ni cerrar una sesión abierta. Actualizar manifiesto y ofrecer reversión. Una ampliación de telemetría C/Rust exige coordinar ABI y consumidor; preferir traza interna separada si evita cambios innecesarios al host.

Preparar al usuario una comparación A/B con mismos reglajes, combustible y maniobra. **La prueba y aceptación en T3 siguen siendo del usuario.** No declarar que cuatro ruedas deben soportar carga en todo momento: el criterio es movimiento relativo, contacto y recuperación físicamente admisibles, sin oscilaciones espurias.

Recalibrar muelles, amortiguadores o barras (P3) queda fuera de la corrección estructural. Solo considerarlo después de aceptación técnica y observación manual, manteniendo intacto el reglaje del usuario.

## 4. Responsabilidades de archivos

| Archivo | Responsabilidad prevista |
| --- | --- |
| `simulation.rs` | Ensamblaje mecánico, masa/inercia y única integración del cuerpo; separar suspensión y contacto tangencial. |
| `simplified_axle_integration.rs` | Solución conjunta, coherencia de barra, aceptación/rechazo y diagnósticos de convergencia. |
| `vertical_suspension_integration.rs` | Contrato de restricciones, impulso de suelo/topes y compatibilidad con el esquema conjunto. |
| `suspension.rs` | Fuerzas de elementos, estado aceptado, impulsos y soporte publicado sin dependencia indebida de grounded. |
| `simplified_contact_kinematics.rs` | Hueco/velocidad normal y separación entre contacto y transmisión de suspensión. |
| `wheel_mechanics.rs`, `vehicle_mass_properties.rs` y propietarios de masa/configuración | Inventario, masa física/virtual y migración explícita si cambia el contrato. Leer sus AGENTS antes de editar. |
| `suspension_loads.rs`, `suspension_component_kinematics.rs`, tablas y solver acoplado existentes | Reutilizar piezas verificadas de wrench/Jacobianos/masa; no cambiar de backend implícitamente. |
| `open_formula.rs` | Orden de pasos, reset, publicación y traza con identidad de reglaje. |
| `tests/` | Bancos estructurales nuevos e integración con la autoridad de reglajes ya corregida. |
| Informe y manifest de candidato | Evidencia, límites, hashes, reversión y puerta manual pendiente. |

## 5. Referencias externas y cautela

La documentación de [PhysX: fuerza de suspensión](https://nvidia-omniverse.github.io/PhysX/physx/5.4.2/_api_build/struct_px_vehicle_suspension_force.html) distingue fuerza/momento transmitidos al cuerpo y carga normal utilizada por el neumático. La de [dinámica de extensión](https://nvidia-omniverse.github.io/PhysX/physx/5.4.2/_api_build/struct_px_vehicle_suspension_state_calculation_params.html) explica que limitar su velocidad puede aumentar las pérdidas de contacto. Son referencias de contratos; no prueban que su aproximación de masas sea intercambiable con la del proyecto ni justifican importar un limitador.

## 6. Entrega prevista antes de tocar producción

1. Reproducción mínima de los dos defectos P0 y métricas de la base.
2. Derivación y tabla de cierre de masas/inercias, signos y fuerzas internas/externas.
3. Diseño del solver y criterios de aceptación, incluidos estados incompatibles.
4. Implementación posterior dividida en cambios revisables, con bancos que fallen antes y pasen después.
5. Candidato instalado con validación técnica, sin atribuirle aceptación de conducción.

La planificación no ejecuta los puntos anteriores. Estado de este encargo: inspección y plan documental completos; código e instalación sin cambios.

## 7. Verificación de referencias y evidencia congelada

Comprobación por lectura y búsqueda sobre `4a14467`: no se compiló ni se ejecutó el motor, y no se modificó ningún archivo bajo `rust/`. Cada anclaje anterior se contrastó con el texto del propio archivo.

| Referencia | Qué se comprobó |
| --- | --- |
| `simulation.rs:608–677` | Rama `!is_grounded`: proceso de neumático con carga cero y `continue` en 676; el soporte `chassis_suspension_force` solo se aplica tras esa salida (`769–774`, `822/827`). |
| `simulation.rs:634–675`, `779–820` | Wrench geométrico condicionado a `is_geo && resolves_internal_link_forces()` y `coupled_context.is_none()`; sin efecto con el backend de producción. |
| `suspension.rs:526–528` | `resolves_internal_link_forces()` devuelve `backend == Standard`. |
| `simplified_suspension.rs:28–32`, `open_formula.rs:519` | Backend de producción = Simplified; `BACKEND_SUSPENSION_SIMPLIFIED` es el que se publica. |
| `simplified_axle_integration.rs:142–173`, `190`, `196–218` | Residuo por esquina; proyección contra `compression_bounds_metres` únicamente; tolerancia `2e-9`; respaldo secuencial tras 32 iteraciones. |
| `vertical_suspension_integration.rs:85–151`, `184–198` | Límite de suelo activo, `ground_is_binding`, impulso normal `>= 0` y clasificación suelo/tope del impulso aceptado. |
| `simulation.rs:1125–1126` | `total_mass` del perfil completo y `gravity_force = -9.80665 * total_mass` en la integración del cuerpo. |
| `wheel_mechanics.rs:69–77` | Masa no suspendida resuelta por inventario, multiplicada por `virtual_unsprung_mass_multiplier` (rango 0,25–3,0). |

Evidencia congelada citada en §1 (carpeta `latest_telemetry/`, no regenerada aquí):

- `runtime.log:1860` — 41,174 s, N=(0,0; 3856,0; 0,0; 6174,5) N, roll −0,0040 rad, `rejected=0`.
- `runtime.log:1865` — 41,424 s, N=(4453,6; 0,0; 4604,9; 0,0) N, roll −0,0027 rad, `rejected=0`.
- `samples.csv:166–167` — las mismas dos muestras en formato CSV (Δt = 0,250 s).
- SHA-256: `runtime.log` = `1556c74a47cc78cdabe657c372569ae9496e421c1a9af28eebb40237a0fe3517`; `samples.csv` = `b2e038dc36576f88af8db61fa0c1c4957df1777faefb4199b44d24d585a52dbb`; `capture.json` = `433307239098d591aba928d2429ea787155fe70d5d8b9c91e5711354cce1c1d3`.

Límite de esta verificación: confirma la topología del código y la procedencia del log, no el comportamiento en ejecución. Sigue pendiente de Fase A cuantificar cuántos subpasos alcanzan el fallback del eje y cuánta fuerza de suspensión no llega al cuerpo en T3.

## 8. Estado de ejecución (posterior a este plan)

Ejecutado el 22/10/2026 sobre la misma base. Informe con medidas: [phase_abc_report.md](../structural_correction/phase_abc_report.md); bancos: `rust/vehicle-physics-engine/tests/structural_contact.rs`.

- **Fase A**: cerrada a nivel de bancos. Los dos P0 quedaron reproducidos con números: la esquina levantada perdía 1310,977 N de 1334,246 N, y cada subpaso con eje apoyado agotaba 32 iteraciones y tomaba el fallback (2 por paso).
- **Fase B**: la proyección del residuo incluye ya el límite de suelo. Fallbacks 2 → 0 por paso; iteraciones 32 → 4 en reposo y 1 en conducción plana, sin cambiar las cargas estáticas.
- **Contrato de masa (§2)**: cerrado con masas físicas. Total 612,600 kg = cuerpo 546,600 kg + cuatro esquinas 66,000 kg; inercia del cuerpo (254,97; 281,50; 56,61) kg m². El cuerpo integra solo la masa suspendida y cada esquina su peso y momento. Consecuencia declarada: el coche se asienta ~3 mm delante y ~7,5 mm detrás más alto, sin tocar reglajes.
- **Fase C**: la fuerza del elemento se aplica siempre en el centro del buje y el soporte no se duplica. Identidad cuerpo/elementos verificada (379,812 N frente a 379,825 N) y el lado aéreo de la barra sigue entregando par (−633,230 N).
- **Verificación**: 8/8 bancos estructurales y 15/15 de autoridad de reglajes; caída libre del centro de masas −9,8068 m/s²; conducción plana 4/4 apoyadas y 0 fallbacks.

Pendiente antes de Fase F: término de aceleración de anclaje (resto de Fase D), barrido 1/120–1/480 con referencia independiente, geometría efectiva y casos de rayos/plano inclinado (Fase E), y la compilación release con ABI, hashes y candidato. La conducción y aceptación en T3 siguen siendo del usuario.

**Avance posterior (misma sesión):** la aceleración de anclaje ya entra en la ecuación de la esquina (predictor de un paso) y el barrido 1/240–1/480 converge a la misma altura (0,0000 m de diferencia) y velocidad (0,005 m/s). El límite de 1/120 s se cerró subdividiendo internamente los marcos del host (`MAX_INTERNAL_STEP_SECONDS = 1/240 s`, máximo 8): 1/120 s da 4/4 ruedas y 2,057 m/s, igual que 1/240 y 1/480.

**Fase F:** candidato release instalado el 22/10/2026 23:43 (SHA-256 `8c1cf079…`), ABI idéntica, respaldo y rollback en `C:\Users\bill\open-formula-backups\2026-10-22-structural-8c1cf079`, manifiesto en `implementation/reports/structural_correction/installation.json`. Perfiles y base de datos del usuario intactos. Quedan de Fase E: plano inclinado, pérdida de rayos, reset/boxes y los diez controles con el coche rodando. La conducción y aceptación en T3 son del usuario.

