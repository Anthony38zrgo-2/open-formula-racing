# Encargo siguiente: por qué la rueda interior no baja con distintos reglajes

Repositorio: `D:/open-formula`. Rama de publicación: `codex/initial-port`. Este encargo continúa el diagnóstico de Melbourne T3 después de instalar la corrección que conecta los controles del garaje. El usuario probó manualmente y sigue observando que la rueda no baja con distintas configuraciones; sospecha del modelo de suspensión. Investigar antes de proponer más constantes.

## Lectura inicial y alcance

Leer este archivo, `completion.md`, `diagnosis.md`, los AGENTS aplicables y el estado actual del repositorio. El `instrucciones.md` raíz contiene un encargo histórico de simuv5: no asumir que describe el motor activo ni implementar ese plan como parte de esta investigación. El runtime actual es Rust, suspensión simplificada, neumático vertical rígido.

No asumir que conectar los controles ha corregido la extensión física. Las pruebas existentes acreditan conexión, equivalencia de elementos e idempotencia; no demuestran corrección de las ecuaciones de movimiento ni conducción aceptable.

Conservar los reglajes del usuario y el trabajo ajeno. T3 y la aceptación de conducción quedan al usuario. Se puede investigar con bancos y añadir instrumentación técnica. No cambiar grip, aero, muelles o barras para ocultar un error del solver; no forzar contacto por temporizador ni introducir una segunda autoridad vertical. No crear un nuevo chat o enviar mensajes a otros agentes como consecuencia de este documento.

## Evidencia más reciente, ya congelada

Carpeta `latest_telemetry/`:

- `runtime.log`: copia byte a byte de la sesión iniciada el 22/10/2026 a las 21:59:03 según el log, terminada normalmente. 766045 bytes; SHA-256 `1556c74a47cc78cdabe657c372569ae9496e421c1a9af28eebb40237a0fe3517`.
- `samples.csv`: 678 muestras `RUSTDIAG`, con número de línea de origen y separación de reinicios del reloj si ocurren.
- `capture.json`: hashes de DLL y perfiles al capturar, ventanas de FR con reacción cero mientras FL tiene reacción positiva, límites de interpretación.
- `saved_setups.json`: copia JSON del registro SQLite leída en modo de solo lectura. No es una historia del reglaje aplicado durante toda la sesión.
- `installed_profile_documents.json`: los documentos instalados al capturar, separados por nombre de dominio, para reconstruir la base exacta sin depender de una distribución que pueda cambiar.

La DLL instalada al capturar tiene SHA-256 `7088504e89d84a0a0cd8daf884433d8557ca1d6a8ff877e75035144149ae55cd`, coincidente con la candidata validada e instalada en el trabajo anterior. El log registra su ruta de carga, pero no su hash histórico; verificar esta distinción al reconstruir procedencia.

En el entorno de la curva previamente identificada como T3, FR tiene N=0 en cinco muestras de 42,174 a 43,174 s. Inicio aproximado SD (1886,306; 827,748; 1,680). Examinar desde 39 hasta 46 s, incluyendo frenada y recuperación, no únicamente la muestra del vértice. Los números de línea están en `capture.json`; el grupo comienza en la línea 1881 del log.

A las 42,174 s: N=(3395,1; 0; 1681,1; 1168,1) N, compresión=(90,6; 78,2; 119,1; 108,8) mm; 12/12 rayos con suelo, impulso de fondo cero y 0 pasos rechazados. La FR no está necesariamente quieta: distinguir compresión respecto al cuerpo, altura del buje en mundo y hueco real al suelo.

Orden Rust: **FL, FR, RL, RR**. Speed Dreams expone **FR, FL, RR, RL** y el puente los transforma. `travel` del log es compresión absoluta; el desplazamiento desde diseño es `q = compression - spring_length * resting_ratio`. No usar el nombre `travel` como si fuese directamente droop.

El «Reglaje 1» capturado, modificado antes de esta sesión según SQLite, contiene:

- Extensión 0,20 delante y detrás.
- Amortiguación 0,74 / 0,71; compresión 1,99 / 1,90.
- Ratio de altura 0,37 / 0,40; topes 1 / 1.
- Barras geométricas 1,00 / 0,80, no 0,20 / 0,20 del diagnóstico inicial.

La captura no contiene eventos de aplicación que permitan demostrar que esos valores se usaron en cada muestra. Instrumentar el reglaje efectivo y su identidad al aplicarlo antes de comparar variantes. Los valores de altura nuevos también cambian la conversión entre compresión absoluta y q: revisar si ese cambio preserva correctamente la referencia geométrica, precarga y estado inicial.

## Archivos a seguir y preguntas mecánicas

