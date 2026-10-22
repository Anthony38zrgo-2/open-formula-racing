# Williams: toe local por rueda y tercer muelle trasero

Solicitud del usuario: conservar el mismo valor de toe a izquierda/derecha, añadir un tercer muelle trasero de 285 kN/m y permitir configurarlo en el garaje.

Estado: implementado, compilado e instalado en `distribution`. Pruebas nativas y del garaje correctas. Conducción con este conjunto todavía pendiente. Sin commit ni push.

## Comportamiento

- El Williams declara `Formula Controls/toe convention = wheel local`.
- Toe delantero: **−0,1° en ambas ruedas**. Toe trasero: **+0,3° en ambas ruedas**. El generador y el garaje ya no necesitan guardar el valor izquierdo con signo contrario.
- En esta convención, positivo es convergencia y negativo divergencia en cualquiera de los lados. `setup.toe[].value` conserva el valor configurado; la orientación interna en coordenadas del chasis refleja la base local de cada lado. Es la conversión de coordenadas necesaria para obtener geometría simétrica sin invertir el valor del setup.
- Los coches que no declaran esta convención conservan la interpretación anterior de yaw firmado.
- `Rear Heave Spring/spring = 285000 N/m`, bellcrank 1. Se usa el tercer elemento nativo, integrado en el solver vertical existente. No se añade otro integrador ni una barra para sustituirlo.
- No se han añadido coeficientes nuevos de amortiguación de heave; se conservan los amortiguadores de rueda y los valores nativos por defecto del tercer elemento.
- En **Suspensión → Barras / Chasis → Tercer muelle**, el valor inicial es **285,0 kN/m**, ajustable de 0 a 1.000 kN/m en pasos de 5 kN/m. Cero desactiva el muelle. El control apunta a `setup.heaveSpring[1]` y aplica mediante la ruta de reconfiguración física del garaje.
- El modelo 3D de la pestaña de suspensión se reduce en altura para dejar sitio al nuevo control sin solapamientos. El resto de pestañas conserva su tamaño de vista.

## Archivos y responsabilidades

- `src/modules/simu/simuv5/wheel.cpp`: interpretación del toe local, manteniendo el valor del usuario y compatibilidad con yaw firmado.
- `tools/prepare_williams_vehicle.py`: nuevo parámetro `--rear-heave-stiffness`, predeterminado 285000; convención de toe local y valores iguales en ambos lados; informe de generación actualizado.
- `src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp`: control del tercer muelle trasero en kN/m y espacio para mostrarlo.
- `tools/validate_formula_vertical_authority.cpp`: pruebas con las funciones de producción de toe, fuerza de heave y reconfiguración.
- `tools/validate_formula_garage_ui.py`: validación del campo, conversión de unidades, controles y disposición.
- `implementation/migration_status.json`: estado de esta entrega en una entrada nueva, conservando el historial existente.
- XML e informe candidato/distribuido: regenerados. Solo estos dos archivos cambiaron dentro del candidato; gráficos y audio quedaron idénticos al respaldo.
- `distribution/lib/games/speed-dreams-2/modules/simu/simuv5.dll` y `.../userinterface/legacymenu.dll`: reconstruidos e instalados. Manifiesto actualizado únicamente para ambos binarios y XML/informe del coche.

Los cambios previos del garaje, siluetas, setup de frenada y validadores se conservaron. No se modificaron frenos, freno motor, diferencial, aerodinámica, temperatura, presión, rigidez de rueda ni barras con este encargo. La telemetría continúa activada como estaba en el XML anterior.

## Validación

Compilación Release con UCRT64: `simuv5`, `legacymenu`, `validate_formula_vertical_authority` y `validate_williams_chassis`, correcta.

Banco nativo de autoridad vertical: **102 comprobaciones, 0 fallos**. Incluye:

- Toe negativo y positivo iguales en las cuatro entradas, conservados por `SimWheelReConfig`.
- Orientaciones físicas simétricas y compatibilidad de la convención anterior.
- Carga del muelle trasero a 285 kN/m en la configuración consumida por el solver.
- 1 mm de compresión común añade 142,5 N por rueda trasera desde el tercer muelle, sin modificar las delanteras.
- Balanceo puro no recibe rigidez adicional del tercer muelle.
- Cambiar el setup a 300 kN/m llega al solver, y puede restaurarse a 285 kN/m.
- El banco conserva sus pruebas de rechazo intencionado de estados inválidos; los errores sintéticos que registra forman parte de esas comprobaciones.

También terminaron con salida 0: `validate_williams_chassis`, `validate_formula_tire_vertical_system`, `validate_rigid_contact_safety`, `validate_formula_tire_forces`, `validate_formula_tire_geometry` y `validate_formula_aerodynamics`.

Banco de garaje: **8.790 comprobaciones, 0 fallos**. Verifica campo ligado al setup trasero, valor inicial 285 kN/m, conversión a N/m, accesibilidad de todos los parámetros, ratón/teclado, dimensiones y ausencia de solapamientos de controles. Captura `garage-ui/tab-0.png` revisada visualmente. Es una instancia aislada de los widgets de producción; algunos valores de los otros campos son datos del banco y no representan el setup real del Williams. No sustituye una prueba de aplicación interactiva durante una sesión de conducción.

`git diff --check`: correcto.

## Distribución y evidencia

Respaldo y resultados: `D:/open-formula-migration-evidence/toe-heave-20261021/`. Incluye fuentes antes de editar, diff/estado inicial, candidato anterior completo, binarios anteriores, logs nativos, capturas y hashes instalados.

SHA-256 instalado, idéntico al binario compilado en ambos casos:

- simuv5.dll: `51d6770450e4d87d04256af7885745e16696b16b33a4bd8a87beefe46c1a8e3b`.
- legacymenu.dll: `f5215a9df79eb7af147c6cc0042f8fe5d3b0645b869f96cd6b79cfb371d010af`.
- XML Williams: `45677e97077f85116e5524c1df29b44e37e03c709cc405edc0773a49bfc2bd24`.

La estabilidad de entrada en curva, frenada, velocidad sostenida, pianos y rasantes requiere nueva conducción. Añadir el tercer muelle cambia soporte y reparto de precargas; las aprobaciones de conducción de setups anteriores no se trasladan automáticamente.
