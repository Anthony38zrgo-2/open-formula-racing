# Corrección estructural: reproducción, contrato de masa y transferencia (Fases A–C)

Base: `4a14467ca061982fca87a60212489bef4e018911`, D:/open-formula. Alcance de este informe: lo ejecutado sobre el plan [structural_correction_plan.md](../melbourne_t3_extension/structural_correction_plan.md). No se ha recompilado ni instalado la DLL, no se ha tocado ningún reglaje del usuario y no se ha conducido en T3.

Banco nuevo: `rust/vehicle-physics-engine/tests/structural_contact.rs` (8 pruebas). Traza: `phase_a_trace.jsonl` (se regenera con `OF_STRUCTURAL_TRACE=1`).

## 1. Reproducción con la base sin corregir

Los dos defectos P0 se reprodujeron con números antes de tocar el solver:

| Defecto | Medición en la base |
| --- | --- |
| Rueda sin apoyo pierde su fuerza de suspensión | Con la esquina FR apoyada el elemento transmite 1334,246 N; al levantarla, la fuerza vertical del cuerpo cae 1310,977 N (98,2 % de esa fuerza desaparece). Con carga de barra diferencial, el par de balanceo era idéntico con FR apoyada y levantada (−15 052,234 N m en ambos casos): la esquina aérea no aportaba nada. |
| Convergencia del eje sin suelo | Cada subpaso con eje apoyado agotaba las 32 iteraciones y tomaba el fallback: 2 fallbacks por paso (uno por eje delantero), `axle_max_iterations = 32`. El residuo proyectado solo conocía los topes de recorrido, de modo que la reacción de suelo aceptada (`m(v'−v)+hF = J > 0`) nunca podía anularse. |

Controles que ya pasaban antes: impulso publicado = carga × dt (2,780561 N s con 1334,670 N a 1/480 s), suma de reacciones 6002,047 N frente a peso 6007,554 N (−0,092 %), permutación izquierda/derecha invariante y caída libre del cuerpo a −9,80665 m/s² (trivial: sin ruedas no había fuerza que transmitir).

## 2. Fase B — proyección con suelo en la convergencia del eje

`simplified_axle_integration.rs`:

- `velocity_bounds` combina el tope de recorrido con el límite de suelo (`max(droop, ground.limit_at(h))`), con el mismo recorte degenerado que ya usaba el solve de esquina.
- `projected_residual` usa esos límites; el criterio `2e-9 N s` pasa a ser alcanzable cuando la esquina está sostenida por el suelo.
- Se añadieron `AxleSolveDiagnostics { iterations, converged }` y los contadores públicos `SuspensionSystem::axle_fallback_count` / `axle_max_iterations` (solo traza, sin efecto en fuerzas).

Efecto medido: fallbacks 2 → 0 por paso; iteraciones 32 → 4 en reposo y 1 en conducción plana. Las cargas estáticas no cambian (sección 3.2).

## 3. Puerta de diseño: contrato de masa (adelanto de Fase D)

El plan exige cerrar la masa antes de aplicar la transferencia de la Fase C: aplicar la fuerza del elemento a la rueda aérea sin separar masas rompe la caída libre (se midió −3,02 m/s² en el cuerpo con el reparto antiguo). Se adoptó la opción recomendada: masas de rueda físicas, contadas una sola vez.

| Presupuesto | Valor |
| --- | --- |
| Masa total configurada (con combustible) | 612,600 kg (combustible 7,600 kg) |
| Masa no suspendida por esquina (inventario) | FL 15,0 · FR 15,0 · RL 18,0 · RR 18,0 kg |
| Suma de esquinas | 66,000 kg |
| Masa del cuerpo (suspendida) | 546,600 kg |
| Centro de masas local | (0,000; −0,2437; +0,4578) m |
| Inercia principal total | (396,03; 462,18; 104,90) kg m² |
| Inercia principal del cuerpo | (254,97; 281,50; 56,61) kg m² |

Implementación: `simulation::corner_unsprung_mass_kg`, `sprung_mass_kg`, `sprung_principal_inertia`; el cuerpo integra la masa suspendida y su inercia (término diagonal de ejes paralelos por esquina), cada esquina integra la suya con gravedad axial (`body_orientation.y · UP`). El multiplicador de masa virtual deja de alimentar la dinámica: solo queda como calibración de amortiguación radial (`wheel_mechanics::physical_unsprung_mass_kg` frente al heredado `unsprung_mass_kg`, ambos 1,0 en el perfil canónico).

Equilibrio estático conservado: ΣJ = 6002,840 N frente a 6007,554 N de peso (−0,078 %), y ΣF del cuerpo = masa suspendida × g.

