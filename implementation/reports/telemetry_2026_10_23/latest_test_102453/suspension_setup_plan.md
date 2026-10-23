# Plan de equilibrio de suspensión — Williams / Melbourne

Fecha: 23 de octubre de 2026. Alcance: planificación; no se han aplicado reglajes ni modificado producción.

**Revisión posterior:** la [segunda opinión con bancos de rigidez local](../../../../docs/investigation/second_opinion_spring_arb_2026-10-23.md) encuentra que el candidato 0.30/0.24 todavía puede dejar la barra como contribución mayoritaria. La selección definitiva y el barrido trasero deben seguir esa revisión; las variantes de este documento se conservan como propuestas previas, sin validación de conducción.

## Objetivo físico

Los muelles deben sostener la plataforma y aportar la base de rigidez de balanceo; los amortiguadores deben controlar la velocidad de los movimientos; las barras deben completar la rigidez y ajustar el reparto entre ejes; los topes deben actuar como reserva de recorrido. No se busca que todos produzcan fuerzas parecidas: el amortiguador debe tender a cero al cesar el movimiento y la barra debe tender a cero con recorridos iguales respecto a sus referencias.

Reducir la barra permite más independencia entre ruedas, pero también puede aumentar el balanceo y variar alturas, camber y balance aerodinámico. Tampoco elimina la transferencia de carga exigida por la maniobra. El objetivo es evitar la descarga prolongada observada manteniendo control del coche, no garantizar cuatro apoyos en cualquier salto, piano o aceleración lateral.

## Base confirmada

La sesión terminó a las 10:24:53, con 244.44 s físicos. El informe hermano analysis.md conserva el diagnóstico y sus límites. A 100.600 s, FR tiene muelle +1699 N, amortiguador -8.6 N y barra -1837.5 N, con carga normal cero y separación 10.3 mm. Los episodios prolongados seleccionados no alcanzan topes duros.

| Parámetro | Delante | Detrás |
| --- | ---: | ---: |
| Ratio efectivo de barra | 1.00 | 0.80 |
| Rigidez de acoplamiento registrada, N/m | 103040.4 | 15681.5 |
| Muelle físico en coordenada de elemento, N/m | 134300 | 142800 |
| Amortiguador de compresión, Ns/m | 6970 | 14000 |
| Amortiguador de extensión, Ns/m | 11475 | 22000 |
| Trim damping_ratio | 0.74 | 0.71 |
| Trim bump | 1.00 | 0.90 |
| Trim rebound | 1.12 | 1.35 |
| resting_ratio | 0.271429 | 0.336615 |

Las rigideces de barra son a nivel de rueda; los coeficientes físicos de muelle/amortiguador son de elemento. No compararlos directamente sin la transmisión geométrica. El valor de diseño 0.15/0.09 escrito en el bloque geométrico tampoco constituye una calibración histórica validada.

## 1. Comprobar autoridad y medir la contribución real

Antes de conducir candidatos:

1. Conservar el reglaje base como variante identificable. Registrar perfil efectivo, combustible, temperaturas/presiones, hashes de DLL y condiciones de pista.
2. Aplicar las barras mediante la transacción de garaje: suspension.geometry_physical.front_arb.ratio y rear_arb.ratio. Confirmar RUSTSETUPEFFECTIVE, RUSTSETUP DERIVED y RUSTAXLE al aplicar, guardar y recargar. En carga de perfil manda el ratio del eje; durante transacciones el mando geométrico se copia al eje. No editar únicamente un campo JSON que después sea sustituido.
3. Verificar en banco que movimiento simétrico da barra cero; movimiento antisimétrico genera fuerzas iguales y opuestas que restauran el movimiento; reflejar izquierda/derecha refleja el resultado; no hay energía creada por el elemento. Si falla este contrato, tratarlo como defecto físico antes de compensarlo con reglaje.
4. Medir fuerza de muelle y pendiente local dF/dq en el rango realmente recorrido durante las maniobras, incluyendo la esquina descargada, con el mismo evaluador del backend simplificado. La barra usa una rigidez basada en el recorrido de referencia, mientras el muelle cambia su transmisión con el recorrido. No basta dividir fuerzas instantáneas para estimar rigidez.
5. Obtener rigidez de balanceo de muelles y barras con perturbaciones pequeñas y simétricas, conservando alturas medias y registrando momentos. Aproximación para un eje simétrico: K_roll_springs = k_wheel * track² / 2 y K_roll_bar = K_bar * track², en Nm/rad. Con la geometría real, preferir derivadas de momento del evaluador.

