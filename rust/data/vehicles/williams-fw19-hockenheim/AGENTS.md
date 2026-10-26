# Hockenheim — perfil del Williams FW19 1997

Ámbito: perfil físico de la variante Hockenheim/Villeneuve, construido desde el cuestionario respondido en `docs/f1_1997_williams_physical_profile_questionnaire.md`.
Consumidores: la definición de vehículo, la escena Williams y la sesión Fuji de prueba.
Reglas: configuración de baja resistencia para Hockenheim; el circuito de prueba es Fuji y no justifica cambiar el reglaje ni la librea. Los valores `estimated` no son telemetría. No se afirma simulación Williams en los dominios con valores heredados sin declararlo en el manifest.

## Filtros transitorios del neumático

`tire_filters.json` es el documento de ajuste de los filtros de transición (GRIP-02/04/05/06). Se lee en tiempo de carga del perfil: cambiar sus valores y reiniciar la sesión no requiere recompilar la DLL. Los campos ausentes conservan el valor del perfil estable en `tires.json`; los presentes lo sustituyen. `surface_transition_tau_s` es global; el resto se declara por eje (`front`/`rear`).

Estabilidad: `min_relaxation_tau_s` por encima de ~0.012 s, combinado con `relaxation_recovery_scale` alto, produce un ciclo límite de guiñada a alta velocidad (bamboleo en recta). Los valores de referencia 0.008 s y 1.3 se validaron con un pulso de dirección a 74 m/s; no subir el mínimo sin repetir esa prueba.

## Filtros numéricos de la suspensión

`suspension_filters.json` es el dominio numérico de la suspensión (no físico): `road_height_max_rate_m_s` limita la velocidad de cambio del objetivo de contacto y `road_height_filter_tau_s` (0.0 por defecto) sería un paso bajo opcional de 2–4 ms sobre ese mismo objetivo. Los campos `damper_velocity_filter_tau_s` y `arb_travel_filter_tau_s` por eje están limitados a 1 ms por validación y valen 0.0: no se activan sin evidencia de ruido.

Los coeficientes físicos del mismo cambio (ley C1 del tope, amortiguador de tope, mezcla del codo del amortiguador, tope compliant y amortiguador de barra) están en `suspension.json`, dentro de `geometry_physical.front/rear`, porque viajan con el reglaje. No duplicar valores entre ambos documentos.

## Raycast por rueda

`suspension.contact_ray_count` (1 o 3; por defecto 3) decide cuántos rayos transversales lanza el muestreador interno por rueda. Con `1` solo se lanza el rayo central y su impacto se repite en los tres huecos del muestreo: el modelo de contacto ve un plano único, así que no puede detectar contacto parcial ni peralte. Con `3` se recupera el tricast histórico (12 rayos por vehículo). El perfil está en **1** desde la iteración del 2026-10-25; las cargas y la altura de reposo medidas coinciden con el tricast.

No es un knob de reglaje: vive en el perfil y no se guarda en `open_formula.db`. Validación: cualquier valor distinto de 1 o 3 se rechaza al cargar.
