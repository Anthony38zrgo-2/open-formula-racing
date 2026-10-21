# Encargo vigente: neumático sin elasticidad vertical y contacto rígido en simuv5

Fecha: 21 de octubre de 2026. Repositorio exclusivo: D:/open-formula.
Rama observada: codex/initial-port. HEAD observado: bfb708b4500efe15dba347da20af40f46a5e0fe0.

Este documento sustituye íntegramente instrucciones.md. Es un handoff para que el siguiente agente implemente y valide el cambio descrito. Las decisiones de alcance ya están confirmadas; no volver a solicitar elección entre neumático elástico y rígido.

Esta edición solo cambia la documentación. El repositorio contiene cambios de otro trabajo de autoridad física única; no atribuirlos a este encargo, descartarlos ni suponer que ya están terminados o validados.

## 1. Objetivo confirmado por el usuario

Eliminar la elasticidad y amortiguación VERTICAL del neumático. La suspensión debe aportar sus muelles, amortiguadores y demás elementos mecánicos; el neumático mantiene rozamiento, grip y el resto de su comportamiento tangencial, térmico y de desgaste.

El usuario confirmó expresamente: «Sí, neumático sin elasticidad vertical», conservando contacto rígido con el suelo y grip dependiente de la carga normal.

Esto implica:

- Sustituir el resorte/amortiguador radial por una restricción unilateral rueda-suelo resuelta dentro del solver físico único.
- Conservar la reacción normal del suelo que sostiene el coche. No eliminar N ni sustituirla simplemente por cero.
- Mantener fuerzas longitudinales/laterales, deslizamiento combinado, sensibilidad a carga, relajación tangencial, par autoalineante y resistencia a la rodadura.
- Conservar los efectos de temperatura, presión y desgaste sobre adherencia y rodadura. Retirar su influencia sobre rigidez y amortiguación vertical del neumático.
- Conservar suspensión, masas no suspendidas, combustible, inercias, siete grados de libertad y contacto del fondo.
- Conservar transferencia de carga y momentos producidos por frenada y curva. El encargo elimina la elasticidad radial, no la interacción física entre grip, chasis y suspensión.

No recuperar el neumático legacy ni el prototipo retirado tire_vertical_model. No crear un fallback silencioso al modelo elástico.

## 2. Antes de editar: estado real y trabajo concurrente

Inventariar HEAD, rama, git status, diff y archivos sin seguimiento. Respaldar fuera del árbol los archivos que se vayan a modificar, incluidos candidatos ignorados si se usan. No resetear, limpiar ni reemplazar archivos de producción completos para simplificar el cambio.

Durante esta redacción había modificaciones en CMakeLists.txt, migration_status.json, instrucciones.md, aero.cpp, car.cpp, carstruct.h, collide.cpp, sim.h, tire_vertical_solver.cpp/.h y prepare_williams_vehicle.py. También existían sin seguimiento informes, geometría del fondo, scripts y tools/validate_formula_vertical_authority.cpp. Este inventario es una observación, no una lista de archivos autorizados para borrar.

Leer primero:

- implementation/reports/vertical_single_authority.md.
- implementation/reports/vertical_floor_geometry.md, si existe.
- implementation/floor_contact_points.json, si existe.
- implementation/reports/simuv5_legacy_tire_retirement.md.
- implementation/migration_status.json.
- Los fuentes y diffs actuales de todos los archivos que se vayan a tocar.

El informe vertical_single_authority leído en esta revisión describe fase 0 y reproducción del defecto; los fuentes ya contienen tipos/entradas para contactos del chasis. No asumir que el informe cubre todos los cambios pendientes. Verificar qué fases se implementaron realmente y completar únicamente las dependencias necesarias para este objetivo, conservando el trabajo existente.

Coordinar archivos compartidos si otro agente sigue escribiéndolos. No hacer commit ni push. No trabajar en D:/Formula90s. No modificar memorias.

## 3. Arquitectura y contrato físico

### 3.1 Autoridad única

formula_vertical_state sigue siendo el estado autoritativo de heave, roll, pitch y las cuatro extensiones. Neumáticos rígidos, suspensiones, topes y contactos del fondo se resuelven en el mismo sistema antes de publicar DynGCg, DynGC y vistas de rueda.

