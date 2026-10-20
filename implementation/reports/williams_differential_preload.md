# Williams: precarga y bloqueo del diferencial

Solicitud: precarga de 10 Nm, coeficiente de bloqueo en aceleración 0,30 y en retención 0,70.

Estado anterior: sin precarga independiente, umbral de activación de 90 Nm, aceleración 0,30 y retención 0,20. El generador confundía la precarga del perfil de origen con el umbral de activación del modelo nativo.

Se conserva el umbral de activación en 90 Nm, se separa `preload torque=10 N.m` y se configura `coast max slip bias=0.7`. Los coeficientes son ajustes normalizados de la ley nativa de reparto; no garantizan un porcentaje de igualdad entre velocidades. No existe equivalencia directa con porcentajes o rampas de otros simuladores.

La precarga transfiere hasta 10 Nm entre semiejes, con pares iguales y opuestos. Se limita al par necesario para sincronizar las velocidades previstas en el paso de integración, evitando sobrepasar la sincronización. Está disponible para los modelos 1.5 WAY y electrónico, y vale cero por defecto en los demás vehículos. La corrección de signo en retención del Williams se conserva.

Archivos: `src/modules/simu/simuv5/differential.cpp`, `differential.h`, `tools/prepare_williams_vehicle.py`, `tools/validate_formula_differential.cpp`, configuración candidata y copia distribuida del Williams. Se recompiló e instaló `simuv5.dll`.

Validación nativa: PASS para precarga con entrada de par cero, simetría al invertir la diferencia de velocidades, sincronización sin sobrepaso, ausencia de transferencia con velocidades iguales, compatibilidad con precarga cero, aceleración conservada salvo la precarga añadida y aumento de retención de 0,20 a 0,70. Con inercias unitarias, diferencia inicial 10 rad/s y paso de 0,002 s, la precarga reduce la diferencia a aproximadamente 9,96 rad/s.

La conducción y la estabilidad bajo frenada requieren prueba en pista; la validación nativa no constituye aceptación de comportamiento del coche completo.

Arranque: la sesión Williams/Fuji cargó con la biblioteca instalada. SHA-256 de simuv5.dll compilada e instalada: ACC93892B0396C11573CBC09DA7F4F106FACF8EE5D944587D461D1801CBF8F4E. El registro conserva avisos OSGHUD sobre transparent.png; no se realizó validación visual ni conducción. Evidencia: D:/open-formula-migration-evidence/differential-preload-20261020/.
