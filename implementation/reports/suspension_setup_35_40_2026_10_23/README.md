# Suspensión Williams: ride 35/40 mm y topes de 5 mm

Fecha: 23 de octubre de 2026. Candidato local calibrado a petición del usuario; aceptación de conducción pendiente.

## Resultado y parámetros

Las alturas se obtienen modificando muelles y precarga, manteniendo los puntos geométricos y el plano físico de referencia del fondo. No se desplazó el fondo aerodinámico para aparentar otra altura.

| Magnitud | Delante | Detrás |
| --- | ---: | ---: |
| Ride objetivo en reposo | 35 mm | 40 mm |
| Ride medido con DLL, suelo plano, combustible 7.6 kg | aproximadamente 35.014 mm | aproximadamente 39.991 mm |
| Recorrido a compresión desde esa posición estática | aproximadamente 30 mm | aproximadamente 35 mm |
| Altura nominal residual al final de compresión | 5 mm | 5 mm |
| Zona progresiva de trabajo del tope | últimos 5 mm | últimos 5 mm |
| Tolerancia numérica de sobrepaso del límite | 0.1 mm | 0.1 mm |
| Muelle físico, coordenada del elemento | 200 N/mm | 1200 N/mm |
| Rigidez efectiva local del muelle en rueda cerca de reposo | 151.9 N/mm | 144.8 N/mm |
| Precarga geométrica, longitud libre menos instalada | aproximadamente 11.429 mm | aproximadamente 1.64 mm |
| Amortiguador compresión / extensión, Ns/m de elemento | 8075 / 9938 | 47432 / 58378 |
| Ratio de barra | 0.25 | 0.06 |
| Rigidez de acoplamiento de barra en rueda | 38.36 N/mm | 9.88 N/mm |
| Cuota elástica local de barra en roll cerca de reposo | 33.6 % | 12.0 % |

El coeficiente de muelle trasero es mayor porque su transmisión al elemento es mucho menor: relación de movimiento aproximadamente 0.359 frente a 0.829 delante. Comparados en la rueda, los dos ejes tienen rigideces principales similares. Las cuotas de barra son locales y excluyen neumáticos, topes y amortiguación; no son porcentajes de transferencia total de carga ni se mantienen constantes durante todo el recorrido.

La referencia de ride es la altura vertical del plano de fondo existente en las estaciones de los ejes, con suelo plano, neumáticos del perfil y 7.6 kg de combustible. El mando legado `Altura / ratio` sigue siendo una referencia de suspensión sin unidades de mm; los valores de esta entrega son alturas físicas verificadas, obtenidas desde la precarga del elemento.

Para conservar 5 mm de altura residual con 40 mm de ride trasero, el límite geométrico de bump trasero pasa de q=30 a q=40 mm desde diseño. El límite delantero permanece en q=25 mm. Se verificó que el mecanismo preparado alcanza ambos límites. No se modificaron los hardpoints.

## Implementación

- `rust/data/vehicles/williams-fw19-hockenheim/suspension.json`: muelles, longitudes libres, amortiguadores, barras, topes y bump trasero calibrados.
- `profile_provenance.json`: valores estimados y calibrados, con referencias a los parámetros y bancos; no se presentan como medidas históricas del FW19.
- `suspension_geo_config.rs`: propiedades opcionales por eje `bump_stop_travel_m` y `hard_stop_overtravel_m`, validación y conservación en serialización.
- `suspension.rs`: la zona progresiva utiliza la longitud explícita en rueda. Los perfiles que no la declaran conservan la ley anterior.
- `wheel_mechanics.rs`: sobrepaso numérico por eje desde el perfil; los perfiles anteriores sin ese campo conservan su valor previo.
- [Parámetros reproducibles](../../../tools/williams_suspension_setup_35_40.json) y [aplicador](../../../tools/configure_williams_suspension.py). El aplicador modifica exclusivamente los campos de suspensión declarados.

Se conserva la ley de fuerzas y el integrador existentes, incluidas las reacciones de las ruedas sin apoyo. No se añadieron ayudas de estabilidad, cambios de grip o escalados de aerodinámica. La inconsistencia pequeña de interpolación descrita en la segunda opinión no se modificó en este encargo de configuración.

## Validación técnica

**51 pruebas pasan**: 6 de acoplamiento del cuerpo, 15 de autoridad del reglaje, 16 de contacto estructural, 5 específicas del candidato y 9 de telemetría. Las pruebas nuevas verifican altura física con la API real, apoyo sin topes, comienzo en los últimos 5 mm, altura residual de 5 mm, contribución secundaria de barras, serialización, rechazo de longitudes inválidas y compatibilidad de perfiles anteriores.

