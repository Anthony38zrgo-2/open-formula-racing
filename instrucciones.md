# Handoff: nuevo modelo de neumáticos para Speed Dreams / simuv5

Fecha: 20 de octubre de 2026.
Destino: D:/open-formula.
Base inspeccionada: rama codex/initial-port, HEAD 02d93a2a0422c7efd3851ec387cbf24951d7cb9f; árbol limpio antes de esta documentación.

## 1. Mandato, resultado esperado y límites

Añadir un modelo nativo de neumáticos que incorpore las mejores propiedades verificadas del modelo Rust de Formula90s, adaptadas a los contactos, suspensión, transmisión, unidades y ciclo de actualización de simuv5. Incluye fuerzas, carcasa, presión, térmica y desgaste. No basta con copiar mu, stiffness o unas curvas.

Este documento es una especificación para una implementación posterior. Su creación SOLO modifica documentación: no implementa ni activa el modelo, no cambia parámetros físicos, no recompila bibliotecas y no constituye aceptación de conducción. El handoff anterior se conserva en implementation/reports/williams_physics_handoff_before_new_tire_model.md. Los informes anteriores sirven de historial; comprobar siempre el estado instalado y los generadores.

El nuevo modelo debe ser seleccionable por vehículo; el modelo legacy sigue siendo el predeterminado de los demás coches. Primera integración y calibración exclusivamente en el Williams/Fuji. La comparación legacy/nuevo debe ser reproducible con idénticos neumáticos, masas, suspensión, superficie, ayudas, estado térmico y entradas. No compensar defectos con dirección, diferencial, alas, altura del centro de gravedad o mu global.

No implementar una dependencia de Godot ni una DLL Rust para resolver esta tarea. Adaptar las ecuaciones y contratos a C++ nativo dentro de simuv5. Revisar procedencia y compatibilidad de licencias antes de trasladar código; conservar atribuciones aplicables en documentación y avisos de licencia. No modificar D:/Formula90s, su BUILD_SOURCE ni sus binarios.

## 2. Hallazgos confirmados y estado que debe preservarse

- simuv5 usa una curva tipo Magic Formula sobre sqrt(sx² + sy²), con reparto vectorial de la fuerza. No equivale a dos curvas puras independientes ni al presupuesto combinado de Rust.
- El Williams actual tiene mu=2,7, pico lateral aproximadamente 0,115 rad, dynamic friction=88%, elasticity factor=0,86 y stiffness=104,26873608. El 88% es el límite asintótico de la curva nativa: no significa 88% de fuerza para todo deslizamiento posterior al pico.
- El banco nativo aislado mide mu efectivo aproximadamente 2,32–3,55 según carga/eje, sin penalización térmica ni camber. No demuestra falta de agarre máximo, aceleración del coche completo ni fidelidad histórica. No subir mu por defecto.
- La variable tireSpringRate se calcula a partir de presión y geometría, pero no tiene consumidores en el solver simuv5 inspeccionado. No afirmar que ese valor ya endurece físicamente el contacto.
- El modelo térmico actual concentra el neumático en una temperatura global. Su actualización ocurre dentro de SimWheelUpdateForce mediante SimWheelUpdateTire cuando está habilitada.
- Hay dos rutas de integración que llaman rueda/transmisión en simu.cpp. Toda integración nueva debe cubrir ambas sin duplicar calor, desgaste ni par resistente.
- El paso físico de referencia del motor es 0,002 s, definido por RCM_MAX_DT_SIMU. Los 120 FPS configurados son de renderizado: no son la frecuencia física ni térmica.
- Se solicitó literalmente 160 psi para las cuatro ruedas. El generador guarda 1103161,16691 Pa; simuv5 trata ese valor como absoluto en sus ecuaciones. Con 101300 Pa atmosféricos equivale a unos 145,3 psi manométricos. El nuevo modelo no debe reinterpretar ni sustituir silenciosamente ese setup.
- El perfil Rust Williams hockenheim contiene presiones frías manométricas de 130/120 kPa y referencias calientes de 160/150 kPa delante/detrás. No confundir esos 160 kPa con 160 psi. Esos valores son un perfil de juego, no datos certificados del Goodyear de 1997.
- Diferencial actual: precarga independiente 10 Nm, power 0,30, coast 0,70, activación 90 Nm. Aero: fondo 40% y alas 60% en la referencia óptima, sin imponer ese porcentaje durante toda la vuelta. Conservar esos ajustes.
- Conservar materiales, piloto estático, audio autorizado, controles teclado/Xbox, malla de contacto de Fuji, geometría, configuración de suspensión y ventana 1920×1080 con límite 120 FPS.

