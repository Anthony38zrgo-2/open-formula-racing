# Suspensión C1 — informe de implementación (2026-10-25)

Rama `main`, HEAD de partida `b315dce34a16c3b874b0d38ce21c2ecc2776f40a`, sin commit ni push.
Handoff de referencia: `instrucciones.md` (2026-10-25).

Estado: **fases 1–5 implementadas y validadas; fase 6 (filtros de 1 ms) no implementada, declarada
en 0.0**. Estática idéntica a la ley histórica, regresiones y `verify_dll.py` en verde.

---

## 1. Archivos modificados y nuevos

### Nuevos

| Archivo | Contenido |
|---|---|
| `rust/vehicle-physics-engine/src/vehicle_configuration/suspension_filters.rs` | Dominio numérico `suspension_filters` (limitador y filtros ≤ 1 ms) |
| `rust/data/vehicles/williams-fw19-hockenheim/suspension_filters.json` | Valores del perfil |
| `rust/vehicle-physics-engine/tests/suspension_c1.rs` | 11 bancos de aceptación del cambio C1 |
| `rust/vehicle-physics-engine/tests/contact_ray_count.rs` | 5 bancos del parámetro de raycast (1 vs 3 por rueda) |
| `implementation/reports/suspension_c1_2026_10_25/` | Este informe, logs, validaciones, `install.py`, `check_rays.py` |

### Modificados

| Archivo | Cambio |
|---|---|
| `rust/vehicle-physics-engine/src/suspension.rs` | Codo C1 del amortiguador (`geometric_damper_element`, `damper_blended_magnitude`); ley C1 del tope (`bump_stop_force`), amortiguador de tope (`bump_stop_damper_force`), tope duro compliant y `hard_stop_allowance_m`; amortiguador de barra en el camino no acoplado; captura del objetivo de contacto previo; tests unitarios `c1_regularisation_tests` |
| `rust/vehicle-physics-engine/src/simplified_axle_integration.rs` | Amortiguador de barra en el solve implícito del eje (par de velocidades candidatas); banda de compliance en `compression_bounds_metres`; `stop_force` recibe la velocidad |
| `rust/vehicle-physics-engine/src/simplified_contact_kinematics.rs` | **Ruta única** del objetivo de contacto: limitador de velocidad + filtro opcional + clamp de velocidad normal |
| `rust/vehicle-physics-engine/src/suspension_geo_config.rs` | 7 campos físicos nuevos por eje, `BumpStopLaw`, validación y serialización condicional |
| `rust/vehicle-physics-engine/src/vehicle_configuration/{mod,manifest,loader,vehicle_configuration,legacy_profile,canonical_profiles}.rs` | Registro del dominio `suspension_filters` (opcional) en el manifest, el loader y el adaptador legacy |
| `rust/vehicle-physics-engine/build.rs` | El documento embebido del perfil incluye `suspension_filters` |
| `rust/vehicle-physics-engine/tests/suspension_35_40.rs` | Entrada C1: la fuerza a 0.25 mm del inicio del tope ya no es 152 N sino ~0.4 N; la aserción mide «positiva y pequeña» en lugar de «> 1 N» |
| `rust/data/vehicles/williams-fw19-hockenheim/{suspension.json,vehicle.json,AGENTS.md}` | Coeficientes físicos, dominio nuevo y documentación |
| `distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/` | Copia local de los mismos archivos (`distribution/` está ignorado por Git) |
| `rust/vehicle-physics-engine/src/open_formula.rs` | Muestreador de rayos: `contact_ray_count` (1 o 3 por rueda), contadores reales de consultas y `ray_count` en el contexto |
| `rust/vehicle-physics-engine/src/types.rs` | `WheelRayContext.ray_count` (0 = no declarado = tricast) + `effective_ray_count()` y `with_ray_count()` |
| `rust/vehicle-physics-engine/src/vehicle_configuration/suspension/{configuration,document,validation}.rs` | Campo `contact_ray_count`, valor por defecto 3 y validación 1/3 |
| `rust/vehicle-physics-engine/src/suspension.rs` (además de lo anterior) | El bitmask de telemetría declara un solo rayo cuando solo se lanzó el central |
| `rust/vehicle-physics-engine/src/ffi.rs` | El ABI retirado colapsa el tricast del host al rayo central si el perfil pide 1 |