Objetivo provisional de diseño: barras alrededor del 20–40 % de la rigidez elástica de balanceo por eje en el rango operativo principal. Es una preferencia de este encargo para dar más autoridad al muelle, no una regla universal ni una cifra ya medida. Revisar el reparto total delantero/trasero al cambiarla.

## 2. Barrido inicial de barras

Primera fase: conservar los demás ajustes de la tabla base para atribuir el efecto. Reducir ambas barras con un único factor de escala, como una sola variable experimental.

| Variante | Ratio delantero | Ratio trasero | K delante, N/m | K detrás, N/m | Fuerza delantera con 15 mm diferenciales |
| --- | ---: | ---: | ---: | ---: | ---: |
| Base | 1.00 | 0.80 | 103040.4 | 15681.5 | 1545.6 N |
| A — transición | 0.50 | 0.40 | 51520.2 | 7840.8 | 772.8 N |
| B — candidato principal | 0.30 | 0.24 | 30912.1 | 4704.5 | 463.7 N |
| C — más independencia | 0.20 | 0.16 | 20608.1 | 3136.3 | 309.1 N |

Las cifras son escalados algebraicos con muelles y geometría base intactos, sujetos al redondeo del log. No predicen el apoyo final: los recorridos y fuerzas se redistribuirán. La reducción común conserva la proporción entre las barras; no conserva necesariamente el reparto total de rigidez entre ejes, que incluye los muelles.

Probar A antes de B y B antes de C. B es el punto de partida recomendado para buscar un equilibrio, no un reglaje aprobado. Elegir entre ellos por apoyo, balanceo y respuesta, no por minimizar la barra a cualquier coste. Si C sigue dominando según la rigidez local medida, calcular otro valor menor; no endurecer automáticamente el muelle para alcanzar una cuota.

Incluir un banco con ambas barras a cero como control causal. Una maniobra de baja exigencia con ese control puede distinguir la contribución de la barra de un problema de contacto o cinemática que persista. Cero no es la recomendación final de conducción.

## 3. Verificar plataforma y muelles

Tras elegir la zona de barras:

- Medir alturas, recorrido restante a compresión/extensión y fuerza de topes, en reposo y con la carga aerodinámica de las rectas. Frenar en recta para separar cabeceo de balanceo.
- Si el muelle mantiene una plataforma adecuada, conservarlo. Reducir barras no justifica cambiar muelles por sí solo.
- Si aparece agotamiento de recorrido por carga sostenida, revisar primero equilibrio/altura y rigidez local; una mayor amortiguación no sostiene una carga estática. Si hace falta otra rigidez, ensayar cambios pequeños, inicialmente del 5–10 % en el eje afectado, conservando la altura estática mediante el ajuste físico correspondiente y volviendo a medir.
- Los spring_rate_N_per_m físicos no están expuestos como ajuste de garaje en SETUP_FIELDS. Cambiarlos sería una variante explícita del perfil, con su procedencia y validación. No interpretar resting_ratio o longitud como un mando de rigidez de muelle.
- Recalcular K_bar si se cambia el muelle: LegacyRatio deriva su rigidez de k_wheel_rest, de modo que endurecer un muelle puede endurecer también su barra aunque no cambie el ratio. Para conservar un K_bar elegido: ratio_nuevo = K_bar_objetivo / k_wheel_rest_nuevo, verificado en RUSTAXLE.

## 4. Afinar amortiguadores y topes

Con muelles y barras aceptables, ajustar únicamente el eje y la rama del amortiguador que la telemetría señale:

- Extensión: si la rueda sigue tardando en recuperar apoyo mientras el muelle puede extenderla, reducir su coeficiente efectivo en pasos de aproximadamente 10 %. Si aparece oscilación o repetición de pérdidas al recuperar, volver al paso anterior.
- Compresión: si el cambio de dirección produce picos de carga y poca adaptación al suelo, reducir por pasos pequeños; si el transitorio es excesivamente rápido o poco amortiguado, aumentar por pasos pequeños. Separar esa respuesta de la carga sostenida y de la rigidez del muelle.
- Mantener damping_ratio mientras se afina una rama, pues ese mando escala compresión y extensión a la vez. suspension_setup.rs deriva los coeficientes efectivos desde la base original: comprobar siempre Ns/m efectivos, no solo el número del mando. Por ejemplo, una reducción del 10 % de rebound delantero equivale aproximadamente a trim 1.12 → 1.01 y coeficiente 11475 → 10348 Ns/m con el resto intacto.
- Conservar inicialmente topes 2.6/2.8. Revisarlos solo si intervienen de manera prolongada o prematura. Los topes duros respetan la geometría; no ampliar recorridos para esconder la descarga de la barra.

