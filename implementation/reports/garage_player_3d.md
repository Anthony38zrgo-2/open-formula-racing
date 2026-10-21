# Modelo 3D del jugador dentro del garaje

21 de octubre de 2026. Implementado sobre los cambios locales de UI existentes. Sin commit ni push.

## Resultado

El dibujo esquemático se sustituye por la geometría del coche del jugador que osggraph ya tiene cargada, incluidas las ruedas y sus transformaciones. La carrocería se muestra en gris neutro mate, sin texturas, colores de vértice, decoraciones o materiales originales. El piloto se excluye y el habitáculo queda vacío.

- Suspensión: proyección ortográfica superior, morro hacia arriba.
- Aero / Ruedas: proyección ortográfica lateral, coche horizontal.
- Motor / Transmisión: misma vista superior.
- Estrategia / Boxes: conserva sus datos y controles de servicio.

El sombreado suave usa iluminación difusa con ambiente y un reflejo especular tenue del pipeline OpenGL de compatibilidad. No requiere otro programa GLSL. Es un render 3D del modelo real, no una imagen generada ni un SVG.

## Implementación y aislamiento

- src/interfaces/icarpreview.h: capacidad opcional ICarPreviewRenderer. No se modifica la tabla virtual de IGraphicsEngine ni se exige el cambio a todos los motores gráficos.
- src/modules/graphic/osggraph/Car/OsgGaragePreview.cpp/.h: captura de triángulos en coordenadas locales y render gris. Excluye la transformación mundial del coche, conserva transformaciones internas y selecciona la rama exterior del switch COCK sin modificarlo. Omite el switch DRIVER y respeta los restantes hijos activos.
- src/modules/graphic/osggraph/OsgGraph.cpp/.h: conexión con el SDCar correspondiente al jugador. La copia de geometría se reutiliza durante la visita; se invalida al abrir de nuevo el editor y antes de descargar los coches.
- src/modules/graphic/osggraph/CMakeLists.txt: compila el componente nuevo.
- src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp: reemplaza CarDiagram por la vista 3D, convierte el rectángulo del panel a píxeles del viewport actual y elige la orientación según la pestaña.
- tools/validate_formula_garage_ui.py: amplía el banco de UI para cargar los archivos ACC reales con el loader de producción y ejecutar el renderer nuevo.

La copia contiene posiciones y normales calculadas por triángulo; no conserva nodos vivos, texturas, callbacks, shaders o materiales del coche. El dibujo guarda y restaura programa activo, unidades de textura, atributos de iluminación, profundidad, recorte, viewport y matrices. Solo limpia profundidad dentro del rectángulo de la vista; conserva el fondo del panel.

No se actualizan físicas desde la vista ni se animan los reglajes todavía sin aplicar. Se muestra la geometría disponible al abrir el editor. Las imágenes generadas anteriormente se conservan como candidatos y no intervienen en este render.

El motor configurado en esta instalación es osggraph. Otros motores sin esta capacidad muestran «Vista 3D no disponible»; no se cargan modelos de sustitución silenciosos.

## Validación

Compilación Release de osggraph y legacymenu correcta. Banco aislado: 8468 comprobaciones, cero fallos.

Se cargaron la carrocería del Williams y sus cuatro modelos de rueda mediante osgLoader/ReaderWriterACC de producción. La escena de prueba contiene 91155 triángulos válidos, con dimensiones 4.4443 × 1.9891 × 1.1793 m. La colocación de ruedas de este banco se reconstruye desde el XML; la ruta del juego toma directamente las transformaciones del SDCar cargado.

Pruebas nuevas:

- Captura de geometría real y exclusión de la posición mundial.
- Exclusión de DRIVER y selección del exterior aunque la cámara de carrera tenga otro cockpit activo; switch original sin cambios.
- Modelo y materiales originales sin modificaciones.
- Pestañas con orientación superior/lateral correcta.
- Restauración de viewport, proyección, profundidad, recorte y estado multitextura.
- Rectángulo de tamaño cero y escena de carrera ausente rechazados sin dibujar.
- Carga de la DLL osggraph compilada, conversión a la interfaz opcional entre DLL y descarga correctas.
- Regresiones existentes de controles, simetría, accesibilidad de todos los reglajes, ratón escalado y recursos de texto.

Se revisaron las capturas superiores y laterales. Están en implementation/ui/formula-garage/previews/player-3d/. Son renders del banco nativo con recursos reales, no capturas de una sesión completa.

Comando reproducible:

    C:/Users/bill/AppData/Local/Programs/Python/Python314/python.exe tools/validate_formula_garage_ui.py --output D:/open-formula-migration-evidence/garage-player-3d-20261021/validation

Requiere los objetos del loader en el directorio de compilación y Pillow en ese Python para guardar las capturas PNG.

## Instalación y límites

Con el juego cerrado se instalaron osggraph.dll y legacymenu.dll compiladas en sus rutas de distribution/lib/games/speed-dreams-2/modules/. Solo se actualizaron esas dos entradas del manifiesto. Los hashes actuales figuran en garage_player_3d dentro de migration_status.json.

La DLL simuv5 y el XML del Williams permanecen idénticos a los de la reparación de física anterior. No se modificaron materiales, mallas, texturas, parámetros físicos ni cámara de conducción.

Respaldo: D:/open-formula-migration-evidence/garage-player-3d-20261021. Para revertir esta fase, con el juego cerrado restaurar ambas DLL before y distribution_manifest.before.json desde ese directorio.

Queda pendiente abrir el garaje en una sesión real, recorrer las pestañas, aplicar un reglaje, volver a conducir y regresar al garaje. El banco valida el render, recursos y enlace entre módulos, pero no cierra esa prueba completa ni acredita rendimiento sostenido dentro de la sesión.