No se ha tocado: modelo de neumático, `tire_filters.json`, relajación lateral, acoplamiento
cuerpo/esquina, solver 6×6, aerodinámica, masas, inercias, frenos, potencia, ABI 7, `OfpSnapshot`,
firma de `SuspensionSystem::step` ni el contrato de `TriRaycastSample`.

---

## 2. Leyes implementadas y parámetros por defecto

### 2.1 Mezcla C1 del codo del amortiguador (fase 1)

`damper_knee_blend_m_per_s` = ancho total de banda `b`; la mezcla ocupa `knee ± b/2` y se aplica a la
**rigidez**, integrada para obtener la fuerza:

```text
c(u) = c_slow - (c_slow - c_fast) * smoothstep(u),  u = (v - (knee - b/2)) / b
F(v) = c_slow*(knee - b/2) + b * (c_slow*u - (c_slow - c_fast)*(u^3 - u^4/2))
```

Propiedades verificadas (`c1_regularisation_tests`):

* `F` y `dF/dv` continuas en `knee ± b/2`; la pendiente pasa de `c_slow` a `c_fast` sin salto.
* `F` coincide **exactamente** con la ley histórica fuera de la banda, incluida la asíntota
  `c*knee + (v-knee)*c*fast`.
* `F(v)*v ≥ 0` y monótona (disipativa).
* `F(knee)` se mueve `-0.09375*(c_slow - c_fast)*b`: **-5.7 N delante (0.55 %)** y **-44.5 N detrás
  (0.74 %)**. Es el precio de C1 con asíntota exacta: conservar `F(knee)` obliga a que el área bajo
  `knee` siga siendo `c*x`, es decir, a no mezclar. Se documenta y se acota (< 1 %).

| eje | `c_slow` (N·s/m) | `c_fast` (N·s/m) | `knee` (m/s) | `blend` (m/s) | salto de pendiente eliminado (N·s/m) |
|---|---:|---:|---:|---:|---:|
| delantero | 8074.8 | 4037.4 | 0.127 | 0.015 | 4037.4 |
| trasero | 47431.8 | 23715.9 | 0.127 | 0.020 | 23715.9 |

### 2.2 Tope progresivo C1 + amortiguador dedicado (fase 2, P0)

`bump_stop_law = "cubic"`:

```text
F(x) = k3 * x^3,   k3 = F_end / w^3,   F_end = 4 * k_rest * bump_stop_mult * w
```

`w` = `bump_stop_travel_m` (5 mm), `k_rest` = `spring_rate * r^2`. `F(x)` y `dF/dx` son continuas con
`F(0) = dF/dx(0) = 0`, monótona y conservativa (`∫F dx = k3*x^4/4 ≥ 0`). `F_end` es la fuerza
histórica a pleno recorrido, así que `bump_stop_mult` conserva su significado y el pico no se mueve.
Alternativa `"smoothstep"` implementada (`F = F_end*(2u^3 - u^4)`); `"legacy"` reproduce la ley
anterior.

Medido en el perfil (`geometric_stop_reaches_the_documented_full_travel_force...`):

| eje | `k_rest` (N/m) | `F_end` a 5 mm (N) | `k3` (N/m³) |
|---|---:|---:|---:|
| delantero | 153 448 | 3 069 | 2.455e10 |
| trasero | 164 722 | 3 294 | 2.635e10 |

Amortiguador dedicado `bump_stop_damper_Ns_per_m` (en rueda), **solo compresión**, con rampa
smoothstep sobre el primer 10 % del recorrido de tope:

```text
F_d = c_bs * max(v, 0) * smoothstep(excess / (0.1*w))
```

`F_d ≥ 0` para `v ≥ 0` y `F_d = 0` en extensión ⇒ `F_d*v ≥ 0` en la convención del solver (solo
disipa). Entra en `chassis_suspension_force`, en `suspension_impulse` y en el impulso al cuerpo una
sola vez, porque forma parte de la fuerza del elemento que integra el solver.

