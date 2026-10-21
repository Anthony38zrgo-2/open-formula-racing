# Encargo revisado: cerrar la Etapa B del contacto vertical del Williams

Estado al publicar: las Puertas 1 y 2 y la integración de Puerta 3 ya están implementadas; consultar `implementation/reports/williams_idle_oscillation_stage_b_gate3.md` y `solver_publication_validation.md` antes de retomar este encargo. No reimplementar desde cero las etapas completadas. La conducción sostenida a alta velocidad y la aceptación humana siguen pendientes. El prototipo anterior `tire_vertical_model` permanece fuera de la publicación; el solver integrado es `tire_vertical_solver`.

Fecha: 20 de octubre de 2026. Repositorio: D:/open-formula.
Rama y HEAD comprobados: codex/initial-port, 8531ccb1de40a79390f6140ee2eae4fb08f8cf54.

Este documento reemplaza íntegramente las instrucciones anteriores. El siguiente agente debe implementar y validar una solución al acoplamiento rueda–suspensión–chasis. La propuesta concreta es un sistema vertical conjunto de siete grados de libertad, con extensión física de suspensión como coordenada de rueda y centro de rueda derivado geométricamente. Es una propuesta fundamentada, NO un solver ya validado en el juego.

Esta revisión solo cambia instrucciones.md. No modifica física, configuración, DLL ni distribución. La producción debe continuar en la referencia estable de Etapa A hasta que la nueva integración supere las pruebas.

## 1. Punto de partida: Etapa A terminada, Etapa B experimental

Leer primero los fuentes actuales y estos informes:

- implementation/reports/williams_idle_oscillation_audit.md.
- implementation/reports/williams_idle_oscillation_stage_a.md.
- implementation/reports/williams_idle_oscillation_stage_b.md.
- implementation/migration_status.json.

Estado verificado al redactar:

- Selector corregido: legacy y transient coupled ya son modelos efectivos distintos. No rehacer ni perder esta corrección.
- Contacto de referencia de 8531ccb restaurado; reset en equilibrio conservado.
- Candidato y distribución en transient coupled, presión de prueba de 130 kPa manométricos, telemetría normal desactivada. Conservar este estado, no recuperar los antiguos 160 psi.
- tire_vertical_model.cpp/.h existen como experimento y se compilan en el banco aislado; no están enlazados al simuv5 instalado.
- SHA-256 DLL instalada: B14FB36D0286DCE4FA6F0A53531C214BB911DE56224AD84F4A5658353DBC9927.
- SHA-256 XML candidato e instalado: 66FD0A8D05AF00C291C4647D0DDEC3A401B1D51423871AE49BA68B3CBCFDA426.

Referencia de Etapa A, tomada del informe, sin repetir conducción en esta revisión:

| Modelo | Balanceo RMS | Carga delantera | Pasos con carga menor de 1 N |
| --- | ---: | ---: | ---: |
| Legacy real | 0.000411 grados | 1282.9–1386.5 N | 0 % |
| Acoplado real | 0.000123 grados | 1270.3–1404.9 N | 0 % |

Hay numerosos cambios previos sin commit: CMakeLists.txt, migration_status.json, informe del modelo, instrucciones.md, car.cpp, tire_model.cpp y wheel.cpp; además de informes, solver y bancos sin seguimiento. Volver a inventariar rama, HEAD, status, diff y archivos sin seguimiento. Respaldarlos externamente antes de editar. No resetear, limpiar ni sustituir archivos completos. No trabajar en D:/Formula90s.

## 2. Por qué el intento anterior no basta

No aceptar «solo es un offset geométrico» como diagnóstico cerrado. Hay varios contratos incompletos comprobados en los fuentes y en las pruebas de esta revisión.

### 2.1 Referencia de altura y centro de rueda son distintos

SimCarUpdateWheelPos calcula wheel->pos.z desde la pose del chasis. Es una referencia del chasis en la posición horizontal de la rueda; NO es la altura del centro físico de rueda.

En la variante descartada, stage-b-attempt/wheel.cpp utilizaba max_extend - static_deflection como total_travel y decidía contacto por max_extend > 0. Ninguna condición expresa por sí sola el hueco real entre neumático y carretera. El signo de una altura del chasis no determina si la rueda toca el suelo.

### 2.2 susp.v no significa siempre velocidad geométrica

