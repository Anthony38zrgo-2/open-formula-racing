# Reglajes del garaje en SQLite

Implementado el 22 de octubre de 2026 en D:/open-formula.

## Comportamiento

- REGLAJES es la quinta y ultima pestaña del garaje.
- Una sola lista del circuito actual: nombre, equipo y ultima modificacion.
- Crear nuevo reglaje pide un nombre y conserva los valores actuales del editor.
- Guardar actualiza el registro seleccionado. Cargar recupera el registro completo en el editor; Aplicar y volver sigue confirmando el cambio en el coche.
- Eliminar requiere confirmacion. El borrado conserva los ajustes que ya estan en el editor.
- Navegacion con raton, rueda, flechas y mando. La lista desplaza las filas que exceden la ventana visible.
- Los cambios sin guardar se protegen al cargar otro reglaje o cancelar la edicion.
- La interfaz no muestra archivos, rutas, JSON ni opciones de importacion/exportacion.

## Datos

Base por usuario: `distribution/user-settings/open_formula.db`.

| Tabla | Contenido inicial |
| --- | --- |
| circuit | Melbourne 1997, Australia, identificador interno y longitud nativa del port de 5299.673 m |
| team | Williams e identificador interno |
| car_setup | Sin reglajes de ejemplo; se completa al crear reglajes en el juego |

`car_setup` conserva ID, nombre, circuito, equipo, coche, motor fisico, huella SHA-256 del perfil base, version del formato, valores editables completos y fechas UTC de creacion/modificacion. La interfaz presenta la fecha en hora local.

Los valores se almacenan internamente como un objeto de 33 parametros, validado contra el catalogo editable de Rust. No se guarda el documento completo del coche ni se permite cargar parametros fijos. Cambios de coche, circuito, motor o perfil incompatible se rechazan antes de modificar el editor.

SQLite usa sentencias preparadas, claves foraneas, escrituras atomicas y version de esquema 1. Un esquema posterior se rechaza sin modificarlo. La primera inicializacion de tablas e indices se ejecuta en una transaccion. No se activa el subsistema opcional de replay de Speed Dreams para esta funcionalidad.

Solo Melbourne esta indexado en esta primera version. Los registros de circuitos/equipos adicionales requieren incorporacion explicita al catalogo. No habia archivos de reglaje anteriores en `distribution/user-settings/setups`; no se elimino ni migro contenido del usuario.

## Archivos principales

- `formula_setup_database.h/.cpp`: catalogos, esquema y operaciones CRUD.
- `formula_setup_store.h/.cpp`: catalogo editable y validacion compartida; las funciones anteriores de archivo quedan fuera del flujo del garaje.
- `formula_garage_menu.cpp`: lista, acciones, confirmaciones, fechas y desplazamiento.
- `legacymenu/CMakeLists.txt`: dependencia e instalacion de SQLite, y banco de comprobacion.
- `reference.html`: referencia visual actualizada; su demostracion de navegador es independiente de la persistencia SQLite nativa.

## Verificacion

- Compilacion de legacymenu y del banco SQLite correcta.
- 35 comprobaciones de base de datos sin fallos: CRUD, reinicio, fechas, nombres duplicados, claves foraneas, aislamiento por circuito, incompatibilidades, campos protegidos, entradas SQL como texto y version futura.
- Aplicacion y edicion del reglaje recuperado en la DLL Rust real correctas; parametros fijos y archivo fuente del vehiculo permanecen sin cambios.
- 9285 comprobaciones del banco grafico del garaje sin fallos, incluyendo creacion, carga, guardado antes de cargar otro registro, borrado confirmado/cancelado, desplazamiento y orden de pestañas.
- Revisada visualmente la captura `garage_reglajes_sqlite/tab-rust-4.png`. Es una captura del banco grafico con datos de prueba, no una sesion de conduccion.
- Base instalada verificada con `integrity_check=ok`, sin errores de claves foraneas, un circuito, un equipo y cero reglajes de prueba.
- Los modulos instalados legacymenu y rustphysics cargan con su dependencia SQLite. DLLs instaladas contrastadas con los artefactos compilados.

El juego que ya estaba abierto no se interrumpio. Es necesario reiniciarlo para utilizar los modulos nuevos. La comprobacion interactiva dentro de una partida queda pendiente.
