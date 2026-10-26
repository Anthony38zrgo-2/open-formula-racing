# GOAL — Handoff de suspensión: regularización y filtros por componente

**Fecha:** 25 de octubre de 2026.
**Repositorio exclusivo:** `D:/open-formula`.
**Rama:** `main`.
**HEAD de partida:** `b315dce34a16c3b874b0d38ce21c2ecc2776f40a`.
**Sustituye:** el handoff anterior de `instrucciones.md` (2026-10-21, neumático rígido). El anterior queda en el historial de Git; no se pierde.

Este documento es un handoff para el siguiente agente. El objetivo es hacer la suspensión **más predecible y menos sensible o crítica**, aplicando regularización C1, amortiguación física y, solo si hace falta, filtros de 1 ms. No es una autorización para cambiar el modelo de neumático, el acoplamiento cuerpo/esquina ni la aerodinámica.

---

## Estado de la implementación (2026-10-25, sesión 1)

Informe completo, medidas y hashes: `implementation/reports/suspension_c1_2026_10_25/README.md`.

| Fase | Estado | Resumen |
|---|---|---|
| 1. Codo C1 del amortiguador | **Hecha** | Banda `knee ± blend/2` integrada sobre la rigidez; C1, asíntota exacta, `F(knee)` −0.55 % / −0.74 %. `blend = 0.015 / 0.02`. |
| 2. Tope C1 + amortiguador | **Hecha** | `bump_stop_law = "cubic"` (`F = k3·x³`, `F_end` = fuerza histórica a pleno recorrido) + amortiguador unilateral 500/550 N·s/m. |
| 3. Altura de carretera | **Hecha** | Ruta única en `configure_simplified_contact_kinematics`, limitador 15 m/s y filtro opcional 0.0; dominio `suspension_filters.json`. |
| 4. Tope duro compliant | **Hecha** | Banda 0.4 mm, muelle cuadrático C1 16.5 MN/m + 1.5 kN·s/m; clamp solo al final; bounds extendidos a `max(overtravel, compliance)`. |
| 5. Amortiguador de barra | **Hecha** | `arb_damper_Ns_per_m` 200/150, antisimétrico, dentro del solve implícito del eje. |
| 6. Filtros de 1 ms | **Diferida, en 0.0** | Dominio preparado y capado a 1 ms; sin evidencia de ruido. Pulso a 74 m/s cubierto con un protocolo equivalente en el árbol (`suspension_c1::high_speed_steering_pulse_does_not_grow`): la guiñada decrece, sin ciclo límite. |
| Extra. Raycast 1 vs 3 por rueda | **Hecha (en 1)** | `suspension.contact_ray_count` (1 o 3, por defecto 3) en el muestreador interno; el perfil queda en 1 (4 consultas por paso en vez de 12). En plano, mismas cargas y altura; se pierde contacto parcial y peralte por huella. Bancos: `contact_ray_count` (5). |

Regresiones en verde (`body_coupling` 6, `setup_authority` 15, `structural_contact` 16, `suspension_35_40` 5, `vertical_telemetry` 9, `wheel_centre_contact` 16, nuevos `suspension_c1` 11 y `contact_ray_count` 5) y `verify_dll.py` en verde con ABI 7. Estática idéntica a la ley histórica (ride 0.03501418/0.03999095 m, cargas 1335.841/1667.909 N). Las 19–20 fallas unitarias restantes son preexistentes (verificadas contra `b315dce`). Sin commit ni push.

**Siguiente paso sugerido**: decidir si el substepping del eje (0.2) se implementa con interpolación del cuerpo o se declara fuera de alcance; y repetir el pulso original del neumático (fuera del árbol) si se vuelve a tocar `tire_filters.json`.

---

## GOAL

1. **Tope progresivo C1 + amortiguador dedicado (P0).**
   Sustituir la ley actual del tope progresivo por una fuerza C1, con pendiente inicial cero y energía conservativa, y añadir un amortiguador dedicado en compresión. Mantener el recorrido de 5 mm y la altura residual de 5 mm.

2. **Limitador de velocidad de la altura de carretera (P1).**
   Unificar el objetivo de contacto, eliminar el doble camino que hoy sobrescribe `normal_road_compression_metres`, y limitar su velocidad de cambio (10–15 m/s) antes de la restricción. Filtro paso bajo opcional de 2–4 ms, desactivado por defecto.