No añadir correcciones externas de posición/velocidad, un segundo integrador de ruedas ni una copia rutinaria de DynGCg hacia el estado interno al terminar cada paso. Reset, inicialización y recolocación son transiciones explícitas y atómicas.

Preservar las decisiones del trabajo anterior:

- Contacto del fondo rígido unilateral disipativo, con geometría derivada de datos del vehículo y contrastada con el modelo visual; aproximaciones declaradas cuando falten datos.
- Retirar el clamp externo de +/-1 rad sin convertirlo arbitrariamente en tope mecánico. Diagnosticar el dominio angular del modelo.
- aero.cpp puede cambiar únicamente para asegurar lecturas coherentes del estado y alturas. No recalibrar curvas aerodinámicas.
- Conservar eventos y daños aplicables sin duplicarlos en predictor, iteraciones o subpasos.

Si estas propiedades todavía no están terminadas, son dependencias de integración del nuevo contacto. El informe final debe distinguir lo heredado, lo completado aquí y lo pendiente.

### 3.2 Contacto de rueda sin resorte radial

Ahora el soporte se obtiene de N = max(0, k_radial*d + c_radial*d_velocity), con condición de deformación positiva. Sustituir esa ley por contacto geométrico unilateral.

En la aproximación vertical plana actual:

    H_i = body_height - wheel_x_i*pitch + wheel_y_i*roll
    C_i = H_i + R_i - L_i
    gap_i = C_i - R_i - G_i = H_i - L_i - G_i

H es la referencia del chasis, C el centro de rueda, R el radio geométrico, L la extensión física y G la altura del suelo. Respetar los offsets reales respecto a statGC y las convenciones del solver. El radio efectivo de rodadura no sustituye a R en estas identidades.

Condiciones objetivo: gap >= 0, reacción normal N >= 0, sin reacción mientras la rueda se separa del suelo, y contacto activo compatible con la velocidad relativa/terreno. Esquema fijado: Euler implícito con active set de restricciones unilaterales dentro del solver acoplado (§3.4); no añadir un solver LCP separado ni una proyección posterior independiente.

- Resolver las reacciones mediante restricciones y sus Jacobianos dentro del bloque conjunto, con restitución normal nula como punto de partida para los impactos de rueda. No usar k enorme como rigidez infinita.
- En vuelo, mantener masa no suspendida, fuerzas internas y recorrido de suspensión; N=0 y fuerza de adherencia nula por falta de apoyo.
- N no equivale siempre a la fuerza del muelle: incluye la dinámica de la masa no suspendida y demás fuerzas. No copiar susp.force como carga normal universal.
- Incorporar cuatro contactos de rueda a la gestión de restricciones existente. Revisar capacidad, índices, redundancia, rango y contactos simultáneos con topes y fondo. No eliminar contactos arbitrariamente para evitar singularidades.
- Publicar la carga normal media del paso físico a partir de los impulsos aceptados, según §3.7. No publicar la fuerza del último subpaso como si fuera la media ni aplicar impulsos dos veces. Evitar filtros/clamps de carga inventados para ocultar impactos.
- Para pendientes, usar una geometría/Jacobiano coherentes. Si se conserva una aproximación de altura vertical, declarar sus límites y validarla; no llamarla contacto normal 3D completo ni perder componentes de reacción previamente existentes sin analizarlo.
- Las alturas del terreno y sus velocidades deben describir el mismo movimiento temporal en cada subpaso. Resolver esa coherencia cuando sea necesaria para la restricción nueva, sin convertir el encargo en un rediseño de selección de triángulos.

### 3.3 Grip, radio de rodadura y térmica

El modelo tangencial recibe N, contacto, velocidades y estado térmico. Consume la carga normal aceptada; no determina soporte vertical, movimiento del chasis ni recorrido de suspensión.

La deformación radial actual también afecta radio efectivo, huella, calentamiento y volumen del gas. No ponerla a cero indiscriminadamente ni prometer conservar las mismas temperaturas después de retirar su dinámica.

Implementar una separación explícita:

1. Deformación GEOMÉTRICA radial del contacto rígido: cero dentro de tolerancia, sin energía elástica ni disipación radial.
2. Magnitudes EMPÍRICAS usadas por huella/rodadura/térmica: conservar una estimación algebraica cuasiestática desde carga, presión y temperatura, con parámetros renombrados explícitamente según §3.5. No integrar una segunda carcasa ni devolver esa estimación a la geometría del centro de rueda o al solver vertical.
3. Radio geométrico fijo según datos del vehículo para contacto; radio efectivo de rodadura separado para slip y pares, con contrato único entre giro, transmisión y neumático.
4. Auditar el calentamiento por histéresis: el término actual usa velocidad de deformación. No derivar una pseudodeformación y contabilizar automáticamente su valor absoluto como calor físico sin justificar el origen de la energía. Mantener deslizamiento, convección, conducción, gas y desgaste; documentar y validar cualquier sustitución del término de histéresis y del cambio de volumen. Si falta una ley defendible, declarar la limitación en lugar de inventar calor para igualar una temperatura anterior.

Una estimación interna puede afectar grip y, a través de las fuerzas tangenciales, transferencia de carga. Eso es una interacción física permitida; lo prohibido es que actúe como resorte o amortiguador vertical adicional.

### 3.4 Decisión numérica: active set acoplado

Usar Euler implícito para velocidad/posición y active set primal-dual para ruedas, fondo y topes, ampliando el esquema existente. En cada subpaso resolver las ecuaciones de movimiento y las restricciones activas dentro del mismo sistema Newton/KKT; añadir restricciones violadas y liberar las que requieran reacción con signo inadmisible. Comprobar factibilidad, signo y complementariedad al aceptar el subpaso. Para los contactos activos, imponer el hueco final con la cinemática del terreno coherente; las restricciones de recorrido conservan su convención de signo documentada.

Justificación: permite reutilizar las ecuaciones de suspensión no lineal y la autoridad única actuales, y evita otro solver físico fuera del bloque. La complementariedad es una condición de aceptación; no obliga a usar un algoritmo LCP separado. Restitución de rueda nula y sin resorte de penalización. Registrar tolerancias escaladas, iteraciones y cambios del conjunto activo; detectar ciclos y fallos explícitamente.

### 3.5 Decisión de configuración: retirar dinámica radial y migrar calibración

- Eliminar del contrato vertical y del XML generado rigidez radial, radial damping ratio y sus modificadores mecánicos. No conservar nombres que aparenten controlar soporte vertical.
- Conservar la calibración algebraica útil con nombres nuevos y unidades explícitas: empirical load compliance (m/N), empirical deformation limit (m), rolling radius load deflection factor (adimensional) y contact patch empirical deflection gain (adimensional). La compliance se obtiene inicialmente como el inverso de la antigua rigidez de referencia; el límite y los factores se migran sin ajuste de tuning.
- La deformación empírica se calcula algebraicamente desde la carga normal media y la compliance, con su límite, sin velocidad radial integrada. Conservar las dependencias existentes de presión/temperatura sobre esta estimación solo como calibración de huella, radio de rodadura y volumen del gas. Ninguno de estos campos entra en el solver vertical, la matriz de masa ni la geometría de apoyo.
- La amortiguación radial antigua se elimina sin parámetro empírico sustituto. El término térmico asociado a velocidad radial deja de representar una pérdida mecánica activa: retirarlo de esa contabilidad y declarar el cambio de fuente de calor. No reemplazarlo por calor ficticio calculado con la derivada absoluta de la deformación empírica. Las demás fuentes térmicas y el balance permanecen; cualquier futura recalibración de calor de rodadura es otro alcance.
- Implementar migración explícita de XML y prepare_williams_vehicle.py, con informe de campos convertidos/eliminados. En ejecución, rechazar configuración antigua no migrada con mensaje que indique cómo migrarla; no ignorarla ni convertirla silenciosamente. Configuración nueva y antigua mezcladas también se rechazan. La herramienta de migración debe ser idempotente.
- Probar que variar estos campos empíricos cambia únicamente sus salidas declaradas y, manteniendo fuerzas tangenciales prescritas, no altera la respuesta vertical.

### 3.6 Política de rango, redundancia y contactos incompatibles