Hipótesis aún no demostradas: rigidez/deformación correctas para el FW19; curva real de fuerzas de sus neumáticos; equilibrio final entre grip mecánico y aero; estabilidad con mando; explicación completa de los vuelcos. No convertirlas en hechos al implementar.

## 3. Fuentes Rust y responsabilidades en el destino

Leer las implementaciones actuales, no copiar comentarios o parámetros de revisiones antiguas:

| Referencia Rust, solo lectura | Propiedad a extraer |
| --- | --- |
| D:/Formula90s/game/crates/vehicle-physics-engine/src/tire.rs | Curvas puras, presupuesto combinado, relajación, recuperación, camber y par autoalineante |
| D:/Formula90s/game/crates/vehicle-physics-engine/src/wheel_mechanics.rs | Deformación, radio efectivo, contacto y sensibilidad a carga |
| D:/Formula90s/game/crates/vehicle-physics-engine/src/tire_thermals.rs | Banda en tres zonas, carcasa, gas, presión y modificadores mecánicos |
| D:/Formula90s/game/crates/vehicle-physics-engine/src/tire_wear.rs | Desgaste por zona y relación entre energía, temperatura y pérdida de goma |
| D:/Formula90s/game/crates/vehicle-physics-engine/src/vehicle_configuration/tires/ | Esquema, conversión y validación de parámetros |
| D:/Formula90s/game/data/vehicles/f1_1997/williams/profiles/hockenheim/tires.json | Perfil de referencia; separar valores de juego de evidencia histórica |

Rust dispone de Standard y backends simplificados. Standard conserva memoria de deslizamiento, migración de picos y rampa de contacto; los simplificados eliminan parte de esos estados. La arquitectura destino debe partir de los contratos físicos completos y justificar cualquier simplificación con pruebas, no presentar todos los backends Rust como idénticos.

| Archivo destino existente | Responsabilidad concreta |
| --- | --- |
| src/modules/simu/simuv5/wheel.cpp y wheel.h | Adaptador de rueda, selección, estado e intercambio de fuerzas con el motor |
| src/modules/simu/simuv5/simu.cpp | Orden de actualización en las dos rutas, subpasos y publicación final |
| src/modules/simu/simuv5/susp.cpp, axle.cpp y car.cpp | Auditar compatibilidad del contacto vertical y de las reacciones; cambiar solo lo necesario y medido |
| src/modules/simu/simuv5/transmission.cpp y differential.cpp | Consumidores del par de reacción y velocidades; conservar su modelo, integrar el contrato de rueda |
| src/modules/simu/simuv5/atmosphere.cpp | Densidad, presión y temperatura ambiental efectivas |
| src/modules/simu/simuv5/track_ground_mesh.cpp y .h | Contacto, normal y propiedades de superficie existentes |
| src/interfaces/car.h y consumidor HUD correspondiente | Publicar temperatura, presión, desgaste y par autoalineante con unidades coherentes; revisar compatibilidad antes de ampliar la interfaz |
| src/modules/simu/simuv5/CMakeLists.txt y CMakeLists.txt raíz | Compilar módulos nuevos y registrar bancos de prueba |
| tools/prepare_williams_vehicle.py | Única generación persistente del selector y parámetros del Williams |
| tools/assemble_portable_distribution.py | Copiar configuración candidata y generar manifiesto portable |
| implementation/migration_status.json e implementation/reports/ | Estado confirmado, límites, procedencia y resultados |

Crear módulos internos propuestos tire_force_model.cpp/.h, tire_contact_model.cpp/.h y tire_thermal_model.cpp/.h. Añadir tire_wear_model.cpp/.h si la complejidad lo justifica. Mantener configuración, estado y resultados separados; evitar un wheel.cpp monolítico. Los nombres nuevos deben ser autoexplicativos, sin abreviaturas y sin comentarios de código. No usar nombres abreviados heredados en símbolos nuevos salvo al interactuar con la interfaz existente.

