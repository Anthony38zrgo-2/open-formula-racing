# Pantalla de carga

`pit-lane-background.png` es la imagen suministrada por el usuario el 20 de octubre de 2026 para el arranque y la carga de Open Formula. La versión vigente es la imagen editada JPG suministrada posteriormente por el usuario. Se convierte a PNG para el cargador existente, sin reescalar, recortar ni retocar sus píxeles decodificados. No se ha verificado una autoría o licencia externa adicional.

La imagen de barra adjunta se utiliza únicamente como referencia visual. La barra se dibuja mediante geometría OpenGL, sin copiar la imagen de stock ni su marca de agua.

`loading_presentation.cpp` mantiene el mismo fondo, barra centrada y una línea de estado durante el arranque y la carga de carrera. La barra avanza por etapas de carga; no representa un porcentaje de bytes ni una estimación temporal.