No asignar una prioridad física artificial de rueda sobre fondo o fondo sobre tope. Todas las desigualdades físicas se verifican al aceptar el paso. El ejemplo de cuatro ruedas, tres puntos de fondo y un tope debe estar cubierto por un ensayo explícito; ocho restricciones candidatas no equivalen necesariamente a ocho condiciones independientes en siete coordenadas.

Escalar variables/filas y usar factorización con detección de rango (QR con pivoteo o SVD), con tolerancias relativas justificadas. Resolver una base independiente para obtener el movimiento. Conservar todas las desigualdades para validar factibilidad; una fila redundante no pierde su significado físico. Reconstruir reacciones admisibles de mínima norma en unidades físicas, sujetas a signos y al impulso generalizado requerido, incluyendo todos los contactos equivalentes. No obtener las cargas publicadas de la elección arbitraria de pivotes. Usar orden estable de identificadores para desempates y exigir invariancia al permutar el orden de los contactos.

Regularización elegida: tratamiento algebraico del rango, sin añadir epsilon a la masa/KKT como compliance oculta y sin ablandar huecos o topes. Una reconstrucción sin reacciones admisibles obliga a revisar el active set; no recortar multiplicadores después de resolver.

Si las restricciones son físicamente incompatibles o el subpaso no converge, subdividir adaptativamente el intervalo hasta un mínimo documentado y probado. Si sigue fallando, devolver fallo explícito, conservar el último estado aceptado completo y detener la evolución del vehículo afectado de manera diagnosticable. No avanzar el tiempo de ese vehículo como si el intervalo hubiese tenido éxito, descartar solo el contacto incómodo ni publicar un estado parcial. El caso y el contador de fallo deben quedar visibles; no equivale a conducción aprobada.

### 3.7 Definición única de N y consumo por grip/térmica

Por rueda y subpaso aceptado s: J_s es el impulso de contacto del intervalo y N_s = J_s/h_s su fuerza media. Si el solver usa multiplicadores con unidades de fuerza, convertir una sola vez J_s = lambda_s*h_s. Predictor, iteraciones y subpasos rechazados no se acumulan.

Para el paso físico completo de duración dt, publicar N_step = sum(J_s)/dt. Es la media del intervalo, no la fuerza instantánea final. Grip y térmica reciben exactamente esta misma N_step y la misma duración dt; mantener una única actualización aceptada de cada uno por paso físico en esta implementación. Registrar por separado N_s, impulsos, contacto al final y fracción temporal de apoyo. No filtrar N para uno de los consumidores y dejarla sin filtrar para el otro.

El estado de contacto al final y el apoyo durante el intervalo son datos distintos: un intervalo puede terminar en vuelo después de un impacto. La evaluación de fuerzas medias del intervalo debe reconocer el impulso que sí ocurrió, mientras que el siguiente paso con hueco positivo y sin nuevo contacto debe producir N=0 y ningún grip residual. Adaptar los flags de entrada del modelo para no descartar el apoyo del intervalo usando únicamente el flag final ni prolongarlo al intervalo siguiente. Añadir ambos casos al banco de integración.

La evaluación no lineal del grip y la térmica con carga media es una aproximación explícita de este alcance; no afirmar que equivale a integrar sus leyes por subpaso. Comprobar convergencia al reducir el paso físico completo, además del subpaso vertical. Para dt=0, conservar el estado sin dividir, generar impulso o acumular calor nuevos. En equilibrio inicial, calcular la carga estática desde la solución de restricciones, sin inventar un impulso de inicialización.

### 3.8 Instalación y aceptación: responsabilidades fijadas

Se espera que el agente, tras superar bancos e integración, instale localmente DLL y XML candidatos en la distribución para dejar preparada la prueba manual. Está dentro del encargo; no requiere repetir una confirmación de instalación reversible. Antes, verificar las rutas efectivas y que no haya una sesión usando esos archivos; no cerrar una partida del usuario sin autorización. Si está abierta, preparar el candidato y declarar instalación pendiente hasta disponer de los archivos.

Respaldar DLL/XML existentes fuera del árbol, registrar hashes antes/después, conservar reglajes ajenos, verificar que el ejecutable carga el candidato y entregar instrucciones concretas de reversión. Un fallo de bancos no autoriza instalar. Esto no autoriza commit, push ni publicación remota.

