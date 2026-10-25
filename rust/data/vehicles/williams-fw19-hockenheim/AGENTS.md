# Hockenheim — perfil del Williams FW19 1997

Ámbito: perfil físico de la variante Hockenheim/Villeneuve, construido desde el cuestionario respondido en `docs/f1_1997_williams_physical_profile_questionnaire.md`.
Consumidores: la definición de vehículo, la escena Williams y la sesión Fuji de prueba.
Reglas: configuración de baja resistencia para Hockenheim; el circuito de prueba es Fuji y no justifica cambiar el reglaje ni la librea. Los valores `estimated` no son telemetría. No se afirma simulación Williams en los dominios con valores heredados sin declararlo en el manifest.

## Filtros transitorios del neumático

`tire_filters.json` es el documento de ajuste de los filtros de transición (GRIP-02/04/05/06). Se lee en tiempo de carga del perfil: cambiar sus valores y reiniciar la sesión no requiere recompilar la DLL. Los campos ausentes conservan el valor del perfil estable en `tires.json`; los presentes lo sustituyen. `surface_transition_tau_s` es global; el resto se declara por eje (`front`/`rear`).

Estabilidad: `min_relaxation_tau_s` por encima de ~0.012 s, combinado con `relaxation_recovery_scale` alto, produce un ciclo límite de guiñada a alta velocidad (bamboleo en recta). Los valores de referencia 0.008 s y 1.3 se validaron con un pulso de dirección a 74 m/s; no subir el mínimo sin repetir esa prueba.
