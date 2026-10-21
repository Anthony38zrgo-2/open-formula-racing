# Garaje nativo basado en el HTML de referencia

Seguimiento: [modelo 3D del jugador](garage_player_3d.md), que reemplaza el esquema central y documenta las DLL instaladas actualmente. Este informe conserva los resultados de la primera fase de UI.

21 de octubre de 2026. Física anterior guardada primero en el commit 4212abf. Los cambios posteriores de UI se dejan sin commit; no hubo push.

## Implementación

Referencia proporcionada por el usuario: C:/Users/bill/Downloads/open_formula_garage_4_colores_v3.html. Copia conservada en implementation/ui/formula-garage/reference.html. Se usó como referencia visual; sus valores y cálculos de demostración no constituyen datos de simulación.

Se reemplazó el editor de reglajes de Formula Garage por una interfaz nativa OpenGL con la paleta grafito #171B1F, amarillo #E3B23C, acero #607E91 y humo #E9ECE8. Incluye cabecera con degradado, cuatro pestañas, tarjetas por esquina, barras horizontales con botones -/+, datos del coche y acciones inferiores.

- Suspensión: topes, cuatro amortiguaciones, muelles y altura por rueda; barras y dirección en el centro.
- Aero / Ruedas: presión fría, caída y convergencia por rueda; alas y frenada en el centro; temperatura actual de cada banda.
- Motor / Transmisión: limitador, relaciones de cambio, grupo final, diferencial, dirección y presión de freno.
- Estrategia / Boxes: combustible y sustitución de neumáticos conectados al servicio existente; estado actual del coche.

Se mantienen todos los campos que tenía el editor, junto con simetría independiente por eje, controles de ratón/teclado/mando, cancelación y aplicación. Restablecer recupera el reglaje leído al abrir el editor. La simetría solo copia el mismo parámetro a la rueda contraria, aunque ahora varias familias de parámetros se muestran en una pestaña.

La referencia incluye planificación de paradas, compuestos, refrigeración, conductos y curvas de desgaste de demostración. No se inventaron nuevos parámetros físicos ni predicciones para esos elementos. La pestaña de boxes muestra únicamente los servicios y datos que ya están conectados al coche. Importación/exportación y almacenamiento local del HTML tampoco se trasladaron como si fueran persistencia de reglajes del juego.

El alcance sigue siendo Formula Garage, disponible en la práctica individual de Fuji que ya lo utilizaba. El menú genérico de otros vehículos/carreras no se reemplazó. El esquema central del monoplaza es ilustrativo.

## Archivos

- src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp: presentación, controles, agrupación, escalado del canvas y recursos de fuentes locales.
- src/modules/userinterface/legacymenu/CMakeLists.txt: enlace explícito con SDL2_ttf e instalación de fuentes.
- implementation/ui/formula-garage/fonts/: Chakra Petch Bold y Rajdhani SemiBold, con licencias SIL OFL y provenance.json con fuentes y SHA-256. Se cargan sin conexión y solo afectan a este editor.
- tools/validate_formula_garage_ui.py: banco aislado que extrae las funciones de producción, las renderiza en un contexto SDL/OpenGL oculto y prueba los controles con datos de vehículo de prueba. Sustituye las acciones del motor de carrera por contadores; no simula una sesión completa.
- implementation/ui/formula-garage/previews/: cuatro capturas generadas por ese banco, no capturas de una sesión de conducción.
- implementation/migration_status.json: procedencia, validación e instalación.

## Validación e instalación

Compilación Release de legacymenu correcta con CMake/Ninja/UCRT64. Banco aislado: 8445 comprobaciones, 0 fallos. Incluye accesibilidad de todos los reglajes anteriores, límites y ausencia de solapamiento entre zonas de interacción, extremos de los deslizadores, aislamiento de la simetría, acciones de pestaña, teclado, restauración, fuentes disponibles, caché de texto acotada y liberación de texturas. La comprobación de OpenGL termina sin errores.

El ratón se verifica con letterboxing y las dimensiones 1920x1080, 1280x720 y 1024x768. Se revisaron visualmente las cuatro capturas a 1280x800. El mando conserva el mapeo existente a las mismas acciones de teclado; no se hizo una prueba física con un mando conectado.

La DLL compilada se copió a distribution/lib/games/speed-dreams-2/modules/userinterface/legacymenu.dll con el juego cerrado. Fuentes y licencias están instaladas en distribution/share/games/speed-dreams-2/data/fonts/formula-garage. El manifiesto registra únicamente el cambio del módulo del menú y esos nuevos recursos; los hashes actuales están en garage_html_ui dentro de migration_status.json.

La DLL de física y el XML del vehículo permanecen idénticos a los de la corrección anterior. Respaldo del menú y manifiesto anteriores: D:/open-formula-migration-evidence/garage-html-ui-20261021. Los registros y ejecutable del banco están en su subdirectorio validation.

Para revertir el menú, con el juego cerrado restaurar legacymenu.before.dll y distribution_manifest.before.json desde ese respaldo. Las fuentes nuevas pueden permanecer como archivos sin uso; no sustituyen fuentes globales.

Pendiente: abrir el garaje dentro del juego, cambiar y aplicar un reglaje, volver a PIT, comenzar a conducir y regresar al garaje. Esta puerta de integración no se considera cerrada por el banco aislado. No se abrió ni controló una nueva sesión gráfica del usuario.