**Consecuencia física declarada:** los muelles ahora sostienen solo la masa suspendida, así que el coche se asienta más alto. Compresión de reposo medida: delantera 43,38 → 40,35 mm; trasera 67,83 → 60,30 mm (≈ 3 y 7,5 mm). No se ha cambiado ningún valor de reglaje ni de geometría: es el nuevo equilibrio del modelo.

## 4. Fase C — transferencia al cuerpo en el anclaje

`simulation.rs`:

- La fuerza del elemento (`chassis_suspension_force`: muelle + amortiguador + barra + topes) se aplica **siempre** al cuerpo, en el centro del buje (`wheel_anchor_local`), antes de la bifurcación `is_grounded`.
- En la rama apoyada, `support_force_world` pasa a `Vec3::ZERO` para el backend simplificado: el soporte no se cuenta dos veces; solo queda la fuerza tangencial del neumático en el parche.
- La geometría de aplicación es continua al separar y recuperar contacto (mismo punto y misma fuerza; lo único que desaparece son las fuerzas tangenciales).

Medición: FR apoyada transmite 1193,087 N y FR levantada 785,471 N; el delta del cuerpo (379,812 N) coincide con la suma de deltas de los elementos (379,825 N), es decir, no se pierde nada y la diferencia es el propio cambio de estado del elemento. Con barra diferencial, el lado aéreo sigue entregando −633,230 N de barra y 355,199 N de elemento.

## 5. Verificación ejecutada

`cargo test --offline -p vehicle_physics_engine --test structural_contact` → 8/8.
`cargo test --offline -p vehicle_physics_engine --test setup_authority` → 15/15.

| Banco | Antes | Ahora |
| --- | --- | --- |
| `p0_airborne_wheel` | cuerpo pierde 1310,977 N | delta del cuerpo = delta de elementos (379,812 / 379,825 N) |
| `axle_fallback_usage` | 2 fallbacks por paso, 32 iteraciones | 0 fallbacks, 4 iteraciones |
| `free_fall` (centro de masas) | n/a (cuerpo −3,02 m/s² con el reparto viejo) | −9,8068 m/s² |
| `published_normal_impulse` | consistente | consistente (error 0 N s) |
| `p0_airborne_arb` | par idéntico al levantar (nada transmitido) | lado aéreo con barra y elemento; identidad de fuerza cuerpo/elementos |
| `left_right_permutation` | invariante | invariante |
| `mass_contract` | no existía | cierra: 546,6 + 66,0 = 612,6 kg |
| `flat_ground_drive` | n/a | 4/4 apoyadas, 0 fallbacks, 5,69 m/s tras 2 s al 30 % |

## 6. Límites de esta entrega

- **Marcos más gruesos que 1/240 s** se subdividen internamente (`MAX_INTERNAL_STEP_SECONDS`), con un máximo de 8 subpasos: un marco de 1/120 s ejecuta dos solves de 1/240 s y un host a 60 Hz, cuatro. Es coste, no aproximación: el estado publicado avanza exactamente el tiempo pedido.
- **Sin T3**: no se ha conducido. La altura de reposo cambia ~6 mm y la inercia de guiñada baja de 462 a 281 kg m²; el efecto en pista lo valora el usuario.
- **Sin DLL**: no se ha compilado en release, ni verificado ABI, ni instalado candidato; `--lib` sigue sin compilar por fixtures ajenos, de ahí que los bancos sean targets de integración.
- La telemetría no se ha ampliado: los contadores nuevos son campos de estado; publicarlos exige coordinar el consumidor.
- **Referencia de `q`**: `rest_compression_m` vale 0,0950 m delante y 0,1094 m detrás, mientras el coche reposa en 0,038–0,066 m; el `q` estático queda en −0,055 m delante y −0,043 m detrás, dentro del sobre recorrido preparado ([−0,085, +0,025] y [−0,090, +0,030] m). Es una discrepancia de referencia previa a esta corrección: no se ha tocado porque la autoridad de reglajes y las cargas estáticas están validadas contra ella, pero queda anotada para Fase E.

## 7. Reproducción

```powershell
cd D:\open-formula\rust
$env:OF_STRUCTURAL_TRACE = "1"
cargo test --offline -p vehicle_physics_engine --test structural_contact -- --nocapture
cargo test --offline -p vehicle_physics_engine --test setup_authority
```

## 8. Continuación (Fase D): aceleración de anclaje y barrido temporal

