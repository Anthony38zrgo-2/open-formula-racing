# Panel compacto de neumáticos

Se reduce el panel de 477.576×392.04 a 330×230 px a escala 1080p: aproximadamente 59.5% menos superficie. Se conserva la cuadrícula FL/FR/RL/RR, posición inferior izquierda, Libron y paleta del garaje.

Cada rueda muestra presión, tres zonas térmicas independientes, temperatura máxima medida, barra de goma restante con porcentaje y temperatura del disco de freno. Las zonas interiores/exteriores se reflejan según el lado del coche. El desgaste no modifica el color térmico: dispone de barra propia. Usa la fracción de profundidad restante que Rust ya publica en la vista de rueda; un valor ausente o fuera de rango muestra --% y no se rellena la barra.

Se retiran de esta vista el título grande y las líneas CAR/GAS/RIM. Sus datos físicos y telemetría permanecen intactos; no se añade un panel expandido ni se cambia el modelo físico.

Validación: osggraph compilado e instalado; revisión visual de la captura real de Fuji hud_tyres_compact/frame-120.png. Presión, temperatura y goma restante visibles sin solapamientos. Resolución 1920×1080 conservada. Sin commit ni push.
