# Entrega actual: escritor de telemetria en segundo plano

Instaladas las DLL con hilo dedicado, cola limitada a dos ventanas, capturas por episodio y resumen a **25 Hz**. **9 pruebas de telemetria y 48 comprobaciones de DLL pasan**. Documentacion vigente: [async_writer.md](async_writer.md). FPS en juego y T3 pendientes de prueba manual.

- `vehicle_physics_engine.dll`: `dcc0b44437ee5746cffe2325fd6c71f01c61af97bf901658d197ad1916faee90`
- `rustphysics.dll`: `00ec1ad1ca057b623bb69d189dff555e9464257d76c13086629a61c12087eddb`

Respaldo: `C:\Users\bill\open-formula-backups\2026-10-23-telemetry-6cd97aab`. Recibo: `installation.json`.

---

# Telemetria corregida y lista para prueba manual

Estado actual: correcciones de la revision instaladas. **44 pruebas de integracion y 47 comprobaciones de DLL pasan**; las 7 pruebas de telemetria se repitieron sobre el ultimo fuente. ABI 7, firma `0xa91cdeba9d3fe775`. T3 sigue pendiente del usuario.

Contrato vigente, detalles y limites: [correction.md](correction.md). La descripcion anterior que sigue debajo es historica: el CSV actual tiene **164 columnas**, la carga normal y la suma de elementos son distintas, la separacion positiva indica rueda elevada, la perdida de apoyo no depende de ver suelo, y la ventana informa si se sobrescribio historia.

DLL actuales:
- `vehicle_physics_engine.dll`: `e76faab4b13139b0dc48c4ea20766d705ce9b268a71c0001787ededb76c212b7`
- `rustphysics.dll`: `5d4ee69fa9361619edde7cef28f424382bb33b436fded8ea7825ae739e928c26`

Respaldo y rollback: `C:\Users\bill\open-formula-backups\2026-10-23-telemetry-38ea21ad`. Instalacion/hashes y archivos protegidos: `installation.json`.

---

# Telemetría de suspensión vertical, 23 de octubre de 2026

**Estado: instrumentación instalada y verificada sin arrancar el juego.** 41 comprobaciones de ABI y comportamiento sobre la DLL real y la malla real de Melbourne, más las 37 pruebas de física existentes (6 de acoplamiento, 16 estructurales, 15 de autoridad de reglaje), todas en verde. **No se ha conducido**: la pérdida de apoyo de FR en T3 y el rechazo de `35.33 s` siguen sin reproducirse, y esa reproducción es lo que esta entrega hace posible.

DLL instaladas: `vehicle_physics_engine.dll` `91dc8c9c765093a05c15121aaab8e04f6f587b0b179ae7a59d697dc33c7e6198` y `rustphysics.dll` `569577736ddb112d9ceecd71f39d5e617a8a60e21bff77c292516bb713087e73`. ABI `7`, firma de disposición `0xa91cdeba9d3fe775`, comprobada por el mismo portero que usa el juego al cargar (`abi_probe.exe`, incluido). Respaldo de las DLL anteriores a esta entrega y reversión en `C:/Users/bill/open-formula-backups/2026-10-23-telemetry-b8bcf05f` (cerrar el juego antes de ejecutar `rollback.ps1`); los directorios `-db3dc4f4` y `-57412034` son instantáneas intermedias del mismo proceso. No se modificó ningún reglaje guardado, ni el perfil del coche, ni `open_formula.db`: sus hashes están en `installation.json` y se compararon antes y después.

## Lo que ahora se registra

Cada fila es una medida de la resolución aceptada (o rechazada), no una reconstrucción a partir del estado visual. Las unidades son SI salvo que se indique otra cosa.

