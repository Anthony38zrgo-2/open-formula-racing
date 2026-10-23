# Corrección de reglajes instalada; T3 pendiente de prueba manual

Fecha: 22 de octubre de 2026, America/Bogota. Instalación local autorizada por el usuario. Sin commit ni push.

## Resultado

La DLL instalada en `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` conecta los controles existentes con los elementos que consume la suspensión simplificada. Se conserva el perfil base y el reglaje SQLite existente.

- `suspension_setup.rs` conserva una referencia de diseño y traduce amortiguación, compresión, extensión y topes de cada eje a sus coeficientes geométricos. Los valores se calculan de forma absoluta: repetir un reglaje no acumula multiplicadores.
- `open_formula.rs` aplica esa traducción al editar/restaurar el reglaje y sincroniza el ratio de barra visible con el utilizado. Publica `effective_suspension` para inspección técnica.
- `suspension_geo_config.rs` expone la consulta del ratio geométrico; `lib.rs` registra el módulo.
- `tests/setup_authority.rs` cubre los contratos mediante la interfaz de simulación usada por el host.
- No se cambió el neumático, la integración vertical ni el contacto para forzar que una rueda toque el suelo.

## Pendientes resueltos del banco

1. Amortiguación trasera: el banco esperaba 1,5 veces el coeficiente, pero pedía `1.06 / 0.71`, aproximadamente 1,4929577. Se corrigieron los coeficientes de referencia, no la ley física para acomodar un resultado. Ahora la trayectoria coincide exactamente con el perfil físico equivalente.
2. Tope delantero: la caída original desde 0,7 m no lo excitaba. La variante desde 1,3 m sí produce respuesta al cambiar el tope y coincide exactamente con el elemento físico equivalente. Los demás controles conservan el banco original.
3. El verificador ahora falla mediante aserciones si un control no responde, si no coincide con su elemento, si se altera el perfil sin reglaje o si aplicar el mismo reglaje dos veces cambia la respuesta. Se corrigió también la firma de la función ABI de errores y se ejecuta Python con UTF-8.

## Validación ejecutada

| Comprobación | Resultado |
| --- | --- |
| `cargo test --offline -p vehicle_physics_engine --test setup_authority` | 15 pruebas, 0 fallos |
| `cargo build --offline --release -p vehicle_physics_engine` | Correcto |
| Banco dinámico: 10 controles, ambos ejes | Todos producen respuesta y coinciden exactamente con su elemento físico equivalente |
| Perfil sin reglaje, DLL anterior frente a candidata | Trayectoria numérica idéntica |
| Mismo reglaje aplicado una y dos veces | Trayectoria numérica idéntica |
| `validate_formula_setups.py` | 35 comprobaciones SQLite, 0 fallos; aplicar/editar/restaurar restricciones de configuración correcto |
| Banco repetido cargando la DLL desde la distribución después de instalar | Correcto |
| Firma ABI | Coincide con la interfaz del host |
| SHA-256 instalado y manifiesto de distribución | Coinciden |
| Base de reglajes y todos los JSON del perfil instalado | SHA-256 conservados |
| `git diff --check` | Sin errores de espacios |

No se ejecutó toda la suite de pruebas de biblioteca: el target `--lib` tiene referencias a fixtures de otros vehículos ausentes, documentadas en el propio test de integración. La validación descrita arriba es la efectivamente ejecutada.

Evidencia: `verification/verification.json`, `storage_validation/summary.json`, `installation.json`. El banco sintético no reproduce T3 ni demuestra que la FR vaya a recuperar antes el apoyo en esa curva. En ese banco el número de muestras sin carga con el reglaje completo pasa de 66 a 69; no se utiliza para afirmar una mejora de conducción.

## Instalación y reversión

No había procesos Open Formula/Speed Dreams abiertos al sustituir la DLL.

- DLL anterior: `28756170fcb0e92f3bd9e27aa9a2c3034bfc9d1a2383964ebdc7eadfd1e3ae5b`.
- DLL instalada: `7088504e89d84a0a0cd8daf884433d8557ca1d6a8ff877e75035144149ae55cd`.
- Respaldo externo de DLL y manifiesto: `C:/Users/bill/open-formula-backups/2026-10-22-suspension-0bf973b0`.
- Reversión con el juego cerrado: ejecutar `C:/Users/bill/open-formula-backups/2026-10-22-suspension-0bf973b0/rollback.ps1`. El script comprueba que no exista una sesión y que la DLL siga siendo esta candidata antes de restaurar.

Se actualizó únicamente la entrada de esta DLL en `distribution/distribution_manifest.json`. No se modificó `distribution/user-settings/open_formula.db`, no se reescribieron perfiles ni se lanzó una sesión de conducción.

Para repetir el banco tras la instalación, usar `verify_setup_authority.py --reference-dll C:/Users/bill/open-formula-backups/2026-10-22-suspension-0bf973b0/vehicle_physics_engine.dll --candidate-dll D:/open-formula/distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` con `python -X utf8`. La referencia debe ser el respaldo anterior, no la DLL ya sustituida.

## Prueba que queda al usuario

1. Abrir `distribution/OpenFormula.exe` normalmente.
2. En el garaje de Melbourne, cargar **Reglaje 1** y aplicarlo. Su nombre, valores guardados y compatibilidad con el perfil base se conservan. Ahora las barras 0,20 y los ajustes de amortiguadores/topes tienen efecto físico.
3. Probar T3 y valorar el tiempo sin apoyo de FR y la recuperación al salir. Extensión representa amortiguación de rebote: subirla aumenta la resistencia al extenderse, no el recorrido.

**Estado final: validación técnica e instalación completas para este candidato; aceptación de conducción en T3 reservada al usuario.**
