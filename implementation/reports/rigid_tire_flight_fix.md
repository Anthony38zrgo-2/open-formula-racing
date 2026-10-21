# Reparación del lanzamiento y propagación de NaN

Seguimiento posterior: [corrección del bloqueo por reactivación de contactos](rigid_contact_active_set_fix.md). Ese informe documenta la DLL actualmente instalada; este conserva la reparación y los resultados de la fase anterior.

Fecha: 21 de octubre de 2026. Base Git: bfb708b4500efe15dba347da20af40f46a5e0fe0, sobre los cambios locales existentes de contacto rígido. Sin commit ni push.

## Resultado

Los mecanismos reproducidos en rigid_tire_flight_audit.md están corregidos y la DLL candidata está instalada. No se cambiaron reglajes ni XML del vehículo. La conducción humana en Fuji sigue pendiente; no se afirma que se haya reproducido y medido la secuencia exacta del accidente original.

La política de geometría inicial es explícita: un estado penetrado más allá del error numérico se rechaza y congela de forma diagnosticable el vehículo; no se recupera convirtiendo la penetración en velocidad física. No es un límite artificial a la fuerza ni una corrección de suspensión. Las transiciones normales de terreno se interpolan desde la muestra anterior hacia la actual, lo que evita introducir ese solape por un desfase del adaptador.

## Cambios físicos y numéricos

- tire_vertical_solver.cpp/.h: comprobación de finitud de configuración, estado, tiempo, terreno, sistema lineal, iterados y solución. Un NaN/Inf no puede aceptarse como convergido. Fallos identificados por causa.
- Rechazo de penetración inicial significativa antes de resolver: no se reintenta ese mismo solape a un dt menor. La tolerancia de 1e-7 m cubre redondeo; no se convierte tampoco ese pequeño offset tolerado en velocidad de separación.
- Los residuos de contacto se escalan a velocidad dividiendo por dt, con el mismo escalado en el Jacobiano. La búsqueda de Newton exige mejora finita del residual.
- Active set: se liberan contactos del fondo con reacción negativa, se verifican TODAS las desigualdades habilitadas y desaparece el tratamiento que aceptaba cualquier signo mediante valid=1.
- La reconstrucción de reacciones usa el rango real de las columnas, en lugar de intentar invertir siempre una matriz normal 7x7 singular con pocos contactos. Conserva el impulso generalizado y los signos admisibles.
- Al entrar una desigualdad violada se incorpora a la base independiente. Así se evita ciclar entre puntos de apoyo paralelos con distintos offsets, detectado en el caso de 14 puntos del fondo. Los puntos equivalentes siguen participando en la reconstrucción y todas sus desigualdades se verifican.
- car.cpp: subpasos nominales reales, subdivisión solo para fallos numéricos recuperables e interpolación coherente del terreno. La historia de muestras se confirma únicamente al aceptar el paso. Se contabilizan los subpasos aceptados y se comprueba la representación de las vistas float.
- Se elimina de SimCarCollideZ la proyección de altura/rideHeight y la reflexión de velocidad. Solo notifica contacto a partir de impulsos del fondo aceptados. Se elimina también el clamp angular externo de +/-1 rad. El solver conserva la autoridad física.
- car.cpp, sim.h y simu.cpp: transacción de dinámica del coche y vistas públicas; un fallo restaura el estado completo previo y marca el vehículo como detenido por fallo. Ambas rutas de simulación cancelan también los cambios previos del paso de motor/caja/dirección. Los pasos posteriores y la colisión entre coches excluyen el vehículo fallido.
- collide.cpp: se excluyen coches con fallo de los callbacks y de la aplicación de impulsos de colisión; un fallo vertical no deja evolucionando el vehículo con fuerzas antiguas.

La notificación de contacto del fondo se conserva. No se ha inventado una nueva ley de daño a partir de impulsos ni restaurado daños de rueda para cualquier golpe de chasis. El mapeo completo de daño del fondo continúa como limitación declarada del trabajo previo.

## Pruebas

Compilación Release con CMake/Ninja/UCRT64 en C:/Users/bill/open-formula-build/engine. Las advertencias preexistentes de formato en collide.cpp no impiden el enlace.

| Banco | Resultado actual |
| --- | --- |
| validate_rigid_contact_safety (nuevo) | 96 comprobaciones, 0 fallos |
| validate_formula_tire_vertical_system | 208 comprobaciones, 0 fallos |
| validate_formula_vertical_authority | 71 comprobaciones, 0 fallos |
| validate_formula_tire_geometry | salida 0 |
| validate_formula_tire_contact | salida 0 |
| validate_formula_tire_forces | salida 0 |
| validate_formula_tire_energy | salida 0 |
| validate_formula_tire_thermal | salida 0 |
| validate_formula_tire_selector | salida 0 |
| validate_formula_aerodynamics | salida 0 |
| validate_williams_chassis | salida 0 |