El agente puede realizar bancos, integración y pruebas instrumentadas disponibles, y entregar estados separados: validación técnica, instalación y conducción. La puerta D requiere revisión expresa del usuario y el agente no puede autoconcederla ni reutilizar una aprobación de otra versión. Sin esa revisión, entregar el candidato instalado y técnicamente validado, con «aceptación humana de conducción pendiente» y los escenarios que faltan. No dejar todo el trabajo técnico sin terminar esperando esa revisión, ni declarar el encargo completamente cerrado cuando falte.

## 4. Archivos y responsabilidades

Los caminos siguientes son relativos a D:/open-formula.

| Archivos | Trabajo requerido |
| --- | --- |
| src/modules/simu/simuv5/tire_vertical_solver.cpp/.h | Sustituir soporte radial por contactos rígidos de rueda. Adaptar fuerzas, residuos, Jacobianos, restricciones, equilibrio inicial, validación de configuración, diagnósticos y energía. Mantener contacto de fondo y topes. Revisar también la implementación quarter-car del mismo archivo para que los bancos representen el modelo vigente. |
| src/modules/simu/simuv5/car.cpp | SimCarBuildVerticalConfiguration deja de generar rigidez/amortiguación radial desde modificadores térmicos. Adaptar entradas, publicación, cargas/contactos y acoplamiento provisional/final. Mantener coherencia de estado y vistas. |
| src/modules/simu/simuv5/tire_model.cpp | Adaptar reset y SimTireModelUpdateWheelWithVerticalState: no inicializar un aplastamiento físico N/k. Consumir contacto rígido y N; separar magnitudes empíricas y evitar reescrituras físicas independientes. |
| src/modules/simu/simuv5/tire_contact_model.cpp/.h | Separar huella, sensibilidad a carga y radio efectivo de la deformación geométrica vertical. Retirar salidas que aparenten controlar una suspensión radial activa. |
| src/modules/simu/simuv5/tire_model_configuration.cpp/.h | Aplicar la política fija de §3.5: retirar parámetros dinámicos radiales, validar los campos empíricos nuevos y rechazar configuración antigua o mezclada sin migración explícita. |
| src/modules/simu/simuv5/tire_model_state.h | Si hace falta, distinguir estado rígido de contacto y estimaciones empíricas sin añadir una masa radial integrada. |
| src/modules/simu/simuv5/tire_thermal_model.cpp/.h | Revisar histéresis, volumen y modificadores para que la térmica conserve coherencia energética y no reintroduzca soporte vertical. |
| src/modules/simu/simuv5/tire_vertical_geometry.cpp/.h | Si hace falta, exponer hueco/velocidad de contacto rígido y separar radios y vistas geométricas. |
| tools/prepare_williams_vehicle.py y configuración candidata del Williams | Migrar configuración reproducible. Identificar antes qué archivos están ignorados y cómo se genera el XML instalado; no editar solo el XML generado. Conservar reglajes ajenos. |
| tools/validate_formula_tire_vertical_system.cpp | Actualizar quarter-car y coche completo al modelo rígido. Reemplazar expectativas basadas en aplastamiento radial; mantener las comprobaciones de suspensión nativa. |
| tools/validate_formula_tire_contact.cpp, validate_formula_tire_geometry.cpp, validate_formula_tire_forces.cpp, validate_formula_tire_energy.cpp y validate_formula_tire_thermal.cpp | Adaptar contratos y añadir regresiones de contacto, tangencial y térmica. Verificar nombres actuales antes de editar. |
| tools/validate_formula_vertical_authority.cpp | Integrar las invariantes con el nuevo contacto de rueda y los contactos de fondo, en el orden real de actualización. |
| CMakeLists.txt y src/modules/simu/simuv5/CMakeLists.txt | Ajustar solo targets o fuentes necesarios. |
| implementation/reports/rigid_tire_contact.md (nuevo) y implementation/migration_status.json | Registrar diseño, archivos, resultados medidos, artefactos y límites; separar validación nativa, integración y conducción. |

Revisar carstruct.h, sim.h, simu.cpp, wheel.cpp, collide.cpp, aero.cpp y tire_force_model.cpp para contratos y dependencias. Modificarlos únicamente donde la integración lo requiera. No cambiar curvas de grip, aero o reglajes para compensar un error de contacto. No tocar simuv2/simuv4.