Auditar antes cómo el motor reserva, pone a cero, copia y libera tWheel/tCar. Si usa memoria C, no introducir contenedores con constructores o punteros propietarios sin adaptar su ciclo de vida. Preferir estado de tamaño fijo por rueda. Reiniciar correctamente al iniciar sesión, entrar en boxes, cambiar compuesto, reiniciar carrera y destruir el vehículo.

## 4. Contrato de coordenadas, unidades y signos

Crear primero una tabla verificable de coordenadas del motor, marco de rueda y normal del terreno. En simuv5 se usa avance X y vertical Z; no trasladar sin conversión el marco Rust (+X derecha, +Y arriba, -Z avance). Confirmar signos con pruebas de cada rueda, marcha atrás y rotación de dirección.

Unidades internas: metros, segundos, kilogramos, radianes, rad/s, newtons, N·m, joules, watts, pascales absolutos y kelvin. Grados, Celsius, psi y presión manométrica se convierten solamente en fronteras explícitas.

Presión: absolute_pressure = gauge_pressure + ambient_absolute_pressure. La presión usada en gas ideal debe ser absoluta. Definir en configuración el tipo y unidad del valor, temperatura de referencia y volumen/gas de referencia; mostrar en HUD qué convención usa. No añadir o restar la atmósfera dos veces.

Si falta la convención en un coche legacy, conservar su comportamiento. Para el nuevo selector exigir configuración inequívoca; un valor fuera del dominio calibrado genera diagnóstico, no una conversión oculta ni una supuesta presión normal. Registrar los 160 psi como caso extremo de robustez, no como referencia de calibración. La selección de una presión de conducción distinta exige una decisión explícita posterior del usuario.

Definir el signo de slip longitudinal desde la velocidad relativa en el contacto, una referencia de velocidad regularizada a baja velocidad y signos correctos en reversa. La fuerza sobre el coche y el par sobre la rueda deben formar un par acción/reacción compatible con el convenio de spin del diferencial.

## 5. Modelo de fuerzas obligatorio

1. Curvas puras independientes: pico lateral en ángulo, pico longitudinal en slip ratio, caída progresiva y meseta por eje. Partir de los objetivos 0,115 rad, 0,12 y caída 0,88 del perfil; no convertirlos en datos reales certificados.
2. Presupuesto combinado único: normalizar las demandas por capacidades lateral/longitudinal y proyectar sobre una elipse continua. La fuerza publicada nunca excede ese presupuesto. La frenada consume capacidad, no multiplica mu.
3. Mantener el ratio longitudinal/lateral de superficie como parámetro independiente; el Road Rust vale 0,95. Separar fricción del compuesto y multiplicador de pista: no aplicar dos veces el 2,7 que el port trasladó desde el perfil de superficie.
4. Sensibilidad a carga con referencia física por rueda y disminución de mu al aumentar carga. Implementar curvas continuas, con fuerza tendiendo a cero con carga cero y sin salto en la referencia. Comparar la ley Rust con la legacy, no copiar ambos multiplicadores simultáneamente.
5. Camber: carga lateral por inclinación y redistribución de contacto térmico, con espejo correcto izquierda/derecha. A igual camber simétrico las contribuciones laterales en recta deben cancelarse.
6. Radio efectivo, deformación y cobertura/contacto deben proceder del modelo mecánico, no de constantes duplicadas. La temperatura y presión afectan la capacidad y respuesta mediante modificadores limitados y con significado físico.
7. Par autoalineante: trail neumático dependiente del estado del contacto y saturación. Su magnitud debe caer al saturarse; no añadir un momento artificial para estabilizar el chasis ni duplicar el feedback existente.
8. Transitorios: relajación por distancia con coeficiente 1-exp(-speed*duration/length), tiempo mínimo si está justificado, rampa de contacto y memoria de pérdida/recuperación separadas. Probar una versión sin relajación frente a otra con ella antes de fijar valores; evitar doble filtrado con FLOAT_RELAXATION2 del legacy y con controles.
9. Migración de picos con deslizamiento combinado: incorporar la formulación Rust solamente si los ensayos demuestran continuidad, monotonicidad de la demanda y ninguna ganancia de capacidad oculta. Debe poder aislarse en el banco.
10. Fricción en reposo/baja velocidad: resolver o regularizar de forma disipativa. No generar drift, yaw creciente o autoaceleración; tampoco congelar artificialmente una rueda que debería girar por par aplicado.

