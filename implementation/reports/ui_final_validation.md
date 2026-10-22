# Estado final de HUD y garaje

Incluye paleta grafito/dorado y verde #8FD36B, acelerador y temperaturas óptimas con el mismo verde, bandas de pilotos verdes con texto oscuro, fondos al 95%, esquinas redondeadas uniformes, margen inferior común de 48 px y tacómetro con relleno lateral y numeración separada del arco.

Neumáticos: panel 330×230 px, zonas térmicas independientes, temperatura máxima, presión, frenos y goma restante; desgaste blanco por encima del 50%, amarillo al 50% y rojo al 25%. La física no cambia.

Garaje y PIT: botones amplios EMPEZAR / IR AL GARAJE; editor con Libron Regular/Bold y ajuste de anchura de títulos. Vista del coche en suspensión situada en y=317, punto intermedio entre las posiciones anteriores 287 y 346; las demás pestañas mantienen su disposición.

Validación final del renderer y controles: 8971 comprobaciones, 0 fallos, con descriptores de la DLL Rust y geometría real del coche. Capturas finales en garage_ui_final/tab-rust-0.png a tab-rust-3.png y tab-pit.png. Las capturas de carrera de hud_green_rounded_final/frame-005.png y hud_green_rounded/frame-060.png verifican HUD y acelerador activo. No se afirma validación manual completa de aplicar/cancelar y volver a pista.

osggraph y legacymenu compilados e instalados. Resolución conservada en 1920×1080; screen.xml mantiene SHA-256 E58D908EEBEFD02A2E495A36D4A03FE9FE89A3A522D96FE5491B4A9D6FBD018F. Los ejecutables, BMP y logs intermedios de pruebas permanecen locales.