3. **Mezcla C1 del codo del amortiguador (P1).**
   Sustituir el codo duro de `damper_knee_m_per_s` por una mezcla C1 en una banda de 0.01–0.02 m/s, conservando fuerza y asíntotas, y garantizando disipación positiva.

4. **Tope duro compliant (P1/P2).**
   Sustituir la proyección inelástica del tope duro por un tope compliant (muelle + amortiguador) dentro de `hard_stop_compliance_m` de 0.3–0.5 mm. Mantener el clamp como último recurso y no permitir violar el envelope geométrico.

5. **Amortiguador de barra (P2).**
   Añadir amortiguación física a la barra anti-roll, proporcional a la velocidad relativa de recorrido y opuesta a ella, con 100–300 N·s/m en rueda. No cambia el equilibrio estático.

6. **Filtros de 1 ms solo si las mediciones muestran ruido (P2/P3).**
   No añadir filtros por defecto. Solo después de 1–5, y solo si hay evidencia de ruido: velocidad del amortiguador o recorrido relativo de barra con `tau ≤ 1 ms`. Nunca sobre fuerza total de suspensión, aceleración del cuerpo o fuerza del muelle.

**Criterio general:** primero C1 y amortiguación física; los filtros son el último recurso. Cualquier filtro sobre fuerza añade fase en el bucle rueda–chasis y puede crear un ciclo límite, como ya ocurrió con el neumático.

---

## 0. Contexto y estado de partida

### 0.1 Camino activo

El juego usa `rustphysics` → `ofp_step` → `OpenFormulaSimulation` → `VehicleSimulator` → `SuspensionSystem`. El backend de producción es **Simplified** con modelo **Geometric** (`SuspensionBackend::production()`), y el host entrega `dt = 0.002 s` (500 Hz).

La resolución vertical es un solve implícito por eje, delantero y después trasero, acoplado al cuerpo mediante un punto fijo 6×6. No es un solve monolítico de cuatro ruedas y cuerpo; eso es una decisión de arquitectura existente y **no es el objeto de este GOAL**.

### 0.2 Dependencia numérica: substepping

`WheelMechanicalTuning::mechanical_substep_s` vale `1/480`, pero `solve_simplified_geometric_axle` fija `substeps = 1` y descarta el objetivo (`simplified_axle_integration.rs:595-603`). A 500 Hz el paso de 2 ms es menor que 1/480 y el efecto es pequeño; a 120 Hz el paso es 8.3 ms, cuatro veces el objetivo. **Antes de calibrar filtros, decidir si se implementa substepping del eje con interpolación del cuerpo o si se documenta que la validación es solo a 500 Hz.** No usar filtros para compensar un paso de integración demasiado grande.

### 0.3 Lección del neumático

En el commit `b315dce` se activaron todos los filtros del neumático. El valor `min_relaxation_tau_s = 0.045 s` produjo un ciclo límite de guiñada a alta velocidad; se corrigió a `0.008 s` y `relaxation_recovery_scale = 1.3`. La fase de un paso bajo es `atan(2π f τ)`:

| `tau` | 1 Hz | 2 Hz | 20 Hz (rebote de rueda) |
|---:|---:|---:|---:|
| 1 ms | 0.4° | 0.7° | **7.2°** |
| 2 ms | 0.7° | 1.4° | **14.4°** |
| 5 ms | 1.8° | 3.6° | **36°** |
| 10 ms | 3.6° | 7.2° | **51.5°** |
| 45 ms | 15.8° | 29.5° | — |

Conclusión operativa: **filtros sobre fuerza ≤ 1 ms**; 2 ms ya es marginal; 5 ms es peligroso. Los filtros de entrada de carretera pueden ser 2–4 ms, pero es preferible un limitador de velocidad.

### 0.4 Datos actuales del Williams