| eje | `bump_stop_damper_Ns_per_m` |
|---|---:|
| delantero | 500 |
| trasero | 550 |

### 2.3 Ruta única y limitador de la altura de carretera (fase 3, P1)

`configure_simplified_contact_kinematics` es el **único** escritor autoritativo del objetivo en el
camino de producción. `sample_contact` solo siembra una estimación para la entrada de compatibilidad
sin cuerpo (`SuspensionSystem::step`). En la ruta única:

```text
target = previous + (raw - previous)                     (si el filtro está a 0)
target = previous + (raw - previous)*(1 - exp(-dt/tau))  (si tau > 0)
target = previous + clamp(target - previous, ±max_rate*dt)
velocity = clamp(raw_velocity, ±max_rate)
```

`Previous` es el valor que entró en el solver en el paso anterior (capturado antes de `sample_contact`);
la primera muestra salta directa a la geometría (`normal_road_target_initialized`).

| campo | valor | unidad |
|---|---:|---|
| `road_height_max_rate_m_s` | 15.0 | m/s |
| `road_height_filter_tau_s` | 0.0 | s (2–4 ms recomendado si se activa) |

`max_road_velocity_m_s = 15` (tuning) **no se ha cambiado**; sigue acotando la señal del camino
legacy. No se filtra la fuerza normal, ni la reacción, ni el impulso.

### 2.4 Tope duro compliant (fase 4, P1/P2)

Dentro de `hard_stop_compliance_m` (`c`) la penetración `p = x - x_max` produce un muelle C1
(secante `k` en el borde) más un amortiguador unilateral:

```text
F = k * p^2 / c                      (F(0)=0, dF/dx(0)=0, F(c)=k*c)
F_d = c_hs * max(v,0) * (p / c)
```

El clamp inelástico deja de ser la primera respuesta: el integrador recibe
`bounds = envelope ± max(hard_stop_overtravel_m, compliance)` y solo en `x ≥ x_max + c` se suma la
ley histórica de fuga y se marca `clamped`. La banda de compliance hace que `p > 0.1 mm` (el
overtravel numérico histórico) ya no implique proyección inelástica.

| eje | `hard_stop_compliance_m` (m) | `hard_stop_stiffness_N_per_m` (N/m) | `hard_stop_damper_Ns_per_m` (N·s/m) |
|---|---:|---:|---:|
| delantero | 0.0004 | 16 500 000 | 1500 |
| trasero | 0.0004 | 16 500 000 | 1500 |

`0.0` en `hard_stop_compliance_m` conserva exactamente el comportamiento anterior. La rueda puede
penetrar como máximo `c + hard_stop_overtravel_m` (0.5 mm) antes de que el clamp la retenga.

### 2.5 Amortiguador de barra (fase 5, P2)

```text
F_self = c_arb * (v_self - v_opp),   F_opp = -F_self
```

Antisimétrico ⇒ el par neto del eje es nulo y la estática no cambia. Dentro del solve implícito del
eje usa las **dos velocidades candidatas**; en el camino no acoplado usa la velocidad del otro lado al
inicio del tick. `F_self*v_self + F_opp*v_opp = c_arb*(v_self - v_opp)^2 ≥ 0`.

| eje | `arb_damper_Ns_per_m` (rueda) |
|---|---:|
| delantero | 200 |
| trasero | 150 |

### 2.6 Filtros de 1 ms (fase 6, P2/P3) — no implementados

Dominio preparado con `damper_velocity_filter_tau_s` y `arb_travel_filter_tau_s` **por eje** y
validación que rechaza `tau > 1 ms` (`MAX_FORCE_FILTER_TAU_S`), pero **ambos valen 0.0**: no se ha
encontrado ruido de alta frecuencia que los justifique (ver §6). No se filtran
`chassis_suspension_force`, `body_acceleration_world`, `angular_acceleration`, la fuerza del muelle ni
el impulso del tope.

### 2.7 Raycast por rueda: 1 o 3 (iteración 2026-10-25b)