| Dato pedido | Dónde aparece | Qué permite distinguir |
|---|---|---|
| Separación neumático-suelo, punto y normal de contacto, resultado de cada raycast | `RUSTSUSP sep_m sep_n_m shortfall_m`, `RUSTDIAG sep=(…)`; `contact_point`/`contact_normal` y `ray_hit_mask` en el snapshot | Si existe suelo detectable y cuánto le falta a la rueda para alcanzarlo. `shortfall_m > 0` significa que ningún rayo encontró suelo; `sep_m > 0` con `ground=1` significa que la rueda lo ve y aún no llega |
| Compresión, velocidad axial, límites efectivos | `RUSTSUSP comp_m vax_m_s lim_m=(min,max) travel_m` | Si la rueda se está extendiendo, está detenida en un tope o va retenida antes de alcanzarlo |
| Fuerzas separadas de muelle, amortiguador y barra, con signo y relación de movimiento | `RUSTSUSP spring_n damper_n arb_n stop_n sum_n mr` | Qué componente impide o favorece que la rueda baje. Positivo empuja la rueda hacia el suelo; `sum_n` es la suma con signo |
| Impulsos de contacto, tope de extensión y de compresión | `RUSTSUSP jN jBump jDroop stop=(bump,droop)` | Qué restricción está sosteniendo o bloqueando la rueda |
| Posición, velocidad y aceleración del anclaje; aceleración del chasis | `RUSTSUSP anchor_p anchor_v anchor_a axial_a`; `RUSTDIAG` publica la aceleración angular del chasis en el snapshot | Si el movimiento del chasis aleja el anclaje más rápido de lo que baja la rueda |
| Compresión exigida por el suelo, actual y predicha, frente al máximo | `RUSTSUSP demand_m pred_m headroom_m`; `RUSTFAIL demand_m limit_m excess_m` | La incompatibilidad exacta que produjo el rechazo |
| Iteraciones, residuos, subdivisiones y motivo, con rueda y eje | `RUSTAXLE iterations converged substeps residual_m_s`; `RUSTFAIL wheel axle iteration subdivisions code`; `RUSTDIAG iters conv resid` | Si el problema es de restricciones incompatibles o de falta de convergencia |

Códigos de rechazo (`code`): `1` suelo incompatible con el tope de compresión, `2` el eje no cierra contacto y recorrido a la vez, `3` la transacción cuerpo-esquina no cierra sus aceleraciones, `4` estado no finito.

## Dos niveles de captura

- **Resumen a 50 Hz** (`diagnostic interval seconds = 0.02`; `0.01` da 100 Hz). Cada muestra escribe `RUSTDIAG` (una línea), cuatro `RUSTSUSP` (una por rueda) y dos `RUSTAXLE` (una por eje). Los 250 ms anteriores quedan divididos por doce: una pérdida de contacto o una recuperación breve ya no cae entre dos muestras.
- **Anillo de subpaso e iteración dentro de la DLL de Rust**, activado con `diagnostic trace = 1`. Guarda cada tick aceptado, cada iteración cuerpo-esquina, cada subpaso de eje y el candidato rechazado. Cuando hay pérdida de contacto sostenida más de 250 ms, llegada a un tope o rechazo, escribe su propia ventana en `user-settings/rustphysics/`: **3 s anteriores y 1 s posteriores** al evento (`.csv` con una fila por registro y 76 columnas: 16 escalares y 15 por rueda en orden FL/FR/RL/RU, más un `.json` con el motivo, la ventana y —si hubo rechazo— rueda, eje, demanda, límite y exceso). En un rechazo el volcado es inmediato porque la simulación se congela: contiene el último tick aceptado (`kind=1`) junto al candidato rechazado (`kind=4`), que es exactamente la pareja que faltaba.
- El anillo no cuesta nada cuando está apagado (una lectura atómica por registro) y se dimensiona para 20 480 registros. Cada ventana que escribe deja además una línea `RUSTTRACE reason=… trigger_t=… wheel=… rows=… csv=…` en el registro del juego, para que el archivo no aparezca sin explicación.

## Las dos ambigüedades corregidas

- `reach` desaparece. Ese campo contenía `steering_angle_rad`, no una distancia de alcance; ahora se llama `steer_rad` y conserva su unidad.
- Tras un rechazo se lee el snapshot otra vez para que el contador esté actualizado, y se escribe **una** línea `RUSTFAIL` con `failed=1 frozen=1 rejected=<n> state_time=<t>`. El resumen periódico deja de repetir el mismo `time=` congelado como si fuera telemetría viva (`frozen` corta la repetición). El estado publicado sigue siendo el último aceptado: ni la posición, ni el tiempo, ni la carga se sustituyen por los de un paso rechazado.