En SimWheelUpdateRide se reconstruye una velocidad a partir del cambio de extensión. Después, SimWheelUpdateForce modifica susp.v con una reacción del tipo -dt*force/mass. Al terminar el paso es una velocidad predictora del esquema legacy, no necesariamente la derivada física de la extensión observada.

La velocidad RMS aproximada de 0.206 m/s citada para el reposo de Etapa A es compatible con ese impulso: 1339 N * 0.002 s / 13 kg es aproximadamente 0.206 m/s. No utilizarla como evidencia de que la rueda oscila a esa velocidad, ni copiar -susp.v al estado de velocidad de compresión de un integrador nuevo.

La variante descartada sí inicializaba compression_velocity_meters_per_second = -wheel->susp.v. Además omitía movimiento del chasis, carretera y bellcrank en esa conversión. Sustituir ese puente por un estado físico propio, definido durante todo el paso.

### 2.3 El banco no reproduce la suspensión nativa ni certifica energía

El callback del banco usa fuerza = +k*travel + c*velocity. La suspensión nativa tiene spring.K negativo y pierde fuerza al aumentar la extensión, con precarga. Su variable de velocidad es de compresión y tiene escalado por bellcrank. Los modelos no son intercambiables.

Prueba nueva ejecutada fuera del repositorio: cambiar únicamente la perturbación de +2 mm a -2 mm hace fallar «vertical perturbation energy non increasing», aunque sigue convergiendo. Por tanto, la suma de energías usada por ese ensayo no certifica el balance del sistema y no basta con su 10/10 original. No interpretar ese fallo aislado como prueba de creación neta de energía del coche: demuestra una validación insuficiente/inconsistente.

También se probó elapsed_seconds = 0 con total_travel negativo, compresión y velocidad no nulas: el solver devuelve ceros y falla ambas comprobaciones de conservación del estado. Su retorno de «sin contacto» precede a la comprobación de tiempo cero.

Evidencias de estas pruebas y copia de las instrucciones anteriores:
D:/open-formula-migration-evidence/vertical-handoff-replan-20261020/.
Contiene lower-perturbation-results.txt, zero-elapsed-airborne-results.txt, fuentes de los probes, hashes del árbol previo y proposed-mass-matrix-check.json. Ninguna DLL experimental se instaló en esta revisión.

## 3. Solución propuesta: geometría explícita y una sola coordenada física por rueda

### 3.1 Convención inicial para plano horizontal y ángulos pequeños

Definir por rueda, en metros y segundos:

- H: altura de la referencia del chasis que actualmente publica wheel->pos.z.
- G: altura REAL de carretera, sin desplazarla por deflexión.
- R: radio geométrico descargado. No confundirlo con radio efectivo de rodadura.
- L: extensión física de suspensión, sin bellcrank; positiva cuando la rueda baja respecto al chasis.
- C: altura mundial del centro físico de rueda.
- d: deformación radial, positiva al comprimir el neumático.
- b: bellcrank de la suspensión.

La identidad compatible con la representación actual, para el modelo vertical plano, es:

    C = H + R - L
    d_signed = G + R - C = L - (H - G)
    L = H - G + d_signed
    C_velocity = H_velocity + R_velocity - L_velocity
    d_velocity = G_velocity + L_velocity - H_velocity

La última identidad cancela R porque el mismo radio geométrico aparece en centro y contacto; si se hace R variable hay que mantener consistente toda la parametrización. El radio efectivo de rodadura puede depender de d, pero no volver a introducirlo en estas identidades como si fuera otro radio geométrico.

Ejemplo de prueba: H-G = 0.050 m, R = 0.312 m y d = 0.0086 m implican L = 0.0586 m y C-G = 0.3034 m. No restar otros 8.6 mm «de referencia». Este ejemplo comprueba la geometría, no impone la altura de equilibrio final del Williams.

Guardar como estado autoritativo L y L_velocity, más los tres estados verticales del chasis. C y d se calculan desde la misma geometría. Alternativamente se puede guardar C y su velocidad con una transformación equivalente, pero no integrar independientemente C, L y d y corregir después sus discrepancias.

No deducir ausencia de contacto de H-G <= 0 ni de total_travel <= 0. Mantener el hueco firmado aunque d física publicada sea max(d_signed, 0). En vuelo se sigue integrando la rueda y su suspensión; no borrar posición/velocidad ni quitar las fuerzas internas de suspensión. Contacto unilateral solo impide tracción de la carretera.