Las regresiones de autoridad conservan una instantánea del perfil anterior como fixture, porque comprueban valores de diseño concretos e idempotencia, mientras las pruebas nuevas cubren el candidato actual. Dos supuestos de bancos heredados se corrigieron: la comparación temporal usaba una altura fija del reglaje anterior en lugar del resultado de paso fino; la prueba de peralte colocaba inicialmente el coche nivelado sobre una pendiente que exigía unos 42 mm de bump, antes ocultos por 30 mm de sobrepaso numérico. Ahora se inicia alineado a la pendiente y los rayos sintéticos intersectan la dirección real de suspensión. No se relajaron límites de producción para aceptar esos casos.

[Resultados de pruebas](tests_final.log), [validación de DLL](dll_validation.json) y [resumen de aceptación](validation.json).

### Velocidad mantenida en banco

| Velocidad | Ride delantero | Ride trasero | Actuación de topes en la ventana estable |
| --- | ---: | ---: | ---: |
| Reposo | 35.014 mm | 39.991 mm | 0 % |
| 100 km/h | 34.028 mm | 38.653 mm | 0 % |
| 200 km/h | 30.041 mm | 32.222 mm | 0 % |
| 250 km/h | 27.998 mm | 28.042 mm | 0 % |
| 300 km/h | 25.952 mm | 23.028 mm | 0 % |

[Datos](speed_sweep.csv). Se mantiene externamente la velocidad horizontal para medir la plataforma con las fuerzas aerodinámicas de producción. Es un banco de carga, no una vuelta conducida ni una medición de velocidad máxima.

### Maniobras con integración libre

El candidato completa los cuatro escenarios sin rechazo de paso y sin fuerza de tope. En frenada recta, frenada con inversión de giro y slalom, no se midieron intervalos con N<1 N en ninguna rueda mientras el vehículo se movía a más de 5 m/s. El roll máximo en la inversión de giro fue aproximadamente 0.483°, frente a 0.552° de la base, y en el slalom 0.224°, frente a 0.293°.

El escalón de 10 mm bajo una rueda conserva vuelos breves: aproximadamente 0.138 segundos-rueda delanteros y 0.036 traseros, frente a 0.100/0.026 de la base. La cifra suma ruedas; no significa que todo el eje estuviera en vuelo durante ese tiempo. El coche recupera apoyo y el roll máximo baja de aproximadamente 0.830° a 0.612°. Sigue siendo un escenario específico para revisión humana; no se declara una mejora universal sobre pianos.

[Resumen de maniobras](maneuver_summary.csv). Las entradas son sintéticas y no reproducen exactamente la T3 de la sesión del usuario. El primer candidato 0.15/0.12 mostró inestabilidad en inversión de giro y se descartó; sus datos están en `candidate_v1/`. El reparto elegido 0.25/0.06 se evaluó con las mismas entradas. Su interpretación depende de los muelles y la precarga nuevos; no equivale a aplicar esos ratios a la suspensión anterior.

## Instalación y restauración

Se instalan exclusivamente la DLL `vehicle_physics_engine.dll`, el dominio `suspension.json` y su `profile_provenance.json`, actualizando sus hashes y tamaños en el manifiesto de distribución. La interfaz binaria sigue en ABI 7 con la misma firma; el puente C++ no requiere un cambio de interfaz.

El recibo [installation.json](installation.json) contiene hashes, archivos protegidos, respaldo y restauración. `baseline_rollback` restaura la distribución que había antes de este encargo completo; `rollback` restaura la versión inmediatamente anterior a la última instalación. La base SQLite de reglajes y los archivos protegidos mantienen sus hashes. Los reglajes antiguos permanecen guardados; el sistema de compatibilidad puede rechazarlos al pertenecer a otra base física. No se migraron ni sobrescribieron silenciosamente.

El respaldo de fuentes previo al trabajo está indicado en [baseline.json](baseline.json). La DLL instalada se verifica también mediante carga directa y API en [installed_dll_validation.json](installed_dll_validation.json).

Pendiente: prueba manual del usuario en frenada fuerte, inversión de dirección, T3 y pianos, con la nueva configuración. La validación técnica y la instalación no equivalen a aceptación de conducción. La [telemetría está temporalmente desactivada](../../../docs/investigation/telemetry_switch.md) a petición del usuario.

## Reproducción

Desde la raíz del repositorio:

```powershell
python tools/configure_williams_suspension.py --check
Push-Location rust
cargo test --offline --release -p vehicle_physics_engine --test suspension_35_40 --test setup_authority --test structural_contact --test body_coupling --test vertical_telemetry
cargo build --offline --release -p vehicle_physics_engine --lib
Pop-Location
python implementation/reports/suspension_setup_35_40_2026_10_23/verify_dll.py
python implementation/reports/suspension_setup_35_40_2026_10_23/validate.py
```

`calibrate.rs` conserva el procedimiento de calibración desde la instantánea anterior; `maneuvers.rs` utiliza el integrador de producción y rayos de un suelo sintético. Los parámetros congelados del aplicador permiten reproducir la configuración sin volver a ajustar numéricamente el perfil.

La compilación se ejecuta desde `rust/` para cargar `rust/.cargo/config.toml`, que enlaza estáticamente el CRT y mantiene la DLL autocontenida para el host C++ de la distribución.