Publicar una sola pareja de fuerzas de tracción y su par de reacción. Resistencia a la rodadura se contabiliza una vez; determinar si el motor existente la aplica como fuerza, par o ambas antes de integrar el nuevo modelo. No reutilizar simultáneamente el resultado legacy.

## 6. Contacto radial y adaptación a suspensión nativa

Antes de añadir un muelle, dibujar el balance de masas, resortes y fuerzas existente entre terreno, rueda y chasis. Medir contacto en equilibrio y durante un escalón; no asumir que la variable tireSpringRate ya resuelve esa dinámica.

Implementar una carcasa radial con rigidez y amortiguación explícitas por eje, deformación no negativa, recorrido físico finito y radio efectivo. La presión caliente modifica rigidez; derivar amortiguación para conservar aproximadamente la razón de amortiguamiento. Usar límites de protección documentados y detectar cuándo se alcanzan; no usarlos para esconder inestabilidad.

Adaptar la carcasa como elemento en serie con la suspensión o mediante una formulación equivalente estable, demostrando las reacciones y masas efectivas. No sumar dos muelles sobre la misma penetración ni aplicar la carga normal completa dos veces. No crear otro rigid body o duplicar la masa no suspendida del motor.

La fuerza normal que alimenta fricción, calefacción y desgaste debe ser la misma reacción mecánica aceptada en el subpaso. Separar geometría muestreada, deformación de goma y recorrido de suspensión. En vuelo: fuerza de contacto y calentamiento por suelo cero, giro libre coherente, intercambio térmico con aire activo.

Conservar la malla original de Fuji y las normales de contacto. Evaluar pendientes, bordillos y contacto parcial en el marco del suelo, sin sustituir las alturas por un plano mundial. Definir qué hace el modelo cuando falta una muestra válida de terreno.

## 7. Modelo térmico obligatorio

Estado por rueda: temperatura interior/central/exterior de banda, temperatura de carcasa, temperatura del gas, presión absoluta, desgaste por zona y cantidades de energía acumuladas. Cada nodo tiene una capacidad térmica positiva explícita.

Red de intercambios: entre zonas de banda; banda-carcasa; carcasa-gas; banda-pista cuando hay contacto; banda-aire y carcasa-aire; intercambio adicional solo si existe una fuente definida. No inventar temperatura de pista a partir del aire sin identificarlo como fallback.

Para cada arista: heat_power = conductance * (source_temperature - destination_temperature). Restar y sumar la misma energía en los dos nodos; el intercambio interno se cancela en el balance total. El calor hacia aire/pista cuenta como flujo externo con su signo, permitiendo calentamiento cuando el ambiente está más caliente.

Calor de deslizamiento: derivarlo del trabajo disipado por las fuerzas reales y la velocidad relativa del contacto. Con fuerza sobre el coche y velocidad de deslizamiento definidas coherentemente, usar -force_dot_slip_velocity para la parte disipativa. Documentar aparte almacenamiento y liberación elástica; no convertir energía elástica recuperable ni empuje de camber íntegro en calor. Evitar estimar calor otra vez con el par del motor o con la misma potencia multiplicada por dos ejes.

Integrar la potencia del mismo subpaso que generó la fuerza, distribuirla entre banda/pista y zonas por carga/camber con pesos normalizados. La histéresis de carcasa procede de deformación/rodadura con eficiencia explícita y no duplica las pérdidas de rodadura ni las de deslizamiento.

Temperatura de banda regula grip; temperatura de carcasa regula propiedades estructurales con una influencia moderada; temperatura del gas regula presión mediante gas ideal y una corrección pequeña de volumen por deformación. No introducir una campana de grip por presión para ocultar errores de contacto: el efecto debe seguir la cadena presión → compliance/deformación → contacto/respuesta, con límites comprobados.

Presión caliente: p_abs = reference_p_abs * gas_temperature_kelvin / reference_temperature_kelvin * reference_volume / current_volume. Usar una referencia fija y gas consistente; no multiplicar repetidamente por la temperatura relativa al paso anterior. Evitar que los límites de presión alteren masa de gas sin contabilización.