## 5. Orden de ejecución

1. Registrar y respaldar la base real. Identificar el estado del trabajo de autoridad única y reproducir los bancos relevantes antes de introducir cambios.
2. Definir contrato de contacto rígido, radios y N. Añadir casos analíticos de quarter-car: equilibrio, vuelo, impacto y separación. Fijar tolerancias justificadas antes de comparar el candidato.
3. Implementar restricción de rueda dentro del solver, retirar fuerzas/energía radial y adaptar equilibrio. Preservar masas e inercias. Resolver redundancias y fallos de convergencia sin publicar estados parciales.
4. Integrar cuatro ruedas, topes y contactos de fondo; actualizar adaptador, predictor/final y diagnósticos. Aplicar únicamente reacciones y eventos del resultado aceptado.
5. Separar las magnitudes de huella/rodadura/térmica; actualizar configuración, reset y bancos. Mantener contratos físicos y energéticos explícitos.
6. Ejecutar regresiones e integración. Comparar candidato y base con iguales condiciones; explicar diferencias esperadas por quitar la elasticidad radial.
7. Compilar candidato, verificar procedencia y probar en simulación. Instalar DLL/XML locales tras superar bancos e integración conforme a §3.8, con respaldo, hashes y vuelta atrás. No declarar conducción validada por compilar o llegar a RACE.
8. Entregar el informe y actualizar el estado real. Mantener pendientes visibles si falta alguna puerta; no usar aprobaciones de alta velocidad de versiones anteriores.

## 6. Pruebas obligatorias

### A. Contacto rígido y suspensión

- Equilibrio estático: suma de reacciones y momentos compatible con peso, fuerzas externas y apoyos. gap dentro de tolerancia; deformación radial geométrica cero. Sin deriva de heave/roll/pitch.
- Comparar equilibrio con la base: documentar el cambio de altura al desaparecer el aplastamiento radial. No añadir offsets ni retocar precarga silenciosamente para esconderlo.
- Cambiar los antiguos parámetros radiales del solver, incluidos modificadores térmicos: la respuesta vertical debe permanecer igual bajo idénticas entradas mecánicas y fuerzas tangenciales prescritas. Si se conservan parámetros empíricos de grip, probar su aislamiento en lugar de exigir que el grip sea idéntico al cambiarlos.
- Cambiar muelle/amortiguador de suspensión: debe cambiar la respuesta según el modelo nativo. Mantener bellcrank, barras y tercer elemento coherentes.
- Vuelo: N=0, sin adherencia; rueda y suspensión siguen evolucionando. Separación sin atracción ni pegado artificial.
- Impacto/aterrizaje simétrico y de una rueda: impulsos y signos correctos, sin rebote elástico radial añadido; comprobar referencia analítica en el caso sencillo.
- Topes sin contacto del fondo, contacto del fondo fuera de topes y contacto combinado de ruedas/topes/fondo: restricciones válidas, sin singularidades no tratadas ni multiplicación de daños.
- Pendientes, cambio de rasante, piano de una rueda y apoyo sobre terreno móvil: geometría y trabajo del terreno coherentes. Verificar el dominio angular y documentar límites.
- Jacobianos por diferencias finitas en regímenes suaves y pruebas separadas de activación/desactivación. Redundancias: solución física admisible aunque el reparto individual de multiplicadores no sea único.
- Caso obligatorio de rango: cuatro ruedas, tres puntos de fondo y un tope; repetir permutando los contactos y perturbando ligeramente la geometría. Verificar movimiento, impulso generalizado y reparto admisible reproducible, además de fallo explícito para geometría incompatible.
- Tiempo cero conserva estado sin impulso/daño nuevos. Entradas inválidas y fallo de convergencia no mezclan estado anterior con vistas parcialmente nuevas.

### B. Grip, giro y térmica

