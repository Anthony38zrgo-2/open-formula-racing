# Handoff: calibración física del Williams 1997 en Speed Dreams

Fecha: 20 de octubre de 2026.

## Objetivo y alcance

Investigar y resolver cuatro problemas del Williams FW19 en Fuji76-77: dirección excesivamente brusca con mando, vuelcos durante cambios rápidos de dirección en asfalto liso, enfriamiento demasiado rápido de neumáticos y velocidad punta baja. Resolver causas y validar el comportamiento del coche completo; no aceptar una mejora visual, una compilación o un cálculo aislado como prueba de conducción estable.

Este handoff es documentación. Su creación no modifica código, configuraciones físicas ni ejecutables. Las propuestas numéricas siguientes NO están aplicadas ni aprobadas como calibración definitiva.

## Estado del proyecto y restricciones

- Proyecto destino: `D:\open-formula`. El usuario autorizó posteriormente inicializar Git y realizar el primer commit. Rama inicial: `codex/initial-port`. Verificar rama, HEAD y cambios pendientes antes de continuar; preservar cambios ajenos. La autorización del commit no implica aprobación de la calibración física ni de los problemas pendientes.
- Origen declarado por el usuario: https://github.com/brhndursun/speed-dreams-simuv5. La correspondencia exacta del árbol local con un commit de ese origen no está demostrada.
- Fuente de contenido: `D:\Formula90s`, rama `main-clean`, HEAD observado `f3efcf246c2a2abf62477037ed0243f2f9a93a9b`. Tiene cambios ajenos preexistentes. Inventariar su estado antes de cualquier intervención; no modificarlo para calibrar el port.
- Preservar la sesión Williams/Fuji, las texturas y el recorte alfa, el piloto estático oculto en cabina, los controles simultáneos de teclado/Xbox y la desactivación de penalizaciones por salida de pista.
- Único audio autorizado: `D:\Formula90s\game\sounds\banks\v10-v2-bank\engine_powered_maximum_loop.wav`. No ampliar el banco.
- No cambiar coches o configuraciones compartidas para corregir este Williams sin demostrar que existe un defecto general del motor. Preferir parámetros y funciones con alcance explícito.
- Cualquier trabajo sobre Godot debe cumplir el AGENTS.md de Formula90s y usar su MCP obligatorio. Este port funciona con Speed Dreams/simuv5, sin depender del solver Rust ni de Godot.

## Archivos y responsabilidades

| Sistema | Archivos principales |
| --- | --- |
| Generación del vehículo | `D:\open-formula\tools\prepare_williams_vehicle.py` |
| Configuración candidata | `D:\open-formula\implementation\candidates\williams-nineteen-ninety-seven\williams-nineteen-ninety-seven.xml` |
| Configuración distribuida | `D:\open-formula\distribution\share\games\speed-dreams-2\cars\models\williams-nineteen-ninety-seven\williams-nineteen-ninety-seven.xml` |
| Entrada simultánea y curva del mando | `D:\open-formula\src\libs\robottools\formula_gear_controls.cpp` y `.h` |
| Integración del conductor | `D:\open-formula\src\libs\robottools\rthumandriver.cpp` |
| Dirección física y límite de velocidad de cremallera | `D:\open-formula\src\modules\simu\simuv5\steer.cpp` |
| Barras, fuerzas y dinámica de carrocería | `D:\open-formula\src\modules\simu\simuv5\axle.cpp`, `susp.cpp`, `wheel.cpp`, `car.cpp` |
| Aerodinámica, motor y transmisión | `D:\open-formula\src\modules\simu\simuv5\aero.cpp`, `engine.cpp`, `transmission.cpp`, `atmosphere.cpp` |
| Apoyo sobre terreno original | `D:\open-formula\src\modules\simu\simuv5\track_ground_mesh.cpp` y `.h` |
| Colisiones con objetos | `D:\open-formula\src\modules\simu\simuv5\collide.cpp` |
| Preparación de Fuji | `D:\open-formula\tools\prepare_fuji_track.py`, `align_fuji_visual.py`, `prepare_fuji_object_collisions.py` |
| Empaquetado y arranque | `D:\open-formula\tools\assemble_portable_distribution.py`, `open_formula_launcher.cpp` |
| Estado y evidencia | `D:\open-formula\implementation\migration_status.json`, `D:\open-formula-migration-evidence` |

