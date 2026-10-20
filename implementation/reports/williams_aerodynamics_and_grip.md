# Williams: distribución aerodinámica y agarre mecánico

Fecha: 20 de octubre de 2026. Rama codex/initial-port; HEAD 57ddca9375bc18136f319234f53a930db42a6563. Se preservó el checkout previamente modificado; respaldo y registros en D:/open-formula-migration-evidence/aerodynamics-grip-20261020/.

## Alcance y condición de referencia

Se solicita 40% fondo y 60% alas, con menor dependencia del suelo, y revisar el agarre mecánico. El porcentaje se define con flujo horizontal, sin derrape ni balanceo, a eficiencia máxima del fondo y antes de limitar la carga total. No es un reparto constante durante una vuelta: alturas, incidencia y orientación alteran las contribuciones.

El fondo pasa de Cl*A=1,5949 a 1,1270866 m²; las alas juntas pasan de 1,2228165 a 1,6906299 m². Se conserva el total de referencia de 2,8177165 m². El aporte máximo del fondo pasa de aproximadamente 56,6% a 40%. La carga trasladada a las alas se distribuye según sus posiciones y la posición del fondo, conservando el centro de presión longitudinal en la referencia. Se conservan áreas geométricas, posiciones, ángulos de 18° delante y 17° detrás y límites globales de carga.

A las alturas nominales de fondo 30/45 mm y batalla 2,89 m, el factor combinado de altura e inclinación es aproximadamente 0,669. Con ese factor, el aporte del fondo pasa de aproximadamente 46,6% a 30,8% y la carga total nominal aumenta aproximadamente 6,8%, sin cambiar su máximo de referencia. El reparto longitudinal fuera de la referencia puede cambiar.

Las polares de carga se escalan por separado para cada ala. Para representar la resistencia asociada, la parte de cada polar de resistencia por encima de su mínimo aumenta con el cuadrado del factor de carga. Es una aproximación de calibración, no una polar medida. Se conserva la resistencia base de carrocería; la resistencia total de referencia crece aproximadamente 9,4%. La velocidad punta puede disminuir y debe probarse.

## Neumáticos

Se confirmó mu=2,7, asfalto Fuji con fricción 1,0, carga de operación 3200 N delante y 3600 N detrás, y sensibilidad a carga nativa 1,6/0,8. No se aumentó mu: ya permite un nivel alto de fuerza en el banco.

Se detectó una incompatibilidad entre el perfil de origen y la curva nativa: el perfil de Formula90s usa caída lateral a 0,88 y pico a 0,115 rad; el donante nativo conservaba caída asintótica a 0,35, rigidez 18 y elasticidad 0,86. Se exportan ahora explícitamente caída a 88%, elasticidad 0,86 y rigidez calculada 104,26873608 para que el pico lateral ocurra a 0,115 rad (6,59°), con el mismo mu máximo. El pico anterior con esa ley estaba aproximadamente a 11,2°. El número de rigidez pertenece a la ley nativa y no es una rigidez física en N/rad.

La ley nativa usa una curva combinada: el ajuste también afecta tracción y frenada, y no reproduce todas las características del neumático de Formula90s. No se modificaron presión, calentamiento, refrigeración, desgaste, suspensión ni diferencial. La transición inicial de fuerza será más rápida y requiere prueba con mando.

## Evidencia y límites

Las pruebas ejecutan SimAeroUpdate/SimWingUpdate y SimWheelUpdateForce del motor nativo. Aerodinámica: 15 combinaciones de densidad/velocidad, reparto 40/60, conservación de carga y centro de presión de referencia, barrido de alturas, pérdida a cabeceo alto y techo de carga: PASS.

Neumáticos: las cuatro ruedas con cargas de 1200/3200/6000 N, suelo de fricción 1, sin camber y sin penalización térmica: PASS. Pico lateral medido a 0,115 rad y fuerza a 1 rad aproximadamente 93,2% del pico. Coeficiente pico entre 2,32 y 3,55 según carga/eje. No son aceleraciones del coche completo. Se mantienen sensibilidad a carga y el círculo de esfuerzo combinado nativo. Balance energético y seis escenarios térmicos existentes: PASS.

Se verificó que las secciones ajenas del XML permanecen idénticas y que repetir la redistribución no acumula escalado. La distribución portable fue ensamblada sin dependencias sin resolver. No hizo falta modificar ni recompilar el solver de producción: esta calibración usa parámetros existentes.

El contexto histórico confirmado es que 1997 utilizaba slicks y precede a los neumáticos ranurados de 1998; las referencias públicas consultadas no proporcionan una curva fuerza/carga/temperatura del Goodyear del FW19. Por tanto, no se declara fidelidad histórica ni estabilidad validada en conducción.

Referencia del contexto de 1997: https://ms.bridgestone.co.jp/special/document/f1/en/techs_regs/1997/. No aporta una curva cuantitativa del Goodyear del Williams.

Archivos: tools/prepare_williams_vehicle.py; tools/validate_formula_aerodynamics.cpp; tools/validate_formula_tire_energy.cpp; XML candidato y copia distribuida del Williams; instrucciones.md y este informe.

Aceptación pendiente: curvas lentas sin apoyo aerodinámico, entrada con frenada combinada y diferencial 70% de retención, progresividad con mando, cambios de altura a alta velocidad, velocidad punta y temperaturas durante vueltas completas.

Arranque verificado: sesión Williams/Fuji en RACE, registro Ready/Set/Go y mu=2,700 en las cuatro ruedas. Se conserva el límite de renderizado de 120 FPS. Registro copiado en la carpeta de evidencia; no se condujo el coche.