- Con N, velocidades, radio efectivo, huella y estado térmico iguales, comparar Fx, Fy, slip, relajación, par autoalineante y par de reacción con la base. Los cambios de carga debidos al nuevo contacto deben analizarse aparte del cambio de ley tangencial.
- Aceleración, frenada, bloqueo, patinaje y curva: límites de adherencia y transferencia de carga coherentes, sin suprimir momentos de fuerzas tangenciales.
- Radio de rodadura consistente entre velocidad periférica, slip, torque y transmisión; radio geométrico del contacto independiente de la estimación empírica.
- Temperatura/presión/desgaste siguen afectando grip y rodadura; no afectan rigidez ni amortiguación vertical. Probar este aislamiento con fuerzas tangenciales controladas.
- No contabilizar energía elástica ni disipación del antiguo resorte radial. Contabilizar pérdidas de impacto, amortiguadores, trabajo externo y del suelo por separado. No exigir energía decreciente cuando existe trabajo externo positivo.
- Validar balance térmico y energía de deslizamiento, y explicar el tratamiento elegido para histéresis y volumen. No llamar conservada a una térmica que simplemente dejó de calentarse al fijar deformación cero.
- Migración XML idempotente, rechazo de campos antiguos/mezclados, generación reproducible y aislamiento vertical de los parámetros empíricos. Comprobar N_step = sum(J_s)/dt, entrada idéntica de N a grip/térmica y casos de apoyo parcial seguido de vuelo sin fuerza residual.

### C. Integración, convergencia y regresiones

- Al finalizar TODO el paso y al comenzar el siguiente: DynGCg.pos.z = body_position[0] + statGC.z; ángulos/velocidades y DynGC coinciden con las conversiones del estado autoritativo. Aplicar tolerancias acordes con float/double reales.
- Comprobar las dos rutas de actualización, inicialización, reset, cambio de reglaje y retorno a boxes. Sin correcciones externas de SimCarCollideZ ni clamp angular que dejen otra pose interna.
- Verificar que aero consume alturas/velocidades coherentes y que predictor, Newton y subpasos no publican ni acumulan fuerzas/daños múltiples.
- Comparar dt/subpasos de 2 ms, 1 ms y 0.5 ms con el mismo movimiento temporal del terreno. Medir hueco, impulso total, N utilizada por grip, recorrido, energía y estados finales. No comparar una excitación distinta al variar dt.
- Ejecutar bancos actuales de vertical, geometría, contacto, fuerzas, selector, térmica, energía, chasis, aerodinámica y autoridad única. Registrar comandos, códigos de salida y resultados nuevos; no reutilizar totales históricos.
- Build observado: C:/Users/bill/open-formula-build/engine, fuente D:/open-formula. Verificar toolchain/targets actuales; preservar CMAKE_DISABLE_FIND_PACKAGE_SOLID=TRUE si se reconfigura. Compilar simuv5 y todos los bancos afectados.

### D. Conducción y cierre

- Williams/Fuji: reposo, baja velocidad, aceleración, frenada fuerte, curva sostenida, alta velocidad, pianos, vuelo/aterrizaje, rasante y contacto del fondo. Comparar condiciones y reglajes reproducibles con la base.
- Registrar por paso: dt, hueco y contacto por rueda, impulsos, N utilizada por grip, recorrido/velocidad de suspensión, pose y velocidad interna/publicada, fuerzas tangenciales, contactos del fondo, energía, residual/iteraciones, temperatura, presión y desgaste.
- No afirmar de antemano que el contacto rígido elimina todos los rebotes: puede transmitir irregularidades con mayor brusquedad. Aceptar el cambio por cumplimiento del modelo solicitado y validación, no por ausencia de un resorte en el código.
- Puertas de cierre: contacto rígido sin soporte radial elástico, suspensión operativa, grip conservado bajo entradas equivalentes, coherencia energética, autoridad única, regresiones e integración aprobadas y revisión humana de conducción. Las puertas que falten deben quedar declaradas pendientes.

## 7. Entrega y límites

Entregar lista exacta de archivos, formulación adoptada, contratos de N/radios/huella/térmica, comandos y resultados, comparación con la base, hashes de DLL/XML probados, trazas originales, procedimiento de vuelta atrás y pendientes.

No añadir tuning de muelles, amortiguadores, potencia, aero o curvas de grip para compensar el cambio sin una justificación y alcance separados. No rediseñar la selección de triángulos ni reintroducir una carcasa vertical integrada. Este encargo autoriza implementar el modelo confirmado y sus dependencias de coherencia; no autoriza commit ni push.
