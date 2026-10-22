# Verde vivo, esquinas y márgenes del tacómetro

Verde #8FD36B para estado térmico óptimo y acelerador (barra/porcentaje). Las bandas de pilotos usan el mismo verde con nombres en grafito para conservar contraste. Un shader local de las tarjetas conserva la pestaña blanca de posición y el contorno alfa, sustituyendo solo la zona de color; no se modifican las imágenes originales. La zona verde compensa el alfa 128 del recurso anterior para que su tono no se diluya sobre el cielo.

Neumáticos, mapa y tacómetro utilizan el mismo radio efectivo de esquinas que volante/pedales, derivado de su escala. Se conservan fondo al 95% y margen inferior común de 48 px a 1080p.

Tacómetro: ancho del panel ampliado 48 px manteniendo la escala del contenido. Esto deja aproximadamente 24 px de relleno lateral sin acercarlo al borde de pantalla. Las temperaturas tienen además margen derecho interior; las etiquetas del arco se alejan 26 unidades de diseño, aproximadamente 9 px a 1080p.

Compilación e instalación de osggraph correctas. Captura de la revisión final: hud_green_rounded_final/frame-005.png; la sesión anterior hud_green_rounded/frame-060.png muestra también el acelerador activo. Resolución 1920×1080 conservada. Sin commit ni push.