### 3.2 Adaptador hacia simuv5

En la ruta nueva, publicar después de resolver:

    susp.x = b * L
    susp.v = -b * L_velocity
    susp.a = +b * L_acceleration, si se conserva su definición legacy (previous_v - new_v)/dt

La aceleración FÍSICA de compresión es -b*L_acceleration; no confundirla con susp.a legacy, cuyo cálculo tiene el signo contrario a la derivada de susp.v. Mantener esa distinción explícita en el adaptador y evaluar inerters mediante la dinámica derivada, no copiando su término legacy con un signo supuesto.

Son vistas compatibles para la ruta nueva, no autorización para continuar después con el predictor/impulso legacy. Evitar segunda integración de susp.v, desplazamiento artificial de G y doble escalado/clamp mediante SimSuspCheckIn. Evaluar límites físicos dentro de la resolución y publicar luego el resultado.

Calcular H_velocity con la derivada de la MISMA transformación usada para H: traslación, roll y pitch. En la aproximación actual:

    H = chassis_height - GC_height - wheel_x*sin(pitch) + wheel_y*sin(roll)
    H_velocity = chassis_vertical_velocity
                 - wheel_x*cos(pitch)*pitch_velocity
                 + wheel_y*cos(roll)*roll_velocity

Usar wheel_x/wheel_y después del recentrado al centro de gravedad, no los xpos brutos del XML. En el Williams nominal, delante wheel_x es aproximadamente +1.60395 m y detrás -1.28605 m; verificar los valores cargados. Incluir los términos adicionales que requiera una transformación más completa si se cambia la convención.

La malla debe muestrearse en las coordenadas horizontales correctas y con una referencia de altura que seleccione la superficie prevista. TrackGroundMesh elige entre alturas candidatas por distancia a reference_height: no sustituir sin comprobar ese argumento por otro punto que pueda elegir otra capa de pista.

Las ecuaciones anteriores son el primer banco PLANO. Para Fuji, definir distancia/contacto respecto a la normal local y el Jacobiano correspondiente, o justificar explícitamente la aproximación de altura vertical. Diferenciar fuerza normal de su componente vertical. Si se aplican fuerzas orientadas con la normal, evitar duplicar la aproximación de pendiente/gravedad ya presente en car.cpp. No certificar inclinaciones o pianos solo por pasar el banco plano.

## 4. Dinámica propuesta: bloque conjunto de siete grados de libertad

Resolver juntos heave, roll y pitch del chasis y las cuatro extensiones L. El problema no se cierra resolviendo cuatro ruedas con el chasis inmóvil y aplicando después fuerzas a una masa distinta.

No añadir cuatro masas gravitatorias a un car->mass que ya representa el coche completo ni cambiar globalmente car->mass/Minv para remediarlo. Preservar la masa total con combustible y las inercias de referencia. La opción recomendada es una matriz de masa en coordenadas relativas que reproduzca la inercia rígida actual cuando no cambia L.

### 4.1 Derivación de referencia para ángulos pequeños

Sea body_velocity = (vertical_velocity, roll_velocity, pitch_velocity).
Para cada rueda, J_i = (1, wheel_y, -wheel_x) y la velocidad vertical de centro es J_i*body_velocity - L_velocity_i.
Sea D la matriz diagonal de las cuatro masas no suspendidas y J la matriz de cuatro filas J_i. Sea M_reference la matriz rígida 3x3 de masa total e inercias verticales actuales, con combustible conforme al contrato vigente.

La energía cinética propuesta se obtiene separando la contribución vertical de las ruedas inmovilizadas y añadiendo su movimiento real:

    T = 1/2 * body_velocity^T * (M_reference - J^T*D*J) * body_velocity
        + 1/2 * (J*body_velocity - L_velocity)^T * D
                * (J*body_velocity - L_velocity)

Equivale a la matriz conjunta:

    M = [ M_reference    -J^T*D ]
        [ -D*J             D   ]

Con L_velocity = 0 se recupera exactamente M_reference; no se cuenta dos veces la misma masa de rueda. Exigir que M_reference - J^T*D*J sea definida positiva. No arreglar una matriz incompatible recortando autovalores o inventando masa.

