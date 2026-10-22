# Correcciones Rust de presentación, garaje y contratos — 2026-10-22

## Alcance y estado

Implementadas las correcciones solicitadas. Compilación completa e instalación correctas.
El caso de integración confirma las correcciones de presentación y la API usada por el garaje,
pero **la integración global de conducción sigue FALLIDA**: el coche pierde contacto con la
malla durante la maniobra y cae. No se atribuye aquí ese fallo al setup ni al guion sin diagnóstico.
No se modificó ni se compiló D:/Formula90s. Sin commit ni push.

## Cambios

- `src/modules/simu/rustphysics/rustphysics.cpp`: construye `CarElt::pub.posMat` al crear el
  vehículo y en cada publicación. La matriz anterior quedaba sin inicializar por el backend.
  Inicializa dimensiones y posiciones visuales de conductor/capó desde el XML visual, sin
  importar la física simuv5. Publica límites RPM del perfil Rust para evitar divisiones por
  cero del HUD. No altera las ecuaciones del chasis ni añade una segunda integración.
- `src/interfaces/iphysicsengine.h`, `iraceengine.h` y
  `src/modules/racing/standardgame/standardgame.h`: interfaz de garaje independiente del
  módulo concreto. Los backends antiguos conservan su ruta. Rust declara propiedad del
  setup; si falla su configuración, no se muestran parámetros nativos ficticios.
- `src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp`: los cuatro
  grupos muestran los descriptores Rust con valores actuales, límites, pasos y unidades.
  Ángulos visibles en grados y transporte en radianes. Datos de diseño de solo lectura.
  Aplicar envía únicamente campos modificados; un rechazo mantiene abierto el editor.
  Cancelar/restablecer deshacen la edición pendiente. No muestra tercer muelle con Rust.
  Esta entrega expone los 38 descriptores actuales; no afirma que sea un inspector exhaustivo
  de todas las claves del JSON original.
- `rust/vehicle-physics-engine/src/open_formula.rs` y `open_formula_ffi.rs`, junto con
  `rust_library.*` y `rust_abi.h`: ABI 3; descriptores serializados sin nuevos structs
  dependientes del compilador; validación de rangos y transacción atómica del setup.
  Los cambios ajenos al combustible conservan su masa actual. El ARB editable apunta a
  `suspension.geometry_physical.{front_arb,rear_arb}.ratio`, utilizado por la geometría activa.
- Persistencia separada en `distribution/user-settings/config/rust_setup_<indice>.tsv`.
  Al recrear el coche se validan los overrides antes de aplicarlos. Restaurar el perfil base
  elimina esos overrides. Un fallo de escritura se informa explícitamente.
- `src/modules/simu/rustphysics/CMakeLists.txt`: Cargo se invoca siempre desde el target de
  CMake y decide incrementalmente qué recompilar. No se instala silenciosamente una DLL
  existente por no encontrar Cargo. Se conserva la comprobación de versión y layout ABI.
- `vehicle_configuration/fuel/{configuration,document,validation}.rs`, `legacy_profile.rs`
  y `rust/data/vehicles/williams-fw19-hockenheim/fuel.json`: densidad explícita y validada
  de 0,75 kg/L para el contrato con Speed Dreams. Es un supuesto de conversión del port,
  no una medición histórica. Rust mantiene kg; CarElt/tanque/servicios de boxes usan litros.
- Máscara aerodinámica: los dos bits de impacto físicos se trasladan a los índices 0 y 3,
  conservando exactamente 12 rayos de rueda y 2 del fondo.

## Compilación e instalación

Workspace de compilación: `C:/Users/bill/open-formula-build/engine`.
CMake: `C:/Users/bill/open-formula-build-tools/msys64/ucrt64/bin/cmake.exe`.

```
cmake --build C:/Users/bill/open-formula-build/engine --parallel 6
cmake --install C:/Users/bill/open-formula-build/engine
```

Cargo se ejecuta en `D:/open-formula/rust` con `build --offline --release`.
Se reconstruyeron todos los consumidores de las interfaces modificadas. Un build posterior
invocó Cargo de nuevo con la DLL existente y terminó incrementalmente en 0,12 s.

SHA-256 de DLL Rust instalada (igual a la compilada): `8fc322a8c76c1cbdcf91310a608de9b2d2627e319b2330a28e1ff0c11b746a2d`.

## Único caso de integración final y repetición correctiva

Guion: `implementation/reports/rust_integration/run-corrections-session.ps1`.
Ejecuta la distribución real Williams/Fuji con render y escenario integrado habilitados,
y restituye configuración y overrides del usuario al finalizar. No ejecuta suites en el origen.

1. Primer intento: falló la creación por el nuevo campo de densidad no incorporado aún al
   schema estricto. Se preservó en `corrections-20261022/attempt-1/`. Se corrigió schema,
   validación y diagnóstico del error, y se repitió el mismo caso.
2. Repetición: **0 avisos `detected NaN`**, sin fallos de creación, ABI ni pasos rechazados.
3. `RUSTGARAGE fields=38 readback=1 arb=1 bias=1`: misma API que usa la pantalla;
   acepta ARB frontal 0,55 y reparto de frenada 0,58 y ambos valores se releen correctamente.
4. `RUSTSETUP document arb=1 bias=1 third_spring=0`, restauración correcta y reset con
   código de retorno 0. El override generado por el caso se retira/restaura al terminar.
5. Conversión registrada: aproximadamente 7,4336 kg / 0,75 = 9,9114 litros.
6. El recorrido termina perdiendo todos los impactos y cayendo por debajo del circuito.
   Eso impide declarar pasada la integración general, aunque los cambios de setup siguen
   aceptándose y el render no vuelve a emitir NaN en esta captura.

No hubo aprobación visual manual de la disposición del editor ni validación manual completa
por teclado/ratón. La sesión prueba la API común y su lectura posterior, no sustituye esa
revisión visual. Tampoco acredita colisiones completas ni estabilidad física del port.

Evidencia: `corrections-20261022/{stderr.log,stdout.log,verification.json,summary.txt}`.
Logs de compilación e instalación: `corrections-*-build.log`, `corrections-install.log`.