Editar el generador además de su salida cuando se cambien parámetros persistentes. Un ajuste directo exclusivamente en `distribution` se perderá al regenerar. Revisar también posibles setups guardados en `distribution\user-settings` que puedan alterar los parámetros efectivos.

## Problema 1: mando sin filtrado temporal suficiente

### Confirmado

El usuario informa que la dirección cambia demasiado deprisa. En `applyFormulaSteeringControls`, el stick izquierdo pasa por zona muerta y curva de potencia 1,8, pero cualquier objetivo no nulo se asigna inmediatamente a `previous_steering`. El único suavizado temporal propio de este adaptador actúa al volver a cero, a 6,85 unidades/s. Tampoco hay reducción del recorrido del mando en función de la velocidad.

No describirlo como ausencia absoluta de todo filtro: sí existen curva estática, zonas muertas y un límite posterior de velocidad de cremallera en `SimSteerUpdate`. Lo que falta en el adaptador es filtrado temporal de entrada y de inversión entre objetivos no nulos. Aún no está demostrado que sea la única causa del vuelco.

### Trabajo requerido

1. Registrar eje bruto, objetivo tras curva, comando filtrado, ángulo real de ruedas y velocidad del coche.
2. Añadir un filtro independiente de la frecuencia de actualización, con parámetros explícitos para aplicación, inversión y retorno al centro. Evaluar limitador de pendiente y/o filtro exponencial; evitar latencia innecesaria y doble filtrado excesivo con la cremallera.
3. Investigar una reducción progresiva de sensibilidad o recorrido a alta velocidad. No imponerla como solución física al vuelco ni eliminar el recorrido útil en maniobras lentas.
4. Mantener RT/LT, A/X para marchas, flechas y A/Z de teclado. Verificar que el filtrado del mando no rompe el comportamiento del teclado.
5. Configurar parámetros del filtro en un lugar persistente y documentado, con alcance del Williams/perfil de controles.

### Aceptación

- Ensayar escalón, barrido, inversión completa, liberación, ruido alrededor del centro y desconexión del mando.
- Repetir con distintos pasos temporales y comprobar tiempos de respuesta equivalentes.
- Registrar que una inversión del stick produce una transición continua del comando y comprobar la respuesta efectiva de las ruedas.
- Probar mando Xbox físico; el controlador virtual no sustituye esta prueba.

## Problema 2: vuelcos al transferir carga

### Confirmado

El usuario indica que vuelca al dar un volantazo incluso en asfalto liso. Las barras iniciales estaban en cero. El ajuste actualmente instalado es 100.000 N/m delante y 80.000 N/m detrás, y el usuario lo considera insuficiente.

Las categorías MP1/92MP1 incluidas en `speed-dreams-data\data\cars\categories` usan 2.000 lbs/in por eje, aproximadamente 350.254 N/m. En esta implementación `SimAxleUpdate` calcula la fuerza de barra como rigidez por diferencia de recorrido de suspensión. No aplicar automáticamente otro multiplicador por el bellcrank declarado de la barra.

### Hipótesis pendientes

- Entrada de dirección demasiado rápida o excesiva para la velocidad.
- Transferencia de carga, contacto de ruedas, signos o unidades de fuerzas, amortiguación, inercias o altura efectiva del centro de gravedad.
- Agarre lateral no calibrado: el multiplicador 2,7 se copió del modelo original a otro modelo de neumático; no implica la misma adherencia efectiva. La categoría nativa 92MP1 usa 1,5 como referencia de configuración, no como valor que deba copiarse sin pruebas.
- Aero incompleta y posible interacción con el muestreo del terreno o con colisiones invisibles. Aislar el caso sobre suelo plano antes de atribuirlo a las barras.

### Trabajo y aceptación

Capturar roll, velocidad angular de roll, aceleración lateral, ángulo de dirección, carga/contacto y recorrido de cada rueda, fuerza de cada barra y normal/altura del suelo. Comparar un escenario plano controlado con Fuji. Verificar que las barras ejercen un momento restaurador con signos correctos y que no aparece energía artificial.

Evaluar, como candidato y no como cura demostrada, 350.000 N/m delante y 280.000 detrás: multiplica ambas barras por 3,5 y conserva el reparto actual. Comparar con el ajuste vigente y variaciones moderadas; no seguir multiplicando rigidez a ciegas. Una barra más rígida puede descargar antes la rueda interior y empeorar respuesta sobre pianos.