La comprobación algebraica nominal con los datos actuales del Williams dio matriz conjunta y resto definidos positivos; el menor autovalor numérico de la matriz conjunta fue aproximadamente 9.665 en las coordenadas elegidas. Esto solo comprueba viabilidad algebraica nominal: no es validación del solver, de sus unidades de tolerancia ni del coche completo.

Para ángulos finitos usar el Jacobiano coherente con la geometría y los términos cinemáticos/convectivos derivados de T. No sustituir J por cosenos variables y omitir sin más sus derivadas. La extensión inicial puede usar la aproximación de pequeños ángulos del motor, pero debe declararla y validarla.

### 4.2 Fuerzas y trabajo de referencia

Para una rueda, definir N como fuerza de carretera hacia arriba y S como fuerza de suspensión hacia arriba sobre el chasis, equivalente a fuerza hacia abajo sobre la rueda. En el banco plano:

    N = max(0, tire_stiffness*d + tire_damping*d_velocity), con contacto geométrico
    Q_contact_body = J_i^T * N
    Q_contact_extension_i = -N
    Q_suspension_extension_i = +S
    Q_gravity_body = (-total_mass*g, 0, 0), para la referencia plana centrada
    Q_gravity_extension_i = +wheel_mass_i*g

Los términos de masa acoplados realizan la transferencia entre chasis y rueda. No sumar además +S al cuerpo después de haber incluido esas fuerzas generalizadas. Una formulación en centros mundiales y masa suspendida separada puede ser equivalente, pero debe demostrarlo y mantener masa, gravedad y centro de gravedad; no mezclar ambas representaciones.

Comprobar primero el caso de un cuarto de coche: la transformación debe producir las ecuaciones físicas equivalentes

    wheel_mass * center_acceleration = N - S - wheel_mass*g
    sprung_mass * chassis_acceleration = S - sprung_mass*g

En equilibrio, N = S + wheel_mass*g. No imponer N = S después de introducir una masa no suspendida física con gravedad. Integrar gravedad, precarga y posición de equilibrio, sin offsets de fuerza elegidos para hacer pasar el ensayo.

El callback de suspensión debe evaluar la ley NATIVA de muelle, precarga y amortiguador en L y -L_velocity con su escalado correcto. No usar +k*L del banco anterior. Crear una evaluación pura que no cambie susp.x/v/a, fuerza previa, daños ni estados durante Newton/iteraciones. Conservar el comportamiento legacy por su ruta actual. Si inerter/inertance participa, incluirlo coherentemente en las ecuaciones, no como aceleración retrasada inadvertida.

Barras estabilizadoras y heave deben pertenecer al mismo sistema: energía/gradientes y fuerzas de cada lado derivados de una convención única, sin calcular primero un lado con estados viejos y el otro con nuevos. No añadir un multiplicador de bellcrank por intuición a SimAxleUpdate; verificar el trabajo y el convenio nativo, especialmente con b distinto de uno.

### 4.3 Resolución numérica y restricciones

Usar Euler implícito como primera implementación, con posición y velocidad del mismo extremo temporal. Resolver el residuo conjunto 7x7 mediante Newton con control de paso o un método equivalente, y conjuntos activos para cambios de tramo del amortiguador, contacto y topes. Cuatro bisecciones escalares independientes no bastan para cerrar el acoplamiento del chasis y las barras.

Los límites de extensión y compresión necesitan reacciones/impulsos consistentes sobre ambos cuerpos y energía perdida contabilizada. No resolver y después recortar por separado L y d rompiendo C = H+R-L. No recortar S a cero indiscriminadamente: la condición unilateral pertenece a la carretera; la suspensión sigue acoplada en vuelo.

Comprobar elapsed_seconds <= 0 antes de cualquier proyección/reset de estados: devolver el estado sin cambios, o un error explícito para entrada inválida sin modificar producción. No anular estados de vuelo como efecto colateral.

Separar convergencia de Newton de estabilidad física. Registrar residuo, iteraciones, impulsos y causa de fallo. Un fallo de convergencia no autoriza aplicar a medias un estado ni cambiar silenciosamente a legacy durante una sesión.

## 5. Integración concreta, conservando la Etapa A