- Muelle: 200 000 N/m delante, 1 200 000 N/m detrás; rigidez efectiva en rueda 151.9 / 144.8 N/mm; relaciones de movimiento 0.829 / 0.359.
- Amortiguador de elemento: 8 074.8 / 9 938.3 N·s/m delante; 47 431.8 / 58 377.6 N·s/m detrás; codo 0.127 m/s; factor rápido 0.5.
- Tope progresivo: `bump_stop_mult = 1.0`, `bump_stop_travel_m = 0.005`, `hard_stop_overtravel_m = 0.0001`.
- Barra: 38.36 N/mm delante, 9.88 N/mm detrás.
- Reposo: 35.014 mm delante, 39.991 mm detrás; cargas 1335.8 / 1667.9 N por rueda.
- Fuerza del tope a pleno recorrido de 5 mm: ~3.04 kN delante, ~2.90 kN detrás. Pendiente: ~152 kN/m al entrar y ~1.52 MN/m al final.
- Fuerza del amortiguador en el codo: ~1.0 kN delante, ~6.0 kN detrás. Salto de pendiente: ~4.0 / 23.7 kN·s/m.
- Tope duro: ~0.55 kN a 0.1 mm, ~10 kN a 0.5 mm, con pendiente muy alta y clamp inelástico.

### 0.5 Puntos críticos actuales

| Componente | Problema | Evidencia |
|---|---|---|
| Tope progresivo | Cambio de pendiente al entrar; sin amortiguador dedicado | `suspension.rs:1989-2056` |
| Tope duro | Proyección inelástica; pico de fuerza | `vertical_suspension_integration.rs:125-207` |
| Amortiguador | Codo duro; salto de pendiente del 50 % | `suspension.rs:1834-1856` |
| Altura de carretera | Filtro inactivo y objetivo sobrescrito | `suspension.rs:1095-1100`; `simplified_contact_kinematics.rs:93-96` |
| Barra | Sin amortiguación; acoplamiento de balanceo | `suspension.rs:1971-1987` |
| Contacto vertical | Restricción rígida; carga normal escalonada | `vertical_suspension_integration.rs:1-18` |
| Substepping | `mechanical_substep_s` ignorado | `simplified_axle_integration.rs:595-603` |

---

## 1. Tope progresivo C1 + amortiguador dedicado (P0)

### 1.1 Situación actual

`geometric_stop_force` usa:

```text
excess = x - (x_max - working_length)
force  = k_rest * mult * excess * (1 + 3 * progress²)
```

con `progress = excess / working_length` limitado a 2. En `excess = 0+` la pendiente vale `k_rest * mult` (~152 kN/m delante), y al final de 5 mm vale ~10k (~1.52 MN/m). No hay amortiguador dedicado: solo el amortiguador principal, que puede estar en su zona digresiva.

### 1.2 Cambio requerido

1. **Ley C1**: sustituir la fuerza por una función con `F(0)=0`, `dF/dx(0)=0` y `F(5 mm) ≈ 3.0 kN`, derivada de un potencial. Opciones:
   - cúbica `F = k3 * excess³` con `k3 = F_max / (5 mm)³` (fuerza C1, potencial `k3*x⁴/4`);
   - o `smoothstep` entre la tasa lineal y la tasa de pleno recorrido, con derivada nula en ambos extremos.
2. **Amortiguador dedicado** en compresión: `F_d = -c_bs * v` cuando el tope está comprometido, con `c_bs` de 300–800 N·s/m en rueda. No aplicar en extensión (o usar una asimetría explícita). La potencia `F_d * v` debe ser ≤ 0.
3. **Datos**: añadir a `suspension.json` por eje:
   - `bump_stop_law`: `"cubic"` o `"smoothstep"`;
   - `bump_stop_cubic_coefficient_N_per_m3` (si aplica);
   - `bump_stop_damper_Ns_per_m`;
   - conservar `bump_stop_travel_m`, `bump_stop_mult`, `hard_stop_overtravel_m`.
4. **Aplicación**: la fuerza del tope y el amortiguador deben entrar en `chassis_suspension_force`, en `suspension_impulse` y en el impulso al cuerpo una sola vez. No filtrar la fuerza total.

### 1.3 Criterios de aceptación

- `F(x)` y `dF/dx` continuas en el punto de entrada, con diferencia finita relativa < 1e-3.
- `F` monótona y ≥ 0; energía del muelle `∫F dx` no negativa.
- `F_d * v ≤ 0` en todo el recorrido; energía disipada ≥ 0.
- Reposo sin fuerza de tope; altura 35.014 / 39.991 mm (±0.1 mm) y cargas 1335.8 / 1667.9 N (±1 N).
- Ensayo de impacto/kerb: pico de fuerza ≤ +10 % del actual, sin aumento de levantamiento de rueda, sin oscilación después del impacto.
- `suspension_35_40`, `body_coupling`, `structural_contact` y `vertical_telemetry` siguen pasando.

