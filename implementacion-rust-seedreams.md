# Backlog de implementación: físicas Rust simplificadas en Open Formula

Fecha: 21 de octubre de 2026. Directorio de trabajo previsto del agente: **D:/**.

Repositorio de origen: **D:/Formula90s**. Repositorio de implementación y entrega: **D:/open-formula**.

Este archivo contiene el encargo actualizado del usuario. Sustituye las recomendaciones incompatibles de la evaluación anterior, especialmente la propuesta de trasladar un tercer muelle o conservar el setup adaptado de simuv5. No es una solicitud de seguir investigando indefinidamente: implementar el port en el destino, completar la adaptación y ejecutar la prueba de integración final definida aquí.

## 1. Decisiones confirmadas y alcance

1. Reemplazar completamente las físicas activas de Open Formula por la lógica física Rust simplificada de Formula90s. Speed Dreams conserva carrera, presentación, controles de dispositivo, HUD, sonido y garaje.
2. Únicamente la combinación simplificada del jugador: suspensión simplificada, aerodinámica simplificada, neumáticos simplificados con relajación y estados térmicos simplificados. No seleccionar automáticamente la variante simplificada de IA ni instalar el mundo acoplado nuevo como sustituto directo.
3. Mantener **tres raycasts reales por rueda y dos del fondo**: 12 + 2. No rellenar tres impactos a partir de uno.
4. **Descartar el tercer muelle.** No portar el elemento trasero de 285 kN/m añadido en Open Formula. No añadir heave físico independiente a Rust, ni sustituirlo por otra fuerza artificial. Los modos de heave usados para diagnóstico no son un tercer muelle.
5. **Eliminar la elasticidad y amortiguación vertical del neumático al adaptar Rust**, conservando contacto rígido unilateral, reacción normal, suspensión y grip dependiente de carga. No traer sin adaptar la ley radial elástica del origen.
6. Usar como base física el **perfil JSON original del Williams de Formula90s**, identificado más abajo. No usar como calibración inicial los números convertidos/retocados para simuv5.
7. Todo parámetro físico del perfil cargado debe quedar reflejado en el garaje: controles de setup donde sea editable y un inspector real de configuración para los demás campos, con valores, unidades y condición de solo lectura explícitos. Ningún control puede representar un parámetro que Rust ignore.
8. **No ejecutar pruebas en D:/Formula90s.** Ese repositorio es fuente de lectura/copia; no modificarlo, compilarlo, ejecutar Godot allí, limpiar cachés ni lanzar sus scripts de validación.
9. **No hacer pruebas intermedias ni crear baterías de tests para este encargo.** Integrar y adaptar todo en D:/open-formula, permitiendo las compilaciones necesarias para producir los binarios y corregir errores del compilador. Al terminar, realizar **una prueba de integración final** del sistema completo. No convertir este trabajo en un proyecto de benchmarks, pruebas A/B o reproducción de suites del origen.
10. No hacer commit ni push salvo nueva petición explícita. Preservar los cambios preexistentes y los recursos visuales/audio.

La preparación de este backlog solo crea este documento; no modifica la configuración ni los binarios instalados. El tercer muelle del motor C++ anterior sigue presente hasta que el agente complete la sustitución: debe quedar fuera de la nueva configuración, interfaz y ejecución Rust.

## 2. Fuente física y JSON que hay que traer

### 2.1 Perfil de origen exacto

Entrada canónica:

`D:/Formula90s/game/data/vehicles/f1_1997/williams/profiles/hockenheim/vehicle.json`

La definición `D:/Formula90s/game/data/vehicles/f1_1997_williams_hockenheim_villenueve.tres` referencia expresamente esa ruta mediante physics_config_path. El lanzador `D:/Formula90s/scripts/run_f1_97.ps1` selecciona el Williams y los cuatro backends simplificados.

Este es el «otro JSON» al que se refiere el encargo: el perfil original Rust del Williams, no el XML generado para Speed Dreams, ni el perfil F1 2030 que algunas funciones incrustan por defecto. No cambiar de variante o reemplazarlo por un perfil genérico.

Copiar el manifiesto y sus documentos de dominio, conservando las referencias relativas:

| Archivo | Papel en el port |
| --- | --- |
| vehicle.json | Identidad, contrato de coordenadas y referencias de dominio. |
| chassis.json | Masa, geometría e inventario físico. |
| suspension.json | Suspensión y geometría física original, sin tercer muelle añadido. |
| tires.json | Neumático tangencial, mecánica empírica, térmica, presión y desgaste. |
| powertrain.json | Motor, caja, embrague, diferencial y sus controles reales. |
| brakes.json | Frenos y térmica de frenos. |
| aerodynamics.json | Alas, drag y efecto suelo de la versión simplificada. |
| steering.json | Dirección y filtrado correspondiente. |
| driving_assistance.json | Ayudas y estados iniciales definidos en el perfil. |
| fuel.json | Combustible, capacidad, consumo y contribución de masa. |
| underfloor_contact.json | Parámetros originales del fondo, a adaptar al contacto del nuevo host. |
| coupled_world.json | Dependencia documental que puede exigir el cargador; no autoriza activar ese mundo. Conservar el documento si hace falta para cargar el perfil, aunque no se ejecute su arquitectura. |
| suspension_presentation.json | Referencia para poses visuales si la publicación la requiere. |
| audio.json | Referencia/procedencia del perfil. No sustituir por ello el audio actual de Open Formula. |
| profile_provenance.json | Procedencia y clasificación de aproximaciones, si existe. |

Ubicación propuesta y estable en destino:

`D:/open-formula/rust/data/vehicles/williams-fw19-hockenheim/`

Los valores originales son la base. Adaptar exclusivamente lo necesario para el nuevo host, la retirada del neumático radial elástico, la ausencia de tercer muelle y la conexión de configuración. **No reaplicar automáticamente** reparto 65 %, mu trasero 0,42, diferencial 10/15, precarga 10 N·m, temperatura 80 °C, presión 100 kPa o cualquier otro override del generador simuv5. Si alguno coincide con el JSON, se conserva por estar en el JSON, no por herencia del port C++.

Como referencias identificativas leídas al redactar: brakes.json original declara reparto 0,59 y max_brake_torque 4000; el diferencial Rust usa rampas de potencia/retención y precarga, no los porcentajes de slip bias del motor anterior. El agente debe copiar los documentos reales y sus hashes, no reconstruir el perfil escribiendo estos ejemplos a mano.

### 2.2 Fuente Rust

Raíz del crate:

`D:/Formula90s/game/crates/vehicle-physics-engine/`

Copiar una instantánea del código necesario, Cargo.toml, dependencias de compilación y resolución de dependencias a un paquete local bajo:

`D:/open-formula/rust/vehicle-physics-engine/`

Puede conservarse inicialmente el nombre del crate para reducir cambios mecánicos. No mantener dependencias de ruta a D:/Formula90s ni de su ubicación instalada. Una vez copiado, **todas las modificaciones y compilaciones ocurren en D:/open-formula**.

La parte física del fondo también se encuentra en:

`D:/Formula90s/game/crates/formula90-core/src/underfloor.rs`

Trasladar su lógica necesaria al paquete físico de destino, con sus tipos/configuración, sin importar el orquestador completo ni sus dependencias de audio y sesión.

## 3. Estado inicial y protección de trabajo existente — RUST-00

Prioridad: P0. Dependencias: ninguna.

- [ ] Empezar en D:/, leer este backlog y las instrucciones aplicables de cada repositorio antes de tocar sus archivos.
- [ ] Consultar rama, HEAD y estado Git de ambos repositorios. Usar rutas de trabajo explícitas: D:/ no es un repositorio Git padre común.
- [ ] Registrar esos datos y el inventario de archivos que se vayan a cambiar bajo `D:/open-formula/implementation/rust-physics-migration/`.
- [ ] Respaldar fuera del checkout, bajo `D:/open-formula-migration-evidence/rust-physics-port-<fecha>/`, los archivos del destino que se modifiquen, incluyendo cambios previos y candidatos ignorados.
- [ ] No resetear ramas, limpiar carpetas ni reemplazar fuentes enteros para deshacerse de modificaciones previas.
- [ ] Registrar SHA completo y hashes de fuentes/perfiles copiados. Distinguir origen y cambios de adaptación.
- [ ] Mantener D:/Formula90s solo en lectura. No ejecutar cargo test/check/build, Godot, SCons, validadores ni scripts de arranque de ese repositorio.

Estado observado al redactar, que hay que volver a consultar: origen HEAD f3efcf246c2a2abf62477037ed0243f2f9a93a9b; destino HEAD a68ee9e. Ambos tienen modificaciones previas. El destino incluye cambios de garaje, siluetas, setup, toe y tercer muelle de encargos anteriores; conservar los cambios ajenos y retirar solo lo que esta migración vuelve incompatible.

Entrega: inventario y respaldo concretos; ninguna validación de conducción ni suite del origen.

## 4. Extraer la ejecución exclusivamente simplificada — RUST-01

Prioridad: P0. Dependencias: RUST-00.

- [ ] Crear el workspace Rust local de destino y copiar el crate/perfil identificados.
- [ ] Conservar suspensión, contactos y geometría preparada simplificados de `suspension.rs`, `simplified_suspension.rs`, `simplified_axle_integration.rs`, `simplified_contact_kinematics.rs` y sus dependencias comunes.
- [ ] Conservar `TireBackend::SimplifiedWithRelaxation`. Excluir del constructor de producción Standard, SimplifiedWithoutRelaxation y SimplifiedArtificialIntelligence.
- [ ] Fijar `AerodynamicsBackend::Simplified`, `SuspensionBackend::Simplified` y estados lentos simplificados explícitamente en la API de destino. No depender de variables de entorno para obtener el comportamiento pedido.
- [ ] No usar `VehicleDynamicsFidelity::Simplified` como sustituto de esos cuatro selectores: corresponde a otra ruta, especialmente del neumático IA y del mundo acoplado.
- [ ] Conservar powertrain, frenos, desgaste, térmica, datos y propiedades de masa que consuman esas rutas.
- [ ] Retirar las rutas de ejecución Standard y sus fallbacks del binario/API de producción. Conservar la geometría matemática compartida necesaria para preparar la suspensión; no confundir esa preparación con ejecutar Standard.
- [ ] Si un perfil no puede prepararse, devolver un error claro. No activar otro backend ni un perfil F1 2030 por defecto.
- [ ] Adaptar build.rs: actualmente espera Git y un perfil F1 2030 fuera del crate. Hacer explícitas la procedencia del código copiado y la carga del Williams dentro del destino.
- [ ] No arrastrar formula90-core, game-sim, vehicle-audio-engine, la GDExtension ni todos los miembros del workspace del origen para conseguir enlazar las físicas.

Entrega: biblioteca Rust local compilable con constructor y selección inequívocos. Se permite compilar; no ejecutar suites ni bancos de componentes.

## 5. Definir el contrato y el propietario físico — RUST-02

Prioridad: P0. Dependencias: RUST-01.

Propietario único: Rust mantiene pose, velocidades, orientación, suspensión, giro de ruedas, transmisión, combustible, térmica y estado de contacto. El adaptador C++ transporta comandos y publica vistas; no integra otro coche.

- [ ] Definir una ABI C versionada de creación, destrucción, avance, lectura de snapshot, reset/colocación, setup y servicios de boxes.
- [ ] Transportar todos los datos del fondo. No reutilizar sin ampliar el step de ffi.rs que llama a `VehicleSimulator::step`: esa ruta deshabilita las medidas del fondo. La base reutilizable con medidas es `step_with_aerodynamics_environment`.
- [ ] Llevar errores y panics a resultados controlados, sin atravesar la frontera C. Definir propiedad y vida de buffers/handles, tamaños explícitos y límite de llamadas por tick.
- [ ] Elegir DLL cdylib y adaptador mediante ABI C, o enlazado compatible demostrado por la compilación del destino. La máquina inspeccionada tiene Rust MSVC y C++ MinGW/UCRT64: no suponer compatibilidad de bibliotecas C++ estáticas por compartir x64.
- [ ] Publicar un snapshot completo por paso, evitando una simulación sombra para alimentar gráficos, HUD o sonido.
- [ ] Documentar y centralizar conversión de ejes: simuv5 X delante/Y izquierda/Z arriba frente a Rust X derecha/Y arriba/−Z delante.
- [ ] Aplicar la misma transformación coherente a puntos, normales, velocidades lineales/angulares, fuerzas, pares y bases/cuaterniones. Resolver también el desplazamiento de origen y COM.
- [ ] Permutar ruedas explícitamente: Open Formula FR/FL/RR/RL; Rust FL/FR/RL/RR. Mapeo de índices `[1,0,3,2]`.
- [ ] Resolver unidades de rpm/rad·s⁻¹, temperatura y presión por campo, incluyendo gauge frente a absoluta.
- [ ] Conservar el valor local del toe en el setup; la conversión de orientación física por lado pertenece al modelo, no a valores con signo contrario en la interfaz.

Entrega: contrato implementado y consumido por el adaptador, no solo un documento de intención.

## 6. Circuito físico y muestreo de 14 rayos — RUST-03

Prioridad: P0. Dependencias: RUST-02.

- [ ] Reutilizar la geometría física de Fuji existente como entrada; importar triángulos, normales y materiales al sistema de consulta Rust/Parry y construir su aceleración espacial al cargar.
- [ ] Separar suelo transitable, muros/objetos y geometría del coche. No convertir la malla visual completa sin clasificación en suelo transitable.
- [ ] Implementar segmentos 3D con origen, dirección normalizada, longitud máxima y exclusión del propio coche. `TrackGroundMesh::sample` es una consulta de altura, no el reemplazo completo de esos segmentos.
- [ ] Reproducir la distribución transversal interior/centro/exterior de `F194RustVehicle::setup_raycasts`, a partir de los anclajes y ancho de rueda del perfil original.
- [ ] Calcular doce impactos independientes. Un rayo sin impacto sigue sin impacto; no duplicar el central, reutilizar automáticamente el de otra rueda ni inventar suelo desde el frame anterior.
- [ ] Entregar distancia desde el origen del rayo, punto global, normal y superficie a `TriRaycastSample`, conservando el comportamiento ponderado original y sus máscaras de contacto.
- [ ] Mantener los dos rayos del fondo de la ruta simplificada: centro delantero y garganta del difusor. Equivalen a los índices 0 y 3 y máscara 0b01001 del formato anterior. No activar las cinco sondas.
- [ ] Reutilizar los mismos dos resultados del fondo para aerodinámica/contacto cuando corresponda; no volver a raycastear a escondidas para cada consumidor.
- [ ] No usar sin adaptar el muestreo de `coupled_vehicle_world.rs`: hoy replica un impacto en tres muestras por rueda y recorre cinco posiciones aero.
- [ ] Añadir contadores ligeros al diagnóstico final: consultas de ruedas, consultas de fondo, impactos válidos y backend. No crear un benchmark separado.

Presupuesto: 12 + 2 consultas de suelo por paso de muestreo. Consultas geométricas de choque de carrocería, muros y coches son distintas de los raycasts de apoyo y no deben generar un segundo soporte del neumático.

Entrega: consultas de terreno reales y alimentación de fuerzas/presentación. No realizar una prueba aislada de raycasts; se observarán en la integración final.

## 7. Adaptar el contacto vertical rígido sin tercer muelle — RUST-04

Prioridad: P0. Dependencias: RUST-02 y RUST-03.

Archivos de partida: vertical_suspension_integration.rs, simplified_contact_kinematics.rs, simplified_axle_integration.rs, suspension.rs, simulation.rs y lógica física extraída de underfloor.rs.

- [ ] Identificar las lecturas de rigidez/amortiguación radial y separar sus usos geométricos, dinámicos y empíricos.
- [ ] Sustituir la ley radial elástica por una restricción unilateral rueda-suelo. Contrato: hueco no negativo, reacción normal no negativa y reacción nula en separación, dentro de tolerancias numéricas declaradas.
- [ ] Mantener radio geométrico para contacto y radio efectivo de rodadura para slip/pares como magnitudes separadas.
- [ ] Conservar masas no suspendidas, muelles/amortiguadores de suspensión, barras, topes, geometría equivalente y seis grados de libertad del chasis. No añadir un tercer muelle.
- [ ] Formular el Jacobiano de contacto desde el punto/normal reales y la cinemática chasis-rueda. Incluir velocidades lineales, angulares y extensión; conservar trabajo y momentos respecto al COM.
- [ ] Resolver reacciones y límites en el mismo avance Rust, de forma coherente con la integración del chasis y de las ruedas. Reutilizar álgebra/algoritmos de contacto existentes cuando encajen; no incorporar por ello el mundo acoplado completo ni activar su modelo distinto de neumático.
- [ ] Usar integración implícita/active set para las restricciones normales y topes cuando sea necesario; aceptar el paso con factibilidad, signo y complementariedad. No reemplazar el contacto por un resorte de k enorme ni por una proyección externa que inyecte energía.
- [ ] No eliminar simplemente la fuerza radial: la reacción que sostiene el coche debe seguir existiendo y alimentar el grip. Tampoco copiar la fuerza del muelle como carga normal universal, pues falta la dinámica no suspendida.
- [ ] Publicar la carga normal media a partir de impulsos aceptados cuando existan subpasos. No aplicar el impulso otra vez como fuerza adicional al cuerpo.
- [ ] Conservar grip longitudinal/lateral, deslizamiento combinado, relajación simplificada, sensibilidad a carga, camber/toe, autoalineante, rodadura, temperatura, presión y desgaste.
- [ ] Mantener solo una estimación algebraica de deformación si la huella, rodadura o volumen térmico la necesitan. No devolver esa estimación al soporte vertical ni integrarla como otra carcasa elástica.
- [ ] Retirar o sustituir justificadamente el calentamiento radial asociado a una dinámica eliminada. No fabricar calor derivando una pseudodeformación para igualar temperaturas anteriores.
- [ ] Adaptar el fondo a los dos sensores y a las colisiones rígidas de carrocería. No sumar respuesta de suelo, fondo y cuerpo para el mismo contacto. Conservar una sola autoridad de velocidad/posición.
- [ ] Tratar correctamente suspensión de ruedas en vuelo y reentrada en contacto, sin fuerza de adherencia cuando N=0.

Los parámetros radiales originales que dejen de ser aplicables deben quedar retirados del schema activo o identificados como datos históricos sin uso; no mostrarlos como ajustes efectivos del garaje. El perfil adaptado debe declarar su versión y conservar una copia de referencia del original.

Entrega: contacto rígido integrado en Rust y normal publicada al neumático. Sin tercer muelle ni fuerza compensatoria equivalente. No ejecutar pruebas de componente durante esta fase.

## 8. Integración del cuerpo y colisiones completas — RUST-05

Prioridad: P0. Dependencias: RUST-04.

- [ ] Reutilizar/adaptar `VehicleSimulator::integrate_standalone` para pose y orientación; gravedad e inercia se aplican una sola vez en Rust.
- [ ] Mantener un reloj fijo coherente con la carrera. El destino usa normalmente 2 ms/500 Hz; no cambiar todo el simulador a 120 Hz para acomodar un valor por defecto del origen.
- [ ] Si hay subpasos internos, contabilizar tiempo y fuerzas/impulsos sin adelantar otra vez la transmisión, combustible, térmica o eventos. No multiplicar silenciosamente el presupuesto de raycasts.
- [ ] Conectar detección geométrica y respuesta de carrocería, muros y coches al mismo estado físico. Parry aporta consultas; por sí solo no resuelve la dinámica ni los impulsos.
- [ ] Reutilizar de physical_collision_world.rs y generalized_vehicle_contacts.rs lo necesario para esa responsabilidad. No convertir el port en otra implementación paralela del mundo completo de Formula90s.
- [ ] Evitar contacto normal duplicado rueda-suelo mediante rayos y mediante geometría rígida de rueda. Definir qué contactos soportan y cuáles representan impacto lateral/borde.
- [ ] Conservar respuesta de fondo y carrocería en pianos, pendientes y vuelcos. Dos rayos centrales no detectan todos los bordes laterales del suelo: usar contactos geométricos de carrocería para esa respuesta, sin añadir sondas de fondo.
- [ ] Inicializar masa, combustible, COM e inercia desde el perfil resuelto, sin añadir ruedas o combustible dos veces. Distinguir masa física del vehículo y parámetros virtuales del modelo simplificado.
- [ ] Implementar pausa, reset, recolocación y congelación por error como transiciones explícitas. Un error no debe activar simuv5 ni dejar el coche parcialmente adelantado.

Entrega: Rust posee todo el avance físico y la respuesta a colisiones necesarios para la sesión; C++ no corrige después la pose por su cuenta.

## 9. Conectar IPhysicsEngine y retirar simuv5 de la ejecución — RUST-06

Prioridad: P0. Dependencias: RUST-02 a RUST-05.

Ubicaciones propuestas:

- `D:/open-formula/src/modules/simu/rustphysics/`: adaptador C++ e integración del módulo.
- `D:/open-formula/src/modules/simu/CMakeLists.txt` y CMake raíz: compilación/empaquetado del nuevo motor.
- `D:/open-formula/src/interfaces/iphysicsengine.h`: contrato existente; ampliar solo si es imprescindible y con todos los consumidores coordinados.
- Configuración de motor de carrera en implementation/initial-settings, distribution/initial-settings y configuración efectiva del usuario.

- [ ] Implementar initialize, configureCar, reconfigureCar, updateSituation, updateCar, setCar, getCar y shutdown.
- [ ] Hacer que el módulo seleccionado sea el nuevo motor Rust, con carga de ABI/procedencia y error claro si falta el binario/perfil.
- [ ] configureCar carga el Williams original adaptado, inicializa estado, rellena metadatos y publica setup válido al garaje.
- [ ] updateSituation lee controles y avanza el mundo una sola vez. updateCar no puede duplicar pasos del mismo coche/tiempo cuando se use otro modo de carrera.
- [ ] Publicar en CarElt pose, velocidades/aceleraciones, ruedas/poses, spin, normales, cargas, superficies, temperatura/presión/desgaste, marcha, rpm, combustible, daños y datos de dirección necesarios.
- [ ] Actualizar posición de pista, vueltas, sectores, pits y eventos sin aplicar fuerzas adicionales por esa publicación.
- [ ] Conservar sonido actual, cámaras, modelo, librea y animación de ruedas, alimentados desde el snapshot real. No traer el audio Rust de Formula90s por dependencia accidental.
- [ ] Conservar el mapeo teclado/Xbox existente; dejar una sola aplicación de filtrado, ayudas, embrague y ajuste de rpm. No filtrar dos veces dirección o pedal.
- [ ] Retirar del camino activo SimCarPhysicsStep, SimEngineUpdateTq, SimTransmissionUpdate, solver vertical C++, fuerzas aero/neumáticos C++ y respuesta de colisiones simuv5.
- [ ] Retirar del build/distribución activa la selección simuv5 cuando la migración esté terminada. Puede mantenerse una copia fuera del runtime como respaldo, sin fallback ni selector de producción.
- [ ] No usar el Cavallo del catálogo como fallback físico si le falta perfil Rust. El encargo inicial integra Williams/Fuji; cualquier vehículo sin perfil compatible debe producir una incompatibilidad explícita.

Entrega: el ejecutable normal usa Rust para toda la física del Williams y mantiene los servicios del simulador.

## 10. Garaje conectado al JSON Rust real — RUST-07

Prioridad: P0. Dependencias: RUST-06 y perfil adaptado de RUST-04.

Owner principal: `D:/open-formula/src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp`, junto al nuevo adaptador y schema de setup Rust.

### 10.1 Reglas de publicación y edición

- [ ] Leer del snapshot/configuración Rust los valores mostrados al abrir el garaje. No rellenar con constantes del antiguo XML o datos de una simulación sombra.
- [ ] Reemplazar el cableado directo a tCarSetup cuando ese campo no representa el modelo Rust. Puede conservarse como transporte solo si la traducción y la aplicación son reales y completas.
- [ ] Crear una tabla/descriptor de parámetros con ruta JSON, etiqueta, unidad, conversión, límites, paso, condición de edición y efecto de aplicación. Evitar índices de página mágicos que desplacen otros controles.
- [ ] Asegurar cobertura de todos los campos físicos del perfil: controles principales para setup y sección avanzada/inspector por dominios para el resto. Los campos de diseño o preparación no editables en sesión se muestran como tales con su valor cargado; no ocultarlos bajo valores decorativos.
- [ ] Aplicar cambios como transacción validada por Rust. Publicar los valores aceptados; ante error conservar el estado/configuración anteriores y mostrar la causa.
- [ ] Reconstruir las tablas/preparación simplificada que dependan de la edición, sin ejecutar Standard. Mantener el criterio de conservación/reset de térmica, desgaste, combustible y movimiento definido para boxes.
- [ ] Guardar el setup de usuario como override versionado separado del perfil original; restaurar predeterminados debe volver al JSON Rust base adaptado, no al setup simuv5. No escribir sobre la copia de referencia original.
- [ ] Retirar el control «Tercer muelle» y las referencias a 285 kN/m de la nueva ruta. Tampoco dejarlo visible a cero como un parámetro editable que no existe.
- [ ] Mantener diseño y recursos del garaje, ampliando paneles/desplazamiento cuando haga falta sin solapamientos. No rediseñar arte ni cambiar siluetas como parte de este encargo.

### 10.2 Cobertura mínima visible

| Área | Parámetros reales que debe reflejar |
| --- | --- |
| Suspensión | Muelles y amortiguadores físicos, compresión/extensión lenta/rápida, recorridos, topes, alturas y geometría efectiva según el schema Rust; sin tercer muelle. Distinguir tasa en elemento y tasa en rueda. |
| Barras | Parámetros que consume la suspensión simplificada/geometría activa. No mostrar arb_ratio heredado si esa ruta realmente consume otro campo. |
| Ruedas | Toe/camber locales, presión, dimensiones y temperaturas/estado, más límites y magnitudes relevantes del perfil en el inspector. |
| Aerodinámica | Incidencias, áreas/coeficientes/configuración del fondo y estado de las dos sondas. Mostrar campos de diseño como solo lectura si no son setup. |
| Frenos | Reparto, capacidad/par máximo y campos del modelo Rust. No mantener un deslizador de MPa sin una conversión física real a lo que consume Rust. |
| Diferencial | Rampas de potencia y retención en grados, precarga y configuración del LSD Rust. No rotular las rampas como porcentajes de bloqueo del modelo anterior. |
| Motor/caja | Limitador, relaciones/final, comportamiento de cambios/embrague y freno motor que exponga el schema; curvas y datos fijos visibles en configuración avanzada. |
| Combustible/servicios | Cantidad, capacidad, repostaje y neumáticos, todos sobre el mismo vehículo Rust autoritativo. |
| Dirección/ayudas | Ángulo, respuesta y ayudas realmente activas; sin doble filtrado ni estados discordantes. |
| Configuración avanzada | Árbol/listado de todos los campos físicos del JSON resuelto, sus unidades/origen y si admiten edición. El valor actual y el base deben distinguirse cuando exista override. |

Entrega: garaje operativo sobre la configuración de física real, con persistencia y restauración coherentes.

## 11. Empaquetar todo antes de probar — RUST-08

Prioridad: P0. Dependencias: RUST-07.

- [ ] Compilar únicamente en el workspace de destino con artefactos separados y procedencia del código copiado/adaptado. Corregir errores de compilación según aparezcan, sin lanzar bancos intermedios.
- [ ] Instalar la biblioteca Rust, adaptador, configuración del motor y perfil completo dentro de distribution. El ejecutable no debe necesitar D:/Formula90s ni rutas del entorno del desarrollador.
- [ ] Ajustar scripts de generación del Williams para que no vuelvan a imponer los valores simuv5 ni regeneren el tercer muelle. Separar exportación visual de configuración física cuando sea necesario.
- [ ] Incluir todos los JSON requeridos por el cargador y el perfil de usuario separado. Mantener audio/recursos visuales del destino.
- [ ] Actualizar manifiesto de distribución y migration_status con backend, perfil, hashes y estado pendiente de prueba final.
- [ ] Completar el registro de archivos añadidos/modificados y de funciones físicas C++ retiradas del camino activo.
- [ ] No ejecutar las antiguas suites nativas C++ ni las suites Rust del origen «por si acaso». No crear pruebas nuevas de componentes como prerrequisito.

Entrega: versión completa instalada y lista para la única integración final.

## 12. Una prueba de integración final — RUST-09

Prioridad: P0. Dependencias: RUST-08 terminada. Esta es la única fase de ejecución de pruebas del encargo.

Realizar **un caso de integración de extremo a extremo Williams/Fuji** usando la distribución real de D:/open-formula y el perfil migrado. Puede instrumentarse con un único guion de maniobras y una sola sesión; no dividirlo en una batería de bancos independientes. Usar las herramientas de control disponibles; no confundir una compilación correcta o la apertura del menú con esta prueba.

Secuencia de la sesión:

1. Arrancar por la ruta normal del destino. Registrar motor Rust activo, backends simplificados, hash/versión del perfil y ausencia de dependencia de Formula90s/Godot/simuv5.
2. Cargar el Williams antes de meta en Fuji. Mantenerlo brevemente detenido y comprobar que el estado permanece finito, apoyado y sin deriva creciente evidente.
3. Acelerar, cambiar de marcha, levantar gas, frenar y recorrer una curva. Incluir un tramo de piano/desnivel y un contacto de carrocería/muro controlado si el guion puede hacerlo de forma reproducible y segura para la sesión.
4. Observar el contador: tres consultas independientes por rueda y dos del fondo por paso definido. Confirmar máscaras de impacto y carga normal cuando una rueda pierde/reencuentra suelo.
5. Confirmar que el soporte de rueda es rígido, que la suspensión conserva movimiento y que no se consume una fuerza de resorte/amortiguador radial como soporte normal. Usar el diagnóstico integrado, sin abrir un test de neumático aparte.
6. Entrar en boxes/garaje dentro de la misma sesión. Comprobar correspondencia entre perfil/snapshot y valores visibles, presencia de los campos reales del diferencial/frenos y ausencia de tercer muelle.
7. Cambiar dos ajustes seguros, de familias distintas, aplicar y salir a pista. Registrar que Rust aceptó los valores y que el vehículo publicado y el garaje comparten esa configuración. Comprobar cancelar/restaurar y guardado del override dentro del mismo caso, sin volver al XML simuv5.
8. Confirmar actualización de ruedas, HUD, rpm/marcha, audio y combustible. Ejecutar un reset/retorno a boxes y finalizar la sesión sin una simulación sombra activa.

Criterios para dar esta integración por pasada:

- Estado único Rust, ninguna ruta Standard/IA equivocada ni fallback simuv5.
- Perfil original Williams correctamente copiado y adaptado; sin tercer muelle ni overrides heredados involuntariamente.
- Muestreo real 12+2 y respuesta geométrica de carrocería sin soporte duplicado.
- Contacto vertical rígido y grip con N válida; sin NaN, rechazo persistente de pasos ni descontrol evidente en las maniobras del caso.
- Garaje lee/aplica/restaura parámetros reales; presentación y servicios consumen el snapshot correcto.
- Distribución autosuficiente y registro de evidencia consistente.

Si el caso falla, corregir la causa y repetir **el mismo caso final**, sin añadir suites, comparaciones A/B ni pruebas en Formula90s. Registrar el fallo y el resultado posterior con honestidad. Una pasada de esta integración acredita ese recorrido y esas funciones, no precisión histórica, estabilidad en todos los circuitos ni rendimiento de una parrilla de 22 coches.

## 13. Entrega y cierre — RUST-10

Dependencias: RUST-09.

- [ ] Marcar este backlog por ítem, conservando trazabilidad de cambios y dependencias.
- [ ] Dejar informe final en `D:/open-formula/implementation/reports/rust_simplified_physics_integration.md`.
- [ ] Indicar archivos afectados, ubicación del perfil base y override, motor seleccionado, comandos de compilación usados y hashes instalados.
- [ ] Incluir el resultado de la única integración final, logs/capturas y límites observados. Si algo quedó incompleto, nombrarlo sin declararlo terminado.
- [ ] Confirmar que D:/Formula90s no fue modificado ni sometido a pruebas/rebuild y que no hay dependencias de ejecución hacia él.
- [ ] Confirmar que no se hicieron commit ni push.

## 14. Referencias de consulta, no encargos adicionales

- Evaluación previa: `D:/open-formula/implementation/reports/rust_simplified_physics_port_feasibility_2026-10-21.md`. Sus propuestas de tercer muelle y conservación de setup simuv5 quedan revocadas por este backlog.
- Contrato destino: `D:/open-formula/src/interfaces/iphysicsengine.h`.
- Muestreo/host de origen: `D:/Formula90s/native/src/vehicle/f1_94_rust_vehicle.cpp` y `D:/Formula90s/native/src/core/f90_core.cpp`, solo lectura.
- Perfil y lanzador de origen: rutas del apartado 2, solo lectura. No ejecutarlos.
- Interfaces físicas de origen: ffi.rs, simulation.rs y types.rs del crate físico, solo lectura antes de copiar.
- Garaje existente y configuración distribuida en D:/open-formula.

La evaluación anterior ya hizo comprobaciones y suites de origen. **No repetirlas.** El encargo presente prioriza integrar en el destino y ejecutar únicamente la integración final cuando todo esté adaptado.