1. Mantener intacta la ruta de referencia y añadir una selección explícita y temporal de integración vertical por vehículo, distinta del selector de neumáticos. El valor normal seguirá siendo reference hasta superar la aceptación. No hacer el cambio nuevo predeterminado para todos los coches.
2. Sustituir la interfaz ambigua total_travel/compression del experimento por entradas/salidas con alturas, extensiones y velocidades inequívocas. Estado de tamaño fijo, compatible con la reserva y copia C de tCar/tWheel.
3. Factorizar adaptador geométrico y evaluación pura de suspensión para que el banco y simuv5 llamen al MISMO código. No mantener una geometría simplificada en el banco y otra sin probar en wheel.cpp.
4. Implementar primero el bloque de cuarto de coche; después el bloque de cuatro ruedas con heave/roll/pitch. Registrar equilibrio y energía antes de conectarlo a Fuji.
5. Centralizar la integración vertical del coche una vez por paso, en las dos rutas SimUpdate y SimUpdateSingleCar. Preparar las cuatro ruedas antes de resolver. La ruta nueva NO debe ejecutar después la integración vertical legacy de rueda ni la de heave/roll/pitch del chasis. Traslación horizontal, guiñada, transmisión, frenos y giro de ruedas mantienen su actualización prevista.
6. Separar fuerzas para consumidores: el neumático, térmica/desgaste y HUD reciben N de contacto; el solver conserva S y las reacciones internas por separado. wheel->forces.z no puede representar alternativamente N y S según el consumidor sin un adaptador explícito. Evitar aplicar contacto y suspensión dos veces al chasis.
7. Incluir cargas externas de aero y momentos existentes una sola vez en las fuerzas generalizadas. Definir el orden de fuerzas laterales/longitudinales y sus momentos respecto al bloque vertical. Si hay iteraciones de evaluación, usar estados provisionales: no avanzar memoria de slip, térmica, desgaste ni transmisión en cada intento.
8. Una vez aceptado el estado mecánico, actualizar el resto del neumático una vez por paso. La carcasa nueva tiene una sola fuente de deformación y velocidad: no volver a ejecutar el oscilador de TireContactModelUpdate encima del nuevo solver. Separar evaluación de radio/huella/modificadores de integración vertical. Mantener el oscilador solamente en la ruta reference.
9. Inicializar con un equilibrio conjunto de gravedad, suspensión precargada, contacto y geometría cargada. No inicializar velocidad nueva desde el susp.v predictor legacy. Un cambio de neumático/reglaje no puede borrar velocidades físicas o teletransportar el centro sin una política conservativa y explícita; el reinicio completo de carrera sí reconstruye la pose de partida.
10. Publicar posición visual del centro desde la misma geometría; comprobar rueda/chasis, rideHeight usada por aero, malla, colisiones y relPos.z. El arreglo no puede funcionar solo porque el dibujo de rueda es independiente de su contacto.

Archivos principales: tire_vertical_model.cpp/.h, tire_model_state.h y wheel.h para estados; wheel.cpp y tire_model.cpp para adaptadores; car.cpp y simu.cpp para ensamblaje/integración única; susp.cpp y axle.cpp para evaluación consistente. Revisar carstruct.h, sim.h y CMakeLists.txt solo cuando sus contratos lo requieran. Conservar símbolos nuevos autoexplicativos, sin abreviaturas ni comentarios de código. No rehacer el modelo térmico o de diferencial para esta tarea.

## 6. Pruebas y orden de promoción

### Puerta 1: geometría y unidades

Usar el adaptador real con los valores cargados del XML y después del recentrado. Probar plano, alturas conocidas, extensión/compresión, b = 1 y b distinto de 1, heave y roll/pitch de ambos signos. Verificar C, L, d y velocidades; trasladar juntos chasis y carretera no puede alterar carga/deformación. El ejemplo de 50/8.6 mm debe conservar la identidad exactamente dentro de precisión numérica.

Probar subida de carretera y movimiento de chasis por separado: no confundir velocidad de compresión con velocidad relativa de suspensión. Incluir vuelo con chasis quieto y con chasis moviéndose; el centro debe seguir su dinámica, no perder su estado por el signo de H-G.

### Puerta 2: cuarto de coche y banco completo con leyes nativas

Probar equilibrio con gravedad y precarga, perturbación positiva Y negativa de centro/suspensión, impulso pequeño al chasis, contacto perdido/recuperado y topes. Repetir a 500 y 1000 Hz con una MISMA excitación física; no reutilizar la frecuencia de integración como frecuencia sinusoidal.