El modelo usa capacidades/conductancias finitas y positivas. Integrar la red con un método estable, preferiblemente implícito conservativo para intercambios, o subpasos térmicos con estabilidad demostrada. No depender de clamps de temperatura para pasar pruebas; toda saturación debe quedar registrada y su energía residual explicada.

Desgaste: derivarlo de energía de deslizamiento, temperatura y compuesto por zona. Debe ser no negativo, sin recuperación espontánea, sin desgaste por deslizamiento en el aire o en reposo sin trabajo. Alimentar suavemente espesor, capacidad térmica y pérdida de grip. Mantener compatibilidad con boxes y compuestos; reinicio y pinchazo son eventos explícitos, no flags locales que se olvidan cada paso.

No reutilizar automáticamente coolingFactor=0,000075 o tireSpeedCoolFactor=0,25 como conductancias nuevas: pertenecen a otra ley. Los parámetros térmicos Rust son candidatos iniciales con unidades y procedencia, no equivalencias ni una garantía de temperatura real.

## 8. Ciclo de integración y compatibilidad

Auditar los dos caminos de simu.cpp. El orden actual relevante es contacto/altura → barras → fuerzas de rueda (incluye térmica legacy) → transmisión → rotación → chasis. El nuevo orden debe conservar el contrato de transmisión y decidir explícitamente cómo integrar el acoplamiento rueda-suelo.

Contrato propuesto por subpaso:

1. Leer estado térmico/mecánico previo y resolver modificadores de presión/carcasa.
2. Muestrear contacto, resolver suspensión/carcasa y obtener carga normal aceptada.
3. Resolver slip, fuerza combinada y reacción de rueda; integrar giro/transmisión de forma consistente con esas fuerzas. Si se itera o subintegra, acumular trabajo una sola vez sobre la solución aceptada.
4. Acumular energía de contacto, pérdidas y desgaste usando las velocidades correspondientes a la fuerza aplicada; explicar el uso de estado inicial, medio o final.
5. Actualizar red térmica, gas y presión una sola vez por intervalo aceptado; los modificadores resultantes entran en el siguiente subpaso, salvo solución acoplada explícita.
6. Publicar fuerzas, reacción, temperatura, desgaste, presión y feedback una sola vez.

Mientras el selector nuevo esté activo, no ejecutar también SimWheelUpdateTire, penalización currentGripFactor, curvas mfB/mfC/mfE o relajación legacy sobre las fuerzas nuevas. El camino legacy debe permanecer funcional y aislado.

Decidir si se integran subpasos mecánicos internos; no cambiar globalmente el paso de todos los coches. La térmica no debe ejecutarse en callbacks de dibujo. Registrar el paso físico real y el número de actualizaciones por rueda; un frame puede contener varios pasos físicos.

Adaptar el HUD y carElt: indicar si la temperatura agregada es banda media, centro o carcasa; conservar acceso por rueda y registrar los tres nodos de banda. Documentar presión mostrada y convertirla en un solo sitio. Investigar el consumidor antes de cambiar nombres/unidades públicas. Evitar romper drivers, ayudas o acceso a _wheelSlipOpt, _tyreEffMu, spinTq, rollRes y torqueAlign.

Selector propuesto por coche en Formula Controls: tire model = legacy o transient coupled. Configuración nueva en Formula Tires, con bloques Front/Rear y parámetros de compuesto/superficie separados. Añadir versión de esquema y validación, dejando los nombres definitivos documentados. No reutilizar pressure ambiguo para parámetros nuevos.

## 9. Secuencia de implementación y entregables