1. `rust/vehicle-physics-engine/src/open_formula.rs`: muestreo de rayos, creación/reset, secuencia de `step`, conversión de pose y publicación. Comprobar origen/dirección/longitud de rayos, contacto real frente a mero hit, transformaciones y orden temporal.
2. `simulation.rs`: integración del cuerpo, gravedad, fuerzas/reacciones de suspensión, masa no suspendida y actualización de contactos. Trazar quién posee cada aceleración, sin asumir que el solver acoplado alternativo es el utilizado por Open Formula.
3. `suspension.rs` y `simplified_contact_kinematics.rs`: q, velocidad relativa, contacto normal, límites móviles y transformación de la reacción. Auditar signos y unidades, plano de contacto, proyección sobre el eje del cuerpo y cómo se calcula su velocidad.
4. `simplified_axle_integration.rs`: `PreparedAxleCorner::element_forces`, fuerza de barra y resolución conjunta del eje. Derivar los balances de rueda exterior/interior y cuerpo. Comprobar que la barra sea interna, que sus fuerzas y trabajo se compensen y que una rueda sin suelo siga evolucionando con el balance físico correcto.
5. `vertical_suspension_integration.rs`: predictor, residuo implícito, active set/proyección, topes, caída libre y reacción unilateral. La API recibe masa y una función de fuerza; identificar explícitamente dónde se incorporan gravedad de la masa no suspendida, aceleración del anclaje, términos del marco rotante e intercambio con el cuerpo. Que no aparezcan como argumentos explícitos es una pista para rastrear, no una prueba de ausencia en todo el modelo.
6. `simplified_suspension.rs`, `suspension_kinematics.rs` y datos geométricos: precarga, muelle que puede desasentarse, relación de movimiento y su derivada, tabla interpolada, rama cinemática, límites alcanzables reales, signo de amortiguador y stops. Medir el límite real, no solo el droop nominal del perfil.
7. `suspension_setup.rs`: confirmar coeficientes efectivos del reglaje realmente cargado. Separar la traducción ya corregida de defectos posteriores del modelo.
8. `src/modules/simu/rustphysics/rustphysics.cpp`: comprobar que la publicación visual no aparente una rueda suspendida cuando el estado físico es distinto.

Estas son líneas de investigación, no causas confirmadas. No afirmar que la gravedad falta, la barra tiene signo invertido o el active set bloquea una rueda sin una reproducción y un balance de fuerzas/impulsos que lo prueben.

## Instrumentación y bancos que debe entregar el agente

Registrar por paso aceptado y por rueda: identificación del reglaje/coefs efectivos, q y límites, velocidad relativa, altura/velocidad mundial del buje, punto/normal/plano del suelo, hueco y velocidad normal, hits por rayo, fuerza de muelle, amortiguador, barra, topes, peso/términos inerciales, impulso normal, impulso de tope, iteraciones/residuo y motivo de activación/desactivación. Separar estados finales de cargas medias por intervalo. Evitar saturar el log general; crear una traza CSV acotada y reproducible.

Antes de cambiar producción, reproducir y comparar:

- Una esquina sin suelo: evolución de masa no suspendida y cuerpo con muelle/damper; verificar gravedad y aceleración relativa en caída libre según la convención elegida.
- Retirar/bajar el apoyo de una rueda partiendo del equilibrio y medir extensión/recuperación, con y sin barra; comparar con una solución analítica simple o un modelo independiente.
- Eje asimétrico con rueda exterior comprimida e interior sin contacto: balance de impulsos, acción/reacción y energía de barra.
- Elevación y aceleración prescrita del cuerpo, roll y pista inclinada: detectar errores al confundir velocidades relativas con velocidades mundiales.
- Muelle desasentado, máximo droop, tope y aterrizaje; active set con entrada y salida de contacto, para descartar restricciones pegadas.
- Barridos de dt/subpasos y orden de rueda: estabilidad, convergencia, simetría y ausencia de impulso de tracción normal.
- Dos reglajes realmente diferentes con idéntico estado inicial/input, trazando coeficientes efectivos. Medir cuándo y por qué cambia la recuperación, sin prometer cuatro apoyos durante cualquier transferencia lateral.

Usar como regresión `cargo test --offline -p vehicle_physics_engine --test setup_authority`. Hay 15 pruebas que pasan. El verificador dinámico de `verify_setup_authority.py` exige respuesta de los diez controles y equivalencia al elemento físico. Requiere como referencia la DLL anterior respaldada (ver `completion.md`); no comparar dos copias de la misma DLL y declarar regresión aprobada. El target `cargo test --lib` tiene fixtures ajenos ausentes: distinguirlo del resultado de estas pruebas.

## Entrega esperada

Informe con: causa confirmada o hipótesis pendientes; ecuaciones y convención de signos; reproducción mínima; balance cuantitativo de fuerzas/impulsos/energía; archivos y corrección propuesta; regresiones y procedencia de DLL. Si hay corrección, aplicar solo después de aislar el defecto y mantener los reglajes del usuario. Separar validación técnica, instalación y prueba manual. **La conducción en T3 y su aceptación las realiza el usuario.**