### 1.4 Pruebas

- Unitarias de la ley: continuidad, monotonía, energía, amortiguador unilateral.
- Integración: caída sobre el tope, kerb de una rueda, bache simétrico, alta velocidad.
- Comparar fuerza, recorrido, velocidad y energía antes/después.

---

## 2. Limitador de velocidad de la altura de carretera (P1)

### 2.1 Situación actual

En `sample_contact` la rama Simplified copia `road_compression_m = target` sin filtro (`suspension.rs:1095-1100`). Además, `configure_simplified_contact_kinematics` recalcula `normal_road_compression_metres` desde la pose del cuerpo y el punto de contacto (`simplified_contact_kinematics.rs:93-96`), sobrescribiendo el valor anterior. `max_road_velocity_m_s = 15` existe en `WheelMechanicalTuning` pero no se aplica a la restricción.

### 2.2 Cambio requerido

1. **Una sola ruta**: después de `configure_simplified_contact_kinematics`, aplicar el limitador/filtro a `normal_road_compression_metres` y a `normal_road_velocity_metres_per_second`. No filtrar dos veces ni dejar que `sample_contact` y `configure...` escriban valores distintos.
2. **Limitador**: limitar el cambio de compresión objetivo por paso a `max_rate_m_s * dt` (10–15 m/s). Es un límite de velocidad, no un paso bajo; no añade retardo en régimen estacionario.
3. **Filtro opcional**: `road_height_filter_tau_s` de 2–4 ms, desactivado por defecto (`0.0`). Si se activa, usarlo en la misma ruta y auditar penetración.
4. **Datos**: añadir `road_height_max_rate_m_s` y `road_height_filter_tau_s` por perfil; el valor inicial recomendado es 15 m/s y 0.0. No cambiar `max_road_velocity_m_s` sin justificar.
5. **No filtrar la fuerza normal** ni la reacción; solo el objetivo de contacto.

### 2.3 Criterios de aceptación

- La tasa de cambio del objetivo nunca supera `max_rate_m_s`.
- En piano/bordillo: penetración ≤ 0.5 mm adicional respecto a la base; sin chattering (no aumento de transiciones de la máscara de contacto); sin levantamiento de rueda adicional.
- En rasante: sin salto de fuerza ni oscilación de la carrocería.
- Reposo y cargas sin cambios.
- A 500 Hz y 120 Hz; si a 120 Hz no se puede, documentar que el substepping sigue pendiente.

### 2.4 Pruebas

- Escalón de 10, 20 y 40 mm bajo una rueda.
- Piano longitudinal y transversal.
- Cambio de rasante y peralte.
- Comparar máscara de contacto, fuerza normal, aceleración vertical y energía.

---

## 3. Mezcla C1 del codo del amortiguador (P1)

### 3.1 Situación actual

`geometric_damper_element` usa un `if` duro en `v_s = knee`:

```text
if v_s > knee:  F = (v_s - knee)*c*fast + knee*c
else:           F = v_s*c
```

La fuerza es continua, pero la pendiente cae de `c` a `c*fast` de golpe. Delante `c = 8074.8`, `fast = 0.5`; detrás `c = 47431.8`, `fast = 0.5`, con ~6 kN en el codo.

### 3.2 Cambio requerido

1. **Mezcla C1** en una banda `knee ± blend/2` (o `knee` a `knee+blend`) con `smoothstep` o `hermite`: conservar `F(knee)` y las asíntotas `c*v` y `c*fast*v + offset`.
2. **Datos**: `damper_knee_blend_m_s` por eje; valor inicial 0.015 delante y 0.02 detrás. `0.0` conserva la ley actual.
3. **Disipación**: la fuerza debe seguir oponiéndose a la velocidad y `F*v ≤ 0`; no introducir fuerza en el cruce.
4. **No filtrar la velocidad** en esta fase; la mezcla C1 es instantánea. Un filtro de 1 ms solo puede añadirse en la fase 6.

### 3.3 Criterios de aceptación

