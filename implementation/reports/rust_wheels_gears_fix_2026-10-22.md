# Ruedas y marchas del Williams con Rust — 2026-10-22

## Resultado

Fix compilado e instalado. ABI 4 y DLL instalada idéntica a la compilada.
No se modificó ni compiló Formula90s. Sin commit ni push.

## Causas y correcciones

- Las ruedas se escalaban a cero: CarElt no recibía radio de llanta, altura de flanco
  ni anchura. El puente publica ahora radio y anchura del perfil Rust; el diámetro visual
  y de rodadura concuerdan. La llanta y disco usan dimensiones del XML visual.
- La altura de rueda se estimaba mediante radio menos extensión de muelle. Ahora el buje
  mundial publicado por Rust se transforma mediante la inversa de la matriz del modelo.
  Esto conserva exactamente su colocación mundial al dibujar y evita mezclar orígenes.
- Se publica el ángulo de giro visual integrando la velocidad angular aceptada por Rust.
  Es estado de presentación; no introduce fuerzas, masas ni una segunda física.
- El controlador humano limitaba las peticiones con `_gearNb=0` y `_gearOffset=0`.
  Ahora recibe 6 marchas hacia delante, offset 1 para reversa y relaciones de caja
  multiplicadas por el grupo final. Neutral conserva relación cero. La primera relación
  total es 15,8565 y la sexta 5,7288; reversa total -13,95.
- Los metadatos se actualizan también tras aplicar/restaurar setup.

## Archivos

- `src/modules/simu/rustphysics/rustphysics.cpp`: metadatos, pose y rotación de ruedas;
  límites/relaciones de caja; instrumentación del controlador humano en el escenario final.
- `rust/vehicle-physics-engine/src/open_formula.rs` y `open_formula_ffi.rs`:
  exportación `ofp_vehicle_specs`, obtenida de la configuración realmente activa.
- `src/modules/simu/rustphysics/rust_abi.h`, `rust_library.h` y `rust_library.cpp`:
  estructura fija y símbolo obligatorio; ABI 4 y firma de layout ampliada.
- `src/libs/robottools/formula_gear_controls.h` y su `CMakeLists.txt`:
  exportación de la misma función usada por rthumandriver para que el caso integrado
  compruebe los límites reales del controlador sin copiar su lógica.
- `CMakeLists.txt`: el validador preexistente, que compila esa implementación directamente,
  declara enlace estático para mantener compatible la compilación general. No se ejecutó.

## Verificación en una sesión integrada

Compilación completa con CMake e instalación en distribution. No se ejecutaron suites
separadas ni pruebas en el origen. Runner:
`implementation/reports/rust_integration/run-wheels-gears-session.ps1`.

- `RUSTSPECS forward=6 offset=1 ratios=(15.8565,5.7288)`.
- Frente: radio 0,311949 m, diámetro de render 0,623898 m, ancho 0,307541 m.
- Detrás: radio 0,322016 m, diámetro de render 0,644032 m, ancho 0,389105 m.
- `RUSTHUMANGEAR enabled=1 first=1 neutral=0 reverse=-1 top=6 final=0`.
- Marchas observadas en snapshots Rust: [0, 1, 2, 3, 4, 5, 6].
- El escenario de lanzamiento/cambio pasa ahora las solicitudes por
  `applyFormulaGearControls`, la función real del piloto humano. Los eventos de tecla
  se suministran programáticamente a esa función; no se afirma prueba manual de A/Z.
- 0 avisos `detected NaN` en la captura. La sesión conserva lectura/aplicación de garaje.
- Configuración y overrides del usuario restaurados al terminar.

SHA-256 DLL Rust: `2c817d4c321c930b0a6bc6ab91c6b6c3f35c81d60b015ed9812bd21888d4ce02`.

## Límite pendiente

La integración global de conducción sigue fallida: durante la maniobra se pierden los
impactos de suelo y el coche cae fuera de la malla. Este fix no diagnostica esa pérdida
ni acredita conducción estable. Tampoco hubo aprobación visual manual de la imagen:
la evidencia de ruedas verifica dimensiones publicadas y transformación de pose.

Evidencia: `implementation/reports/rust_integration/wheels-gears-20261022/`.
Logs de build/install: `wheels-gears-build.log`, `wheels-gears-install.log`.
