# Corrección de posición del HUD

22 de octubre de 2026. Destino: D:/open-formula.

- Tacómetro, velocidad y gasolina quedan anclados abajo a la derecha; ruedas encima y temperaturas de agua/aceite en un bloque superior separado.
- La escala de la columna considera su altura completa, incluidos ambos espacios entre paneles.
- Se ocultan los grupos antiguos sustituidos y sus geometrías externas: los arcos del tacómetro legacy no dependían de la visibilidad de su grupo XML.
- El ocultamiento se mantiene durante las actualizaciones del HUD y se configura también en el empaquetado.
- Las barras de los pedales usan su borde inferior al convertir de coordenadas Godot a OSG, evitando cubrir las etiquetas.
- Se elimina el separador de la antigua ubicación de las temperaturas dentro del tacómetro.
- Se conserva Libron y la resolución guardada de 1920×1080, en ventana. No se modifica screen.xml: hash SHA-256 observado antes y después de la primera sesión E58D908EEBEFD02A2E495A36D4A03FE9FE89A3A522D96FE5491B4A9D6FBD018F.

Validación: compilación de osggraph e instalación en distribution; sesión instrumentada de Fuji y revisión visual de capturas a 1920×1080. Evidencia inicial en hud_position_fix/ y captura de la revisión final en hud_position_fix_final/. Estas pruebas verifican presentación e integración, no aceptación humana ni validación general de físicas.

Sin commit ni push. Se conservan los cambios previos de migración y dirección.
