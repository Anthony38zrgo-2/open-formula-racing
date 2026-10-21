# Imágenes de Motor y Estrategia

21 de octubre de 2026. Implementadas e instaladas sobre la UI actual. Sin commit ni push.

- Motor / Transmisión: la imagen proporcionada del motor y pistón sustituye al coche 3D en el espacio central, bajo los controles de diferencial.
- Estrategia / Boxes: la imagen proporcionada de los mecánicos ocupa una tarjeta central. Combustible y servicio quedan arriba a los lados; temperaturas delanteras y traseras, debajo. Todos los controles siguen disponibles.
- Suspensión y Aero / Ruedas conservan las vistas 3D superior y lateral del jugador.

Los originales se copiaron sin modificar bytes a implementation/ui/formula-garage/images/engine.png y strategy.jpg. provenance.json conserva procedencia y hashes. El PNG tiene transparencia alfa y el JPEG es RGB. Se dibujan completos, centrados y manteniendo proporciones; no se recortaron, recolorearon ni generaron variantes.

TabImage en formula_garage_menu.cpp carga los recursos una sola vez y los conserva en una caché independiente del texto. Las texturas se liberan al cerrar la sesión. El CMake del menú instala los recursos en data/img/formula-garage.

Validación: compilación Release de legacymenu correcta. Banco nativo de UI/3D con 8470 comprobaciones, cero fallos; incluye carga de ambas imágenes y acceso a todos los reglajes. Se revisaron visualmente las pestañas 2 y 3. El banco configura ahora GfLocalDir para leer la gamma del screen.xml mediante el cargador de texturas de producción.

Capturas del banco, no de una sesión completa: implementation/ui/formula-garage/previews/tab-images/tab-2.png y tab-3.png.

Se instaló la DLL de menú compilada con el juego cerrado y se registraron DLL/recursos en el manifiesto. No cambian físicas, música ni el módulo gráfico. Los hashes actuales están en garage_tab_images dentro de migration_status.json. Falta revisar estas pantallas dentro de la sesión real.

Respaldo de DLL, fuente y manifiesto anteriores: D:/open-formula-migration-evidence/garage-tab-images-20261021. Reversión con el juego cerrado: restaurar legacymenu.before.dll y distribution_manifest.before.json. Las imágenes nuevas pueden permanecer sin uso.
