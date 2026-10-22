# Minimapa y Air/Time

22 de octubre de 2026.

Minimapa trasladado al centro izquierdo, combinado con temperatura del aire y hora de carrera en un único panel. Fondo exacto del resto del HUD: palette().background, #06152566. Libron Regular/Bold. Panel colocado por encima de las ruedas en viewports cortos para evitar solapamientos.

La lectura de aire usa el mismo estado publicado del coche que la UI anterior. La hora conserva la suma de hora inicial del escenario y tiempo de carrera, formateada HH:MM:SS. El bloque legacy trackdataWidget queda oculto para evitar duplicados.

Compilación de osggraph correcta e instalación en distribution. Captura del juego revisada visualmente: hud_map_air_time/frame-120.png. Resolución conservada en 1920×1080; screen.xml no se modifica. Sin commit ni push.

## Ajuste posterior: esquina superior derecha

Por petición posterior del usuario, el bloque completo pasa a la esquina superior derecha. Margen superior y derecho: 32 px a 1920×1080, escalados con el HUD y con un mínimo de 24 px. Conserva fondo, contenido y Libron. Evidencia actualizada: hud_map_top_right/frame-030.png.