Aceptar solo tras maniobras repetidas y reproducibles de cambio de carril/slalom a velocidades crecientes, sin vuelcos espurios en asfalto liso, sin cargas imposibles ni valores no finitos, y con comprobación posterior de pianos/escapatorias. Declarar las condiciones ensayadas; no prometer que el coche sea físicamente imposible de volcar.

## Problema 3: enfriamiento rápido de neumáticos

### Confirmado en configuración y ecuaciones

Se heredaron parámetros térmicos del Cavallo: temperatura inicial 30 °C, óptima 80 °C, `tire speed cooling factor=1.0`, coeficientes genéricos de calentamiento y masa de llanta predeterminada de 7 kg. No constituyen una calibración del neumático F1 1997.

En `SimWheelUpdateTire`, la pérdida convectiva se multiplica por `1 + 1.5 * tireSpeedCoolFactor`: actualmente 2,5. El pequeño `tire cooling factor=0.000075` actúa en otro término; reducir únicamente ese valor no corrige la pérdida dominante a velocidad alta.

Estimación analítica a 260 km/h, neumático a 80 °C, aire a 30 °C, sin desgaste, sin agua y SIN generación de calor: enfriamiento inicial aproximado 2,88 °C/s delante y 2,33 °C/s detrás; semivida del exceso de temperatura de unos 12/15 segundos. Son cálculos de términos del modelo, no una curva térmica medida en conducción. El calentamiento por deformación y deslizamiento reduce la pérdida neta.

### Trabajo y aceptación

Instrumentar energía de histéresis, calentamiento lateral/longitudinal, convección, enfriamiento adicional, capacidad térmica, carga, velocidad de rueda, camber, presión y desgaste. Verificar unidades y que los términos no introducen calentamiento/enfriamiento no físico. Revisar también la corrección de carga por camber del algoritmo antes de compensarla con parámetros.

Separar pruebas de neumático caliente en recta, neumático frío calentándose, curva sostenida y frenada. Evaluar un barrido del factor de refrigeración, por ejemplo 1,0 frente a 0,25: el multiplicador convectivo pasaría de 2,5 a 1,375, una reducción del 45 %. Este candidato NO está aplicado ni validado. No fijar la temperatura artificialmente ni desactivar la simulación térmica para ocultar el síntoma. Definir y justificar objetivos de temperatura y enfriamiento; no inventar una referencia histórica del Williams.

## Problema 4: velocidad punta baja y drag

### Confirmado

El usuario observa aproximadamente 260 km/h en quinta. El archivo original `D:\Formula90s\game\data\vehicles\f1_1997\williams\profiles\hockenheim\aerodynamics.json` declara `drag_coefficient_scope=whole_vehicle_reference`.

El port copia Cx=0,83 y área frontal 1,15 m², añade alerones nativos `FLAT` de 0,5347/0,7122 m² a 18/16 grados, omite las polares originales y el modelo de fondo, y configura sustentación de carrocería delantera/trasera en cero. Ese conjunto no reproduce el paquete aerodinámico original: añade resistencia de placas planas a una referencia del vehículo completo y pierde la carga original.

Con las fórmulas nativas y densidad nominal 1,29 kg/m³, la demanda aerodinámica calculada es aproximadamente 408 kW a 260 km/h y 626 kW a 300 km/h. No incluye rodadura ni pérdidas de transmisión; tampoco es telemetría. Investigar además `rho` global en `aero.cpp`: se inicializa antes de configurar la atmósfera de la sesión, mientras el registro de Fuji muestra una densidad cercana a 1,219 kg/m³.

Con radio trasero 0,322016 m, grupo final 4,65 y limitador a 17.500 RPM, el desarrollo cinemático permite aproximadamente 317 km/h en quinta y 371 km/h en sexta; 260 km/h en quinta corresponde a unas 14.351 RPM. Esto descarta el limitador de quinta como explicación basada en esos parámetros, pero no demuestra la velocidad real alcanzable.

### Trabajo y aceptación

