# Corrección del marco visual del Williams — 2026-10-22

## Diagnóstico confirmado

El fix anterior recuperó tamaño y datos de transmisión, pero seguía colocando el chasis
como si el origen del modelo fuera el centro de masa. Las mallas de chasis/conductor
se exportaron desde el origen geométrico de Formula90s, con un desplazamiento vertical
baked de **0,48865 m**, documentado en `williams_generation_report.json`.

Usar `COM - altura_estatica` para ese modelo añadía la componente longitudinal del COM
al chasis y no a las ruedas. Además, corregía altura sobre Z mundial en vez del eje
vertical del coche, y reconstruía una orientación mediante Euler. Esto impedía conservar
la alineación en cabeceo/balanceo. Se confirmó también que el signo de camber del puente
era el opuesto al usado por Speed Dreams: izquierda y derecha estaban invertidas.

## Implementación

- `src/modules/simu/rustphysics/rustphysics.cpp`: `posMat` se construye directamente
  desde el origen y la base completos del estado Rust. Se resta el desplazamiento de
  exportación sobre el eje vertical del cuerpo. La posición publicada para carrera/HUD
  sigue siendo el COM; no se modifica el integrador ni se mueve la física para compensar
  una malla. Las ruedas siguen transformando el buje mundial por la inversa de esa misma
  matriz. Camber: perfil a la derecha y negativo del perfil a la izquierda.
- `rust/vehicle-physics-engine/src/open_formula.rs` y `open_formula_ffi.rs`: el camber
  del snapshot visual procede de `kinematic_camber_rad`, relativo al chasis. El camber
  efectivo respecto al plano de contacto contin�a en neum�ticos/temperatura; no se usa
  para volver a inclinar la malla sobre una carrocer�a que ya est� rotada.
  La direcci�n/toe visual conserva el signo de Rust: +Y de Rust se convierte en +Z
  de Speed Dreams. Se elimin� la inversi�n extra del �ngulo. La trayectoria
  vertical de buje usada por Simplified no se modifica.
- `tools/prepare_williams_vehicle.py`: registra `Graphic Objects/source mesh origin
  offset z` a partir del desplazamiento real usado por el exportador.
- XML instalado del Williams: nuevo atributo de origen, tomado del informe de generación
  existente; no se regeneraron assets ni se volvió a imponer el setup simuv5.
- `src/modules/graphic/osggraph/Viewer/OsgScreens.cpp`: captura opcional de los frames
  30/90/180 del framebuffer del juego cuando existe `OPEN_FORMULA_CAPTURE_DIR`. Desactivada
  en juego normal; no captura el escritorio ni requiere cambiar controles del usuario.

La composición visual queda, para cada vértice original p:

```
modelo = cambio_de_ejes(p) + offset_exportacion
pose_modelo = pose_origen_Rust * inversa(offset_exportacion)
pose_modelo * modelo = pose_Rust * p
```

El COM desaparece de la conversión de malla. Una traslación o rotación del coche transforma
chasis y bujes en el mismo marco. Se conserva el movimiento real de suspensión de Rust;
no se fijan las ruedas artificialmente a una altura para ocultar el problema.

## Validación

Compilación completa e instalación correctas en `D:/open-formula/distribution`.
Runner: `implementation/reports/rust_integration/run-wheel-alignment-session.ps1`.
Se repitió el mismo caso final, con capturas del render añadido a la sesión.

- Revisadas visualmente `frame-030.png` (reposo/inicio) y `frame-090.png` (arranque,
  primera marcha a unos 66 km/h): las ruedas delanteras se sitúan junto a los extremos
  de suspensión, desaparece el desfase longitudinal y el camber tiene el signo correcto.
- Revisada `frame-180.png`: la alineación sigue el coche, pero también documenta la
  caída bajo el circuito, que continúa siendo un fallo físico abierto.
- 0 avisos `detected NaN`; sin rechazo de ABI ni pasos físicos.
- Se conservan comprobación de marchas y aplicar/releer/restaurar setup del garaje.
- No se revisó una vista exterior de las ruedas traseras. Comparten la misma conversión,
  pero las capturas de cockpit no equivalen a una aprobación visual de las cuatro ruedas.
- Los brazos de suspensión de la malla del chasis son geometría estática: esta corrección
  no añade animación de sus articulaciones durante el recorrido de suspensión.

No se declara pasada la integración de conducción. No se modificó ni compiló
`D:/Formula90s`; no se ejecutaron suites adicionales. Sin commit ni push.

Evidencia: `implementation/reports/rust_integration/wheel-alignment-20261022/`.
Build/install: `wheel-alignment-build.log`, `wheel-alignment-install.log`.

SHA-256 del adaptador instalado: `b3b0f29f6421df7c0f0a1dbea8f9b66c70afc9046f1c9c679a0a51bcd213ad21`.

SHA-256 de Rust instalada: `1534cb623cb73fbccf83c8d2243fb855e0213b5b81e2f3cca79c9794984aa79a`.