375 comprobaciones explícitas en los tres bancos principales, más ocho bancos de regresión correctos.

Casos nuevos relevantes:

- El reproducer original de 10 mm se rechaza a 2 ms, 1 ms, 0.5 ms y 31.25 microsegundos con velocidad e impulso nulos y estado sin modificar. Antes, a 31.25 microsegundos producía 320 m/s y 5.61e9 N.
- Se rechazan NaN e infinitos en posiciones, velocidades, suspensión, muestras, dt, fuerzas externas y parámetros mecánicos.
- El contacto del fondo se libera ante fuerza ascendente, sin impulso negativo oculto.
- Impactos desde un estado admisible conservan restitución nula y no crean energía sobre suelo inmóvil, sin fuerzas externas.
- Cuatro ruedas, tres puntos del fondo y un tope simultáneos: resultado factible e invariancia del movimiento al permutar contactos.
- 14 puntos del fondo: 50 pasos convergentes y todas las desigualdades geométricas verificadas, sin permitir penetración adicional en puntos inactivos.
- Fallo vertical en la transacción de producción: posición, giro de rueda y vistas no avanzan; el siguiente paso permanece detenido. Un NaN de entrada no se publica a render.
- El adaptador de producción ejecuta cuatro subpasos de 0.5 ms en un paso de 2 ms. Se corrige también el redondeo float que podía producir un quinto subpaso espurio.
- Cambio de muestra de terreno y 5000 pasos físicos (10 s) de irregularidad prescrita de 2 mm a 8 Hz mediante las funciones de carretera y actualización vertical reales: sin fallo ni NaN, velocidad vertical máxima 0.052163869 m/s y carga máxima 3714.10498 N. Es una prueba controlada del adaptador, no conducción completa en Fuji.

Los casos antiguos que comenzaban con ruedas dentro del terreno se sustituyeron por perturbaciones compatibles con contacto rígido o por caídas desde posiciones admisibles. Se mantienen pruebas específicas de rechazo de geometría inválida.

La comprobación de pasividad compara energía mecánica real con trabajo EXTERNO del terreno. El trabajo positivo de una restricción ya no se resta como una justificación de energía creada. Las estimaciones separadas de disipación siguen siendo diagnósticos y no un certificado por sí solas. CheckClose rechaza explícitamente NaN/Inf.

## Instalación y reversión

- DLL instalada: distribution/lib/games/speed-dreams-2/modules/simu/simuv5.dll.
- SHA-256: 759F8BE64E0ADCA9281C4AD81C4FA2E438237921B0214CA89E53314A340AD86D.
- DLL anterior respaldada: 733D44685F3F3BB04359D4746722E9941A75B320813E0F254A14F016F8C2B15D.
- XML instalado, sin modificar: EDEEB8F3F3F448EDC1E33F35FB431C6AFE6344D3E9E75AB3C0B82BB82343F2AD.
- Se actualizó exclusivamente la entrada de la DLL en el manifiesto de distribución.
- Directorio de respaldo y registros: D:/open-formula-migration-evidence/rigid-tire-flight-fix-20261021.

Se comprobó que no existía una sesión abierta antes de sustituir la DLL. Para volver a la versión previa, con el juego cerrado, copiar simuv5.before.dll del respaldo a la ruta de la DLL y restaurar distribution_manifest.before.json si se revierte también el manifiesto. El XML respaldado es idéntico al instalado y no necesita cambiarse. La versión anterior sigue siendo la que presentó el fallo; el respaldo sirve para comparación, no acredita su estabilidad.

## Límites y siguiente validación

- No se ejecutó una nueva sesión gráfica ni se autoconcedió aceptación humana. Faltan conducción real en Fuji, pianos, pendientes, alta velocidad sostenida y retorno a boxes con esta DLL.
- Ante geometría incompatible o un fallo numérico irrecuperable, el coche queda detenido y se registra la causa; no se oculta la anomalía ni se deja continuar hasta NaN. Si ocurre durante conducción, conservar ese primer diagnóstico y las condiciones de contacto para investigar el origen de la incompatibilidad.
- No se retocaron muelles, amortiguadores, curvas de grip o aerodinámica para enmascarar el problema. El dominio angular y el contacto mediante altura vertical siguen sujetos a los límites declarados del modelo.
- La evidencia del accidente original permanece en rigid-tire-flight-audit-20261021. El contacto concreto que inició esa sesión sigue sin atribución instrumentada.