`suspension.contact_ray_count` (entero, `1` o `3`; por defecto `3`) elige cuántos rayos transversales
lanza el muestreador interno por rueda. **El perfil está en `1` en esta iteración.**

| valor | rayos por rueda | consultas por paso (4 ruedas) | qué puede ver |
|---|---:|---:|---|
| `1` | 1 (solo el central) | 4 | un plano de contacto uniforme bajo el centro de la rueda |
| `3` | 3 (inner/centro/outer) | 12 | contacto parcial (bordillos, piano) y peralte por diferencia inner/outer |

Implementación:

* El único productor de rayos es `OpenFormulaSimulation::sample_wheel_rays`. Con `1` lanza solo el rayo
  central y **repite ese impacto** en los huecos inner/outer del `TriRaycastSample`: el modelo de
  contacto (pesos 1:2:1, fracción de contacto, plano ponderado) ve entonces un plano único y
  `contact_fraction = 1.0`. No se lanzan dos consultas al mundo que no se usan.
* Los contadores publicados (`wheel_ray_queries`, `wheel_ray_hits`) cuentan **consultas reales**: 4 por
  paso en modo simple, 12 en tricast.
* Telemetría: `ray_grounded` informa `[false, true, false]` en modo simple; los otros dos huecos no son
  evidencia de suelo. El bitmask del CSV y de `OfpWheelDiagnostics` no miente.
* El ABI retirado `f1_94_physics_step` recibe los rayos del host; si el perfil pide `1`, el motor
  colapsa el tricast al rayo central para que el mismo perfil dé la misma física en todos los hosts.
* Validación: cualquier valor distinto de `1` o `3` se rechaza al cargar el perfil
  (`suspension.contact_ray_count must be 1 or 3`). Ausente ⇒ `3`, así que los perfiles viejos cargan
  con el comportamiento histórico.
* No es un knob de reglaje: no se guarda en `open_formula.db` ni entra en `setup_fields_text`.

Lo que se pierde con `1`, declarado: no hay contacto parcial (una rueda con medio neumático en un
piano se resuelve como si todo el neumático estuviera en la superficie del rayo central) ni peralte
medido en la huella (`dynamic_camber` recibe inclinación 0 y queda en la cinemática). Medido en plano:
la altura de reposo y las cargas son **idénticas** al tricast (diferencia < 1e-4 N).

---

## 3. Compilación y tests

```powershell
cd D:/open-formula/rust
cargo build --release -p vehicle_physics_engine
cargo test  --release -p vehicle_physics_engine --tests --no-fail-fast
cargo test  --release -p vehicle_physics_engine --lib c1_regularisation -- --nocapture
python ../implementation/reports/suspension_c1_2026_10_25/verify_dll.py `
  target/release/vehicle_physics_engine.dll `
  ../data/vehicles/williams-fw19-hockenheim/vehicle.json after_dll_validation.json
```

### Integración (release, `--tests --no-fail-fast`)

| suite | resultado |
|---|---|
| `body_coupling` | 6/6 |
| `setup_authority` | 15/15 |
| `structural_contact` | 16/16 |
| `suspension_35_40` | 5/5 |
| `vertical_telemetry` | 9/9 |
| `wheel_centre_contact` | 16/16 |
| `suspension_c1` (nuevo) | 11/11 |
| `contact_ray_count` (nuevo) | 5/5 |

### Unitarias

* `c1_regularisation_tests` (7): continuidad/energía del codo, monotonía, amortiguador unilateral,
  ley C1 del tope, tope a pleno recorrido, roundtrip de perfil y rechazo de valores inválidos.
* `suspension_filters::tests` (3): límite de 1 ms, banda 2–4 ms del filtro de entrada, dominio
  ausente = todo desactivado.