## Reglaje efectivo

Después de aplicarlo y al crear el coche se escribe `RUSTSETUPEFFECTIVE` y, debajo, una línea `RUSTSETUP` por valor:

- `FIELD path=… label="…" value=… unit="…" min=… max=… step=… editable=0|1` para los 38 campos del garaje;
- `DERIVED wheel=…` con compresión en reposo, longitudes de muelle libre/instalado, precarga, rigidez del muelle, amortiguamiento de compresión y extensión, rodilla y factor rápido, recorridos de extensión y compresión, radio de neumático;
- `DERIVED axle=…` con la relación de barra y su configuración efectiva;
- `DERIVED mass_kg`, reparto delantero y reparto de frenada;
- `DERIVED effective_elements=…` con los elementos que el solver consume de verdad.

Se lee de la configuración en ejecución, nunca de la petición: el log no puede mostrar un valor que el coche no adoptó.

## Verificación (`telemetry_validation/`)

`verify_telemetry.py` carga la DLL real y la malla real de Melbourne, y comprueba: versión y firma de ABI; tamaños y desplazamientos (3480 / 264 / 48 / 80 bytes); cuatro apoyos con separación sub-milimétrica; límites que contienen la compresión; coherencia de `sum_n` con los cuatro elementos; relación de movimiento positiva; cinemática del anclaje finita; ambos ejes convergentes; anillo llenándose y volcado completo; las 76 columnas por rueda; `FIELD`/`DERIVED` con unidades y límites; un rechazo sintético con `code=1 wheel=2 axle=1 demand=0.1804 limit=0.1694 excess=0.0110`, contador en 1 y estado congelado; y una pérdida de apoyo real que escribe su ventana de 3 177 filas con 623 filas de rueda sin suelo.

`abi_probe.cpp` reproduce el portero del cargador C++ y confirma que aceptaría la DLL instalada; se compila con
`g++ -std=gnu++11 -O2 -I src/modules/simu/rustphysics abi_probe.cpp` desde `implementation/reports/telemetry_2026_10_23`. Las 37 pruebas de física existentes siguen pasando sin cambios.

Artefactos en `telemetry_validation/`: `telemetry_validation.json` (las 41 comprobaciones), `settled_trace_excerpt.csv` (extracto de 200 filas del volcado completo, que regenera el verificador), `rejection_trace.csv`, `effective_setup.txt`, `trace/` (las dos ventanas automáticas con su `.json`) y `verify_stdout.txt`/`verify_stderr.txt` con las líneas `RUSTTRACE` tal como llegan al registro del juego.

## Próxima prueba (la que falta)

Abrir `distribution/OpenFormula.exe`, cargar el reglaje y repetir T3. Al terminar, en `distribution/user-settings/`:

1. `runtime.log`: buscar `RUSTFAIL` (qué restricción rechazó y con qué números) y, en las muestras previas, `RUSTSUSP wheel=FR` — `sep_m`, `vax_m_s` y `distancia a lim_m` dicen si FR se extendía, estaba en un tope o iba retenida; `spring_n`, `damper_n`, `arb_n` y `sum_n` dicen qué componente la sujetaba.
2. `rustphysics/`: las ventanas `.csv`/`.json` de la pérdida de contacto de FR y del rechazo. La fila `kind=4` es el candidato exacto que se rechazó; la última `kind=1` anterior es el estado aceptado.

Con eso se puede decidir entre extensión insuficiente, amortiguación/barra, geometría de contacto o defecto del solver sin seguir ajustando constantes a ciegas.

## Lo que esta entrega no demuestra

No hay sesión de conducción: no se ha reproducido T3, ni la pérdida de FR, ni el rechazo de `35.33 s`. Los disparadores de pérdida de contacto y de tope se ejercieron en banco (el de pérdida de contacto con un coche en el aire), pero no con la maniobra real. Tampoco se ha medido el coste por paso dentro del juego; el resumen a 50 Hz escribe del orden de 270 KB/s de texto y el anillo añade unos 20 MB de memoria residente sólo mientras la traza está activada.
