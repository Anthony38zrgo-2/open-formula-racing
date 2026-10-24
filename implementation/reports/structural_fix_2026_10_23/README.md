# Corrección del acoplamiento cuerpo–ruedas, 23 de octubre de 2026

**Estado final: candidata instalada y comprobada desde la distribución.** 37 pruebas de integración Release (6 de acoplamiento, 16 estructurales, 15 de autoridad) y 35 comprobaciones SQLite sin fallos. La suite completa `--lib` no se ejecutó: mantiene referencias a fixtures de otros vehículos ausentes, ya documentadas en los bancos anteriores.

DLL instalada: `ba4dc5a73277bb13b4f86f9fb54790ae9cb3d99530bf423ca3f2dc813edbead7`; ABI `0xeebdad2292b2359d`. Respaldo de la DLL anterior y manifiesto en `C:/Users/bill/open-formula-backups/2026-10-23-suspension-coupling-ca311edd`; reversión mediante su `rollback.ps1`, con el juego cerrado. No había una sesión abierta durante la instalación.

La prueba offline con la malla real de Melbourne pasó con perfil base y «Reglaje 1»: 240 pasos aceptados por caso, un segundo simulado, cuatro apoyos finales y ningún rechazo. Coste medido tras instalar: 0,52 / 0,58 ms medios por paso en esos casos; no representa una medición de FPS del juego completo. Se conservaron los hashes de todos los JSON del perfil y de `user-settings/open_formula.db`. No se inició una partida ni se sobrescribió su telemetría.

Continúa el trabajo local de corrección estructural. No recalibra reglajes, grip ni aero. La aceptación de conducción en T3 corresponde al usuario.

## Cambios mecánicos

- `simplified_body_coupling.rs`: complemento de Schur de la masa del cuerpo y cuatro masas puntuales con recorrido axial. Las ruedas conservan su contribución transversal y angular; no se acelera todo el coche como si solo pesara la masa suspendida. Se mantienen los términos cruzados de traslación/rotación respecto a la misma referencia de masa del perfil. La inercia intrínseca de las esquinas permanece en el tensor orientado con el cuerpo; el giro de rueda conserva su propietario anterior.
- `simulation.rs`: el cuerpo y las esquinas iteran sobre el mismo intervalo implícito. Cada intento parte de una copia completa del estado y configuración; solo se acepta el intervalo cuando cierran las aceleraciones. Los intentos rechazados no acumulan combustible, calor, tiempo ni giro de rueda. Se subdivide antes de fallar; si no hay solución, se conserva el estado aceptado completo y se publica el motivo.
- `simplified_axle_integration.rs`: suelo incluido en el criterio conjunto, resolución escalar acotada tras eliminar una esquina si la iteración rápida no converge, y validación de las dos esquinas finales. Se rechaza una combinación incompatible de suelo y tope; no se publica el antiguo fallback secuencial sin comprobar. La aceleración del anclaje incluye traslación, aceleración angular, centrípeta y Coriolis. La posición final del plano se calcula al mismo tiempo implícito que el cuerpo, proyectando sobre la normal real.
- `suspension.rs`: reacción del cuerpo calculada desde los impulsos internos aceptados del intervalo, incluyendo el impulso opuesto de los límites de recorrido. Se excluyen de esa reacción peso y pseudofuerzas de la esquina. La reacción normal publicada se convierte de coordenada generalizada a normal física una vez, usando su proyección.
- `open_formula.rs`: fallo transaccional de la sesión, sin publicar posición/tiempo parciales; respuesta de impulsos de cuerpo con la misma movilidad acoplada. ABI pública sin cambios.
- `vehicle_mass_properties.rs`: caché de auditorías puras de masa limitada a una transacción de un solo coche. Distingue combustible y backend, se restaura al salir incluso en subdivisiones y no persiste entre coches o cambios de reglaje. Evita repetir las mismas auditorías en cada iteración sin cambiar coeficientes ni resultados.

El modelo sigue siendo una suspensión simplificada de masas axiales: no se presenta como un multibody completo de cada brazo y balancín. No se activó el backend Standard ni otro mundo físico paralelo.

## Pruebas

Suite `body_coupling`: seis pruebas independientes para masa total horizontal, transporte de inercia de guiñada, aceleración de anclaje rotante, momento lineal mundial en caída libre, impulso de topes y reversión de un intervalo inviable.

Suite `structural_contact`: se elimina el atributo de prueba duplicado; el oráculo de caída libre convierte la aceleración relativa a mundial. El banco de pasos gruesos compara contra una ejecución fina, en lugar de exigir la velocidad del modelo defectuoso anterior. Los bancos de plano e inclinación fallan si el paso anterior fue rechazado, para no aceptar una simulación congelada como estable. Se mantienen los escenarios de bordillo, contacto parcial, reset y barrido temporal.

Suite `setup_authority`: se mantienen las quince pruebas de conexión, equivalencia e idempotencia de reglajes.

La prueba offline `smoke_dll.py` comprueba el ABI y carga la malla real de Melbourne con el perfil base y el reglaje SQLite del usuario, sin iniciar el juego ni sobrescribir su runtime.log. Solo acredita arranque/asentamiento, estado finito y pasos aceptados; no valida T3 ni conducción con mando.

Resultados finales y procedencia: `smoke.json`, `installation.json` y registro de pruebas de esta entrega. La instalación solo debe ejecutarse después de pasar esas verificaciones; el script guarda DLL/manifiesto anteriores fuera del repositorio y registra hashes de los perfiles y la base de reglajes para comprobar que no cambian.

## Uso y reversión

Abrir `D:/open-formula/distribution/OpenFormula.exe` y cargar/aplicar el reglaje deseado antes de probar T3. La extensión sigue siendo amortiguación de rebote; aumentarla no aumenta el recorrido disponible.

El archivo `installation.json` identifica la DLL instalada y la ruta absoluta al `rollback.ps1` del respaldo. Ejecutar la reversión con el juego cerrado. No se hace commit ni push como parte de esta entrega.