* Librería completa: **176/195** (175/195 cuando el writer asíncrono pierde la carrera; es
  `bounded_writer_never_waits_for_io_and_owns_immutable_snapshot`, sensible al tiempo). Las **19–20
  fallas son el mismo conjunto que HEAD** (verificado con `git stash` + `cargo test --release --lib`:
  165–166/185 en `b315dce`); son ambientales o de contrato previo (`data/vehicles/f1_2030/...`
  inexistente en esa ruta, aserciones de cadena ABI v6, fixtures de neumático, writer asíncrono). No
  hay fallas nuevas: la diferencia de 10 pruebas es exactamente `c1_regularisation_tests` (7) +
  `suspension_filters::tests` (3).

### `verify_dll.py` (ABI real)

`passed: true`, ABI 7, firma `0xa91cdeba9d3fe775`, 2000 pasos aceptados, 0 rechazos, 0 registros de
traza sin opt-in, `floor_impulse_ns = [0, 0]`, `stop_forces_n = [0, 0, 0, 0]`.

---

## 4. Comparación antes / después

| Métrica | Antes (`b315dce`) | Después | Diferencia |
|---|---:|---:|---|
| Ride delantero / trasero (m) | 0.03501418 / 0.03999095 | 0.03501418 / 0.03999095 | < 1e-11 |
| Carga FL / FR (N) | 1335.84110 / 1335.84113 | 1335.84108 / 1335.84113 | < 1e-4 |
| Carga RL / RR (N) | 1667.90852 / 1667.90855 | 1667.90852 / 1667.90857 | < 1e-4 |
| Fuerza de tope en reposo (N) | 0 | 0 | — |
| Impulso de suelo en reposo (N·s) | [0, 0] | [0, 0] | — |
| Fuerza de tope a pleno recorrido, del. / tras. (N) | 3069 / 3294 | 3069 / 3294 | 0 (ley C1 llega al mismo `F_end`) |
| Pendiente de entrada del tope (N/m) | 153 448 | 0 | C1 |
| Salto de pendiente en el codo (N·s/m) | 4037 / 23 716 | 0 | C1 |
| `F(knee)` del. / tras. (N) | 1025.5 / 6023.8 | 1019.8 / 5979.3 | −0.55 % / −0.74 % |
| Profundidad de la banda de compliance (m) | 0.0001 (overtravel) | 0.0005 (0.4 + 0.1) | +0.4 mm |
| Amortiguador de barra (N·s/m) | 0 | 200 / 150 | nuevo |
| Objetivo de contacto: cambio máximo por paso | sin límite | 31.2 mm @ 480 Hz | limitado |
| Rayos de contacto por rueda | 3 (12 por vehículo) | 1 (4 por vehículo) | consultas ×1/3 |

Medidas de los bancos nuevos (`c1_acceptance_measurements.log`):

* **Estática**: C1 y ley histórica dan la misma pose (`-0.015627330 m` ambas) y las mismas cargas
  (diferencia < 1e-3 N); carga total 6007.5 N vs peso 6007.6 N; ningún tope ni barra soporta al coche.
* **Limitador**: con el dominio, la caída de 60 mm bajo una rueda mueve el objetivo como máximo
  `0.0312 m` por paso (= 15 m/s · 1/480 s); sin el dominio, `0.0600 m` en un solo paso.
* **Banda de compliance**: empujando 150 µm más allá del tope, la rueda penetra `0.12016 m`
  (+160 µm) sin clamp (impulso 0) frente a `0.12010 m` (+100 µm, tope duro) de la ley histórica; la
  fuerza de tope es 6816 N frente a 6396 N (+6.6 %) para un 60 % más de recorrido útil.
* **Sobre-demanda de 1.2 mm**: la rueda se detiene en `+400 µm` (borde de banda), el estado sigue
  factible (`mechanical_failure = None`) y al liberar vuelve a `0.095000 m` (su recorrido estático).
* **Barra**: integral de |velocidad de balanceo| `0.01320 → 0.01310 rad` (−0.8 %), pico
  `0.5882 → 0.5987 rad/s` (+1.8 %), reparto estático idéntico (`908.3 N`), sin efecto en recta.
* **Impacto**: precompresión de 4 mm sobre el tope y liberación en un paso → la desviación de cola es
  `0.000000 m` (sin oscilación residual).