1. Medir acelerador/freno/embrague efectivos, marcha, RPM, par, potencia transmitida, pérdidas, pendiente, velocidad del aire y daños. Descartar limitador de velocidad o arrastre de frenos.
2. Corregir el contrato de drag total frente a contribuciones por elemento. Adaptar las polares originales al modelo nativo o implementar una representación equivalente, conservando reparto y alturas de aplicación de las fuerzas.
3. Restituir/calibrar la carga de fondo y alerones con pruebas de fuerza frente a velocidad, pitch y altura; verificar la densidad efectiva usada.
4. Comparar aceleración y velocidad estacionaria en quinta/sexta sobre recta plana, con entradas y clima controlados. Ajustar desarrollos después del balance de potencia y resistencia.

No aumentar potencia arbitrariamente ni borrar drag para alcanzar una cifra prefijada. La velocidad objetivo debe tener una referencia declarada y condiciones de prueba reproducibles.

## Orden de trabajo y evidencia exigida

1. Inventariar cambios y respaldar los archivos afectados. Registrar procedencia del binario/configuración realmente cargados.
2. Preparar telemetría y escenarios reproducibles. Confirmar especialmente filtro del mando y cargas durante el vuelco.
3. Corregir el contrato aerodinámico y el problema de dirección; evaluar el efecto por separado.
4. Calibrar barras y suspensión con comparaciones controladas; no mezclar simultáneamente múltiples cambios sin evidencia atribuible.
5. Calibrar el balance térmico y repetir aceleración y maniobras de estabilidad con neumáticos en condiciones equivalentes.
6. Regenerar candidatos, instalar/empaquetar, comprobar hashes de binarios y validar la distribución portable sin depender del PATH de compilación.
7. Completar una vuelta y las maniobras de aceptación con mando físico. Mantener explícitos los resultados pendientes de aprobación humana.

Las pruebas geométricas previas de terreno/objetos y las pruebas con mando virtual no validan estabilidad, aerodinámica, temperatura ni respuesta física al choque. No marcar estos cuatro problemas como resueltos basándose solo en que la escena carga o compila.

## Compilación, ejecución y registros

- CMake/Ninja: `C:\Users\bill\open-formula-build\engine`.
- Herramientas UCRT64: `C:\Users\bill\open-formula-build-tools\msys64\ucrt64\bin`.
- Dependencias PLIB: `C:\Users\bill\open-formula-build\dependencies`.
- Python usado: `C:\Users\bill\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe`.
- Para regenerar la configuración CMake, conservar la configuración de compilación existente. Se utilizó `CMAKE_DISABLE_FIND_PACKAGE_SOLID=TRUE` para seguir construyendo FreeSOLID desde el árbol y evitar que su instalación previa cambie los targets durante la reconfiguración.
- Compilar con `cmake --build C:/Users/bill/open-formula-build/engine -j 6` e instalar con `cmake --install C:/Users/bill/open-formula-build/engine`, usando el PATH UCRT64 correspondiente.
- Regenerar el Williams con `tools\prepare_williams_vehicle.py` y empaquetar con `tools\assemble_portable_distribution.py`; revisar sus argumentos antes de ejecutarlos.
- Lanzador de la entrega: `D:\open-formula\distribution\OpenFormula.exe`. Abrir directamente `distribution\bin\speed-dreams-2.exe` puede usar otro perfil de usuario y no reproduce necesariamente la sesión preparada.
- Registro de ejecución: `D:\open-formula\distribution\user-settings\runtime.log`.
- Diagnóstico detallado ya existente: `D:\open-formula\implementation\reports\williams_physics_investigation.md`.
- Evidencia y respaldos: `D:\open-formula-migration-evidence`.

## Fuentes consultadas

- iRacing, construcción y ajuste de estabilizadoras: https://www.iracing.com/commodores-garage-11-swayanti-roll-bars/
- iRacing, comparación en skidpad del Skip Barber: https://www.iracing.com/skidpadman-effect-of-the-rear-anti-roll-bar/
- Studio 397, mecanismos y parámetros térmicos de neumáticos: https://www.studio-397.com/wp-content/uploads/2016/12/rF2_TGM_TyreTool_QuickStart_V3.pdf
- Referencia directamente comparable de unidades y configuración: archivos locales `MP1.xml` y `92MP1.xml` en `D:\open-formula\speed-dreams-data\data\cars\categories`.

Las referencias explican mecanismos y ofrecen puntos de comparación; no establecen una configuración histórica verificada del Williams FW19 ni garantizan equivalencia numérica entre simuladores.
