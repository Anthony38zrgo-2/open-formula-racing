# Paleta del garaje y legibilidad del HUD

Se aplica la paleta del garaje: grafito #1E2429 (88% opacidad), blanco #E9ECE8, azul grisaceo #607E91, dorado #E3B23C y gris secundario #A8B5BA. Los estados normales son blancos; se conservan azul para frio y rojo para frenos y alertas termicas.

Tacometro ampliado 15%, controles 10%, panel de ruedas aumentado en altura manteniendo su ancho. Mayor separacion de filas, banda de rodadura y frenos destacados; carcasa/gas/llanta secundarios. Agua y aceite compactados en dos filas. Bordes finos y margenes exteriores de 32 px a 1080p. Minimapa/Air/Time conserva la esquina superior derecha y su tamano externo, con mayor area interna para el trazado. Libron se conserva en el HUD.

La ventana PIT conserva las acciones originales y ahora usa grafito con borde azul grisaceo, acento dorado y botones amplios EMPEZAR / IR AL GARAJE. No cambia el editor de reglajes ni su conexion con Rust.

Validacion: osggraph y legacymenu compilados e instalados; captura de carrera a 1920x1080 revisada en hud_garage_palette/frame-120.png. Banco de renderizado de garaje y PIT: 8971 comprobaciones, 0 fallos; captura garage_palette_validation/tab-pit.png. No se afirma validacion manual completa del flujo de boxes. screen.xml conserva SHA-256 E58D908EEBEFD02A2E495A36D4A03FE9FE89A3A522D96FE5491B4A9D6FBD018F. Sin commit ni push.