* **Frecuencias**: 500 / 240 / 120 Hz dan la misma pose y el mismo reparto delantero (diferencia
  < 1e-3); la carga total queda a 0.2 N del peso en los tres casos.
* **Bordillo de 15 mm bajo las cuatro ruedas** (caso duro): pico de **carga de contacto idéntico**
  (85 036 N en ambos), ningún levantamiento de rueda (0 en ambos) y vuelta exacta a la pose previa
  (`-0.0156 m`). El pico de *fuerza de tope* sube de 2550 N a 4935 N: es la consecuencia buscada de
  permitir 4× más penetración antes del clamp y de añadir el amortiguador dedicado (hasta
  `c_bs·v`); la fuerza que llega al neumático no cambia. En el camino cuasiestático (sin impacto) el
  mismo escenario da 6816 N frente a 6396 N (+6.6 %).

---

## 5. Hashes

| Artefacto | SHA-256 |
|---|---|
| `rust/target/release/vehicle_physics_engine.dll` | `289783d4cea4615caec963c8e44adb15975a184e2e4ed1311aca0cccd43cc8f9` |
| `suspension.json` | `f8762fe0653a97817018459318782360090f9dc0ebe34f60dc8f5469dc7fad81` |
| `suspension_filters.json` | `1320f2a51be956e83564658c9fe98338fc623e87ea1c707ae1292e7f06581c75` |
| `vehicle.json` | `7314bad388a973e375994e83161f8956fb4fbc7c4f6e96740aa815ace6c4cc85` |

DLL de partida (`b315dce`): `9722a6a92f0237f91909eb63c4758f7ff83bcf862f04ddacf095350a6091094a`.

### 5.1 Instalación local para probar en el juego

`install.py` copia el binario y el perfil validados a la distribución local, con copia de seguridad y
`rollback.ps1`. Verificado después de instalar con el **perfil instalado**
(`installed_dll_validation.json`): ABI 7, 2000 pasos aceptados, ride 0.03501418 / 0.03999095 m y cargas
1335.841 / 1667.909 N. `check_rays.py` confirma además sobre el binario desplegado que el modo de un
rayo está activo: `wheel_ray_queries = 4`, `wheel_ray_hits = 4` por paso.

| Elemento | Ruta |
|---|---|
| Lanzador | `distribution/OpenFormula.exe` |
| DLL instalada | `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` (`289783d4…`) |
| Perfil | `distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/` |
| Copia de seguridad + rollback | `C:\Users\bill\open-formula-backups\2026-10-25-install-c1-917274d0\rollback.ps1` |

Para volver a la build anterior: cerrar el juego y ejecutar ese `rollback.ps1`. Para volver solo al
tricast sin cambiar de binario, poner `"contact_ray_count": 3` en `suspension.json` (se lee al crear la
sesión).

Aviso sobre reglajes guardados: `user-settings/open_formula.db` guarda `base_profile_sha256` y
`formula_setup_store.cpp` rechaza un reglaje cuya huella del perfil base no coincida. El reglaje
guardado («Reglaje 2», huella `90e33cb9…`) corresponde al perfil anterior, así que el juego puede
ignorarlo. No afecta a la prueba: los campos C1 **no** son knobs de reglaje (viven en `suspension.json`),
y los 33 valores del reglaje guardado siguen siendo válidos. Si se quiere conservar, volver a guardarlo
desde el menú tras cargar la sesión.

---

## 6. Fase 6 — resultado negativo declarado

No se han añadido filtros. Justificación:

1. Las dos discontinuidades de pendiente conocidas (entrada del tope y codo del amortiguador) están
   eliminadas por construcción; el banco de impacto no muestra oscilación residual de cola
   (`0.000000 m` en 960 pasos) ni fuerza de tope creciente.
2. Los filtros disponibles están capados a 1 ms por validación. A 20 Hz (rebote de rueda) 1 ms son
   7.2° de fase; 2 ms son 14.4° y ya produjeron un ciclo límite en el neumático (lección de `b315dce`).
