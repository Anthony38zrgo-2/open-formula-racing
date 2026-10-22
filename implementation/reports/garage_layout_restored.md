# Recuperación del diseño del garaje

Se confirmó que el cambio de presentación fue introducido en 7f55ac6: la ruta Rust retornaba antes de dibujar los paneles originales y presentaba una tabla genérica de dos columnas. No se habían perdido los recursos gráficos.

Se recupera la composición anterior de cuatro pestañas: suspensión y aero/ruedas con cuatro paneles por esquina y vista central del coche; motor/transmisión con ilustración del motor; estrategia/boxes con su dibujo original. Se conservan colores, tipografía, cabecera, bordes y botones. La vista superior vuelve a su altura anterior de 270 unidades.

La presentación sigue leyendo los descriptores reales de Rust y aplicando sus transacciones existentes. Los parámetros compartidos por eje se identifican expresamente; no se añaden falsos reglajes independientes ni un tercer muelle. Las relaciones individuales no publicadas como editables quedan de solo lectura; los datos inexistentes se muestran como --. Se mantiene el servicio de neumáticos por la ruta ResetPhysics existente.

Archivos: formula_garage_menu.cpp y tools/validate_formula_garage_ui.py. No se modifican recursos originales, físicas ni HUD de carrera. Resolución conservada: 1920×1080, con el mismo hash de screen.xml.

Validación: legacymenu compilado e instalado; DLL instalada y compilada con hash 79B65F997854B171D95522A1463E151F518671314A0C35A92AFDD1AEA6DEDBF6. El banco SDL/OpenGL oculto usa widgets de producción, geometría real del coche y descriptores leídos de la DLL Rust. Resultado: 8968 comprobaciones, 0 fallos. Incluye límites de controles y disponibilidad de los dibujos de motor y estrategia. Capturas tab-rust-0.png a tab-rust-3.png revisadas visualmente; tras un intérprete sin Pillow en la segunda ejecución se exportaron los BMP con el intérprete Python que sí lo incluye.

Límite: el banco comprueba renderizado y controles; no sustituye la interacción completa de aplicar/cancelar y volver a pista dentro del juego. Sin commit ni push.