1. TIRE-01 — Baseline y contratos: inventario, hashes, unidades, signos, ciclo de vida y orden de ambas rutas. Capturar legacy a la presión actual sin cambiarla. Entregar mapa de responsabilidades y pruebas de signos/presión.
2. TIRE-02 — Núcleo de fuerzas puro: entradas/salidas explícitas, curvas por eje, sensibilidad a carga y presupuesto combinado. Banco fuera del juego antes de conectar al vehículo.
3. TIRE-03 — Carcasa/contacto: carga normal coherente, deformación, amortiguación y radio efectivo; demostrar equilibrio/energía sin ayudas ni cambios de suspensión ajenos.
4. TIRE-04 — Transitorios y feedback: comparar con/sin relajación, añadir solo los estados necesarios para respuesta y recuperación verificables; integrar trail y reacción de transmisión.
5. TIRE-05 — Térmica/presión/desgaste: red conservativa, gas y pérdidas de grip, validación por nodo y energía. Incluir pruebas que detecten doble actualización y doble calentamiento.
6. TIRE-06 — Adaptador simuv5: selector, dos rutas de integración, carElt/HUD, generación persistente y reset/boxes. Los otros vehículos continúan legacy.
7. TIRE-07 — Calibración Williams: curvas con iguales cargas/presiones/temperaturas; contraste Rust nativo y candidato sin mezclar configuraciones. Separar presión solicitada de escenario de referencia acordado. Mantener aero, diferencial y controles constantes.
8. TIRE-08 — Conducción y entrega: telemetría, regresiones, build instalado, manifiesto, reporte de candidatos y límites. La validación humana en pista cierra la aceptación de manejo; no sustituirla por un benchmark.

Cada entrega debe identificar archivos propios, decisiones, referencia exacta, escenarios, resultados y riesgos. Una fase aprobada no autoriza reescribir motor, colisiones o transmisión fuera del contrato descrito.

## 10. Pruebas y criterios de aceptación

Crear bancos nuevos de fuerzas/contacto/térmica con nombres completos y registrarlos en CMake. Conservar validate_formula_tire_energy como regresión legacy; no cambiar sus expectativas para ocultar una rotura. Añadir trazas de referencia y tests diferenciales desde Rust si se puede ejecutar su núcleo sin Godot, indicando revisión/hash y tolerancias. La fidelidad entre implementaciones se comprueba con salidas, no con parámetros de igual nombre.

### Fuerzas y transmisión

- Barridos laterales y longitudinales de cero, pico, caída, bloqueo y wheelspin; simetría de signos y entre lados.
- Malla combinada de frenada/aceleración con giro: utilización normalizada ≤1 más tolerancia numérica, sin saltos en el borde, sin aumento artificial al saturar el otro eje.
- Cargas 0, 100, 1200, 3200, 6000 y 12000 N; contacto parcial, vuelo y aterrizaje. Registrar fuerza absoluta y mu efectivo; no inferir agarre del vehículo solo desde mu.
- Recta con camber simétrico: sin deriva neta espuria; camber y dirección invertidos conservan simetrías físicas.
- Rueda libre: sin fuente de energía; par de contacto, giro, motor y freno balanceados. Prueba de frenada que contabilice energía del chasis y rotación.
- Comparar marcha atrás y cruce por cero con alta/baja velocidad; sin denominadores singulares o signos inestables.

### Contacto y frecuencia

- Carga estática, escalón vertical, bordillo y caída: masas y reacciones cerradas, deformación finita y energía no creciente cuando no hay excitación.
- Frecuencias de banco 120/240/480/500/1000 Hz, incluyendo el paso nativo 0,002 s. Comparar respuestas a igual tiempo y entrada; el render no se modifica para pasar pruebas.
- Criterio inicial de convergencia: diferencias <2% en fuerzas estacionarias, <5% en métricas transitorias y <1 C en escenarios térmicos de 60 s entre 500 y 1000 Hz. Documentar condiciones y ajustar tolerancias solo con evidencia numérica previa.
- No fijar por conveniencia una deflexión o temperatura universal para un FW19: usar referencia declarada y distinguir objetivo de calibración de dato medido.

### Térmica, presión y desgaste

- Equilibrio sin fuentes y temperaturas iguales; sistema aislado conserva energía; conductancias internas cancelan exactamente sus transferencias por arista.
- Cada paso: suma de cambios de energía = calor externo entrante - saliente + disipación depositada. Objetivo de residual relativo <0,1% en ensayos cerrados, sin compensaciones artificiales.
- Calentamiento a potencia constante, recuperación en recta, parada, vuelo, lluvia si existe soporte, zonas interior/exterior con camber, cambio de compuesto y boxes.
- Gas ideal: a volumen constante p_abs/T constante; distinguir frío/caliente, psi/kPa/Pa y absoluto/manométrico; nunca presión/temperatura absolutas negativas.
- 160 psi: caso extremo explícito. Verificar robustez y diagnósticos; no demostrar plausibilidad física mediante clamps. Las pruebas de conducción deben declarar su presión efectiva, sin sustitución silenciosa.
- Desgaste monótono con trabajo disipado, cero desgaste por fricción sin contacto, estado de pinchazo persistente y reset correcto.
- Prueba contador: ninguna rueda recibe dos actualizaciones térmicas para el mismo intervalo en ninguna de las dos rutas.