3. **Protocolo de pulso de dirección a 74 m/s**: se ha añadido un protocolo equivalente en el árbol
   (`high_speed_steering_pulse_does_not_grow`): 74 m/s en recta, 0.15 s a la izquierda, 0.15 s a la
   derecha y 4 s a manos libres. Resultado: el envelope de guiñada **decrece** de 0.2268 rad/s (0–2 s)
   a 0.0001 rad/s (2–4 s) y el pico de balanceo (0.042 rad/s) no crece. No aparece ciclo límite. No es
   el pulso original con el que se validó el neumático (aquel se ejecutó fuera del árbol), pero cubre
   la misma puerta para el cambio de suspensión.
4. El limitador de la altura de carretera no es un filtro: no añade retardo en régimen estacionario.

---

## 7. Vuelta atrás

Todo el cambio es aditivo y opcional, sin commit:

1. **Por datos (sin recompilar el modelo de leyes)**: en `suspension.json` borrar
   `damper_knee_blend_m_per_s`, `bump_stop_law`, `bump_stop_damper_Ns_per_m`,
   `hard_stop_compliance_m`, `hard_stop_stiffness_N_per_m`, `hard_stop_damper_Ns_per_m` y
   `arb_damper_Ns_per_m`; y quitar `suspension_filters` de `vehicle.json` (el dominio es opcional y su
   ausencia restaura el comportamiento histórico). Los valores por defecto de todos los campos nuevos
   son `0` / `legacy`.
2. **Por código**: `git checkout -- rust/vehicle-physics-engine rust/data/vehicles` y borrar
   `suspension_filters.rs`, `suspension_filters.json` y `tests/suspension_c1.rs`.
3. **Perfil instalado**: restaurar
   `distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/{suspension.json,vehicle.json}`
   desde Git y borrar `suspension_filters.json` de esa carpeta.

---

## 8. Pendientes y límites declarados

1. **Substepping del eje**: `solve_simplified_geometric_axle` sigue con `substeps = 1` e ignora
   `mechanical_substep_s`; no se ha implementado interpolación del cuerpo. La validación es a 500 Hz
   de host (2 ms); 240 y 120 Hz se validan porque `VehicleSimulator` subdivide internamente
   (`internal_substeps`) y el estado de reposo coincide. **No se usan filtros para compensar el paso.**
2. **Pulso a 74 m/s**: cubierto con un protocolo equivalente en el árbol (§6), no con el original del
   neumático, que se ejecutó fuera del árbol.
3. **Camino legacy 1-DOF** (`Legacy1Dof`): conserva el codo duro y la ley histórica del tope; no es el
   camino de producción (`Simplified` + `Geometric`).
4. **`F(knee)` del amortiguador**: se mueve < 1 % por la mezcla C1 (ver §2.1); es una consecuencia
   matemática de exigir C1 y asíntota exactas, no un descuido de calibración.
5. **Energía del amortiguador de tope**: `damper_dissipated_j` sigue registrando solo el amortiguador
   principal; la disipación del tope se valida por construcción (`F_d*v ≥ 0`) y en el banco de impacto,
   no se acumula en esa métrica.
6. **19–20 fallas unitarias preexistentes** fuera de las puertas del handoff (idénticas a HEAD).
7. **Modo de un rayo**: pierde contacto parcial y peralte por huella por construcción (§2.7). En
   plano las cargas y la altura de reposo son idénticas al tricast; en piano fuerte o en una rueda
   medio fuera de la pista la fuerza se resuelve con el plano del rayo central. Es el intercambio
   declarado del modo barato, reversible con `contact_ray_count`.
8. Sin commit ni push, por instrucción explícita.

---

## 9. Confirmación de no objetivos

* Modelo de neumático, `tire_filters.json`, relajación lateral: intactos.
* Acoplamiento cuerpo/esquina, solver 6×6 y dueño de la integración: intactos.
* Aerodinámica, masas, inercias, frenos, potencia: intactos.
* ABI 7, firma `ofp_abi_signature`, `OfpSnapshot`, FFI: intactos (verificado con `verify_dll.py`).
* Muelles, barras (rigidez) y alturas de reposo: sin cambios; no se ha reajustado nada para ocultar
  problemas de integración.