- `F` y `dF/dv` continuas en `knee - blend` y `knee + blend`; sin salto de pendiente.
- `F(knee)` y las asíntotas no cambian fuera de la banda.
- Reposo y cargas sin cambios; `F*v ≤ 0`; energía disipada ≥ 0.
- En bump/rebound de alta velocidad no aparecen oscilaciones ni picos nuevos.
- `suspension_35_40` y `structural_contact` pasan.

### 3.4 Pruebas

- Barrido de fuerza vs velocidad de -1 a +1 m/s, con y sin mezcla.
- Comparar pendiente numérica en el codo.
- Impacto de kerb y rebote; energía.

---

## 4. Tope duro compliant (P1/P2)

### 4.1 Situación actual

`integrate_vertical_suspension` impone el límite de recorrido como una proyección inelástica de velocidad. El impulso de tope se publica en `travel_limit_impulse_newton_seconds`. La fuerza equivalente crece muy rápido: ~0.55 kN a 0.1 mm, ~10 kN a 0.5 mm, con clamp.

### 4.2 Cambio requerido

1. **Tope compliant**: dentro de `hard_stop_compliance_m` (0.3–0.5 mm), aplicar un muelle C1 (cúbico o lineal+cúbico) y un amortiguador `hard_stop_damper_Ns_per_m` (1–2 kN·s/m), con fuerza opuesta a la penetración y a la velocidad de penetración.
2. **Clamp de último recurso**: si se supera la compliance, mantener la proyección inelástica para no violar el envelope; el clamp no debe ser la primera respuesta.
3. **Datos**: `hard_stop_compliance_m`, `hard_stop_stiffness_N_per_m` (o coeficiente cúbico), `hard_stop_damper_Ns_per_m`; `0.0` conserva el clamp actual.
4. **Energía**: la compliance debe almacenar y devolver energía de forma controlada; el amortiguador disipa. Auditar que no se inyecta energía.

### 4.3 Criterios de aceptación

- No se supera el envelope más allá de `hard_stop_compliance_m` (salvo el clamp final).
- Fuerza máxima acotada; sin pico > +10 % del actual.
- Sin levantamiento de rueda adicional; sin oscilación post-impacto.
- Energía disipada ≥ 0; sin rebote elástico no físico.
- Reposo y cargas sin cambios.
- A 500/240/120 Hz; documentar si el paso limita la compliance.

### 4.4 Pruebas

- Caída libre sobre el tope; impacto de kerb; compresión cuasiestática hasta el límite.
- Comparar impulso, fuerza máxima, energía y levantamiento de rueda.

---

## 5. Amortiguador de barra (P2)

### 5.1 Situación actual

La barra es lineal en la diferencia de recorrido (`suspension.rs:1971-1987`). No tiene amortiguación propia. Las tasas actuales son 38.36 N/mm delante y 9.88 N/mm detrás.

### 5.2 Cambio requerido

1. Añadir un amortiguador de barra `arb_damper_Ns_per_m` (100–300 N·s/m en rueda), con fuerza en cada rueda proporcional a la velocidad relativa de recorrido y opuesta a ella. La fuerza debe entrar en el impulso de suspensión y en la reacción del cuerpo una sola vez.
2. Mantener la barra actual sin cambios en estática; el amortiguador no debe alterar el equilibrio.
3. Datos por eje; `0.0` desactiva.
4. No añadir filtro de recorrido en esta fase; si hace falta, 1 ms en la fase 6.

### 5.3 Criterios de aceptación

- Reposo y reparto de carga sin cambios.
- En bache a una rueda, la respuesta de balanceo es más suave; sin oscilación de roll.
- `F_arb * v_rel ≤ 0`; energía disipada ≥ 0.
- No afecta a la recta ni al frenado en línea recta.
- `body_coupling` y `suspension_35_40` pasan.

### 5.4 Pruebas

- Bache a una rueda, escalón lateral, roll step.
- Comparar roll, cargas y energía con y sin amortiguador.

---

## 6. Filtros de 1 ms solo si las mediciones muestran ruido (P2/P3)

### 6.1 Regla

No añadir filtros por defecto. Primero implementar y medir 1–5. Solo si aparece ruido de alta frecuencia (fuerza, velocidad o contacto) que no se explique por el integrador, añadir:

- filtro de velocidad del amortiguador: `tau ≤ 1 ms`;
- filtro de recorrido relativo de barra: `tau ≤ 1 ms`;
- filtro de carga normal **solo para el modelo de neumático**: `tau = 2–5 ms`, nunca para la restricción de suspensión.

### 6.2 Prohibiciones

- No filtrar `chassis_suspension_force`, `body_acceleration_world`, `angular_acceleration`, la fuerza del muelle ni el impulso del tope.
- No usar `tau > 1 ms` en fuerza; 2 ms ya son 14.4° a 20 Hz; 5 ms son 36°.
- No usar filtros para ocultar un paso de integración grande o un substepping pendiente.
- No filtrar dos veces la misma señal (por ejemplo, `road_height_filter_tau_s` y un limitador nuevo).

### 6.3 Criterios de aceptación

- Fase adicional ≤ 7° a 20 Hz para `tau = 1 ms`.
- Sin ciclo límite de guiñada, balanceo ni rebote de rueda en la prueba de pulso a 74 m/s.
- Energía: el filtro no debe aumentar la energía total; si la aumenta, retirarlo.
- A 500/240/120 Hz; documentar resultados negativos si no se necesita.

### 6.4 Pruebas

- Análisis espectral de fuerza/velocidad en kerb y alta velocidad.
- Pulso de dirección a 74 m/s (mismo protocolo que el neumático).
- Comparar con filtros a 0, 0.5 y 1 ms; si no hay mejora medible, dejar 0.

---

## 7. Datos y archivos

### 7.1 Dónde poner cada parámetro

Recomendación:

- **Coeficientes físicos** (tope C1, amortiguador de tope, mezcla de codo, tope compliant, amortiguador de barra) en `suspension.json`, dentro de `geometry_physical.front/rear`, porque son elementos de la suspensión y deben viajar con el reglaje.
- **Filtros y limitadores numéricos** (velocidad de altura de carretera, filtros de 1 ms, carga normal para el neumático) en un dominio opcional `suspension_filters.json`, análogo a `tire_filters.json`, con secciones `front`/`rear` y valores `0.0` para desactivar.
- Si se prefiere un solo archivo, usar `suspension_filters.json` para todo y declarar explícitamente qué campos son físicos y cuáles son filtros. No duplicar los mismos valores en dos archivos.

### 7.2 Archivos previsibles

- `rust/vehicle-physics-engine/src/suspension.rs` — leyes de tope, amortiguador, barra.
- `rust/vehicle-physics-engine/src/simplified_axle_integration.rs` — integración, substepping, aplicación de fuerzas.
- `rust/vehicle-physics-engine/src/vertical_suspension_integration.rs` — tope duro compliant.
- `rust/vehicle-physics-engine/src/simplified_contact_kinematics.rs` — objetivo de contacto único.
- `rust/vehicle-physics-engine/src/suspension_geo_config.rs` — campos de perfil y validación.
- `rust/vehicle-physics-engine/src/vehicle_configuration/suspension/...` — serialización y esquema.
- `rust/vehicle-physics-engine/src/wheel_mechanics.rs` — tuning, si se añaden límites.
- `rust/data/vehicles/williams-fw19-hockenheim/suspension.json` y, si se crea, `suspension_filters.json`.
- Copias en `distribution/share/games/speed-dreams-2/cars/rust/williams-fw19-hockenheim/`.
- Tests: `suspension_35_40.rs`, `body_coupling.rs`, `structural_contact.rs`, `vertical_telemetry.rs`, y un nuevo `suspension_filters.rs` si se crea el dominio.

### 7.3 Compatibilidad

- Mantener ABI 7 y `OfpSnapshot` sin cambios.
- Campos nuevos opcionales con `#[serde(default)]`; perfiles viejos deben cargar con el comportamiento actual.
- No cambiar la firma de `SuspensionSystem::step` ni el contrato de `TriRaycastSample` salvo que sea imprescindible; documentarlo.

---

## 8. Orden de implementación