El resultado esperado es control del transitorio con recuperación de apoyo, sin usar el amortiguador para reemplazar rigidez elástica.

## 5. Afinar reparto entre ejes

Elegida la rigidez total útil, afinar en torno al candidato con pequeños cambios del ratio de un solo eje, aproximadamente 0.02–0.03 por ensayo, confirmando la variación real de rigidez.

Si hay subviraje y margen de balanceo, evaluar ablandar delante; si hay sobreviraje, evaluar ablandar detrás. Contrastar siempre con cargas, neumáticos y respuesta de dirección: son tendencias de ajuste, no diagnósticos automáticos. No endurecer el otro eje hasta reproducir otra rueda interior sostenidamente descargada.

## 6. Ensayo y aceptación

Pruebas comparables, en ambos sentidos y con al menos tres repeticiones por maniobra: frenada fuerte recta; frenada con giro progresivo; izquierda-derecha y derecha-izquierda; curva sostenida; después pianos/baches y alta velocidad. Mantener velocidad de entrada, combustible y estado de neumáticos comparables. No comparar sin más episodios de diferentes vueltas de la sesión anterior.

Instrumentación necesaria para una conclusión: freno/acelerador aplicados, dirección con signo y ángulos de ruedas, posición sobre pista, aceleración lateral/longitudinal, roll/pitch y velocidades, N/contacto/separación, recorrido/topes y fuerzas separadas. El steer_rad actual pierde ángulos negativos y las ventanas pueden truncar historia; registrar entradas y ticks aceptados suficientes para cubrir la maniobra completa. Son dependencias instrumentales si se implementa el plan, no cambios ya hechos.

Puertas propuestas para asfalto continuo y maniobras donde el equilibrio admite cuatro apoyos:

- Reducción de al menos 80 % del tiempo sin apoyo delantero respecto a una base repetida en iguales condiciones. Evitar episodios sostenidos superiores a 100 ms atribuibles a la barra; separar saltos, pianos y pérdidas físicamente inevitables. Estos son umbrales iniciales del proyecto, ajustables con la repetibilidad medida.
- La barra deja de cancelar persistentemente al muelle de la esquina descargada. Verificarlo junto con cinemática, carga normal y contribuciones a rigidez, no como una desigualdad universal de fuerzas.
- No aparecen rechazos del solver, penetraciones, topes duros sostenidos, contacto sostenido del fondo, crecimiento de oscilaciones ni pérdida trasera nueva. Medir clearance mínimo y tiempo/impulso de fondo frente a la base; cualquier evento nuevo necesita explicación antes de aceptar.
- El balanceo conserva márgenes de recorrido y geometría. Medir gradiente roll/g en curva estable compensando el peralte; no imponer un ángulo absoluto a toda la pista.
- Respuesta de dirección, estabilidad de frenada y recuperación no empeoran. La conducción del usuario decide la aceptación final; banco, carga de sesión e instalación no equivalen a esa aceptación.

Si con barras pequeñas o cero persiste el mismo fallo, detener la escalada de ajustes y volver al contacto, alcance de rayos, marco móvil y acoplamiento del chasis. Un reglaje debe resolver un desequilibrio, no ocultar una ecuación incorrecta.

## Entregable previsto al ejecutar

Variantes de setup identificadas y reversibles, tabla de coeficientes efectivos y métricas por maniobra, variante elegida y motivo, límites pendientes y procedimiento de restauración. En esta fase solo se creó este plan.

Fuentes de diseño: [OptimumG — Bar Talk](https://optimumg.com/bar-talk/) explica la elección de muelles para plataforma/contacto y la aportación adicional de barras a rigidez y reparto de balanceo; [Öhlins — TTX46CS/TTX36, Adjustment and Set-up](https://www.ohlins.com/storage/75BCF25BEBEB0E0888B403FA1682CB3A8A90202F0E1554F5AFFE7E400EA8002C/dd6b0930ac584bb981d9daaff413d637/pdf/media/b7cc030f623d48b0b4604c6ed4299470/OM_07446-01_TTX46CS_TTX36.pdf) orienta el ajuste gradual de compresión/extensión. Los ratios, variantes y umbrales anteriores son propuestas propias para este modelo, no valores prescritos por esas fuentes.