- La ecuación de la esquina incorpora ya `m_u · (g + a_cuerpo)·eje`. La aceleración del cuerpo entra con un paso de retardo (predictor explícito), que es lo que permite mantener el solve de suspensión separado de la integración del cuerpo.
- Barrido 1/240 frente a 1/480: misma altura de reposo (diferencia 0,0000 m) y velocidad final 2,057 frente a 2,052 m/s con el mismo input; compresiones idénticas a 1e-8 m. Convergencia dentro del rango admitido.
- Escalón de bordillo de 15 mm a 6 m/s: 4/4 apoyadas tras la recuperación, 0 fallbacks, pico de carga 75,2 kN; el estado publicado sigue siendo finito.
- El proyecto de Newton 2×2 se probó y se retiró: empeoraba la convergencia en los estados con restricción activa. El esquema actual es Gauss-Seidel simple y, si detecta divergencia (`norma > 1,5 × mejor`), una iteración amortiguada derivada de la ganancia `h·k_arb/m`.

## 9. Cierre del límite de 1/120 s (subdivisión interna)

El equilibrio asimétrico a 1/120 s (2/4 ruedas cargadas, 1,559 m/s frente a 2,05) no era un fallo de convergencia: el solver convergía, con 0 fallbacks y 4 iteraciones, a un estado donde una pareja de ruedas quedaba colgada del tope de droop y el cuerpo se sostenía con la otra. Reproducido a 0, 10 y 20 mm de profundidad de plano, siempre 2/4: no era contacto marginal.

Corrección: `VehicleSimulator::step` y `step_with_aerodynamics_environment` subdividen el marco cuando supera `MAX_INTERNAL_STEP_SECONDS = 1/240 s` (máximo 8 subpasos). Un marco de 1/120 s pasa a dos solves de 1/240 s y el estado publicado avanza el tiempo pedido.

Resultado: a 1/120 s, 4/4 apoyadas, 0 fallbacks y 2,057 m/s, idéntico a 1/240 y 1/480 (2,057 / 2,052). La prueba `coarse_host_frames_are_subdivided_internally` lo vigila.

Fase E iniciada además con dos bancos: contacto parcial (un rayo de tres, fracción 0,25, carga 1338,9 N, sigue apoyada) y envolvente de geometría (el recorrido usado queda dentro de `q_min/q_max` preparados en ambos ejes).

## 10. Candidato instalado (Fase F, instalación reversible)

- Release compilado desde la misma base, con todos los fuentes anteriores a la DLL (comprobado por fecha).
- ABI: `ofp_abi_signature` = `0xeebdad2292b2359d`, idéntica a la DLL instalada anteriormente.
- Instalado en `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll`, SHA-256 `233d34912769474587cb34c97323a3ccb8d2ee325f018ff19ceda429e4b3a42a`, sin sesión abierta (no había proceso del juego).
- Respaldo en `C:\Users\bill\open-formula-backups\2026-10-22-structural-8c1cf079` (DLL anterior `7088504e…` y `distribution_manifest.json`), con `rollback.ps1` que restaura ambos y se niega si la DLL instalada no es la esperada.
- `distribution_manifest.json` actualizado en la entrada de la DLL (tamaño y hash); el resto de las 5460 entradas intacto.
- Perfiles del vehículo: 15 de 16 ficheros protegidos sin cambios. El único distinto es `user-settings/open_formula.db`, escrito por la sesión del usuario a las 21:59, antes de este trabajo; no se ha abierto en escritura.
- Pruebas en el momento de instalar: 17/17 estructurales y 15/15 de autoridad de reglajes.
- Manifiesto del candidato: `installation.json`. **Aceptación de conducción en T3: pendiente del usuario**; guía A/B incluida allí mismo.

Verificación de la DLL instalada (no solo del rlib), con el banco de autoridad de la DLL anterior como referencia (`dll_verification/`): la DLL crea, carga perfiles, avanza y publica; los **diez controles** del garaje siguen resolviendo al elemento físico equivalente (`matches_the_equivalent_physical_element: true` en los diez) y todos cambian la trayectoria; el reglaje guardado se reproduce de forma idempotente (diferencia 0,0). La trayectoria de diseño ya no es idéntica a la del build anterior, que es exactamente lo esperado al corregir fuerzas.

Cobertura de Fase E en este candidato: barrido temporal, contacto parcial, envolvente de geometría, plano inclinado, pérdida de todos los rayos de una rueda, reset y los quince controles de autoridad. Queda la repetición de los diez controles del garaje con el coche rodando y el retorno a boxes dentro de una sesión de pista.

## 11. Siguiente

1. Cerrar Fase E: repetir los diez controles del garaje con el coche rodando y el retorno a boxes en pista (reset ya cubierto en banco).
2. Fase F: comparación A/B y aceptación en T3 por el usuario. Si la rechaza, `rollback.ps1` restaura la DLL anterior y el manifiesto de distribución.
