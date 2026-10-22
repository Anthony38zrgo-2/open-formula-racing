# Juego reconstruido e instalado para prueba

Fecha: 22 de octubre de 2026. Rama `codex/initial-port`, HEAD `9c9164869c8210a558abb5e9aa5e760e5eacf5dd`, con los cambios locales de audio incluidos.

- Build completo Release/UCRT64 terminado: 237/237 acciones correctas. Se reconstruyeron los consumidores afectados por el nuevo tamaño de `tCarElt`.
- Instalación CMake completada en `D:/open-formula/distribution`.
- 36 binarios instalados comparados por SHA-256 con sus artefactos de build; todos coinciden.
- Dependencias de 100 binarios inspeccionadas: ninguna ausente.
- Bancos y mezcla instalados en `distribution/share/games/speed-dreams-2/sounds`: 18 WAV V10, 12 commons y cuatro documentos/configuraciones, 34 archivos verificados.
- Prueba de la DLL instalada de audio correcta: ABI 1, variante `OFP-V10-3500`, V10 activo, mínimo efectivo 3500, render estéreo/mono finito, audible y sin clipping.
- DLL física instalada correcta: ABI 6 y 22 símbolos requeridos presentes.
- Carga de módulos instalados verificada: `snddefault`, `rustphysics`, `osggraph`, `standardgame` y `legacymenu`.
- Ejecutable instalado responde a `--help`; devuelve 1 como salida de ayuda del motor, sin iniciar carrera.
- 508 configuraciones protegidas conservan sus hashes anteriores a la instalación; no se regeneraron controles, reglajes ni sesión.
- `distribution_manifest.json` actualizado: 5460 archivos, sin incluir la configuración mutable del usuario ni el propio manifest.

## Cómo probar

Abrir `D:/open-formula/distribution/OpenFormula.exe`. El launcher selecciona `williams-fuji-practice`, Williams FW19/Villeneuve en Fuji76-77; la configuración activa usa `rustphysics`, `osggraph`, `snddefault` y OpenAL. Ralentí físico y acústico: 3500 RPM.

No se ha iniciado una sesión interactiva ni validado escucha humana en esta reconstrucción. La prueba de conducción, balance sonoro, cockpit/exterior y cortes del motor queda para el usuario. Los resultados y hashes de instalación están en `audio_rebuild_ready.json`; el log de instalación está en `audio_rebuild_install.log`.
