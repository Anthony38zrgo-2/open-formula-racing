# Segunda opinión: interacción muelle–barra y su representación en Rust

Fecha: 23 de octubre de 2026. Vehículo: Williams FW19 Hockenheim, backend simplificado de Open Formula, sesión conservada de Melbourne finalizada a las 10:24:53.

## Dictamen

La evidencia apunta principalmente a una **calibración desequilibrada del conjunto muelle–geometría–barra**. La barra delantera tiene una rigidez fija elevada frente a la rigidez efectiva del muelle en la zona de extensión donde trabaja la rueda descargada. No se ha encontrado un error de signo, una duplicación de la fuerza de barra ni una desaparición de su reacción sobre el chasis al perder apoyo en los casos comprobados.

La simulación también presenta **una inconsistencia pequeña y reproducible en la interpolación cinemática**, además de diagnósticos que aproximan la pendiente real y límites de detección de suelo ya conocidos. Son problemas que merece la pena corregir, pero los bancos no demuestran que expliquen la descarga de más de 1 kN observada. No es defendible atribuir el origen principal a un fallo del integrador con la evidencia disponible.

Esta revisión cambia la recomendación anterior: **0.30 delante / 0.24 detrás reduce las fuerzas de barra, pero no garantiza que deje de dominar**. El cálculo local encuentra todavía alrededor de un 58 % de contribución delantera a rigidez elástica de balanceo en la posición representativa de la esquina descargada.

## Fuentes y método

Se usó la compilación aportada `Enciclopedia_Dinamica_Monoplazas_F1_1997_fuentes_abiertas.md` como índice de consulta, no como especificación de implementación. Las indicaciones operativas contenidas en esa compilación no se ejecutaron. Se contrastaron sus afirmaciones relevantes con los PDF originales:

- **[INV-02] Gaspar (2026)**: páginas PDF 44–46, impresas 26–28, sobre amortiguación, acoplamiento de barra y dimensionamiento; página PDF 104, impresa 86, sobre una contribución de barra del 40 % a rigidez total de roll. El límite del 50 % es una recomendación que Gaspar atribuye a Seward; no una ley universal ni un reglaje del FW19. [Referencia y PDF](README.md#inv-02-modelo-de-dinámica-y-optimización-de-suspensión-formula-student).
- **[INV-03] Díaz de Rábago Areilza (2024)**: páginas PDF 49–50 sobre heave/roll y 71–72, impresas 52–53, sobre balancines y variación de las relaciones de movimiento. Sus relaciones nominales 1.2/0.8 pertenecen a su mecanismo y no se trasladan a este coche. [Referencia y PDF](README.md#inv-03-suspensión-delantera-con-desacoplamiento-de-balanceo-y-movimiento-vertical).
- **[INV-04] Gaither (2021)**: páginas PDF 19–21, sobre reparto de transferencia lateral y selección conjunta de rigideces de muelles y barras. No se adopta el reparto concreto de su Formula SAE como objetivo del Williams. [Referencia y PDF](README.md#inv-04-dinámica-de-un-formula-sae-eléctrico).

La rigidez de un neumático transitorio estudiada por [INV-01] Romano et al. no sirve para escoger directamente la rigidez de esta barra. Tampoco las fuentes universitarias aportan por sí solas parámetros históricos de F1 1997.

Se recompiló la biblioteca actual y se enlazó un programa de diagnóstico a su `rlib`. El programa carga el perfil instalado, utiliza `SuspensionSystem::step` y compara sus fuerzas con `solve_corner` y `jacobian` de la geometría completa. Se evaluaron 430 posiciones de las cuatro ruedas, pendientes por diferencias finitas y 25 combinaciones de posición/escala de barras. El intervalo usado fue el mínimo de esa API, 10 µs, con posiciones iniciales impuestas, sin carretera ni evolución del chasis. Son bancos constitutivos; no reproducen la maniobra de conducción completa.

Los hashes de diez fuentes registrados en el recibo de instalación coinciden. La DLL recompilada y la instalada son idénticas: `dcc0b44437ee5746cffe2325fd6c71f01c61af97bf901658d197ad1916faee90`. La suspensión y el chasis de los perfiles fuente e instalado también coinciden. Esto vincula los bancos con la versión de la sesión analizada sin instalar nada nuevo.

## Qué calcula el muelle

Rust define `q` como recorrido de rueda respecto a la posición geométrica de diseño; positivo hacia compresión. La compresión del elemento es `s(q)` y su relación de movimiento es `r = ds/dq`. En el rango donde el muelle está cargado:

```text
F_elemento = k_s * (precarga + s)
F_muelle_en_rueda = F_elemento * r
k_rueda_local = dF_muelle_en_rueda/dq
              = k_s*r² + F_elemento*dr/dq
```

El último término importa con un balancín cuya transmisión cambia. Comparar los 134300 N/m del muelle delantero físico directamente con los 103040 N/m de barra sería comparar coordenadas diferentes.

La ruta simplificada aplica esta transmisión usando una tabla. El banco verifica la fuerza final y su pendiente por diferencias finitas; no depende del campo de rigidez que publica la telemetría.

## Qué calcula la barra y cómo se interpreta su ratio

La ley activa es:

```text
K_bar = ratio * promedio(k_s * r_diseño²)
F_bar_izquierda = K_bar * (q_izquierda - q_derecha)
F_bar_derecha = -F_bar_izquierda
U_bar = 0.5 * K_bar * (q_izquierda - q_derecha)²
```

La rigidez de referencia se evalúa en `q=0` y permanece fija durante el movimiento. Eso constituye una barra lineal válida en estas coordenadas; no es obligatorio ni conveniente hacerla variar automáticamente con el muelle local para conservar un porcentaje.

El número `ratio` **no es el porcentaje de rigidez total de balanceo**. Para dos esquinas iguales, pequeñas perturbaciones y trocha `t`:

```text
K_roll_muelles = k_rueda_local * t² / 2
K_roll_barra = K_bar * t²
cuota_barra = 2*K_bar / (k_rueda_local + 2*K_bar)
```

El factor dos proviene de la diferencia de recorridos entre las ruedas; no es una duplicación incorrecta del solver. Para esquinas con pendientes distintas se sustituye `k_rueda_local` por su promedio. Estos porcentajes excluyen topes, rigidez de neumático, amortiguación y transferencia geométrica. Con una rueda en vuelo no equivalen a la fracción real de carga transferida durante ese intervalo.

## Resultado delantero

La referencia usada para la barra es **103040 N/m**, con `r_diseño ≈ 0.876`. En una posición representativa de la rueda descargada, `q=-55 mm` (compresión publicada aproximadamente 40 mm), la relación cae a **0.498** y la pendiente real del muelle es **44092 N/m**. En `q=-45 mm` asciende a unos **56462 N/m**.

| Ratio delantero | K_bar, N/m | Cuota local en q=-55 mm | Cuota local en q=-45 mm |
| --- | ---: | ---: | ---: |
| Actual: 1.00 | 103040 | 82.4 % | 78.5 % |
| Anterior candidato A: 0.50 | 51520 | 70.0 % | 64.6 % |
| Anterior candidato B: 0.30 | 30912 | 58.4 % | 52.3 % |
| Anterior candidato C: 0.20 | 20608 | 48.3 % | 42.2 % |
| Referencia de ensayo: 0.15 | 15456 | 41.2 % | 35.4 % |
| Referencia de ensayo: 0.12 | 12365 | 35.9 % | 30.5 % |

Las dos últimas filas son cálculos sobre la ley comprobada, no conducción aprobada. El recorrido final cambiará cuando se modifique la barra.

La telemetría da una comprobación independiente del mecanismo: a 100.600 s, FR tiene muelle **+1699 N**, amortiguador **-8.6 N** y barra **-1837.5 N**, sin apoyo. La barra supera al muelle casi sin intervención del amortiguador. La pérdida empieza con rayos que aún ven suelo y sin tope duro de extensión.

La posición `q=0` es una referencia geométrica, no el equilibrio cargado. Que el coche se asiente en otro recorrido no es por sí solo un error de integración. Sí vuelve inadecuado interpretar el ratio como una relación constante con la rigidez del muelle que realmente está trabajando.

El propio `profile_provenance.json` clasifica el aumento 0.15→1.00 delante y 0.09→0.80 detrás como una estimación de ajuste, no como una medida histórica. La discrepancia entre valores planos y geométricos del archivo no produce dos barras: la carga del perfil y la transacción del garaje sincronizan una autoridad única, y las pruebas de reglajes lo verifican.

## Resultado trasero: no basta aplicar el mismo factor

Detrás hay otra situación: la geometría es fuertemente regresiva en la zona comprimida. La fuerza del muelle aumenta poco, aunque el recorrido siga aumentando.

| Recorrido trasero q | Relación r | Fuerza del muelle en rueda | Pendiente local |
| --- | ---: | ---: | ---: |
| -50 mm | 0.550 | 1476 N | 28575 N/m |
| 0 mm | 0.371 | 2179 N | 5468 N/m |
| +15 mm | 0.338 | 2242 N | 2976 N/m |
| +25 mm | 0.319 | 2265 N | 1734 N/m |

Con ratio 0.80, la barra trasera tiene K_bar≈15682 N/m. Su cuota elástica local, excluyendo topes, pasa de aproximadamente **52 %** en q=-50 mm a **85 %** en q=0 y **91 %** cerca de q=+15 mm. Con ratio 0.24 todavía ronda **76 %** en q=+15 mm.

Esto no debe interpretarse aislando el coche de sus topes: en la sesión, a velocidades superiores a 5 m/s, RL tiene fuerza progresiva de tope superior a 1 N en un **63.0 %** de sus muestras y RR en un **59.0 %**; el percentil 95 de esa fuerza es aproximadamente 2434/2356 N. Los percentiles 50 de recorrido son +15.9/+14.6 mm. Ejemplo a 140 s y 83.844 m/s: RL transmite unos 2267 N de muelle y 2061 N de tope progresivo.

Un tope progresivo puede formar parte intencionada de la plataforma de un monoplaza con carga aerodinámica. Su intervención frecuente no demuestra por sí sola un defecto. Pero obliga a calibrar **muelle, geometría, precarga y tope como conjunto**, en lugar de declarar que el muelle principal está haciendo todo el trabajo. El flag de tope duro puede seguir en cero aunque el tope progresivo ya ejerza fuerza.

La tendencia regresiva aparece también al evaluar la geometría completa. Por tanto no nace de un gran error de la tabla simplificada: es una propiedad de la geometría y los parámetros cargados. Falta evidencia histórica o de ensayo que valide esta curva como representación del FW19.

## Qué está bien y qué está pendiente en Rust

| Hallazgo | Clasificación | Evidencia y alcance |
| --- | --- | --- |
| Barra nula con recorridos simétricos; fuerzas opuestas con diferencia de recorrido | Comportamiento correcto en los bancos | Las 25 combinaciones del banco cumplen ambas propiedades |
| Barra lineal de K fijo, con energía 0.5*K*delta² | Representación coherente pero aproximada | No reconstruye la geometría real de una barra de torsión; el ratio requiere calibración |
| Muelle completo frente al simplificado | Aproximación pequeña en el rango comprobado | Error máximo de fuerza para fuerzas >200 N: 0.095 % delante y 0.719 % detrás; no explica el desequilibrio de fuerzas observado |
| Interpolación independiente de s y r | Inconsistencia cinemática confirmada | En general r_tabla no coincide exactamente con d(s_tabla)/dq; fuerza y gradiente de la energía del elemento difieren hasta 1.85 % delante y 2.25 % detrás en el barrido con F>200 N |
| Campo de pendiente publicado | Diagnóstico aproximado | Diferencias de hasta varios puntos porcentuales frente a dF/dq; dr/dq se interpola y se limita. No usarlo como única medición para calibrar |
| Alcance de rayos | Limitación confirmada, contribución dinámica pendiente | Longitud spring_length+radio; hay misses tras la descarga. No demuestra ausencia de pista ni inicia el episodio principal con rayos válidos |

Para aislar la inconsistencia de interpolación se calculó también el gradiente de la energía con la geometría completa: coincide con su fuerza con error absoluto inferior a 0.000001 N en los puntos del banco. El desajuste de la tabla no demuestra creación neta de energía durante una vuelta ni causalidad del levantamiento: un informe de energía del vehículo completo requeriría otro ensayo. Sí justifica que una futura interpolación obtenga desplazamiento y derivadas de una representación compatible.

Las 37 pruebas existentes ejecutadas pasan: 6 de acoplamiento del cuerpo, 15 de autoridad de reglaje y 16 de contacto estructural. Cubren, entre otros, reacción de las esquinas sin apoyo, balance de impulso/momento, simetría izquierda/derecha y aplicación de controles. Estos resultados tampoco prueban que toda la simulación sea correcta ni sustituyen una reproducción instrumentada de la frenada.

## Revisión del rumbo de ajuste

1. **Delante**, evaluar en banco una zona alrededor de **0.12–0.15**, escogida por la rigidez local medida, como candidato a que el muelle aporte la mayor parte de rigidez de balanceo en la rueda descargada. El 0.30 anterior queda como comparación, no como recomendación de equilibrio ya conseguida.
2. **Detrás**, medir la curva completa de apoyo del conjunto muelle–tope a las cargas aerodinámicas de uso, y fijar la plataforma deseada antes de dar por bueno un ratio. Reducir a 0.24 sin revisar la curva regresiva no garantiza independencia suficiente. No trasladar automáticamente el factor delantero.
3. Elegir K_bar físico o K_roll objetivo y documentar las coordenadas. El esquema ya admite una barra de tasa explícita y relación de movimiento, pero exponerla en el garaje y conservar compatibilidad con reglajes actuales sería un trabajo de implementación separado. Una barra física debe conservar su K elegido cuando se cambia el muelle; no adaptarse a cada instante para mantener un porcentaje artificial.
4. Corregir de forma acotada la compatibilidad entre s(q), su derivada y la fuerza, y distinguir rigidez medida de diagnóstico aproximado. Verificar simetría, energía/trabajo y convergencia antes/después con los mismos parámetros. No aumentar muelles, amortiguación o grip para tapar esa inconsistencia.
5. Después afinar amortiguación y reparto entre ejes mediante maniobras repetibles. Registrar dirección con signo, pedales, fuerzas, recorrido, apoyo y activación progresiva/dura de topes. Comparar curva estable y transitorios; la proporción entre barras no equivale al reparto total de transferencia lateral.

**Conclusión de causalidad:** configuración y calibración del modelo constituyen la explicación dominante y demostrada a nivel de leyes de fuerza. Hay defectos secundarios de representación en la simulación. Su participación en el episodio real no está cuantificada; no se debe afirmar que ambos tienen el mismo peso ni que un cambio de dos ratios resuelve toda la plataforma. La comprobación pendiente es una reproducción cerrada con entradas equivalentes y un candidato calibrado.

## Trazabilidad y reproducción

- [Programa Rust del banco](../../implementation/reports/spring_bar_second_opinion_2026_10_23/probe.rs).
- [Curvas por rueda](../../implementation/reports/spring_bar_second_opinion_2026_10_23/spring_curve.csv) y [rigideces modales](../../implementation/reports/spring_bar_second_opinion_2026_10_23/modal_stiffness.csv).
- [Métricas y hashes](../../implementation/reports/spring_bar_second_opinion_2026_10_23/metrics.json) y [script de análisis](../../implementation/reports/spring_bar_second_opinion_2026_10_23/analyze.py).
- [Gráfico comparativo](../../implementation/reports/spring_bar_second_opinion_2026_10_23/spring_bar_comparison.png).
- [Diagnóstico de la sesión](../../implementation/reports/telemetry_2026_10_23/latest_test_102453/analysis.md).

Los registros completos de la sesión y los renders de páginas se conservan localmente y no se publican. `analyze.py` documenta el análisis original y necesita esos registros para regenerar sus métricas; los CSV constitutivos, las métricas y el gráfico sí acompañan este informe. Los hashes corresponden a la base anterior al reglaje 35/40, no al candidato posterior.

Puntos del código revisados: `simplified_axle_integration.rs:126` (fuerzas de esquina), `:473` (K de barra), `:766` (fuerzas opuestas); `suspension.rs:1455` (referencia k_s*r_diseño²), `:1627` (ley de barra), `:1314` (derivada publicada); `simplified_suspension.rs:64` (interpolación); `suspension_setup.rs:264` y `:297` (autoridad carga/garaje); `open_formula.rs:650` (alcance de rayos). Rutas relativas a `rust/vehicle-physics-engine/src/`.

Desde la raíz del repositorio:

```powershell
cargo build --manifest-path rust/Cargo.toml --offline --release -p vehicle_physics_engine --lib
rustc --edition=2021 implementation/reports/spring_bar_second_opinion_2026_10_23/probe.rs --extern vehicle_physics_engine=rust/target/release/deps/libvehicle_physics_engine.rlib -L dependency=rust/target/release/deps -O -o implementation/reports/spring_bar_second_opinion_2026_10_23/probe.exe
./implementation/reports/spring_bar_second_opinion_2026_10_23/probe.exe implementation/reports/spring_bar_second_opinion_2026_10_23
python implementation/reports/spring_bar_second_opinion_2026_10_23/analyze.py
cargo test --manifest-path rust/Cargo.toml --offline --release -p vehicle_physics_engine --test setup_authority --test structural_contact --test body_coupling
```

El script de análisis usa matplotlib y la telemetría preservada indicada. El programa Rust conserva el comportamiento del solver: evalúa la ruta pública, sin copiar un integrador alternativo ni modificar la biblioteca.

Se crearon documentos y bancos de diagnóstico. No se modificaron fuentes de producción, reglajes, perfiles instalados ni DLL de la distribución; no hubo conducción, commit ni push durante esta revisión.
