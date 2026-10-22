# Siluetas grises y centrado de los coches

21 de octubre de 2026. Sobre el commit a68ee9e; sin commit ni push nuevos.

Se editaron las dos imágenes mediante la herramienta integrada image_gen, con fondo transparente y formas simplificadas en gris neutro. El motor conserva bloque, admisión, escapes y pistón separado. Boxes conserva dos mecánicos con casco, manguera y boquilla de repostaje para que la acción siga siendo reconocible. Los originales permanecen intactos.

Recursos finales:

- implementation/ui/formula-garage/images/engine-gray-v2.png.
- implementation/ui/formula-garage/images/strategy-gray-v2.png.
- Prompts completos: images/gray-silhouettes-v2.md.
- Procedencia y hashes: images/provenance.json.

En formula_garage_menu.cpp se sustituyen las referencias visuales de Motor y Estrategia. TabImage carga los PNG conservando sus colores de UI, sin aplicar la conversión de gamma de fotografías/escena que oscurecía sus grises. Se mantiene alfa y proporción sin modificar los PNG generados.

En Suspensión y Aero / Ruedas, el centro vertical de la vista del jugador pasa de y=287 a y=315 en el canvas de 1280x800: sube 28 unidades y queda centrado en el espacio libre entre los controles superiores e inferiores. No cambia cámara, geometría ni material del coche; solo su rectángulo en la UI.

Compilación Release de legacymenu correcta. Banco de UI/3D: 8470 comprobaciones, cero fallos. Ambas imágenes RGBA contienen transparencia real. Se revisaron las cuatro capturas del banco, incluidas las dos vistas del coche y las siluetas finales con el gris preservado. Capturas en implementation/ui/formula-garage/previews/gray-silhouettes/.

Se instalaron la DLL y los PNG con el juego cerrado y se actualizó el manifiesto. Los hashes de esta instalación constan en garage_gray_silhouettes dentro de migration_status.json. No cambian música, reglajes físicos ni módulo gráfico. Falta la revisión visual dentro de una sesión real.

Respaldo anterior: D:/open-formula-migration-evidence/garage-gray-silhouettes-20261021. Para revertir con el juego cerrado, restaurar legacymenu.before.dll y distribution_manifest.before.json. Los nuevos PNG pueden permanecer como candidatos sin uso.
