# Música exclusiva del editor del garaje

21 de octubre de 2026. Implementada e instalada sobre los cambios locales de UI/3D existentes. Sin commit ni push.

## Comportamiento

La pista proporcionada por el usuario, C:/Users/bill/Downloads/07. Run.mp3, suena únicamente al abrir el editor GARAGE, durante cualquiera de sus pestañas. Cambiar de pestaña o redibujar no reinicia la reproducción.

Entrada con fade-in de 800 ms. Aplicar o cancelar vuelve a PIT con fade-out de 800 ms; salir a conducir también solicita ese fundido. La cola de salida termina de sonar durante la transición y luego queda en silencio. No se inicia en PIT, conducción ni otros menús. La reproducción se repite mientras el editor permanezca abierto.

Se respeta la opción global de música habilitada y su volumen/mute. El garaje declara «None» como música genérica de pantalla para impedir que arranque simultáneamente la pista general del menú.

Una reentrada rápida deja terminar el fundido saliente y después entra suavemente, sin cortar o reiniciar a mitad de su envolvente. La salida normal es asíncrona. Solo el cierre definitivo de sesión puede esperar un fundido corto de 150 ms antes de liberar recursos, con espera limitada a 250 ms.

## Implementación

- src/modules/userinterface/legacymenu/racescreens/formula_garage_music.cpp/.h: reproducción en un canal SDL_mixer independiente, fundidos, propiedad del canal y liberación segura. No usa el único canal Mix_Music, que el gestor general de pantallas puede detener abruptamente.
- src/modules/userinterface/legacymenu/racescreens/formula_garage_menu.cpp: arranque al abrir el editor; actualización del volumen durante sus frames; salida en ShowPit/BeginDriving y liberación en FormulaGarageShutdown.
- src/modules/userinterface/legacymenu/CMakeLists.txt: enlace SDL2_mixer, compilación del componente e instalación del MP3.
- implementation/audio/garage/run.mp3: original sin conversión, idéntico byte a byte. provenance.json registra origen, hash y formato.
- tools/validate_formula_garage_music.cpp: prueba con decodificador/mixer reales y dispositivo SDL dummy.
- tools/validate_formula_garage_ui.py: mantiene las regresiones de UI y modelo 3D con el componente nuevo enlazado.

La pista dura 273.097143 s, estéreo a 44100 Hz. Se decodifica como chunk completo al primer acceso de cada sesión y se reutiliza, consumiendo aproximadamente 48 MB de PCM a 16 bits estéreo. Esta decisión permite aislar sus fundidos de los cambios del reproductor general sin modificar ese subsistema. No se copian sus metadatos a los menús ni se cambia la música predeterminada global.

## Validación

Compilación Release de legacymenu correcta. Banco de música: 24 comprobaciones, cero fallos. Se verifica el MP3 original, producción real de PCM, subida y bajada gradual del volumen, parada al finalizar el fundido, ausencia de PCM posterior, reentrada rápida, persistencia entre actualizaciones/pestañas, mute, volumen no finito, carga fallida y conservación del mixer compartido al cerrar.

Banco nativo de UI/3D: 8468 comprobaciones, cero fallos. No se cambian los controles, el modelo 3D ni los parámetros físicos.

El banco musical se ejecutó con SDL_AUDIODRIVER=dummy; certifica decodificación, estados y salida PCM, pero no constituye escucha humana en el juego. Falta confirmar de oído la mezcla y el volumen durante entrada, edición, aplicación/cancelación y vuelta a conducción.

Fuentes del banco: tools/validate_formula_garage_music.cpp y formula_garage_music.cpp. Compilación UCRT64 con -std=c++11, -DTRACE_LEVEL=5, -DHAVE_CONFIG_H; incluir el directorio de build, src/libs/tgf, src/libs/portability, racescreens y SDL2; enlazar libtgf.dll.a, SDL2_mixer y SDL2. Ejecutar validate_formula_garage_music.exe seguido de la ruta del MP3.

## Instalación

- distribution/lib/games/speed-dreams-2/modules/userinterface/legacymenu.dll.
- distribution/share/games/speed-dreams-2/data/music/formula-garage/run.mp3.
- SHA-256 de la pista original y la instalada: 68038B826A7803290F5CF676F2BE1931670F0A602C2FA7D8B49CEC14A819B530.
- Los hashes de DLL/manifiesto actuales figuran en garage_music dentro de implementation/migration_status.json.

Se verificó que el juego estaba cerrado antes de instalar. Solo se actualizó la entrada del menú y se añadió el MP3 al manifiesto; la DLL de física, la gráfica y el XML del coche permanecen sin cambios.

Respaldo: D:/open-formula-migration-evidence/garage-music-20261021. Para revertir, con el juego cerrado restaurar legacymenu.before.dll y distribution_manifest.before.json. El archivo musical nuevo puede permanecer sin uso.