### Coche completo

- Reposo 30 s, rueda libre, aceleración y frenada recta, círculo de radio constante, slalom, entrada frenando, salida acelerando, bordillos y vueltas completas en Fuji.
- Mantener constantes aero 40/60 de referencia, diferencial 10 Nm/30%/70%, geometría, combustible y ayudas; registrar presiones y temperaturas reales.
- Buscar vibración, deriva en reposo, energía creciente, wheel hop, cambios bruscos de yaw y vuelcos. No declarar estos defectos resueltos sin ese ensayo.
- Probar mando físico y teclado. Separar respuesta del neumático del filtro de steering; no retocar ambos a la vez para facilitar el ensayo.
- Medir coste por coche a 500 Hz y flota de referencia, memoria fija y ausencia de asignaciones por rueda/paso. Publicar hardware y carga; no inventar equivalencia a partir de FPS.

## 11. Telemetría mínima

Por rueda y con tiempo físico: selector/version, carga normal, cobertura, normal de suelo, compresión de suspensión, deformación y velocidad de carcasa, radio efectivo, spin, slip objetivo/efectivo por eje, capacidades, fuerzas, utilización combinada, régimen, trail y par autoalineante.

Añadir temperaturas de las cinco masas térmicas, presión absoluta y manométrica, temperatura ambiental/pista, energías depositadas/transferidas/perdidas, residual de balance, desgaste por zona y contadores de clamps/fallbacks. Registrar parámetros efectivos y fuentes de cada setup. Reducir frecuencia de escritura sin perder acumulados por paso ni contaminar el rendimiento normal.

## 12. Rebuild, distribución y cierre

Antes de cada edición verificar y registrar rama, HEAD y estado; preservar modificaciones ajenas y respaldar los archivos propios. No cambiar/resetear ramas sucias. Mantener commits atómicos, añadir rutas explícitas y revisar git diff --cached. La autorización anterior de commit no implica push ni aprobación física.

Build nativo conocido: C:/Users/bill/open-formula-build/engine; herramientas C:/Users/bill/open-formula-build-tools/msys64/ucrt64/bin. Añadir allí los targets necesarios mediante CMake. No reutilizar objetos/bibliotecas entre ramas; verificar procedencia de fuente y toolchain. Si cambia el layout de tWheel/tCar, recompilar todos sus consumidores, no solo el .cpp nuevo.

Comprobar que el juego está cerrado antes de reemplazar simuv5.dll; no matar sesiones ajenas. Instalar la biblioteca realmente recompilada en distribution/lib/games/speed-dreams-2/modules/simu, verificar hash y regenerar la distribución con tools/assemble_portable_distribution.py. Nunca promover una DLL de otra rama o sustituirla solo por fecha.

Arrancar con distribution/OpenFormula.exe para conservar la sesión Williams/Fuji. Separar prueba de arranque, inspección visual y conducción. No usar herramientas Godot para validar el port nativo; si se trabaja sobre Godot de origen, cumplir su AGENTS.md y MCP obligatorio.

Las carpetas distribution, implementation/candidates e implementation/initial-settings están ignoradas. Persistir el ajuste en el generador y conservar una instantánea textual de configuración efectiva y hashes en el informe de pruebas; no forzar medios/binarios a Git. Actualizar migration_status.json con el alcance exacto y conservar los flags de validación pendientes mientras no se hayan cumplido.

Definition of done: modelo seleccionable y aislado, fuerzas/contacto/térmica acoplados sin doble contabilización, contrato de presión resuelto explícitamente, tests y telemetría reproducibles, biblioteca instalada con procedencia verificada, regresiones legacy sin romperse y conducción aceptada por el usuario. Compilación, ausencia de NaN y curvas de banco por sí solas no completan la tarea.