La energía debe incluir T de la matriz conjunta, potencial elástico de muelles con precarga, neumáticos, barras, gravedad, trabajo de base/carretera y pérdidas de amortiguación, clamps e integración. Para una base fija y sin trabajo externo, no debe haber crecimiento sostenido. Con carretera/base móvil, incluir su trabajo: energía mecánica no tiene por qué ser monótona. Registrar residual de balance y tolerancias justificadas, no solo sumar cuadrados de desplazamientos absolutos.

El banco de cuatro ruedas debe permitir al chasis moverse en heave/roll/pitch y utilizar las mismas barras/heave/adaptador que el juego. Perturbar roll en ambos signos; exigir recuperación sin intercambio espontáneo sostenido de apoyos y sin ciclos de descarga. Comprobar que masa/gravedad total no cambia al congelar o liberar el movimiento relativo de ruedas.

Mantener pasando selector, contacto, fuerzas, térmica y energía existentes, corrigiendo los ensayos falsos sin debilitar sus comprobaciones. Los 10/10 del solver anterior y la positividad nominal de la matriz no sustituyen estas puertas.

### Puerta 3: coche completo antes de instalar como normal

Compilar una variante independiente, trazable al checkout actual, sin reutilizar DLL experimentales antiguas. Probar primero un plano controlado y después Williams/Fuji con distribution/OpenFormula.exe. Controles neutros verificables: dirección, acelerador y frenos a cero, embrague desacoplado y neutro. Los mandos forzados viven solo en diagnóstico.

Capturar a 500 Hz durante al menos diez segundos de física tras el transitorio: pose/velocidad del chasis, H/G/C/L/d y sus velocidades, fuerzas N/S, estado de contacto, barras, reacciones de topes, residuo del solver y balance energético. Distinguir v geométrica nueva del predictor susp.v legacy al comparar registros.

Comparar legacy real, reference de Etapa A y solver nuevo con la misma masa/combustible, presión de 130 kPa manométricos, temperatura, desgaste, superficie y reglaje. Exigir apoyo sostenido y carga media coherente con peso/aero, sin el patrón de aproximadamente un tercio de pasos descargados ni picos espontáneos de 5–10 kN en reposo. Medir RMS y espectro de roll/cargas; no declarar aceptación solo por una bajada visual del balanceo o por forzar N positiva.

Validar finalmente frenado hasta cero, aceleración, retención, cambios de marcha, marcha atrás, pendientes, cambios de superficie, pianos, boxes/reinicio y conducción sostenida. Mantener diferencial, alas, dirección, presión y grip de la referencia para poder atribuir mejoras al solver. Comprobar normal local y trabajo de contacto en Fuji, no solo coordenadas del plano.

Conservar siempre un caso de regresión legacy: la ruta nueva no puede modificar su masa ni su comportamiento. La aceptación de conducción requiere revisión humana; distinguirla de las pruebas automatizadas y del reposo.

## 7. Preservación, instalación y entrega

No tocar DLL/XML de producción mientras se desarrollan bancos. Antes de cambiar la instalación, guardar hashes y copias. No cerrar una sesión de juego ajena ni afirmar que se instaló una biblioteca que Windows dejó bloqueada. Las sesiones diagnósticas propias deben cerrarse y liberar la DLL.

Si el coche completo pierde apoyo o es peor que la referencia, conservar Etapa A y registrar el fallo medido. No borrar el solver experimental ni las evidencias, no esconderlo con offsets y no promoverlo por haber aprobado el banco aislado. Continuar con los residuos geométricos/mecánicos hasta localizar la discrepancia.

Al superar las puertas, activar explícitamente la integración nueva para el Williams, regenerar candidato/distribución solo con cambios autorizados y verificar que DLL, XML y modelo efectivo corresponden al resultado validado. Dejar telemetría normal y controles del jugador restaurados. No instalar las DLL diagnósticas con mandos forzados.

Actualizar migration_status.json e informes A/B con estado final real, ecuaciones, archivos, comandos, hashes, condiciones comparables, resultados y pendientes. No declarar la Etapa B cerrada si queda solo la referencia A o el banco experimental. No hacer commit ni push salvo solicitud posterior; entonces staging explícito y commits atómicos sin incorporar automáticamente cambios previos o binarios.

La solución buscada conserva compliance y contacto físico; suprime la realimentación artificial por carretera desplazada mediante un estado y unas ecuaciones acopladas coherentes. Mantener Etapa A es la protección durante el desarrollo, no el resultado final de este encargo.
