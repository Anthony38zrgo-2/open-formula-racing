# Williams: corrección de apariencia de fibra de carbono

Fecha: 20 de octubre de 2026. Rama codex/initial-port; base 02d93a2a0422c7efd3851ec387cbf24951d7cb9f. Se preservan las modificaciones documentales previas de instrucciones.md y el handoff archivado. Respaldo y evidencia en D:/open-formula-migration-evidence/carbon-appearance-20261020/.

## Causa confirmada

El shader car.frag de Speed Dreams lee el mapa difuso sin multiplicar el color del material ACC. Por eso el factor oscuro 0,25 del material de carbono restaurado no llegaba al color final. Además, OsgShader establece un especular global de 0,8 y smoothness 300 para todo el coche; si se activa el modo de reflexiones, mezcla un 30% de entorno por igual para todos los materiales. El problema observado no era una textura ausente ni un nuevo fallo de UV.

La textura de carbono usada por los materiales BlenderKit también tenía un factor implícito blanco y perdía sus propiedades PBR al exportarse a ACC. Normal/roughness/clearcoat del GLB no se reproducen íntegramente en el shader Phong del port. Esta corrección es una adaptación de apariencia, no un renderer PBR completo.

## Implementación

Se incorpora implementation/shaders/car_material_appearance.frag como shader opcional seleccionado mediante Graphic Objects/material appearance shader. Los vehículos que no lo seleccionan siguen utilizando car.frag sin modificar; no se editó el submódulo speed-dreams-data.

El exportador puede asignar apariencia explícita a materiales. Solo tres materiales del Williams reciben nombres formula_carbon_ y parámetros propios: fw19_cf.tGA:fw19_cf.tga, UniformBlenderKitCarbonFiber.006 y UniformBlenderKitCarbonFiber.004. Valores de calibración: factor difuso 0,25, especular 0,04 y shininess 32. El loader ACC publica esos valores como uniforms por material. La variante de shader los aplica solo a esos materiales; en modo de reflexión, su mezcla de entorno se limita a 0,03. Los materiales pintados conservan la fórmula anterior.

Se conservan textura original, canales UV, transformación de geometría, normales generadas y posiciones. La apariencia se persiste en tools/prepare_williams_vehicle.py y el shader se copia mediante tools/assemble_portable_distribution.py. No se oscureció ni editó ninguna imagen fuente.

## Comprobaciones

- Compilación del target osggraph: PASS.
- Validador de materiales: 54 primitivas, 4 grupos con canal UV secundario, 8 grupos de carbono restaurado, 3 materiales de apariencia y todos los archivos de textura presentes: PASS.
- Comparación con respaldo: todo el contenido OBJECT world y posterior del ACC permanece idéntico; solo cambian las líneas de material de índices 2, 10 y 11: PASS.
- Todas las secciones físicas del XML permanecen idénticas; solo se añade el selector del shader en Graphic Objects: PASS.
- DLL compilada e instalada con SHA-256 idéntico: 7436289711436A4F4ECD7B87E457734D4D281D1E5B806FEC21CB3257235C85BE.
- Arranque Williams/Fuji e inspección visual antes/después de cabina: el volante y los elementos de carbono visibles de suspensión dejan de verse plateados y conservan el tejido oscuro. La pintura mantiene su apariencia. No aparecen errores de compilación de shader en el registro inspeccionado.

La inspección visual fue realizada con Computer Use en una sesión iniciada para la prueba, que se cerró después. No equivale a aprobación estética del usuario ni a validación de todas las cámaras/horas/modos de reflexión. Persisten avisos previos del HUD y combinación de parámetros ajenos a este cambio.

Archivos: tools/convert_binary_scene.py; tools/prepare_williams_vehicle.py; tools/validate_vehicle_material_export.py; tools/assemble_portable_distribution.py; src/modules/graphic/osggraph/Loader/ReaderWriterACC.h; src/modules/graphic/osggraph/Render/OsgShader.cpp; implementation/shaders/car_material_appearance.frag; este informe y salidas generadas ignoradas por Git. No se modificaron física, neumáticos, aero ni el documento de implementación del neumático.
