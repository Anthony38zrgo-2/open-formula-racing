# Libron en toda la interfaz del juego

Fecha: 21 de octubre de 2026. Rama codex/initial-port; HEAD de partida 37f8058ef814e1864c59126f3c4fe38ebf0f29d5. Se preservaron los cambios previos de física, UI y reglajes.

Descargado Libron v0.31 de https://github.com/nicoverbruggen/libron/releases/download/v0.31/Libron.zip. Los cuatro archivos TrueType se mantienen sin modificar en implementation/fonts/Libron, con licencia SIL OFL 1.1 y procedencia/hashes. Autoría de Libron, Readerly y Newsreader añadida a CREDITS.md.

Menús y consola usan Libron Bold; textos y elementos normales del HUD usan Libron Regular. Los elementos del HUD antes en negrita mantienen ese peso. Se conservan tamaños, colores y posiciones. Se actualizaron las tres familias de screen.xml y las 110 referencias del HUD, incluidas las variantes por idioma, en configuración de datos, ajustes iniciales y ajustes de usuario instalados.

Los valores predeterminados de guifont.cpp y el texto inicial de OsgHUD.cpp también usan Libron. tools/configure_game_fonts.py instala los archivos y actualiza exclusivamente la selección de fuentes. prepare_practice_session.py y assemble_portable_distribution.py invocan esa configuración para conservarla al regenerar la distribución. No se modificó el submódulo de datos upstream ni las fuentes de herramientas de desarrollo ajenas al juego.

Validación: compilación correcta de tgfclient y osggraph; bibliotecas instaladas. SDL_ttf abrió cuatro variantes en seis tamaños (24 combinaciones), verificó los caracteres españoles y midió texto UTF-8 correctamente. Verificadas 452 referencias de fuentes entre cuatro conjuntos de configuración. El arranque canónico distribution/OpenFormula.exe llegó a RACE sin errores de carga de fuentes. La sesión de prueba fue cerrada. No se realizó revisión visual completa de todos los menús o idiomas; Libron estándar no incorpora todos los alfabetos.

Manifiesto de distribución actualizado para las fuentes, configuraciones y bibliotecas instaladas. Evidencia y respaldos en D:/open-formula-migration-evidence/libron-20261021. Sin commit ni push.