0. **Inventario y base**: confirmar HEAD, `git status`, rama; ejecutar `body_coupling`, `suspension_35_40`, `structural_contact`, `vertical_telemetry`, `wheel_centre_contact` y `verify_dll.py`; registrar ride, cargas y fuerzas actuales. Decidir el substepping (0.2).
1. **Codo C1 del amortiguador** (bajo riesgo, rápido): implementar, medir y comparar.
2. **Tope progresivo C1 + amortiguador** (P0): implementar, medir, validar energía y estática.
3. **Limitador de altura de carretera** (P1): unificar la ruta, limitar y validar kerb/rasante.
4. **Tope duro compliant** (P1/P2): implementar, validar impacto y envelope.
5. **Amortiguador de barra** (P2): implementar, validar bache a una rueda.
6. **Filtros de 1 ms** (P2/P3): solo si las mediciones lo justifican; dejar 0 por defecto.

Cada fase: compilar release, ejecutar tests, medir, actualizar el informe y la documentación. No commit ni push sin instrucción explícita.

---

## 9. Pruebas y puertas

### 9.1 Unitarias

- Continuidad C1 de la ley del tope y del codo del amortiguador (diferencias finitas).
- Monotonía, signo y energía del tope/amortiguador/barra.
- Tope compliant: fuerza, penetración, energía y clamp final.
- Validación de perfiles: campos nuevos, valores inválidos, compatibilidad con perfiles viejos.

### 9.2 Integración

- Reposo: 35.014 / 39.991 mm; cargas 1335.8 / 1667.9 N.
- Caída/impacto sobre el tope progresivo y sobre el tope duro.
- Kerb de una rueda, piano longitudinal, cambio de rasante.
- Bache a una rueda, roll step, frenada en recta, curva sostenida.
- Pulso de dirección a 74 m/s (protocolo del neumático) para detectar ciclos límite.
- 500 Hz, 240 Hz y 120 Hz; si 120 Hz no es válido, declararlo.

### 9.3 Regresiones

- `body_coupling` (6), `setup_authority` (15), `structural_contact` (16), `suspension_35_40` (5), `vertical_telemetry` (9), `wheel_centre_contact` (16; el fallo de `%TEMP%` es ambiental).
- `tire::` 36/40; los 4 fallos por fixture ausente son ambientales.
- `verify_dll.py`: ABI 7, firma, ride, cargas, sin topes ni impulsos de fondo, 2000 pasos aceptados.

### 9.4 Métricas

- Fuerza máxima de tope/amortiguador/barra.
- Levantamiento de rueda y máscara de contacto.
- Aceleración vertical/lateral de la carrocería.
- Guiñada/balanceo tras el pulso.
- Energía disipada y energía total; ninguna ganancia no explicada.
- Transiciones de contacto (chattering).

---

## 10. Límites y no-objetivos

- No cambiar el modelo de neumático, `tire_filters.json` ni la relajación lateral.
- No cambiar el acoplamiento cuerpo/esquina ni el solver 6×6.
- No cambiar aerodinámica, masas, inercias, frenos ni potencia.
- No reajustar muelles o barras para ocultar un problema de integración o de filtro.
- No usar filtros de fuerza de 10–45 ms.
- No filtrar la fuerza total de suspensión, la aceleración del cuerpo ni la fuerza del muelle.
- No cambiar ABI, FFI ni `OfpSnapshot`.
- No commit ni push sin instrucción explícita.
- `distribution/` está ignorado por Git: instalar localmente para probar, pero no esperar que esos artefactos entren en el commit.

---

## 11. Entrega

El agente debe entregar:

1. Lista exacta de archivos modificados y nuevos.
2. Leyes implementadas y parámetros por defecto (con unidades).
3. Comandos de compilación y tests, con resultados.
4. Comparación antes/después: ride, cargas, fuerza de tope, codo, barra, energía.
5. Hashes de la DLL y del perfil probados.
6. Procedimiento de vuelta atrás.
7. Pendientes y límites declarados (por ejemplo, substepping a 120 Hz o compliance no validada).
8. Confirmación de que no se ha tocado el modelo de neumático ni el acoplamiento cuerpo/esquina.

**Definición de terminado:** los seis puntos implementados o diferidos con evidencia; estática sin cambios; sin ciclo límite en la prueba de pulso; topes y kerbs con picos acotados y sin levantamiento adicional; energía no negativa; regresiones y `verify_dll.py` en verde; documentación y handoff actualizados. Si algún punto no se implementa, debe quedar desactivado por defecto (`0.0`) y declarado.
