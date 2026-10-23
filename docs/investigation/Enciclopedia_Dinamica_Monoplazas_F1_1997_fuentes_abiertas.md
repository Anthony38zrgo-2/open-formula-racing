---
title: "Enciclopedia de dinámica y mecánica de monoplazas — base para F1 1997"
language: es
format: markdown
edition: "1.0 — 23 de octubre de 2026"
project: Open Formula Simulation
source_documents: 4
pdf_pages_total: 276
content_policy: "Conversión íntegra del texto extraíble de los cuatro PDF suministrados por el usuario, con índice temático original."
---

# Enciclopedia de dinámica y mecánica de monoplazas

**Base documental para reconstruir las físicas de los coches de Fórmula 1 de 1997.**

Esta enciclopedia reúne **cuatro trabajos de investigación** adjuntos por el usuario. Consta de dos capas: (1) una **guía temática en español**, que indica qué conceptos y páginas estudiar, y (2) la **transcripción completa del texto que puede extraerse de los cuatro PDF**, conservando el idioma y la referencia de la página original. No es una traducción íntegra de las fuentes ni sustituye las imágenes originales; es una conversión de consulta e indexación.

> **Alcance histórico:** los cuatro trabajos describen Formula Student/Formula SAE, modelos teóricos de neumáticos y técnicas modernas de dinámica vehicular. **No aportan por sí solos reglajes ni valores originales de las escuderías de F1 de 1997.** Los números de los vehículos universitarios son ejemplos experimentales, **no parámetros históricos del Williams FW19 ni del resto de la parrilla**.

> **Advertencia sobre la fidelidad:** el texto se ha extraído de PDF automáticamente. Se conservan las ecuaciones, cifras, símbolos y epígrafes cuando aparecen en la capa textual del PDF, pero algunas fórmulas matriciales, fracciones, superíndices, tablas y esquemas pueden perder su disposición. Las figuras no están incrustadas en este único archivo `.md`; sus títulos y referencias de página sí aparecen en la transcripción. Para implementar fórmulas críticas, cotejar siempre con la **página del PDF original**.

## Navegación principal

- [Fuentes y datos bibliográficos](#fuentes-y-datos-bibliográficos)
- [Mapa temático y rutas de consulta](#mapa-temático-y-rutas-de-consulta)
- [Enciclopedia por sistemas](#enciclopedia-por-sistemas)
- [Glosario técnico bilingüe](#glosario-técnico-bilingüe)
- [Guía práctica de aplicación a Open Formula](#guía-práctica-de-aplicación-a-open-formula)
- [Texto original — GASPAR, 2026](#fuente-gaspar)
- [Texto original — DÍAZ DE RÁBAGO, 2024](#fuente-diaz)
- [Texto original — GAITHER, 2021](#fuente-gaither)
- [Texto original — ROMANO et al., 2023](#fuente-romano)

## Cómo citar y encontrar un concepto

Las referencias **GAS**, **DIA**, **GAI** y **ROM** identifican cada fuente. **PDF pág. N** señala la página física del archivo (contando la portada), no necesariamente el número impreso en el documento. Por ejemplo, `[GASPAR · PDF pág. 75](#gaspar-p075)` lleva a la sección de Pacejka en el archivo original de Gaspar. Los enlaces internos funcionan en visores Markdown que aceptan anclas HTML.

El apartado «Enciclopedia por sistemas» está redactado en español para localizar conceptos. El apartado «Texto original» transcribe el material fuente, sin modificar su terminología original. Las interpretaciones específicas para el videojuego se señalan como **Aplicación a Open Formula**.

<a id="fuentes-y-datos-bibliográficos"></a>

## Fuentes y datos bibliográficos

| Código | Estudio | Año | PDF págs. | Alcance | Repositorio |
|---|---|---:|---:|---|---|
| **GAS** | Diogo Luís Marques Gaspar: *Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização do Sistema de Suspensão* | 2026 | 108 | Modelo vehicular de 12 grados de libertad, transferencia de cargas, Pacejka, suspensión, control, Simulink y contrastación con datos de pista. | [Original](https://iconline.ipleiria.pt/entities/publication/49669f46-0a29-4bca-8d71-47e1e9b815a8) |
| **DIA** | José Díaz de Rábago Areilza: *Analysis and Design of a Roll-Heave Decoupled Front Suspension for a Formula Student Prototype* | 2024 | 101 | Diseño y cálculo de suspensión delantera con control separado de heave y roll, balancines, motion ratio, wheel hop, CAD y FEA. | [Original](https://repositorio.comillas.edu/bitstreams/887ca4f7-e552-4a81-a3af-8779b3db931e/download) |
| **GAI** | Audrey Gaither: *Vehicle Dynamics on an Electric Formula SAE Racecar* | 2021 | 30 | Neumáticos, camber, cinemática, geometría de dirección, transferencia lateral, integración con aerodinámica y decisiones de diseño. | [Original](https://dspace.mit.edu/entities/publication/cb994dec-2439-445e-bd4c-a235120c068b) |
| **ROM** | Luigi Romano, Fredrik Bruzelius, Mattias Hjort y Bengt Jacobson: *Development and Analysis of the Two-Regime Transient Tyre Model for Combined Slip* | 2023 | 37 | Modelo transitorio de neumáticos, deslizamiento combinado, longitud de relajación, rigidez de carcasa, estabilidad y validación experimental. | [Original](https://research.chalmers.se/publication/530309/file/530309_Fulltext.pdf) |

**Derechos:** los textos originales mantienen sus titulares, autores y condiciones de uso. El artículo de Romano et al. está publicado en acceso abierto bajo CC BY 4.0; ello **no implica** que las otras tesis compartan esa licencia. Esta compilación es una transformación de los archivos aportados para consulta y no atribuye una licencia nueva a las obras originales.

<a id="mapa-temático-y-rutas-de-consulta"></a>

## Mapa temático y rutas de consulta

| Buscar información sobre… | Primera lectura | Complemento | Prioridad para el simulador |
|---|---|---|---|
| Estructura de un modelo vehicular completo | [GASPAR · PDF pág. 64](#gaspar-p064), [GASPAR · PDF pág. 88](#gaspar-p088) | [GASPAR · PDF pág. 26](#gaspar-p026) | Alta |
| Sistema de coordenadas y grados de libertad | [GASPAR · PDF pág. 26](#gaspar-p026), [GASPAR · PDF pág. 29](#gaspar-p029) | [DIAZ · PDF pág. 36](#diaz-p036) | Alta |
| Reparto de cargas longitudinal y lateral | [GASPAR · PDF pág. 28](#gaspar-p028), [GASPAR · PDF pág. 69](#gaspar-p069) | [GAITHER · PDF pág. 19](#gaither-p019) | Alta |
| Slip angle, slip ratio y camber | [GASPAR · PDF pág. 32](#gaspar-p032), [GASPAR · PDF pág. 73](#gaspar-p073) | [GAITHER · PDF pág. 8](#gaither-p008) | Crítica |
| Magic Formula / Pacejka | [GASPAR · PDF pág. 38](#gaspar-p038), [GASPAR · PDF pág. 75](#gaspar-p075) | [GASPAR · PDF pág. 53](#gaspar-p053) | Alta |
| Combined slip (frenada y giro simultáneos) | [ROMANO · PDF pág. 12](#romano-p012), [ROMANO · PDF pág. 14](#romano-p014) | [GASPAR · PDF pág. 38](#gaspar-p038) | Crítica |
| Relajación y transitorios de neumáticos | [ROMANO · PDF pág. 5](#romano-p005), [ROMANO · PDF pág. 9](#romano-p009) | [ROMANO · PDF pág. 20](#romano-p020) | Alta |
| Carga vertical y sensibilidad del neumático | [GAITHER · PDF pág. 8](#gaither-p008), [GASPAR · PDF pág. 53](#gaspar-p053) | [ROMANO · PDF pág. 23](#romano-p023) | Crítica |
| Suspensión: masa suspendida/no suspendida | [GASPAR · PDF pág. 39](#gaspar-p039), [GASPAR · PDF pág. 81](#gaspar-p081) | [DIAZ · PDF pág. 59](#diaz-p059) | Crítica |
| Wheel hop, rebotes y frecuencias propias | [DIAZ · PDF pág. 61](#diaz-p061), [DIAZ · PDF pág. 64](#diaz-p064) | [GASPAR · PDF pág. 42](#gaspar-p042) | Crítica |
| Separar roll de heave | [DIAZ · PDF pág. 49](#diaz-p049), [DIAZ · PDF pág. 67](#diaz-p067) | [DIAZ · PDF pág. 81](#diaz-p081) | Media |
| Motion ratio y geometría de balancines | [DIAZ · PDF pág. 67](#diaz-p067), [DIAZ · PDF pág. 72](#diaz-p072) | [GAITHER · PDF pág. 13](#gaither-p013) | Alta |
| Barras estabilizadoras y rigidez al balanceo | [GASPAR · PDF pág. 44](#gaspar-p044), [GASPAR · PDF pág. 104](#gaspar-p104) | [GAITHER · PDF pág. 20](#gaither-p020) | Alta |
| Dirección, caster, kingpin y scrub radius | [GAITHER · PDF pág. 13](#gaither-p013), [GAITHER · PDF pág. 23](#gaither-p023) | [DIAZ · PDF pág. 41](#diaz-p041) | Media |
| Aerodinámica y fuerza vertical | [GASPAR · PDF pág. 48](#gaspar-p048), [GAITHER · PDF pág. 21](#gaither-p021) | [GASPAR · PDF pág. 82](#gaspar-p082) | Alta |
| Frenos y propulsión | [GASPAR · PDF pág. 64](#gaspar-p064), [GASPAR · PDF pág. 67](#gaspar-p067) | [GASPAR · PDF pág. 46](#gaspar-p046) | Alta |
| Simulación de vuelta y telemetría | [GASPAR · PDF pág. 48](#gaspar-p048), [GASPAR · PDF pág. 91](#gaspar-p091) | [GASPAR · PDF pág. 96](#gaspar-p096) | Alta |
| Validación experimental y limitaciones | [ROMANO · PDF pág. 23](#romano-p023), [GASPAR · PDF pág. 105](#gaspar-p105) | [DIAZ · PDF pág. 95](#diaz-p095) | Crítica |

<a id="enciclopedia-por-sistemas"></a>

## Enciclopedia por sistemas

### 1. Arquitectura general de la simulación vehicular

**Concepto.** La dinámica de un monoplaza surge de la interacción entre traslación y rotación del chasis, velocidad angular de las ruedas, fuerzas de contacto, masas suspendidas/no suspendidas, dirección, frenado y propulsión. El número de grados de libertad representa las variables independientes elegidas, no un indicador automático de mayor realismo.

**Fuente.** Gaspar construye un modelo de **12 grados de libertad** en MATLAB/Simulink, con subsistemas de fuerzas, neumáticos, suspensión y entrada del conductor ([GASPAR · PDF pág. 64](#gaspar-p064)). La tesis de Gaither destaca que las decisiones sobre neumáticos, aerodinámica, dirección y suspensión deben considerarse conjuntamente ([GAITHER · PDF pág. 7](#gaither-p007)).

**Aplicación a Open Formula.** Crear un diagrama claro de qué módulo calcula fuerzas, qué módulo integra estados y dónde se aplica el contacto rueda-suelo. Conservar una única convención de unidades, signos y ejes. No copiar la estructura de Simulink literalmente: se puede implementar con ecuaciones equivalentes en Rust o C++.

### 2. Sistema de coordenadas, velocidades y yaw

**Concepto.** Para calcular las fuerzas de neumáticos es imprescindible pasar la velocidad local del punto de contacto al referencial de cada rueda, teniendo en cuenta la velocidad de traslación, la rotación del chasis y el ángulo de dirección. No es suficiente usar la velocidad del centro de masas para las cuatro ruedas.

**Fuente.** Gaspar presenta sistemas de coordenadas, un modelo de dos ruedas y otro de cuatro ruedas ([GASPAR · PDF pág. 26](#gaspar-p026), [GASPAR · PDF pág. 29](#gaspar-p029)). La derivación de slip angle y la velocidad periférica de las ruedas aparece en sus capítulos de implementación ([GASPAR · PDF pág. 70](#gaspar-p070), [GASPAR · PDF pág. 72](#gaspar-p072)).

**Aplicación a Open Formula.** Auditar los signos lateral/longitudinal de las cuatro ruedas y validar por separado avance recto, marcha atrás, giro a velocidad constante y frenada. Un error de referencial puede parecer un fallo de suspensión o de agarre.

### 3. Transferencia de carga, centros de gravedad y balance dinámico

**Concepto.** Frenada, aceleración y paso por curva alteran la carga normal de cada rueda, lo que modifica las fuerzas disponibles. Esa transferencia está relacionada con aceleraciones, masa, altura del centro de gravedad, batalla, trochas, geometría y rigidez de balanceo. No debe confundirse con el desplazamiento visual de la carrocería.

**Fuente.** Gaspar trata fuerzas longitudinales/laterales y posteriormente implementa un subsistema de transferencia de cargas ([GASPAR · PDF pág. 28](#gaspar-p028), [GASPAR · PDF pág. 69](#gaspar-p069)). Gaither introduce la distribución de transferencia lateral total, **TLLTD**, en relación con la respuesta del vehículo ([GAITHER · PDF pág. 19](#gaither-p019)).

**Aplicación a Open Formula.** Medir la suma de las cuatro cargas normales y el reparto por ejes en frenada/cornering. En suelo plano, si no actúan otras fuerzas verticales, debe respetarse el equilibrio de fuerzas verticales a escala del vehículo. Una rueda puede descargarse o perder contacto, pero no debe generar una fuerza normal negativa.

### 4. Neumáticos: sensibilidad a carga normal

**Concepto.** La capacidad absoluta de fuerza aumenta habitualmente al aumentar la carga normal, pero el coeficiente efectivo de fricción puede reducirse. Por ello una transferencia de carga intensa puede disminuir el potencial combinado del vehículo. El comportamiento depende del neumático ensayado.

**Fuente.** Gaither muestra explícitamente la relación entre carga normal y coeficiente de fricción en su análisis de neumáticos FSAE ([GAITHER · PDF pág. 8](#gaither-p008), [GAITHER · PDF pág. 9](#gaither-p009)). Gaspar presenta datos experimentales del Hoosier R20 usados para parametrizar su modelo ([GASPAR · PDF pág. 53](#gaspar-p053)).

**Aplicación a Open Formula.** Aplicar una sensibilidad de adherencia a carga normal en un único punto del modelo, evitando multiplicadores superpuestos. No sustituir los datos Goodyear/Bridgestone de 1997 por las curvas del Hoosier R20 sin calibración.

### 5. Slip angle, slip ratio y camber

**Concepto.** *Slip angle* describe la diferencia angular entre el plano de la rueda y su dirección de desplazamiento local; *slip ratio* describe el deslizamiento longitudinal por diferencia entre velocidad periférica y velocidad longitudinal; *camber* es la inclinación de la rueda respecto del plano vertical.

**Fuente.** Gaspar introduce los tres conceptos en su apartado 4.5 y los calcula explícitamente en los subsistemas 6.4 y 6.6 ([GASPAR · PDF pág. 32](#gaspar-p032), [GASPAR · PDF pág. 70](#gaspar-p070), [GASPAR · PDF pág. 73](#gaspar-p073)). Gaither relaciona camber y generación de fuerza lateral, y documenta decisiones de geometría de suspensión orientadas a camber dinámico ([GAITHER · PDF pág. 9](#gaither-p009), [GAITHER · PDF pág. 16](#gaither-p016)).

**Precaución.** Las convenciones de signo y normalización de slip ratio varían según el autor. Elegir **una sola definición canónica** y hacer conversiones explícitas al importar ecuaciones de otras fuentes.

### 6. Combinación de esfuerzos longitudinales y laterales

**Concepto.** Frenar o acelerar consume parte de la capacidad de fuerza del neumático y modifica la capacidad lateral disponible. El círculo o elipse de fricción es una representación práctica del límite combinado, pero su forma depende del modelo y de los datos.

**Fuente.** Gaspar introduce el círculo de fricción, el combined slip y la Magic Formula ([GASPAR · PDF pág. 37](#gaspar-p037), [GASPAR · PDF pág. 38](#gaspar-p038)). Romano et al. desarrollan un modelo transitorio que incorpora el acoplamiento longitudinal-lateral, más allá de los modelos estacionarios simples ([ROMANO · PDF pág. 12](#romano-p012), [ROMANO · PDF pág. 14](#romano-p014)).

**Aplicación a Open Formula.** Preferir un único cálculo coherente de Fx/Fy bajo deslizamiento combinado. Evitar un multiplicador añadido después del cálculo lateral que viole los límites de fuerza ya impuestos por el modelo.

### 7. Fórmula Mágica de Pacejka y parametrización

**Concepto.** La Magic Formula aproxima la respuesta fuerza-deslizamiento mediante parámetros ajustados a curvas experimentales. Es una representación empírica, no una tabla universal de coefficients para cualquier neumático. Su implementación requiere consistencia en unidades, ángulos y carga normal.

**Fuente.** Gaspar explica el modelo y emplea datos de neumáticos Hoosier R20; su implementación usa tablas de interpolación en función de Fz para alimentar el cálculo de fuerzas ([GASPAR · PDF pág. 38](#gaspar-p038), [GASPAR · PDF pág. 53](#gaspar-p053), [GASPAR · PDF pág. 75](#gaspar-p075)).

**Aplicación a Open Formula.** Separar los parámetros de identificación experimental del integrador físico. Validar curvas Fx–slip ratio y Fy–slip angle para varias cargas antes de introducir desgaste o temperatura.

### 8. Transitorios y longitud de relajación

**Concepto.** La fuerza no necesariamente cambia de inmediato cuando varía el deslizamiento. La longitud de relajación cuantifica, en modelos de primer orden, la distancia característica de desarrollo de la respuesta; el artículo de Romano describe convencionalmente la distancia para alcanzar aproximadamente el 63 % de la fuerza estacionaria.

**Fuente.** Romano et al. explican el modelo *single contact point* y desarrollan un sistema más amplio que trata tanto deformación de carcasa como dinámica de los elementos de la banda de rodadura ([ROMANO · PDF pág. 4](#romano-p004), [ROMANO · PDF pág. 6](#romano-p006), [ROMANO · PDF pág. 9](#romano-p009)). Analizan la anisotropía y la relación entre las longitudes longitudinal/lateral, además de comparar con experimentos ([ROMANO · PDF pág. 15](#romano-p015), [ROMANO · PDF pág. 23](#romano-p023)).

**Aplicación a Open Formula.** Si existe memoria transitoria en el modelo, debe estar definida por un estado con significado físico. Evitar encadenar múltiples suavizadores que produzcan respuestas artificialmente lentas y compliquen la depuración.

### 9. Suspensión: masa suspendida y masa no suspendida

**Concepto.** La masa suspendida incluye los componentes soportados por la suspensión; la no suspendida incluye gran parte del conjunto rueda-freno-mangueta. Su movimiento relativo, junto a la rigidez del neumático, forma un sistema dinámico cuya respuesta a baches puede ser muy distinta del movimiento de la carrocería.

**Fuente.** Gaspar explica los elementos de muelle, amortiguador, rueda y masa no suspendida ([GASPAR · PDF pág. 39](#gaspar-p039), [GASPAR · PDF pág. 42](#gaspar-p042)), y construye un modelo específico de movimiento de suspensión ([GASPAR · PDF pág. 81](#gaspar-p081)). Díaz de Rábago utiliza un modelo de dos grados de libertad en heave precisamente para incluir fenómenos como *wheel hop* ([DIAZ · PDF pág. 11](#diaz-p011), [DIAZ · PDF pág. 59](#diaz-p059)).

**Aplicación a Open Formula.** Probar cada rueda contra un bache conocido con fuerza de contacto, velocidad de compresión, recorrido y velocidad vertical registrados. Identificar si la energía anómala proviene de la fuerza de neumático, del amortiguador, de una colisión rígida o del integrador.

### 10. Amortiguación, rigidez, oscilaciones y wheel hop

**Concepto.** Un muelle almacena energía; un amortiguador disipa energía según velocidad de compresión/extensión. Un sistema poco amortiguado puede oscilar; uno con amortiguación excesiva puede transmitir cargas de manera abrupta. Las frecuencias de carrocería y de wheel hop corresponden a modos diferentes.

**Fuente.** Gaspar desarrolla conceptos de amortiguación crítica y tipos de amortiguadores ([GASPAR · PDF pág. 42](#gaspar-p042), [GASPAR · PDF pág. 43](#gaspar-p043)). En el proyecto específico de Díaz de Rábago se diseñaron aproximadamente **3,91 Hz** para heave y **24,36 Hz** para wheel hop; son resultados de su prototipo, **no valores prescritos para un F1 de 1997** ([DIAZ · PDF pág. 11](#diaz-p011), [DIAZ · PDF pág. 64](#diaz-p064)).

**Aplicación a Open Formula.** Instrumentar energía, fuerza del amortiguador, recorrido restante y contacto por rueda. La suspensión debe disipar energía neta, salvo las fuerzas externas aplicadas; revisar signos y escalado temporal si se observa aumento espontáneo de amplitud.

### 11. Roll, heave y mecanismos de desacoplamiento

**Concepto.** *Heave* es el desplazamiento vertical común del chasis; *roll* es la rotación alrededor del eje longitudinal. Mecanismos con balancines y elementos separados permiten ajustar la resistencia de los dos movimientos con menor interdependencia.

**Fuente.** Díaz de Rábago desarrolla un conjunto de dos balancines asimétricos, un elemento elástico de heave y otro de roll, con relaciones de movimiento nominales **1,2 para heave** y **0,8 para roll** en su diseño ([DIAZ · PDF pág. 11](#diaz-p011), [DIAZ · PDF pág. 12](#diaz-p012), [DIAZ · PDF pág. 67](#diaz-p067)). El modelo y su geometría se describen ampliamente, pero no constituyen un diseño histórico de Williams o Ferrari.

**Aplicación a Open Formula.** Si se modela un tercer elemento, definir claramente la contribución de heave y de roll en términos de desplazamiento relativo de ruedas. No sumarlo a la rigidez de cada esquina sin comprobar que se conserva la descomposición modal.

### 12. Motion ratio, pushrod/pullrod, balancines y cinemática

**Concepto.** La relación entre desplazamiento de rueda y desplazamiento del muelle depende de la geometría. La rigidez efectiva vista en rueda depende del cuadrado de esa relación, **según cómo se haya definido el motion ratio**. Un error al invertir la relación puede multiplicar el error de rigidez.

**Fuente.** Díaz de Rábago estudia restricciones de empaquetado, balancines, rigidez y motion ratio en la definición física de su suspensión ([DIAZ · PDF pág. 67](#diaz-p067), [DIAZ · PDF pág. 72](#diaz-p072), [DIAZ · PDF pág. 81](#diaz-p081)). Gaither estudia los efectos de geometría, camber y centros de balanceo ([GAITHER · PDF pág. 13](#gaither-p013), [GAITHER · PDF pág. 16](#gaither-p016)).

**Aplicación a Open Formula.** Definir `motion_ratio` como `recorrido_muelle/recorrido_rueda` o su inverso, nunca ambas acepciones. Verificar la derivada del recorrido de muelle frente al recorrido de rueda en la simulación.

### 13. Barra estabilizadora y distribución lateral

**Concepto.** Una barra estabilizadora acopla parcialmente los desplazamientos verticales de las dos ruedas de un eje y modifica la rigidez al balanceo y la distribución de transferencia lateral. Una suspensión rígida en roll puede seguir siendo relativamente blanda en heave, dependiendo del esquema mecánico.

**Fuente.** Gaspar explica el dimensionamiento de la barra y realiza una prueba comparativa. En su simulación, al configurar una contribución del **40 % a la rigidez total en roll**, el pico máximo de roll descendió de 1,105° a 0,832° ([GASPAR · PDF pág. 44](#gaspar-p044), [GASPAR · PDF pág. 104](#gaspar-p104)). Gaither trabaja con las tasas de balanceo y el reparto lateral ([GAITHER · PDF pág. 19](#gaither-p019), [GAITHER · PDF pág. 21](#gaither-p021)).

**Aplicación a Open Formula.** Aplicar las fuerzas opuestas de la barra por diferencia de recorridos y revisar las condiciones de extensión máxima y pérdida de contacto; no usar una fuerza adicional de orientación del chasis que esconda un error de transferencia de cargas.

### 14. Dirección, ángulo de pivote, caster y scrub radius

**Concepto.** La geometría de dirección influye en estabilidad, cambio de camber, autoalineamiento, carga de volante y variación geométrica a lo largo del recorrido. Son parámetros mecánicos distintos de la curva de respuesta del mando o del asistente de dirección.

**Fuente.** Gaither describe kingpin inclination, caster y scrub radius y presenta valores de su vehículo MY19 ([GAITHER · PDF pág. 13](#gaither-p013), [GAITHER · PDF pág. 16](#gaither-p016), [GAITHER · PDF pág. 19](#gaither-p019)). Son cifras de Formula SAE y no se deben copiar a un FW19.

**Aplicación a Open Formula.** Separar geometría de suspensión, ángulo aplicado de dirección y posibles asistencias de control. Validar toe/bump steer frente a compresión por rueda.

### 15. Aerodinámica, alturas y fuerza por rueda

**Concepto.** La carga aerodinámica altera las cargas verticales y, por tanto, las fuerzas máximas de contacto. En un modelo básico, las fuerzas dependen de la velocidad; un modelo más completo también puede depender de altura, cabeceo y balanceo.

**Fuente.** Gaspar incorpora aerodinámica dentro del modelo general y de las ecuaciones de movimiento del chasis ([GASPAR · PDF pág. 48](#gaspar-p048), [GASPAR · PDF pág. 82](#gaspar-p082)). Gaither analiza las interdependencias de aerodinámica y suspensión ([GAITHER · PDF pág. 21](#gaither-p021)).

**Aplicación a Open Formula.** Aplicar la carga aerodinámica en puntos de fuerza definidos, no únicamente como incremento directo de Fz en las ruedas si luego el chasis vuelve a transferir esa carga, pues habría riesgo de doble contabilización.

### 16. Frenos y propulsión

**Concepto.** Las acciones de freno y motor generan momentos sobre las ruedas y fuerzas longitudinales solo a través del modelo de contacto neumático-suelo. La variación de carga normal afecta al máximo frenado y tracción de cada rueda.

**Fuente.** Gaspar presenta dimensionamiento y simulación de frenos, cadena de propulsión y cálculo de velocidad angular individual ([GASPAR · PDF pág. 46](#gaspar-p046), [GASPAR · PDF pág. 64](#gaspar-p064), [GASPAR · PDF pág. 67](#gaspar-p067), [GASPAR · PDF pág. 72](#gaspar-p072)).

**Aplicación a Open Formula.** Mantener separación entre `torque_engine`, `torque_brake`, dinámica rotacional de la rueda y `Fx` de neumático. No convertir directamente el par aplicado en una fuerza ilimitada sobre el chasis.

### 17. Simulación del tiempo por vuelta

**Concepto.** Los modelos de tiempo por vuelta pueden ser estacionarios, cuasiestacionarios o transitorios. El análisis de una vuelta permite contrastar aceleración, frenada y velocidad de paso por curva, pero no identifica de modo único todas las constantes de la física.

**Fuente.** Gaspar compara categorías de métodos y documenta ensayos de aceleración y autocross ([GASPAR · PDF pág. 48](#gaspar-p048), [GASPAR · PDF pág. 91](#gaspar-p091), [GASPAR · PDF pág. 96](#gaspar-p096)).

**Aplicación a Open Formula.** Calibrar mediante una secuencia de pruebas: aceleración recta, frenada recta, skidpad, bache aislado, cambio de apoyo y finalmente vuelta completa; evitar compensar un error de neumáticos con potencia o alerones.

### 18. Validación, errores y límites de los resultados

**Concepto.** Una simulación puede funcionar en recta y fallar en curvas transitorias; también puede reproducir una curva global sin describir correctamente las cargas de cada rueda. La validación debe indicar qué observable se comparó y qué no se pudo verificar.

**Fuente.** Gaspar reporta **RMSE de 1,805 km/h** en una prueba de aceleración, pero explica que su simulación de curva no fue viable a causa de inestabilidad del controlador de dirección y utiliza entonces telemetría como entrada para análisis de suspensión ([GASPAR · PDF pág. 4](#gaspar-p004), [GASPAR · PDF pág. 105](#gaspar-p105)). Romano compara los modelos contra un esquema de referencia y experimentos longitudinales, con validación **parcial**, no una comprobación universal de todos los regímenes ([ROMANO · PDF pág. 23](#romano-p023), [ROMANO · PDF pág. 29](#romano-p029)). Díaz de Rábago efectúa análisis estructural FEA y afirma que la validación física sobre un prototipo aún sería un paso posterior ([DIAZ · PDF pág. 13](#diaz-p013), [DIAZ · PDF pág. 95](#diaz-p095)).

**Aplicación a Open Formula.** Conservar un registro de escenarios, señales medidas, márgenes de error y supuestos. En casos de levantamiento de rueda o rebote, verificar balance energético y estados de contacto antes de ajustar valores para mejorar el tiempo de vuelta.

<a id="glosario-técnico-bilingüe"></a>

## Glosario técnico bilingüe

| Inglés / Portugués | Español | Notas y fuente |
|---|---|---|
| Anti-roll bar / barra estabilizadora | Barra estabilizadora | Acoplamiento en roll; [GASPAR · PDF pág. 44](#gaspar-p044) |
| Bump steer | Variación de dirección por recorrido | Fenómeno cinemático; [GAITHER · PDF pág. 13](#gaither-p013) |
| Camber / cambagem | Caída de rueda | Inclinación respecto a vertical; [GAITHER · PDF pág. 9](#gaither-p009) |
| Camber thrust | Fuerza lateral por caída | [GAITHER · PDF pág. 9](#gaither-p009) |
| Caster | Avance del pivote | Geometría de dirección; [GAITHER · PDF pág. 16](#gaither-p016) |
| Combined slip | Deslizamiento combinado | Acoplamiento longitudinal/lateral; [ROMANO · PDF pág. 12](#romano-p012) |
| Contact patch | Huella de contacto | Región de transmisión de fuerzas; [ROMANO · PDF pág. 5](#romano-p005) |
| Critical damping / amortecimento crítico | Amortiguación crítica | Referencia de respuesta oscilatoria; [GASPAR · PDF pág. 42](#gaspar-p042) |
| Heave | Movimiento vertical común | Movimiento vertical del chasis; [DIAZ · PDF pág. 59](#diaz-p059) |
| Kingpin inclination | Inclinación del eje de pivote | [GAITHER · PDF pág. 15](#gaither-p015) |
| Load sensitivity | Sensibilidad a la carga | Variación de fuerza con Fz; [GAITHER · PDF pág. 8](#gaither-p008) |
| Motion ratio | Relación de movimiento | Definir convenio explícito; [DIAZ · PDF pág. 12](#diaz-p012) |
| Pacejka / Magic Formula | Fórmula Mágica | Modelo paramétrico de neumáticos; [GASPAR · PDF pág. 75](#gaspar-p075) |
| Pitch | Cabeceo | Giro alrededor del eje lateral; [GASPAR · PDF pág. 81](#gaspar-p081) |
| Ride rate | Rigidez efectiva a rueda | Diferente de constante del muelle; [GAITHER · PDF pág. 21](#gaither-p021) |
| Roll | Balanceo | Giro alrededor del eje longitudinal; [DIAZ · PDF pág. 59](#diaz-p059) |
| Roll center | Centro de balanceo | Propiedad cinemática; [GAITHER · PDF pág. 16](#gaither-p016) |
| Scrub radius | Radio de pivote sobre el suelo | Dirección y fuerzas en volante; [GAITHER · PDF pág. 16](#gaither-p016) |
| Slip angle | Ángulo de deriva | Orientación rueda vs velocidad local; [GASPAR · PDF pág. 32](#gaspar-p032) |
| Slip ratio | Relación de deslizamiento longitudinal | Existen distintas convenciones; [GASPAR · PDF pág. 73](#gaspar-p073) |
| Spring rate | Constante elástica | Distinguir de rigidez efectiva; [DIAZ · PDF pág. 61](#diaz-p061) |
| Sprung mass / massa suspensa | Masa suspendida | Chasis y elementos soportados; [GASPAR · PDF pág. 42](#gaspar-p042) |
| TLLTD | Distribución de transferencia lateral total | [GAITHER · PDF pág. 19](#gaither-p019) |
| Transient tyre response | Respuesta transitoria del neumático | [ROMANO · PDF pág. 4](#romano-p004) |
| Unsprung mass / massa não suspensa | Masa no suspendida | Rueda, partes de freno/mangueta; [GASPAR · PDF pág. 42](#gaspar-p042) |
| Wheel hop | Rebote de rueda | Modo propio rueda/neumático; [DIAZ · PDF pág. 64](#diaz-p064) |
| Yaw | Guiñada | Giro alrededor de eje vertical; [GASPAR · PDF pág. 29](#gaspar-p029) |

<a id="guía-práctica-de-aplicación-a-open-formula"></a>

## Guía práctica de aplicación a Open Formula

### Reglas de trazabilidad

1. **No atribuir parámetros de Formula Student a F1 1997.** Los valores numéricos de las tesis se refieren a prototipos universitarios salvo indicación expresa.
2. **Separar teoría, parámetros, estimaciones y calibración.** Una ecuación científica puede ser pertinente aunque sus coeficientes experimentales no lo sean.
3. **Anotar la fuente exacta de cada ecuación.** Usar el código de fuente y la página original; cotejar con los PDF los símbolos no recuperados correctamente.
4. **Evitar módulos que corrigen dos veces el mismo fenómeno.** Transferencia de cargas, neumáticos, barras estabilizadoras, tercer elemento y aerodinámica deben formar un único sistema de fuerzas.
5. **Validar los transitorios.** Registrar estados de rueda, carga normal, velocidad de suspensión, momento en pitch/roll/yaw y energía total.
6. **Modelar los límites físicos.** Sin tracción con rueda suspendida, sin fuerza normal negativa y sin generación espontánea de energía al cruzar un bache.

### Diagnósticos mínimos recomendados para rebotes/levantamiento de rueda

| Prueba | Señales | Fallo que busca |
|---|---|---|
| Bache rectangular a velocidad fija | Fz, recorrido, velocidad de suspensión y altura rueda | Impulsos discontinuos o contacto duplicado |
| Caída libre controlada | Energía mecánica, velocidad vertical y reacción de neumático | Inestabilidad del integrador o fuerza elástica mal orientada |
| Giro sostenido en suelo plano | Fz por rueda, roll, velocidad angular roll | Distribución de cargas y acoplamiento antiroll |
| Cambio de apoyo izquierda-derecha | Fy, slip angle, relajación y momentos de yaw | Transitorios de neumático demasiado lentos o saltos de signo |
| Frenada con dirección | Fx/Fy y norma combinada | Violación del límite de combined slip |

**Nota:** estas cinco pruebas son una propuesta metodológica para el proyecto, no un protocolo textual establecido en los cuatro documentos.

---

# Biblioteca fuente: texto original íntegro extraíble

**Método:** se transcribe a continuación el texto disponible en la capa textual de cada PDF, sin resumir capítulos ni suprimir ecuaciones deliberadamente. Los saltos de línea conservan aproximadamente la extracción del PDF; las páginas se identifican con un ancla y un encabezado. Algunos párrafos, tablas y símbolos matemáticos quedarán desalineados por las limitaciones inherentes al PDF. Los bloques `text` conservan el espaciado para que los números y las ecuaciones permanezcan reconocibles. No se inventan fórmulas ausentes.

<a id="fuente-gaspar"></a>

## Fuente GAS — Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização do Sistema de Suspensão

**Autor(es):** Diogo Luís Marques Gaspar. **Año:** 2026. **Idioma:** portugués. **Páginas del PDF:** 108. **Documento:** `content.pdf`. **Original:** https://iconline.ipleiria.pt/entities/publication/49669f46-0a29-4bca-8d71-47e1e9b815a8.

### Índice de páginas de esta fuente

[1](#gaspar-p001) · [2](#gaspar-p002) · [3](#gaspar-p003) · [4](#gaspar-p004) · [5](#gaspar-p005) · [6](#gaspar-p006) · [7](#gaspar-p007) · [8](#gaspar-p008) · [9](#gaspar-p009) · [10](#gaspar-p010) · [11](#gaspar-p011) · [12](#gaspar-p012) · [13](#gaspar-p013) · [14](#gaspar-p014) · [15](#gaspar-p015)

[16](#gaspar-p016) · [17](#gaspar-p017) · [18](#gaspar-p018) · [19](#gaspar-p019) · [20](#gaspar-p020) · [21](#gaspar-p021) · [22](#gaspar-p022) · [23](#gaspar-p023) · [24](#gaspar-p024) · [25](#gaspar-p025) · [26](#gaspar-p026) · [27](#gaspar-p027) · [28](#gaspar-p028) · [29](#gaspar-p029) · [30](#gaspar-p030)

[31](#gaspar-p031) · [32](#gaspar-p032) · [33](#gaspar-p033) · [34](#gaspar-p034) · [35](#gaspar-p035) · [36](#gaspar-p036) · [37](#gaspar-p037) · [38](#gaspar-p038) · [39](#gaspar-p039) · [40](#gaspar-p040) · [41](#gaspar-p041) · [42](#gaspar-p042) · [43](#gaspar-p043) · [44](#gaspar-p044) · [45](#gaspar-p045)

[46](#gaspar-p046) · [47](#gaspar-p047) · [48](#gaspar-p048) · [49](#gaspar-p049) · [50](#gaspar-p050) · [51](#gaspar-p051) · [52](#gaspar-p052) · [53](#gaspar-p053) · [54](#gaspar-p054) · [55](#gaspar-p055) · [56](#gaspar-p056) · [57](#gaspar-p057) · [58](#gaspar-p058) · [59](#gaspar-p059) · [60](#gaspar-p060)

[61](#gaspar-p061) · [62](#gaspar-p062) · [63](#gaspar-p063) · [64](#gaspar-p064) · [65](#gaspar-p065) · [66](#gaspar-p066) · [67](#gaspar-p067) · [68](#gaspar-p068) · [69](#gaspar-p069) · [70](#gaspar-p070) · [71](#gaspar-p071) · [72](#gaspar-p072) · [73](#gaspar-p073) · [74](#gaspar-p074) · [75](#gaspar-p075)

[76](#gaspar-p076) · [77](#gaspar-p077) · [78](#gaspar-p078) · [79](#gaspar-p079) · [80](#gaspar-p080) · [81](#gaspar-p081) · [82](#gaspar-p082) · [83](#gaspar-p083) · [84](#gaspar-p084) · [85](#gaspar-p085) · [86](#gaspar-p086) · [87](#gaspar-p087) · [88](#gaspar-p088) · [89](#gaspar-p089) · [90](#gaspar-p090)

[91](#gaspar-p091) · [92](#gaspar-p092) · [93](#gaspar-p093) · [94](#gaspar-p094) · [95](#gaspar-p095) · [96](#gaspar-p096) · [97](#gaspar-p097) · [98](#gaspar-p098) · [99](#gaspar-p099) · [100](#gaspar-p100) · [101](#gaspar-p101) · [102](#gaspar-p102) · [103](#gaspar-p103) · [104](#gaspar-p104) · [105](#gaspar-p105)

[106](#gaspar-p106) · [107](#gaspar-p107) · [108](#gaspar-p108)

### Índice de secciones del documento

- [Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização do Sistema de Suspensão](#gaspar-p001) (PDF pág. 1)
- [Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização do Sistema de Suspensão](#gaspar-p002) (PDF pág. 2)
- [Originalidade e Direitos de Autor](#gaspar-p003) (PDF pág. 3)
- [Resumo](#gaspar-p004) (PDF pág. 4)
- [Abstract](#gaspar-p005) (PDF pág. 5)
- [Lista de tabelas](#gaspar-p012) (PDF pág. 12)
- [Lista de siglas e acrónimos](#gaspar-p013) (PDF pág. 13)
- [Glossário](#gaspar-p017) (PDF pág. 17)
- [1. Introdução](#gaspar-p019) (PDF pág. 19)
- [2. Competição Formula Student](#gaspar-p020) (PDF pág. 20)
  - [2.1. Estrutura da Competição](#gaspar-p020) (PDF pág. 20)
  - [2.2. LART](#gaspar-p021) (PDF pág. 21)
- [3. Estado de Arte](#gaspar-p023) (PDF pág. 23)
  - [3.1. Ferramentas Comerciais](#gaspar-p023) (PDF pág. 23)
    - [3.1.1. Optimum G](#gaspar-p023) (PDF pág. 23)
    - [3.1.2. IPG CarMaker](#gaspar-p024) (PDF pág. 24)
  - [3.2. Ferramentas de Simulação de Desenvolvimento Próprio](#gaspar-p024) (PDF pág. 24)
    - [3.2.1. Vehicle Dynamics Analysis in a Formula Student Racing Car, Henrique Amaral](#gaspar-p024) (PDF pág. 24)
    - [3.2.2. Modelling of a high-performance vehicle in MATLAB/Simulink, Rickard Hoglund](#gaspar-p025) (PDF pág. 25)
- [4. Conceitos de Dinâmica de Veículos](#gaspar-p026) (PDF pág. 26)
  - [4.1. Sistema de Coordenadas](#gaspar-p026) (PDF pág. 26)
  - [4.2.  Forças Longitudinais](#gaspar-p026) (PDF pág. 26)
  - [4.3.  Forças Laterais](#gaspar-p028) (PDF pág. 28)
  - [4.4. Definição do Movimento do Veículo](#gaspar-p029) (PDF pág. 29)
    - [4.4.1. Modelo de Duas Rodas](#gaspar-p029) (PDF pág. 29)
    - [4.4.2. Modelo de Quatro rodas](#gaspar-p031) (PDF pág. 31)
  - [4.5.  Pneus](#gaspar-p032) (PDF pág. 32)
    - [4.5.1. Slip Angle](#gaspar-p033) (PDF pág. 33)
    - [4.5.2. Slip Ratio](#gaspar-p035) (PDF pág. 35)
    - [4.5.3. Camber](#gaspar-p036) (PDF pág. 36)
    - [4.5.4. Resistência ao Rolamento](#gaspar-p037) (PDF pág. 37)
    - [4.5.5. Círculo de Fricção](#gaspar-p037) (PDF pág. 37)
    - [4.5.6. Combined Slip](#gaspar-p038) (PDF pág. 38)
    - [4.5.7. Formula Mágica](#gaspar-p038) (PDF pág. 38)
  - [4.6.  Suspensão](#gaspar-p039) (PDF pág. 39)
    - [4.6.1. Equação do movimento](#gaspar-p040) (PDF pág. 40)
    - [4.6.2. Molas](#gaspar-p041) (PDF pág. 41)
    - [4.6.1. Amortecedor](#gaspar-p042) (PDF pág. 42)
    - [4.6.2. Sistema Anti-Roll Bar](#gaspar-p044) (PDF pág. 44)
    - [4.6.3. Dimensionamento do sistema Anti-Roll bar](#gaspar-p045) (PDF pág. 45)
  - [4.7. Sistema de Travagem](#gaspar-p046) (PDF pág. 46)
  - [4.8. Aerodinâmica](#gaspar-p048) (PDF pág. 48)
  - [4.9. Simulação de tempo por volta](#gaspar-p048) (PDF pág. 48)
    - [4.9.1. Regime Permanente](#gaspar-p049) (PDF pág. 49)
    - [4.9.2. Regime Quase-Permanente](#gaspar-p049) (PDF pág. 49)
    - [4.9.3. Regime Transitório](#gaspar-p050) (PDF pág. 50)
  - [4.10.  Modelo de Espaço de estados](#gaspar-p051) (PDF pág. 51)
- [5. Dados de Referência](#gaspar-p053) (PDF pág. 53)
  - [5.1.  Pneus Hoosier R20](#gaspar-p053) (PDF pág. 53)
  - [5.2.  Formula Student T24e](#gaspar-p058) (PDF pág. 58)
    - [5.2.1. Sistema de aquisição de dados](#gaspar-p060) (PDF pág. 60)
    - [5.2.2. Telemetria das provas FSPT e FSG](#gaspar-p061) (PDF pág. 61)
- [6. Desenvolvimento do Modelo de Comportamento Dinâmico](#gaspar-p064) (PDF pág. 64)
  - [6.1.  Sistema de Travagem](#gaspar-p064) (PDF pág. 64)
  - [6.2.  Sistema de Propulsão](#gaspar-p067) (PDF pág. 67)
  - [6.3. Sistema de Transferência de Cargas](#gaspar-p069) (PDF pág. 69)
  - [6.4.  Sistema de Cálculo do Slip Angle](#gaspar-p070) (PDF pág. 70)
  - [6.5. Sistema de Cálculo das Velocidades Angulares das Rodas](#gaspar-p072) (PDF pág. 72)
  - [6.6.  Sistema de Cálculo do Slip Ratio](#gaspar-p073) (PDF pág. 73)
  - [6.7. Sistema da Fórmula Mágica de Pacejka](#gaspar-p075) (PDF pág. 75)
  - [6.8.  Sistema de Cálculo das Acelerações](#gaspar-p079) (PDF pág. 79)
  - [6.9. Modelo de Comportamento da Suspensão](#gaspar-p081) (PDF pág. 81)
  - [6.10. Modelo do Condutor](#gaspar-p086) (PDF pág. 86)
  - [6.11. Sistema Completo](#gaspar-p088) (PDF pág. 88)
- [7. Simulações](#gaspar-p091) (PDF pág. 91)
  - [7.1.  Simulação da Prova de Aceleração](#gaspar-p091) (PDF pág. 91)
  - [7.2.  Simulação da Prova de Autocross](#gaspar-p096) (PDF pág. 96)
    - [7.2.1. Análise do camber dinâmico](#gaspar-p102) (PDF pág. 102)
    - [7.2.2. Implementação de Anti Roll bar](#gaspar-p104) (PDF pág. 104)
- [8. Conclusão](#gaspar-p105) (PDF pág. 105)
  - [8.1. Trabalhos Futuros](#gaspar-p106) (PDF pág. 106)
- [9. Bibliografia](#gaspar-p107) (PDF pág. 107)

### Índice de figuras y tablas localizadas en el texto

- [PDF pág. 22](#gaspar-p022): Figura 1 - Formula Student T24e.
- [PDF pág. 26](#gaspar-p026): Figura 2 - Referencial ortogonal do veículo, (Domingues dos Santos et al., 2014).
- [PDF pág. 27](#gaspar-p027): Figura 3 - Diagrama de corpo livre do veículo (DCL), (Https://News.Cision.Com/Koenigsegg/r/, n.d.).
- [PDF pág. 28](#gaspar-p028): Figura 4 - DCL lateral do veículo.
- [PDF pág. 29](#gaspar-p029): Figura 5 - Modelo de duas rodas sem acelerações, (Mastinu & Ploechl, 2014).
- [PDF pág. 30](#gaspar-p030): Figura 6 - Modelo de duas rodas com três graus de liberdade, (Ramanata, 1998).
- [PDF pág. 31](#gaspar-p031): Figura 7 - Modelo planar de quatro rodas.
- [PDF pág. 33](#gaspar-p033): Figura 8 - Ilustração do slip Angle,(Tyre Slip Angle – Geometry Explained Suspension Secrets, 2024).
- [PDF pág. 34](#gaspar-p034): figura 9 para diferentes cargas sobre o pneu.
- [PDF pág. 34](#gaspar-p034): Figura 9 - Relação entre o slip angle e a força lateral(Tyre Slip Angle – Geometry Explained
- [PDF pág. 35](#gaspar-p035): Figura 10 - Aderência longitudinal face ao slip ratio.
- [PDF pág. 36](#gaspar-p036): Figura 11 - Camber thrust, (Damian Harty & Michael Blundell, 2015).
- [PDF pág. 38](#gaspar-p038): Figura 12 - Círculo de fricção.
- [PDF pág. 40](#gaspar-p040): Figura 13 - Modelo do sistema com um grau de liberdade, (Rodrigues, 2016).
- [PDF pág. 42](#gaspar-p042): Figura 14 - Diagrama de simplificação da rigidez da suspensão, (H. Marzbani & Reza N. Jazar,
- [PDF pág. 44](#gaspar-p044): Figura 15 - Curva de amortecimento de um amortecedor de competição, (Domingues dos Santos et
- [PDF pág. 45](#gaspar-p045): Figura 16 - Barra de torção, adaptado de (H. Marzbani & Reza N. Jazar, 2024).
- [PDF pág. 50](#gaspar-p050): Figura 17 - Círculo de fricção combinado(Amaral, 2022).
- [PDF pág. 53](#gaspar-p053): Figura 18 - Código para interpretação dos dados de forças laterais produzidas pelos pneus.
- [PDF pág. 54](#gaspar-p054): Figura 19 - Gráfico 2D Fy vs SA.
- [PDF pág. 55](#gaspar-p055): Figura 20 - Parâmetros laterais do pneu Hoosier R20 para formulação da fórmula mágica.
- [PDF pág. 55](#gaspar-p055): Figura 21 - Gráfico 3D Fy vs Fz vs SA.
- [PDF pág. 56](#gaspar-p056): Figura 22 - Parâmetros longitudinais do pneu Hoosier R20 para formulação da fórmula mágica.
- [PDF pág. 57](#gaspar-p057): Figura 23 - Força Longitudinal em função do Slip Ratio e da carga Fz.
- [PDF pág. 57](#gaspar-p057): Figura 24 - Coeficiente de fricção em função do SR e de Fz.
- [PDF pág. 58](#gaspar-p058): Figura 25 - Dimensões do T24e.
- [PDF pág. 59](#gaspar-p059): Tabela 1 - Dados do T24e.
- [PDF pág. 60](#gaspar-p060): Figura 26 - Módulo MTi-680G.
- [PDF pág. 61](#gaspar-p061): Figura 27 - Localização do MTi no veículo.
- [PDF pág. 62](#gaspar-p062): Figura 28 - Dados do teste de aceleração [km/h].
- [PDF pág. 62](#gaspar-p062): Figura 29 - Dados da velocidade do T24e.
- [PDF pág. 63](#gaspar-p063): Figura 30 – Dados da aceleração do T24e.
- [PDF pág. 63](#gaspar-p063): Figura 31 - Dados de yaw do T24e.
- [PDF pág. 65](#gaspar-p065): Tabela 2 - Dados do sistema de travagem.
- [PDF pág. 65](#gaspar-p065): Figura 32 - Fluxograma do dimensionamento do sistema de travagem.
- [PDF pág. 66](#gaspar-p066): Figura 33 - Esquema das forças a atuar no sistema de travagem desenvolvido.
- [PDF pág. 66](#gaspar-p066): Figura 34 - Modelação do sistema de travagem.
- [PDF pág. 67](#gaspar-p067): figura 35 encontra-se presente a curva de potência deste motor.
- [PDF pág. 67](#gaspar-p067): Figura 35 - Curva de potência do motor Emrax 228, (Emrax, 2025).
- [PDF pág. 67](#gaspar-p067): Figura 36 - Esquema do sistema de propulsão do T24e.
- [PDF pág. 68](#gaspar-p068): Figura 37 - Modelo do sistema de propulsão.
- [PDF pág. 70](#gaspar-p070): Figura 38 - Esquema de funcionamento do modelo de cargas verticais.
- [PDF pág. 70](#gaspar-p070): Figura 39 - Modelo das cargas verticais.
- [PDF pág. 71](#gaspar-p071): figura 40 está representado o esquema de funcionamento deste modelo.
- [PDF pág. 71](#gaspar-p071): Figura 40 - Esquema de funcionamento do modelo de cálculo do slip angle.
- [PDF pág. 72](#gaspar-p072): Figura 41 - Sistema de cálculo do Slip Angle.
- [PDF pág. 72](#gaspar-p072): Figura 42 - Esquema de funcionamento do modelo de cálculo das velocidades angulares.
- [PDF pág. 73](#gaspar-p073): Figura 43 - Sistema de cálculo das velocidades angulares.
- [PDF pág. 74](#gaspar-p074): Figura 44 - Sistema de cálculo do slip ratio.
- [PDF pág. 75](#gaspar-p075): Figura 45 - Sistema para cálculo do escorregamento longitudinal.
- [PDF pág. 75](#gaspar-p075): Figura 46 - Subsistema dos parâmetros experimentais dos pneus.
- [PDF pág. 76](#gaspar-p076): Figura 47 - Código desenvolvido para cálculo da fórmula mágica.
- [PDF pág. 77](#gaspar-p077): Figura 48 - Código para implementação do círculo de fricção.
- [PDF pág. 77](#gaspar-p077): Figura 49 - Esquema de funcionamento do modelo da fórmula mágica.
- [PDF pág. 78](#gaspar-p078): Figura 50 - Sistema completo para cálculo das forças dos pneus.
- [PDF pág. 80](#gaspar-p080): Figura 51 - Código para definição das matrizes do modelo de acelerações.
- [PDF pág. 81](#gaspar-p081): Figura 52 - Esquema de funcionamento do modelo de cálculo das acelerações.
- [PDF pág. 81](#gaspar-p081): Figura 53 - Modelo de cálculo das acelerações.
- [PDF pág. 85](#gaspar-p085): Figura 54 - Código para cálculo das matrizes do modelo de suspensão.
- [PDF pág. 85](#gaspar-p085): Figura 55 - Código para análise do deslocamento vertical de cada roda.
- [PDF pág. 86](#gaspar-p086): Figura 56 – Esquema de funcionamento do modelo de comportamento da suspensão.
- [PDF pág. 86](#gaspar-p086): Figura 57 - Modelo de comportamento da suspensão.
- [PDF pág. 87](#gaspar-p087): Tabela 3 - Parâmetros dos controladores PID.
- [PDF pág. 87](#gaspar-p087): Figura 58 - Esquema de funcionamento do modelo do condutor.
- [PDF pág. 88](#gaspar-p088): Figura 59 – Modelo simulink do condutor.
- [PDF pág. 89](#gaspar-p089): Figura 60 - Diagrama de interligação dos subsistemas do modelo.
- [PDF pág. 90](#gaspar-p090): Figura 61 - Modelo completo desenvolvido no simulink.
- [PDF pág. 92](#gaspar-p092): Figura 62 - Gráfico comparativo entre a velocidade de referência e a velocidade simulada.
- [PDF pág. 93](#gaspar-p093): Figura 63 - RMSE da simulação da prova de aceleração.
- [PDF pág. 94](#gaspar-p094): Figura 64 - Pitch durante a prova de aceleração.
- [PDF pág. 95](#gaspar-p095): Figura 65 - Gráficos do movimento de heave e das cargas verticais na prova de aceleração.
- [PDF pág. 96](#gaspar-p096): Figura 66 - Gráficos da força e escorregamento longitudinal da roda traseira esquerda na prova de
- [PDF pág. 97](#gaspar-p097): Figura 67 - Dados de referência da prova de autocross face aos dados simulados.
- [PDF pág. 97](#gaspar-p097): figura 68. Nesta abordagem, em vez de calcular a trajetória do veículo de forma
- [PDF pág. 98](#gaspar-p098): Figura 68 - Modelo completo para análise da suspensão.
- [PDF pág. 98](#gaspar-p098): Figura 69 - Gráfico de comparação entre aceleração longitudinal e pitch.
- [PDF pág. 99](#gaspar-p099): Figura 70 - Gráfico de comparação entre aceleração lateral e roll.
- [PDF pág. 100](#gaspar-p100): Figura 71 - Correlação entre Roll e distribuição de carga lateral.
- [PDF pág. 100](#gaspar-p100): Figura 72 - Correlação entre o pitch e a carga longitudinal.
- [PDF pág. 101](#gaspar-p101): Figura 73 - Curso de suspensão nos primeiros 80s da prova de autocross.
- [PDF pág. 102](#gaspar-p102): Figura 74 - Evolução do camber na roda frente esquerda nos primeiros 80s de prova.
- [PDF pág. 103](#gaspar-p103): Figura 75 - Evolução do camber na roda traseira direita nos primeiros 80s de prova.
- [PDF pág. 104](#gaspar-p104): Figura 76 - Comparação dos resultados utilizando anti roll bar.

### Transcripción por página

<a id="gaspar-p001"></a>

#### GAS · PDF página 001 de 108

```text
Modelo de Comportamento Dinâmico do
Veículo Formula Student para otimização
       do Sistema de Suspensão



                                 Projeto

                 Mestrado em Engenharia Automóvel





                   Diogo Luís Marques Gaspar





                                     Leiria, abril de 2026
```

<a id="gaspar-p002"></a>

#### GAS · PDF página 002 de 108

```text
Modelo de Comportamento Dinâmico do
Veículo Formula Student para otimização
       do Sistema de Suspensão


                                 Projeto

                 Mestrado em Engenharia Automóvel





           Diogo Luís Marques Gaspar, Número: 2230422




 Trabalho de Projeto Automóvel realizado sob a orientação do Professor Doutor Sérgio

                                       Santos.




                                     Leiria, abril de 2026
```

<a id="gaspar-p003"></a>

#### GAS · PDF página 003 de 108

```text
  Originalidade e Direitos de Autor



O presente relatório de projeto é original, elaborado unicamente para este fim, tendo sido

devidamente citados todos os autores cujos estudos e publicações contribuíram para o

elaborar.

Reproduções  parciais  deste documento  serão  autorizadas na condição de que  seja

mencionado os Autores e feita referência ao ciclo de estudos no âmbito do qual o mesmo foi

realizado, a saber, Curso de Mestrado em Engenharia Automóvel, no ano letivo 2025/2026,

da Escola Superior de Tecnologia e Gestão do Instituto Politécnico de Leiria, Portugal, e,

bem assim, à data das provas públicas que visaram a avaliação destes trabalhos.





                                                                                                                                                            iii
```

<a id="gaspar-p004"></a>

#### GAS · PDF página 004 de 108

```text
  Resumo



O presente trabalho descreve o desenvolvimento de um simulador de dinâmica de veículos

com 12 graus de liberdade em MATLAB Simulink para um protótipo de Fórmula Student,

motivado pela ausência de ferramentas de simulação, tanto próprias como comerciais,

durante o período de participação na equipa entre 2021 e 2023, e pela ambição de aprofundar

competências em dinâmica de veículos e em MATLAB e Simulink com vista a ingressar no

mercado de trabalho da competição automóvel.

O modelo integra subsistemas de dinâmica longitudinal e lateral, subsistema de movimento

da suspensão e modelo de pneus baseado na Fórmula Mágica de Pacejka, entre outros, sendo

validado com dados de telemetria reais obtidos nas provas da FSPT 2025 e da FSG 2025. A

simulação da prova de aceleração demonstrou que o modelo replica com boa aproximação

o comportamento longitudinal do veículo, obtendo-se um RMSE de 1,805 km/h. A

simulação em curva revelou-se inviável devido à instabilidade do controlador de direção em

condições transientes, tendo o simulador sido reorientado como ferramenta de análise

cinemática, recebendo diretamente dados de telemetria para calcular grandezas tipicamente

não medidas em pista, como os movimentos de pitch, roll e heave da carroçaria e o

deslocamento individual da suspensão em cada roda. A utilidade prática da ferramenta foi

demonstrada através da análise do efeito da introdução de uma barra estabilizadora, onde se

verificou uma redução de 25% no ângulo de roll. A ferramenta desenvolvida representa um

contributo concreto para a equipa, colmatando uma lacuna que persistiu ao longo de vários

anos.





Palavras-chave: Formula Student, Suspensão, Matlab, Desenvolvimento.





                                                                                                                iv
```

<a id="gaspar-p005"></a>

#### GAS · PDF página 005 de 108

```text
  Abstract



This work describes the development of a 12 degrees-of-freedom vehicle dynamics

simulator in MATLAB Simulink for a Formula Student prototype, motivated by the absence

of simulation tools, both in-house and commercial, during the period of participation in the

team between 2021 and 2023, and by the ambition to deepen expertise in vehicle dynamics

and MATLAB and Simulink with the goal of pursuing a career in motorsport engineering.

The model integrates subsystems for longitudinal and lateral dynamics, suspension motion

and a tyre model based on Pacejka's Magic Formula, among others, and was validated

against real telemetry data obtained during the FSPT 2025 and FSG 2025 competitions. The

acceleration event simulation demonstrated that the model replicates the longitudinal

behaviour of the vehicle with good accuracy, achieving an RMSE of 1.805 km/h. The

cornering simulation proved unfeasible due to instability of the steering controller under

transient conditions, leading to the simulator being repurposed as a kinematic analysis tool,

receiving telemetry data directly to compute quantities typically not measured during events,

such as pitch, roll and heave body motions and individual suspension displacement at each

wheel. The practical utility of the tool was demonstrated through the analysis of the effect

of introducing an anti-roll bar, where a 25% reduction in roll angle was observed. The

developed tool represents a concrete contribution to the team, addressing a gap that persisted

over several years.





Keywords: Formula Student, Suspension, Matlab, Development.





                                                                                       v
```

<a id="gaspar-p006"></a>

#### GAS · PDF página 006 de 108

```text
Índice



Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização
do Sistema de Suspensão ...................................................................................................... i

Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização
do Sistema de Suspensão ..................................................................................................... ii

Originalidade e Direitos de Autor ..................................................................................... iii

Resumo ................................................................................................................................ iv

Abstract ................................................................................................................................ v

Lista de tabelas .................................................................................................................. xii

Lista de siglas e acrónimos ............................................................................................... xiii

Glossário ........................................................................................................................... xvii

    Introdução .................................................................................................................... 1

    Competição Formula Student .................................................................................... 2

2.1.   Estrutura da Competição ........................................................................................ 2

2.2.  LART ........................................................................................................................ 3

    Estado de Arte .............................................................................................................. 5

3.1.   Ferramentas Comerciais ......................................................................................... 5
   3.1.1.   Optimum G ........................................................................................................ 5
   3.1.2.   IPG CarMaker ................................................................................................... 6

3.2.   Ferramentas de Simulação de Desenvolvimento Próprio .................................... 6
   3.2.1.    Vehicle Dynamics Analysis in a Formula Student Racing Car, Henrique
  Amaral  6
   3.2.2.    Modelling of a high-performance vehicle in MATLAB/Simulink, Rickard
  Hoglund 7

    Conceitos de Dinâmica de Veículos ............................................................................ 8

4.1.   Sistema de Coordenadas ......................................................................................... 8

4.2.   Forças Longitudinais ............................................................................................... 8

4.3.   Forças Laterais ...................................................................................................... 10

4.4.   Definição do Movimento do Veículo .................................................................... 11
   4.4.1.   Modelo de Duas Rodas .................................................................................... 11
   4.4.2.   Modelo de Quatro rodas .................................................................................. 13



                                                                                                               vi
```

<a id="gaspar-p007"></a>

#### GAS · PDF página 007 de 108

```text
4.5.   Pneus ....................................................................................................................... 14
   4.5.1.    Slip Angle ......................................................................................................... 15
   4.5.2.    Slip Ratio ......................................................................................................... 17
   4.5.3.   Camber ............................................................................................................ 18
   4.5.4.    Resistência ao Rolamento ................................................................................ 19
   4.5.5.    Círculo de Fricção ........................................................................................... 19
   4.5.6.   Combined Slip ................................................................................................. 20
   4.5.7.    Formula Mágica ............................................................................................... 20

4.6.   Suspensão ............................................................................................................... 21
   4.6.1.    Equação do movimento ................................................................................... 22
   4.6.2.   Molas ............................................................................................................... 23
   4.6.1.    Amortecedor .................................................................................................... 24
   4.6.2.    Sistema Anti-Roll Bar ...................................................................................... 26
   4.6.3.    Dimensionamento do sistema Anti-Roll bar .................................................... 27

4.7.   Sistema de Travagem ............................................................................................ 28

4.8.   Aerodinâmica ......................................................................................................... 30

4.9.   Simulação de tempo por volta .............................................................................. 30
   4.9.1.   Regime Permanente ......................................................................................... 31
   4.9.2.   Regime Quase-Permanente.............................................................................. 31
   4.9.3.   Regime Transitório .......................................................................................... 32

4.10.    Modelo de Espaço de estados ............................................................................ 33

    Dados de Referência .................................................................................................. 35

5.1.   Pneus Hoosier R20 ................................................................................................. 35

5.2.   Formula Student T24e .......................................................................................... 40
   5.2.1.    Sistema de aquisição de dados......................................................................... 42
   5.2.2.    Telemetria das provas FSPT e FSG ................................................................. 43

    Desenvolvimento do Modelo de Comportamento Dinâmico ................................. 46

6.1.   Sistema de Travagem ............................................................................................ 46

6.2.   Sistema de Propulsão ............................................................................................ 49

6.3.   Sistema de Transferência de Cargas .................................................................... 51

6.4.   Sistema de Cálculo do Slip Angle ......................................................................... 52

6.5.   Sistema de Cálculo das Velocidades Angulares das Rodas ............................... 54

6.6.   Sistema de Cálculo do Slip Ratio .......................................................................... 55




                                                                                                                          vii
```

<a id="gaspar-p008"></a>

#### GAS · PDF página 008 de 108

```text
6.7.   Sistema da Fórmula Mágica de Pacejka ............................................................. 57

6.8.   Sistema de Cálculo das Acelerações ..................................................................... 61

6.9.   Modelo de Comportamento da Suspensão .......................................................... 63

6.10.    Modelo do Condutor ......................................................................................... 68

6.11.    Sistema Completo .............................................................................................. 70

    Simulações .................................................................................................................. 73

7.1.   Simulação da Prova de Aceleração ...................................................................... 73

7.2.   Simulação da Prova de Autocross ........................................................................ 78
   7.2.1.    Análise do camber dinâmico ........................................................................... 84
   7.2.2.    Implementação de Anti Roll bar ...................................................................... 86

    Conclusão ................................................................................................................... 87

8.1.   Trabalhos Futuros ................................................................................................. 88

     Bibliografia ................................................................................................................. 89





                                                                                                                                viii
```

<a id="gaspar-p009"></a>

#### GAS · PDF página 009 de 108

```text
Lista de Figuras



Figura 1 - Formula Student T24e. ..................................................................................................................... 4

Figura 2 - Referencial ortogonal do veículo, (Domingues dos Santos et al., 2014). ......................................... 8

Figura 3 - Diagrama de corpo livre do veículo (DCL), (Https://News.Cision.Com/Koenigsegg/r/, n.d.).......... 9

Figura 4 - DCL lateral do veículo. ................................................................................................................... 10

Figura 5 - Modelo de duas rodas sem acelerações, (Mastinu & Ploechl, 2014). ............................................. 11

Figura 6 - Modelo de duas rodas com três graus de liberdade, (Ramanata, 1998). ......................................... 12

Figura 7 - Modelo planar de quatro rodas. ...................................................................................................... 13

Figura 8 - Ilustração do slip Angle,(Tyre Slip Angle – Geometry Explained Suspension Secrets, 2024). ....... 15

Figura 9 - Relação entre o slip angle e a força lateral(Tyre Slip Angle – Geometry Explained Suspension
Secrets, 2024). ................................................................................................................................................. 16

Figura 10 - Aderência longitudinal face ao slip ratio. ..................................................................................... 17

Figura 11 - Camber thrust, (Damian Harty & Michael Blundell, 2015). ........................................................ 18

Figura 12 - Círculo de fricção. ........................................................................................................................ 20

Figura 13 - Modelo do sistema com um grau de liberdade, (Rodrigues, 2016). .............................................. 22

Figura 14 - Diagrama de simplificação da rigidez da suspensão, (H. Marzbani & Reza N. Jazar, 2024). ...... 24

Figura 15 - Curva de amortecimento de um amortecedor de competição, (Domingues dos Santos et al., 2014).
......................................................................................................................................................................... 26

Figura 16 - Barra de torção, adaptado de (H. Marzbani & Reza N. Jazar, 2024). ........................................... 27

Figura 17 - Círculo de fricção combinado(Amaral, 2022). ............................................................................. 32

Figura 18 - Código para interpretação dos dados de forças laterais produzidas pelos pneus. ......................... 35

Figura 19 - Gráfico 2D Fy vs SA. ................................................................................................................... 36

Figura 20 - Parâmetros laterais do pneu Hoosier R20 para formulação da fórmula mágica. .......................... 37

Figura 21 - Gráfico 3D Fy vs Fz vs SA. .......................................................................................................... 37

Figura 22 - Parâmetros longitudinais do pneu Hoosier R20 para formulação da fórmula mágica. ................. 38

Figura 23 - Força Longitudinal em função do Slip Ratio e da carga Fz. ......................................................... 39

Figura 24 - Coeficiente de fricção em função do SR e de Fz. ......................................................................... 39

Figura 25 - Dimensões do T24e. ..................................................................................................................... 40

Figura 26 - Módulo MTi-680G. ...................................................................................................................... 42




                                                                                                                ix
```

<a id="gaspar-p010"></a>

#### GAS · PDF página 010 de 108

```text
Figura 27 - Localização do MTi no veículo. ................................................................................................... 43

Figura 28 - Dados do teste de aceleração [km/h]............................................................................................. 44

Figura 29 - Dados da velocidade do T24e. ...................................................................................................... 44

Figura 30 – Dados da aceleração do T24e. ...................................................................................................... 45

Figura 31 - Dados de yaw do T24e. ................................................................................................................. 45

Figura 32 - Fluxograma do dimensionamento do sistema de travagem. ......................................................... 47

Figura 33 - Esquema das forças a atuar no sistema de travagem desenvolvido. ............................................. 48

Figura 34 - Modelação do sistema de travagem. ............................................................................................. 48

Figura 35 - Curva de potência do motor Emrax 228, (Emrax, 2025). ............................................................. 49

Figura 36 - Esquema do sistema de propulsão do T24e. ................................................................................. 49

Figura 37 - Modelo do sistema de propulsão. .................................................................................................. 50

Figura 38 - Esquema de funcionamento do modelo de cargas verticais. ......................................................... 52

Figura 39 - Modelo das cargas verticais. ......................................................................................................... 52

Figura 40 - Esquema de funcionamento do modelo de cálculo do slip angle.................................................. 53

Figura 41 - Sistema de cálculo do Slip Angle. ................................................................................................. 54

Figura 42 - Esquema de funcionamento do modelo de cálculo das velocidades angulares. ............................ 54

Figura 43 - Sistema de cálculo das velocidades angulares. ............................................................................. 55

Figura 44 - Sistema de cálculo do slip ratio. ................................................................................................... 56

Figura 45 - Sistema para cálculo do escorregamento longitudinal. ................................................................. 57

Figura 46 - Subsistema dos parâmetros experimentais dos pneus. .................................................................. 57

Figura 47 - Código desenvolvido para cálculo da fórmula mágica. ................................................................ 58

Figura 48 - Código para implementação do círculo de fricção. ....................................................................... 59

Figura 49 - Esquema de funcionamento do modelo da fórmula mágica. ........................................................ 59

Figura 50 - Sistema completo para cálculo das forças dos pneus. ................................................................... 60

Figura 51 - Código para definição das matrizes do modelo de acelerações. ................................................... 62

Figura 52 - Esquema de funcionamento do modelo de cálculo das acelerações. ............................................ 63

Figura 53 - Modelo de cálculo das acelerações. .............................................................................................. 63

Figura 54 - Código para cálculo das matrizes do modelo de suspensão. ......................................................... 67

Figura 55 - Código para análise do deslocamento vertical de cada roda. ........................................................ 67

Figura 56 – Esquema de funcionamento do modelo de comportamento da suspensão. .................................. 68



                                                                                       x
```

<a id="gaspar-p011"></a>

#### GAS · PDF página 011 de 108

```text
Figura 57 - Modelo de comportamento da suspensão. .................................................................................... 68

Figura 58 - Esquema de funcionamento do modelo do condutor. ................................................................... 69

Figura 59 – Modelo simulink do condutor. ..................................................................................................... 70

Figura 60 - Diagrama de interligação dos subsistemas do modelo. ................................................................. 71

Figura 61 - Modelo completo desenvolvido no simulink. ............................................................................... 72

Figura 62 - Gráfico comparativo entre a velocidade de referência e a velocidade simulada. .......................... 74

Figura 63 - RMSE da simulação da prova de aceleração. ............................................................................... 75

Figura 64 - Pitch durante a prova de aceleração. ............................................................................................. 76

Figura 65 - Gráficos do movimento de heave e das cargas verticais na prova de aceleração. ......................... 77

Figura 66 - Gráficos da força e escorregamento longitudinal da roda traseira esquerda na prova de aceleração.
......................................................................................................................................................................... 78

Figura 67 - Dados de referência da prova de autocross face aos dados simulados. ......................................... 79

Figura 68 - Modelo completo para análise da suspensão. ............................................................................... 80

Figura 69 - Gráfico de comparação entre aceleração longitudinal e pitch. ...................................................... 80

Figura 70 - Gráfico de comparação entre aceleração lateral e roll. ................................................................. 81

Figura 71 - Correlação entre Roll e distribuição de carga lateral. ................................................................... 82

Figura 72 - Correlação entre o pitch e a carga longitudinal. ............................................................................ 82

Figura 73 - Curso de suspensão nos primeiros 80s da prova de autocross. ..................................................... 83

Figura 74 - Evolução do camber na roda frente esquerda nos primeiros 80s de prova. .................................. 84

Figura 75 - Evolução do camber na roda traseira direita nos primeiros 80s de prova. .................................... 85

Figura 76 - Comparação dos resultados utilizando anti roll bar. .................................................................... 86





                                                                                                               xi
```

<a id="gaspar-p012"></a>

#### GAS · PDF página 012 de 108

```text
  Lista de tabelas



Tabela 1 - Dados do T24e. .............................................................................................................................. 41

Tabela 2 - Dados do sistema de travagem. ...................................................................................................... 47

Tabela 3 - Parâmetros dos controladores PID. ................................................................................................ 69





                                                                                                                          xii
```

<a id="gaspar-p013"></a>

#### GAS · PDF página 013 de 108

```text
Lista de siglas e acrónimos



   a                Aceleração

  ARB             Anti-Roll Bar

  AutoX           Autocross

  A_y             Aceleração lateral

  A_x             Aceleração longitudinal

  B                Fator de rigidez da Fórmula Mágica

  C                Fator de forma da Fórmula Mágica

   C_crit          Amortecimento crítico

  C_D             Coeficiente de arrasto aerodinâmico

  CAN             Controller Area Network

  CG             Centro de Gravidade

  CSV           Comma-Separated Values

  DCL           Diagrama de Corpo Livre

  DOF            Graus de Liberdade

  ESTG           Escola Superior de Tecnologia e Gestão

  FSAE          Formula SAE

  FSG            Formula Student Germany

  FSPT           Formula Student Portugal

   F_f             Carga sobre o eixo frontal

   F_r             Carga sobre o eixo traseiro





                                                                                                                          xiii
```

<a id="gaspar-p014"></a>

#### GAS · PDF página 014 de 108

```text
F_x             Força longitudinal

F_y             Força lateral

F_z             Carga vertical




g                Aceleração gravítica

GPS            Global Positioning System

h                 Distância do centro de gravidade ao solo

h_aero           Altura do ponto de aplicação da força de arrasto

I_z           Momento de inércia sobre o eixo z

J_e                Inércia do motor

J_T               Inércia total do sistema de transmissão

J_w               Inércia do conjunto roda

k_arb            Rigidez do sistema anti-roll bar

k_arm           Rigidez dos braços da barra estabilizadora

k_bar            Rigidez da barra de torção

k_r              Rigidez combinada da roda e pneu

k_s              Rigidez da mola

k_t              Rigidez do pneu

k_w             Rigidez da roda

LTS            Simulação de tempo por volta

L_f              Distancia do centro de massa ao eixo frontal

L_r              Distancia do centro de massa ao eixo traseiro





                                                                                             xiv
```

<a id="gaspar-p015"></a>

#### GAS · PDF página 015 de 108

```text
M             Massa do veículo

PID              Proporcional, Integral e Derivativo

P_h              Pressão hidráulica

QSS           Regime Quase-Permanente

RMSE          Raiz do Erro Quadrático Médio

SA            Ângulo de escorregamento

SS            Regime Permanente

SR             Escorregamento Longitudinal

R_e            Raio efetivo da roda

R_m            Raio médio da pastilha de travão

R_w            Raio da roda

TS            Regime Transitório

TTC             Tire Test Consortium

T_b              Binário de travagem

T_w             Largura entre vias do eixo

U_f             Relação do diferencial

V_c             Velocidade crítica em curva

V_x             Velocidade longitudinal do veículo

V_y             Velocidade lateral do veículo

W_b             Distância entre eixos

Z_s            Movimento vertical da carroçaria

μ                 Coeficiente de aderência do pneu





                                                                               xv
```

<a id="gaspar-p016"></a>

#### GAS · PDF página 016 de 108

```text
θ              Angulo de pitch

γ              Angulo de roll

δ               Deslocamento





                                                                                            xvi
```

<a id="gaspar-p017"></a>

#### GAS · PDF página 017 de 108

```text
Glossário



                  Elemento de suspensão que liga os dois lados do mesmo eixo,

   Anti-Roll Bar     resistindo ao movimento de roll relativo entre as rodas. Aumenta

                   a rigidez em roll sem alterar a rigidez vertical do eixo.

                    Barra que conecta o pedal de travão às duas bombas principais

                  de travão, permitindo ao condutor ajustar a distribuição da força
   Balance Bar
                  de travagem entre o eixo dianteiro e o eixo traseiro através do

                      ajuste da posição da alavanca.

  Bound         Movimento de compressão do amortecedor.

                 Ângulo formado entre o plano da roda e a vertical, quando

  Camber         observado de frente. Camber negativo inclina o topo da roda

                    para o interior do veículo.

                   Representação gráfica do limite de aderência do pneu, que
   Círculo de
                     define a combinação máxima de forças longitudinais e laterais
   Fricção
                  que o pneu consegue desenvolver para uma dada carga vertical.

                  Condição em que o pneu desenvolve simultaneamente forças

  Combined Slip    longitudinais e laterais, havendo interação entre o

                   escorregamento longitudinal e o escorregamento lateral.

  Diagrama de     Representação esquemática de um corpo isolado com todas as

  Corpo Livre      forças e momentos externos que atuam sobre ele.

                Fenómeno de compressão da suspensão dianteira durante a

  Dive             travagem, devido à transferência de carga longitudinal do eixo

                       traseiro para o dianteiro.

                   Força aerodinâmica vertical dirigida para baixo, gerada pela

  Downforce       geometria da carroçaria e pelos elementos aerodinâmicos do

                      veículo.

  Heave          Movimento vertical de translação da carroçaria do veículo.





                                                                                                        xvii
```

<a id="gaspar-p018"></a>

#### GAS · PDF página 018 de 108

```text
               Comportamento dinâmico em que o eixo traseiro perde
Sobreviragem
                  aderência antes do dianteiro durante uma curva, fazendo com
(oversteer)
                que o veículo rode mais do que o pretendido pelo condutor.

              Movimento de rotação da carroçaria em torno do eixo
Pitch
                   transversal do veículo.

              Movimento de rotação da carroçaria em torno do eixo
Roll
                   longitudinal do veículo.

                  Distância horizontal entre o ponto de contacto do pneu com o
Scrub Radius
                  solo e o ponto onde o eixo de direção interseta o plano do solo.

               Ângulo entre a direção de deslocamento do pneu e o seu plano
Slip Angle
                de rotação.

                Parâmetro adimensional que quantifica o escorregamento

Slip Ratio         longitudinal do pneu, comparando a velocidade periférica da

                 roda com a velocidade linear do veículo.

              Fenómeno de compressão da suspensão traseira durante a

Squat             aceleração, devido à transferência de carga longitudinal do eixo

                   dianteiro para o traseiro.

               Ângulo de orientação da roda em relação ao eixo longitudinal do
Toe
                   veículo, quando observado de cima.

Subviragem     Comportamento dinâmico em que o eixo dianteiro perde

(understeer)      aderência antes do traseiro durante uma curva.

Yaw           Movimento de rotação do veículo em torno do eixo vertical.





                                                                                                          xviii
```

<a id="gaspar-p019"></a>

#### GAS · PDF página 019 de 108

```text
  Introdução



No panorama académico, a Fórmula Student destaca-se como uma das competições de
engenharia mais prestigiadas a nível mundial, desafiando equipas de estudantes a
desenvolver e a construir protótipos de forma a aplicar os conhecimentos adquiridos
durante o curso, bem como a adquirir novas competências e aptidões.

Um dos pontos fulcrais na construção de um Fórmula Student é o desenvolvimento e
posterior afinação do sistema de suspensão. Na ausência de ferramentas de simulação,
estas decisões baseiam-se frequentemente em métodos empíricos e na intuição da equipa,
o que limita o potencial de desenvolvimento do veículo, especialmente face à escassez de
tempo e recursos disponível para testes físicos em pista. Durante o período compreendido
entre 2021 e 2023, no decurso do envolvimento com a equipa, existia precisamente esta
lacuna, a inexistência de uma ferramenta capaz de prever o comportamento dinâmico do
carro ou de quantificar o impacto de alterações de configuração do mesmo.

O presente trabalho surge assim de uma dupla motivação. Por um lado, pretende-se
colmatar esta lacuna através do desenvolvimento de uma ferramenta de simulação que
suporte a tomada de decisão da equipa de forma fundamentada. Por outro, procura-se
aprofundar competências em dinâmica de veículos e em MATLAB e Simulink, uma vez
que são pilares essenciais para posteriormente ingressar no mercado de trabalho da
competição automóvel.

Para cumprir estes objetivos, foi desenvolvido um modelo matemático de 12 graus de
liberdade implementado em Simulink e validado com dados de telemetria reais obtidos
nas provas da FSPT 2025 e da FSG 2025. O modelo integra subsistemas de dinâmica
longitudinal, lateral e vertical, incluindo modelos de pneus e de condutor, permitindo não
só simular o comportamento global do veículo como também calcular e analisar
grandezas tipicamente não medidas em pista.

O documento está organizado em nove capítulos. O Capítulo 2 expõe os fundamentos da
competição Fórmula Student. O Capítulo 3 revê o estado da arte das ferramentas de
simulação existentes. O Capítulo 4 apresenta os fundamentos teóricos de dinâmica de
veículos necessários ao desenvolvimento do trabalho. O Capítulo 5 descreve a forma
como foram obtidos os dados experimentais  utilizados. O Capítulo 6  detalha o
desenvolvimento do modelo e dos seus subsistemas. O Capítulo 7 apresenta as simulações
realizadas para as duas provas distintas e a discussão dos resultados obtidos. Por fim, o
Capítulo 8 expõe as conclusões do trabalho, e o Capítulo 9 as referências.





                                                                                    1
```

<a id="gaspar-p020"></a>

#### GAS · PDF página 020 de 108

```text
  Competição Formula Student


A Formula Student é uma competição de engenharia académica destinada a estudantes
universitários de todo o mundo, centrada no projeto e construção de um monolugar de
competição do tipo fórmula. Proposta pela SAE International (Society of Automotive
Engineers) em 1981, nos Estados Unidos da América, sob a designação de Formula SAE,
a iniciativa rapidamente ganhou projeção internacional, chegando à Europa nas décadas
seguintes. Desde então, a competição não parou de crescer. Atualmente, os eventos
espalham-se  por  diversos  países,  reunindo  anualmente  centenas  de  equipas  de
universidades e institutos politécnicos de todo o globo.

A sua origem responde a uma necessidade real, a de aproximar a formação académica da
prática de engenharia. O percurso universitário, por mais completo que seja, raramente
coloca o estudante perante as exigências de um projeto real como trabalhar em equipa sob
pressão, tomar decisões com consequências físicas e tangíveis, ou lidar com imprevistos
que nenhum manual antecipa. É precisamente neste espaço que a Formula Student se
insere, desafiando os estudantes a desenvolver, construir e testar um protótipo.

   2.1. Estrutura da Competição

A avaliação das equipas divide-se em duas componentes: os eventos estáticos e os eventos
dinâmicos.

Os eventos estáticos avaliam o trabalho de engenharia e de gestão que suporta o protótipo.
Incluem a apresentação de negócio (Business Presentation), a análise de custos (Cost
Report) e a prova de design (Engineering Design), onde a equipa defende perante o júri
as opções técnicas tomadas ao longo do desenvolvimento do veículo. Completam esta
componente as verificações técnicas de segurança, o teste de inclinação e o teste de ruído.

Nos eventos dinâmicos, o veículo é colocado à prova em pista em quatro provas distintas.
A prova de Aceleração consiste em percorrer 75 metros em linha reta a partir do repouso
no menor tempo possível. O Skid Pad avalia a aderência lateral através de um percurso
em forma de oito. O Autocross é uma prova de velocidade num circuito misto de curvas
técnicas e retas, onde se mede a agilidade do conjunto veículo-piloto. Por fim, o
Endurance é a prova mais exigente, com cerca de 22 km, testando a fiabilidade, a
durabilidade e a eficiência energética do protótipo em condições de corrida prolongada.

A competição organiza-se ainda em duas classes distintas, concebidas para acomodar
diferentes graus de maturidade dos projetos. A Classe 1 é a categoria principal, destinada
a equipas que apresentam um protótipo totalmente projetado, construído e funcional,
sujeito às verificações técnicas de segurança e apto a competir em todos os eventos
dinâmicos. A Classe 2, por sua vez, é uma classe de conceito orientada para equipas que
ainda não dispõem de um veículo a funcionar, sendo avaliada apenas nos eventos estáticos
com base no projeto, no planeamento e no desenvolvimento computacional do veículo.



                                                                                    2
```

<a id="gaspar-p021"></a>

#### GAS · PDF página 021 de 108

```text
Esta estrutura permite que equipas mais recentes distribuam por duas épocas o projeto e
a construção de um veículo de Classe 1, tornando mais gradual a curva de aprendizagem.
A classe é ainda acompanhada de um sufixo que identifica o tipo de motorização,
distinguindo os veículos de combustão dos elétricos; é neste contexto que se enquadra a
designação Classe 1E, correspondente a um veículo elétrico de Classe 1.

   2.2. LART

A LART, Leiria Academic Racing Team, é a equipa de Formula Student do Instituto
Politécnico de  Leiria, dedicada ao  projeto e desenvolvimento do monolugar de
competição com sistema de propulsão totalmente elétrico, competindo na Classe 1E. Em
2025, a equipa participou em dois eventos internacionais, a Formula Student Portugal
(FSPT), realizada em Portugal, e a Formula Student Germany (FSG), um dos eventos de
maior prestígio e competitividade a nível mundial, realizado no circuito de Hockenheim,
na Alemanha. A Formula Student Portugal assume particular relevância no presente
trabalho, por ter sido a prova onde foram recolhidos parte dos dados de telemetria
utilizados. Trata-se de um evento recente no calendário, cuja primeira edição decorreu
em 2022 e que, a partir de 2023, passou a realizar-se em Castelo Branco, tornando-se a
primeira competição do género em território nacional. A edição de 2025, da qual provêm
os dados da prova de aceleração utilizados nesta dissertação, correspondeu já à terceira
edição do evento e reuniu mais de seis centenas de estudantes e dezenas de equipas
universitárias de várias nacionalidades, refletindo o rápido crescimento e a consolidação
da competição no panorama internacional.

A presente tese tem como base o veículo T24e, o quarto protótipo desenvolvido e
construído pela LART, de propulsão totalmente elétrica, presente na figura 1. Trata-se de
um monolugar de competição com uma massa total de 330 kg incluindo piloto e uma
distância entre eixos de 1,55 m, cujas características completas servem de base à
parametrização do modelo desenvolvido ao longo deste trabalho.





                                                                                    3
```

<a id="gaspar-p022"></a>

#### GAS · PDF página 022 de 108

```text
Figura 1 - Formula Student T24e.





                                                    4
```

<a id="gaspar-p023"></a>

#### GAS · PDF página 023 de 108

```text
  Estado de Arte


A Fórmula 1 representa o estado de arte do desenvolvimento automóvel baseado em
simulação. Numa disciplina onde os testes em pista são severamente limitados pelo
regulamento desportivo, apenas seis dias de testes de pré-época e quatro horas de treinos
livres por grande prémio, as equipas transferiram grande parte do seu processo de
desenvolvimento para o ambiente virtual.

Esta  cultura de  simulação,  proveniente da Fórmula  1, tem vindo  a  difundir-se
progressivamente pela indústria automóvel mais abrangente  e, naturalmente, pelas
competições académicas. No contexto do Formula Student, a necessidade de simulação
torna-se também de extrema importância uma vez que as equipas têm orçamentos
reduzidos, não existem peças suplentes, e os pilotos não têm experiência de corrida
profissional. Nestas condições, um erro de design ou de afinação do carro que numa
equipa profissional seria corrigido em alguns treinos livres, pode comprometer o bom
desempenho da equipa nas competições. A simulação torna-se, portanto, não um luxo,
mas uma necessidade estrutural, uma vez que permite antecipar o comportamento do
veículo antes do desenvolvimento físico, reduzir o número de iterações em hardware e,
após os testes, extrair de dados de telemetria de grandezas que não foram medidas
diretamente, alargando assim o valor de cada sessão em pista.

Este capítulo apresenta uma revisão das ferramentas e metodologias de simulação mais
relevantes para o contexto da Formula Student, organizada em dois grupos, sendo estes
os  softwares  comerciais  especializados em dinâmica de  veículos  e  os modelos
desenvolvidos internamente por equipas académicas.

   3.1. Ferramentas Comerciais

No panorama das ferramentas comerciais, a escolha da plataforma adequada implica
sempre um compromisso entre o nível de detalhe do modelo, os recursos necessários para
o operar e o custo da licença. As secções seguintes apresentam as ferramentas com maior
presença documentada no contexto da Formula Student, analisando as suas capacidades
e limitações.


        3.1.1. Optimum G
A OptimumG, empresa fundada por um dos júris mais conhecido nas competições de
Formula Student realizadas na europa, Claude Rouelle, oferece diferentes softwares de
simulação, sendo estes o OptimumKinematics para análise cinemática da suspensão, o
OptimumTire para modelação de pneus, e o OptimumLap para simulação de tempo por
volta.

O OptimumLap é uma ferramenta baseada num modelo de massa pontual em regime
quase-permanente. O veículo é definido por 10 parâmetros de entrada, massa, área frontal,
coeficientes aerodinâmicos, raio de roda, resistência ao rolamento, coeficientes de atrito


                                                                                    5
```

<a id="gaspar-p024"></a>

#### GAS · PDF página 024 de 108

```text
longitudinal e lateral, relações de transmissão e curva de binário do motor. O traçado pode
ser gerado a partir de dados de telemetria reais ou descarregado de uma base de dados de
circuitos disponível na plataforma. Os resultados abrangem cerca de 60 canais de saída,
incluindo tempo por volta, perfil de velocidade, consumos energéticos e acelerações,
entre outros. A empresa indica um desvio típico de até 10% face a dados reais, (Optimum
G, 2026).

As limitações do OptimumLap são estruturais, o modelo de massa pontual não contabiliza
a transferência de carga entre rodas, os efeitos transitórios da suspensão, nem incorpora
modelos de pneu semi-empíricos como a Fórmula Mágica de Pacejka, assumindo
coeficientes de atrito constantes.


        3.1.2. IPG CarMaker
O IPG CarMaker é uma plataforma de simulação desenvolvida pela IPG Automotive
especificamente para o desenvolvimento e teste de automóveis ligeiros em todas as fases
do processo de desenvolvimento. O software permite construir um protótipo virtual
completo do veículo, incluindo a dinâmica de veículo, o sistema de propulsão, o modelo
de pneu e o traçado da pista, e combiná-lo com um modelo de condutor adaptativo que
aprende os limites do veículo e do traçado de forma automática. O modelo multicorpo do
veículo permite analisar em detalhe o comportamento da suspensão, calculando os
deslocamentos individuais de cada roda, a variação de camber, a transferência de carga e
os movimentos de roll, pitch e heave da carroçaria em qualquer manobra simulada.

   3.2. Ferramentas de Simulação de Desenvolvimento Próprio

Para além das ferramentas comerciais, diversas equipas de Fórmula Student têm
desenvolvido internamente os seus próprios simuladores de dinâmica de veículos. Esta
abordagem reduz custos, eliminando a dependência de licenças comerciais e aprofunda o
conhecimento técnico da equipa sobre os fenómenos físicos em jogo. Ao contrário dos
softwares pagos, onde o utilizador está limitado às funcionalidades disponibilizadas pelo
fabricante, um modelo desenvolvido internamente pode ser adaptado, expandido e
integrado com dados de telemetria reais sem qualquer restrição, o limite do que se pode
alcançar é determinado pelo conhecimento e pelo esforço da própria equipa. As secções
seguintes apresentam alguns trabalhos relevantes publicados na literatura académica.


        3.2.1. Vehicle Dynamics Analysis in a Formula Student Racing Car,
          Henrique Amaral
Na dissertação desenvolvida por (Amaral, 2022) com base no veículo Formula Student
T14, desenvolvido pela equipa do  Instituto politécnico de Leiria, verificou-se um
exemplo de software de simulação de desenvolvimento próprio.

Neste Amaral descreveu a ferramenta desenvolvida como um simulador de tempo por
volta utilizando o Simulink. Atendendo aos requisitos específicos da equipa, o simulador
foi construído sem a utilização de blocos de dinâmica de veículos predefinidos, ou seja,



                                                                                    6
```

<a id="gaspar-p025"></a>

#### GAS · PDF página 025 de 108

```text
o algoritmo é totalmente  personalizável. O simulador desenvolvido  utilizou uma
abordagem de massa pontual,  isto  é, o chassi  foi convertido num único ponto,
desprezando por exemplo os efeitos de transferência de peso. Apesar dessa limitação, a
simulação utilizou um algoritmo dos componentes do motor definido pelo binário,
relações de transmissão, inércia rotacional e consumo de combustível. Calculou também
as forças aerodinâmicas com um módulo aerodinâmico que controla a força negativa de
sustentação e arrasto presente em cada ponto da pista. O comportamento do pneu foi
definido por um modelo básico que prevê forças longitudinais, laterais e de resistência de
rolamento do pneu, (Amaral, 2022).

Em comparação com a simulação realizada no software CarMaker onde foi obtido um
erro de apenas 6,31% face aos dados reais obtidos em pista. O modelo em Simulink
apresentou desvios maiores, na ordem dos 16% a 17%.

Embora o Simulink tenha erros absolutos maiores, Amaral concluiu que é uma ferramenta
valiosa para prever tendências e comparar o impacto relativo de diferentes componentes
ou configurações de forma rápida e gratuita.


        3.2.2. Modelling of a high-performance vehicle in MATLAB/Simulink,
          Rickard Hoglund
A dissertação de (Höglund, 2022) apresentou o desenvolvimento de um simulador de
dinâmica de veículos no software Simulink. Este estudo focou-se na criação de um
modelo matemático capaz de prever o comportamento do veículo, dividindo o problema
em duas componentes principais: a dinâmica longitudinal e a dinâmica lateral.

O modelo longitudinal recebe uma velocidade de referência e utiliza um controlador PID
para simular o comportamento do condutor (aceleração ou travagem). A partir daqui o
simulador  integra  blocos  para o  motor, embraiagem,  transmissão  e  cálculo de
escorregamento do pneu. O movimento final é calculado através de uma equação de
resistência ao movimento, que contabiliza a força de tração, o arrasto aerodinâmico e a
resistência ao rolamento.

Para o comportamento em curva, adotou-se um modelo de 4 rodas onde este assume o
veículo como uma massa pontual com quatro pontos de contacto com o solo, calculando
a aceleração lateral e o yaw rate. Para a interação pneu-estrada, utilizou-se a Fórmula
Mágica de Pacejka, acoplada ao conceito de círculo de tração (garantindo que o pneu não
excede o seu limite de aderência ao combinar forças longitudinais e laterais).

Comparada à simulação utilizando o software Adams/Car o autor concluiu que o modelo
do simulink foi preciso para situações de baixas velocidades e pouco transientes, tendo
apresentado algumas limitações nas condições inversas de maiores velocidades.





                                                                                    7
```

<a id="gaspar-p026"></a>

#### GAS · PDF página 026 de 108

```text
  Conceitos de Dinâmica de Veículos


   4.1. Sistema de Coordenadas

O estudo da dinâmica de veículos exige a utilização de um sistema de coordenadas que
permita descrever, de forma rigorosa, o movimento e as forças a que está sujeito o veículo.
Sendo assim, é utilizado o sistema de coordenadas ortogonal (x, y, z), fixado no centro
de gravidade do veículo, conforme ilustrado na figura 2, amplamente adotado na literatura
técnica, seguindo a convenção de eixos SAE.





                Figura 2 - Referencial ortogonal do veículo, (Domingues dos Santos et al., 2014).

Os movimentos rotacionais do veículo resultam diretamente das forças aplicadas e da
transferência de peso entre os eixos ou vias. A rotação em torno do eixo y, designada por
pitch, ocorre quando forças longitudinais são aplicadas, durante a aceleração ou travagem,
provocando a transferência de peso entre os eixos frontal e traseiro. Por sua vez, a rotação
em torno do eixo x, conhecida como roll, é induzida por forças laterais, típicas em curvas,
que geram uma transferência de peso entre as vias, levando o veículo a rotacionar sobre
este eixo. Já a rotação em torno do eixo z, ou yaw, ocorre quando as rodas exteriores
apresentam velocidades superiores às rodas interiores, fazendo o veículo descrever uma
trajetória curva e rodar sobre este eixo.



   4.2. Forças Longitudinais

A dinâmica de um veículo em movimento resulta da complexa interação entre três
elementos fundamentais, sendo estes, o próprio veículo, o piso sobre o qual se desloca, e
o ambiente circundante. Por sua vez é possível simplificar esta interação através de um
diagrama de corpo livre, figura 3, onde se define um conjunto de forças equivalentes
aplicadas ao veículo (considerando o mesmo como um corpo rígido).





                                                                                    8
```

<a id="gaspar-p027"></a>

#### GAS · PDF página 027 de 108

```text
        Figura 3 - Diagrama de corpo livre do veículo (DCL), (Https://News.Cision.Com/Koenigsegg/r/, n.d.).



Conforme representado na figura 3, as forças que atuam no veículo contêm pontos de
aplicação distintos originando momentos em torno do centro de massa do veículo e
consequentemente transferências de peso entre os eixos.

Aplicando a segunda lei de Newton, responsável por descrever que a força resultante
aplicada ao veículo é igual ao produto da massa da sua massa pela aceleração a que está
sujeito, obtém-se as seguintes equações:



               𝑚𝑎𝑥= 𝐹𝑥−𝐹𝑎𝑥−𝑚𝑔sin(α𝑟)                         (1)

               𝑚𝑎𝑧= 𝐹𝑧−𝐹𝑎𝑧−𝑚𝑔cos(𝛼𝑟)                         (2)


Sendo 𝐹𝑥 a força de tração do veículo, 𝐹𝑎 a força de arrasto aerodinâmico, 𝛼𝑟 o ângulo de
inclinação do piso e 𝑚𝑔sin(𝛼𝑟) a componente x do peso do veículo e 𝑚𝑔cos(𝛼𝑟) a
componente z.

Calculando o somatório dos momentos sobre cada eixo do veículo obtém-se a força
normal no eixo dianteiro 𝐹𝑧12, e a força normal no eixo traseiro 𝐹𝑧34. 𝐿 é a distância entre
eixos do veículo, h é a altura do centro de massa e ℎaero a altura do ponto de aplicação da
força de arrasto. Desta forma obtém-se as equações seguintes:



                    −𝐹𝑎ℎaero −𝑚𝑎𝑥ℎ−𝑚𝑔ℎsin(α𝑟) + 𝑚𝑔𝑟cos(α𝑟)            (3)
             𝐹𝑧12 =
                                   𝐿


                       𝐹𝑎ℎaero + 𝑚𝑎𝑥ℎ+ 𝑚𝑔ℎsin(α𝑟) + 𝑚𝑔𝑓cos(α𝑟)            (4)
              𝐹𝑧34 =
                                   𝐿




                                                                                    9
```

<a id="gaspar-p028"></a>

#### GAS · PDF página 028 de 108

```text
   4.3. Forças Laterais

No caso de o veículo descrever uma trajetória curva sujeito a uma determinada aceleração
lateral, serão originadas forças centrípetas pelos pneus e consequentemente será exercida
uma força centrífuga sobre o veículo no seu centro de massa, figura 4.





                                     Figura 4 - DCL lateral do veículo.



Assim como anteriormente, recorre-se à segunda lei de Newton para se obter as forças
normais em cada roda do eixo, 𝐹𝑧1 e 𝐹𝑧2. A largura entre vias do eixo é representada por
T.



                      𝑚𝑔                                         (5)                               𝐹𝑧1 =   −𝑚𝑎𝑦ℎ
                              2    𝑇



                      𝑚𝑔  𝑚𝑎𝑦ℎ                                (6)
                               𝐹𝑧2 =   +
                              2    𝑇





                                                                                   10
```

<a id="gaspar-p029"></a>

#### GAS · PDF página 029 de 108

```text
   4.4. Definição do Movimento do Veículo

        4.4.1. Modelo de Duas Rodas
A definição das equações de equilíbrio que permitem descrever o comportamento do
veículo são de extrema importância para o desenvolvimento de um simulador como
pretendido. Para este efeito podem ser utilizados vários modelos, sendo um dos mais
conhecidos o modelo de duas rodas, figura 5.

Quando o veículo se encontra a baixa velocidade, as acelerações longitudinal e lateral
podem ser consideradas desprezáveis, pelo que os pneus não necessitam de gerar forças
laterais e, consequentemente, não se verificam ângulos de escorregamento. Uma vez que
as rodas traseiras não desenvolvem ângulos de escorregamento, a projeção do eixo
traseiro intersecta o centro instantâneo de rotação da trajetória. Nestas condições é
utilizada a versão mais simplificada do modelo de duas rodas.





                Figura 5 - Modelo de duas rodas sem acelerações, (Mastinu & Ploechl, 2014).



A velocidades maiores pode introduzir-se neste modelo as acelerações longitudinais e
laterais a que o veículo está sujeito, figura 6. A aceleração longitudinal irá originar por
parte dos pneus forças longitudinais e a aceleração lateral irá originar ângulos de
escorregamento tanto nos pneus dianteiros como nos pneus traseiros, o que por sua vez
levará à produção de forças laterais. Nesta situação considera-se um modelo de duas rodas
com três graus de liberdade: movimento longitudinal, movimento lateral, e rotação sobre
o eixo z.



                                                                                   11
```

<a id="gaspar-p030"></a>

#### GAS · PDF página 030 de 108

```text
            Figura 6 - Modelo de duas rodas com três graus de liberdade, (Ramanata, 1998).


Segundo o autor (Ramanata, 1998), as equações que descrevem, respetivamente a rotação
sobre o eixo do z, o movimento lateral e o movimento longitudinal são as seguintes:


                                  𝐼θ̈ = 𝑎𝐹η𝑓cos(δ) + 𝑎𝐹ξ𝑓−𝑏𝐹ξ𝑟                         (7)

                   𝑚(𝑉ξ̇ + 𝑉ηθ̇) = 𝐹η𝑓sin(δ) + 𝐹ξ𝑓cos(δ) + 𝐹ξ𝑟                  (8)

                      𝑚(𝑉η̇ −𝑉ξθ̇) = 𝐹η𝑓+ 𝐹η𝑟+ 𝐹ξ𝑓sin(δ)                     (9)



Onde:

  o  𝐹𝜉𝑓, 𝐹𝜉𝑟= força lateral nos pneus dianteiros e traseiros;
  o  𝐹𝜂𝑓, 𝐹𝜂𝑟 = força longitudinal nos pneus dianteiros e traseiros;
  o  a = distância do centro de gravidade ao eixo dianteiro;
  o  b = distância do centro de gravidade ao eixo traseiro;
  o m = massa do veículo;
  o   I = momento de inércia do veículo;
  o  𝑉𝜂, 𝑉𝜉 = velocidade longitudinal/lateral do veículo no referencial;
  o   x, y = posição longitudinal/lateral do veículo no referencial;
  o  δ = ângulo de direção;
  o  η = eixo longitudinal no referencial;
  o  θ = ângulo de rotação;
  o    θ̇ = velocidade de rotação;
  o  ξ = eixo lateral no referencial.





                                                                                   12
```

<a id="gaspar-p031"></a>

#### GAS · PDF página 031 de 108

```text
        4.4.2. Modelo de Quatro rodas
O modelo planar de quatro rodas permite uma análise mais precisa do movimento do
veículo uma vez que são utilizados um maior número de graus de liberdade. Assim como
o nome  indica, neste modelo as quatro rodas do veículo influenciam o mesmo
individualmente, figura 7.





                                 Figura 7 - Modelo planar de quatro rodas.



Considerando a existência de arrasto aerodinâmico apenas sobre o eixo x e aplicando a
segunda lei de Newton, obtém-se as seguintes equações, (Narby, 2006).

Direção x:

       𝑚(𝑣𝑥̇ −𝑣𝑦ψ̇ ) = (𝐹𝑥11 + 𝐹𝑥12) cos(δ) −(𝐹𝑦11 + 𝐹𝑦22) sin(δ) + 𝐹𝑥21                                                                                (10)
            + 𝐹𝑥22 −𝐹𝑎



Direção y:

       𝑚(𝑣𝑦̇ + 𝑣𝑥ψ̇ ) = (𝐹𝑥11 + 𝐹𝑥12) sin(δ) + (𝐹𝑦11 + 𝐹𝑦12) cos(δ) + 𝐹𝑦21
                                                                                (11)
            + 𝐹𝑦22



Momento sobre o eixo z:

          𝐼𝑧ψ̈ = 𝑙𝐹((𝐹𝑥11 + 𝐹𝑥12) sin(δ) + (𝐹𝑦11 + 𝐹𝑦12) cos(δ))
                   −𝑙𝑅(𝐹𝑥21 + 𝐹𝑥22)                                       (12)
                    𝑇𝑤
             +   ((𝐹𝑥11 −𝐹𝑥12) cos(δ) + (𝐹𝑦12 −𝐹𝑦11) sin(δ))
                    2



                                                                                   13
```

<a id="gaspar-p032"></a>

#### GAS · PDF página 032 de 108

```text
Colocando as acelerações sobre cada direção em evidência é possível obter as expressões
que determinam a aceleração do veículo sobre as direções x e y e a aceleração rotacional.



           1
            𝑣𝑥̇ =    ((𝐹𝑥11 + 𝐹𝑥12) cos δ −(𝐹𝑦11 + 𝐹𝑦12) sin δ + 𝐹𝑥21 + 𝐹𝑥22
          𝑚𝑣
                                                                                (13)
             −1 ρ𝐶𝐷𝐴𝑓𝑣𝑥2) + 𝑣𝑦ψ̇
                   2

           1
           𝑣𝑦̇ =    ((𝐹𝑦11 + 𝐹𝑦12) cos δ + (𝐹𝑥11 + 𝐹𝑥12) sin δ + 𝐹𝑦21 + 𝐹𝑦22)
         𝑚𝑣                                                               (14)
                  −𝑣𝑥ψ̇

        1
      ψ̈ =   (𝑙𝐹((𝐹𝑥11 + 𝐹𝑥12) sin δ + (𝐹𝑦11 + 𝐹𝑦12) cos δ) −𝑙𝑅(𝐹𝑦21 + 𝐹𝑦22)
            𝐼𝑧
           + 0.5𝑇𝑤((𝐹𝑥11 −𝐹𝑥12) cos δ + (𝐹𝑦12 −𝐹𝑦11) sin δ          (15)

           + (𝐹𝑥21 −𝐹𝑥22)))



   4.5. Pneus

Os pneus são o único ponto de contacto entre o veículo e o solo, sendo por isso elementos
fundamentais na transmissão de forças entre a estrada e o chassis. Todas as ações
dinâmicas do veículo — aceleração, travagem e curvar — dependem diretamente da
capacidade de os pneus gerarem e transmitirem forças através da sua área de contacto.

Estas forças são desenvolvidas devido às propriedades viscoelásticas da borracha que
compõe o pneu. A borracha apresenta simultaneamente deformação elástica (recuperável)
e viscosa (permanente), o que permite modelar o pneu, de forma simplificada, como um
sistema mola-amortecedor. Esta natureza viscoelástica faz com que o deslocamento do
pneu se encontre em desfasamento relativamente à força aplicada, originando dissipação
de energia sob a forma de calor.

O comportamento da borracha pode ser caracterizado pelo seu módulo de elasticidade
(rigidez) e pela sua perda de energia (histerese), ambos dependentes da temperatura e da
frequência de excitação a que o pneu está sujeito. Esta variabilidade torna o pneu um dos
componentes mais complexos de modelar num veículo. No entanto, é possível aproximar
o seu comportamento através de modelos empíricos ou analíticos, sendo fundamental,
para tal, compreender os mecanismos responsáveis pela geração de aderência e a forma
como as forças resultantes podem ser representadas.





                                                                                   14
```

<a id="gaspar-p033"></a>

#### GAS · PDF página 033 de 108

```text
A aderência entre o pneu e o pavimento não segue um modelo de atrito de Coulomb
clássico com coeficiente constante. Em vez disso, a aderência de um pneu pode ser
decomposta em três componentes principais:

    1.  Atrito – relacionado com o deslizamento e altamente dependente da temperatura
       e da velocidade relativa entre o pneu e o solo.
    2.  Encravamento, conhecido como “Interlocking” – ocorre quando o pneu se
      deforma mecanicamente para se ajustar às micro-irregularidades da superfície da
        estrada.
    3.  Adesão – resulta da ligação temporária entre as cadeias moleculares da borracha
       e o pavimento.

A segunda e terceira componente aumentam com a largura do pneu. Além disso, a
combinação da primeira e segunda componente com as cargas verticais originadas pelo
peso do veículo dá origem à chamada aderência mecânica(Seward, 2014).


        4.5.1.  Slip Angle
O slip angle, representado por α, define-se como o ângulo entre o plano de rotação da
roda e a trajetória imposta pelas condições de aderência entre o pneu e a estrada. Esta
diferença angular surge devido à resistência ao deslizamento oferecida pela borracha do
pneu, que gera uma deformação elástica do contacto com o solo, visível na figura 8.





        Figura 8 - Ilustração do slip Angle,(Tyre Slip Angle – Geometry Explained Suspension Secrets, 2024).



Segundo (Narby, 2006), matematicamente, o slip angle pode ser expresso pela equação
seguinte, onde 𝑉𝑦 representa a velocidade lateral e 𝑉𝑥 a velocidade longitudinal. Este
modelo simplificado permite avaliar o slip angle com base no estado cinemático da roda.




                                                                                   15
```

<a id="gaspar-p034"></a>

#### GAS · PDF página 034 de 108

```text
                                                                                (16)                               tan(α) = −𝑉𝑦
                                               𝑉𝑥



Em regimes de slip angle reduzido, a relação entre força lateral e o slip angle é
aproximadamente linear. A inclinação da reta desta relação é dada pela rigidez lateral do
pneu, ou cornering stiffness, simbolizada por 𝐶𝐹𝛼. No entanto, à medida que o slip angle
cresce, a resposta do pneu torna-se não linear, a força lateral atinge um valor máximo,
correspondente ao pico de aderência lateral, após o qual o pneu entra em saturação e inicia
o deslizamento. Este comportamento é possível ser visualizado no gráfico presente na
figura 9 para diferentes cargas sobre o pneu.





             Figura 9 - Relação entre o slip angle e a força lateral(Tyre Slip Angle – Geometry Explained
                                         Suspension Secrets, 2024).





                                                                                   16
```

<a id="gaspar-p035"></a>

#### GAS · PDF página 035 de 108

```text
        4.5.2.  Slip Ratio
O slip ratio, ou rácio de escorregamento longitudinal, é descrito como a relação entre a
velocidade angular da roda e a velocidade efetiva do veículo quando é aplicado binário
na mesma.

Se o rácio entre a força de aderência longitudinal, 𝐹𝑥, e a carga vertical sobre a roda, 𝐹𝑧,
for representado graficamente em função do slip ratio em percentagem, figura 10,
verifica-se que o pico de aderência ocorre por volta dos 10 a 15% de escorregamento.
Acima deste valor, a aderência diminui à medida que a roda começa a patinar ou
contrariamente bloqueia(Seward, 2014), como se verifica na figura 10.





                              Figura 10 - Aderência longitudinal face ao slip ratio.



Deste modo torna-se necessário calcular a velocidade angular da roda para se obter o
escorregamento longitudinal, sendo que esta é calculada integrando a aceleração angular,
𝜔̇ que é dada pela equação (17). Onde 𝐹𝑥 é a força de atrito, 𝐹𝑟𝑟 é a força de resistência
ao rolamento, 𝑇𝑏 é o binário resultante da travagem e 𝑇𝑤 é o binário proveniente do motor
aplicado no eixo de tração, (Höglund, 2022).



                      𝑇𝑤−𝐹𝑥𝑟𝑤−𝐹𝑟𝑟𝑟𝑤−𝑇𝑏                         (17)
                              ω̇ =
                                              𝐽𝑇





                                                                                   17
```

<a id="gaspar-p036"></a>

#### GAS · PDF página 036 de 108

```text
Assim, sendo o formula student um automóvel de tração traseira é necessário ter em conta
a inércia do sistema de transmissão, que neste caso não contém caixa de velocidades. A
inércia total, 𝐽𝑇, pode ser calculada através da equação (18). 𝐽𝑤 é a inércia de uma roda e
Je é a inércia do motor que por sua vez é multiplicada pela relação do diferencial 𝑈𝑓.



                                                  2                              (18)                                       𝐽𝑒(𝑈𝑓)
                      𝐽𝑇= 𝐽𝑤+
                                   2



        4.5.3. Camber
O camber pode ser definido como o ângulo formado pela inclinação da roda em relação
ao plano longitudinal do veículo. Na figura 11 encontra-se ilustrada uma configuração de
camber negativo, ou seja, o topo da roda inclinado para dentro do veículo, denotado como
γ.





                Figura 11 - Camber thrust, (Damian Harty & Michael Blundell, 2015).



Quando uma roda com camber é submetida a um movimento retilíneo forçado, ocorre
uma distorção na orientação do pneu, resultando na origem de uma força lateral designada
por camber thrust. Este é calculado através da expressão (19), sendo 𝐶𝛾 o coeficiente de
rigidez de camber do pneu (camber stiffness).




                                                                                   18
```

<a id="gaspar-p037"></a>

#### GAS · PDF página 037 de 108

```text
                               Fγ = Cγ ⋅𝛾                                 (19)



Contudo, segundo (Pacejka, 2006), esta componente de força lateral é geralmente de
magnitude significativamente inferior à gerada pelo slip angle, sendo frequentemente
omitida em modelos dinâmicos de veículos devido à sua reduzida relevância. Em
condições de aceleração lateral, a transferência de carga lateral provoca um aumento da
força 𝐹𝑧 na roda exterior e uma diminuição na roda interior. Consequentemente, o camber
thrust da roda exterior assume preponderância, podendo ser combinado com a força de
aderência associada ao ângulo de escorregamento para contrabalançar a força centrífuga.




        4.5.4.  Resistência ao Rolamento
A resistência ao rolamento é caracterizada, tal como o nome indica, como a força de
resistência inerente ao movimento do pneu em rotação livre. Esta ocorre, principalmente,
pela deformação do composto de borracha na zona de contacto com o solo, e sendo
influenciada por diversos fatores, tais como a velocidade e a pressão do pneu.

A influência da resistência ao rolamento pode ser quantificada através do coeficiente de
resistência ao rolamento, designado por 𝐶𝑟𝑟, o qual representa a relação entre a força de
resistência e a carga vertical aplicada ao pneu. Com base neste coeficiente, a força de
resistência ao rolamento 𝐹𝑟𝑟 pode ser estimada pela seguinte expressão:


                          𝐹𝑟𝑟= 𝐶𝑟𝑟⋅𝐹𝑍                                (20)





        4.5.5. Círculo de Fricção
O círculo de fricção, figura 12, também denominado de diagrama g-g, delimita o limite
de tração do pneu em qualquer direção que o mesmo possa tomar.





                                                                                   19
```

<a id="gaspar-p038"></a>

#### GAS · PDF página 038 de 108

```text
                                 Figura 12 - Círculo de fricção.

As forças longitudinais e laterais produzidas pelos pneus não são independentes umas das
outras, sendo geralmente experienciadas em conjunto, por exemplo em aceleração na
saída de uma curva. Contudo, a resultante das forças produzidas por um pneu não pode
exceder o produto da carga normal pelo coeficiente de fricção. Considerando que os pneus
podem apresentar coeficientes de fricção longitudinais e laterais diferentes, os fatores
combinados podem ser expressos pela equação (21), onde 𝜇𝑥 𝑒 𝜇𝑦 são os coeficientes de
fricção longitudinal e lateral e 𝜇𝑥max 𝑒 𝜇𝑦max os respetivos máximos, (Seward, 2014).



                                     2           2                               (21)                           𝜇𝑥        𝜇𝑦
                     (    ) + (    ) ≤1
                               𝜇𝑥max       𝜇𝑦max



Aplicando este diagrama às acelerações longitudinais e laterais é possível verificar se o
condutor se encontra no limite do pneu ou se ainda existe espaço para melhorias de forma
a maximizar a tração disponível.



        4.5.6. Combined Slip
Contrariamente a uma idealização linear, a capacidade de um pneu em gerar a força
máxima num dos eixos é intrinsecamente comprometida pela presença de solicitações no
outro. Este fenómeno, evidencia que o pico de força longitudinal é atingido na ausência
de  componentes   laterais  significativas,  e  vice-versa.  Esta  relação  pode  ser
conceptualizada através do círculo de fricção referido anteriormente.

O escorregamento combinado é definido pela simultaneidade de existência de slip angle
e slip ratio.



        4.5.7. Formula Mágica
A forma tradicional de modelar o comportamento dos pneus em simulações de dinâmica
veicular é através da Fórmula Mágica de Pacejka. Trata-se de um modelo semi-empírico



                                                                                   20
```

<a id="gaspar-p039"></a>

#### GAS · PDF página 039 de 108

```text
amplamente utilizado, que ajusta curvas não lineares a dados experimentais de forças
geradas no contacto do pneu com o solo. O modelo permite estimar a força longitudinal
𝐹𝑥, a força lateral 𝐹𝑧, e o momento de auto-alinhamento, 𝑀𝑧, (Pacejka, 2006).

A fórmula base é dada pela equação seguinte:



            𝑦(𝑥) = 𝐷⋅sin[𝐶⋅arctan(𝐵𝑥−𝐸(𝐵𝑥−arctan(𝐵𝑥)))]           (22)



Onde B representa o fator de rigidez, C o fator de forma, D o valor de pico e E o fator de
curvatura. Os coeficientes são obtidos com base em características específicas das curvas
experimentais: valor de pico da força 𝑦𝑚, valor de slip (angle ou longitudinal) a que o
pico ocorre 𝑥𝑚, o valor para o qual a curva estabiliza a grandes valores de slip 𝑦𝑎 e o
declive na origem 𝑦′(0) que representa a rigidez do pneu na zona linear.

Com estes valores, os parâmetros da equação são calculados da seguinte forma:



                𝐷= 𝑦𝑚= μ𝐹𝑧                                (23)

               𝐶= 2 −2 arcsin (𝑦𝑎                             (24)                           π     𝐷)

                                     𝑦′(0)
                  𝐵=                                        (25)
                         𝐶⋅𝐷

                             π
                                tan (2𝐶) −𝐵𝑥𝑚                            (26)              𝐸=
                            arctan(𝐵𝑥𝑚) −𝐵𝑥𝑚





   4.6. Suspensão

No contexto da competição automóvel, como o caso do Formula Student, o sistema de
suspensão assume um papel central na maximização do desempenho dinâmico do veículo.
A suspensão não só garante o contacto consistente dos pneus com o solo, como também
influencia diretamente o comportamento do veículo em aceleração, travagem e curva,
fatores determinantes para a performance em pista.

A representação precisa do sistema de suspensão em modelos de simulação dinâmicos
permite prever com maior rigor o impacto de diferentes configurações, facilitando o



                                                                                   21
```

<a id="gaspar-p040"></a>

#### GAS · PDF página 040 de 108

```text
processo de afinação e desenvolvimento do veículo. Modelos simplificados podem
fornecer uma primeira aproximação, tratando o veículo como uma massa pontual e a
suspensão como um sistema equivalente de mola e amortecedor. No entanto, para
aplicações mais exigentes, como no caso da competição Formula Student, é fundamental
recorrer a modelos mais detalhados, que considerem as massas suspensas e não suspensas,
a individualidade de cada canto da suspensão, a rigidez dos pneus e a ação de elementos
como as barras estabilizadoras.



        4.6.1. Equação do movimento
O modelo físico mais simples de um sistema vibratório é o modelo linear discreto com
um grau de liberdade, como se representa na figura 13. O modelo inclui o elemento de
inércia de massa m, o elemento elástico de rigidez k, um mecanismo dissipador de energia
de tipo viscoso e de constante C e uma excitação externa f(t) que é função do tempo t. A
posição instantânea do sistema é referenciada pela coordenada x(t) que representa o
deslocamento linear e constitui o grau de liberdade do sistema. A velocidade 𝑥̇(𝑡) e a
aceleração 𝑥̈(𝑡) são dadas, respetivamente, pelas primeira e segunda derivadas de x(t) em
ordem ao tempo(Rodrigues, 2016).





                   Figura 13 - Modelo do sistema com um grau de liberdade, (Rodrigues, 2016).

Deste modo aplica-se o modelo físico anterior ao sistema equivalente de mola e
amortecedor responsável por controlar o movimento relativo entre a massa suspensa e a
massa não suspensa, uma vez que este se trata de um sistema vibratório amortecido.

Considerando o diagrama de corpo livre representado na figura 13 e aplicando a 2ª lei de
Newton, a equação de equilíbrio dinâmico escreve-se:



                        𝑚𝑥̈(𝑡) + 𝑐𝑥̇(𝑡) + 𝑘𝑥(𝑡) = 𝑓(𝑡).                       (27)





                                                                                   22
```

<a id="gaspar-p041"></a>

#### GAS · PDF página 041 de 108

```text
        4.6.2. Molas
As molas têm como função permitir um grau de movimento independente das rodas à
medida que as cargas sobre estas variam consoante as superfícies irregulares. Na
competição automóvel o objetivo é otimizar a aderência da área de contacto do pneu em
cada canto do carro. Isto faz parte do que se pode genericamente designar por compliance.
O movimento das molas ocorre também como consequência da variação das cargas nas
rodas durante a aceleração, em curva e na travagem(Seward, 2014).

A Spring rate ou rigidez da mola, 𝑘𝑠, indica o quanto a mola se deforma por unidade de
força aplicada, normalmente expressa em N/mm.

A Wheel rate, ou rigidez da roda, 𝑘𝑤,encontra-se relacionada com a rigidez da mola
através do Motion Ratio, este indica quanto a mola se desloca comparativamente com o
deslocamento da roda, e é dado pela equação (28).



                                         𝑘𝑤ℎ𝑒𝑒𝑙                               (28)
                      𝑀𝑅2 =
                                          𝑘𝑠𝑝𝑟𝑖𝑛𝑔



A Ride Rate, 𝑘𝑟, é definida como a rigidez da roda combinada com a rigidez do pneu, 𝑘𝑡.



                           1   1   1                                (29)
                     =  +
                             𝑘𝑅   𝑘𝑠   𝑘𝑡



Assumindo um movimento harmónico simples, a frequência natural da vibração do chassi
suspenso é dada por:



                                                                                (30)
                               1 √𝑘𝑅                         𝑓𝑠=
                             2π  𝑚𝑠



Na figura 14 encontra-se demonstrada visualmente a interação entre os parâmetros de
rigidez anteriores.





                                                                                   23
```

<a id="gaspar-p042"></a>

#### GAS · PDF página 042 de 108

```text
           Figura 14 - Diagrama de simplificação da rigidez da suspensão, (H. Marzbani & Reza N. Jazar,
                                                       2024).



Além das rigidezes já faladas, na figura 14 encontra-se ainda presente a massa suspensa,
𝑚𝑠, e a massa não suspensa, 𝑚𝑢. A massa suspensa compreende o chassi, a carroçaria, o
motor, o condutor e os demais componentes associados. Já a massa não suspensa abrange
o conjunto da roda, englobando os pneus, jantes, mangas de eixo, sistema de travagem,
entre outros elementos relacionados. A massa dos componentes que estabelecem a ligação
entre estas duas categorias, como os braços de suspensão e os eixos de transmissão, é
convencionalmente distribuída por meio de uma repartição proporcional entre ambas.


        4.6.1. Amortecedor
Quando uma massa suportada por uma mola é deslocada da sua posição de equilíbrio
estático e libertada, esta tende a oscilar. Na ausência de um amortecedor, essas oscilações
persistem indefinidamente, com amplitude e frequência constantes. Os amortecedores
são, portanto, essenciais para otimizar o contacto do pneu com a superfície, ao dissiparem
a energia cinética acumulada nas molas sob a forma de calor, contribuindo assim para a
estabilidade e controlo do veículo.

O amortecedor é definido pelo seu coeficiente de amortecimento, C, uma medida da força
de amortecimento a uma velocidade de 1 m/s, tendo assim a unidade N/m/s. Se este valor
for superior ao amortecimento critíco, 𝐶𝑐𝑟𝑖𝑡, o sistema é considerado sobreamortecido,
apresentando um retorno lento à posição de equilíbrio. Por outro lado, se C for inferior a
𝐶𝑐𝑟𝑖𝑡, o sistema é subamortecido, permitindo oscilações em torno do ponto de equilíbrio
antes de se estabilizar.

O amortecimento crítico é o valor mínimo necessário para que o sistema regresse à
posição de equilíbrio sem oscilações, e é calculado através da equação seguinte.




                                                                                   24
```

<a id="gaspar-p043"></a>

#### GAS · PDF página 043 de 108

```text
                                             𝐶crit = 4π𝑚𝑠𝑓𝑠                               (31)



Para avaliar a relação entre o amortecimento real e o crítico, define-se o rácio de
amortecimento, ζ, como:



                                         𝐶                                    (32)                           ζ=
                                                                    𝐶crit



Segundo o autor, (Seward, 2014), em veículos de passageiros, valores típicos de ζ situam-
se em torno de 0,25, oferecendo um compromisso aceitável entre conforto e controlo. Em
contextos de competição, onde a prioridade recai sobre o desempenho dinâmico e a
resposta precisa do veículo, é comum adotar rácios de amortecimento na ordem dos 0,65
a 0,7, por se ter demonstrado eficaz na maximização da aderência e previsibilidade do
comportamento do veículo em situações limite.

Nos sistemas convencionais utilizados em competições como o Formula Student, os
amortecedores apresentam ainda diferentes coeficientes de amortecimento para os
movimentos de compressão (bound) e retorno (rebound), bem como para diferentes
velocidades da roda. Este comportamento permite dividir o amortecimento em quatro
regimes distintos:

  o  Compressão em baixa velocidade;
  o  Compressão em alta velocidade;
  o  Retorno em baixa velocidade;
  o  Retorno em alta velocidade.

Estes quatro regimes do comportamento de um amortecedor são visíveis no exemplo dado
no gráfico presente na figura 15.





                                                                                   25
```

<a id="gaspar-p044"></a>

#### GAS · PDF página 044 de 108

```text
          Figura 15 - Curva de amortecimento de um amortecedor de competição, (Domingues dos Santos et
                                                                       al., 2014).



Um aumento do amortecimento em compressão (bound) tem como efeito imediato uma
melhoria da resposta transitória da suspensão, até que se atinja um ponto crítico em que
o amortecedor se aproxima de um estado de bloqueio. Neste regime, a capacidade da
suspensão em absorver irregularidades é severamente comprometida, sendo as forças
transmitidas quase integralmente ao chassis. Este fenómeno anula a função da suspensão,
aproximando o comportamento do veículo ao de um kart, ou seja, sem qualquer filtragem
das perturbações do solo. Por outro lado, um amortecimento de compressão insuficiente
conduz a uma resposta excessivamente branda, dificultando o controlo do veículo em
curva, sobretudo em pisos irregulares. Assim, um compromisso adequado é essencial para
garantir que o automóvel possa ser conduzido de forma previsível e explorado até aos
seus limites com segurança.

No que diz respeito ao amortecimento em extensão (rebound), aplicam-se princípios
semelhantes. Um aumento neste parâmetro reduz a velocidade de retorno da suspensão
após uma compressão, o que pode ser vantajoso em manobras com cargas transitórias
rápidas, como em slaloms, ao limitar o rolamento do chassis. No entanto, um valor
excessivo de amortecimento em extensão pode impedir que a roda recupere contacto com
o solo de forma eficiente, comprometendo a tração e a estabilidade. Este efeito, tal como
no caso da compressão, poderá resultar na perda de eficácia do sistema de suspensão.


        4.6.2. Sistema Anti-Roll Bar
O sistema anti-roll bar, comumente utilizado desde os automóveis de dia a dia até aos
veículos de competição, tem como principal função aumentar a rigidez de rolamento do
veículo, sem aumentar a rigidez quando ambas as rodas do mesmo eixo estão sujeitas à
mesma carga, por exemplo em aceleração ou travagem. O aumento da rigidez de




                                                                                   26
```

<a id="gaspar-p045"></a>

#### GAS · PDF página 045 de 108

```text
rolamento permite assim diminuir a transferência de massa entre vias, e manter o pneu a
funcionar num intervalo de graus de camber mais reduzido e dentro da zona de maior
produção de força lateral.

Para veículos de competição, tem também uma importância significativa na afinação do
equilíbrio do veículo, ao controlar o equilíbrio de transferência de carga longitudinal,
influenciando o comportamento de subviragem ou sobreviragem.

Por sua vez, existe uma clara desvantagem em fornecer muita rigidez ao rolamento
através  deste  sistema. Com  sistemas  muito  rígidos,  a  suspensão  deixa de  ser
verdadeiramente independente. As rodas de um eixo passam a agir como um eixo rígido
em single-wheel bump. Embora a rigidez do sistema de suspensão não afete o movimento
de uma roda individual em bump e rebound, sob movimento de suspensão de uma única
roda, a suspensão torna-se mais rígida e, consequentemente, menos eficaz. Por esta razão,
recomenda-se que o sistema anti-roll bar não contribua com mais de 50% da rigidez ao
rolamento do veículo(Seward, 2014).


        4.6.3. Dimensionamento do sistema Anti-Roll bar
Para  efeitos de dimensionamento, é  essencial determinar com precisão a  rigidez
proporcionada pela barra, tendo em consideração a sua geometria e o material a utilizar
no fabrico da mesma.





                Figura 16 - Barra de torção, adaptado de (H. Marzbani & Reza N. Jazar, 2024).



Segundo (H. Marzbani & Reza N. Jazar, 2024), num cenário típico de rolamento, os
braços da barra movimentam-se em sentidos opostos, o que implica que, por simetria, o
ponto médio da barra de torção permanece fixo. Assim, é comum considerar apenas
metade da barra de comprimento L, para efeitos de cálculo, G o módulo de corte e D o
diâmetro da secção circular da barra, estes parâmetros são visíveis na figura 16. O
objetivo é determinar a rigidez no ponto de aplicação da força, situado no topo de um
braço com comprimento A.



                                                                                   27
```

<a id="gaspar-p046"></a>

#### GAS · PDF página 046 de 108

```text
                             π𝐺𝐷4                                (33)
                        𝐾𝑏𝑎𝑟=                                32𝐿𝐴2



Adicionalmente, os próprios braços da barra deformam-se sob carga, comportando-se
como vigas. A deflexão, δ, de um braço com comprimento A, sujeito a uma força pontual
F, é dada pela equação 34, representado E o módulo de elasticidade e I o segundo
momento de área da secção transversal do braço.



                               𝐹𝐴3                                  (34)
                              δ =
                                 3𝐸𝐼



Desta forma, a rigidez dos braços do sistema de anti-roll bar é dada pela equação (35).



                              3𝐸π𝐷4                               (35)
                      𝐾𝑎𝑟𝑚=                                 64𝐴3



Por fim, a rigidez do sistema de anti-roll bar é dado pela equação 36, uma vez que a barra
de torção e os braços atuam em série.



                               𝐾𝑏𝑎𝑟⋅𝐾𝑎𝑟𝑚                             (36)
                      𝐾𝑎𝑟𝑏=
                            𝐾𝑏𝑎𝑟+ 𝐾𝑎𝑟𝑚




   4.7. Sistema de Travagem

O sistema de travagem de um veículo Formula Student é composto por dois circuitos
hidráulicos independentes, um para o eixo dianteiro e outro para o eixo traseiro. Ambos
são acionados simultaneamente através do pedal de travão, presente na balance bar. A
balance bar é uma barra presente na pedaleira que liga o pedal de travão às duas bombas
principais de travão, permitindo regular a distribuição da pressão hidráulica entre os dois
eixos através do ajuste do ponto de articulação da barra.

Quando o condutor pressiona o pedal, a força aplicada é então dividida entre as duas
bombas principais presentes na balance bar. Segundo (Limpert, 2011), a pressão



                                                                                   28
```

<a id="gaspar-p047"></a>

#### GAS · PDF página 047 de 108

```text
hidráulica gerada no circuito é diretamente proporcional à força aplicada no pedal, sendo
determinada pelo rácio do pedal e pela área do pistão da bomba principal. Cada uma das
bombas gera uma pressão hidráulica que é transmitida às pinças de travão respetivas,
resultando numa força de contacto entre as pastilhas e os discos. Essa força, combinada
com o atrito entre as pastilhas e o disco, origina o binário resistente que atua sobre as
rodas.

A distribuição da força de travagem entre eixos é um parâmetro crítico para a segurança
e eficiência do sistema. De acordo com (Limpert, 2011), a distribuição ótima de travagem
é aquela que garante que as rodas dianteiras bloqueiam antes das traseiras, assegurando a
estabilidade direcional do veículo durante a travagem.

Segundo (Amaral, 2022), para efeitos de simulação, é possível simplificar este processo
assumindo que:

  o A área do pistão da bomba secundária corresponde à área de contacto da pastilha;
  o O coeficiente de atrito entre a pastilha e disco permanece constante durante a
       travagem,
  o O sistema opera em condições ideais (sem perdas ou deformações significativas).

O binário de travagem aplicado a cada roda pode ser calculado com base na força
transmitida pelas bombas secundárias ao disco de travão. A formulação simplificada
apresentada a seguir assume que a área de contacto da pastilha é igual à área do pistão, e
que o coeficiente de atrito se mantém constante durante a travagem, (Amaral, 2022).



                      𝑇𝑏= 𝐹𝑏⋅𝑅𝑚                                (37)


                   𝐹𝑏= 𝐴𝑝⋅𝑝ℎ⋅μ𝑝⋅𝑛𝑝                            (38)



Onde:

    •  𝑇𝑏 = Binário de travagem [Nm];

    •  𝐹𝑏 = Força de travagem [N];

    •  𝑅𝑚 = Raio médio da pastilha [m];

    •  𝐴𝑝 = Área do pistão do travão [m²];

    •  𝑃ℎ = Pressão hidráulica [Pa];

    •  𝜇𝑝 = Coeficiente de atrito pastilha-disco;

    •  𝑛𝑝 = Número de pastilhas de travão.




                                                                                   29
```

<a id="gaspar-p048"></a>

#### GAS · PDF página 048 de 108

```text
   4.8. Aerodinâmica

A aerodinâmica é um dos pontos de estudo presente na dinâmica de veículos que maior
relevância tem ganho com a evolução dos automóveis. Apesar de não se ter como objetivo
o estudo aerodinâmico do Formula Student, é necessário definir, por via de equações, a
interação entre o ar e o veículo.

Duas forças principais influenciam o comportamento do veículo: a força de arrasto e a
força de sustentação (ou downforce). A força de arrasto é oposta ao movimento do
veículo, resultante da diferença de pressão do ar entre a frente e a traseira do carro e do
atrito viscoso do ar sobre a superfície. Sendo calculada pela equação (39), onde ρ é a
massa volúmica do ar, 𝐶𝐷 é o coeficiente de arrasto do veículo e A é a área frontal do
veículo. Assume-se que em condições normais a massa volúmica do ar é de 1,225 kg/m³.



                              1                                       (39)
                      𝐹𝑎=  ρ𝐶𝐷𝐴𝑣2
                              2



A força de sustentação, por sua vez, é gerada pela diferença de pressão entre a parte
inferior e superior do veículo. No caso do Formula Student, procura-se gerar downforce,
ou seja, uma força com sentido negativo (tendo em conta o sistema de coordenadas
apresentado previamente) que pressiona o formula sobre o piso, aumentando a carga sobre
os pneus e consecutivamente a capacidade de aceleração lateral. Esta força é expressa
pela equação (40), onde 𝐶𝐿 representa o coeficiente de sustentação, (Guiggiani, 2014).



                              1                                       (40)
                      𝐹𝐿=  ρ𝐶𝐿𝐴𝑣2
                              2




   4.9. Simulação de tempo por volta

A simulação de tempo de volta (Lap Time Simulation - LTS) é amplamente utilizada no
desporto motorizado para prever e analisar o comportamento dinâmico de um veículo
num traçado. Permite reduzir falhas de conceção, minimizar custos com protótipos e
diminuir a necessidade de testes reais, aumentando a eficiência e a consistência dos
resultados.

Existem três abordagens principais de LTS, cada uma com diferentes pressupostos,
complexidade e aplicações:




                                                                                   30
```

<a id="gaspar-p049"></a>

#### GAS · PDF página 049 de 108

```text
  o  Regime Permanente (Steady-State, SS);
  o  Regime Quase-Permanente (Quasi-Steady State, QSS);
  o  Regime Transitório (Transient State, TS).


        4.9.1. Regime Permanente
A abordagem em regime permanente constitui o método mais simples e amplamente
utilizado na simulação de tempo por volta. Este método assume que o veículo se encontra,
em todos os instantes, em equilíbrio dinâmico.

O circuito é discretizado em segmentos de reta e curvas com raio constante. Nas retas, a
aceleração longitudinal é considerada constante, sendo a velocidade final obtida pela
equação (41), onde 𝑣0 representa a velocidade inicial, 𝑎𝑥 a aceleração longitudinal e d o
deslocamento.



                                                                                (41)
                 𝑣= √𝑣02 + 2𝑎𝑥𝑑



Em situação de curva, considera-se uma velocidade constante, denominada velocidade
crítica (𝑣𝑐), que depende da massa do veículo (m), do raio da curva (𝑅𝑐) e da força lateral
máxima transmitida pelos pneus (𝐹𝑦).



                                                                                (42)
                             √𝐹𝑦⋅𝑅𝑐                      𝑣𝑐=
                      𝑚



A modelação do veículo é geralmente realizada através de um modelo de massa pontual,
que utiliza unicamente os coeficientes de atrito longitudinal e lateral. Neste contexto, as
forças máximas são função direta destes coeficientes e da força normal.

Embora esta metodologia seja útil em fases iniciais de desenvolvimento e análise de
desempenho,  apresenta  limitações  significativas  na  representação   realista  do
comportamento  dinâmico,  especialmente em  fases  transitórias como  aceleração,
travagem e variações rápidas de trajetória.


        4.9.2. Regime Quase-Permanente
O regime quase-permanente constitui uma evolução do método em regime permanente,
permitindo a inclusão de efeitos transitórios de forma simplificada. Nesta metodologia, o
comportamento do  veículo é modelado como uma sequência de  estados quase-
estacionários ao longo de curtos intervalos da pista. Para cada segmento, calcula-se a



                                                                                   31
```

<a id="gaspar-p050"></a>

#### GAS · PDF página 050 de 108

```text
curvatura local e aplica-se uma solução estática, seguida da transição para o segmento
seguinte.

Neste método em vez de considerar forças constantes do pneu em regime estacionário,
recorre-se a uma formulação baseada no círculo de fricção, conjugada com o modelo
Pacejka Magic Formula. Deste modo permite representar de forma mais realista a
interação entre as forças longitudinais e laterais no pneu, figura 17, refletindo com maior
precisão o seu comportamento dinâmico(Amaral, 2022).





                    Figura 17 - Círculo de fricção combinado(Amaral, 2022).



        4.9.3. Regime Transitório
Em condições reais, os veículos encontram-se sujeitos a acelerações nos 3 eixos de
movimento referidos anteriormente, por exemplo ao descrever uma curva, desta forma
raramente se encontram em regime estacionário. Por conseguinte, a abordagem em
regime transitório baseia-se numa simulação dinâmica completa do comportamento do
veículo, incorporando todos os efeitos transitórios relevantes. Esta abordagem requer a
modelação detalhada dos vários subsistemas do veículo, nomeadamente a suspensão, os
modelos de pneu, os efeitos aerodinâmicos e as entradas do condutor. Para resolver as
equações de movimento associadas, é necessário recorrer à integração numérica com
passos temporais reduzidos, de forma a captar com precisão a resposta dinâmica do
sistema.

Apesar da complexidade inerente a este tipo de modelação, diversos estudos têm
demonstrado que os resultados obtidos ao nível do tempo por volta apresentam diferenças
pouco significativas face a abordagens simplificadas, com discrepâncias típicas na ordem
dos 0,77%(Amaral, 2022). Importa, contudo, interpretar este valor com cautela: embora
a diferença no tempo por volta seja reduzida, o tempo por volta é uma métrica global que
esconde diferenças locais relevantes. Um modelo transitório capta a evolução instantânea
de grandezas como as transferências de carga, os ângulos de camber ou o deslocamento
da suspensão em cada instante, informação que uma abordagem simplificada não fornece
e que é precisamente a mais útil para a afinação do veículo. Por esse motivo, a utilização




                                                                                   32
```

<a id="gaspar-p051"></a>

#### GAS · PDF página 051 de 108

```text
de modelos transitórios torna-se particularmente vantajosa em contextos que exigem uma
análise de desempenho detalhada ou na afinação precisa da configuração do veículo.



   4.10.    Modelo de Espaço de estados

O modelo de espaço de estados é uma ferramenta fundamental na modelação e simulação
de sistemas dinâmicos, como aquele que se pretende realizar utilizando o software
Simulink.

Apesar de serem utilizadas equações de segunda ordem ou superior para descrever a
dinâmica de um veículo, estas podem ser expressas matematicamente como um sistema
de equações diferenciais de primeira ordem. Um sistema que é descrito por um sistema
de equações diferenciais de primeira ordem diz-se estar na forma de espaço de estados.
Conhecer o estado de um sistema torna possível calcular todos os sinais de saída futuros
se todos os sinais de entrada futuros forem conhecidos, (Narby, 2006). Um sistema linear
de tempo contínuo na forma de espaço de estados pode ser escrito como:



                                   𝑥̇(𝑡) = 𝐴𝑥(𝑡) + 𝐵𝑢(𝑡)                           (43)

                          𝑦(𝑡) = 𝐶𝑥(𝑡) + 𝐷𝑢(𝑡)                           (44)



onde x é o vetor de estado, u é o vetor de sinal de entrada e y é o vetor de sinal de saída.
As matrizes A, B, C e D definem a dinâmica do sistema e têm significados físicos distintos
(Ogata, 2022).

A matriz A, de dimensão n×n onde n é o número de estados, designada por matriz de
estado, descreve a dinâmica interna do sistema, traduzindo de que forma os estados atuais
influenciam a sua própria evolução temporal. Os seus coeficientes são determinados
diretamente pelas equações de movimento do sistema, sendo função da rigidez, dos
coeficientes de amortecimento e das massas envolvidas.

A matriz B, de dimensão n×m onde m é o número de entradas, designada por matriz de
entrada, relaciona as entradas externas do sistema com a evolução dos estados.

A matriz C, de dimensão p×n onde p é o número de saídas, designada por matriz de saída,
define quais os estados ou combinações de estados que constituem as saídas do sistema,
tomando a forma de uma matriz identidade quando se pretende observar diretamente
todos os estados.

Por fim, a matriz D, de dimensão p×m, designada por matriz de transmissão direta,
representa a influência direta das entradas nas saídas sem passar pelos estados, sendo nula




                                                                                   33
```

<a id="gaspar-p052"></a>

#### GAS · PDF página 052 de 108

```text
na maioria dos sistemas físicos, uma vez que as entradas apenas afetam as saídas através
da dinâmica interna do sistema.

Para obter estas matrizes a partir das equações de movimento, reescreve-se cada equação
de segunda ordem como duas equações de primeira ordem, introduzindo a derivada do
estado como um novo estado (Ogata, 2022). Por exemplo, para um sistema massa-mola-
amortecedor com deslocamento z, define-se o vetor de estados como 𝑥= [𝑧, 𝑧˙]𝑇, do qual
resulta  diretamente  a  forma  matricial  do  sistema.  Este  processo  é  aplicado
sistematicamente a cada grau de liberdade do modelo, sendo o vetor de estados final a
concatenação dos  deslocamentos  e  velocidades de  todos  os  graus de  liberdade
considerados, como será detalhado na secção 6.9.





                                                                                   34
```

<a id="gaspar-p053"></a>

#### GAS · PDF página 053 de 108

```text
  Dados de Referência


Neste capítulo pretende-se apresentar todos os passos necessários para a obtenção de
dados reais e tratamento dos mesmos relativamente aos pneus Hoosier 20.5x7.0-13 R20
e de telemetria do T24e. Estes são estritamente necessários para a correta configuração
do simulador que se pretende desenvolver.

   5.1. Pneus Hoosier R20

Para a obtenção dos dados dos pneus, a equipa do Formula Student disponibilizou o
acesso ao fórum formula SAE tire test consortium (FSAE TTC) que se trata de uma
iniciativa colaborativa sem fins lucrativos criada para disponibilizar dados experimentais
de pneus de elevada qualidade às equipas participantes na competição FSAE.

Os ensaios a cada pneu são realizados na instalação da empresa Calspan Tire Research
Facility  (TIRF), em  Buffalo, Nova  Iorque,  e  os  resultados  obtidos  nos  testes
experimentais são publicados posteriormente no fórum. Devido a estes serem publicados
em formato CSV é necessário desenvolver código em Matlab para que se consiga analisar
os mesmos. Na figura 18 verifica-se o código desenvolvido para extrair os dados
pretendidos através do Matlab.





             Figura 18 - Código para interpretação dos dados de forças laterais produzidas pelos pneus.





                                                                                   35
```

<a id="gaspar-p054"></a>

#### GAS · PDF página 054 de 108

```text
A figura 19 apresenta um gráfico 2D da relação entre a força lateral produzida pelo pneu
face ao slip angle para diferentes cargas sobre o pneu.





                                       Figura 19 - Gráfico 2D Fy vs SA.



As cargas Fz apresentadas são aquelas que foram utilizadas para testar os pneus, sendo
depois necessário interpolar os dados para as diferentes cargas que os pneus podem estar
sujeitos durante a utilização real em pista. Esta interpolação é realizada através da
formulação da fórmula mágica de Pacejka apresentada no capítulo anterior, deste modo
obteve-se os 4 parâmetros dos pneus necessários, figura 20.





                                                                                   36
```

<a id="gaspar-p055"></a>

#### GAS · PDF página 055 de 108

```text
        Figura 20 - Parâmetros laterais do pneu Hoosier R20 para formulação da fórmula mágica.



O gráfico 3D presente na figura 21 apresenta de forma mais elucidativa o comportamento
do pneu face ao slip angle e à carga Fz.





                                 Figura 21 - Gráfico 3D Fy vs Fz vs SA.



                                                                                   37
```

<a id="gaspar-p056"></a>

#### GAS · PDF página 056 de 108

```text
Para a obtenção dos dados longitudinais dos pneus reiterou-se o mesmo processo, tendo-
se contruído os seguintes gráficos para visualização dos parâmetros a utilizar na fórmula
mágica, figura 22.





           Figura 22 - Parâmetros longitudinais do pneu Hoosier R20 para formulação da fórmula mágica.



Construíram-se ainda dois gráficos para visualização da força longitudinal em função do
slip ratio e da carga vertical, figura 23, e para visualização do coeficiente de fricção em
função do slip ratio e da carga vertical, figura 24.





                                                                                   38
```

<a id="gaspar-p057"></a>

#### GAS · PDF página 057 de 108

```text
Figura 23 - Força Longitudinal em função do Slip Ratio e da carga Fz.





    Figura 24 - Coeficiente de fricção em função do SR e de Fz.





                                                                 39
```

<a id="gaspar-p058"></a>

#### GAS · PDF página 058 de 108

```text
   5.2. Formula Student T24e

Os dados de referência do veículo necessários para o desenvolvimento do simulador
englobam os dados que caracterizam o veículo estaticamente e os dados de GPS do
mesmo em utilização em pista. Na figura 25 é possível verificar o T24e sobre diferentes
vistas num plano 2D.





                                 Figura 25 - Dimensões do T24e.





                                                                                   40
```

<a id="gaspar-p059"></a>

#### GAS · PDF página 059 de 108

```text
Na  tabela 1 encontram-se assim, os dados fornecidos pelo formula  student que
caracterizam o veículo.




                                    Tabela 1 - Dados do T24e.

         Sigla  Descrição                           Valor   Unidade

      𝒎𝒗   Massa Total do Veículo (c/ piloto)       330     kg

      𝑾𝒃   Distância entre Eixos                    1,55   m

       𝑻𝒘   Largura Entre vias Via                    1,2    m

        𝒉𝒄𝒈   Altura do Centro de Gravidade           0,285  m

           𝒍𝒇    Distância do Eixo Dianteiro ao CG        0,802  m

           𝒍𝒓    Distância do Eixo Traseiro ao CG         0,748  m

          𝑱𝒙𝒙   Inércia de Roll                         83,326  Kg*m²

          𝑱𝒚𝒚   Inércia de Pitch                        214,782  Kg*m²

           𝑱𝒛𝒛    Inércia de yaw                         190,426  Kg*m²

        𝑱𝒘    Inércia do Conjunto Roda/Pneu           0,32    Kg*m²

       𝑲𝑹𝒇   Rigidez da Roda frontal                 42,03   N/mm

       𝑲𝑹𝒓   Rigidez da Roda Traseira                 52,4    N/mm

        𝑪𝒇𝒃   Amortecimento Dianteiro (bound)        1,68    Ns/m

        𝑪𝒇𝒓   Amortecimento Dianteiro (rebound)       2,19    Ns/m

        𝑪𝒓𝒃   Amortecimento Traseiro (bound)         3,78    Ns/m

         𝑪𝒓𝒓   Amortecimento Traseiro (rebound)        4,92    Ns/m

       𝑪𝑫   Coeficiente de Resistência Aerodinâmica  0,9         -

        𝑪𝑳    Coeficiente de Sustentação                1,6         -

        𝑨𝒇   Área frontal                           1,0348   m²





                                                                                   41
```

<a id="gaspar-p060"></a>

#### GAS · PDF página 060 de 108

```text
        5.2.1. Sistema de aquisição de dados
O sistema de aquisição de dados do T24e, no que concerne à dinâmica do veículo, está
centralizado no módulo MTi-680G, desenvolvido pela empresa Xsens. Devido à ausência
de sensores de hall para leitura da velocidade individual de roda e de potenciómetros
lineares para a medição do curso de suspensão, este módulo é o único responsável pela
aquisição dos dados gerados pelo veículo, tais como os valores de yaw, velocidades e
acelerações que serão analisados neste trabalho.





                                     Figura 26 - Módulo MTi-680G.



O MTi-680G, figura 26, é um sistema GNSS/INS, sistema de navegação global por
satélite com sistema de navegação inercial, onde a sua função é agregar e processar dados
de movimento e de posicionamento num único módulo.

A aquisição de dados opera segundo o princípio de fusão de sensores estritamente
acoplados. O hardware do MTi-680G integra uma unidade de medida inercial, IMU,
composta por acelerómetros, capazes de medir forças até 10 g, e giroscópios, com um
fundo de escala de 2000 deg/s. Estes componentes inerciais operam com frequências de
até 2 kHz de amostragem interna, capturando as acelerações e rotações transientes do
chassis. Contudo, devido à natureza da integração matemática necessária para derivar
velocidade e posição a partir da aceleração, os dados puramente inerciais estão sujeitos a
erros cumulativos ao longo do tempo, um fenómeno conhecido como desvio inercial ou
drift.

Para mitigar esta limitação e garantir a estabilidade dos dados, o sistema recorre ao seu
recetor GNSS interno. A componente crítica que eleva a precisão deste sistema aos
padrões exigidos no formula student é a introdução de correções RTK através da norma
RTCM 3.2/3.3. Utilizando algoritmos avançados de estimação de estado, o processador
do sensor cruza as medições inerciais de alta frequência com as coordenadas espaciais


                                                                                   42
```

<a id="gaspar-p061"></a>

#### GAS · PDF página 061 de 108

```text
absolutas de baixa frequência fornecidas pela constelação de satélites. Este processo de
filtragem resulta numa estimativa de posição com precisão centimétrica (1 cm + 1 ppm
CEP) e em vetores de velocidade com uma raiz do erro quadrático médio (RMS) de
apenas 0,05 m/s, (Xsense, 2025). Na figura 27 verifica-se a localização do módulo no
T24e.





                                  Figura 27 - Localização do MTi no veículo.



        5.2.2. Telemetria das provas FSPT e FSG
Relativamente  aos dados de  utilização em  pista foram  recolhidas  as  gravações
provenientes do sistema de aquisição de dados presente no carro de uma prova de
aceleração e de uma prova de autocross. O teste de aceleração ocorreu na FSPT de 2025
onde foi apenas possível obter o valor da velocidade em módulo do veículo, figura 28,
dentro do que seriam os dados relevantes para este projeto.





                                                                                   43
```

<a id="gaspar-p062"></a>

#### GAS · PDF página 062 de 108

```text
                             Figura 28 - Dados do teste de aceleração [km/h].

Na prova de autocross realizada na FSG 2025 já foi possível obter um conjunto de dados
mais completo. A Figura 29 apresenta as velocidades longitudinal e lateral do veículo ao
longo da prova, onde é bem visível o comportamento típico de um percurso de autocross,
com variações de velocidade frequentes e bruscas mudanças de direção. A Figura 30
mostra as acelerações longitudinal e lateral, sendo possível identificar os instantes de
maior solicitação do veículo, nomeadamente nas travagens mais intensas e nas curvas
mais rápidas. Por fim, a Figura 31 apresenta o ângulo de yaw, cuja variação contínua ao
longo de toda a prova reflete a natureza sinuosa do percurso e constitui um dos dados
mais relevantes para validar o comportamento lateral do modelo.





                             Figura 29 - Dados da velocidade do T24e.




                                                                                   44
```

<a id="gaspar-p063"></a>

#### GAS · PDF página 063 de 108

```text
Figura 30 – Dados da aceleração do T24e.





    Figura 31 - Dados de yaw do T24e.





                                                       45
```

<a id="gaspar-p064"></a>

#### GAS · PDF página 064 de 108

```text
  Desenvolvimento do Modelo de
  Comportamento Dinâmico


A modelação da dinâmica de um veículo é complexa devido ao número elevado de
variáveis que é necessário definir para descrever o mesmo, o ambiente que o rodeia, e
como estes interagem. Deste modo, é necessário definir a priori do desenvolvimento de
qualquer modelo quais são os pressupostos e as simplificações que serão realizadas, sem
que estas adulterem significativamente a veracidade dos resultados.

Assim, no presente trabalho, o veículo é representado por um modelo com 12 graus de
liberdade, permitindo o deslocamento lateral e longitudinal do mesmo, o movimento
vertical da massa suspensa (heave), bem como as rotações em torno dos três eixos
ortogonais previamente definidos (roll, pitch e yaw). O modelo contempla ainda a rotação
independente de cada uma das quatro rodas e a direção das rodas do eixo dianteiro.

Dada a extrema dificuldade que seria modelar as irregularidades da estrada, considerou-
se que a massa suspensa do veículo será a massa total do veículo, uma vez que a superfície
da estrada não provocará excitações na massa não suspensa. Além disso, considerou-se
ainda que o centro de rolamento do veículo se situa ao nível do solo e que o centro de
pressões aerodinâmicas coincide com o centro de massa do mesmo.

O desenvolvimento do modelo de comportamento dinâmico do veículo Formula Student
foi realizado através do software MATLAB, utilizando a ferramenta Simulink. Este
modelo está dividido em dez sistemas, sendo estes:

  o  Sistema de travagem;
  o  Sistema de propulsão;
  o  Sistema de cálculo das transferências de carga;
  o  Sistema de cálculo dos slip angles;
  o  Sistema de cálculo do escorregamento longitudinal;
  o  Sistema de cálculo das velocidades angulares;
  o  Sistema de cálculo da fórmula mágica de Pacejka;
  o  Sistema de cálculo das acelerações;
  o  Modelo de comportamento da suspensão;
  o  Sistema do condutor.


   6.1. Sistema de Travagem

O sistema de travagem do formula student é composto por uma Balance Bar onde estão
presentes duas bombas principais de travão AP Racing CP7855-91PRTE, colocadas
especificamente de forma a obter-se uma distribuição de travagem de 60-40%. Cada eixo
do veículo tem o seu circuito de travagem independente do outro, sendo a pressão
hidráulica fornecida pela bomba principal de travão respetiva. Por sua vez em ambas as



                                                                                   46
```

<a id="gaspar-p065"></a>

#### GAS · PDF página 065 de 108

```text
quatro rodas são utilizadas as pinças de travão Wilwood GP200 que contém dois êmbolos
cada uma.




                                  Tabela 2 - Dados do sistema de travagem.

 Nº de pastilhas de travão por eixo          4

 Pressão hidráulica máxima               80 bar

 Coeficiente de atrito pastilha-disco           0.4

 Área do pistão da bomba secundária        0,003168 𝑚2

 Raio médio do disco                        0,12 m


Na figura 32 visualiza-se o fluxograma desenvolvido para modelar o sistema de travagem.
Para tal recorreu-se às equações descritas na secção 4.7 e aos dados presentes na tabela
anterior.





                    Figura 32 - Fluxograma do dimensionamento do sistema de travagem.

No sistema modelado, figura 34, o parâmetro de entrada é a percentagem de travagem
desejada pelo condutor proveniente de um subsistema anterior do Simulink. Sabendo a
priori qual é a pressão máxima que o sistema de travagem consegue realizar obtém-se a


                                                                                   47
```

<a id="gaspar-p066"></a>

#### GAS · PDF página 066 de 108

```text
pressão no circuito. Posteriormente, esta pressão hidráulica divide-se entre o eixo
dianteiro e o eixo traseiro através da distribuição da força de travagem que ocorre na
balance bar. De seguida o sistema calcula a força e o binário de travagem em cada eixo.
Na figura 33 verifica-se uma ilustração de como funciona o sistema no veículo real.





            Figura 33 - Esquema das forças a atuar no sistema de travagem desenvolvido.





                        Figura 34 - Modelação do sistema de travagem.





                                                                                   48
```

<a id="gaspar-p067"></a>

#### GAS · PDF página 067 de 108

```text
   6.2. Sistema de Propulsão

O sistema de propulsão do Formula Student Te-24 é composto por um motor elétrico
Emrax 228 ligado diretamente ao diferencial autoblocante por uma corrente com uma
relação de transmissão de 4:1, sendo a relação de transmissão do diferencial de 1:1. Na
figura 35 encontra-se presente a curva de potência deste motor.





                       Figura 35 - Curva de potência do motor Emrax 228, (Emrax, 2025).

Para modelar este sistema, figura 37, tem-se como entradas as componentes de velocidade
longitudinal e lateral do veículo, bem como o sinal de aceleração imposto pelo condutor.
A saída do sistema corresponde ao binário aplicado às rodas motrizes, visível na
ilustração da figura 36.





                        Figura 36 - Esquema do sistema de propulsão do T24e.




                                                                                   49
```

<a id="gaspar-p068"></a>

#### GAS · PDF página 068 de 108

```text
Inicialmente, a velocidade escalar do veículo é determinada a partir das componentes Vx
e Vy, calculando a norma do vetor velocidade, posteriormente esta permite saber a
velocidade angular das rodas motrizes.

Considerando a relação de transmissão total do sistema de transmissão de 4:1, o regime
de rotação do motor é obtido diretamente a partir da velocidade angular da roda,
multiplicada pela relação de transmissão.



                                60                                   (45)
                        𝑛𝑚𝑜𝑡𝑜𝑟= 2𝜋𝑟n𝑟𝑜𝑑𝑎



Este sistema precisa de utilizar uma proteção “if else” para condições de arranque. Uma
vez que a velocidade do veículo é zero ou muito próxima de zero, caso não seja pré-
definido um valor de rotação de motor a aplicar, a saída de binário enviado por este
sistema do modelo seria zero, ficando o veículo no estado estacionário.

O binário do motor é determinado através de uma tabela unidimensional (lookup table 1-
D), que caracteriza o binário disponível em função do regime de rotação do motor. O
valor obtido é posteriormente escalado pelo  sinal de aceleração, representando a
solicitação de binário por parte do condutor.

Finalmente, o binário transmitido às rodas é calculado aplicando a relação de transmissão.





                                 Figura 37 - Modelo do sistema de propulsão.





                                                                                   50
```

<a id="gaspar-p069"></a>

#### GAS · PDF página 069 de 108

```text
   6.3. Sistema de Transferência de Cargas

O sistema de transferência de peso implementado, figura 39, calcula a distribuição das
cargas verticais nos quatro pneus do veículo, integrando os efeitos das acelerações
longitudinal e lateral, bem como a influência das forças aerodinâmicas verticais. Este
torna-se essencial para que seja possível noutros sistemas do modelo contabilizar a
sensibilidade à carga dos pneus.

Deste modo recorre-se às seguintes equações para calcular a carga sobre cada pneu. É de
notar que a convenção de sinais em cada equação segue o referencial ortogonal
apresentado inicialmente.


             𝑚𝑣𝑔𝑙𝑅            (𝑚𝑣𝑙𝑅/𝑤𝑏)𝑎𝑦ℎ𝑐𝑔  1        2  𝑙𝑅        (46)         𝐹𝑧11 =      −𝑚𝑣𝑎𝑥ℎ𝑐𝑔 +          +  ρ𝐶𝐿𝐴𝑓𝑣𝑥
             2𝑤𝑏     2𝑤𝑏          𝑇𝑤        2       2𝑤𝑏


             𝑚𝑣𝑔𝑙𝑅            (𝑚𝑣𝑙𝑅/𝑤𝑏)𝑎𝑦ℎ𝑐𝑔  1        2  𝑙𝑅        (47)         𝐹𝑧12 =      −𝑚𝑣𝑎𝑥ℎ𝑐𝑔 −          +  ρ𝐶𝐿𝐴𝑓𝑣𝑥
             2𝑤𝑏     2𝑤𝑏          𝑇𝑤        2       2𝑤𝑏


             𝑚𝑣𝑔𝑙𝐹  𝑚𝑣𝑎𝑥ℎ𝑐𝑔  (𝑚𝑣𝑙𝐹/𝑤𝑏)𝑎𝑦ℎ𝑐𝑔  1        2  𝑙𝐹        (48)
         𝐹𝑧21 =    +      +          +  ρ𝐶𝐿𝐴𝑓𝑣𝑥
             2𝑤𝑏     2𝑤𝑏          𝑇𝑤        2       2𝑤𝑏


             𝑚𝑣𝑔𝑙𝐹  𝑚𝑣𝑎𝑥ℎ𝑐𝑔  (𝑚𝑣𝑙𝐹/𝑤𝑏)𝑎𝑦ℎ𝑐𝑔  1        2  𝑙𝐹        (49)
         𝐹𝑧22 =    +      −          +  ρ𝐶𝐿𝐴𝑓𝑣𝑥
             2𝑤𝑏     2𝑤𝑏          𝑇𝑤        2       2𝑤𝑏





A implementação em Simulink foi realizada através de blocos aritméticos que executam
as operações definidas pelas equações anteriores, como se verifica na figura 38. Este
sistema tem como entradas a aceleração sobre o eixo de x e y e a velocidade sobre o eixo
x, que provém de sistemas anteriores. Como saídas tem-se então a carga sobre cada pneu,
𝐹𝑧11, 𝐹𝑧12, 𝐹𝑧21 e 𝐹𝑧22, que por sua vez serão utilizadas no sistema que define o
comportamento dos pneus. Neste são ainda definidas todas as constantes presentes nas
equações anteriores, como a massa do veículo, distância entre eixos, largura entre vias,
altura do centro de massa e os coeficientes aerodinâmicos.





                                                                                   51
```

<a id="gaspar-p070"></a>

#### GAS · PDF página 070 de 108

```text
                Figura 38 - Esquema de funcionamento do modelo de cargas verticais.





                                 Figura 39 - Modelo das cargas verticais.



   6.4. Sistema de Cálculo do Slip Angle

De modo a caracterizar a dinâmica lateral do veículo é fundamental calcular os slips
angles de cada pneu em cada time-step da simulação. Conforme apresentado no capítulo
4 e considerando as velocidades 𝑉𝑦 e 𝑉𝑥 no centro de massa do veículo pode calcular-se o
slip angle através da equação 16. Mas uma vez que o simulador apresenta 12 graus de
liberdade, é necessário transpor esta expressão e realizar o cálculo individual do slip angle
de cada pneu. Para tal tem-se agora em conta as velocidades lineares do veículo, a
velocidade imposta pela velocidade angular do chassis e o angulo de direção das rodas
do eixo da frente.


Deste modo obtém-se as seguintes expressões para o slip angle individual de cada roda:





                                                                                   52
```

<a id="gaspar-p071"></a>

#### GAS · PDF página 071 de 108

```text
                                                                                (50)
                            𝑉𝑦+ 𝑙𝐹ψ̇
                        α11 = arctan (      ) − 𝛿                                     𝑇𝑤                            𝑉𝑥+    ψ̇                                   2





                                                                                (51)
                            𝑉𝑦+ 𝑙𝐹ψ̇
                        α12 = arctan (      ) − 𝛿
                            𝑉𝑥−𝑇𝑤 ψ̇                                   2



                                                                                (52)
                                   𝑉𝑦−𝑙𝑅ψ̇
                         α21 = arctan (      )                                       𝑇𝑤                             𝑉𝑥+    ψ̇                                     2



                                                                                (53)
                                   𝑉𝑦−𝑙𝑅𝜓̇
                         𝛼22 = arctan (      )
                             𝑉𝑥−𝑇𝑤 𝜓̇                                     2




O sistema no simulink para este modelo, figura 41, tem como base as equações descritas,
que foram implementadas através de blocos de lógica do software. Para tal foi necessário
definir as constantes geométricas do carro presentes nas fórmulas, e colocar através de
blocos “inputs” as variáveis 𝑉𝑦, 𝑉𝑥 e 𝜓̇, que provém de outro sistema. Este tem ainda uma
proteção através de blocos “if else”, para quando a velocidade do veículo for inferior a 1
m/s, este automaticamente definir os slip angles com o valor 0, assim evita divisões por
0 quando o carro estiver parado ou próximo disso, de forma a não parar a simulação. Por
fim, os slip angles são ligados a blocos “output” para seguirem para outro sistema. Na
figura 40 está representado o esquema de funcionamento deste modelo.





                Figura 40 - Esquema de funcionamento do modelo de cálculo do slip angle.




                                                                                   53
```

<a id="gaspar-p072"></a>

#### GAS · PDF página 072 de 108

```text
                             Figura 41 - Sistema de cálculo do Slip Angle.





   6.5. Sistema de Cálculo das Velocidades Angulares das Rodas

A base do sistema de cálculo das velocidades angulares é a expressão (17) para a
aceleração angular, sendo que é possível integrar a mesma para se obter a velocidade
angular tal como referido no capítulo 4.

Assim, neste sistema utiliza-se como inputs as forças dos pneus provenientes de outro
sistema, o binário que chega à roda, e o binário de travagem.

Este calcula também a inércia total através da definição das constantes de inércia do
motor, inércia da roda e da relação do sistema de transmissão, seguindo a expressão (18).

Posto isto, novamente, introduz-se os blocos de lógica para replicar a equação (17), saindo
do sistema como outputs as velocidades angulares de cada roda, figura 43.

Na figura 42 está representado o esquema de funcionamento deste modelo.





            Figura 42 - Esquema de funcionamento do modelo de cálculo das velocidades angulares.





                                                                                   54
```

<a id="gaspar-p073"></a>

#### GAS · PDF página 073 de 108

```text
                            Figura 43 - Sistema de cálculo das velocidades angulares.



   6.6. Sistema de Cálculo do Slip Ratio

O escorregamento longitudinal, comummente referido como slip ratio, é expresso pela
equação 54, onde 𝑉𝑥 é a velocidade longitudinal da roda e 𝑟𝑒Ω a sua velocidade periférica.
Um valor positivo indica condição de tracção (𝑟𝑒Ω > 𝑉𝑥) e negativo indica travagem (𝑟𝑒Ω
< 𝑉𝑥).



                                                                                (54)                  𝑘= −𝑉𝑥−𝑟𝑒Ω
                                             𝑉𝑥



Para exprimir a velocidade longitudinal no referencial do pneu, recorre-se ao slip angle α
previamente calculado. A velocidade combinada da roda 𝑣𝑡 resultante das componentes
lateral e longitudinal induzidas pelo movimento do veículo e pela velocidade angular do
chassis, não está de forma geral, alinhada com o plano do pneu. A sua componente
longitudinal obtém-se por projeção geométrica:



                      𝑣𝑥= 𝑣𝑡⋅cos α                               (55)



                                                                                   55
```

<a id="gaspar-p074"></a>

#### GAS · PDF página 074 de 108

```text
A velocidade combinada 𝑣𝑡 incorpora o contributo da velocidade angular do chassis nas
velocidades locais de cada roda, seguindo o mesmo raciocínio cinemático utilizado no
cálculo do slip angle.


                                                           2                    (56)
                                            2       𝑇
                  𝑣𝑡𝐹𝑟= √(𝑣𝑦+ 𝑙𝐹ψ̇ ) + (𝑣𝑥+   ψ̇ )
                                         2



Com expressões análogas para as restantes rodas, alterando os sinais de 𝑙𝐹 para −𝑙𝑅 nas
                   𝑇𝑤       𝑇𝑤rodas traseiras e de   para −   nas rodas do lado direito.
                    2         2

No simulink, figura 45, foi introduzido blocos inputs para o sistema receber as 2 variáveis
de velocidade do veículo, a velocidade angular dos chassis, as velocidades angulares e o
slip angle de cada roda. E ainda adicionado um bloco que define a constante do raio
efetivo da roda. De seguida introduziram-se os blocos de lógica para reproduzir as
equações mencionadas. Por fim, antes dos 4 “outputs” responsáveis por encaminhar os
valores de slip ratio de cada roda, colocou-se um bloco “saturation” em cada linha para
que este valor seja sempre compreendido entre -1 e 1. Na figura 44 encontra-se
esquematizado as entradas e saídas do modelo assim como as expressões utilizadas.





                             Figura 44 - Sistema de cálculo do slip ratio.





                                                                                   56
```

<a id="gaspar-p075"></a>

#### GAS · PDF página 075 de 108

```text
                    Figura 45 - Sistema para cálculo do escorregamento longitudinal.



   6.7. Sistema da Fórmula Mágica de Pacejka

O sistema de cálculo das forças do pneu recebe como entradas o  slip angle, o
escorregamento longitudinal e a carga vertical de cada roda, e aplica a Fórmula Mágica
de Pacejka para determinar as forças lateral e longitudinal produzidas por cada pneu.

O primeiro passo para a construção deste sistema foi desenvolver um subsistema, figura
46, responsável por calcular os parâmetros 𝑦𝑚, 𝑦𝑎, 𝑦′(0) e 𝑥𝑚 consoante a carga vertical
no pneu em cada time-step. Este é composto por um único bloco input responsável por
trazer o valor de 𝐹𝑧, de seguida este valor é introduzido em 4 tabelas (2D Look up table)
responsáveis por interpolar os valores dos parâmetros dos pneus com base nos gráficos
experimentais apresentados no capítulo 5.





                    Figura 46 - Subsistema dos parâmetros experimentais dos pneus.




                                                                                   57
```

<a id="gaspar-p076"></a>

#### GAS · PDF página 076 de 108

```text
Posteriormente já com os parâmetros definidos em cada carga 𝐹𝑧 que entra no sistema,
utilizou-se o bloco Matlab function para calcular os coeficientes B, C, D e E que permitem
aplicar a fórmula mágica. Desenvolveu-se o código presente na figura 47 para aplicar
dentro da função.





                    Figura 47 - Código desenvolvido para cálculo da fórmula mágica.

Aplicando os dois subsistemas anteriores tanto para as forças longitudinais como para as
forças laterais, torna-se ainda necessário definir como ambas interagem entre si e se os
cálculos das mesmas estão dentro do círculo de fricção. Deste modo foi necessário colocar
mais um bloco Matlab function com o código presente na figura 48. Este assegura que a
resultante das forças se mantém dentro dos limites de aderência do pneu, prevenindo
assim a ocorrência de forças sobrestimadas pelo modelo em situações de aceleração ou
travagem combinadas com curva. Adicionalmente, introduziu-se ainda um fator de
atenuação linear para velocidades inferiores a 0,5 m/s, de forma a prevenir instabilidades
numéricas e singularidades matemáticas inerentes ao cálculo dos slips em regimes de
baixa velocidade, principalmente quando esta é próxima de zero.





                                                                                   58
```

<a id="gaspar-p077"></a>

#### GAS · PDF página 077 de 108

```text
                        Figura 48 - Código para implementação do círculo de fricção.

Por fim, na figura 50 está presente o sistema completo desenvolvido para cálculo das
forças laterais e longitudinais de apenas uma roda, sendo necessário replicar o mesmo
sistema para as restantes. Para calcular a força de resistência ao rolamento multiplicou-se
a carga 𝐹𝑧 pelo coeficiente de resistência ao rolamento de valor 0,015. Na figura 49
encontra-se esquematizado as entradas e saídas do modelo assim como as expressões
utilizadas.





                Figura 49 - Esquema de funcionamento do modelo da fórmula mágica.





                                                                                   59
```

<a id="gaspar-p078"></a>

#### GAS · PDF página 078 de 108

```text
Figura 50 - Sistema completo para cálculo das forças dos pneus.





                                                                 60
```

<a id="gaspar-p079"></a>

#### GAS · PDF página 079 de 108

```text
   6.8. Sistema de Cálculo das Acelerações

O sistema de cálculo das acelerações tem como objetivo determinar as acelerações
longitudinal, lateral e rotacional do veículo com base nas equações (13), (14) e (15)
apresentadas no capítulo 4. Estas são equações diferenciais de segunda ordem, uma vez
que contêm a segunda derivada da posição em ordem ao tempo como variável
dependente. Para que o Simulink as possa resolver numericamente, é necessário
transformá-las em equações de primeira ordem através da representação em espaço de
estados.

Esta transformação consiste em definir um vetor de estados 𝑥(𝑡) que agrupa todas as
variáveis que variam ao longo do tempo, sendo estas as posições longitudinal e lateral, as
velocidades longitudinal e lateral, o ângulo de yaw e a velocidade angular do chassis. É
ainda necessário definir um vetor de entradas 𝑢(𝑡) que contém as grandezas externas que
alimentam o sistema, nomeadamente o ângulo de direção e as forças longitudinais e
laterais de cada pneu,  calculadas  anteriormente no  sistema da fórmula mágica.




                                                   𝑇                          (57)                          𝑥(𝑡) = [𝑥; 𝑦; 𝑉𝑥; 𝑉𝑦; ψ; ψ̇ ]

                                                                 𝑇           (58)             𝑢(𝑡) = [δ; 𝐹𝑥11; 𝐹𝑦11; 𝐹𝑥12; 𝐹𝑦12; 𝐹𝑥21; 𝐹𝑦21; 𝐹𝑥22; 𝐹𝑦22]


O sistema fica então descrito pela equação ẋ(𝑡) = 𝐴· 𝑥(𝑡) + 𝐵· 𝑢(𝑡), onde a matriz 𝐴
relaciona os estados entre si e a matriz 𝐵 relaciona as entradas com a variação dos estados.
A cada time-step, o Simulink multiplica as matrizes pelos respetivos vetores, obtendo as
derivadas dos estados, ou seja, as acelerações e as velocidades, que são depois integradas
para fornecer os valores atualizados de 𝑥, 𝑦, 𝑉𝑥, 𝑉𝑦, 𝜓 𝑒 𝜓̇.

Deste modo tornou-se ainda preciso desenvolver o seguinte código, figura 51, para
colocar dentro de um bloco “matlab function”, com o objetivo de definir as matrizes
𝐴, 𝐵, 𝐶 e 𝐷.





                                                                                   61
```

<a id="gaspar-p080"></a>

#### GAS · PDF página 080 de 108

```text
                Figura 51 - Código para definição das matrizes do modelo de acelerações.



A figura 53 apresenta o modelo construído no simulink, através de 9 blocos “inputs” para
as entradas do sistema, um bloco “matlab function” com 8 blocos “constant” associados,
um bloco “varying state-space”, dois blocos “demux” para ser possível visualizar as
saídas provenientes do “state-space”, e blocos outputs para enviar os valores das variáveis
calculadas para fora do sistema, sendo que na figura 52 é possível verificar de forma
simplificada a ligação entre estes inputs e outputs.





                                                                                   62
```

<a id="gaspar-p081"></a>

#### GAS · PDF página 081 de 108

```text
                Figura 52 - Esquema de funcionamento do modelo de cálculo das acelerações.





                             Figura 53 - Modelo de cálculo das acelerações.



   6.9. Modelo de Comportamento da Suspensão

Este modelo tem como objetivo simular o comportamento da suspensão do Formula
Student e segue a mesma tipologia de espaço de estados utilizada para o cálculo das
acelerações.

Tendo como base a expressão (27), que descreve o movimento de um sistema massa-
mola-amortecedor, reorganiza-se a mesma de forma a isolar a aceleração. A partir desta
deriva-se as três equações que definem os movimentos da carroçaria: o heave, o roll e o
pitch.



                                 𝑓(𝑡) −𝑘𝑥−𝑐𝑥̇                             (59)
                                                 𝑥̈ =
                     𝑚





                                                                                   63
```

<a id="gaspar-p082"></a>

#### GAS · PDF página 082 de 108

```text
O movimento vertical da carroçaria, designado por heave e representado por Zs, é
influenciado pela força aerodinâmica vertical, proporcional ao quadrado da velocidade, e
pelas forças geradas pelas molas e amortecedores dos dois eixos, de onde resulta o fator
2 na equação. Substituindo na forma geral obtém-se a equação seguinte.



                1                                                         (60)                     ρ𝐶𝐷𝐴𝑓𝑥2̇ −2(𝑘 𝑠𝑓+ 𝑘𝑠𝑟)𝑍𝑠−2(𝐶𝑓+ 𝐶𝑟)𝑍𝑠̇                2                      𝑍𝑠̈ =
                             𝑚𝑣



Para os movimentos rotacionais da carroçaria, a massa é substituída pela inércia e as
forças pelos momentos correspondentes. No caso do roll, considerando que o centro de
rolamento se situa ao nível do solo, o momento desestabilizador é gerado diretamente
pela aceleração lateral 𝑎𝑦 a atuar na massa da carroçaria, tendo como braço a altura total
do centro de massa em relação ao solo. Esta simplificação corresponde a adotar o maior
braço de momento possível, sendo por isso conservadora, na medida em que sobrestima
ligeiramente o momento de roll relativamente a uma modelação com o centro de
rolamento acima do solo. Quando a carroçaria apresenta um ângulo de roll, o peso gera
igualmente um momento desestabilizador, ao qual a suspensão se opõe através da sua
rigidez ao movimento de roll e do amortecimento 𝐶𝛾, obtendo-se a equação seguinte.


                −𝑚𝑣ℎ𝑐𝑔𝑎𝑦+ γ(𝑚𝑣𝑔ℎ𝑐𝑔−𝑘γ) + γ̇(−𝐶γ)                (61)
                               γ̈ =
                                             𝐽𝑥𝑥


O raciocínio para o pitch é idêntico, mas em torno do eixo lateral y com inércia 𝐽𝑦𝑦. O
momento desestabilizador é gerado pela aceleração longitudinal 𝑎𝑥 actuando igualmente
à altura do centro de massa, opondo-se-lhe a rigidez em pitch 𝑘𝜃 e o amortecimento 𝐶𝜃.


                 𝑚𝑣ℎ𝑐𝑔𝑎𝑥+ γ(𝑚𝑣𝑔ℎ𝑐𝑔−𝑘θ) + θ̇(−𝐶θ)                 (62)
                              θ̈ =
                                           𝐽𝑦𝑦



Segundo o autor (Milliken, 1995), as rigidezes de pitch e roll podem ser expressas pelas
equações seguintes, notando que devido à possibilidade das distâncias de cada eixo ao
centro de massa serem diferentes, assim como a rigidez das molas, o cálculo da rigidez
de pitch divide-se entre travagem e aceleração.





                                                                                   64
```

<a id="gaspar-p083"></a>

#### GAS · PDF página 083 de 108

```text
                              𝑇𝑤2(𝑘𝑟𝑓+ 𝑘𝑟𝑟)                             (63)
                           𝑘γ =
                                 2

                         𝑘θ travagem = 𝑤𝑏 2𝑘𝑠𝑓 𝑙𝐹                           (64)

                        𝑘θ aceleração = −𝑤𝑏 2𝑘𝑠𝑟 𝑙𝑅                         (65)

                              𝑇𝑤2(𝐶𝑟𝑓+ 𝐶𝑟𝑟)                             (66)
                           𝐶γ =
                                 2



Para a análise do deslocamento da suspensão em cada roda individualmente, é necessário
exprimir o deslocamento vertical de cada roda em função dos movimentos da carroçaria.
Seguindo a convenção SAE, 𝑍𝑠 positivo corresponde à carroçaria a subir. Com θ positivo
considerou-se que o eixo traseiro comprime, reduzindo 𝑍𝑠 na traseira e aumentando na
frente. Com γ positivo a carroçaria inclina para a direita e o lado direito desce e comprime.

Combinando os três movimentos, obtêm-se as expressões do deslocamento vertical em
cada roda:

                   𝑍𝑠11 = 𝑍𝑠+ 𝑙𝐹⋅sin(θ) + (𝑇𝑤 ) ⋅sin(γ)                  (67)
                                      2

                                                                                (68)                   𝑍𝑠12 = 𝑍𝑠+ 𝑙𝐹⋅sin(θ) −(𝑇𝑤 ) ⋅sin(γ)
                                      2
                   𝑍𝑠21 = 𝑍𝑠−𝑙𝑅⋅sin(θ) + (𝑇𝑤 ) ⋅sin(γ)                  (69)
                                      2

                                                                                (70)                   𝑍𝑠22 = 𝑍𝑠−𝑙𝑅⋅sin(θ) −(𝑇𝑤 ) ⋅sin(γ)
                                      2



Analogamente ao modelo das acelerações, transformou-se as equações de segunda ordem
em equações de primeira ordem resolúveis pelo Simulink por integração numérica. Neste
caso o vetor de estados agrupa os deslocamentos e as velocidades nos três graus de
liberdade da carroçaria.



                          𝑥(𝑡) = [ 𝑍𝑠, 𝜃, 𝛾, Ż𝑠, 𝜃̇, 𝛾̇ ]ᵀ                         (71)



O vetor de entradas contém as grandezas fornecidas pelo modelo de cálculo das
acelerações, onde 𝑣𝑥 é necessário para o cálculo da força aerodinâmica, 𝑎𝑥 alimenta a
equação do pitch e 𝑎𝑦 alimenta a equação do roll.



                                                                                   65
```

<a id="gaspar-p084"></a>

#### GAS · PDF página 084 de 108

```text
                            𝑢(𝑡) = [ 𝑣𝑥, 𝑎𝑥, 𝑎𝑦]ᵀ                            (72)



No código presente na figura 54 encontra-se presente a definição das matrizes necessárias
para o bloco de espaço de estados. Este tem ainda a particularidade de definir que quando
o deslocamento de cada eixo atinge os limites de compressão ou extensão da suspensão,
a rigidez aumenta abruptamente com a adição da rigidez dos batentes de fim de curso e
define ainda o amortecimento dos amortecedores consoante estes estejam em extensão ou
compressão.





                                                                                   66
```

<a id="gaspar-p085"></a>

#### GAS · PDF página 085 de 108

```text
                    Figura 54 - Código para cálculo das matrizes do modelo de suspensão.



Desenvolveu-se ainda o código presente na figura 55 para análise individual do
deslocamento sofrido pela suspensão em cada roda e análise da velocidade de movimento
de cada uma.





                    Figura 55 - Código para análise do deslocamento vertical de cada roda.



Por fim, e seguindo a mesma filosofia de desenvolvimento dos modelos no Simulink
utilizada até aqui, construiu-se o modelo da figura 57. Verificando-se o esquema de
funcionamento do mesmo na figura 56.





                                                                                   67
```

<a id="gaspar-p086"></a>

#### GAS · PDF página 086 de 108

```text
            Figura 56 – Esquema de funcionamento do modelo de comportamento da suspensão.





                             Figura 57 - Modelo de comportamento da suspensão.



   6.10.   Modelo do Condutor

O modelo do condutor, figura 58, tem como objetivo controlar o acelerador, o travão e o
ângulo de direção a aplicar em cada instante de simulação. Para tal, são utilizados valores
de referência reais de velocidade e ângulo de yaw, obtidos a partir de dados registados em
pista.

Dois controladores PID comparam continuamente o erro entre os valores de referência e
os valores calculados pelo simulador, ajustando os comandos de forma a aproximar o
mais possível o valor calculado do valor real. O primeiro controlador, responsável pelo
acelerador e pelo travão produz um sinal entre o intervalo [−1, 1] onde valores positivos
serão direcionados para o modelo do sistema de propulsão e os valores negativos serão
direcionados para o modelo do sistema de travagem.

Para o controlo da direção, optou-se por utilizar o ângulo de yaw como variável de
referência em vez do ângulo de direção uma vez que este apresenta valores mais estáveis
e com menos correções face aos dados de referência existentes para o ângulo de direção.




                                                                                   68
```

<a id="gaspar-p087"></a>

#### GAS · PDF página 087 de 108

```text
Assim, o segundo controlador PID compara o ângulo de yaw de referência com o ângulo
de yaw atual do veículo, produzindo um sinal entre o intervalo [−1, 1] que posteriormente
é multiplicado pelo valor de ângulo de direção máximo que uma roda do Formula Student
consegue atingir. Este valor posteriormente entra diretamente no modelo de cálculo das
acelerações e do modelo que calcula os slips angles.

Na tabela 3 verificam-se os valores de ajuste dos controladores PID, valores estes obtidos
através de várias comparações de comportamento entre o modelo e os valores reais.





                            Tabela 3 - Parâmetros dos controladores PID.

 Controlador PID         P                     I           D

    Longitudinal            2                  0.45                0,05

      Direção               0.12                0,05                0,05



O modelo Simulink, figura 59, construído para representar o comportamento do piloto
contém três blocos “From Workspace” para importar os dados de referência, dois blocos
“PID Controller”, dois blocos para subtrair o valor de referência ao valor real, dois blocos
“switch” para fazer a divisão do sinal entre o acelerador e o travão consoante se o valor
proveniente do controlador é maior ou menor que zero, e três blocos outputs para enviar
os sinais de saída respetivos.





                    Figura 58 - Esquema de funcionamento do modelo do condutor.





                                                                                   69
```

<a id="gaspar-p088"></a>

#### GAS · PDF página 088 de 108

```text
                                 Figura 59 – Modelo simulink do condutor.

   6.11.    Sistema Completo

Após o desenvolvimento de todos os subsistemas que constituirão o modelo, verifica-se
no diagrama presente na figura 60 como estes se encontram interligados.





                                                                                   70
```

<a id="gaspar-p089"></a>

#### GAS · PDF página 089 de 108

```text
                    Figura 60 - Diagrama de interligação dos subsistemas do modelo.



O processo inicia-se no modelo do condutor, que recebe como referência a velocidade e
o yaw rate registados em pista onde origina três comandos: a percentagem de acelerador,
a percentagem de travão e o ângulo de direção. A percentagem de acelerador alimenta o
sistema de propulsão, que calcula o binário motor 𝑇𝑤 disponível nas rodas traseiras em
função do regime do motor Emrax 228. A percentagem de travão alimenta o sistema de
travagem, que determina o binário resistente 𝑇𝑏 em cada eixo com base na pressão
hidráulica gerada na balance bar.

Ambos os binários referidos entram no sistema de cálculo das velocidades angulares, que
integra a aceleração angular de cada roda para obter 𝜔. Esta velocidade angular alimenta
o cálculo do slip ratio, que quantifica o escorregamento longitudinal de cada pneu em
função da diferença entre a velocidade periférica da roda e a velocidade longitudinal do
veículo.

Em paralelo, o slip angle é calculado a partir das velocidades 𝑣𝑥 e 𝑣𝑦, do yaw rate e do
ângulo de direção. O sistema de transferência de cargas calcula a carga vertical 𝐹𝑧 em





                                                                                   71
```

<a id="gaspar-p090"></a>

#### GAS · PDF página 090 de 108

```text
cada roda, integrando os efeitos das acelerações longitudinal e lateral sobre a distribuição
de peso do veículo.

Após calculadas as cargas sobre cada pneu e os seus escorregamentos, estes são então
introduzidos no modelo da Fórmula Mágica de Pacejka, onde serão por sua vez calculadas
as forças produzidas pelos pneus nas direções 𝑥 e 𝑦.

As forças assim calculadas constituem as entradas dos dois subsistemas finais do modelo.
O modelo de acelerações utiliza 𝐹𝑥, 𝐹𝑦 e δ para resolver o sistema de equações de
movimento e determinar as componentes das velocidades, o yaw rate e as acelerações 𝑎𝑥
e 𝑎𝑦 do veículo. Em paralelo, o modelo de suspensão recebe 𝑣𝑥, 𝑎𝑥 e 𝑎𝑦 e calcula os
movimentos de heave, roll e pitch da carroçaria, bem como o deslocamento individual de
cada roda. Na figura 61 está presente o modelo completo desenvolvido.





                        Figura 61 - Modelo completo desenvolvido no simulink.





                                                                                   72
```

<a id="gaspar-p091"></a>

#### GAS · PDF página 091 de 108

```text
  Simulações


No presente capítulo pretende-se realizar dois ensaios com o objetivo de validar o
comportamento do simulador desenvolvido em diferentes condições de operação,

O primeiro ensaio consiste num teste de aceleração, utilizando os dados já tratados no
capítulo 5 relativamente à prova de aceleração da Fórmula Student Portugal 2025 (FSPT
2025). O segundo ensaio pretende avaliar o comportamento do simulador em situação de
curva, utilizando da prova de autocross da Fórmula Student Germany 2025 (FSG 2025),
envolvendo simultaneamente aceleração, travagem e mudanças de direção.

Após a validação do modelo pretende-se analisar o comportamento da suspensão,
comparar as forças produzidas pelos pneus face aos dados experimentais dos mesmos, e
analisar a influência de diferentes setups de suspensão.



   7.1. Simulação da Prova de Aceleração

A prova de aceleração nas competições de Fórmula Student consiste numa reta de 75
metros onde o veículo parte da posição de repouso com o objetivo de percorrer o percurso
no menor tempo possível. Sendo esta uma prova puramente longitudinal, a análise
incidirá apenas sobre os parâmetros correspondentes, deixando-se de parte grandezas
como as forças laterais produzidas pelos pneus.

De modo a verificar se o simulador consegue replicar o comportamento do veículo, a
simulação foi executada com os dados de referência compreendidos entre os 32s e os 45s,
intervalo que corresponde à prova de aceleração propriamente dita e ao percurso
subsequente até à entrada na via das boxes, figura 62.





                                                                                   73
```

<a id="gaspar-p092"></a>

#### GAS · PDF página 092 de 108

```text
        Figura 62 - Gráfico comparativo entre a velocidade de referência e a velocidade simulada.



Para quantificar a discrepância entre a os valores de velocidade da simulação e os valores
da referência, calculou-se o RMSE, a raiz quadrada da média dos quadrados dos desvios.
Esta métrica penaliza desvios de maior magnitude, como o que é visível no intervalo entre
os 32s e 39s, a zona de maior exigência dinâmica da simulação.





                                   𝑛
                                                          2                      (73)              𝑅𝑀𝑆𝐸= √1                              𝑛∑(𝑣𝑟𝑒𝑓,𝑖−𝑣𝑠𝑖𝑚,𝑖)
                                       𝑖=1



Através da fórmula anterior foi calculado o RMSE de 1,805 km/h. A haste vertical no
gráfico de barras da figura 62 indica a variabilidade do erro instantâneo, confirmando
que, apesar da precisão média, existe um pico de dispersão que atinge os 3,5 km/h.





                                                                                   74
```

<a id="gaspar-p093"></a>

#### GAS · PDF página 093 de 108

```text
                        Figura 63 - RMSE da simulação da prova de aceleração.



Analisando as figuras 63 e 64, pode concluir-se que o simulador demonstra uma boa
capacidade de replicar o perfil de velocidade de referência ao longo de todo o intervalo
de análise. O desvio principal ocorre na fase de aceleração máxima, entre os 34s e os
35,5s, onde a velocidade de referência atinge um pico de aproximadamente 81 km/h que
o simulador não replica na totalidade. Este desvio é consequência da natureza reativa do
controlador PID do modelo do condutor, em regime de aceleração intensa, o controlador
demora alguns instantes a impor o comando máximo de acelerador, o que se traduz num
atraso da resposta simulada face à referência. Após este intervalo, o simulador recupera
progressivamente, sendo as duas curvas praticamente sobrepostas nas fases posteriores
de velocidade praticamente constante, travagem e condução a velocidade reduzida
correspondentes ao regresso à via das boxes.

Posto isto, é agora possível analisar a dinâmica do veículo calculada pelo simulador
durante a prova de aceleração. Uma vez que o arranque ocorre entre o início da simulação
aos 32s e sensivelmente aos 35,5s, será analisado apenas este intervalo.

Entre os 32s e os 33,7s, o veículo encontra-se em repouso e verifica-se a presença de
oscilações numéricas no heave e nas cargas verticais. Esta instabilidade resulta do facto
de o modelo não ter sido concebido para operar em condições de velocidade nula, onde a
ausência de forças dinâmicas significativas torna o sistema numericamente sensível,
sendo necessário realizar operações de proteção contra divisões por zero. Esta limitação
não compromete a validade da análise, uma vez que a prova de aceleração se inicia
precisamente quando o veículo começa a mover-se.





                                                                                   75
```

<a id="gaspar-p094"></a>

#### GAS · PDF página 094 de 108

```text
                             Figura 64 - Pitch durante a prova de aceleração.



A partir dos 33,7s, com o início da aceleração, o modelo apresenta um comportamento
fisicamente coerente. Em repouso, a suspensão dianteira encontrava-se já comprimida
num valor de −18,59 mm e a traseira em −15,98 mm, reflexo da carga estática do veículo,
com cargas verticais de 781 N e 838 N respetivamente. Com a aceleração, a transferência
de carga para o eixo traseiro provoca a extensão da suspensão dianteira até −6,07 mm,
correspondendo a uma variação total de 12,52 mm, enquanto a suspensão traseira
comprime adicionalmente de −15,98 mm até −27,79 mm, numa variação de 11,81 mm.
Como esperado, a variação de deslocamento não é igual nos dois eixos, uma vez que a
rigidez traseira é superior à dianteira, 52,4 N/mm face a 42,0 N/mm. Este fenómeno de
squat reflete-se no ângulo de pitch, que aumenta progressivamente até atingir um máximo
de 0,863°. Em termos de cargas verticais, a carga no eixo dianteiro reduz-se de 781 N
para 255 N, enquanto a carga no eixo traseiro aumenta de 838 N para 1456 N,
confirmando a magnitude da transferência de carga longitudinal durante a aceleração.

Na figura 65 encontram-se os gráficos do movimento da suspensão da roda frente
esquerda e da roda trás esquerda, assim como o gráfico das cargas verticais em cada uma
respetivamente.





                                                                                   76
```

<a id="gaspar-p095"></a>

#### GAS · PDF página 095 de 108

```text
        Figura 65 - Gráficos do movimento de heave e das cargas verticais na prova de aceleração.



A descida abrupta do ângulo de pitch após os 35,5s, visível na figura 64, corresponde à
travagem no final da reta, onde a transferência de carga se inverte, a frente comprime e a
traseira estende, confirmando o comportamento de “nose dive” esperado nesta fase.

Na figura 66 é possível avaliar o comportamento do pneu traseiro esquerdo durante a
prova de  aceleração em  termos de  força  longitudinal  produzida  e o  respetivo
escorregamento longitudinal. A partir dos 33,7s, com o início da aceleração, a força
longitudinal cresce progressivamente até atingir um máximo de 2952,95 N, acompanhada
por um escorregamento longitudinal máximo de 0,0595. Consultando o gráfico presente
na figura 21 onde é  feita a caracterização do pneu, para uma carga  vertical de
aproximadamente 1456 N registada no eixo traseiro durante a aceleração, o pico de força
disponível ocorre para um slip ratio de 0,085, onde o pneu seria capaz de gerar entre 3200
N e 3400 N. O simulador atingiu apenas 0,0595 de escorregamento, o que significa que
o pneu não chegou ao seu ponto de capacidade máxima, existindo ainda uma margem de
tração por explorar.





                                                                                   77
```

<a id="gaspar-p096"></a>

#### GAS · PDF página 096 de 108

```text
         Figura 66 - Gráficos da força e escorregamento longitudinal da roda traseira esquerda na prova de
                                                    aceleração.



   7.2. Simulação da Prova de Autocross

A prova de autocross consiste num percurso definido por um conjunto de cones, onde o
veículo percorre uma pista técnica com acelerações, travagens e mudanças de direção,
tendo como objetivo completar o percurso no menor tempo possível. Esta é uma prova de
carácter transiente que solicita simultaneamente os esforços longitudinais e laterais do
veículo, tornando a sua simulação mais exigente.

Ao contrário do que sucedeu na prova de aceleração, a simulação do autocross revelou-
se inviável nas condições atuais do modelo. Quando a simulação é executada apenas
tendo como referência a velocidade longitudinal do veículo, o modelo, novamente,
consegue acompanhar o perfil de velocidade de referência. Contudo, assim que o
controlador PID do sistema de direção é  ativado, o modelo colapsa ao fim de
aproximadamente 1,5s de simulação, como se pode observar na figura 67, onde as
velocidades longitudinal e lateral e o ângulo de yaw divergem para valores fisicamente
impossíveis. Esta instabilidade resulta muito provavelmente da elevada sensibilidade do



                                                                                   78
```

<a id="gaspar-p097"></a>

#### GAS · PDF página 097 de 108

```text
controlador da direção a entradas que variam muito rapidamente, características de um
percurso de autocross com mudanças de direção frequentes e repentinas.





                Figura 67 - Dados de referência da prova de autocross face aos dados simulados.



Face a esta limitação, e com o objetivo de retirar valor prático do trabalho desenvolvido,
o simulador foi reorientado para funcionar como uma ferramenta de análise cinemática,
figura 68. Nesta abordagem, em vez de calcular a trajetória do veículo de forma
autónoma, o simulador recebe diretamente os dados de referência registados em pista e
utiliza-os para calcular grandezas que tipicamente não são medidas durante as provas,
como os movimentos de pitch, roll e heave da carroçaria e o deslocamento individual da
suspensão de cada roda. Esta abordagem revela-se particularmente útil quando o veículo
não dispõe de sensores de inclinação ou potenciómetros de suspensão instalados ou
operacionais durante a prova. Adicionalmente, permite analisar como o veículo reagiria
com diferentes configurações de setup, nomeadamente através da variação da rigidez das
molas e das barras estabilizadoras, fornecendo à equipa uma ferramenta de apoio à
decisão na preparação do veículo para competição sem necessidade de realizar testes
físicos adicionais.

Para além do modelo de suspensão, foi também introduzido o cálculo das cargas verticais
em cada roda individualmente. O deslocamento de cada roda foi obtido aplicando
diretamente a Lei de Hooke, dividindo a força vertical calculada pela rigidez da mola
correspondente. Esta abordagem foi preferida em detrimento das equações demonstradas
no capítulo 4, que expressam o deslocamento de cada roda em função dos movimentos
globais da carroçaria em heave, pitch e roll. Embora matematicamente mais simples, estas
equações assumem que o movimento da carroçaria se propaga de forma igual nas
suspensões de cada roda, o que não reflete de forma realista o comportamento individual
de cada roda quando sujeita a cargas distintas. Optou-se assim por esta via pela sua
simplicidade e robustez: ao calcular o deslocamento de cada roda diretamente a partir da
força vertical que sobre ela atua e da rigidez da respetiva mola, evita-se a propagação de
erros associada à reconstrução do movimento individual a partir dos movimentos globais
da carroçaria, garantindo um resultado fisicamente coerente roda a roda.





                                                                                   79
```

<a id="gaspar-p098"></a>

#### GAS · PDF página 098 de 108

```text
                        Figura 68 - Modelo completo para análise da suspensão.



Para executar a simulação, optou-se por utilizar apenas os primeiros 80 segundos dos
dados de referência, de forma a reduzir o volume de dados a tratar, mas ainda assim num
intervalo suficiente para observar a presença de acelerações longitudinais e laterais
significativas, bem como as respostas de pitch e roll da carroçaria que delas resultam.





                    Figura 69 - Gráfico de comparação entre aceleração longitudinal e pitch.




                                                                                   80
```

<a id="gaspar-p099"></a>

#### GAS · PDF página 099 de 108

```text
                Figura 70 - Gráfico de comparação entre aceleração lateral e roll.



Analisando os gráficos presentes na figura 69 e na figura 70 verifica-se uma correlação
direta e fisicamente coerente entre as acelerações registadas em pista e os movimentos da
carroçaria calculados pelo modelo, assim como era esperado.

Durante os primeiros 20 segundos, o veículo encontra-se em repouso e tanto o ângulo de
pitch como o de roll permanecem nulos, confirmado que as definições das condições
iniciais do modelo estão corretas.

A partir dos 20 segundos, com o início da prova, o ângulo de pitch acompanha de forma
clara as variações da aceleração longitudinal. Sempre que o veículo acelera, o pitch
assume valores positivos, refletindo a transferência de carga para o eixo traseiro. Nos
instantes de travagem mais intensa, como nos 48s e 75s da simulação, o pitch toma valores
negativos, confirmando a transferência de carga para o eixo frontal. As variações do
ângulo de pitch mantém-se num intervalo de aproximadamente ±0,6°, o que é indicativo
da elevada rigidez longitudinal do Formula Student.

Relativamente à dinâmica lateral, o ângulo de roll acompanha diretamente a aceleração
lateral registada. Nas curvas para a direita, onde 𝑎𝑦 é negativo, o roll torna-se positivo,
ou seja, a carroçaria inclina para o exterior da curva e o mesmo inverte-se quando 𝑎𝑦 é
positivo. Os valores máximos de roll situam-se em torno de ±1°.





                                                                                   81
```

<a id="gaspar-p100"></a>

#### GAS · PDF página 100 de 108

```text
                    Figura 71 - Correlação entre Roll e distribuição de carga lateral.



A figura 71 ilustra a correlação entre o ângulo de roll e a transferência de carga lateral no
eixo dianteiro. Até aos 20 segundos, as cargas em ambas as rodas são praticamente iguais,
rondando os 800 N, confirmando uma distribuição simétrica da carga estática.

Com o início da prova, a transferência de carga lateral acompanha diretamente o ângulo
de rol, quando o roll é positivo parte da carga é transferida para a roda direita, e quando
é negativo parte da carga transfere-se para a esquerda. Nos instantes de maior solicitação
lateral, entre os 40s e os 60s, esta assimetria torna-se mais pronunciada, com a roda mais
carregada a atingir valores próximos de 1500 N e a oposta a aproximar-se dos 300 N.





                    Figura 72 - Correlação entre o pitch e a carga longitudinal.



                                                                                   82
```

<a id="gaspar-p101"></a>

#### GAS · PDF página 101 de 108

```text
De forma análoga, a figura 72 ilustra a correlação entre o ângulo de pitch e a transferência
de carga longitudinal. Em aceleração, com pitch positivo, parte da carga sobre o eixo da
frente transfere-se para o eixo traseiro. Nos instantes de travagem, onde o pitch é negativo,
o comportamento inverte-se, com a carga frontal a aumentar e a carga sob a traseira a
reduzir, confirmando a coerência do modelo na dinâmica longitudinal.





                Figura 73 - Curso de suspensão nos primeiros 80s da prova de autocross.



Em repouso, a suspensão dianteira apresenta uma compressão estática de 19 mm e a
traseira de 16 mm. Esta diferença resulta da combinação de dois fatores: por um lado, a
distribuição de massas do veículo, cujo centro de gravidade se encontra mais próximo do
eixo traseiro (a 0,748 m) do que do dianteiro (a 0,802 m), o que faz recair sobre o eixo
traseiro uma fração ligeiramente superior da massa total; por outro, a utilização de
rigidezes distintas entre eixos. A compressão estática de cada eixo traduz, assim, o
equilíbrio entre a carga vertical que sobre ele atua e a rigidez da mola correspondente.

Com o início da prova, os deslocamentos deixam de ser simétricos entre rodas do mesmo
eixo, consequência da sobreposição dos movimentos de pitch e roll induzidos pelas
acelerações registadas em pista. Os picos de compressão da suspensão no eixo da frente
atingem os 35 mm nos instantes de travagem mais intensa, enquanto no eixo traseiro não
excede os 30 mm, como se verifica na figura 73.

As oscilações de alta frequência visíveis no sinal de heave resultam diretamente do ruído
presente nos sinais de aceleração utilizados como entrada, e não de instabilidade numérica
do modelo desenvolvido.





                                                                                   83
```

<a id="gaspar-p102"></a>

#### GAS · PDF página 102 de 108

```text
        7.2.1. Análise do camber dinâmico


Outro ponto de análise relevante é a variação do camber das rodas ao longo da prova.
Apesar de, neste trabalho, não terem sido obtidos os dados dos pneus que permitam
verificar a variação da produção de cargas laterais consoante o ângulo de camber, o
modelo desenvolvido permite calcular o camber dinâmico para futuras análises.

Através dos coeficientes de camber em roll e em bump do Formula Student, e conhecendo
o camber estático do setup utilizado, é possível obter o valor do camber da roda ao longo
de toda a prova. No caso do eixo frontal, os parâmetros utilizados foram os seguintes:

  o  Camber Estático: -2º;
  o  Camber em Roll: 0,69º/º;
  o  Camber em Bump: 0,026º/mm.

Esta monitorização é essencial para garantir que a geometria da suspensão compensa
eficazmente a inclinação do chassis, mantendo o pneu numa gama de trabalho estável.





        Figura 74 - Evolução do camber na roda frente esquerda nos primeiros 80s de prova.



No gráfico presente na figura 74 apresenta-se a evolução do camber na roda dianteira
esquerda ao longo dos 80 segundos de simulação. O gráfico inicia-se no valor de camber
estático do setup utilizado, aproximadamente −2°, mantendo-se nesse valor durante a fase
inicial em que o veículo se encontra praticamente parado, e só se afastando dele a partir



                                                                                   84
```

<a id="gaspar-p103"></a>

#### GAS · PDF página 103 de 108

```text
do momento em que a prova arranca e as acelerações induzem compressão na suspensão
e roll na carroçaria. Com o decorrer da prova, o camber oscila predominantemente entre
−1,0° e −3,0°, nunca assumindo valores positivos, o que confirma que a geometria da
suspensão compensa eficazmente a inclinação do chassis ao longo de toda a prova. Nos
instantes de maior solicitação lateral, entre os 40s e os 80s, é onde se verificam os picos
negativos de -3,0°, correspondentes às curvas mais exigentes onde a suspensão exterior
comprime significativamente.





                Figura 75 - Evolução do camber na roda traseira direita nos primeiros 80s de prova.



No que toca ao eixo traseiro os parâmetros da suspensão são um pouco diferentes:

  o  Camber Estático: -1,5º;
  o  Camber em Roll: 0,545º/º;
  o  Camber em Bump: 0,033º/mm.

Por sua vez, o gráfico da figura 75 revela que o eixo traseiro mantém ângulos de camber
mais baixos em comparação com o eixo frontal, nomeadamente entre -0,4º e -2,5. Esta
configuração é propositada para favorecer a tração longitudinal, assegurando que, mesmo
perante as oscilações dinâmicas da prova, as rodas mantenham uma área de contacto
generosa com o solo para a transmissão da potência. Numa situação de prova de
aceleração o expectável seria afinar o camber estático de modo que durante a mesma, o
camber da roda se mantenha sempre próximo de zero para maximizar a área de contacto
do pneu.





                                                                                   85
```

<a id="gaspar-p104"></a>

#### GAS · PDF página 104 de 108

```text
        7.2.2. Implementação de Anti Roll bar


De forma a demonstrar a utilidade prática do modelo desenvolvido, pretende-se simular
a introdução de uma barra estabilizadora no veículo de forma a verificar a sua influência
no comportamento dinâmico. Segundo o autor (Seward, 2014) a barra estabilizadora não
deve contribuir com mais de 50% da rigidez total em roll em nenhum dos eixos, uma vez
que acima deste limite a suspensão deixa de ser verdadeiramente independente, as rodas
de um mesmo eixo passam a comportar-se como um eixo rígido em irregularidades
unilaterais do pavimento, tornando a suspensão menos eficaz. Com base neste critério,
foi simulada uma contribuição de 40% da rigidez total em roll, analisando o seu efeito no
ângulo de roll da carroçaria e na distribuição de carga lateral.





                    Figura 76 - Comparação dos resultados utilizando anti roll bar.



Na figura 76 verifica-se como a introdução de uma barra estabilizadora com uma
contribuição de 40% da rigidez total em roll permitiu reduzir o pico de roll máximo de
1,105° para 0,832° e o pico mínimo de -0,979° para -0,737°, representando uma redução
de aproximadamente 25% em ambos os sentidos sem necessidade de aumentar a rigidez
vertical das molas. Um intervalo de valores de roll mais pequeno, permite manter os
valores de camber das  rodas também  estes num  intervalo menor,  isto  torna-se
fundamental para o desempenho e otimização da suspensão do veículo, uma vez que uma
equipa que tenha acesso aos dados de forças laterais que os pneus produzem consoante
os valores de camber e carga vertical a que estão sujeitos, conseguem deste modo manter
os pneus na sua gama de maior produção de carga lateral.


                                                                                   86
```

<a id="gaspar-p105"></a>

#### GAS · PDF página 105 de 108

```text
  Conclusão


O presente trabalho teve como objetivo o desenvolvimento de um simulador de dinâmica
de veículos em Simulink para um protótipo de Fórmula Student, com o intuito de criar
uma ferramenta capaz de replicar o comportamento do veículo em diferentes condições
de prova e apoiar a equipa no desenvolvimento e nas decisões de setup do carro.

No que diz respeito aos resultados obtidos, a simulação da prova de aceleração
demonstrou que o modelo foi capaz de replicar o comportamento longitudinal do veículo
com um RMSE de 1,805 km/h, com os movimentos de pitch, heave e a transferência de
carga  longitudinal  a  revelarem-se  fisicamente  coerentes  e  consistentes com  o
comportamento esperado de um Formula Student. Durante a aceleração, a suspensão
dianteira passou de uma compressão estática de 18,59 mm para 6,07 mm, enquanto a
traseira comprimiu adicionalmente de 15,98 mm para 27,79 mm, reflexo direto da
transferência de carga longitudinal que reduziu a carga no eixo dianteiro de 781 N para
255 N e aumentou a carga no eixo traseiro de 838 N para 1456 N. Este fenómeno de squat
traduziu-se num ângulo de pitch máximo de 0,863°, confirmando a coerência física do
modelo. Foi ainda possível verificar que o pneu traseiro esquerdo atingiu uma força
longitudinal máxima de 2952,95 N para um escorregamento de 0,0595, valor abaixo do
ponto de capacidade máxima do pneu, estimado em 0,085 para a carga vertical registada,
o que indicou a existência de uma margem de tração por explorar.

Após a validação do modelo na prova de aceleração, procedeu-se à simulação da prova
de  autocross.  Esta  revelou-se,  contudo,  inviável nas condições  atuais, devido à
instabilidade do controlador PID de controlo da direção perante as entradas transientes
características de um percurso de autocross. Face a esta limitação, o simulador foi
reorientado como ferramenta de análise cinemática, uma abordagem que se revelou de
elevado valor prático para a equipa. Ao receber diretamente os dados de telemetria
registados em pista, o simulador permitiu calcular grandezas tipicamente não medidas
durante as provas, como os movimentos de  pitch,  roll e heave da  carroçaria, o
deslocamento individual da suspensão em cada roda e o camber dinâmico, colmatando
assim uma lacuna de informação que  persiste em equipas que não dispõem de
instrumentação completa. A análise da prova de autocross da FSG 2025 demonstrou que
os picos de compressão da suspensão dianteira atingiram os 35 mm nos instantes de
travagem mais intensa, que o ângulo de roll se manteve dentro de um intervalo de
aproximadamente ±1° e que o ângulo de pitch se manteve num intervalo de ±0,6°,
indicativo da elevada rigidez longitudinal do veículo. O camber dinâmico da roda
dianteira esquerda oscilou predominantemente entre −1,0° e −3,0°, nunca assumindo
valores positivos, confirmando que a geometria da suspensão compensou eficazmente a
inclinação do chassis ao longo de toda a prova. A capacidade de apoiar decisões de setup
foi ainda demonstrada através da simulação da introdução de uma barra estabilizadora
com uma contribuição de 40% da rigidez total em roll, onde se verificou uma redução de
aproximadamente 25% no ângulo de roll, de 1,105° para 0,832° no pico máximo, sem
necessidade de qualquer teste físico adicional.


                                                                                   87
```

<a id="gaspar-p106"></a>

#### GAS · PDF página 106 de 108

```text
   8.1. Trabalhos Futuros

O trabalho desenvolvido estabeleceu uma base sólida sobre a qual assentam três linhas de
desenvolvimento futuro. A primeira, e mais imediata, é o desenvolvimento de um
controlador de direção mais robusto e dotado de antecipação, capaz de lidar com as
entradas transientes rápidas características de um percurso de autocross, ultrapassando
assim a instabilidade que atualmente impede a validação do modelo em condições de
curva. A segunda consiste na validação experimental do modelo de comportamento da
suspensão através da instalação de potenciómetros nos amortecedores do veículo, o que
permitiria comparar diretamente o deslocamento real de cada roda em pista com o
estimado pelo modelo e, dessa forma, aumentar a confiança nos resultados obtidos. Por
fim, a evolução mais ambiciosa passa pela integração de um simulador de tempo por volta
com o modelo de dinâmica de suspensão desenvolvido neste trabalho, combinando a
previsão do desempenho do veículo ao longo de uma volta completa com a análise
detalhada do comportamento da suspensão, e tornando a ferramenta capaz de avaliar o
desempenho do veículo em qualquer circuito.

A realização deste trabalho foi igualmente relevante a nível pessoal, tendo permitido
consolidar competências em dinâmica de  veículos e em MATLAB e Simulink,
fundamentais para a ambição de futuramente trabalhar como engenheiro de performance
no mundo da competição automóvel. A ferramenta desenvolvida representou ainda um
contributo concreto e duradouro para a equipa, ficando disponível para ser utilizada e
desenvolvida pelas gerações seguintes.





                                                                                   88
```

<a id="gaspar-p107"></a>

#### GAS · PDF página 107 de 108

```text
  Bibliografia


Amaral, henrique. (2022). Vehicle Dynamics Analysis in a Formula Student Racing
    Car.

Damian Harty, & Michael Blundell. (2015). The Multibody Systems Approach to
     Vehicle Dynamics (second).

Domingues dos Santos, R., Alberto Gonçalves de Sousa, L., & Manuel Varejão de
     Oliveira Faria Supervisor, L. (2014). Model ans Dynamic Simulation Program For
     Vehicle Analysis Accounting Suspension Compliance Mechanical Engineering
    Examination Committee.

Emrax. (2025). EMRAX_228_datasheet_v1.6 (1).

Guiggiani, M. (2014). The science of vehicle dynamics: Handling, braking, and ride of
     road and race cars. In The Science of Vehicle Dynamics: Handling, Braking, and
    Ride of Road and Race Cars (Vol. 9789401785334). Springer Netherlands.
     https://doi.org/10.1007/978-94-017-8533-4

H. Marzbani, & Reza N. Jazar. (2024). Vehicle Vibrations.

Höglund, R. (2022). Modelling of a high-performance vehicle in MATLAB/Simulink and
    Canopy Simulations. http://www.kth.se/sci

https://news.cision.com/koenigsegg/r/. (n.d.). Retrieved December 10, 2025, from
     https://news.cision.com/koenigsegg/r/

Limpert, R. (2011). Brake Design and Safely Third Edition. http://books.sae.org

Mastinu, G., & Ploechl, M. (2014). Road and off-road vehicle system dynamics
    handbook. Road and Off-Road Vehicle System Dynamics Handbook, 1–1780.
     https://doi.org/10.1201/b15560

Milliken, W. F. , & M. D. L. (1995). Race Car Vehicle Dynamics.

Narby, E. (2006). Institutionen för systemteknik Department of Electrical Engineering
    Examensarbete Modeling and Estimation of Dynamic Tire Properties
    Examensarbete utfört i Reglerteknik vid Tekniska högskolan i Linköping av.

Ogata, Katsuhiko. (2022). Modern control engineering. Prentice Hall.

Optimum G. (2026). OptimumLap - Simple simulations. Powerful insights.

Pacejka, H. (2006). Tyre and Vehicle Dynamics.

Ramanata, P. P. (1998). Chapter 2 Vehicle Dynamics Modeling.

Rodrigues, J. (2016). ApontamentosVSM_Ed2021.2.



                                                                                   89
```

<a id="gaspar-p108"></a>

#### GAS · PDF página 108 de 108

```text
Seward, D. (2014). Race Car Design.

Tyre Slip Angle – Geometry Explained Suspension Secrets. (2024).
     https://suspensionsecrets.co.uk/tyre-slip-angle/

Xsense. (2025). MTi-680G.





                                                                                   90
```

---

<a id="fuente-diaz"></a>

## Fuente DIA — Analysis and Design of a Roll-Heave Decoupled Front Suspension for a Formula Student Prototype

**Autor(es):** José Díaz de Rábago Areilza. **Año:** 2024. **Idioma:** inglés, con resumen en español. **Páginas del PDF:** 101. **Documento:** `TFG-DiazdeRabagoAreilza,Jose.pdf`. **Original:** https://repositorio.comillas.edu/bitstreams/887ca4f7-e552-4a81-a3af-8779b3db931e/download.

### Índice de páginas de esta fuente

[1](#diaz-p001) · [2](#diaz-p002) · [3](#diaz-p003) · [4](#diaz-p004) · [5](#diaz-p005) · [6](#diaz-p006) · [7](#diaz-p007) · [8](#diaz-p008) · [9](#diaz-p009) · [10](#diaz-p010) · [11](#diaz-p011) · [12](#diaz-p012) · [13](#diaz-p013) · [14](#diaz-p014) · [15](#diaz-p015)

[16](#diaz-p016) · [17](#diaz-p017) · [18](#diaz-p018) · [19](#diaz-p019) · [20](#diaz-p020) · [21](#diaz-p021) · [22](#diaz-p022) · [23](#diaz-p023) · [24](#diaz-p024) · [25](#diaz-p025) · [26](#diaz-p026) · [27](#diaz-p027) · [28](#diaz-p028) · [29](#diaz-p029) · [30](#diaz-p030)

[31](#diaz-p031) · [32](#diaz-p032) · [33](#diaz-p033) · [34](#diaz-p034) · [35](#diaz-p035) · [36](#diaz-p036) · [37](#diaz-p037) · [38](#diaz-p038) · [39](#diaz-p039) · [40](#diaz-p040) · [41](#diaz-p041) · [42](#diaz-p042) · [43](#diaz-p043) · [44](#diaz-p044) · [45](#diaz-p045)

[46](#diaz-p046) · [47](#diaz-p047) · [48](#diaz-p048) · [49](#diaz-p049) · [50](#diaz-p050) · [51](#diaz-p051) · [52](#diaz-p052) · [53](#diaz-p053) · [54](#diaz-p054) · [55](#diaz-p055) · [56](#diaz-p056) · [57](#diaz-p057) · [58](#diaz-p058) · [59](#diaz-p059) · [60](#diaz-p060)

[61](#diaz-p061) · [62](#diaz-p062) · [63](#diaz-p063) · [64](#diaz-p064) · [65](#diaz-p065) · [66](#diaz-p066) · [67](#diaz-p067) · [68](#diaz-p068) · [69](#diaz-p069) · [70](#diaz-p070) · [71](#diaz-p071) · [72](#diaz-p072) · [73](#diaz-p073) · [74](#diaz-p074) · [75](#diaz-p075)

[76](#diaz-p076) · [77](#diaz-p077) · [78](#diaz-p078) · [79](#diaz-p079) · [80](#diaz-p080) · [81](#diaz-p081) · [82](#diaz-p082) · [83](#diaz-p083) · [84](#diaz-p084) · [85](#diaz-p085) · [86](#diaz-p086) · [87](#diaz-p087) · [88](#diaz-p088) · [89](#diaz-p089) · [90](#diaz-p090)

[91](#diaz-p091) · [92](#diaz-p092) · [93](#diaz-p093) · [94](#diaz-p094) · [95](#diaz-p095) · [96](#diaz-p096) · [97](#diaz-p097) · [98](#diaz-p098) · [99](#diaz-p099) · [100](#diaz-p100) · [101](#diaz-p101)

### Índice de secciones del documento

- [Resúmenes iniciales](#diaz-p009)
- [Contexto y objetivos](#diaz-p026)
- [Vehicle Dynamics](#diaz-p036)
- [Suspension and Roll/Heave Control](#diaz-p041)
- [Dynamic Response Design](#diaz-p059)
- [Physical Design](#diaz-p067)
- [Final Assembly](#diaz-p081)
- [Finite Element Analysis](#diaz-p084)
- [Conclusions](#diaz-p095)

### Índice de figuras y tablas localizadas en el texto

- [PDF pág. 25](#diaz-p025): Table 1. Definition of the rocker pivot point and geometry ................................................ 54
- [PDF pág. 25](#diaz-p025): Table 2. The parameters of the mesh of the left-side rocker ............................................... 67
- [PDF pág. 25](#diaz-p025): Table 3. The parameters of the mesh of the right-side rocker ............................................. 67
- [PDF pág. 25](#diaz-p025): Table 4. The parameters of the mesh of the controlled region of the left-side rocker ........ 68
- [PDF pág. 25](#diaz-p025): Table 5. The parameters of the mesh of the controlled region of the right-side rocker ...... 68
- [PDF pág. 27](#diaz-p027): Figure 1. The team at Formula Student Spain 2023
- [PDF pág. 28](#diaz-p028): Figure 2. Layout for the Skidpad event. [1]
- [PDF pág. 29](#diaz-p029): Figure 3. Maximum point allocation for the different events in Formula Student [1]
- [PDF pág. 30](#diaz-p030): Figure 4. The IFS-05 Competing at the endurance event in Formula Student Spain.
- [PDF pág. 32](#diaz-p032): Figure 5. Generic FS track layout. [2]
- [PDF pág. 38](#diaz-p038): Figure 6. Illustration of slip angle. [4]
- [PDF pág. 40](#diaz-p040): Figure 7. Free body diagram describing the forces involved in load transfer. a) longitudinal and b) lateral. [7]
- [PDF pág. 41](#diaz-p041): Figure 8. Diagram of the different rotational movements defined. [9]
- [PDF pág. 42](#diaz-p042): Figure 10. Solid axle for link suspension. [5]
- [PDF pág. 42](#diaz-p042): Figure 11. MacPherson Strut suspension. [5]
- [PDF pág. 43](#diaz-p043): Figure 12. Double Wishbone suspension of the current IFS-06 Prototype
- [PDF pág. 44](#diaz-p044): Figure 13. Diagram showing the negative camber angle. [10]
- [PDF pág. 45](#diaz-p045): Figure 14. Diagram showing the toe angles. [11]
- [PDF pág. 46](#diaz-p046): Figure 15. Diagram defining mechanical trail and caster angle. [12]
- [PDF pág. 46](#diaz-p046): Figure 16. Diagram showing the KPI and scrub radius.[13]
- [PDF pág. 48](#diaz-p048): Figure 17. The rear u-shape ARB of the ISC’s IFS-06
- [PDF pág. 48](#diaz-p048): Figure 18. The roll-heave decoupled suspension system on TU Brno’s prototype. [14]
- [PDF pág. 49](#diaz-p049): Figure 19. The fully decoupled hydraulic suspension of AMZ’s Formula student prototype. [15]
- [PDF pág. 51](#diaz-p051): Figure 20. Close up of the ISF-06 front suspension
- [PDF pág. 51](#diaz-p051): Figure 21. Close up of the IFS-06 rear suspension
- [PDF pág. 54](#diaz-p054): Figure 22. CAD model of the current IFS-06 chassis, the basis for the design
- [PDF pág. 55](#diaz-p055): Figure 23. The IFS-06 Front suspension in the Kinematic 3D Analysis MATLAB app
- [PDF pág. 56](#diaz-p056): Figure 24. The current spring-damper element
- [PDF pág. 57](#diaz-p057): Figure 25. Image of the car oversteering at last year’s Formula Student Spain Skidpad event
- [PDF pág. 58](#diaz-p058): Figure 26. Camber gain during roll of the current suspension geometry
- [PDF pág. 59](#diaz-p059): Figure 27. Diameter comparison between the old (upper) and the new (lower) rear ARB
- [PDF pág. 60](#diaz-p060): Figure 28. Front heave Dynamic model
- [PDF pág. 61](#diaz-p061): Figure 29. Front roll Dynamic model
- [PDF pág. 64](#diaz-p064): Figure 30. Graph simulating a 1 cm disturbance in heave
- [PDF pág. 66](#diaz-p066): Figure 31. Graph simulating a 1.26 g lateral acceleration for roll
- [PDF pág. 67](#diaz-p067): Figure 32. Internal cockpit cross section template
- [PDF pág. 68](#diaz-p068): Figure 33. Design template for the front suspension system into the cockpit
- [PDF pág. 69](#diaz-p069): Figure 34. Diagram showing the compression of the heave spring and no reaction of the roll spring when both tyres move equally. [16]
- [PDF pág. 69](#diaz-p069): Figure 35. Diagram showing the compression of the roll spring and no reaction of the heave spring when both tyres move opposite. [16]
- [PDF pág. 70](#diaz-p070): Figure 36. Image of the CAD of the chassis with the bar to be deleted highlighted
- [PDF pág. 71](#diaz-p071): Figure 37. Image of the CAD of the chassis with the position of the pushrod highlighted
- [PDF pág. 72](#diaz-p072): Figure 38. Dynamic motion ratio for the roll spring
- [PDF pág. 72](#diaz-p072): Figure 39. Dynamic motion ratio for the heave spring
- [PDF pág. 73](#diaz-p073): Table 1. Definition of the rocker pivot point and geometry
- [PDF pág. 74](#diaz-p074): Figure 40. Font, Side and Isometric view of the rocker for the right side of the vehicle
- [PDF pág. 75](#diaz-p075): Figure 41. Font, Side and Isometric view of the rocker for the left side of the vehicle
- [PDF pág. 76](#diaz-p076): Figure 42. Cut view of the NTN 6902LLU/5K bearing
- [PDF pág. 77](#diaz-p077): Figure 43. Front, isometric and top views of the rocker mounting point design
- [PDF pág. 78](#diaz-p078): Figure 44. Diagram showing the compression of the “double spring” device and the compression of the spring
- [PDF pág. 78](#diaz-p078): Figure 45. Diagram showing the elongation of the “double spring” device and the compression of the spring
- [PDF pág. 79](#diaz-p079): Figure 46. Image of the damper mounting CAD
- [PDF pág. 79](#diaz-p079): Figure 47. Image of the rocker mounting CAD
- [PDF pág. 80](#diaz-p080): Figure 48. Image of the CAD model of the general design of the heave spring extension mounted on the spring damper system
- [PDF pág. 80](#diaz-p080): Figure 49. Image of the CAD model of the mounting point for the damper
- [PDF pág. 81](#diaz-p081): Figure 50. Image of the CAD model of the mounting point for the rocker
- [PDF pág. 81](#diaz-p081): Figure 51. Image of the CAD model of the final design assembled
- [PDF pág. 82](#diaz-p082): Figure 52. Image of the CAD model of the front plane of the design mounted into the chassis
- [PDF pág. 82](#diaz-p082): Figure 53. Image of the CAD model of the top plane of the design mounted into the chassis
- [PDF pág. 83](#diaz-p083): Figure 54. Image of the CAD model of the isometric view of the design mounted into the chassis
- [PDF pág. 85](#diaz-p085): Figure 55. Image of the CAD model of the left-side rocker with the changes highlighted
- [PDF pág. 85](#diaz-p085): Figure 56. Image of the CAD model of the right-side rocker with the changes highlighted
- [PDF pág. 86](#diaz-p086): Table 2. The parameters of the mesh of the left-side rocker
- [PDF pág. 86](#diaz-p086): Table 3. The parameters of the mesh of the right-side rocker
- [PDF pág. 87](#diaz-p087): Table 4. The parameters of the mesh of the controlled region of the left-side rocker
- [PDF pág. 87](#diaz-p087): Table 5. The parameters of the mesh of the controlled region of the right-side rocker
- [PDF pág. 88](#diaz-p088): Figure 57. Mesh of the left-side rocker
- [PDF pág. 88](#diaz-p088): Figure 58. Closeup of the mesh of the controlled region of the left-side rocker
- [PDF pág. 89](#diaz-p089): Figure 59. Mesh of the right-side rocker
- [PDF pág. 89](#diaz-p089): Figure 60. Closeup of the mesh of the controlled region of the right-side rocker
- [PDF pág. 90](#diaz-p090): Figure 61. Image of the von Mieses stresses of the right-side rocker
- [PDF pág. 90](#diaz-p090): Figure 62. Image of the static deflection of the right-side rocker
- [PDF pág. 91](#diaz-p091): Figure 63. Image of the factor of safety of the right-side rocker
- [PDF pág. 91](#diaz-p091): Figure 64. Detail of the most stressed part of the right-side rocker
- [PDF pág. 92](#diaz-p092): Figure 65. Image of the von Mieses stresses of the left-side rocker
- [PDF pág. 92](#diaz-p092): Figure 66. Image of the static deflection of the left-side rocker
- [PDF pág. 93](#diaz-p093): Figure 67. Image of the factor of safety of the left-side rocker
- [PDF pág. 93](#diaz-p093): Figure 68. Detail of the most stressed part of the right-side rocker
- [PDF pág. 101](#diaz-p101): Figure 69. SDG’s

### Transcripción por página

<a id="diaz-p001"></a>

#### DIA · PDF página 001 de 101

```text
 GRADO EN INGENIERÍA EN TECNOLOGÍAS
             INDUSTRIALES





             TRABAJO FIN DE GRADO

Analysis and design of a roll-heave decoupled front
   suspension for a Formula Student prototype





               Autor: José Díaz de Rábago Areilza

             Director: Antonio Hernández-Ros Briales





                          Madrid
```

<a id="diaz-p002"></a>

#### DIA · PDF página 002 de 101

*Página sin texto extraíble (consultar el PDF original).*

<a id="diaz-p003"></a>

#### DIA · PDF página 003 de 101

```text
      Declaro, bajo mi responsabilidad, que el Proyecto presentado con el título

Analysis and design of a roll-heave decoupled front suspension for a Formula Student

                                    prototype

     en la ETS de Ingeniería - ICAI de la Universidad Pontificia Comillas en el

           curso académico 2023/24 es de mi autoría, original e inédito y

             no ha sido presentado con anterioridad a otros efectos.

El Proyecto no es plagio de otro, ni total ni parcialmente y la información que ha sido

           tomada de otros documentos está debidamente referenciada.





           Fdo.: José Díaz de Rábago Areilza       Fecha: 10/07/ 2024



                       Autorizada la entrega del proyecto

                 EL DIRECTOR DEL PROYECTO





          Fdo.: Antonio Hernández-Ros Briales     Fecha: 10/ 07/ 2024
```

<a id="diaz-p004"></a>

#### DIA · PDF página 004 de 101

*Página sin texto extraíble (consultar el PDF original).*

<a id="diaz-p005"></a>

#### DIA · PDF página 005 de 101

```text
 GRADO EN INGENIERÍA EN TECNOLOGÍAS
             INDUSTRIALES





             TRABAJO FIN DE GRADO

Analysis and design of a roll-heave decoupled front
   suspension for a Formula Student prototype





               Autor: José Díaz de Rábago Areilza

             Director: Antonio Hernández-Ros Briales





                          Madrid
```

<a id="diaz-p006"></a>

#### DIA · PDF página 006 de 101

*Página sin texto extraíble (consultar el PDF original).*

<a id="diaz-p007"></a>

#### DIA · PDF página 007 de 101

```text
Acknowledgements


Having reached the end of my engineering degree, I look back with gratitude to all the
people who have been there with unwavering support through difficult times.

To my family, who have encouraged and enabled me to follow my path in life always.

To my project lead Antonio, who was the perfect person to understand this project and help
me guide it to the goal.

To my friends, especially to Álvaro, Brandon, Íñigo, Javier, Javier, Miguel, Pablo, Pedro
and Santiago, who have helped me at all times to get back up from the ground and continue
and to celebrate the achievements.

To the whole ISC Formula Student racing team, who have helped me increase my love for
engineering, as well as allowed me to fulfil my dream of designing, a competition vehicle.
```

<a id="diaz-p008"></a>

#### DIA · PDF página 008 de 101

*Página sin texto extraíble (consultar el PDF original).*

<a id="diaz-p009"></a>

#### DIA · PDF página 009 de 101

```text
ANALYSIS AND DESIGN OF A ROLL-HEAVE DECOUPLED FRONT
SUSPENSION FOR A FORMULA STUDENT PROTOTYPE
Author: Díaz de Rábago Areilza, José.
Supervisor: Hernández-Ros Briales, Antonio.
Collaborating Entity: ICAI– Universidad Pontificia Comillas


ABSTRACT

In Formula Student, the design of the suspension system is the most important part for the
dynamic control of the vehicle and therefore vehicle performance. The thesis describes the
process of studying and designing a new suspension system for a Formula Student prototype.
This design will have as a base the current general suspension geometry and therefore will
have to completely adapt to it. The main goal of the project is to understand the advantages
and design using CAD a system capable of controlling the motions of the vehicle
independently from each other.

Keywords: Formula Student, Vehicle dynamics, Suspension systems, Suspension design,

Roll-heave decoupling, Dynamic analysis, FEA validation, CAD modelling, Tyre dynamics,

SolidWorks.

1.  Introduction

   Formula Student is an international engineering competition for universities in which
   teams from all over the world design, manufacture and compete their formula type
    prototypes. The main focus of the competition is on the engaging of students in real
   world projects, where they can learn to understand the realities of a big scale project like
    designing, manufacturing and competing a race car. During the competition, the vehicle
    is put to the test on track, and the team is assessed on the design decisions behind the
    project’s development. [1]

   The importance of a well-designed and calibrated suspension system as well as a good
   understanding of vehicle dynamics is fundamental in Formula Student, as the on-track
   performance is mostly gained by the maximization of the vehicles available grip at low
   speeds and its handling characteristics. The optimization of both of these metrics is done
   through a correct design of the suspension and a correct understanding of vehicle
   dynamics.

2.  State of the art

   The correct design of a suspension system is done through the understanding of vehicle
   dynamics.

   Vehicle dynamics is a field that is centred around the analysis of the forces a vehicle is
    subjected to, and how it reacts to them. It not only describes the reaction forces generated,
    but also the relative movement of the vehicle. The field of vehicle dynamics in regard to
   suspension design can be separated into three sub fields.

         - Tyre mechanics: Define how the most important part of the vehicle, the tyres,
      behave under any circumstances depending on all the possible analysable parameters.
```

<a id="diaz-p010"></a>

#### DIA · PDF página 010 de 101

```text
       This field is fundamental on the understanding of the vehicles behaviour because the
        tyres are the part responsible for gripping the tarmac, which means they are the way
       the vehicle moves. The study of the effect of specially tyre position on the grip of the
        tyres is fundamental. [2],[3]

      -Kinematics: Kinematics study the relative movement of the wheels and the vehicle,
       defining every possible position the tyre can be in. The kinematic behaviour of the
       vehicle also defines the other relative movements of the vehicle such as the steering
      and the compression of the spring-damper systems. This means that the kinematic
       behaviour of the car defines the handling characteristics, as well as the maximum
       available traction for the tyres at any given moment.

      -Load control: The understanding and control of the variation in the loads seen by
       the tyres at any moment due to the different accelerations the vehicle is subjected to,
         is fundamental in obtaining not only the desired performance from the tyres at any
        point, but also, the correct stability of the vehicle when reacting to the changing loads
       presented by the constantly changing accelerations.

   Once these fields are understood and put together, the result is a complete model of the
    possible dynamic behaviour of the vehicle. In this model, one can observe three main
    rotational movements across the vertical, lateral and longitudinal axis of the vehicle.
   These are known as yaw, pitch and yaw, and greatly affect the kinematics of the vehicle
   when moving.





                          Image 1. Rotational movements of the vehicle. [4]

   The other part which is needed to understand how a suspension is designed, is how a
   suspension system works. The general overview of the mechanisms of a suspension
   system is this. A suspension system is comprised of rigid structures which define the
   kinematics of the system and elastic and dampening elements which define the dynamics
    of the system. The elastic systems can be interconnected between the wheels to better
    control the relative motions of the connected wheels.

3.  Objectives

   The main objective for the project is to analyse, design and integrate a new front
   suspension system which will be able to control independently the vertical (longitudinal)
   dynamics of the  front axle from  the  lateral dynamics, with  the aim being  the
   improvement of the cornering dynamics of the vehicle. Ease of manufacturing, a
```

<a id="diaz-p011"></a>

#### DIA · PDF página 011 de 101

```text
    parametric design to be able to adapt to the system to possible future prototypes and the
    reduction of the prototypes centre of mass are also objectives of the design.

4.  Design and verification

   The design process for the project was divided into two: the dynamic response design
   and the physical design.

   The dynamic response design consisted in the formulation of mathematical equivalent
   systems to the vertical and lateral behaviours of the car and the selection of the design
   parameters to obtain the desired response. The formulations of these systems consisted
    in a one degree of freedom model for the roll movement and a two degree of freedom
   system for the vertical (Heave) movement.

      For the roll movement a one degree of freedom model was chosen because the
       objective for the roll control is mainly to achieve the desired roll angle for a given
       constant lateral acceleration, in order to maintain the wheels at the desired position
        relative to the road during high lateral acceleration manoeuvres.

      For the heave movement a two degree of freedom model was chosen to mainly take
        into account other possible factors such as wheel hop and how they affect the system,
       as well as a way to better understand the dynamic behaviour of the vehicle under
       longitudinal acceleration.





                  Image 2. Two degree of freedom model designed for the heave movement

   The final design parameters of the dynamic response design were:

                             System freq. (Hz)         3.91
                           Wheel hop freq. (Hz)       24.36
                                     Roll gradient (°/g)         1
                        Heave spring stiffness (N/m)   43780
                                 Roll spring stiffness (N/m)    10000
                              Table 1. Dynamic response design parameters

   The physical design consisted in in the modelling of a system that could decouple the
    roll and heave movements on the dampers and springs so when the axle is rolling only
```

<a id="diaz-p012"></a>

#### DIA · PDF página 012 de 101

```text
the roll elements are actuated, and when the axle is moving vertically only the heave
elements are being actuated.

In order to achieve this, two asymmetric rockers were designed which accommodate
between them a spring-damper element parallel to the ground that controls heave, and
one which crosses diagonally that controls roll.





                           Image 3. Roll-heave decoupled system

The motion ratio of the rockers was one of the most important design factors as, together
with the preexisting kinematics of the vehicle define the compression rate for the spring-
damper elements. The final design having a 1.2 motion ratio for heave and a 0.8 motion
ratio for roll.





         Image 4. Graph showing the effective motion ratios for heave (blue) and roll(red)

To be able to incorporate the preexisting spring-damper elements into the design an
extension had to be designed to fit the heave element and a “double spring” device had
to also be designed in order for the roll spring to be able to be compressed by for
compression and traction strokes.





                      Image 5. Double spring mechanism in compression
```

<a id="diaz-p013"></a>

#### DIA · PDF página 013 de 101

```text
                           Image 6. Double spring mechanism in traction

    After the main design phase was finished, the validation of the designs begun. By
    analysing the physical design in a Finite Element Analysis software, the validation of the
   design was finished obtaining a Factor of safety of 1.3 at the lowest value.





            Image 7. Simulation in SolidWorks of the designed pieces and their Factor of safety

5.  Conclusions

   The design and understanding of the roll-heave suspension system has been completed
   and verified, and the design can be taken as successful. The next step to take is to bring
    the design to the real world and validate it in real life. The main objectives achieved
   were:

     - The design of the front suspension to incorporate a roll-heave decoupled system.

     - A better understanding and mathematical modelling of the dynamic behaviour of the
    vehicle.

     - An improved dynamic behaviour of the vehicle by the correct selection of spring
    stiffnesses for both heave and roll.

     - Better understanding of the kinematic behaviour of the vehicle’s suspension.

6.  References

    [1]  FSG, Formula Student Rules 2024. Version 1.1, 2024.

    [2] W. F. Milliken and D. L. Milliken, Race Car Vehicle Dynamics, 1995.

    [3]  C. Smith, Tune to Win, 1978.

    [4]  Hasan-Farag, Rana Raouf. “Active neuro-fuzzy integrated vehicle dynamics controller to

       improve the vehicle handling and stability at complicated manoeuvres.”, 2016.
```

<a id="diaz-p014"></a>

#### DIA · PDF página 014 de 101

```text
ANÁLISIS Y  DISEÑO DE UNA  SUSPENSIÓN DELANTERA
DESACOPLADA EN BALANCEO-CABECEO PARA UN PROTOTIPO
DE FÓRMULA STUDENT
Autor: Díaz de Rábago Areilza, José.
Director: Hernández-Ros Briales, Antonio.
Entidad Colaboradora: ICAI– Universidad Pontificia Comillas


RESUMEN DEL PROYECTO

En la Fórmula Student, el diseño del sistema de suspensión la parte más importante para el

control dinámico y por lo tanto para el rendimiento del vehículo. Este trabajo describe el

proceso detrás del estudio y el diseño de un nuevo sistema de suspensión para un prototipo

de fórmula Student. El diseño tendrá de base la actual geometría de suspensión y por lo tanto

tendrá que adaptarse a esta. El objetivo principal del proyecto es entender las ventajas de un

sistema capaz de controlar de manera independiente todos los movimientos del vehículo y

diseñar este sistema en CAD.

Palabras clave: Formula Student, Dinámica vehicular, Sistemas de suspensión, diseño de

suspensiones, Desacoplamiento de Cabeceo-Balanceo, Análisis dinámico, Validación en

FEA, Modelado en CAD, Dinámica de los neumáticos, SolidWorks.

1.  Introducción

   Fórmula Student es una competición internacional de ingeniería de universidades en la
   que equipos de todo el mundo diseñan, fabrican y compiten prototipos estilo fórmula. El
    principal foco de la competición es el interesar a los estudiantes en proyectos reales, en
   donde pueden entender las realidades detrás de proyectos grandes como es el diseño,
    fabricación y competición de un coche de carreras. En la competición el vehículo es
   probado en la pista y los equipos son evaluados en las decisiones de diseño y liderazgo
   tomadas durante el desarrollo del proyecto. [1]

   La importancia de un sistema de suspensión bien diseñado y con los correctos reglajes,
    junto con un buen conocimiento de la dinámica vehicular es fundamental en Fórmula
    Student, ya que el rendimiento en pista es obtenido principalmente por la maximización
    del  agarre a  bajas velocidades y  la maniobrabilidad  del  prototipo. La  correcta
   optimización de estos parámetros debe llevarse a cabo a través del diseño de la
   suspensión y del conocimiento de la dinámica vehicular.

2.  Estado del arte

   El correcto diseño de un sistema de suspensión debe ser realizado desde un profundo
   conocimiento de la dinámica vehicular.

   La dinámica vehicular es el campo de la ciencia centrado en el análisis de las fuerzas a
    las que está sometido un vehículo y como el vehículo reacciona a las fuerzas. La
```

<a id="diaz-p015"></a>

#### DIA · PDF página 015 de 101

```text
dinámica vehicular no solo describe las fuerzas de reacción del vehículo, sino que
también define los movimientos relativos de este. La dinámica vehicular, referida al
diseño de suspensiones puede ser dividida en tres subcampos.

    - Mecánica de los neumáticos: Define cómo la parte más importante del vehículo,
    los neumáticos, se comportan bajo cualquier circunstancia dependiendo de todos los
   parámetros analizables. Este campo es fundamental para  el entendimiento del
   comportamiento del vehículo, ya que las ruedas son la parte responsable de agarrarse
    al asfalto, lo que significa que son la razón por la que el vehículo puede moverse. El
    estudio del efecto especialmente de la posición relativa de los neumáticos con el suelo
   en el agarre de estos es fundamental. [2],[3]

   -Cinemática: La cinemática estudia el movimiento relativo de las ruedas con el resto
    del vehículo, definiendo todas las posibles posiciones en las que la rueda puede estar.
   El comportamiento cinético del vehículo define también el resto de los movimientos
    relativos como son los del sistema de dirección o los de la compresión de los muelles-
   amortiguadores. Esto significa que el comportamiento cinético define también la
   maniobrabilidad del coche y la máxima tracción posible de las ruedas debido a su
    posición.

   -Control de carga: El entendimiento del control y la variación de las cargas vistas
   por las ruedas en todo momento (debidas a las aceleraciones a las que está sometido
    el vehículo) es fundamental para obtener, no solo el rendimiento requerido de las
    ruedas, sino que también es necesario para obtener la correcta estabilidad dinámica
    del vehículo.

Una vez entendidos estos campos se entiende como al unirlos se puede obtener un
modelo dinámico del vehículo. En este modelo se pueden observar tres movimientos
rotacionales en los ejes vertical, longitudinal y lateral. Estos movimientos se llaman
guiñada, cabeceo y balanceo y afectan en gran nivel a la cinemática del vehículo en
movimiento.





                     Imagen 1. Movimientos rotacionales del vehículo. [4]

La otra parte fundamental para entender el diseño de suspensiones es el funcionamiento
de un sistema de suspensión. En general, el sistema de suspensiones se puede definir así.
Un sistema de suspensión está formado por estructuras rígidas que definen la cinemática
del sistema y por elementos elásticos y amortiguadores que controlan la dinámica del
```

<a id="diaz-p016"></a>

#### DIA · PDF página 016 de 101

```text
    sistema. Estos elementos elásticos y amortiguadores pueden estar conectados entre sí
   para poder controlar mejor los movimientos relativos de las ruedas conectadas.

3.  Objetivos

   El principal objetivo del proyecto es analizar, diseñar e integrar un nuevo sistema de
   suspensión  delantero que  tiene  la capacidad de  controlar independientemente  el
   desplazamiento vertical del eje (como el que se ve durante el cabeceo), del balanceo del
    eje, con la meta siendo la mejora del comportamiento del vehículo en curva. La facilidad
   de fabricación, diseño paramétrico para poder adaptar el sistema a futuros prototipos y
    la reducción del centro de masa del prototipo son también objetivos del diseño.

4.  Diseño y verificación

   El proceso de diseño del proyecto se dividió en dos el diseño de la respuesta dinámica,
   y el diseño físico.

   El diseño de la respuesta dinámica consistió en la formulación de modelos matemáticos
    equivalentes a los sistemas dinámicos de los comportamientos laterales y verticales y la
    selección de los parámetros de diseño para la obtención de la respuesta dinámica deseada.
   La formulación de estos sistemas consistió en un sistema de un grado de libertad para el
   movimiento de cabeceo, y en un sistema de dos grados de libertad para el movimiento
    vertical.

       Para el movimiento de cabeceo un sistema de un grado de libertad fue elegido ya que
        el objetivo con el control del balanceo es principalmente conseguir cierto ángulo para
      una aceleración lateral constante, para poder mantener las ruedas en la posición
       deseada con respecto a la carretera en maniobras de gran aceleración lateral.

       Para el movimiento vertical un sistema de dos grados de libertad fue elegido para
       principalmente tener en cuenta factores de múltiples modos de vibración como es el
        “salto de rueda” y ver los posible problemas que pueden causar al sistema en general.
      También para poder entender mejor al comportamiento del vehículo en aceleración
        longitudinal.
```

<a id="diaz-p017"></a>

#### DIA · PDF página 017 de 101

```text
           Imagen 2. Modelo de dos grados de Libertad diseñado para el movimiento vertical

Los parámetros finales del diseño de la respuesta dinámica fueron:

                             Frec. del sistema (Hz)       3.91
                           Frec. de salto de rueda (Hz)    24.36
                        Gradiente de balanceo (°/g)     1
                          Rigidez del muelle v. (N/m)   43780
                          Rigidez del muelle b. (N/m)   10000
                 Tabla 1. Parámetros finales del diseño de la respuesta dinámica

El diseño físico consistió en el modelado de un sistema que pudiera desacoplar el
balanceo del movimiento vertical en los muelles-amortiguadores, de tal manera que
cuando el eje delantero sufre balanceo solo se ven afectados los elementos encargados
del balanceo, y cuando el movimiento es vertical, solo se ven afectados los elementos
encargados del movimiento vertical.

Para poder obtener un diseño como el descrito, dos balancines asimétricos fueron
diseñados para acomodar entre ellos un muelle-amortiguador paralelo con el suelo para
encargarse del movimiento vertical y uno en diagonal para encargarse del balanceo.





                     Imagen 3. Sistema desacoplado en balanceo-cabeceo

La relación cinética de los balancines es uno de los factores más importantes en este
diseño, ya que junto a la cinemática preexistente definen el ratio de compresión de los
muelles-amortiguadores. El diseño final tiene una relación de 1.2 en movimiento vertical
y de 0.8 en balanceo.





   Imagen 4. Gráfico con las relaciones cinéticas para movimiento vertical (azul) y balanceo (rojo)
```

<a id="diaz-p018"></a>

#### DIA · PDF página 018 de 101

```text
   Para poder incorporar los muelles-amortiguadores previos en el diseño se tuvo que
    fabricar una extensión para el elemento de movimiento vertical y un sistema de “muelle
   doble” se tuvo que diseñar para que el elemento de balanceo pudiera ser comprimido
   para tanto movimientos de tracción como de compresión.





                        Imagen 5. Mecanismo de doble muelle en compresión





                          Imagen 6. Mecanismo de doble muelle en tracción

   Tras la fase de diseño principal, la fase de validación comenzó. La validación se llevó a
   cabo en un software de simulación por elementos finitos obteniendo un factor de
   seguridad mínimo de 1.3.





          Imagen 7. Simulación en SolidWorks de las piezas diseñadas con su factor de seguridad

5.  Conclusiones

   El diseño y entendimiento del sistema desacoplado en cabeceo-balanceo ha sido
   completo y verificado, con el diseño pudiendo ser dado por válido. El siguiente paso será
    traer el diseño a la realidad y verificarlo correctamente fuera de la simulación. Los
    principales objetivos conseguidos fueron:

     - El diseño de la suspensión delantera para incorporar un sistema desacoplado.

     - Un mejor entendimiento y modelado del comportamiento dinámico del vehículo.
```

<a id="diaz-p019"></a>

#### DIA · PDF página 019 de 101

```text
     - Un mejorado comportamiento dinámico del vehículo gracias a la correcta selección de
    las rigideces verticales y de balanceo.

     - Un mejor entendimiento del comportamiento cinético de la suspensión del vehículo.


6.  Referencias

    [1] FSG, Formula Student Rules 2024. Version 1.1, 2024.

    [2] W. F. Milliken and D. L. Milliken, Race Car Vehicle Dynamics, 1995.

    [3] C. Smith, Tune to Win, 1978.

    [4] Hasan-Farag, Rana Raouf. “Active neuro-fuzzy integrated vehicle dynamics controller to

   improve the vehicle handling and stability at complicated manoeuvres.”, 2016.
```

<a id="diaz-p020"></a>

#### DIA · PDF página 020 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                          INDEX OF THE PROJECT

            Index of the project


Chapter 1.  Introduction ..................................................................................................... 7

   1.1  Formula Student .................................................................................................................... 7

   1.2  The ISC Formula Student Racing Team ............................................................................. 10

   1.3  Role of the suspension on a Formula Student prototype ..................................................... 12

   1.4  Objectives ............................................................................................................................ 14

   1.5  Methodology and Resources ............................................................................................... 15

Chapter 2.  State of the art ................................................................................................ 17

   2.1  Introduction to vehicle dynamics ........................................................................................ 17

   2.2  Suspension ........................................................................................................................... 22

     2.2.1 Suspension design ........................................................................................................... 22

     2.2.2 Roll and heave control .................................................................................................... 30

     2.2.3 IFS-06 ............................................................................................................................. 32

Chapter 3.  Design ............................................................................................................ 34

   3.1  Definition of design objectives ............................................................................................ 34

     3.1.1 Team's objectives ............................................................................................................ 34

     3.1.2 Physical objectives ......................................................................................................... 36

     3.1.3 Lap time objectives ......................................................................................................... 38

   3.2  Dynamic response design .................................................................................................... 40

     3.2.1 Design considerations .................................................................................................... 40

     3.2.2 Dynamic response design ............................................................................................... 42

   3.3  Physical design .................................................................................................................... 48

     3.3.1 Physical design constraints ............................................................................................ 48

     3.3.2 Physical design of the components ................................................................................. 49

     3.3.3 Final complete design ..................................................................................................... 62

Chapter 4.  Design validation ........................................................................................... 65

     4.1.1 FEA overview ................................................................................................................. 65

     4.1.2 Design adaptation .......................................................................................................... 66

     4.1.3 Meshing .......................................................................................................................... 67

     4.1.4 Load and fixture application .......................................................................................... 70


                                                      I
```

<a id="diaz-p021"></a>

#### DIA · PDF página 021 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                          INDEX OF THE PROJECT

     4.1.5 Analysis of the results ..................................................................................................... 71

Chapter 5.  Conclusion and road map ............................................................................. 76

   5.1  Conclusion ........................................................................................................................... 76

   5.2  Path of development for the suspension .............................................................................. 77

Chapter 6.  Bibliography .................................................................................................. 80

ANNEX I. Sustainable development goals ........................................................................ 82





                                                      II
```

<a id="diaz-p022"></a>

#### DIA · PDF página 022 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               FIGURE INDEX

               Figure index



Figure 1. The team at Formula Student Spain 2023 .............................................................. 8

Figure 2. Layout for the Skidpad event ................................................................................. 9

Figure 3. Maximum point allocation for the different events in Formula Student .............. 10

Figure 4. The IFS-05 Competing at the endurance event in Formula Student Spain .......... 11

Figure 5. Generic FS track layout ........................................................................................ 13

Figure 6. Illustration of slip angle ....................................................................................... 19

Figure 7. Free body diagram describing the forces involved in load transfer ..................... 21

Figure 8. Diagram of the different rotational movements defined ...................................... 22

Figure 10. Solid axle for link suspension ............................................................................ 23

Figure 11. MacPherson Strut suspension ............................................................................ 23

Figure 12. Double Wishbone suspension of the current IFS-06 Prototype ......................... 24

Figure 13. Diagram showing the negative camber angle .................................................... 25

Figure 14. Diagram showing the toe angles ........................................................................ 26

Figure 15. Diagram defining mechanical trail and caster angle .......................................... 27

Figure 16. Diagram showing the KPI and scrub radius ....................................................... 27

Figure 17. The rear u-shape ARB of the ISC’s IFS-06 ....................................................... 29

Figure 18. The roll-heave decoupled suspension system on TU Brno’s prototype ............. 29

Figure 19. The fully decoupled hydraulic suspension of AMZ’s prototype ....................... 30

Figure 20. Close up of the ISF-06 front suspension ............................................................ 32

Figure 21. Close up of the IFS-06 rear suspension ............................................................. 32

Figure 22. CAD model of the current IFS-06 chassis, the basis for the design .................. 35

Figure 23. The IFS-06 Front suspension in the Kinematic 3D Analysis MATLAB app .... 36

Figure 24. The current spring-damper element ................................................................... 37

Figure 25. Image of the car oversteering at the Skidpad event ........................................... 38

Figure 26. Camber gain during roll of the current suspension geometry ............................ 39

Figure 27. Diameter comparison between the old and the new rear ARB .......................... 40

Figure 28. Front heave Dynamic model .............................................................................. 41

                                                      III
```

<a id="diaz-p023"></a>

#### DIA · PDF página 023 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               FIGURE INDEX

Figure 29. Front roll Dynamic model .................................................................................. 42

Figure 30. Graph simulating a 1 cm disturbance in heave .................................................. 45

Figure 31. Graph simulating a 1.26 g lateral acceleration for roll....................................... 47

Figure 32. Internal cockpit cross section template .............................................................. 48

Figure 33. Design template for the front suspension system into the cockpit ..................... 49

Figure 34. Diagram showing the compression of the heave spring  ................................... 50

Figure 35. Diagram showing the compression of the roll spring  ....................................... 50

Figure 36. Image of the CAD of the chassis with the bar to be deleted highlighted ........... 51

Figure 37. Image of the CAD of the chassis with the pushrod highlighted  ....................... 52

Figure 38. Dynamic motion ratio for the roll spring  .......................................................... 53

Figure 39. Dynamic motion ratio for the heave spring ....................................................... 53

Figure 40. Font, Side and Isometric view of the rocker for the right side of the vehicle ... 55

Figure 41. Font, Side and Isometric view of the rocker for the left side of the vehicle  ..... 56

Figure 42. Cut view of the NTN 6902LLU/5K bearing ..................................................... 57

Figure 43. Front, isometric and top views of the rocker mounting point design ................ 58

Figure 44. Diagram showing the compression of the “double spring” device .................... 59

Figure 45. Diagram showing the elongation of the “double spring” device ....................... 59

Figure 46. Image of the damper mounting CAD ................................................................. 60

Figure 47. Image of the rocker mounting CAD .................................................................. 60

Figure 48. Image of the CAD model of the general design of the spring extension ........... 61

Figure 49. Image of the CAD model of the mounting point for the damper ....................... 61

Figure 50. Image of the CAD model of the mounting point for the rocker......................... 62

Figure 51. Image of the CAD model of the final design assembled ................................... 62

Figure 52. Image of the CAD model of the front plane of the design ................................. 63

Figure 53. Image of the CAD model of the top plane of the design ................................... 63

Figure 54. Image of the CAD model of the isometric view of the design ........................... 64

Figure 55. Image of the CAD model of the left-side rocker with the changes .................... 66

Figure 56. Image of the CAD model of the right-side rocker with the changes ................. 66

Figure 57. Mesh of the left-side rocker ............................................................................... 69

Figure 58. Closeup of the mesh of the controlled region of the left-side rocker ................. 69


                                  IV
```

<a id="diaz-p024"></a>

#### DIA · PDF página 024 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               FIGURE INDEX

Figure 59. Mesh of the right-side rocker ............................................................................. 70

Figure 60. Closeup of the mesh of the controlled region of the right-side rocker ............... 70

Figure 61. Image of the von Mieses stresses of the right-side rocker ................................. 71

Figure 62. Image of the static deflection of the right-side rocker ....................................... 71

Figure 63. Image of the factor of safety of the right-side rocker ........................................ 72

Figure 64. Detail of the most stressed part of the right-side rocker .................................... 72

Figure 65. Image of the von Mieses stresses of the left-side rocker ................................... 73

Figure 66. Image of the static deflection of the left-side rocker.......................................... 73

Figure 67. Image of the factor of safety of the left-side rocker ........................................... 74

Figure 68. Detail of the most stressed part of the right-side rocker .................................... 74

Figure 69. SDG's ................................................................................................................. 82





                         V
```

<a id="diaz-p025"></a>

#### DIA · PDF página 025 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               FIGURE INDEX

                Table index




Table 1. Definition of the rocker pivot point and geometry ................................................ 54

Table 2. The parameters of the mesh of the left-side rocker ............................................... 67

Table 3. The parameters of the mesh of the right-side rocker ............................................. 67

Table 4. The parameters of the mesh of the controlled region of the left-side rocker ........ 68

Table 5. The parameters of the mesh of the controlled region of the right-side rocker ...... 68





                                 VI
```

<a id="diaz-p026"></a>

#### DIA · PDF página 026 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION


          Chapter 1. INTRODUCTION

1.1  FORMULA STUDENT


Formula Student is an international student engineering competition, which originated in the

United States under the name Formula SAE in 1981. Through the years, it has expanded

globally and risen to be the largest and most prestigious student engineering competition in

the world, attracting thousands of students and presenting them with the challenge to fully

design, manufacture, assembly and compete their prototype vehicles, according to a ruleset

defined by the competition.

The main goal of the competition is the development, construction, and competition of small,

formula style, race cars designed in accordance with the rules by university students. The

competition not only consists of the development of the car, but also everything related to

the project, which includes the team managing, the business planning, the marketing, the

logistics, the data analysis, and much more.

This competition challenges students to go through the development of a real engineering

project gaining hands on experience, presenting the students with a place to employ their

engineering knowledge and put to practice what they learn in a competitive environment.

All this work is then tested during the competitions through the static and dynamic events in

which not only the car and its performance on track is put to the test, but also the team and

their knowledge and ability to carry out the project.

Not only does the competition present the participants with a challenge, but it also presents

them with a great community and an environment which share common values and interests.

It is also an optimal environment to develop new designs and solutions for problems from

the automotive industry, achieving innovation by focusing on performance gain, budgetary

restrictions or even the restrictions presented by the rules. These factors also attract many

big automotive related businesses to Formula Student, either by directly sponsoring different


                                    7
```

<a id="diaz-p027"></a>

#### DIA · PDF página 027 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

teams and competitions or indirectly by rewarding applicants with experience in the

competition.





                                        Figure 1. The team at Formula Student Spain 2023

The different events on every competition are divided into two different types: the dynamic

events, in which, once the prototype has been deemed compliant with the rules,  its

performance is evaluated in relation to the rest of the field; and the static events, in which

the team is evaluated on the design process, the business model of the team and the cost

reporting.

The dynamic events are:

       Acceleration: It consists of a 75-meter-long track in which the car accelerates to

      complete the straight in the shortest time, testing the vehicles capacity to launch and

        accelerate, making teams focus on the power to weight ratio, the ability for the

       powertrain to respond to torque peaks and the overall longitudinal grip of the vehicle.

       Skidpad: It consists of a figure eight track consisting of two circles 15.25 meters in

       diameter and of a track width of 3 meters, the vehicle must perform two full

       consecutive laps of each circle and the time is averaged the sum of the best of each

        circle. This test challenges specially the cornering dynamics of the vehicle, with the


                                    8
```

<a id="diaz-p028"></a>

#### DIA · PDF página 028 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

       suspension both in design and setup being the most important part of the prototype

        for this event.





                                                Figure 2. Layout for the Skidpad event. [1]

       Autocross: It consists of a timed run of a coned marked track designed to test all the

       vehicles handling characteristics, as well as its ability to maximize acceleration in

       short straights. This event essentially tests the performance of every system on the

       prototype and vehicles performance as a complete package.

      Endurance: It is the last event of the competition and consist of a 22-kilometre race

      around the track at which the autocross  is run, where the durability of every

      component is put to the test. This test challenges, not only the vehicles performance,

       but also its reliability and ability to maintain a high-performance level during the

      whole event.

The static events are:

       Engineering design: It consists of a presentation of the different departments of the

       vehicle explaining the design process followed and the logic behind every decision

       taken. It aims to assess the team’s ability to follow coherent design decisions and

       explain them in an articulate manner.

                                    9
```

<a id="diaz-p029"></a>

#### DIA · PDF página 029 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

      Cost and Manufacturing:  It consists of a detailed report of the costs of the

       manufacturing of the prototype. It aims to assess the team’s ability to estimate the

        costs, as well as their knowledge of the different manufacturing techniques.

       Business plan: It consists of a simulated pitch to potential investors in which the

       judges focus on the team’s ability to communicate their vision, as well as their

        potential and viability as a project. It aims to assess the financial knowledge as well

       as the team’s entrepreneurial mindset.

During a competition the points distribution for the events are:





                         Figure 3. Maximum point allocation for the different events in Formula Student [1]

1.2  THE ISC FORMULA STUDENT RACING TEAM


The ISC Formula Student Racing Team is the electric Formula Student Team from ICAI, it

was funded in 2017 and the first prototype was produced a year later in 2018. Born as a fully

electric team, the first years were a challenge, with the design and integration of an electric

power train presenting some great difficulties, but the first running prototype was produced

overcoming most issues in the year 2021. This development continued steadily the following

                                    10
```

<a id="diaz-p030"></a>

#### DIA · PDF página 030 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

years culminating in the IFS-05, the prototype of the season 2022-2023, which was the first

prototype to compete in all the dynamic events, and even finishing the endurance, the most

gruelling event which most electric teams are not able to finish.





                         Figure 4. The IFS-05 Competing at the endurance event in Formula Student Spain.

The team is centred in offering a way for the students to be part of a real engineering project

and challenging them to be able to solve real problems and push themselves to exceed their

own expectations, all of this while pushing to achieve better results every season and passing

down the knowledge gained. Currently consisting of 61 members, the team is currently

standing 6th from the 22 registered Spanish teams in FS-Electric and 133 in the global FS-

Electric ranking.

One of the greatest hurdles that the team needs to overcome is that, because Formula Student

in Spain is relatively unknown, the ability to grow from sponsors and partners is greatly

diminished in contrast with other countries where these types of projects are more known

and supported, this leads to a lower budget as well as a higher threshold to overcome when

trying to attract interest in the project, and while Formula Student is an engineering

competition, the budget is always a constraint for the development of the project, making

“smaller” teams less competitive as a whole.


                                    11
```

<a id="diaz-p031"></a>

#### DIA · PDF página 031 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

Taking onto account these limitations, as well as the relative youth of the project, the

prospects for the team are mainly, to be able to compete with the top Spanish teams in

Formula Student Electric. This means not only with the new electric teams, but also with the

teams that are transitioning from combustion backgrounds, which come with a great

experience and backing but will be faced with the challenges of developing and integrating

an electric powertrain. Based on this, the team should be trying to maximize both the on-

track performance of the prototype, as well as the ability to defend the design, business plan

and cost and manufacturing.

Through the development of this thesis together with the ISC all these objectives can be

approached, as  it helps with the development of the vehicle’s on track performance,

designing a more versatile suspension and documenting it for the design and cost events as

well as forming team members by presenting a new design which completely changes the

current architecture, making them understand the reasons behind this change, and therefore

the reasons behind the design decisions.

1.3  ROLE OF THE  SUSPENSION ON A FORMULA STUDENT

PROTOTYPE


The suspension system on any road going vehicle ensures the correct contact the vehicle and

the road and maximizing the amount of grip available at any point and therefore maximizing

the performance the vehicle can offer at any point.

When talking about Formula Student, the importance of the suspension system is greatly

increased. This is because at the tracks, and cornering speeds at which the competitions are

held, the maximization of the available grip doesn’t depend so much on the downforce that

can be generated, but more on the optimization of the contact between the tyre and the track,

which is defined by the suspension design. All of this means that the design of a Formula

Student suspension is the most important design feature for obtaining the desired cornering

characteristics.



                                    12
```

<a id="diaz-p032"></a>

#### DIA · PDF página 032 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION





                                              Figure 5. Generic FS track layout. [2]

Other factor in which the suspension design is crucial is the longitudinal dynamics of the

vehicle, especially when considering the weight transfer during hard accelerating and

decelerating, as when the design is correctly tuned to the vehicle, the traction is optimized

and therefore the performance is maximized.

When considering Formula Student as a whole, the correct design of the suspension is also

a fundamental piece in the design event, as the suspension interfaces with all the mechanical

components of the vehicle. Therefore, a correct design of the suspension consists of not only

understanding and applying the vehicle dynamics to the prototype, but also being able to

incorporate the design in a coherent manner with the rest of the vehicle.

This integration of the suspension system into a bigger mechanical system should also be

applied to the individual components of the suspension to correctly defend the design, so all

the  interactions between every component of the suspension must be analysed and

understood to be able to correctly defend the design as every change to a design can affect

the whole system.

Finally, the understanding of the suspension in any type of vehicle, is also fundamental to

understand the changes in the set up that need to be made to improve the vehicles

performance, which on formula student is fundamental to be able to correctly prepare the car

for the different events. The understanding of the behaviour is also fundamental to design

the next prototype to mitigate the flaws observed during the competitions and testing and

correct any unwanted tendencies.

                                    13
```

<a id="diaz-p033"></a>

#### DIA · PDF página 033 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

1.4  OBJECTIVES


With this background understood, the objective of the project can be defined as the design

and integration of a new front suspension system which will be able to control independently

the vertical (longitudinal) dynamics of the front axle from the lateral dynamics, with the aim

being the improvement of the cornering dynamics of the vehicle during all the stages:

braking/corner entry, steady state cornering and accelerating/corner exit. This design should

also be easy to manufacture and assemble, as well as be easy to access to change the

parameters, finally the design should try to lower the height of the centre of mass of the

vehicle.

This main objective can be subdivided into sub-objectives such that they can be analysed

individually, as well as given priority in regards of their importance:

         - The design must comply with the ruleset defined by the Formula Student

Competition.

         - The design must be able to integrate with the current front suspension geometry

without compromising its design parameters.

         - The design must be able to fully decouple the heave from the roll movements on

the front axle.

         - The design must be able to adapt to different stiffnesses for both heave and roll to

achieve the desired dynamic objectives.

         - The design should be easy to manufacture and assembly, as well as accessible for

set up changes.

         - The design should lower the centre of mass of the vehicle by lowering or eliminating

components.





                                    14
```

<a id="diaz-p034"></a>

#### DIA · PDF página 034 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

These objectives, together with the main design objectives for the whole prototype, define

the design decisions taken during the development of the project.

1.5  METHODOLOGY AND RESOURCES


The design methodology followed during this project has followed the following steps:

1. Conceptual design: This first step consists mainly in the understanding of the concept of

a decoupled suspension and its advantages and disadvantages. During this phase, the main

objectives of the project were defined, and the concept was evaluated according to these

objectives.

2. Study and planning: During this step, the main research for the correct design for a

suspension system was done, which helped plan the resources to be used during the project

as well as the necessary calculations and analysis that needed to be performed. This phase

was fundamental in defining the scope of the project, maintaining it concise.

3. Design phase: This phase consisted of the physical design and mathematical modelling of

the new suspension system together with  its manufacturing process. This phase was

completely guided by the main objectives of the project. Making every design decision

defined by the main objectives helped streamline the design process and keep unnecessary

features out of the final design.

4. Validation: Throughout this final phase, the design was completely validated through

simulations and calculations. During this phase, the design was also finalised by optimizing

it as much as possible to try and achieve the design objectives described.

The resources used on this thesis were:

MATLAB: Generally used in the engineering filed, it is a programming language which

allows both numeric as well as symbolic computing, data manipulation and representation,

as well as many other features. For this project it has been used to for the mathematical



                                    15
```

<a id="diaz-p035"></a>

#### DIA · PDF página 035 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                INTRODUCTION

dynamic modelling of the vehicle, as well as for the optimization and analysis of the

geometry.

Excel: Mostly used as a spreadsheet program, it is a great tool for data visualization, and

organization, as well as for the automation of calculations. For this project it has been used

for the organization of the results, as well as for data visualization.

Solid Works: A general use CAD (Computer aided design) program with the ability to

perform FEA (Finite element analysis) as well as CAM (Computer aided manufacturing)

and many other functions. For this project it has been used extensively to both design all the

components, as well as perform FEM analysis to validate the structural integrity of all the

components.





                                    16
```

<a id="diaz-p036"></a>

#### DIA · PDF página 036 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART


         Chapter 2.  STATE OF THE ART

2.1  INTRODUCTION TO VEHICLE DYNAMICS


Vehicle dynamics is a field of engineering which studies the dynamic behaviour of any

vehicle, and the effects different actions have on them. For this thesis when referring to

vehicle dynamics we will be specifically referring to the field related to automobiles (with

four wheels in two axles).

To fully comprehend the importance of vehicle dynamics for the design of a racing car, first

one must understand the main goal for any racing vehicle, which is to be the fastest vehicle

on that competition at that track. To be able to have the fastest vehicle, that vehicle must be

able to maximize its acceleration on any direction the track needs and with the maximum

amount of control, and that is how performance is obtained.

Vehicle dynamics studies the principles behind the control, stability, handling, and general

motion of the vehicle. Through the understanding of this field, the design and optimization

of the vehicle can be carried out with confidence, as this field defines the maximum

acceleration  available  at any  point  in any  axis, and  therefore, through  the  correct

understanding of this field, the performance of the vehicle can be maximized.

The behaviour of any vehicle can be understood using vehicle dynamics to study all the

factors that influence the vehicle. These factors can be both driven by the design such as:

tyre choice, suspension geometry, suspension stiffness, etc. As well as be external factors

such as track temperature, air density, track smoothness, etc. By correctly designing the

factors which depend on the designer and accounting for the ones which are external, a better

performing vehicle can be designed for any application.

Regarding to automobiles, the field of vehicle dynamics can be divided into three main

factors which can be designed to affect vehicle performance, which are:


                                    17
```

<a id="diaz-p037"></a>

#### DIA · PDF página 037 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

- Tyre mechanics: The behaviour of the tyres is the most important factor to understand in

vehicle dynamics, as it is through the tyres that the vehicle can achieve the grip needed to

both control and accelerate the vehicle. The study of tyre mechanics involves understanding

how the tyre grips, as well as how the grip is affected by any factor which affect the tyre for

example: relative position with the ground, temperature, stiffness, etc.

- Kinematics: The kinematics of a vehicle define the relative movement of the wheels and

the vehicle without considering the forces involved. It defines the possible positions of the

tyres, as well as the possible compression and compression rates of the spring damper

systems. It defines the steering geometry and the different centres of movement for the

vehicle.

- Load control: Due to the accelerations, the different loads of the vehicle move their point

of application, this can affect the performance of the vehicle and its stability. This control

can be made by the driver using their controls: accelerator, brakes, steering wheel, etc. The

control can also be applied to these devices using external active systems such as anti-lock

braking systems (ABS) or traction control (TC). Finally, there is also the control of the load

transfer of the vehicle by the springs and dampers of the suspension system which can either

active or passively affect the load transferred to each tyre and control the transients to better

stabilize the car.

To understand the field of vehicle dynamics, it is fundamental to understand a vehicles

response to the different external forces it can be subjected to, as well as the basics of how

tyres grip.

To understand the basics behind the way tyre grips the road there are three basic principles

to know. The first one is that the coefficient of friction between the tyre and the road depends

on many factors, one of the most important when discussing vehicle design is how the tyre

surface contacts the road, this means that the relative position of the tyre with the ground

greatly affects the performance of the tyre. The second one is that the maximum tangential

force that a tyre can transmit to the road is directly proportional to vertical (normal) load it

transmits to the road. The final one, and the most difficult to understand, is that the

                                    18
```

<a id="diaz-p038"></a>

#### DIA · PDF página 038 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

mechanism by which the tyre can grip during cornering is not only defined by the coefficient

of friction and the normal load transferred but it is also dependant in the characteristics of

the tyre and how it deforms, this deformation is known as tyre slip. The tyre deforms in such

a way that the contact surface with the road follows the vehicles path whereas the rest of the

tyre doesn’t this difference in angle is known as slip angle. This deformation presents a

resistance which helps increase the amount of lateral acceleration the tyre can support, the

amount of resistance is also dependant on many external factors, but tyre position and load

are also fundamental factors in defining this resistance. [3],[4],[5],[6]





                                              Figure 6. Illustration of slip angle. [4]

Firstly, we will define the axes and reference planes for the vehicle. The longitudinal axis (x

axis) is defined as parallel to the floor of the vehicle and perpendicular to the rear axle. The

lateral axis (y axis) is defined as parallel to the floor of the vehicle and to the rear axle. The

vertical axis (z axis) is defined as perpendicular to the floor plane of the vehicle, generally

the origin of these axis is defined in the centre of mass of the vehicle.




                                    19
```

<a id="diaz-p039"></a>

#### DIA · PDF página 039 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

Once we have defined these axes, we can approach the concept of longitudinal and lateral

load transfer.

Load transfer is the transfer of the load carried by a tyre due to variations in the movement

of the vehicle. In this thesis we will discuss the longitudinal load transfer and the lateral.

These are mainly caused by the balancing of the forces on the tyres due to variations in its

acceleration, which in turn affect the vehicles behaviour like forces acting on its centre of

gravity.

On the longitudinal axis when accelerating, this load transfer increases the front axle load

which increases the front axle grip and the rear decreases. On the lateral axis, the load

transfer increases the load on the outer tyres, increasing the grip on that side while decreasing

it on the other. All these load transfers are greatly influenced by the vehicle’s dimensions,

centre of mass position, static weight distribution, kinematics and springs and dampers. To

increase performance to the vehicle, downforce generating elements can be added which

could also influence these load transfers, by creating more load at different speeds.

The general formula for steady state load transfer during continuous acceleration for a

perfectly rigid vehicle with no springs or dampers depends only on four factors: the total

mass of the vehicle; the continuous acceleration the vehicle is subjected to; the height of the

centre of mass and the distance between the part that increases load and the part that

decreases (for longitudinal transfer this distance is the wheelbase and for the lateral it is the

track). In general, the formula can be written as:

                                   𝑎 × ℎ𝑐
                  ∆𝑊 = 𝑚×
                                                                  𝑙


                                                                          [5]





                                    20
```

<a id="diaz-p040"></a>

#### DIA · PDF página 040 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART





             Figure 7. Free body diagram describing the forces involved in load transfer. a) longitudinal and b) lateral. [7]

These load transfers can be affected and controlled by the suspension system. This control

of the dynamic behaviour of the vehicle can be obtained through the general kinematic and

geometric design, or in a more adjustable manner incorporating  elastic and energy

dissipating elements between the axes.

When incorporating elastic components into the model, the result is the appearance of a

rotational movement which accompanies the load transfer. Depending on in which axis they

appear these movements are:

- Pitch: Rotational movement around the lateral (y) axis.  It is caused by longitudinal

acceleration (positive or negative) it causes the front and rear axles to move vertically in

opposite directions, with the one where the load is increases rising and the other lowering.

This vertical movement of the complete axle is known as heave.

- Roll: Rotational movement around the longitudinal (x) axis. It is caused by a lateral

acceleration (such as the centripetal acceleration suffered during cornering) it causes the

inner and outer tyres to move vertically in opposite directions, with the outer tyres rising and

the inner tyres lowering.





                                    21
```

<a id="diaz-p041"></a>

#### DIA · PDF página 041 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

- Yaw: Rotational movement around the vertical (z) axis. It is caused by the difference in

grip from the inner and outer tyres and causes the front and rear axles to move laterally at

different rates or directions.





                               Figure 8. Diagram of the different rotational movements defined. [9]

2.2  SUSPENSION

2.2.1 SUSPENSION DESIGN

The suspension system in a vehicle is responsible of ensuring and controlling the contact

between the vehicle and the road. Through the correct design of the suspension therefore,

the dynamics of the vehicle can be controlled, which in turn affects not only the performance

of the vehicle, but also its stability, comfortability and safety.

The design of a suspension system can be divided into three distinct parts which are greatly

interconnected. These are: the selection of the architecture, the definition of the geometry

and kinematic behaviour, and the design of the dynamic control of the vehicle.

Firstly, we will discuss the definition of the suspension architecture. Suspension architecture

refers to the different types of mechanical layouts a suspension system can have. There are

many  types  of  suspension  layouts, which  in  turn  all have some  advantages and

disadvantages. The three main systems used today are:



                                    22
```

<a id="diaz-p042"></a>

#### DIA · PDF página 042 de 101

```text
                UNIVERSIDAD PONTIFICIA COMILLAS
                  ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                         STATE OF THE ART

- The Solid Axle suspension: Consisting of a solid axle joining both wheels, making

the individual wheel positions dependent on the other wheel of the axle, and the

position the wheels independent form the position of the vehicle. This is a very robust

and cost-effective solution.





                                     Figure 10. Solid axle for link suspension. [5]

- The MacPherson Strut suspension: consisting of a lower control arm which ties

the wheel and upright to the car and the spring damper system acting directly to the

upright.  This  is  also  a  great  cost-effective  solution which  achieves wheel

independence, while not occupying much space.





                                     Figure 11. MacPherson Strut suspension. [5]



                              23
```

<a id="diaz-p043"></a>

#### DIA · PDF página 043 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

         - The double wishbone suspension: the most complicated of the suspension systems

       discussed, but also the most effective at the control of the wheel position, while also

       offering the most degrees of freedom to achieve the desired design. This architecture

         is the one chosen for the Formula Student prototype.





                                   Figure 12. Double Wishbone suspension of the current IFS-06 Prototype

The advantages of using this more complicated design are mainly: the better control of the

wheel path during suspension travel, which allows for a better control of the tyre behaviour,

resulting in potentially a better performance on track; the improved stiffness of the setup,

which in turn makes the suspension less compliant and therefore ensures the designed path

is correctly followed under load. These advantages of the double wishbone setup are widely

known, as can be observed when analysing high performing vehicles, as well as most

Formula Student teams.

The next point to discuss in suspension design is the kinematic design, also known as the

suspension geometry which is the part which defines the tyre path and position and therefore

is fundamental in defining how the vehicle grips the road. To correctly understand this point,

we must firstly define some concepts regarding the geometry.

- Camber: Defined as the angle Between the tilted wheel plane and the vertical. It is crucial

in defining the behaviour of the contact between the tyre and the road. It is defined positive,

when the top of the wheel leans away from the vehicle. Usually, racing cars have negative

camber.

                                    24
```

<a id="diaz-p044"></a>

#### DIA · PDF página 044 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART





                                   Figure 13. Diagram showing the negative camber angle. [10]

The way camber affects the tyre grip is not only by modifying the contact patch of the tyre

with the road, but also by the lateral deformation of the trail, acting similar to the slip angle,

by having the force known as camber force acting on the direction of the tilt. This force,

similar to the force produced by the slip angle increases with the angle until a certain drop

off point where  it stops. It is also important to note that as this force acts in different

directions on the inside and outside tyres, the increase in the grip of one tyre corresponds

with the decrease of the other.

Understanding this, one can now understand the importance of the camber gain defined by

the way the wheel travels through the suspension travel.

- Toe: Defined as the angle between the wheel plane and the longitudinal plane of the

vehicle. This angle is crucial in defining the handling of the car, as it directly affects the

steering input of the vehicle. It is defined as toe-in when the fronts of the tyres are leaning

more to the centre plane of the vehicle, and toe-out when the opposite occurs. Usually, cars

have toe-in in the front tyres and no toe on the rears.





                                    25
```

<a id="diaz-p045"></a>

#### DIA · PDF página 045 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART





                                         Figure 14. Diagram showing the toe angles. [11]

The way toe affects the handling behaviour of the car is increasing or decreasing the static

slip angle of the tyres. Excessive toe in any direction causes the tyre to scrub and therefore

to increase the tyre wear and decrease grip. Toe-in is used generally to increase straight line

stability, reducing responsiveness in turn in, whereas toe out generally is used to improve

turn in capabilities, helping the vehicle scrub initially the tyres less. Toe generally doesn’t

change very much with the suspension travel, but the steering input can affect the amount of

toe on the front tyres.

- Caster and trail: Caster is defined as the angle as seen from the side between the central

plane of the tyre and the axis defined by the mounting points of the upright known as the

steering or kingpin axis. It affects the size and location of the contact patch of the tyre. The

distance between the intersection of the kingpin axis with the ground plane and the centre

axis of the wheel is defined as the mechanical trail and it affects the steering input force

needed.





                                    26
```

<a id="diaz-p046"></a>

#### DIA · PDF página 046 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART





                                Figure 15. Diagram defining mechanical trail and caster angle. [12]

The caster angle generally affects the steering feel and responsiveness, as well as the self-

ailing moment of the tyre. The trail is a result of the caster angle and is a parameter that is

mostly used for the calculations of the steering force needed to achieve different turns at

different speeds.

- KPI and scrub radius: The KPI (Kingpin Inclination) is defined as the angle as seen from

the front plane of the vehicle between the kingpin axis and the vertical axis of the wheel.

The distance from where the kingpin axis intersects the ground to the intersection of the

wheels vertical central plane with the ground is known as the scrub radius.





                                    Figure 16. Diagram showing the KPI and scrub radius.[13]

                                    27
```

<a id="diaz-p047"></a>

#### DIA · PDF página 047 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

These two factors affect the way the camber changes with suspension travel and steering

input, and also the stability and grip on the tyres, and also the general feel and strength

needed for steering.

The final point to discuss in suspension design is the design of the dynamic control of the

vehicle, specifically the dynamic control of the suspension. The control of the vehicle’s

suspension can have  active or passive elements, and the  different  controls can be

interconnected or completely independent.

Suspensions with active controls have systems which are actively controlled by an external

logic programmed to make the car behave in a predetermined manner reacting to the inputs

of other sensors and data programmed. These systems are greatly complicated but can

achieve a near perfect control of the vehicle’s behaviour. The major drawback of these

systems is the complexity of the system they require to function correctly as well as the need

for precise and quick actuators which can increase the weight of the vehicle and the system.

Passive suspension systems are those in which the dynamic control of the suspension of the

vehicle is achieved using only energy accumulating and energy dissipating elements. These

types of systems are the most common as they generally are much less complex and don’t

rely on external power to function.

These passive systems are generally connected to each individual wheel, controlling the

dynamic behaviour of the wheel by adding a spring and a damper to correctly control the

time and frequency response of the tyre. This individual control is generally seen as too

simple, as the vehicle has more modes of movement than a single wheel hop.

The next step to take is to interconnect each spring system of the same axle so that they can

together also control the lateral load transfer on each axle during cornering. This is where

anti-roll bars come into play. These devices connect both sides of the suspension system on

an axle to add a factor to affect the load transfer between the sides.





                                    28
```

<a id="diaz-p048"></a>

#### DIA · PDF página 048 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART





                                      Figure 17. The rear u-shape ARB of the ISC’s IFS-06

The next step would be to completely decouple the vertical movement of an axle and the

lateral load transfer, which makes the system able to independently control the behaviour of

the vehicle during two very different modes, this heave-roll suspension is generally the norm

in high-ranking Formula Student teams.





                        Figure 18. The roll-heave decoupled suspension system on TU Brno’s prototype. [14]




                                    29
```

<a id="diaz-p049"></a>

#### DIA · PDF página 049 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

The final step in a passive suspension to achieve the best dynamic control would be to

interconnect the axis and the wheels in a diagonal and then all the modes of movement would

be independently controlled. This fully decoupled passive suspension has been achieved in

Formula Student only by one team in the world.





                   Figure 19. The fully decoupled hydraulic suspension of AMZ’s Formula student prototype. [15]

2.2.2 ROLL AND HEAVE CONTROL

Now that the basics of vehicle dynamics and suspension design and the operating principles

of a suspension system have been explained, we can understand the problems presented by

both the heave and pitch movements. Firstly, we will discuss the problems with the vertical

movement of the axle and the possibilities to better control  it during different types of

movement and then, the rolling motion’s problems.

Heave, defined as the vertical movement of the axle, needs to be considered when designing

a suspension system as it greatly affects the wheel positions for the axles during longitudinal

acceleration. Other problems that excessive pitch can cause are bottoming out diffusers or

front wings, or overloading the front tyres under braking and locking up.

This movement can occur in one of two mechanisms, the first one is a perturbance in the

road affecting equally both tyres at the same time, and the second one is due to the

acceleration in the longitudinal axis. For the first example, the spring damper system of the


                                    30
```

<a id="diaz-p050"></a>

#### DIA · PDF página 050 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

vehicle’s suspension should be able to quickly react and absorb the perturbance if tuned

correctly. For the second one, the control of the vertical motion can be controlled by many

mechanisms:

         - Anti-dive/Anti-squat geometry: “Anti-pitch” suspension geometry design is a

     way to design the suspension mounting points to move the pitching centre to reduce

       the pitching movement on the vehicle, therefore reducing the total movement of the

        axles. The problem with having too much anti pith geometry is the loss of control of

       the longitudinal weight transfer during longitudinal acceleration, as well as the high

        stiffness of the vehicle during acceleration and deceleration.

         - Multi-rate springs: When using multi-rate or progressive springs on a vehicle, one

      can affect the characteristics of how it reacts to different inputs. By using multi rate

       springs, one can have a higher stiffness to take care of the more violent vertical

      movements due to road perturbances while maintaining a different stiffness for the

       control of the less aggressive heave during pitch.

         - Mode decoupling: When using a mode decoupled suspension one can design the

       control of the different movement modes differently by selecting different spring

        rates and dampening coefficients.

Roll is defined as the rotation of the vehicle in the longitudinal axis. The control of the roll

of the vehicle is a fundamental part of the design of a suspension system as roll is the main

responsible for wheel movement during steady state cornering. This means that in order to

obtain the maximum cornering performance of a vehicle, its roll control must be optimized

for cornering. Other problems with excessive roll are the destabilization of the vehicle or

modification to the aerodynamic platform.

Roll control is traditionally done by the design of anti-roll bars which in turn define the roll

stiffness of the vehicle, as well as control the lateral load transfer during cornering. This

design is mainly good but has two main drawbacks that are the influence of individual corner

stiffnesses and the inability to independently dampen the roll response. To optimize this


                                    31
```

<a id="diaz-p051"></a>

#### DIA · PDF página 051 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

design one can, try to decouple the individual corner stiffnesses from roll stiffness as well as

incorporate a dampening element into that decoupled system.

2.2.3 IFS-06

The IFS-06, the sixth prototype developed by the ISC Formula Student team. Designed for

the 2023-2024 season, it is the basis for the design of this project. The prototype is a single

motor electric prototype with a steel tube space frame chassis.

The cars suspension system has a double wishbone pushrod setup in both axles with a z-type

anti roll bar in the front axle and a u-type in the rear axle.





                                        Figure 20. Close up of the ISF-06 front suspension





                                        Figure 21. Close up of the IFS-06 rear suspension


                                    32
```

<a id="diaz-p052"></a>

#### DIA · PDF página 052 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                               STATE OF THE ART

Its design metrics were:

- Better camber control, with a static camber of 2.5 and 2 degrees for the front and rear axles.

- Integration of an anti-dive of 30% and an anti-squat of 20%.

- Reduction of the wheelbase and track to 1570 and 1200 mm respectively.

- Optimization of the Motion ratio to 1.2 in both front and back.

- Minimization of roll centre migration during cornering.

- Better control of steering forces.

- Change to Hoosier R20 Tyres.

- Turning circle reduction to 3.9m.

- Incorporation of a 100% Ackerman steering.

- Better roll control on the rear axle by reducing roll stiffness and roll gradient.

This design will serve as the basis for the new decoupled setup designed in this thesis.





                                    33
```

<a id="diaz-p053"></a>

#### DIA · PDF página 053 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN


             Chapter 3.  DESIGN

3.1  DEFINITION OF DESIGN OBJECTIVES


The main objective of the project is, as stated in the introduction: The design and integration

of a new front suspension system which will be able to control independently the vertical

(longitudinal) dynamics of the front axle from the lateral dynamics, with the aim being the

improvement of the cornering dynamics of the vehicle during all the stages: braking/corner

entry, steady state cornering and accelerating/corner exit. This design should also be easy to

manufacture and assemble, as well as be easy to access to change the parameters, finally the

design should try to lower the height of the centre of mass of the vehicle.

This main objective of the project can be projected into the specific design considering

various points of view of what the expected outcome is. This can be divided into three

different and distinct types of objectives: The team’s objectives, the physical objectives and

finally the main performance objectives.

3.1.1 TEAM'S OBJECTIVES

To begin describing the objectives it is fundamental to understand the team’s objectives, as

this project is done with the ISC Formula Student racing team. This means that the team’s

objectives should be considered in every design decision to act as a method of validation

these decisions.

The first and most important objective to have in mind is that this project is part of the design

phase for the IFS-07, next year’s prototype, which has not been designed yet. This means

that even though the physical design has been done based on the chassis of the IFS-06, it

should be able to adapt to next year’s design and therefore, the most important features

should be parametrically designed, and easily changed to accommodate possible changes to

the design of both the chassis and the suspension geometry.



                                    34
```

<a id="diaz-p054"></a>

#### DIA · PDF página 054 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                            Figure 22. CAD model of the current IFS-06 chassis, the basis for the design

The other general team objectives are more directed to the prototype and the team as a whole.

These general objectives should be taken into account when designing but should not be the

main drivers of the design. The objectives can be divided into the season objectives, which

are to improve the total team score on the competitions which were visited last year. This

can be achieved by a better understanding of how we can obtain better scores on the different

events and approach the competitions more as the most important parts of the season and not

the way to end the season, as well as obtaining a better performance in both dynamic and

static events. The other part of the team’s objectives are the prototype’s objectives, which

have been set to mainly improve the understanding, verification and design validation of the

vehicle by both more testing and a more thorough understanding of our design philosophy

and manufacturing methods. The other main objective for the prototype is trying to reduce

the centre of mass height, together with the total mass of the vehicle and can be approached

by all departments by trying to simplify components and systems.

Based on these objectives, the conclusions we can obtain for the design are that it should be

easily modified to be adapted to the possible different chassis and suspension geometry, it




                                    35
```

<a id="diaz-p055"></a>

#### DIA · PDF página 055 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

should try to lower its centre of mass, and  it should not only allow the team to better

understand the car but also it should allow the car to achieve a better on track performance.

3.1.2 PHYSICAL OBJECTIVES

The physical objectives of the design are mainly based on three things, the kinematics of the

suspension, the dynamics of the suspension, and finally the physical constraints defined by

both the existing design and the ruleset defined by the competition.

Firstly, we will discuss the kinematic objectives of the new design. This new design should

be able to incorporate into any suspension geometry without altering the desired kinematic

characteristics, this means that the suspension with and without the roll heave system should

respond kinematically in an identical manner. To correctly validate this, the geometries

should be analysed in kinematic design programs and the camber gain, toe increase and

general wheel position during all of its travel should be identical independently of the design

utilized. Also, regarding kinematics, the objective for the motion ratio will be set to 1.2 to

better utilize the spring-damper system. The last point regarding kinematic design is the

importance of maintaining a common plane of movement for both sides of the suspension,

which will define the plane where the roll-heave system will be mounted. The failure to

obtain the same plane will result in the appearance of unwanted forces which would affect

the design.





                        Figure 23. The IFS-06 Front suspension in the Kinematic 3D Analysis MATLAB app

                                    36
```

<a id="diaz-p056"></a>

#### DIA · PDF página 056 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

The dynamic objectives of the design are: Firstly, and most importantly to be able to produce

a design which can effectively decouple the heave form the roll to better control each

movement individually. Once this general design is achieved, the mathematical modelling

of the system as an individual axle to study the effects of the different rates on both heave

and roll should be obtained. Through this model the real roll and heave stiffnesses of the

vehicle should be parametrically obtained. Based on these objectives, the testing campaign

for the vehicle should be carried out and the different responses and models verified. Some

general objectives for the system are natural heave frequencies higher than 2 Hz to achieve

a better handling car and a roll gradient around 1 deg./g. [3],[4],[5]

Finally, we will discuss the physical objectives, these can be divided into two groups:

The objective to reutilize the most possible amount of pre-existing hardware for both

assembly and design. This means to be able to design the system in such a way that the way

it is assembled is similar to the rest of the systems in the vehicle utilizing mainly M6 or M5

shoulder screws with nylon locking nuts. And designing the systems in such a way that

supports and other machined or bought hardware can be reused. This facilitates and cheapens

the switch to the new system, as the hardware needed doesn’t change. The other main

component which can be brought from the previous design is the spring-damper element,

this would also reduce the costs of the design as well as maintain the mounting hardware

common to both designs.





                                          Figure 24. The current spring-damper element

                                    37
```

<a id="diaz-p057"></a>

#### DIA · PDF página 057 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

The objective to fit in the design envelope defined by both the ruleset constraints and the

physical constraints too. This design envelope will be further defined when describing the

physical design phase. The design also should be defined with the ease to assemble, and

modify parameters in mind, so when working on the car it is not an inconvenience to alter

suspension parameters. This design objective is fundamental, as easy to change parameters

are fundamental for the team to correctly understand and validate the design by modifying

the behaviour and correlating it to the model.

3.1.3 LAP TIME OBJECTIVES

The lap time objectives have been mainly taken into account for the main dynamic response

design of the vehicle. The lap times analysed were last year’s Formula Student Spain

Skidpad times, which before penalties the best time was 5.6s. Analysing the lap, high levels

of oversteer produced by a very stiff rear ARB were observed, based on this analysis, the lap

time objective of 5.4s was defined. This time in the skid pad event can be directly corelated

to the lateral acceleration the vehicle can withstand, for the objective time this acceleration

is equal to 1.26 g of lateral acceleration.





                     Figure 25. Image of the car oversteering at last year’s Formula Student Spain Skidpad event

Setting this acceleration as the objective, the dynamic behaviour of the car can be designed to

achieve its maximum performance at that lateral load.



                                    38
```

<a id="diaz-p058"></a>

#### DIA · PDF página 058 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

Skidpad time was the selected lap time as it is the event where the most important factor for

vehicle performance is the lateral dynamics of the vehicle. This specific Skidpad time also

showed the limits of the performance of the vehicle, as during the following attempts, the

prototype spun out. Therefore, this objective time tries to address those problems by

optimizing the lateral dynamics if the vehicle for those accelerations. This means that the

objective for the design of the vehicles is to achieve the optimum camber at the objective

acceleration to be able to maximize the tyre grip. This camber objective is set to around 3

degrees, understanding this amount of camber to be the optimum for the tyre from studies

conducted last year. Considering the static camber of the wheels and the camber gain of the

front axle the roll angle defined is around 1.2 degrees to obtain the desired camber, taking

our objective acceleration we can define a front axle roll stiffness of around 1 degree per g.





                              Figure 26. Camber gain during roll of the current suspension geometry

Another notable point to understand behind this lap time objective is that the suspension

system has already been modified by introducing a softer rear anti roll bar, so the lap time

might have already been lowered, but it is currently untested. The objective therefore should

also be met by a parametric design of the dynamic response of the vehicle.





                                    39
```

<a id="diaz-p059"></a>

#### DIA · PDF página 059 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                       Figure 27. Diameter comparison between the old (upper) and the new (lower) rear ARB

3.2  DYNAMIC RESPONSE DESIGN

3.2.1 DESIGN CONSIDERATIONS

To define the dynamic response of the vehicle, first we must define the dynamic system

equivalent to the model. This means that the total vehicle behaviour will be represented by

the relative movement of its tyres and how they respond to different perturbances.

To be able to simplify the diagrams, the system will be modelled as a combination of two

models, one for each mode of movement which work independently.

Firstly, we will discuss the heave system. In heave, both wheels simultaneously move, and

this movement  is supported by the compression of the heave spring-damper system.

Therefore, the modelling of this movement can be simplified to a two degrees of freedom

model where the sprung mass of the front axis is supported by the heave spring damper

system which connects it with the unsprung mass which in turn is connected directly to the

road through the wheels which are modelled as a spring with its total rate being the parallel

of an individual vertical wheel rate.

Through this model, we can not only study the general behaviour of the front axle of the

vehicle while on heave but also the response of the wheel hop effect, a resulting factor of the

unsprung mass of the vehicle having a different response to the main sprung mass.





                                    40
```

<a id="diaz-p060"></a>

#### DIA · PDF página 060 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

Through the simplification of this model, the complete heave behaviour of the front axle can

be studied in the same manner to the quarter car model that is greatly studied.





                                             Figure 28. Front heave Dynamic model

For the roll movement on the front axle, the system has been modelled differently, as the

focus of the roll control in not understanding and controlling perturbances or very sudden

violent loads, this axle can be modelled as a torsional spring equivalent to the torsional

stiffness of the vehicle, and the damper can be modelled as an equivalent damper to the one

design. The mathematical formulation for these equivalences will be further described.




                                    41
```

<a id="diaz-p061"></a>

#### DIA · PDF página 061 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

This approach as a system with a single degree of freedom for the roll movement helps better

understand how the different spring rate used in the roll spring-damper system directly

affects the roll stiffness of the axle. Other important property of this model to understand is

that it describes roll as the difference in position of both wheel centres of the same axle.





                                              Figure 29. Front roll Dynamic model

3.2.2 DYNAMIC RESPONSE DESIGN

The dynamic response was designed and optimized using MATLAB. All the scripts are

based on firstly obtaining the correct stiffnesses and damping factors on the wheels (utilizing

the different installation ratios to obtain the correct “real distance factors”), then by

simulating an applied perturbance either solving the differential equation system for the

system with multiple degrees of freedom or the simple equation and obtaining the graphical

response and analysing it.

The heave model has four main parameters which are: front sprung mass, front unsprung

mass, front heave stiffness, front tyre stiffness. Utilizing these parameters to calculate the

dynamic response of the vehicle to a movement. The code used was this:

                                    42
```

<a id="diaz-p062"></a>

#### DIA · PDF página 062 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

clear all
clc


% Variables
m = 330; % [kg] total mass
mfu = 15; % [kg] unsprung front mass
mf = 100-56; % [%] total front mass percentage
ir_h = 1.2; % instalation ratio for the front  heave spring
kw = 110000*2; % [N/m] tyre stiffness
kfhs = 43780*2; % [N/m] heave spring stiffness
cfhd = 1000*2; % [N*s/m] heave damper damping coeficient




kfhw = kfhs*ir_h^2; % damping coeficient in tyre


cfhw = cfhd*ir_h^2; % damping in the tyre


mfs = m*mf/100-mfu; % frunt sprung mass


% Mass matrix
M = [mfu , 0;
    0 , mfs];
% Stiffness matrix
K = [kw+kfhw,-kfhw;
    -kfhw,kfhw];
% Damping matrix
C = [cfhw,-cfhw;
    -cfhw,cfhw];


% definition of oscilation modes and frequencies
[Phi,omega2] = eig(K,M);


ti = 0:pi/1000:2;
ti_2 = 0:pi/10000:pi/10;
pr_1 = Phi(1,1)*sin(ti*sqrt(omega2(1,1))+3*pi/2);
pc_1 = Phi(2,1)*sin(ti*sqrt(omega2(1,1))+3*pi/2);
pr_2 = Phi(1,2)*sin(ti_2*sqrt(omega2(2,2))+3*pi/2);
pc_2 = Phi(2,2)*sin(ti_2*sqrt(omega2(2,2))+3*pi/2);


figure
plot(ti,pr_1,'r'),hold on
plot(ti,pc_1,'g'),hold on
plot(ti,(pr_1+pc_1),'b')


figure
plot(ti_2,pr_2,'r'),hold on
plot(ti_2,pc_2,'g'),hold on
plot(ti_2,(pr_2+pc_2),'b')


% mode equivalent calculations
m1 =transpose(Phi(:,1))*M*Phi(:,1);
m2 =transpose(Phi(:,2))*M*Phi(:,2);

                                    43
```

<a id="diaz-p063"></a>

#### DIA · PDF página 063 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

k1 =transpose(Phi(:,1))*K*Phi(:,1);
k2 =transpose(Phi(:,2))*K*Phi(:,2);
c1 =transpose(Phi(:,1))*C*Phi(:,1);
c2 =transpose(Phi(:,2))*C*Phi(:,2);


%modal contributions
rang_t = [0. 2];
d0 = [0.0101325 0.];
solu_1 = ode45(@(t,y)SDoF(t,y,m1,k1,c1),rang_t,d0);
v_t = (0:0.01:2);
y_1 = deval(solu_1,v_t);
figure
plot(v_t,y_1(1,:)), hold on


d0_2 = [0.1 0.];
solu_2 = ode45(@(t,y)SDoF(t,y,m2,k2,c2),rang_t,d0_2);
y_2 = deval(solu_2,v_t);
v_t_2 = (0:pi/2000:pi/10);
figure
plot(v_t_2,y_2(1,:)), hold on


y_fin = y_1+y_2;
figure
plot(v_t,y_fin(1,:))


figure
plot(v_t,y_1(1,:)), hold on
plot(ti,(pr_1+pc_1))


frechz=(sqrt(omega2))/(2*pi)


function der = SDoF(t,y,m,k,c)
der(1,1) = y(2);
der(2,1) = (1/m)*( - c*y(2)-k*y(1));
end
And the resulting parameters obtained this simulation graph.





                                    44
```

<a id="diaz-p064"></a>

#### DIA · PDF página 064 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                                     Figure 30. Graph simulating a 1 cm disturbance in heave

In this graph we see a simulated heave movement of 0.01m which is completely controlled

in under 1.5 seconds with the second peak having a reduction of in amplitude of over 50%.

This control was designed with the dampening of the dampers set to the medium position

(1000 Ns/ m) and the stiffness of the spring being set to 43780 N/m. With these parameters

the resulting frequencies for the movements were 3.91 Hz for the vertical movement and

24.36 Hz for the wheel hop, these frequencies are well within the desired range, and, as seen

by the graph help the system act fast enough. This design is also coherent with the whole car

as the rear axis has a higher main frequency of 4.2 Hz which is desired to better settle the

whole car.

This code, as previously describes firstly obtains the wheel rates of the suspension and then

by assembling the different mass, stiffness and damping matrixes obtains the dynamic

response of the system to the simulated perturbances.



                                    45
```

<a id="diaz-p065"></a>

#### DIA · PDF página 065 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

Regarding the roll control, Firstly, the equivalent front roll stiffness based on the objective

of the front roll gradient of 1 degree per g was calculated using this formula


𝜙   𝑊𝐹×𝐻          𝜙
𝐴𝑌=  𝐾𝜙𝐹  [3] where 𝐴𝑌 refers to the front roll gradient, 𝐻 to the front weight and 𝑊𝐹 to the

total front stiffness in the wheels, obtaining a front stiffness in the wheels of 8854.8 rad/N

which corresponds to a total front stiffness for the roll spring of about 10000N/m

The simulation for the validation of the roll control was done comparing the behaviour of

the front axle with different roll gradients while subjected to an acceleration of 1.26g with a

damping factor of 0.7. The amount of total roll of the body was the parameter observed,

which we wanted to be around 1.2 degrees to obtain the desired camber gain.

The code used was this:


close all
clear all
clc


% Definition of variables
rpg_old = 1/0.6; %roll gradient old
rpg_new = 1/1; %roll gradient new
seta = 0.7; % damping
m = 280; % mass
a =1.26; % acceleration
hcg = 0.3; % com height
f_eq = a;


k1 = rpg_old;
k2 =rpg_new;


t0 = 0.;
tf = 200;
rang_t = [t0 tf];
d0 = [0. 0.];




aux_t = (t0:0.001:tf)';
aux_f = ones(size(aux_t,1),1)*f_eq;




solu = ode45(@(t,y)SDoF(t,y,m,k1,seta,aux_t,aux_f),rang_t,d0);



                                    46
```

<a id="diaz-p066"></a>

#### DIA · PDF página 066 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

solu2 = ode45(@(t,y)SDoF(t,y,m,k2,seta,aux_t,aux_f),rang_t,d0);


v_t = (t0:0.01:tf);
y = deval(solu,v_t);
y2 = deval(solu2,v_t);


figure
plot(v_t/9.81,y(1,:)), hold on
plot(v_t/9.81,y2(1,:)), hold on
ylabel('degrees of roll angle gained')
xlabel('time under acceleration = 1.26g')
legend('old ARB','new ARB')



function der = SDoF(t,y,m,k,seta,aux_t,aux_f); omega_n = sqrt(k/m);
c_crit = 2*omega_n*m;
c = seta*c_crit;
f = interp1(aux_t,aux_f,t);


der(1,1) = y(2);
der(2,1) = (1/m)*(f - c*y(2)-k*y(1));
end
And the parameters observed were these:





                                  Figure 31. Graph simulating a 1.26 g lateral acceleration for roll

                                    47
```

<a id="diaz-p067"></a>

#### DIA · PDF página 067 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

This resulting graph shows the difference in behaviour between the previous setup with a

gradient of 0.6 degrees per g and the current one with a gradient of 1 degree per g. In the

graph one can how even though the new more flexible system is slower, it better conforms

to achieving the desired objective of 1.2 degrees of roll for the 1.26 g turn. This in turn

means that the correct choice of the stiffness of the roll spring is around the 10000 N/m

value.

3.3  PHYSICAL DESIGN

3.3.1 PHYSICAL DESIGN CONSTRAINTS

The constraints due to ruleset, the ruleset defines areas where no parts are allowed to go. For

the design of a suspension system on the front axle the most crucial area is the inside of the

cockpit. The ruleset states that the cockpit must have a free internal cross section defined by

a template in which no components must intrude. This rule constraints the possible design

for a suspension to fit through the cockpit to a smaller design area.





                                        Figure 32. Internal cockpit cross section template

The constraints due to the base design, the chassis in which the design is being based has

been taken as a hard point allowing only changes to the chassis structures which were

designed to house pre-existing suspension components. This coupled with the decision to

design the system based on the previous geometry defines the possible spaces where a


                                    48
```

<a id="diaz-p068"></a>

#### DIA · PDF página 068 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

system like this can be housed. Through looking at the possible designs, a final design

template was obtained which considered both the ruleset and previous design and defined

the zone inside the cockpit where the system could be housed.





                            Figure 33. Design template for the front suspension system into the cockpit

For the final design constraints, the current materials as well as the fabrication methods we

have available have been taken into account. The first is that we already have great spring-

damper elements which can be incorporated into the design, secondly is that the fabrication

technologies we have available are reduced, as we don’t have access to more modern

methods cush as additive manufacturing for metal. Multi axis CNC machining is the most

versatile method we have access to, but  it is  still needed to be sourced to an outside

machining shop, if we wanted to do the fabrication in house, it would need to be either water

cut or very simple machining.

3.3.2 PHYSICAL DESIGN OF THE COMPONENTS

To begin defining the physical design, an overview of the general design will be made. The

general design for a roll-heave decoupled front suspension consist of three main parts: The

rockers, the heave spring and the roll spring.

The general overview of the design consists in the separation of the roll spring and the heave

spring so that they are actuated only during their respective movements. For roll that is when

both tyres move opposite to each other and for heave when both tyres  rise or  fall


                                    49
```

<a id="diaz-p069"></a>

#### DIA · PDF página 069 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

simultaneously. To decouple the roll from the heave, a system can be designed that

compresses the heave spring only when simultaneous movement of both tyres occurs. Such

a design can be achieved by connecting the spring the top sides of both of the rockers

together so when the tyres rise from both sides simultaneously the spring is compressed. To

decouple the roll control, a similar design can be implemented. The top side of one rocker

will be connected to the bottom side of the other rocker by a spring so when the tyres move

in opposition, the spring is actuated upon.

The following diagrams illustrate how the general design works.





 Figure 34. Diagram showing the compression of the heave spring and no reaction of the roll spring when both tyres move equally. [16]





 Figure 35. Diagram showing the compression of the roll spring and no reaction of the heave spring when both tyres move opposite. [16]





                                    50
```

<a id="diaz-p070"></a>

#### DIA · PDF página 070 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

As it can be seen this system can effectively control independently roll and heave. The

drawbacks of the system are in general two: First the need for a spring that works on both

compression and elongation for roll control and the second one is the inability to control

individually wheel perturbation.

For the first drawback, a “double spring” system can be designed so that the spring can act

in compression on both sides of roll. This can be achieved by enclosing the spring-damper

element in a device which can compresses the element independently of whether its sides

move toward or apart from each other.

For the second drawback, the individual perturbation cannot be independently controlled due

to the interconnection between both sides of the vehicle. This drawback is also common in

most suspensions and could only be addressed either by completely disconnecting the tyres

form each other, which would result in the loss of roll and heave control and therefore is

much worse for the control of the vehicle; of by the designing of a fully decoupled

suspension which involves the fabrication of a hydraulic system to be able to fully

independently control each tyre as well as each movement.

The final design for the roll-heave system was designed to be able to fit inside the design

envelope inside the cockpit due to this design reducing the maximum heigh of the centre of

mass, one of the main objectives of the prototype. In order to fit inside the envelope, the

design needed to fit the pushrods through the chassis, as well as eliminate the bar currently

in place to hold the front ARB and spring-damper elements.





                          Figure 36. Image of the CAD of the chassis with the bar to be deleted highlighted

                                    51
```

<a id="diaz-p071"></a>

#### DIA · PDF página 071 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                       Figure 37. Image of the CAD of the chassis with the position of the pushrod highlighted

3.3.2.1 Rockers

The rocker or bellcrank is the component designed to transmit the wheels motion into the

compression or elongation of the spring. It consists of a pivot point, and three arms. One arm

where the pushrod actuates, one for the actuation of the heave spring and one for the

actuation of the roll spring.

For the correct design of the rockers, an analysis of the motion ratio is needed to determine

the correct position of the pivot point, as well as the relative lengths of the different arms.

The desired Motion ration for heave was of 1.2 and for roll was of 0.8 to obtain the desired

stiffnesses on the wheels. In order to obtain these parameters a study was made on the

possible position of the rocker and the relative length of the arms to optimize during all of

the travel the motion ratios. This analysis was performed in an app designed in MATLAB

which calculates the dynamic motion ratio of the suspension. The results were these two

graphs showing the motion ratio for the suspension though all of its travel. The first one

being the roll spring and the second being the heave spring.





                                    52
```

<a id="diaz-p072"></a>

#### DIA · PDF página 072 de 101

```text
   UNIVERSIDAD PONTIFICIA COMILLAS
  ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
 GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                              DESIGN





 Figure 38. Dynamic motion ratio for the roll spring





Figure 39. Dynamic motion ratio for the heave spring




             53
```

<a id="diaz-p073"></a>

#### DIA · PDF página 073 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

As it can be seen in both tables the motion ratio objective is achieved with an error of less

than 0.1 in the roll ratio, with the heave ratio being practically continuous throughout the

whole travel.

Form this optimization of the motion ratios, the position of the points which define the rocker

were obtained.


                                         x         y          z
                     Pivot point position     906.6     161.0     488.6
                 Heave arm length                 81.1 mm
                    Roll arm length                 50 mm
                  Pushrod arm length               65.5 mm
                  Pushrod-Heave angle              71.21°
                    Pushrod-Roll angle                 37.6°
                                  Table 1. Definition of the rocker pivot point and geometry

It is important to note that the points of the rocker pivot point are obtained in reference to

the rest of the geometry of the vehicle as defined in the CAD model, and the Pushrod-Roll

angle refers to the side where the roll arm is in the same quadrant of the rocker as the pushrod

arm.

Once the geometric definition of the rockers explained, the physical design can be

approached. The design was done in SolidWorks and the total length of the rocker was

determined with the finalised design in order to maintain both springs separated.

The design consists of two rockers to correctly actuate the heave and roll springs.





                                    54
```

<a id="diaz-p074"></a>

#### DIA · PDF página 074 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                        Figure 40. Font, Side and Isometric view of the rocker for the right side of the vehicle

The design of the right rocker, was mainly straightforward, taking knowledge form the

experience gained in the previous prototypes and designing the arms of 4 mm thick

aluminium. And the sections with the reduced diameter have been designed to house the

bearings for the correct functioning of the system.





                                    55
```

<a id="diaz-p075"></a>

#### DIA · PDF página 075 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                        Figure 41. Font, Side and Isometric view of the rocker for the left side of the vehicle

The design of the left rocker not only took the same main cues from the right side, but it also

had to be adapted to fit the rear mounting point of the roll damper. This was achieved by

incorporating a chamfered section.

The material of the rockers will be 7075 aluminium, as it is an alloy which, not only has

great mechanical properties but also a reduces density and  is easy to machine. The

fabrication method for these main components will be muti axis CNC machining, due to the

complicated structures of the arms.

                                    56
```

<a id="diaz-p076"></a>

#### DIA · PDF página 076 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

3.3.2.2 Rocker mounting points.

Once the rockers were designed, the mounting brackets for them were the main focus. Taking

into account the need for a bearing, one was selected using the web applet designed by SKF

bearing selector and looking of a bearing that fitted the parameters of both size and function,

with the function being a rotating shaft with a negligible axial load. Once the SKF bearing

was obtained, the NTN equivalent was searched, as NTN is one of the sponsors of the ISC

Formula Student racing team. The final Bearing selected was the NTN 6902LLU/5K

bearing, a single file ball bearing with radial contact and a steel cage with seals on both sides.





                                      Figure 42. Cut view of the NTN 6902LLU/5K bearing

Once the bearing selection was completed, the designed for the Rocker mounting points was

finalized. Selecting shouldered M5 bolts for mounting with the design incorporating space

for the rounded heads.





                                    57
```

<a id="diaz-p077"></a>

#### DIA · PDF página 077 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                           Figure 43. Front, isometric and top views of the rocker mounting point design

Consisting of a bearing housing, and two holes to accommodate screws to mount to the

mounting points of the chassis, the design was completely simplified. To achieve a geometry

which could be obtained by simple manual machining.

3.3.2.3 "Double spring" mechanism

The next step for the design of the new suspension was to design a “double spring” device

which can compresses the element independently of whether its sides move toward or apart

from each other.

For this design, two main parts were designed. Firstly, the mounting of the damper and

secondly, the mounting points for the rocker. The design consists of the compression of the

spring between the two mounting points of the damper by attaching one sides rocker


                                    58
```

<a id="diaz-p078"></a>

#### DIA · PDF página 078 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

mounting to the others damper mounting so that the rod connecting the two mounting points

is only locked at the ends, allowing the mounting points to move towards each other but not

away from each other, as the connections are made in a diagonal, both sides can compress

when the rocker compresses the device and when the rocker movement elongates the device,

the connecting rods compress the damper mounting points together. The mechanism is

visually explained by the following diagrams.





            Figure 44. Diagram showing the compression of the “double spring” device and the compression of the spring





             Figure 45. Diagram showing the elongation of the “double spring” device and the compression of the spring

The individual parts of the system were designed to be machined in 7075 aluminium. The

rocker mountings were designed to be machined by a multi axis CNC machine to be able to

achieve the complicated geometries, and the damper mounting points were designed in a

simpler manner to be able to be hand machined, even though a further version could be

designed, focused of reducing the weight.


                                    59
```

<a id="diaz-p079"></a>

#### DIA · PDF página 079 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                                         Figure 46. Image of the damper mounting CAD





                                          Figure 47. Image of the rocker mounting CAD

3.3.2.4 Heave spring extension

By reusing the spring-damper elements, the static length of the element must be altered to

obtain the desired design. For this reason, a new mounting point for the top of the element

must be designed to achieve this static length. As this length depends mainly on the overall


                                    60
```

<a id="diaz-p080"></a>

#### DIA · PDF página 080 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN

geometric design, a way to alter the length of the system was implemented by the addition

of shims between the elements to increase the static length.





     Figure 48. Image of the CAD model of the general design of the heave spring extension mounted on the spring damper system





                             Figure 49. Image of the CAD model of the mounting point for the damper




                                    61
```

<a id="diaz-p081"></a>

#### DIA · PDF página 081 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                    DESIGN





                             Figure 50. Image of the CAD model of the mounting point for the rocker

3.3.3 FINAL COMPLETE DESIGN





                                 Figure 51. Image of the CAD model of the final design assembled

                                    62
```

<a id="diaz-p082"></a>

#### DIA · PDF página 082 de 101

```text
           UNIVERSIDAD PONTIFICIA COMILLAS
            ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
           GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                        DESIGN





Figure 52. Image of the CAD model of the front plane of the design mounted into the chassis





 Figure 53. Image of the CAD model of the top plane of the design mounted into the chassis


                        63
```

<a id="diaz-p083"></a>

#### DIA · PDF página 083 de 101

```text
            UNIVERSIDAD PONTIFICIA COMILLAS
             ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
            GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                         DESIGN





Figure 54. Image of the CAD model of the isometric view of the design mounted into the chassis





                         64
```

<a id="diaz-p084"></a>

#### DIA · PDF página 084 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION


        Chapter 4.  DESIGN VALIDATION


4.1.1 FEA OVERVIEW

Finite element analysis (FEA) is a design tool used in engineering for the simulation of large

complex elements that works by subdividing the main elements into smaller elements

defined as a mesh. For the validation of the design of the rockers, Mechanical FEA has been

used, which simulates fixtures and forces applied to elements designed by CAD. This

simulation was carried out  in SolidWorks, which includes a CAE (computer aided

engineering) toolbox.

The quality of the simulations obtained by FEA can vary greatly, that is why the input of the

simulation must be of the highest quality possible.

In order to carry out a simulation, the first element needed is the final design of the piece to

be studied. This final design should try to avoid hard edges where possible, incorporating

chamfers  to gradually change the geometry, as FEA software often have problems

calculating elements on hard edges. For the final preparation step for the designed part, the

material properties should be selected.

Once the geometry and material are correctly defined, the Object will be meshed and

separated into small elements. The size and shape of these elements is crucial for the final

result of the analysis, as well as the time and resources it takes to run. In general, the higher

number of elements leads to a better result, as well as a higher simulation time, this means

that the elements should be smaller to be able to more accurately predict the behaviour of

the part, but only in the main important parts.

The final step before carrying out the simulation is to define the boundary conditions. For a

mechanical simulation as the ones carried out for this thesis, the boundary conditions must

include the fixtures of the part, as well as the forces it will be subjected to. The correct



                                    65
```

<a id="diaz-p085"></a>

#### DIA · PDF página 085 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION

calculation of the forces is a fundamental part of the process, as the unproper selection can

lead to a catastrophic failure of the design.

4.1.2 DESIGN ADAPTATION

The design adaptation for FEA of the final rocker models mainly consisted of the rounding

of the joint between the actuating arms and the main rotating cylinder for all the arms.





                       Figure 55. Image of the CAD model of the left-side rocker with the changes highlighted





                      Figure 56. Image of the CAD model of the right-side rocker with the changes highlighted


                                    66
```

<a id="diaz-p086"></a>

#### DIA · PDF página 086 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION

4.1.3 MESHING

The meshing of the parts was done utilizing SolidWorks embedded meshing software. The

first element meshed was the left-side rocker and based on the results observed form the first

simulation, the parameters for the second simulation were changed to obtain a quicker less

resource consuming result.





                                  Table 2. The parameters of the mesh of the left-side rocker





                                  Table 3. The parameters of the mesh of the right-side rocker




                                    67
```

<a id="diaz-p087"></a>

#### DIA · PDF página 087 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION

Based on the results obtained, it can easily be seen how both meshes have a great quality,

with element counts of over a million elements in both and with less than 0.01% of the

elements having an aspect ratio of over 10 and over 99.6% having an aspect ratio lower than

3. Which means the mesh correctly transitions between the elements without distorting. It is

also important to note that the change in the mesh parameters for the second part resulted in

a reduction of time of 66% with a similar overall mesh quality.

Another notable point is that the meshing of the critical points, defined by the unions between

the actuating arms and the main cylinder, was controlled to different specifications to obtain

a better-quality analysis on the most stressed parts. The parameters for these areas were:





                        Table 4. The parameters of the mesh of the controlled region of the left-side rocker





                       Table 5. The parameters of the mesh of the controlled region of the right-side rocker

As it can be observed in the tables, the parameters of the controlled regions were also altered

to help with the resource load.

The final meshes obtained for the parts were:





                                    68
```

<a id="diaz-p088"></a>

#### DIA · PDF página 088 de 101

```text
        UNIVERSIDAD PONTIFICIA COMILLAS
        ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
       GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                           DESIGN VALIDATION





                  Figure 57. Mesh of the left-side rocker





Figure 58. Closeup of the mesh of the controlled region of the left-side rocker





                    69
```

<a id="diaz-p089"></a>

#### DIA · PDF página 089 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION





                                             Figure 59. Mesh of the right-side rocker





                           Figure 60. Closeup of the mesh of the controlled region of the right-side rocker

As it can be observed, the change in meshing on the second part allowed the total mesh to

be less dense while maintaining the adequate size on the important regions, as well as a good

aspect ratio.

4.1.4 LOAD AND FIXTURE APPLICATION

The loads selected, were obtained based on last year’s analysis of the suspension utilizing a

spreadsheet which calculates the maximum loads suffered by any bar of the suspension

system. These values were taken as the maximum forces to be applied by the pushrod, and

both springs, and are over dimensioned by a factor of about 1.2.


                                    70
```

<a id="diaz-p090"></a>

#### DIA · PDF página 090 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION

The fixture for the study was taken as the endpoints of the cylindrical section to allow

possible internal elastic deformations.

4.1.5 ANALYSIS OF THE RESULTS

The results obtained from the simulations consisted of diagrams of the local deformations,

stress and factor of safety.





                                Figure 61. Image of the von Mieses stresses of the right-side rocker





                                  Figure 62. Image of the static deflection of the right-side rocker

                                    71
```

<a id="diaz-p091"></a>

#### DIA · PDF página 091 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION





                                  Figure 63. Image of the factor of safety of the right-side rocker





                                 Figure 64. Detail of the most stressed part of the right-side rocker

In the simulation results, the right-side design has a minimum factor of safety of 1.6, which

is within the design specifications. The other results show a maximum static deflection of 1

mm which is within the accepted length.  It is also important to note that the point of

maximum stress was found in the union between the actuating arms and the main body.


                                    72
```

<a id="diaz-p092"></a>

#### DIA · PDF página 092 de 101

```text
     UNIVERSIDAD PONTIFICIA COMILLAS
     ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
    GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                       DESIGN VALIDATION





Figure 65. Image of the von Mieses stresses of the left-side rocker





  Figure 66. Image of the static deflection of the left-side rocker


                 73
```

<a id="diaz-p093"></a>

#### DIA · PDF página 093 de 101

```text
     UNIVERSIDAD PONTIFICIA COMILLAS
     ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
    GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                       DESIGN VALIDATION





  Figure 67. Image of the factor of safety of the left-side rocker





Figure 68. Detail of the most stressed part of the right-side rocker


                74
```

<a id="diaz-p094"></a>

#### DIA · PDF página 094 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                           DESIGN VALIDATION

As it can be seen in the simulation results, the left-side design has a minimum factor of safety

of 1.3, which understanding the overestimation of the forces is well within the design

specifications. The other results show a maximum static deflection of 1 mm which is within

the accepted length. Similarly to the other side, point of maximum stress was found in the

union between the actuating arms and the main body, as was expected, and therefore, the

meshing was correctly done.

To conclude, we can comfortably state that the design will correctly perform in the desired

manner without failure.





                                    75
```

<a id="diaz-p095"></a>

#### DIA · PDF página 095 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                   CONCLUSION AND ROAD MAP


    Chapter 5.  CONCLUSION AND ROAD MAP

5.1  CONCLUSION


To conclude this thesis, the design for the roll-heave decoupled suspension can be taken as

fully completed as the correct analysis and justification for the project has been completed.

The understanding gained during the thesis will also prove fundamental for the integration

of the system in future prototypes.

As previously defined, the objective of the thesis was the design and integration of a new

front  suspension system which  will be  able  to  control  independently  the  vertical

(longitudinal) dynamics of the front axle from the lateral dynamics, with the aim being the

improvement of the cornering dynamics of the vehicle during all the stages: braking/corner

entry, steady state cornering and accelerating/corner exit, with the consideration for the

design being that it should be easy to manufacture and assemble, be easy to access to change

the parameters and should try to lower the height of the centre of mass of the vehicle.

Looking at the final design,  it can be seen how  it achieves the objectives by being a

completely decoupled system which will improve, as observed in the simulations, the

behaviour of the vehicle in all the dynamics. The ease of manufacturing has been taken into

account, as well as the ease to change the parameters and the lowering of the centre of mass.

The specific design requirements were also met, by achieving a parametric design which

can easily be adapted to any possible changes in the team’s chassis or suspension geometry

with minimal design changes needed in order to be adapted, and the needed changes are

already studied and documented in this thesis. The dynamic design objectives were also met,

with  the  correct  ride frequencies  obtained, as well  as  the desired  roll  gradient  to

accommodate the design to the vehicle.





                                    76
```

<a id="diaz-p096"></a>

#### DIA · PDF página 096 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                   CONCLUSION AND ROAD MAP

Moreover, the progress achieved by this thesis for the design of the IFS-07 prototype is not

only achieved by the general design developed, but also by the study into the field of vehicle

dynamics carried out, especially in regard to how does this field affect the overall design of

a Formula Student prototype.

To sum up, it can be stated that the significant changes presented by this thesis are:

- Redesign of the front suspension to incorporate a roll-heave decoupled system.

- Better understanding and mathematical modelling of the dynamic behaviour of the vehicle.

- Improved dynamic behaviour of the vehicle by the correct selection of spring stiffnesses

for both heave and roll.

- Better understanding of the kinematic behaviour of the vehicle’s suspension.

Some of the limitations of the thesis include:

- The decoupling of the rear suspension: The rear suspension design was not done, due to

the spatial constraints on the rear of the vehicle.

- The dynamic modelling of the full vehicle: The dynamic modelling of the vehicle was

not achieved, due to the high complexity of the interactions between the systems.

- The manufacturing of the suspension: The manufacturing process of the suspension was

not defined in detail, as well as was limited to some easier to use technologies.

5.2  PATH OF DEVELOPMENT FOR THE SUSPENSION


The future work for the team regarding the suspension system of the vehicle and the

understanding of the vehicles dynamic behaviour is plenty. This work for the next seasons

should include:





                                    77
```

<a id="diaz-p097"></a>

#### DIA · PDF página 097 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                   CONCLUSION AND ROAD MAP

- Rear decupled suspension: With the design for the front axle done, the design for a roll-

heave decoupled rear axle should be made when the spatial limitations are cleared. This

design should help the vehicle, completely and independently control its roll and heave.

- Full dynamic model of the vehicle: With a first approach done in this thesis for the

dynamic modelling, so be it a very simple independent model for the front axle, a total

interconnected dynamic model for the vehicle should be made, to better understand the

behaviours of the designs made before taking them to reality. This model should also be

validated correctly with the real behaviour of the car.

- Verification and telemetry: The  correct  verification  of  the  vehicle through  the

implementation of a correctly defined and calibrated telemetry system is fundamental to

correct design and setup of the suspension system. This verification process should also help

correlate the designed behaviour of the vehicle to the real behaviour of the vehicle.

- Understanding of the fabrication methods: The understanding of the fabrication

methods will directly correlate to the precision and validity of the manufactured system. The

main focus for this point should be the design and fabrication of a welding jig and new

suspension hardpoints to be able to more correctly and accurately define the suspension

geometry.

- General weight reduction of the system: The general wight reduction of the system, with

a special focus in the reduction of the unsprung mass is essential for a better dynamic

behaviour of the vehicle. It is important to note that the majority of the mass of the vehicle’s

suspension system is part of the unsprung mass, with the reduction of weight in both uprights

and wheels being possible, this point should be a main focus.

-Lap time simulation: The justification of every design change on the vehicle should be

tied with the effect that change has on the lap time. This is why a lap time simulation, with

an especial emphasis on the dynamic behaviour of the vehicle is an essential tool for the

design validation. This toll should also help understand better the setup changes, as well as

be validated with the reality.


                                    78
```

<a id="diaz-p098"></a>

#### DIA · PDF página 098 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                   CONCLUSION AND ROAD MAP

The changes here proposed are thought to show what the general design lines for the

suspension and vehicle dynamics department should follow, taking the current state of the

project.





                                    79
```

<a id="diaz-p099"></a>

#### DIA · PDF página 099 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                  BIBLIOGRAPHY


           Chapter 6. BIBLIOGRAPHY


[1]    FSG, Formula Student Rules 2024. Version 1.1, 2024.


[2]    “FSG: Track Layout.” https://www.formulastudent.de/pr/news/details/article/tracklayout/

        (accessed Jul. 1, 2024).



[3]    W. F. Milliken and D. L. Milliken, Race Car Vehicle Dynamics, 1995.



[4]     C. Smith, Tune to Win, 1978.



[5]     T. D. Gillespie, Fundamentals of Vehicle Dynamics, 1992.



[6]     Pacejka, Tire and Vehicle Dynamics, 1981.



[7]    A. Anwar, “The normal vehicle forces effects of a two in-wheel electric vehicle towards

        the human brain on different road profile manoeuvre”. SN Applied Sciences, 2020.



[8]    A. Zarzuelo, Desarrollo de herramientas para el diseño y optimización de la suspensión,

       2020.



[9]     Hasan-Farag, Rana Raouf. “Active neuro-fuzzy integrated vehicle dynamics controller to

       improve the vehicle handling and stability at complicated manoeuvres.”, 2016.



[10]   “What is negative camber” https://www.tirebuyer.com/education/what-is-negative-camber

        (accessed Jul.1, 2024).


                                    80
```

<a id="diaz-p100"></a>

#### DIA · PDF página 100 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                                                                  BIBLIOGRAPHY




[11]   "CAMBER, CASTER, TOE-IN/TOE-OUT EXPLAINED"

        https://www.intraxracing.nl/en/camber/ (accessed Jul. 1, 2024).



[12]   "The caster angle: what it is and how it affects driving dynamics"

        https://trinacriasimracing.wordpress.com/the-caster-angle-what-it-is-and-how-it-affects-

        driving-dynamics/ (accessed Jul. 1, 2024).



[13]    "Kingpin Angle, Scrub Radius & Wheel Centre Offset"

        https://race.software/ulp_lesson/kingpin-angle-scrub-radius-wheel-centre-offset/ (accessed

         Jul. 1, 2024).



[14]   M. Urban, Design of Formula Student Wheel Suspensions, 2020.



[15]    C. Hahn, "Hydraulic decoupled suspension – the journey starts with a bold idea"

        https://blogs.mathworks.com/student-lounge/2018/02/01/hydraulic-decoupled-suspension-

         part1/ (accessed Jul. 1, 2024).


[16]    "Gallery: Mercedes-AMG Project One underpinnings"

        https://uk.motor1.com/photos/527814/mercedes-amg-project-one-

       underpinnings/#1920745_mercedes-amg-project-one-underpinnings (accessed Jul. 1,

        2024).





                                    81
```

<a id="diaz-p101"></a>

#### DIA · PDF página 101 de 101

```text
                    UNIVERSIDAD PONTIFICIA COMILLAS
                       ESCUELA TÉCNICA SUPERIOR DE INGENIERÍA (ICAI)
                      GRADO EN INGENIERÍA EN TECNOLOGÍAS INDUSTRIALES

                              ANNEX I. SUSTAINABLE DEVELOPMENT GOALS


  ANNEX I. SUSTAINABLE DEVELOPMENT GOALS


The project presented, aligns with the Sustainable Development Goals (SDG) such as:

          - Goal 9: Industry innovation and infrastructure. The design of new more efficient

        suspension systems helps develop new innovations in the automotive industry; by

        introducing new and different ideas, the industry can develop these ideas into more

         efficient designs which can therefore be applied to the everyday use.


          - Goal 11: Sustainable cities and communities. The study of new, better responding and

        better riding suspension systems can help make the cars safer for both occupants of the

        vehicle and bystanders, helping the vehicle maintain control in unstable situations therefore

       helping with the creation of safer, and therefore more sustainable, communities where

       automobiles can exist.


          - Goal 13: Climate action. The study and understanding of the dynamics of any vehicle

       can help achieve better energy efficiency, this helps reduce the energy consumption of

        vehicles. The other way this project helps with the climate action is that by being part of an

         electric formula student team, one can understand the new emerging problems with the

       suspension systems of EVs, this knowledge can be applied to better understand these types

        of vehicles and how they can be approached either by big manufacturers, bringing less

        polluting vehicles to market or smaller racing series and how they can in a more

       performance oriented way transition to the greener electric mobility.´





                                                            Figure 69. SDG’s


                                    82
```

---

<a id="fuente-gaither"></a>

## Fuente GAI — Vehicle Dynamics on an Electric Formula SAE Racecar

**Autor(es):** Audrey Gaither. **Año:** 2021. **Idioma:** inglés. **Páginas del PDF:** 30. **Documento:** `gaither-gaither-sb-2-2021-thesis.pdf`. **Original:** https://dspace.mit.edu/entities/publication/cb994dec-2439-445e-bd4c-a235120c068b.

### Índice de páginas de esta fuente

[1](#gaither-p001) · [2](#gaither-p002) · [3](#gaither-p003) · [4](#gaither-p004) · [5](#gaither-p005) · [6](#gaither-p006) · [7](#gaither-p007) · [8](#gaither-p008) · [9](#gaither-p009) · [10](#gaither-p010) · [11](#gaither-p011) · [12](#gaither-p012) · [13](#gaither-p013) · [14](#gaither-p014) · [15](#gaither-p015)

[16](#gaither-p016) · [17](#gaither-p017) · [18](#gaither-p018) · [19](#gaither-p019) · [20](#gaither-p020) · [21](#gaither-p021) · [22](#gaither-p022) · [23](#gaither-p023) · [24](#gaither-p024) · [25](#gaither-p025) · [26](#gaither-p026) · [27](#gaither-p027) · [28](#gaither-p028) · [29](#gaither-p029) · [30](#gaither-p030)

### Índice de secciones del documento

- [Introducción](#gaither-p005)
- [Neumáticos / Tires](#gaither-p007)
- [Kinematics](#gaither-p013)
- [Dynamics](#gaither-p019)
- [Aerodynamics](#gaither-p021)
- [Steering](#gaither-p023)
- [Control Arms](#gaither-p024)
- [Further Research](#gaither-p026)
- [Referencias](#gaither-p030)

### Índice de figuras y tablas localizadas en el texto

- [PDF pág. 5](#gaither-p005): Figure 1: Cheyenne Hua driving MY18 (Photo by Leigh Highby)
- [PDF pág. 6](#gaither-p006): Figure 2: MIT Motorsports and MY19, June 2019
- [PDF pág. 7](#gaither-p007): Figure 3: MY20 Assembly, Spring 2020
- [PDF pág. 8](#gaither-p008): Figure 4: Flow chart showing tire function inputs and outputs.
- [PDF pág. 9](#gaither-p009): Figure 5: Impact of normal force on coefficient of friction on FSAE tire.
- [PDF pág. 10](#gaither-p010): Figure 6: Back view of a tire with camber, arrow showing direction of camber thrust.
- [PDF pág. 10](#gaither-p010): Figure 7 shows the typical relationship between slip ratio and a tire’s longitudinal force
- [PDF pág. 11](#gaither-p011): Figure 7: Typical relationship between Slip Ratio and Traction Force. Plot from Milliken &
- [PDF pág. 11](#gaither-p011): Figure 9 shows the optimal slip ratio and slip angle under a variety of normal forces for
- [PDF pág. 12](#gaither-p012): Figure 8: Relationship between Slip Angle and Lateral Force on Goodyear tire. Plot from
- [PDF pág. 12](#gaither-p012): Figure 9: Optimal Slip Angles and Slip Ratios for tires during MY20 selection process.
- [PDF pág. 13](#gaither-p013): Figure 10: A front view of a suspension system. Roll center (RC) and instant center (IC) on a
- [PDF pág. 14](#gaither-p014): Figure 11: Wheel geometries defined by suspension geometry.
- [PDF pág. 15](#gaither-p015): Figure 12: Top view of tire with positive caster.
- [PDF pág. 16](#gaither-p016): Figure 13: Impact of static and dynamic camber on total lateral force capability.
- [PDF pág. 17](#gaither-p017): Figure 14: The impact of control arm length on roll center movement over a roll movement in
- [PDF pág. 18](#gaither-p018): Figure 15: Simulation of MY19 camber on all four tires during a combined roll and steer motion.
- [PDF pág. 18](#gaither-p018): Figure 16: Simulation of MY19 front and rear roll center heights during a combined roll and
- [PDF pág. 19](#gaither-p019): Figure 17: MY19 Ball joint geometry values.
- [PDF pág. 20](#gaither-p020): Figure 18: Impact of TLLTD on total lateral acceleration capabilities on MY20 vehicle.
- [PDF pág. 20](#gaither-p020): Figure 20.
- [PDF pág. 21](#gaither-p021): Figure 19: Iteration process to determine optimal lateral acceleration and TLLTD range.
- [PDF pág. 21](#gaither-p021): Figure 20: Vehicle dynamics properties on MY19 vehicle.
- [PDF pág. 22](#gaither-p022): Figure 21: Vehicle axis of rotation. Pitch axis shown in green.
- [PDF pág. 22](#gaither-p022): Figure 22 (simulations run by Stefan Borjan (MIT ’22)), which show that the rear wing
- [PDF pág. 23](#gaither-p023): Figure 22: The impact of pitch on aerodynamic performance of the MY19 vehicle. Note that the
- [PDF pág. 24](#gaither-p024): Figure 23: A plot showing unsteady driver torque (left) and total driver torque (right) to turn the
- [PDF pág. 25](#gaither-p025): Figure 24: Predicted forces through the six control arms on the front tire of the MY20 vehicle.
- [PDF pág. 26](#gaither-p026): Figure 25 shows one screenshot of an ANSYS Static Structural model of a rear control arm on
- [PDF pág. 26](#gaither-p026): Figure 25: ANSYS Static Structural model of the rear lower control arm on the MY20 vehicle.
- [PDF pág. 28](#gaither-p028): Figure 26: Measured forces in the suspension forces on the rear lower control arm on the rear
- [PDF pág. 29](#gaither-p029): Figure 27: MY18 (front) and MY19 (rear right) on a testing trip in fall 2019.

### Transcripción por página

<a id="gaither-p001"></a>

#### GAI · PDF página 001 de 030

```text
            Vehicle Dynamics on an Electric Formula SAE Racecar


                                      by

                                 Audrey Gaither


                                   Submitted to the
                         Department of Mechanical Engineering
                   in Partial Fulfillment of the Requirements for the Degree of

                      Bachelor of Science in Mechanical Engineering

                                                   at the

                           Massachusetts Institute of Technology


                                      June 2021


              © 2021 Audrey Gaither. All rights reserved.





Signature of Author:
                                                 Department of Mechanical Engineering
                                                       May 15, 2021


Certified by:
                                                      Amos G. Winter
                                              Associate Professor of Mechanical Engineering
                                                                         Thesis Supervisor


Accepted by:
                                                                   Kenneth Kamrin
                                              Associate Professor of Mechanical Engineering
                                                                 Undergraduate Officer



                                       1
```

<a id="gaither-p002"></a>

#### GAI · PDF página 002 de 030

```text
            Vehicle Dynamics on an Electric Formula SAE Racecar


                                      by

                                 Audrey Gaither


                  Submitted to the Department of Mechanical Engineering
                     on May 15, 2021 in Partial Fulfillment of the
                            Requirements for the Degree of

                      Bachelor of Science in Mechanical Engineering



ABSTRACT

       Vehicle dynamics on a Formula SAE vehicle are inter-dependent with almost all
mechanical systems on the car and require a thorough understanding of design tradeoffs in order
to maximize the vehicle’s acceleration capabilities while maintaining consistent driver feedback.
This thesis summarizes the developments and accumulated knowledge on MIT’s Formula SAE
team with regards to suspension and vehicle dynamics of the 2018 – 2020 seasons in order to
inform the design and vehicle development for future years. Vehicle kinematics, vehicle
dynamics, and tire selection are covered, in addition to the impact of aerodynamics, steering, and
control arms on suspension development. Areas for further research are described. Throughout
the thesis, the importance of quantifying and documenting design decisions is highlighted.


Thesis Supervisor: Amos G. Winter
Title: Associate Professor of Mechanical Engineering





                                       2
```

<a id="gaither-p003"></a>

#### GAI · PDF página 003 de 030

```text
                              Acknowledgements

         I would like to thank all the members of the MIT Motorsports team, past and present, for
building this team and such an inclusive, welcoming environment. I would especially like to
think the members of the MY20 exec team: Annetoinette Figueroa, Dani White, and Elijah
Stanger-Jones for their incredible support during the 2020 season.

         I would also like to thank the MIT Edgerton Center for believing in our team and
providing us with an amazing space and community for us to build our cars.





                                       3
```

<a id="gaither-p004"></a>

#### GAI · PDF página 004 de 030

```text
                                Table of Contents


Abstract                                                                 2

Acknowledgements                                                        3

Table of Contents                                                          4

    1. Introduction                                                        5

        1.1. Team Background                                                5

        1.2. Thesis Scope                                                    7

    2. Tires                                                               7

        2.1. Background                                                     7

        2.2. Definitions                                                      8

        2.3. Tire Selection Process                                             13

    3. Kinematics                                                         13

        3.1. Inputs                                                          13

        3.2. Outputs                                                        16

        3.3. Iteration Process                                                 17

       3.4 2019 Results                                                     17

    4. Dynamics                                                           19

        4.1. Definitions                                                      19

       4.2 Process & 2019 Results                                            20

    5. Team Integrations                                                   21

        5.1. Aerodynamics                                                   21

        5.2. Steering                                                        23

        5.3. Control Arms                                                   24

    6. Areas for Further Research                                           26

        6.1. Suspension System                                               26

        6.2. Related Systems                                                 27

References                                                               30





                                       4
```

<a id="gaither-p005"></a>

#### GAI · PDF página 005 de 030

```text
    1. INTRODUCTION

    1.1 Team Background

      MIT’s Formula SAE Team designs, builds, and races an electric Formula-One stye

racecar every year for the international Formula SAE Competition series. Figure 1 shows MIT’s

2018 vehicle (MY18) performing straight-line acceleration. MIT Motorsports (aka MIT FSAE)

is a student-run team operating out of MIT’s Edgerton Center, an organization that, among other

pursuits, provides space, funding, and support for student organizations on campus. MIT

Motorsports was founded in 2001 to compete in the internal combustion division of the

competition. In 2014, the team switched over to electric vehicles. In June 2017, the team won

second place at the annual competition in Lincoln Nebraska, and, since then, the team has had its

sights set on first place.





               Figure 1: Cheyenne Hua driving MY18 (Photo by Leigh Highby)

      The competition, hosted by the Society of Automotive Engineers (SAE) provides an

opportunity for teams to race their vehicles and present their design rationale to judges. There are

four dynamic, or racing, events: Acceleration, Skidpad, Autocross, and Endurance. Acceleration

is a 75 meter straight, and top times for this event often cut under 4.0 seconds. Skidpad is an

event designed to test lateral acceleration, where drivers circle a 15-meter circle of cones. Top

times for this event range between 5 and 5.5 seconds. Autocross records a team’s fastest lap time

on an autocross track, including straightaways and hairpin turns, and lap times often range

between 60-90 seconds. Lastly, the endurance event, with the maximum number of possible

points, requires teams to complete a 22 km endurance race, involving multiple laps around a

custom track. This event typically takes around 30 minutes and is a grueling test of car & driver

                                       5
```

<a id="gaither-p006"></a>

#### GAI · PDF página 006 de 030

```text
reliability and performance. In addition to these racing events, teams present their design

rationale, economic viability, and cost breakdown to a panel of automotive experts. Figure 2

shows MIT Motorsports celebrating with the 2019 vehicle (MY19) after competition in June

2019.





                      Figure 2: MIT Motorsports and MY19, June 2019

     MIT Motorsports is currently composed of around 50 MIT students. The team is mostly

undergraduate, with representation from many different majors, including mechanical

engineering, aerospace engineering, electrical engineering, computer science, materials

engineering, and business. Women constitute around one-third of the team, which is rare in this

competition series, and can be attributed to the team’s history of inclusion and strong female role

models.

     MIT Motorsports typically competes in one competition every summer, and the annual

timeline is built around that competition. Vehicle design occurs in the fall, followed by

manufacturing over IAP and assembly & testing in the spring semester. An efficient design

process is critical for enabling a productive spring testing season and requires effective

communication and planning. Since 2017, MIT Motorsports has made a substantial push to


                                       6
```

<a id="gaither-p007"></a>

#### GAI · PDF página 007 de 030

```text
streamline documentation and retain knowledge gained in previous years to build faster vehicles

on a faster timeline. The path to a completed racecar often requires many long nights of

assembly and design (see Figure 3, during spring semester of the MY20 build cycle), but many

students remember the development of their racecar as some of their favorite memories at MIT.





                          Figure 3: MY20 Assembly, Spring 2020

       1.2 Thesis Scope

       This thesis summarizes the main drivers of suspension design and vehicle dynamics

developed on MIT Motorsports from 2018-2020. This thesis is not intended to fully describe any

one singular vehicle completely, but rather to combine learnings from multiple design cycles to

give an accurate synopsis of the knowledge on MIT Motorsports with the intent to aid future

design cycles.

    2. TIRES

    2.1 Background

    A complete understanding of vehicle dynamics and suspension always starts with the

tires. The tires are the only component of the vehicle that touches the asphalt, and therefore the

only component of the vehicle capable of transmitting forces to the ground. The goal of the

                                       7
```

<a id="gaither-p008"></a>

#### GAI · PDF página 008 de 030

```text
suspension system on a Formula SAE vehicle is to maximize the vehicle’s acceleration

capability over the course of the competition without providing inconsistent driver feedback. In

the case of tires, the goal of the suspension system is to maximize the amount of force that the

tires can produce over the course of vehicle operation.

       Tire forces are a function of various inputs, shown in Figure 4. At any one moment in

time, a tire is capable of producing some amount of force in any given direction. The goal of the

vehicle dynamicist is to understand how to produce the maximum full vehicle acceleration at any

given moment by adding up the forces from all four tires.





                Figure 4: Flow chart showing tire function inputs and outputs.

       2.2 Definitions

      There are four main inputs that define what forces a tire is capable of producing. In

addition to forces parallel to the ground, tires are also capable of producing moments about all of

their axis, but these will be neglected for the purposes of this thesis.

      The first, and perhaps most important input to the tire function is the normal force on the

tire. When considering friction force in its simplest case, friction is usually modeled as

                                                       Ffriction = Fnormal*mu,

where mu is the coefficient of friction between the object and the ground. This relationship does

apply to tires: the maximum force capability of a tire generally increases along with increasing

normal force. However, the coefficient of friction of tires decreases as normal force increases,

which is a key concept that drives many decisions in vehicle dynamics that will be covered later

in this thesis. Figure 5 is a plot, developed by Cheyenne Hua (MIT ’19) that shows the impact of

normal force on the coefficient of friction on a Formula SAE tire. The coefficient of friction is

defined as the ratio of a tire’s longitudinal or lateral force capability and its normal force.



                                       8
```

<a id="gaither-p009"></a>

#### GAI · PDF página 009 de 030

```text
            Figure 5: Impact of normal force on coefficient of friction on FSAE tire.

       This concept is crucial for vehicle dynamics because it implies that a vehicle should

strive to minimize weight transfer as much as possible in most cases. For instance, when weight

shifts to an outside corner during a turn, those two outer tires have a larger force capability, but

the sum of the force capability on all four tires is lower.

      Another factor in tire force capability is inclination angle, also known as camber. Camber

is measured as the inclination of the tire from the vertical access when viewed from the front or

back of the tire. Negative camber is defined as when the top of the tire is closer to the vehicle

than the bottom. Camber has received a lot of focus on MIT Motorsports because it can vary

significantly as a function of suspension geometry. Camber is important because a cambered tire

produces camber thrust, as shown in Figure 6. When a tire has negative camber, the rubber on

the bottom of one side of the tire is compressed, producing a force towards the center of the

vehicle known as camber thrust. When tires have positive camber, a force away from the vehicle

is produced, which can pull the driver to steer away from the centerline and drive erratically.

Camber can also cause a tire to heat up on the compressed side, leading to uneven tire wear and

performance characteristics.

                                       9
```

<a id="gaither-p010"></a>

#### GAI · PDF página 010 de 030

```text
      Figure 6: Back view of a tire with camber, arrow showing direction of camber thrust.

       Slip Ratio is the driver of a tire’s longitudinal force capability. Slip Ratio is defined as

                           SR = R / Rf – 1,

where “R” represents the angular velocity of a driven tire, and “Rf” represents the angular

velocity of an undriven tire. Another way to think about slip ratio is the ratio between the number

of tire rotations and the distance traveled by the vehicle. A slip ratio of zero means that the

distance traveled by the vehicle is exactly equal to the number of rotations of the tire multiplied

by the circumference of the tire. As the tire slips, it will gain a slip ratio because the distance

traveled will be less than the number of rotations multiplied by the circumference.

       Figure 7 shows the typical relationship between slip ratio and a tire’s longitudinal force

capability, taken from Milliken & Milliken’s Race Car Vehicle Dynamics. At zero slip ratio, the

tire is not capable of producing any longitudinal force. As the slip ratio increases, plastic

deformation in the rubber enables the tire to produce longitudinal forces. The constant slope

shown in the low-slip ratio regime is sometimes referred to as the “tire stiffness.” The

longitudinal force capability increases up to a peak, typically at less than SR = 0.1 for FSAE

tires, and then decreases again. The width of this peak varies between tires, and a wider peak

makes it easier for the driver to operate the vehicle at an ideal slip ratio during a race. FSAE

vehicles typically operate in lower slip ratio regimes, and rarely reach slip ratios of 1 or above.





                                      10
```

<a id="gaither-p011"></a>

#### GAI · PDF página 011 de 030

```text
   Figure 7: Typical relationship between Slip Ratio and Traction Force. Plot from Milliken &
                            Milliken’s Race Car Vehicle Dynamics.

      The last major input to tire forces is the tire’s slip angle, which is the difference between

the orientation of the tire and the vehicle’s velocity vector. Slip angles are required to provide

substantial lateral forces and rely on plastic deformation of the tire, similar to slip ratio and

longitudinal forces. Figure 8 shows the relationship between longitudinal force capability and

slip angle, also adapted from Race Car Vehicle Dynamics. The behavior is very similar to that of

longitudinal forces and slip ratio, with a quasi-linear region at low slip angles (“tire cornering

stiffness”), followed by a peak at higher angles. The width of this peak is very important for

vehicle handling because it directly impacts how drivers should steer the vehicle and the stability

of cornering behavior.

       Figure 9 shows the optimal slip ratio and slip angle under a variety of normal forces for

various tires during the MY20 tire selection process, here to help provide more intuition on

normal ranges for both values. Graphs created by Rebecca McCabe (MIT ’21). As shown, the

optimal slip and slip ratio reach maximums at 13 degrees and 0.08, respectively, for typical

performance of an FSAE vehicle.





                                      11
```

<a id="gaither-p012"></a>

#### GAI · PDF página 012 de 030

```text
Figure 8: Relationship between Slip Angle and Lateral Force on Goodyear tire. Plot from
                 Milliken & Milliken’s Race Car Vehicle Dynamics.





 Figure 9: Optimal Slip Angles and Slip Ratios for tires during MY20 selection process.


                                   12
```

<a id="gaither-p013"></a>

#### GAI · PDF página 013 de 030

```text
       2.3 Tire Selection Process

       Tire selections are made based on the sensitivity of tire forces to any of the four inputs, as

well as thermal and tire wear characteristics. Often, the sensitivity of tire forces can be even

more important than the theoretical maximum force the tires can produce. For instance, if the

maximum lateral force varies significantly with slip angle near the optimal slip angle, the team

will often select a less finicky tire to provide an easier driving experience. During the MY20 tire

selection process, multiple tires showed similar characteristics. The team chose 16” diameter

Hoosier tires (43070 and 43075 for front and rear, respectively) instead of the 18” diameter

Hoosier tires that the team had used for MY17, MY18, and MY19. The tires showed very similar

dynamic characteristics, but the switch was made due to the lower thermal mass of the 16” tires.

Warm tires perform better than cold tires (which is why many competitive race teams warm up

the tires before a race), and a smaller tire will heat up faster during a race than cold ones.

    3. KINEMATICS

    3.1 Inputs

   Vehicle kinematics is the study of how a vehicle’s geometry moves and changes over the

course of a vehicle’s operation. The vehicle dynamicist influences the vehicle’s kinematics by

determining the locations of the suspension linkages. These linkages then define a variety of

other geometric parameters that influence vehicle dynamics. One such parameter is called the

roll center, shown in Figure 10, from Race Car Vehicle Dynamics.





  Figure 10: A front view of a suspension system. Roll center (RC) and instant center (IC) on a
                                      vehicle are labeled.


                                      13
```

<a id="gaither-p014"></a>

#### GAI · PDF página 014 de 030

```text
      From a front view, the point connecting the lines of action of the suspension linkages is

called the instant center. If you draw a line of action from each instant center and connect it with

the contact patch of the opposite tire (or the point at the center of the tire where it meets the

ground), the overlap of these two lines is called the roll center. The roll center is significant

because it is the point on the vehicle where lateral forces can be applied to the car without the

vehicle rolling. The vehicle rotates about this point.

     When a vehicle turns, lateral forces are applied to the car’s center of gravity, which is

located above the roll center. The moment arm between these two points creates a rotation of the

vehicle, which changes how forces are distributed throughout the car. Because of this interaction,

minimizing roll center movement over the course of a vehicle’s performance is important to

maintaining consistent driver feel.

       In addition to the roll center, the suspension geometry in the wheel has various impacts

on vehicle performance. Figure 11, taken from Race Car Vehicle Dynamics, shows many such

important geometries. The point where the upper suspension linkages meet in the wheel is called

the upper ball joint (UBJ), and the point where the lower suspension linkages meet is called the

lower ball joint (LBJ), and the line connecting these two points is referred to as the “steering

axis.” The tire rotates about the steering axis.





                Figure 11: Wheel geometries defined by suspension geometry.

                                      14
```

<a id="gaither-p015"></a>

#### GAI · PDF página 015 de 030

```text
      The caster angle is the inclination of the steering axis when viewed from the side of the

vehicle. Caster is significant because a positive caster angle increases the directional stability of

the vehicle. This is shown in Figure 12, a top view of a wheel with positive caster. The red dot is

the intersection of the steering axis with the ground, and the blue rectangle is the region where

the tire makes contact with the ground. When the tire is turned, longitudinal tire forces act at a

distance d away from the steering axis, creating an “aligning moment” that pushes the driver to

steer the tire back to straight.





                       Figure 12: Top view of tire with positive caster.

       Associated with caster angle is mechanical trail, or the distance between the center of the

tire and the intersection of the steering axis with the ground when viewed from the side of the

vehicle. A larger mechanical trail creates a larger lever arm about the steering axis, and therefore

increases the force required by the driver to turn the vehicle.

      The kingpin angle is defined as the inclination of the steering axis from vertical when

viewed from the front of the vehicle. Positive kingpin is defined as when the UBJ is closer to the

center of the car than the LBJ. Kingpin angles play a significant impact in camber change over

vehicle motion, increasing positive camber on the outside wheel and increasing camber on the

inside wheel during turning. A positive kingpin angle also increases the ride height of the vehicle

when steered.




                                      15
```

<a id="gaither-p016"></a>

#### GAI · PDF página 016 de 030

```text
      Another important result of a kingpin angle is the scrub radius, or the distance between

the steering axis intersection with the ground and the center of the tire when viewed from the

front of the vehicle. A larger scrub radius causes higher driver forces when turning and causes

increased tire wear.

       3.2 Outputs

       Suspension geometry has a variety of important implications for vehicle performance,

such as roll center height movement and camber change. One substantial development made

during the 2019 season was determining, based on tire parameters, what camber change was

optimal for the vehicle. Camber thrust values were calculated for all four tires over the range of

expected lateral motions expected during a competition. These values were added together and

consolidated to produce the graph shown in Figure 13.





        Figure 13: Impact of static and dynamic camber on total lateral force capability.

     As shown in this graph, maximum vehicle performance is achieved when the vehicle’s

static camber (aka the camber on a stationary vehicle) is slightly negative and does not change

over the course of the vehicle’s operation. These insights allowed the team to tune suspension

behavior to minimize camber change, but more importantly to quantify the trade-offs in doing so.


                                      16
```

<a id="gaither-p017"></a>

#### GAI · PDF página 017 de 030

```text
       3.3 Iteration Process

      The iteration process for determining the suspension geometry is a lengthy one and

involves extensive communication with many different system owners. MIT Motorsports uses a

software called Optimum Kinematics to evaluate the suspension kinematics over the course of a

vehicle’s motion. The process will start with a proposed version of suspension geometry. A

subsystem owner will propose a change (i.e. because of an interference between components),

and the vehicle dynamicist will evaluate the impacts of that change by iterating solutions in

Optimum Kinematics. An example is shown in Figure 14, which shows the impact of control

arm length (suspension linkage length) on roll center movement.





  Figure 14: The impact of control arm length on roll center movement over a roll movement in
    the Optimum Kinematics software. Different lines represent different control arm lengths.

       3.4 2019 Results

       In the MY19 vehicle, the team was able to minimize camber change to less than 1 degree

and roll center height change to less than 0.02” over the course of a vehicle’s motion, as shown

in Figures 15 and 16. Static values of the ball joint geometry are shown in Figure 17.





                                      17
```

<a id="gaither-p018"></a>

#### GAI · PDF página 018 de 030

```text
Figure 15: Simulation of MY19 camber on all four tires during a combined roll and steer motion.





  Figure 16: Simulation of MY19 front and rear roll center heights during a combined roll and

                                             steer motion.


                                      18
```

<a id="gaither-p019"></a>

#### GAI · PDF página 019 de 030

```text
Ball Joint Geometry
                               Front             Rear
 Static Camber                   -1 degrees           -1 degrees
 Kingpin Angle                   4.6 degrees          4.2 degrees
 Scrub Radius                   1.043 in            0.45 in
 Mechanical Trail                1.57 in              0.73 in
 Caster Angle                     2.9 degrees          2.4 degrees
 Roll Center Height              0.38 in              0.36 in
 Toe                          0 degrees          0 degrees
 Front View Swing Arm Length   606.6 in            57.4 in
 Instant Center Height           9.49 in              0.87 in


                        Figure 17: MY19 Ball joint geometry values.

    4. DYNAMICS

    4.1 Definitions

       In addition to considering vehicle kinematics and geometry changes over the course of a

competition, the vehicle dynamicist must also consider the dynamics of the vehicle, or how

weight transfer and dynamic motion occur. One of the main advances over the 2019 season was

the increased understanding of the vehicle’s total lateral load transfer distribution, or TLLTD,

and optimization thereof.

     TLLTD is the ratio of weight transfer between the front and rear of a car during a turn.

This ratio impacts the vehicle’s total lateral force capability because it changes the normal forces

distributed to the tires, impacting their coefficient of friction and force capability. The impact of

TLLTD on total lateral acceleration capability in the MY20 vehicle is shown in Figure 18. For

that vehicle, the optimal rear TLLTD (rear lateral load transfer divided by the total lateral load

transfer) is right around 0.55. TLLTD also has impacts on vehicle handling and steering

characteristics. A higher rear TLLTD leads to oversteering because there is less load transfer

between the front steering tires and therefore more lateral load capability in the tires that steer,

and a lower rear TLLTD leads to oversteering, because there is more load transfer in the front

tires, and the associated decrease in mu decreases the total steering capability. Because of this,



                                      19
```

<a id="gaither-p020"></a>

#### GAI · PDF página 020 de 030

```text
MIT Motorsports designs a vehicle with a range of possible TLLTDs (achieved through tuning

various suspension components) to allow the driver to see what TLLTD works best for them.





     Figure 18: Impact of TLLTD on total lateral acceleration capabilities on MY20 vehicle.

       4.2 Process & 2019 Results

      The process to calculate the optimal TLLTD and range requires some assumptions about

the vehicle’s lateral acceleration and car parameters, and then iterates to find a solution. A flow

chart describing the process to find the optimal TLLTD is shown in Figure 19. Values that must

be considered are the vehicle’s overall roll gradient (or the degrees roll per degree of lateral

acceleration), roll rate (required torque to rotate the vehicle), and ride rate (required force to raise

or lower the vehicle’s center of gravity). Vehicle dynamics parameters from MY19 are shown in

Figure 20.

     To change the TLLTD and roll characteristics of a vehicle, the vehicle dynamicist can

change the suspension spring stiffnesses or the anti-roll-bar (ARB) stiffnesses. The anti-roll bars

are bars connecting the right and left sides of the suspension system, acting as torsional springs

and changing the respective and overall roll rates of the system. In 2019, the team had four

different ARB settings on the vehicle to allow for four different TLLTD settings.





                                      20
```

<a id="gaither-p021"></a>

#### GAI · PDF página 021 de 030

```text
    Figure 19: Iteration process to determine optimal lateral acceleration and TLLTD range.

Vehicle Dynamics
 Roll Gradient                0.85 degrees / “g” of lateral acceleration
 Front Spring Stiffness       200 lbs/in
 Rear Spring Stiffness       175 lbs/in
 Front Roll Rate             21400 Nm/rad
 Rear Roll Rate             23400 Nm/rad
 Total Roll Rate             44900 Nm/rad
 Front Ride Rate            24700 N/m
 Rear Ride Rate             22300 N/m


                  Figure 20: Vehicle dynamics properties on MY19 vehicle.

    5. TEAM INTEGRATION

    5.1 Aerodynamics

      The suspension and vehicle dynamics system on an FSAE racecar does not exist in a

vacuum. To optimize the vehicle as a whole, the vehicle dynamicist must work with almost

                                      21
```

<a id="gaither-p022"></a>

#### GAI · PDF página 022 de 030

```text
every mechanical system to make sure that all trade-offs are understood and quantified as best as

possible. One team that plays a significant role in vehicle performance is the aerodynamics team.

The center of pressure of the aerodynamics package is included when calculating the normal

forces and their distribution on the tires, the normal forces change as a function of velocity when

taking aerodynamic performance into account.

      From 2017-2019, MIT Motorsports was considering implementing anti-squat and anti-

dive geometry into the suspension design. This involves changing the placement of suspension

linkages in order to minimize the pitch of the vehicle, or rotation of the vehicle about its lateral

axis (shown in green in the corner of Figure 21). This geometry change would not change any

weight transfer but could potentially have impacts on the efficacy of the aerodynamic elements.





                Figure 21: Vehicle axis of rotation. Pitch axis shown in green.

       In order to quantify the impact of implementing this geometry, the aerodynamics team

ran their CFD model to look at the impact of pitch on the coefficient of lift (CL) and coefficient

of drag (CD) on both aerodynamic wings in the fall of the MY20 season. Results are shown in

Figure 22 (simulations run by Stefan Borjan (MIT ’22)), which show that the rear wing

performance is not highly dependent on the pitch, but the CL of the front wing is highly

dependent on pitch. This impact was not deemed significant enough to merit an overhaul of the




                                      22
```

<a id="gaither-p023"></a>

#### GAI · PDF página 023 de 030

```text
suspension system based on the increased design time (and associated decrease in vehicle testing

time), but simulations like these can be used to continue to quantify this trade-off in the future.





 Figure 22: The impact of pitch on aerodynamic performance of the MY19 vehicle. Note that the
                            front wing CL is highly dependent on pitch.

    5.2 Steering

      The team’s understanding of the relationship between suspension geometry and driver

steering forces has also developed significantly over the 2019-2020 seasons. As mentioned

previously, the job of the vehicle dynamicist is to maximize the acceleration capabilities of the

vehicle while maintaining consistent driver handling. When selecting tires, the sensitivity of the

vehicle’s performance to steering angle is considered such that the car handles consistently and

evenly. Steering forces are also considered, especially when considering the 22 km endurance

event. Drivers on MIT Motorsports are students, typically without any processional driving

experience. If steering forces on the vehicle are too high, drivers will become very fatigued over

the course of the 22 km event, which can cost valuable lap time and be potentially dangerous.

      The 2019 vehicle features a much larger scrub radius than in previous years, as well as a

longer steering bracket length (or the distance between the steering linkage and the steering

axis). These features (along with many others, including the mechanical trail & tire properties)

                                      23
```

<a id="gaither-p024"></a>

#### GAI · PDF página 024 de 030

```text
led to very high steering forces. Drivers were extremely fatigued at the end of the 2019

endurance race, and extensive effort was put into quantifying the impact of suspension geometry

on steering forces during the 2020 season.

     As shown in Figure 23 (a plot made by Rebecca McCabe (MIT ’21)), the torque required

to turn the wheels increases in response to both mechanical trail and scrub radius. These plots

describe steering torques during combined slip, or a scenario where the vehicle is accelerating

longitudinally and laterally at the same time, such as decelerating into a corner. Unstable torque

was also plotted in the left graph. Unstable torque is the tendency of the tires to steer away from

the center line, and is highly dependent on scrub radius, as mentioned in the kinematics section

of this thesis. The steering system is an example of a vehicle system that is highly interdepending

with steering geometry, and this understanding will make it that much easier to make design

tradeoffs in future design cycles.





 Figure 23: A plot showing unsteady driver torque (left) and total driver torque (right) to turn the
  wheels as a function of suspension geometry. Vehicles MY17 through MY20 are indicated in
                                              black.

       5.3 Control Arms

      The control arms are yet another vehicle system that vary substantially based on

suspension geometry. The control arms system is the system of six steel tubes that connect the

tire system to the chassis, or frame structure, of the vehicle. These control arms transmit forces

from the tire packages to the rest of the vehicle, connect the tire packages to the vehicle shocks

and ARBs, and transmit steering forces from the steering rack to the tire packages. A failure in


                                      24
```

<a id="gaither-p025"></a>

#### GAI · PDF página 025 de 030

```text
any suspension linkage would be very costly, and potentially dangerous, at competition. Because

of this, extensive calculations are performed to make sure that the control arms do not fail under

any loading conditions.

      The vehicle suspension lead dictates the points in space where the control arms attach to

the tire packages and the chassis of the vehicle, which in turn impacts the vehicle’s kinematics

and the loads transferred through the control arms. In 2018, a MATLAB plot was developed that

determines the loads in each of the control arms as a function of the lateral, longitudinal, and

vertical load applied on each tire. Figure 24 shows this plot for a front tire on the MY20 vehicle.

The tire forces are modeled as a vector applied to the bottom of the tire, and the forces are

calculated at every degree of possible load application.





 Figure 24: Predicted forces through the six control arms on the front tire of the MY20 vehicle.

      Changing the angles and locations of the control arms can have substantial impacts on the

forces transmitted through them, which leads to different total system masses as tubes are sized

up or down to account for the variance in forces. Typically, the limiting failure mode in sizing

the tubes is buckling, as they are long slender members that are required to withstand high

compression forces. In addition, some suspension linkages are occasionally placed in bending,


                                      25
```

<a id="gaither-p026"></a>

#### GAI · PDF página 026 de 030

```text
which leads to a more complicated structural analysis that is often done in parallel with FEA.

Figure 25 shows one screenshot of an ANSYS Static Structural model of a rear control arm on

the MY20 vehicle. When selecting suspension geometry, the suspension lead must consider the

mass of the control arm system, as well as the sizing. Occasionally, a proposed suspension

geometry will result in a control arm that is so large that it interferes with other components on

the vehicle, such as the rim of the tire, and must be redesigned.





 Figure 25: ANSYS Static Structural model of the rear lower control arm on the MY20 vehicle.

    6. AREAS FOR FURTHER RESEARCH

    6.1 Suspension System

   There are many areas for future research in the suspension system that could improve vehicle

performance and aid in streamlining the design cycle in future years. Starting with tires (as

previously mentioned, the most important aspect of a vehicle’s suspension), understanding the

impact of tire temperature on force capabilities would help inform both tire selection & vehicle

design as well as informing the team on the best ways to warm up the car during competition. As

mentioned, the MY20 vehicle tires were chosen partially because of the expected thermal

performance. The Calspan TTC data does include some data depicting warm-up behavior of the

tires before testing, so a more thorough analysis of that data could be fruitful. In addition,

empirical testing of the vehicle (recording acceleration times as a function of tire temperature)

could help the team understand how different warmups impact the vehicle’s potential.



                                      26
```

<a id="gaither-p027"></a>

#### GAI · PDF página 027 de 030

```text
      From a vehicle dynamics perspective, validating the vehicle’s roll rate, ride rate, and roll

gradient could be beneficial. Extensive time is spent in each vehicle design cycle on selecting

those roll and ride rates to select ARB stiffnesses and vehicle geometry, and it very well could be

that the vehicle does not behave as expected, and other factors (such as frame stiffness, battery

enclosure stiffness, etc.) play a large roll in determining transient vehicle characteristics. These

measurements could be taken by relating the vehicle’s IMU (or inertial measurement unit

onboard the chassis) to tire forces. Tire forces can be determined through strain gauges on

control arms or, with slightly less precision, load cells on the most vertical suspension linkage on

the vehicle.

        Lastly, the more accustomed the team drivers are to operating the vehicles, the more able

they will be to select the right TLLTD setting for them, as well as giving more detailed feedback

on the vehicle handling and performance characteristics. MIT Motorsports attempts to take

testing trips on a weekly basis when weather permits to increase driver capability and

understanding, as well as testing and tuning various vehicle systems, and a specific focus on the

driver’s sensitivity to suspension parameters could be beneficial to future testing seasons.

       6.2 Related Systems

     As mentioned, vehicle dynamics performance is highly dependent on the performance

and understanding of other systems as well. The aerodynamics system has a substantial impact in

the normal force distribution between tires (as well as the associated friction coefficient), and

changes how the vehicle handles and performs as a function of vehicle speed. Over the 2019 and

2020 seasons, effort was spent to better quantify the vehicle’s overall drag and lift coefficients.

Continuing this progress to achieve even high precision, as well as validating properties on

specific wing elements, will lead to a better understanding of the overall system. Currently, the

team’s aerodynamics system is modeled in Star CCM+, but empirical testing has shown that the

team’s modeled CL and CD values can differ substantially from reality. The more testing and

validation are performed, the better the aerodynamics team can understand how to increase

model accuracy and gain intuition into more extensive aspects of vehicle behavior.

       Validating the control arms force distribution model would also allow the team to gain

confidence in the vehicle’s handling and potentially decrease the mass (by decreasing the factor

of safety) of a system that is highly dependent on suspension geometry. In fall of 2019, an

                                      27
```

<a id="gaither-p028"></a>

#### GAI · PDF página 028 de 030

```text
attempt was made to start validating this model. Strain gauges were placed on all control arms on

the rear right tire of the MY19 vehicle, and the predicted forces (as a function of the vehicle’s

acceleration) were compared to the measure forces in the strain gauges. As shown in Figure 26,

there are some significant deviations between the measured and predicted forces. A more

extensive analysis, including a variety of vehicle driving profiles and strain gauges on all four

corners of the vehicle, could be beneficial to continuing to understand these discrepancies and

gaining confidence in the vehicle’s performance.





  Figure 26: Measured forces in the suspension forces on the rear lower control arm on the rear
                 right tire of the MY19 vehicle during a portion of vehicle testing.

        Finally, an increased focus on streamlining documentation and quantifying trade-offs in

the suspension system and the vehicle will continue to speed up the vehicle design cycle and

leave more time for the team’s spring testing season. Testing trips are incredibly valuable to

teaching drivers how to handle the vehicle, as well as increasing vehicle reliability and being

able to mitigate any recurring vehicle failure before competing. Figure 27 shows the MY18 and

MY19 vehicle on a testing trip in the fall of 2019.





                                      28
```

<a id="gaither-p029"></a>

#### GAI · PDF página 029 de 030

```text
           Figure 27: MY18 (front) and MY19 (rear right) on a testing trip in fall 2019.

      The suspension system (and team structure as a whole) has developed extensively since

the team’s 2017 success, and is a testament to the organization, dedication, and capability of the

team members. By continuing this focus on comprehensive vehicle understanding and validation,

MIT Motorsports will remain as one of the most competitive FSAE teams in the United States,

providing its members with invaluable opportunities to learn, build, and celebrate their

engineering successes.





                                      29
```

<a id="gaither-p030"></a>

#### GAI · PDF página 030 de 030

```text
                                     References

Milliken, W., and Milliken, D., 1995, Race Car Vehicle Dynamics, Society of Automotive
       Engineers.

Smith, C., 1978, Tune to Win: The Art and Science of Race Car Development and Tuning,
       Carroll Smith Consulting Incorporated, Rolling Hills Estates, CA.

2020, “2018-2019 Formula SAE® Rules.” [Online].





                                      30
```

---

<a id="fuente-romano"></a>

## Fuente ROM — Development and Analysis of the Two-Regime Transient Tyre Model for Combined Slip

**Autor(es):** Luigi Romano, Fredrik Bruzelius, Mattias Hjort y Bengt Jacobson. **Año:** 2023. **Idioma:** inglés. **Páginas del PDF:** 37. **Documento:** `530309_Fulltext.pdf`. **Original:** https://research.chalmers.se/publication/530309/file/530309_Fulltext.pdf.

### Índice de páginas de esta fuente

[1](#romano-p001) · [2](#romano-p002) · [3](#romano-p003) · [4](#romano-p004) · [5](#romano-p005) · [6](#romano-p006) · [7](#romano-p007) · [8](#romano-p008) · [9](#romano-p009) · [10](#romano-p010) · [11](#romano-p011) · [12](#romano-p012) · [13](#romano-p013) · [14](#romano-p014) · [15](#romano-p015)

[16](#romano-p016) · [17](#romano-p017) · [18](#romano-p018) · [19](#romano-p019) · [20](#romano-p020) · [21](#romano-p021) · [22](#romano-p022) · [23](#romano-p023) · [24](#romano-p024) · [25](#romano-p025) · [26](#romano-p026) · [27](#romano-p027) · [28](#romano-p028) · [29](#romano-p029) · [30](#romano-p030)

[31](#romano-p031) · [32](#romano-p032) · [33](#romano-p033) · [34](#romano-p034) · [35](#romano-p035) · [36](#romano-p036) · [37](#romano-p037)

### Índice de secciones del documento

- [1. Introduction](#romano-p003) (PDF pág. 3)
- [2. Theory of transient generation of tyre forces](#romano-p005) (PDF pág. 5)
  - [2.1. The single contact point models](#romano-p006) (PDF pág. 6)
- [3. Refined two-regime theory](#romano-p009) (PDF pág. 9)
  - [3.1. The sliding functions](#romano-p010) (PDF pág. 10)
  - [3.2. The slip functions](#romano-p010) (PDF pág. 10)
- [4. Two-regime tyre model for combined slip](#romano-p012) (PDF pág. 12)
  - [4.1. Model analysis](#romano-p014) (PDF pág. 14)
    - [4.1.1. Friction limit](#romano-p015) (PDF pág. 15)
    - [4.1.2. Dissipativity](#romano-p016) (PDF pág. 16)
- [5. Model comparison and validation](#romano-p020) (PDF pág. 20)
  - [5.1. Comparison with single contact point tyre models](#romano-p020) (PDF pág. 20)
  - [5.2. Comparison with experimental data](#romano-p023) (PDF pág. 23)
- [6. Conclusion](#romano-p029) (PDF pág. 29)
- [Notes](#romano-p033) (PDF pág. 33)
- [Acknowledgments](#romano-p033) (PDF pág. 33)
- [ORCID](#romano-p033) (PDF pág. 33)
- [References](#romano-p033) (PDF pág. 33)

### Índice de figuras y tablas localizadas en el texto

- [PDF pág. 11](#romano-p011): Figure 1. A qualitative example of steady-state models for the tangential tyre forces satisfying Assump-
- [PDF pág. 12](#romano-p012): Figure 2 illustrates the sigma functions ˆσ(·) and the function ˆσ(·) in Example 3.2 for
- [PDF pág. 13](#romano-p013): Figure 2. Slip variables expressed as sigma functions σx = ˆσx(Ft), σy = ˆσy(Ft) and σ = ˆσ(Ft) (Fig-
- [PDF pág. 15](#romano-p015): Figure 3 provides a graphical interpretation of this result for different relaxation ratios
- [PDF pág. 17](#romano-p017): Figure 3. Friction circle and maximum attainable steady-state tangential force for a tyre with isotropic
- [PDF pág. 18](#romano-p018): Figure 4. Velocity ﬁeld and invariant sets for a tyre with isotropic tread and anisotropic carcass with
- [PDF pág. 19](#romano-p019): Figure 5. Velocity ﬁeld and invariant sets for a tyre with low relaxation ratio χλ = 0.125 for diﬀerent
- [PDF pág. 20](#romano-p020): Figure 6. Friction circle Cμ together with the relaxation ellipse Eχ and the relaxation circle Cχ for a tyre
- [PDF pág. 22](#romano-p022): Figure 7. Comparison between the two-regime tyre model (solid line), the semi-nonlinear (dashed
- [PDF pág. 23](#romano-p023): Table 1. Tyre parameters.
- [PDF pág. 23](#romano-p023): Figure 7. Indeed, even in the case of pure longitudinal or lateral interactions, Equation (1)
- [PDF pág. 24](#romano-p024): Figure 8. Comparison between the two-regime tyre model (solid line), the semi-nonlinear (dashed line)
- [PDF pág. 25](#romano-p025): Figure 9. Comparison between the two-regime tyre model (solid line), the semi-nonlinear (dashed line)
- [PDF pág. 26](#romano-p026): Figure 10. The VTI friction test vehicle BV12.
- [PDF pág. 27](#romano-p027): Table 2. Tyre parameters optimised from experimental data.
- [PDF pág. 27](#romano-p027): Figure 11. Tyre-slip curve ﬁtted from experimental data. Parameter values: ¯Cσ = 7.6, μ = 0.48.
- [PDF pág. 28](#romano-p028): Figure 12. Transient longitudinal response of the tyre to a time-varying slip input σx (red line) predicted

### Transcripción por página

<a id="romano-p001"></a>

#### ROM · PDF página 001 de 037

```text
Development and analysis of the two-regime transient tyre model for
combined slip



Downloaded from: https://research.chalmers.se, 2026-10-23 15:57 UTC





    Citation for the original published paper (version of record):
   Romano, L., Bruzelius, F., Hjort, M. et al (2023). Development and analysis of the two-regime
    transient tyre model for combined slip. Vehicle System Dynamics, 61(4): 1028-1062.
    http://dx.doi.org/10.1080/00423114.2022.2057335





N.B. When citing this work, cite the original published paper.





research.chalmers.se offers the possibility of retrieving research publications produced at Chalmers University of Technology. It
covers all kind of research output: articles, dissertations, conference papers, reports etc. since 2004. research.chalmers.se is
administrated and maintained by Chalmers Library


                                       (article starts on next page)
```

<a id="romano-p002"></a>

#### ROM · PDF página 002 de 037

```text
    Vehicle System Dynamics
     International Journal of Vehicle Mechanics and Mobility





        ISSN: (Print) (Online) Journal homepage: https://www.tandfonline.com/loi/nvsd20

Development and analysis of the two-regime
transient tyre model for combined slip


Luigi Romano, Fredrik Bruzelius, Mattias Hjort & Bengt Jacobson


To cite this article: Luigi Romano, Fredrik Bruzelius, Mattias Hjort & Bengt Jacobson (2023)
Development and analysis of the two-regime transient tyre model for combined slip, Vehicle
System Dynamics, 61:4, 1028-1062, DOI: 10.1080/00423114.2022.2057335

To link to this article: https://doi.org/10.1080/00423114.2022.2057335




     © 2022 The Author(s). Published by Informa
      UK Limited, trading as Taylor & Francis
       Group.

        Published online: 04 Apr 2022.



       Submit your article to this journal


         Article views: 1583



       View related articles



       View Crossmark data



         Citing articles: 3 View citing articles





                           Full Terms & Conditions of access and use can be found at
             https://www.tandfonline.com/action/journalInformation?journalCode=nvsd20
```

<a id="romano-p003"></a>

#### ROM · PDF página 003 de 037

```text
VEHICLE SYSTEM DYNAMICS
2023, VOL. 61, NO. 4, 1028–1062
https://doi.org/10.1080/00423114.2022.2057335



Development and analysis of the two-regime transient tyre
model for combined slip

Luigi Romano   a, Fredrik Bruzeliusa,b, Mattias Hjortb and Bengt Jacobson   a

aDepartment of Mechanics and Maritime Sciences, Chalmers University of Technology, Gothenburg, Sweden;
bDriver and Vehicle, VTI Swedish National Road and Transport Research Institute, Gothenburg, Sweden



   ABSTRACT                                                         ARTICLE HISTORY
   This paper refines the two-regime transient theory developed by     Received 24 August 2021
  Romano et al. [Romano L, Bruzelius F, Jacobson B. Unsteady-state     Revised 27 December 2021
   brush theory. Vehicle Syst Dyn. 2020;59:11–29. DOI: 10.1080/004231     Accepted 18 March 2022
   14.2020.1774625.] to include the effect of combined slip. A nonlinear
   system is derived that describes the non-steady generation of tyre
   forces and considers the coupling between the longitudinal and lat-
   eral characteristics. The proposed formulation accounts for both the
   carcass and the bristle dynamics, and represents a generalisation of
   the single contact point models. A formal analysis is conducted to
   investigate the effect of the tyre carcass anisotropy on the properties
   of the system. It is concluded that a fundamental role is played by
   the ratio between the longitudinal and lateral relaxation lengths. In
   particular, it is demonstrated that the maximum slip that guarantees
    (partial) adhesion conditions does not coincide with the station-
   ary value and decreases considerably for highly anisotropic tyres.
   The dissipative nature of the model is also analysed using elemen-
   tary tools borrowed from the classic theory for nonlinear systems.
  A comparison is performed against the single contact point mod-
    els, showing a good agreement especially towards the full-nonlinear
   one. Furthermore, compared to the single contact point models, the
   two-regime appears to be able to better replicate the exact dynamics
   of the tyre forces predicted by the complete brush theory. Finally, the
   transient model is partially validated against experimental results.



1. Introduction

Transient tyre behaviour is a crucial aspect in vehicle dynamics, especially in conjunction
with severe braking manoeuvres [1]. ABS functions require, indeed, detailed knowledge of
tyre states to be effective. Another important aspect to be considered relates to friction esti-
mation, for which several algorithms have been developed. These may, however, mispredict
the available friction coefficient if transient phenomena are not accounted for properly in
the generation of tyre forces and moments.
  The transient dynamics of pneumatic and non-pneumatic tyres [2] is a rather complex
phenomenon, which involves several interlinked aspects connected to both the viscoelastic



CONTACT Luigi Romano     luigi.romano@chalmers.se

© 2022 The Author(s). Published by Informa UK Limited, trading as Taylor & Francis Group.
This  is an Open Access article distributed under the terms of the Creative Commons Attribution License (http://creativecommons.
org/licenses/by/4.0/), which permits unrestricted use, distribution, and reproduction in any medium, provided the original work is properly
cited.
```

<a id="romano-p004"></a>

#### ROM · PDF página 004 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1029


properties of the rubber compound and the non-stationary flow of new material that con-
tinuously enters and relinquishes the contact patch [3,4]. An accurate, physically motivated
description capable of capturing the salient features of the tyre dynamics often appears to
be prohibitive, because of the overwhelming number of parameters that must be taken into
account.
   In particular, two antipodal approaches may be identified when dealing with transient
tyre modelling. The first one consists of a systematic, microscopic analysis of the shear
forces developed at the tyre-road interface. Available models explore the phenomenon at
a microscopic level and share a great similarity with the ones also used in the context of
railway dynamics studies. An example is the classic brush theory [5–9], where the defor-
mation of the tyre tread is modelled by using separate bristles, which alternatively stick
to the ground or are in relative motion. The complexity of these formulations – based on
the classic theory for partial differential equations – make them unsuitable for simulation
purposes and the design of control algorithms.
   Different approximated models striving for simplicity have been thus proposed in the lit-
erature and are mainly based on the observation that, under certain circumstances, the tyre
transient behaviour may be well approximated by those of string or spring-like elements.
  The infancy of these models probably dates back to the first studies pioneered by
Schlippe [10] and Segel [11], who employed the so-called stretched-string tyre model to
investigate the non-steady-state response of the tyre to a lateral input [12–19]. Being
grounded on a few physical assumptions and relatively easy to interpret, this approach has
gained vital importance over the years and is still one of the most effective when it comes
to typical situations where the slip parameters are relatively small and the assumption
of vanishing sliding holds. Pacejka [20] perfected the analysis by introducing additional
bristle elements to extend the stretched-string model towards applications in which the
friction limit is exceeded over a finite portion of the contact patch. Typically, enhanced
formulations of this time are much more sophisticated and somehow annoying to deal
with analytically [21]. However, they led to fundamental results that represent the concep-
tual basis for simplified models, like the single contact point ones [22–24]. In this category
also fall some variations that are grounded directly on the simpler brush theory [1,25–29].
In any case, these models are all based on the assumption that the tyre dynamics may be
described by a first-order dynamic model, whose main parameter is the so-called relax-
ation length, that is the distance that the tyre needs to travel to develop the 63% of the
steady-state forces. In this approach, the transient properties of the tyre force generation
are then justified by the presence of an additional deformable element, the tyre carcass,
which is held accountable for the whole nonstationary process. In this way, the dual nature
of the tyre is mimicked by a series system which behaves as a spring at low rolling speed
and as a damper at high speeds. This pragmatic approach leads to a very straightforward
model, which generally shows a good agreement with experimental evidence and may be
combined with complex analytical expressions for the tyre forces [30]. This legitimates the
ubiquitous presence of the single point contact models when it comes to vehicle dynam-
ics simulations. Other formulations, which are based on a similar principle, but introduce
higher-order dynamics, are also available in the literature [31,32].
  The LuGre friction model [33] has also been proved to be able to replicate the transient
behaviour of tyres with high accuracy. The LuGre formulation, in its distributed form, clas-
sifies as an enhanced version of the classic brush theory, and accounts for more complex
```

<a id="romano-p005"></a>

#### ROM · PDF página 005 de 037

```text
1030          L. ROMANO ET AL.


phenomena, such as the Stribeck effect. Starting from the distributed form, some authors
have derived approximated models, based on ordinary differential equations (ODEs), to
describe the nonstationary generation of tyre forces [34–42]. This formulation is easier to
implement in conventional simulation environments, and is more adequate when it comes
to controlling purposes.
   Recently, an alternative approach has been proposed by Romano et al. [5] and formalised
in the so-called two-regime theory. Accordingly, a dynamic model is derived as a functional
interpolation between two sets of equations that describe the approximated relationships
between the generalised forces and the slip variables at low and high rolling speeds, respec-
tively. The two-regime models consider the transient phenomena connected to the bristle
and the tyre carcass dynamics, and represent a generalisation of the single contact point
model. However, a major drawback relates to the intrinsic difficulties connected to this
approach, which requires inverting the force-slip mappings. This may be often possible
only locally, and therefore pose stringent limitations on the general validity of the method.
In Romano et al. [5], the derivation of the two-regime tyre models was hence restricted to
the case of pure lateral (or longitudinal) slip. In this paper, the authors extend the original
formulation to account for the effect of combined translational slips.
  The paper is structured as follows: Section 2 recapitulates the salient properties of the
brush models and gives a brief overview of the single contact point models. In Section 3,
a refined version of the two-regime theory is presented and the notions of sliding and
slip functions are introduced. The nonlinear model for combined slip is then derived in
Section 4. The system is analysed with respect to input-to-state stability and the dissipa-
tivity. A comparison against the single contact point models is performed in Section 5,
showing a good match between the two formulations (Section 5.1). Validation towards
experimental results is then carried out in Section 5.2 considering a series of longitudinal
manoeuvres. Finally, the main conclusions of the paper are summarised in Section 6.


2. Theory of transient generation of tyre forces

The brush theory represents the mathematical foundation for both the single contact point
models and the LuGre-based ones. Therefore, this section introduces the governing par-
tial differential equations (PDEs) of the brush models to the extent that is necessary to
understand the paper. A more comprehensive discussion may be found, for example, in
[1,6,7,20].
  The rolling contact between the tyre and the road takes place inside a small area, called
contact patch, and denoted here with P. In the brush theory, it is assumed that P is a
compact, convex subset of R2, in which the governing PDEs of the model are prescribed.
When the camber angle and the turning spin are sufficiently small [6–8], these may be
formulated as follows:

                                                  ∂ut(x, t)         ∂ut(x, t)
        vs(x, t) = −Vr(t) σ(t) + Aϕ(t)x + ˙δt(t) +        −Vr(t)                  ,
                                                      ∂t            ∂x
                   (x, t) ∈P × R>0.                                                   (1)

In Equation (1), vs(x, t) = [ vsx(x,t) vsy(x,t) ]T is the micro-sliding velocity, that is the relative
velocity between each point of the contact patch and the road, ut(x, t) = [ ux(x,t) uy(x,t) ]T is
```

<a id="romano-p006"></a>

#### ROM · PDF página 006 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1031

the planar displacement of the bristle inside the contact patch P, δt(t) = [ δx(t) δy(t) ]T is
the deformation of the tyre carcass (modelled as a linear spring), Vr(t) is the rolling speed,
σ(t) = [ σx(t) σy(t) ]T is the slip vector and Aϕ(t) is the spin tensor, defined as [7]

                                    0   −ϕ(t)
                           Aϕ(t) =                              ,                             (2)
                                        ϕ(t)    0

where ϕ(t) is the spin variable.
  The above PDEs come equipped with a boundary (BC) and initial condition (IC):

               BC :   ut(x, t) = 0,   (x, t) ∈L × R>0,                      (3)
                    IC :   ut(x, 0) = ut0(x),  x ∈P.                            (4)

where L  is the leading edge and collects the point of the boundary ∂P of the contact
patch where the bristles enter P.
  Equation (1) is valid until the tangential shear stress qt(x, t) = Ktut(x, t) acting at the
coordinate x is smaller or equal than the corresponding friction limit μqz(x, t), where μ
is the friction coefficient and qz(x, t) the vertical pressure acting inside the contact patch.
The operator Kt ∈R2×2 is a positive definite matrix, usually assumed to be diagonal. As
soon as the friction limit is exceeded, indeed, the displacement of the bristle becomes

                                                       t)                   ut(x, t) = −K−1t  μqz(x, t)vs(x,  ⇐⇒vs(x, t) ̸= 0,                (5)
                                               vs(x, t)

with vs(x, t) ≜  vs(x, t)  2.
  Owing to the complexity of the problem, a general solution to Equations (1) and (5)
in transient conditions is often prohibitive, and might only be attempted numerically. In
steady state, when the partial derivative ∂ut(x, t)/∂t vanishes or may be neglected, Equa-
tions (1) and (5) provide a set of relationships between the tangential forces and moment
acting in the contact patch and the slip and spin variables in the form

             Ft(σ, ϕ) =      qt(x; σ, ϕ) dx,                                        (6a)
               P

          Mz(σ, ϕ) =      (x + δx)qy  x; σ, ϕ −(y + δy)qx  x; σ, ϕ dx.         (6b)
               P

In the single contact point models, the steady-state relationships according to Equa-
tions (6a) resulting from the integration over P play the role of a static nonlinearity that
is used to model the tyre forces and moment when subjected to a transient slip.


2.1. The single contact point models

This approach was developed by Higuchi in his excellent dissertation [22] and the subse-
quent paper [23], and derives directly from the more sophisticated stretched-string and
bare-string models, as discussed before. At the time, the effort was towards simplified
models characterised by a low computational burden to be employed in vehicle dynam-
ics simulations. The mathematical treatment of the single contact point models presented
in this paper mainly refers to the books by Pacejka [8] and Guiggiani [1].
```

<a id="romano-p007"></a>

#### ROM · PDF página 007 de 037

```text
1032          L. ROMANO ET AL.


  The overall idea behind the single contact point models is to replace the translational
slip variable σ with the transient slip vector

                                                               ˙δt(t)
                         σ ′(t) ≜σ(t) −                                      (7)                                                 Vr(t),

to account for the additional term due to the carcass deflection. A relationship between the
latter and the planar forces is also postulated in the form

                                     Ft(t) = C′δt(t),                                  (8)

where the carcass stiffness matrix C′ is given by

                                       C′xx   C′xy                                C′ =                                                                                              .                                 (9)
                                       C′yx   C′yy

The transient slip vector is then used in place of the conventional one in the steady-state
expressions for the tangential forces obtained integrating over P as in Equation (6a),
i.e. Ft = Ft(σ ′, ϕ). Differentiating the planar force vector with respect to the time and
combining Equation (7) with (8) yields

                             T        ∂Ft σ ′, ϕ
   ˙Ft σ ′(t), ϕ(t) = ∇σ ′Ft σ ′, ϕ    ˙σ ′(t) +               ˙ϕ(t) = Vr(t)C′ σ(t) −σ ′(t)  .
                                   ∂ϕ
                                                                                  (10)
Further simplifications allow to define the concept of the so-called relaxation lengths. In
general, the quantities relating to ϕ are almost irrelevant for the transient generation of tyre
forces and may hence be disregarded [1]. Premultiplying by S′ = C′−1 gives

                       S′ ˜Cσ σ ′(t), ϕ(t)  ˙σ ′(t) = Vr(t) σ(t) −σ ′(t)  ,                (11)

where the matrix of the generalised tyre stiffnesses is defined as

                                 ˜Cσ σ ′, ϕ ≜∇σ ′Ft σ ′, ϕ T .                         (12)

The above equation (11) may be recast more conveniently as

                         ˜                      σ σ ′(t), ϕ(t)  ˙σ ′(t) + Vr(t)σ ′(t) = Vr(t)σ(t),                (13)

where the matrix ˜ σ(σ ′, ϕ) of the generalised relaxation lengths is defined as

                                                         ˜λxσx σ ′, ϕ     ˜λxσy σ ′, ϕ                   ˜                 σ σ ′, ϕ ≜S′ ˜Cσ σ ′, ϕ =                                                    .          (14)
                                                          ˜λyσx σ ′, ϕ     ˜λyσy σ ′, ϕ

The transient slip calculated by means of Equation (13) is used as input to a steady-state
model for the tyre forces. This variant of the single contact point model is referred to as
full-nonlinear, since the matrix ˜ σ(σ ′, ϕ) is itself a function of σ ′. In practical applications,
the matrix of the generalised stiffnesses is approximated with the conventional one Cσ =
```

<a id="romano-p008"></a>

#### ROM · PDF página 008 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1033

˜Cσ(0, 0), and hence the matrix  ˜ σ becomes constant, that is  ˜ σ(σ ′, ϕ) ≡   σ, with   σ
reading

                     λxσx   λxσy        S′xxCxσx + S′xyCyσx   S′xxCxσy + S′xyCyσy       σ = S′Cσ =        =                                                                           .     (15)
                      λyσx   λyσy        S′yxCxσx + S′yyCyσx   S′yxCxσy + S′yyCyσy

This model variation is often referred to as semi-nonlinear single contact point. Additionally,
if the stiffness matrix of the tyre carcass is diagonal, that is

                                        C′x   0                                C′ =                                                                                            ,                               (16)
                                    0   C′y

the matrix   σ further reduces to
                          ⎡       ⎤
                                               Cxσx   Cxσy
                                λxσx   λxσy          C′x      C′x                         σ =        =                                    (17)                                                        Cyσy ⎥⎥⎥⎦.                                 λyσx   λyσy           ⎢⎢⎢⎣ Cyσx
                                                               C′y      C′y

Up to this point, it should be acknowledged that the single contact point models well
approximate the real dynamics of a tyre in transient slip conditions. However, from a pure
conceptual viewpoint, this approach is wrong. Indeed, the transient of the bristles is always
neglected in this case, despite being an essential part of the transient process of forces
and moments generation. Furthermore, the single point contact models do not explain
the empirical and theoretical presence of transient phenomena in e.g. railway applications,
where the wheel may be approximated as a rigid body to some extent.
    It is quite simple, in fact, to show that disregarding the transient connected to the deflec-
tion of the bristles automatically implies steady-state conditions for the tangential forces
exerted at the tyre-road interface. Indeed, under reasonable assumptions, the time deriva-
tives of the planar force vector Ft(t) and the self-aligning moment Mz(t) in Equations (6a)
read, respectively:

                                    ∂qt(x, t)                             ˙Ft(t) =               dx,                                (18a)
                  P    ∂t
                                     ∂qy(x, t)     ∂qx(x, t)
                        ˙Mz(t) =     x      −y          dx.                (18b)
                  P     ∂t           ∂t
Clearly, since Ft(t) = C′δt(t), from Equation (18a), it is immediate to infer that

                                            ˙δt(t) = S′ ˙Ft(t) = 0,                             (19)

and therefore also ˙Mz(t) = 0. The above results demonstrate the fundamental role played
by the transient deflection of the bristle in the nonsteady-state generation of tyre forces. It
seems thus reasonable to account for this effect even when approaching a simplified model
to describe nonstationary processes related to tyre dynamics. The spontaneous question
is how to derive a consistent set of equations to model the phenomenon properly. The
solution proposed by the authors is presented in the next Section 3 and represents a further
generalisation of that already sketched in Romano et al. [5].
```

<a id="romano-p009"></a>

#### ROM · PDF página 009 de 037

```text
1034          L. ROMANO ET AL.


3. Refined two-regime theory

The main scope of this paper consists of the derivation and the analysis of the two-regime
model for the case of combined translational slips. The investigation is conducted from
the perspective of nonlinear systems. To this end, the general idea behind the theory intro-
duced by Romano et al. [5] is succinctly recapitulated. In this paper, however, an alternative
and more comprehensive approach to derive the governing equations of the two-regime
model is proposed.
  To start, the existence of two implicit representations is assumed of the form:

                                ˇϒ  Vrσ, Vrϕ, ˙Ft, ˙Mz = 0,                         (20a)

                             ˆϒ(σ, ϕ, Ft, Mz) = 0.                              (20b)

The above equations (20) postulate certain relationships between the slip variables and
tyre characteristics acting at the tyre contact patch. Ideally, they should approximate the
tyre dynamics at low and high values of the rolling speed.
  From Equations (20), it is possible to locally express the variables (σ, ϕ) invoking the
Implicit Function Theorem. In particular, Equation (20a) gives

                                  1
                          σ =      ˇσ   ˙Ft, ˙Mz   ,                            (21a)
                                 Vr
                                  1
                         ϕ =     ˇϕ   ˙Ft, ˙Mz   ,                           (21b)
                                 Vr
where the functions ˇσ(·, ·) = [ ˇσx(·,·) ˇσy(·,·) ]T and ˇϕ(·, ·) are called sliding functions. On the
other hand, Equation (20b) yields

                            σ = ˆσ(Ft, Mz),                              (22a)
                          ϕ = ˆϕ(Ft, Mz),                             (22b)

in which ˆσ(·, ·) = [ ˆσx(·,·) ˆσy(·,·) ]T and ˆϕ(·, ·) are referred to as slip functions. It is worth
pointing out that Equations (21) and (22) are valid in turn for low and high values of Vr.
   Interpolating between Equations (21) and (22) yields the two-regime tyre models:

                            1
                     σ =      ˇσ   ˙Ft, ˙Mz + ˆσ(Ft, Mz),                      (23a)
                            Vr
                            1
                    ϕ =     ˇϕ   ˙Ft, ˙Mz + ˆϕ(Ft, Mz),                     (23b)
                            Vr
Equations (23) allow to express the slips (σ, ϕ) as a weighted combination of two differ-
ent vector-valued functions: the first captures the tyre behaviour at low rolling speed, the
second one, instead, gives an approximation at high rolling speeds. They also provide a set
of ODEs for the tyre characteristics. If the sliding functions have an isolated equilibrium
at the origin, it is clear that the equilibria of the ODEs (23) coincide with the steady-state
relationships for the tyre forces and moment.
  The main limitation of this approach consists in the fact an explicit representation of
ˇϒ(·, ·, ·, ·) and ˆϒ(·, ·, ·, ·) might often be obtained only locally.
   In the subsequent subsections 3.1 and 3.2, respectively, the sliding and slip functions
will be derived.
```

<a id="romano-p010"></a>

#### ROM · PDF página 010 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1035


3.1. The sliding functions

At low, speed, the sliding functions may be sought as follows:
              ⎡      ⎤
                                        ˇσ   ˙Ft, ˙Mz          S′     S′
                                                                                                                ,                    (24)                           ⎢⎣                   F   M       ˙Ft                          ⎥⎦=   S′                                          ϕF   S′ϕM    ˙Mz                                      ˇϕ   ˙Ft, ˙Mz

where the matrix on the right-hand side is nonsingular, and S′F, S′M, S′ϕF and S′ϕM represent
generalised compliances. The derivation of Equation (24) is detailed in Appendix 1. The
system described by Equation (24) is linear, and, as already anticipated, obviously admits
a unique equilibrium in the origin.
   Furthermore, when the spin slip ϕ and the self-aligning moment Mz are neglected,
Equation (24) reduces to a relationship between the translational sliding functions and
the tangential forces in the form

                                       1
                                      ˇσ   ˙Ft = S′F ˙Ft =     K−1t + S′   ˙Ft,                    (25)
                         AP

where AP is the area of the contact patch P.

Example 3.1: For an tyre with isotropic tread, rectangular contact patch and diagonal
stiffness matrix for the tyre carcass C′, the matrix S′F reads specifically
                  ⎡             ⎤
                                aC′x + Cσ                                            0
                                    C′xCσ                               S′                                                                                  (26)                            F =                                                                         ⎢⎢⎢⎣                                           aC′y + Cσ ⎥⎥⎥⎦,
                                 0
                                                 C′yCσ

where Cσ = 4a2bk, a and b are the contact patch semilength and semiwidth, respectively,
and k is the bristle stiffness.

  For convenience, the following Assumption 3.1 is retained in the remaining of the paper.

Assumption 3.1: The matrices Kt ∈R2×2 and C′ ∈R2×2 are symmetric and positive
definite.

  From Equation (25), it may be inferred that the above Assumption 3.1 ensures that S′F
is also positive definite.


3.2. The slip functions

In steady-state conditions, the explicit relationships between the tangential forces and the
self-aligning moment and the slip variables are often known in the form

                                    Ft = ˆFt(σ, ϕ),                               (27a)
                        Mz = ˆMz(σ, ϕ),                             (27b)
```

<a id="romano-p011"></a>

#### ROM · PDF página 011 de 037

```text
1036          L. ROMANO ET AL.

where ˆFt(σ, ϕ), ˆMz(σ, ϕ) are obtained by integration over P as in Equation (6) or
using empirical formulae1. Inverting Equations (27) yields a local representation of the
translational slip σ and spin ϕ in the form of slip functions (ˆσ, ˆϕ).
   This paper restricts to the case of pure translational slip, that is disregards the (usually
small) contribution due to spin. Furthermore, it considers steady-state relationships as in
Assumption 3.2.

Assumption 3.2: In steady-state conditions, the tyre forces may be described by a rela-
tionship of the type
                                Ft = ˆFt(σ) = ˆFt(σ)σ ,                            (28)
                                        σ
where ˆFt(σ) ≜  ˆFt(σ)  and σ ≜∥σ∥2. Furthermore, it is assumed that ˆFt ∈K when σ
                           2
is restricted to the interval [0, σ peak), for some value σ peak > 0.

   Tyre functions of the same type as in Equation (28) usually describe the forces when
the tread is assumed to be isotropic. According to Assumption 3.2, Equation (28) is also
invertible in the range σ ∈[0, σ peak), where σ peak corresponds to the value for which Ft ≜
∥vt∥2 attains its maximum value, that is ˆFt(σ peak) = Fmaxt   ≜μpeakFz. The values σ peak is
generally smaller than the critical slip value σ cr that causes total sliding inside the contact
patch, as shown in Figure 1.





Figure 1. A qualitative example of steady-state models for the tangential tyre forces satisfying Assump-
tion 3.2. Figure 1(a) illustrates a tyre surface Ft = ˆFt(σ) according to Equation (28), with its components
projected in longitudinal and lateral directions. Figure 1(b) exempliﬁes two diﬀerent tyre models: a sim-
pliﬁedversionofPacejka’sMagicFormula(blueline)andabrushmodelwithdiﬀerentstaticanddynamic
friction coeﬃcients (red line). The models share the same peak value μpeakFz for σ = σ peak; however,
whilst the brush models predict total sliding in the contact patch for σ > σ cr, Pacejka’s Magic Formula
does not. (a) Tyre force surface Ft = ˆFt(σ) according to Equation (28) with projections Fx and Fy for
σx = σy. (b) Two steady-state tyre models satisfying Assumption 3.2.
```

<a id="romano-p012"></a>

#### ROM · PDF página 012 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1037


  The inverse relationship of Equation (28) may be found as
                 σ = ˆσ(Ft) = ˆσ(Ft)Ft ,   Ft ∈  0, μpeakFz   ,                  (29)
                                            Ft
being ˆσ(·) ≜ˆF−1t  (·) also a K-class function. Often, the expression for the function ˆσ(·)
cannot be derived in closed-form. Analytical approximations may be, however, constructed
using fixed-point theorems [43]. There are, of course, exceptional cases in which ˆσ(·) may
be easily determined.

Example 3.2: According to the brush models, for an isotropic tyre with a rectangu-
lar contact patch and unique friction coefficient, the tangential forces depend upon the
translational slip as follows:

                                            1   σ   2        Ft = ˆFt(σ) = ˆFt(σ)σ = Cσσ  1 −σ +                       ,  σ ∈[0, σ cr),    (30)
                      σ           σ cr   3  σ cr

where σ cr ≜3μFz/Cσ. It should be noticed that, for the model in Equation (30), σ peak ≡
σ cr. Accordingly for larger values of the slip, i.e. σ ≥σ cr, the tangential force equals the
peak value μpeakFz ≡μFz, and is oriented as the slip vector, that is Ft = μFzσ/σ. The
inverse relationship of Equation (30) reads [44]
                    ⎛        ⎞
       σ = ˆσ(Ft) = ˆσ(Ft)Ft = σ                                                  cr ⎝1 −  3 1 −Ft ⎠Ft ,   Ft ∈[0, μFz),      (31)
                               Ft                μFz   Ft

where μpeak ≡μ (the peak friction coefficient equals the actual one). Clearly, the function
ˆσ(·) in the above equation (31) is of the type of that in Equation (29).

   Figure 2 illustrates the sigma functions ˆσ(·) and the function ˆσ(·) in Example 3.2 for
the case of pure translational slip.


4. Two-regime tyre model for combined slip

Taking advantage of the theoretical framework developed in Section 3, this paper now
moves to derive and analyse the nonlinear system describing the transient tyre dynamics
at combined slip. Plugging Equations (26) and (31) into (23a) yields the two-regime tyre
model for combined slip:

                                ˙Ft(t) = Vr(t)C′σ σ(t) −ˆσ  Ft(t)    ,                      (32)

where the matrix C′σ is defined as
                            C′xσx   C′xσy                                             C′σx   0                    C′                      =                       σ =                                           ≜S′−1F   .                 (33)                            C′yσx   C′yσy       0    C′σy

The above equation may be reinterpreted in terms of enhanced relaxation lengths premul-
tiplying both sides by     ′σ ≜CσS′F. This yields

                                                    ′                          σ ˙Ft(t) = Vr(t)Cσ σ(t) −ˆσ  Ft(t)    ,                    (34)
```

<a id="romano-p013"></a>

#### ROM · PDF página 013 de 037

```text
1038          L. ROMANO ET AL.





Figure 2. Slip variables expressed as sigma functions σx = ˆσx(Ft), σy = ˆσy(Ft) and σ = ˆσ(Ft) (Fig-
ures 2(a,b) and (c), respectively) as functions of the longitudinal and lateral tyre force given as in
Equations (31). Tyre parameters: Cσ = 30, 000 N, μ = 1, σ cr = 0.3, Fz = 3000 N.


where the matrix
                                             λ′     λ′                                                                      ′        xσx     xσy
                                    σ =                                           (35)                                             λ′     λ′                                                   yσx     yσy

collects the enhanced relaxation terms. These relaxation lengths are referred to as enhanced
because they also account for the transient of the bristles in the contact patch [5]. A fun-
damental role in the transient generation of the tyre forces is played by the relaxation ratio
χλ, defined as
                                      λmin C′σ
                             χλ =                    ,                               (36)
                                    λmax  C′σ
where λmax(·) and λmin(·) denote the maximum and minimum eigenvalues of a matrix,
respectively. By Assumption C′σ is symmetric and positive definite, and therefore λmax(C′σ)
and λmin(C′σ) in Equation (36) are always positive real.

Example 4.1: Combining the results from Examples 3.1 and 3.2, the matrices C′σ becomes
                       ⎡             ⎤
                                           C′xCσ                                                   0
                             0                         C′                                               aC′x + Cσ                                 σx                 C′                    =                                                                                  (37)                     σ =                        0                               C′                                                                                            ⎢⎢⎢⎣                                                       C′yCσ     ⎥⎥⎥⎦,                                          σy                                        0
                                                              aC′y + Cσ

where it has been renamed C′σx ≜C′xσx and C′σy ≜C′yσy without ambiguity. Accordingly,
the matrix of enhanced relaxation lengths reads
                          ⎡      ⎤
                                       Cσ
                                                0
                                                      ′      λ′σx   0          C′σx                       =                                                                                  (38)                           σ =                                                                                                         ⎢⎢⎢⎣                                            Cσ  ⎥⎥⎥⎦,                             0    λ′σy                                           0
                                                                      C′σy
```

<a id="romano-p014"></a>

#### ROM · PDF página 014 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1039

where again it has been renamed λ′σx ≜λ′xσx and λ′σy ≜λ′yσy, and the slip function ˆσ(·)
reading as in Equation (31). In this case, the relaxation ratio may also be interpreted as
                           χλ ≡C′min ≡λ′min ,                             (39)
                                         C′max    λ′max

where C′max ≜max{C′σx, C′σy} and C′min ≜min{C′σx, C′σy}. Similarly, λ′max ≜max{λ′σx, λ′σy}
and λ′min ≜min{λ′σx, λ′σy}. Therefore, the relaxation ratio χλ condenses the two different
relaxation behaviours of the tyre in longitudinal and lateral direction. Usually, C′x > C′y,
which also implies λσy > λσx.


4.1. Model analysis

Now the authors proceed to the formal analysis of the two-regime model. There are two
aspects to investigate. The first one relates to the trajectories of the system described by
Equation (32) in presence of finite slip, which will be treated as a given input. Ideally,
the model should be able to replicate the transient behaviour of the tyre during nomi-
nal operational conditions. For the steady-state model, this translates into the requirement
Ft < μpeakFz, which is equivalent to σ < σ peak. In transient conditions, it is not obvi-
ous that the fulfillment of sups σ(s) < σ peak also implies Ft(s) < μpeakFz. Therefore, an
equivalent transient critical value for the slip should be estimated.
  The second aspect concerns the dissipative nature of the model. Finite slip causes losses
at the tyre contact patch, known as slip losses. Intuitively, the power generated by the slip
should not be expected to be entirely converted into useful tangential forces.
  A major issue with the system described by Equation (32), however, connects to the pres-
ence of the term Ft appearing in the denominator. This is responsible for some difficulties
encountered in simulation, since the situation Ft = 0 is not handled properly. To overcome
this issue, the original sigma functions ˆσ(·) in Equation (32) may be conveniently replaced
with
                                                  Ft
                                               ˆσ ϵ(Ft; ϵ) ≜ˆσ(Ft)            ,                           (40)
                                                    Ft + ϵ
where ϵ ∈R>0 is an arbitrary small constant. In the above equation (40), the constant ϵ
in ˆσ ϵ(·; ϵ) should be regarded as a parameter. Owing to Equation (40), a modified model
may be considered as

                             ˙Ft(t) = Vr(t)C′σ  σ(t) −ˆσ ϵ  Ft(t); ϵ     .                    (41)

For what follows, it may also be useful to introduce the function ˆσϵ(·; ϵ) defined as

                                                      Ft
                                     ˆσϵ(Ft; ϵ) ≜ˆσ(Ft)            .                           (42)
                                                    Ft + ϵ

It should be observed that Assumption 3.2 implies that ˆσϵ(·; ϵ) in Equation (42) is also
a K-class function. The subsequent analysis will be therefore conducted concerning the
modified model described by Equation (41) rather than the original one. The latter
represents a particular case of the former for ϵ →0+.
```

<a id="romano-p015"></a>

#### ROM · PDF página 015 de 037

```text
1040          L. ROMANO ET AL.


4.1.1. Friction limit
In analogy to the steady-state model, an upper bound for the slip input that guarantees that
the total tangential force never reaches the friction limit, that is Ft < μpeakFz, should be
estimated. To proceed, the notion of friction circle is formalised as the set

                     Cμ ≜  Ft ∈R2  Ft ≤μpeakFz    ,                       (43)


                               ◦                                           ◦
whose interior is denoted by C μ. The idea is now to understand when C μ is an invariant
                                                                         ◦
set for Ft(t), that is which pairs of initial conditions Ft0 = Ft(t0) ∈ C μ and inputs σ(t)
                                     ◦
ensure Ft(t) to stay indefinitely in C μ for all t ≥t0. Sufficient conditions are stated in the
following Theorem 4.1.

Theorem 4.1: Assume that σ(t) satisfies

                                                              cr       1                    sup σ(t) < σχ ≜ψχλσ                                  ,                  (44)
                           t≥t0                   1 + ϵ/ μpeakFz

                                             ◦                 ◦
where ψ ∈(0, 1) is arbitrary small. If Ft0 ∈ C μ, then Ft(t) ∈ C μ for all t ≥t0. Furthermore,
the system described by Equation (41) is input-to-state stable2 with γ  : [0, σχ) × R>0 →
[0, μpeakFz) given by
                 ⎧
                                 ⎪⎨0,             σ = 0,
                  γ (σ; ϵ) ≜        σ                                       (45)
                                 ⎪⎩σϵ−1             ; ϵ    ,  σ ∈  0, σχ   ,
                            ψχλ

and satisfying

                                        σ
                            lim γ (σ; ϵ) = ˆFt                 .                         (46)
                        ϵ→0+           ψχλ

Proof: The proof is given in Appendix 2.                       ■

  The above Theorem 4.1 provides an upper bound σχ on the slip input. As it may be
observed from Equation (44), σχ depends on three parameters: ψ, ϵ and χλ. The first two
do not deserve particular attention, since they may be chosen arbitrarily. On the other
hand, the relaxation ratio χλ is a structural parameter and accounts for the anisotropy of
the tyre carcass. Specifically, according to Equation (44), lower values of χλ correspond to
smaller admissible inputs σχ, above which full sliding may occur inside the contact patch.
   Figure 3 provides a graphical interpretation of this result for different relaxation ratios
χλ for a tyre with isotropic tread and rectangular patch (model of Example 4.1, with
μpeak ≡μ and ˆσ(·) replaced by ˆσ ϵ(·; ϵ)) with critical slip σ peak ≡σ cr = 0.3. In the left-
hand side plots, the circles depicted in red represent the friction circle Cμ, whilst the
                                                                                    ◦
green area the sets of attainable steady-state forces for σ < σχ, that is Ft(t) ∈ C χ, with
Cχ ≜{Ft ∈R2 | Ft ≤σϵ−1 (σχ; ϵ)}. The set Cχ is defined here as the relaxation circle. On
the right-hand side of Figure 3, the corresponding maximum values Ft(σχ) are shown,
```

<a id="romano-p016"></a>

#### ROM · PDF página 016 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1041


which amount to the 70.4, 92.1 and 98.44% of the peak force. It is worth noticing that,
for a tyre with isotropic carcass, in general σχ ≡σ peak when ϵ = 0 and ψ →1. The same
condition holds for the one-dimensional models already presented in [5], for which the
parameter ϵ is not needed.
  When the slip input exceeds σχ, it is not obvious that Ft(t) < μpeakFz. Again for the
isotropic model in Example 4.1 with μpeak ≡μ, this is illustrated in Figure 4 for a constant
value of the total slip σ = σ cr, with σx = σy > 0, together with the plot of velocity field of
Equation (41). The red circles represent the friction circle Cμ, whilst the dashed boundary
is the locus of points for which the derivative of the Lyapunov function V(Ft(t)) ≜12F2t (t)
vanishes. Basically, the inner regions enclosed by the dashed curves are invariant sets for
Ft(t), and, therefore, every circle that contains these regions is also an invariant set. It may
be clearly noticed that, for very small values of the relaxation ratio (χλ = 0.125), the dashed
                             ◦                                               ◦
curve lies partially outside C μ, and therefore the trajectories starting in C μ may happen
         ◦
to leave C μ, implying Ft(t) ≥μFz for some finite time t.
  Other qualitative considerations may be drawn by looking at Figure 5, where the invari-
ant sets are plotted for a tyre with relaxation ratio χλ = 0.125 (λ′σy > λ′σx) for different
directions θ of the critical slip value (σx = σ cr cos θ, σy = σ cr sin θ). In particular, it may be
deduced that, when the slip is oriented in the same direction for which the relaxation length
                                                                         ◦
is larger, the invariant set becomes smaller and the risk of leaving C μ decreases. More
specifically, Figure 5(d) is particularly interesting, since it shows that, when the slip has
only lateral component, that is σy = σ cr, the invariant set is completely contained within
Cμ. Again, this is a consequence of the anisotropic nature of the tyre carcass.
   In reality, the requirement on supt≥t0 σ(t) imposed by Theorem 4.1 is quite stringent
and might be relaxed if different maximum values are allowed depending on the slip
direction. In fact, defining the Lyapunov function V(Ft(t)) ≜12F2t (t) as in Appendix 2,
and assuming C′σx > C′σy (λ′σy > λ′σx) as in common practice, it is possible to deduce
immediately that any value of σ(t) such that

                            ∗                       1
                σ(t) < σ χ  θ(t) ≜σχ "                                        (47)
                                                          2                 2
                                         cos θ(t) + χλ sin θ(t)

ensures that ˙V(Ft(t)) is negative definite at time t. From Equation (47), it may be inferred
that, when σx = σ cos θ(t) = 0, the maximum slip in lateral direction coincides with the
critical one (for ϵ →0+). Vice versa, when σ(t) has no lateral component, the maximum
slip is again given by Equation (44) in Theorem 4.1. The fundamental conclusion is that,
in transient conditions, the anisotropy of the tyre carcass maps the friction circle Cμ into a
relaxation ellipse Eχ = {Ft ∈R2 | Ft ≤σϵ−1 (σ χ(θ);∗     ϵ)}. The relaxation set Cχ may be then
defined as the largest circle completely enclosed in Eχ, and σχ ≡supθ σ χ(θ),∗    as illustrated
graphically in Figure 6.

4.1.2. Dissipativity
Since the model is dynamic, there may be different phases in which energy is stored
and released, alternatively. It may be proved that the map X  : Vrσ →Ft is dissipative
for the model, and even more specifically passive .3 This statement may be reformulated
mathematically in the following Property 4.1.
```

<a id="romano-p017"></a>

#### ROM · PDF página 017 de 037

```text
1042          L. ROMANO ET AL.





Figure 3. Friction circle and maximum attainable steady-state tangential force for a tyre with isotropic
tread and anisotropic carcass with diﬀerent relaxation ratios χλ: (a) χλ = 0.33; (b) χλ = 0.57; (c)
χλ = 0.75.
```

<a id="romano-p018"></a>

#### ROM · PDF página 018 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1043





Figure 4. Velocity ﬁeld and invariant sets for a tyre with isotropic tread and anisotropic carcass with
diﬀerent relaxation ratios χλ. σ = σ peak ≡σ cr, σx = σy > 0: (a) χλ = 0.125, (b) χλ = 0.25, (c) χλ =
0.33, (d) χλ = 0.57.


Property 4.1 (Passivity): The map X  : Vrσ →Ft is dissipative (passive) with respect to
the storage function V(Ft(t)) ≜12S′FFt(t) · Ft(t).

Proof: Recalling that C′σ ≜S′−1F   , it follows directly from Equation (41) that

              Vr(t)σ(t) · Ft(t) = S′F ˙Ft(t) · Ft(t) + Vr(t)ˆσ ϵ  Ft(t)  · Ft(t)
                      ≥S′F ˙Ft(t) · Ft(t) = ˙V  Ft(t)  .                      (48)

                                           ■
```

<a id="romano-p019"></a>

#### ROM · PDF página 019 de 037

```text
1044          L. ROMANO ET AL.





Figure 5. Velocity ﬁeld and invariant sets for a tyre with low relaxation ratio χλ = 0.125 for diﬀerent
values of the critical slip direction: (a) θ = 0◦, (b) θ = π6 , (c) θ = π3 , (d) θ = π2 .



   Since the storage function V(Ft(t)) defined in Property 4.1 represents the elastic energy
stored in the tyre carcass, the result above has a clear physical meaning: the power dis-
sipated at the tyre contact patch is always greater than the variation in elastic energy
accumulated in the tyre carcass. In particular, Equation (48) holds with strict inequality for
any Ft(t) ̸= 0. From these considerations, it may be deduced that, in transient conditions,
a finite slip input is not converted completely, and instantaneously, into tangential force,
as it happens in the steady-state case. Therefore, the difference between Vr(t)σ(t) · Ft(t)
and ˙V(Ft(t)) may equivalently be interpreted as a power dissipation taking place inside
the carcass. This might appear counter-intuitive at first instance, since the tyre carcass is
modelled as an elastic element, and is due to the fact that, in transient conditions, the tyre
```

<a id="romano-p020"></a>

#### ROM · PDF página 020 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1045





Figure 6. Friction circle Cμ together with the relaxation ellipse Eχ and the relaxation circle Cχ for a tyre
with isotropic tread and anisotropic carcass (χλ = 0.33). The tyre carcass is stiﬀer in the longitudinal
direction, that is C′x > C′y and hence also C′σx > C′σy, λ′σy > λ′σx.


exhibits an intermediate behaviour between a linear spring and a damper. In this context,
it is also worth remarking that, according to the global equilibrium approach, the quantity
Vr(t)σ(t) · Ft(t) also represents the total slip losses taking place inside the contact patch,
at least in steady-state conditions.


5. Model comparison and validation

The present section addresses the validation of the two-regime tyre model.


5.1. Comparison with single contact point tyre models

The two-regime model developed in Section 4 is first compared versus the single con-
tact points. The following investigation is conducted by only considering translational slips
lower than σχ. The steady-state relationship for the tyre forces is chosen as in Equation (30).
According to Assumption 3.1, the carcass stiffness matrix C′ is assumed to be diagonal, but
with different values for C′x and C′y. Finally, the travelled distance s defined in Section 4 is
used as independent variable in place of the time t. By doing so, the problem under consid-
eration becomes independent of the rolling speed. Owing to these premises, the following
set of equations for the semi-nonlinear single point contact model may be introduced:

                  dσ ′(s)
                       σ    + σ ′(s) = σ(s),                                  (49a)
                         ds
```

<a id="romano-p021"></a>

#### ROM · PDF página 021 de 037

```text
1046          L. ROMANO ET AL.

                     ⎡                                #    $2⎤
                                                       ′(s)   1  σ ′(s)                     Ft(s) = Cσσ ′(s) ⎣1 −σ  +         ⎦,               (49b)
                                  σ cr    3   σ cr


where σ ′(s) =  v′(s)  and the matrix   σ reads specifically                        2

                           ⎡     ⎤
                                       Cσ
                                                0
                                 λσx   0          C′x                            σ =       =                                    (50)                                            Cσ ⎥⎥⎥⎦,                              0    λσy           ⎢⎢⎢⎣ 0
                                                                      C′y

since Cxσy = Cyσx = 0. In Equation (50), it has been renamed C′x ≜C′xx, C′y ≜C′yy and
λσx ≜λxσx, λσy ≜λyσy without ambiguity. The above system given by Equation (49a)
combines a linear ODE (49a) for the transient slip σ ′(s) and an algebraic Equation (49b)
(although nonlinear) in σ ′(s) for the planar force vector Ft(s). The second model is the
full-nonlinear single contact point. The equations are as follows:

                        dσ ′(s)                          ˜                       σ σ ′(s)     + σ ′(s) = σ(s),                           (51a)
                                ds
                   ⎡               ⎤
                                            ˜Cxσx σ ′(s)     ˜Cxσy σ ′(s)
                                                   C′x               C′x                          ˜                                                                                                                      ,               (51b)                       σ σ ′(s) =                                                                                                 ⎢⎢⎢⎢⎣ ˜Cyσx                                 σ ′(s)     ˜Cyσy σ ′(s)    ⎥⎥⎥⎥⎦
                                                    C′y                C′y


with

                   ⎡                                  #                                           #                                                  $2                                                      $2⎤
                                                             1              ∂Fx σ ′(s)           2σx(s)′  + σ y(s)′                                                         σ y(s)′                                              σ x(s)′
  ˜Cxσx σ ′(s) =       = Cσ ⎣1 −        +       cr  +          cr   ⎦,               ∂σ ′x               σ crσ ′(s)       σ        3   σ

                                                                                (52a)
                   ⎡                                  #                                           #                                                  $2                                                      $2⎤
                                                             1              ∂Fy σ ′(s)           2σy(s)′  + σ x(s)′                                              σ y(s)′                                                         σ x(s)′
  ˜Cyσy σ ′(s) =       = Cσ ⎣1 −        +       cr  +          cr   ⎦,               ∂σ ′y               σ crσ ′(s)       σ        3   σ

                                                                             (52b)
                          ∂Fx σ ′(s)    ∂Fy σ ′(s)
  ˜Cxσy σ ′(s) ≡˜Cyσx σ ′(s) =       =
                           ∂σ ′y         ∂σ ′x
                                              ′         ′   %          &
                σ x(s)σ y(s)    1      2
       = −Cσ          −             .                                 (52c)
                   σ cr    σ ′(s)   3σ cr


Equation (51a) together with Equations (51b) and (52a) are again used as input to
Equation (49b). In this case, however, the ODE (51a) is highly nonlinear.
```

<a id="romano-p022"></a>

#### ROM · PDF página 022 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1047





Figure 7. Comparison between the two-regime tyre model (solid line), the semi-nonlinear (dashed
line), the full-nonlinear single point contact model (dash-dotted line) and Guiggiani’s nonlinear full
contact patch (thick line). Generally speaking, a better agreement between the two-regime model and
the nonlinear full contact patch may be observed. (a) Simulation results for the two-regime tyre model
(solid line), the semi-nonlinear (dashed line), the full-nonlinear single point contact model (dash-dotted
line) and Guiggiani’s nonlinear full contact patch (thick line) for diﬀerent value of the longitudinal slip
input σx = 0.07 and σx = 0.21. (b) Simulation results for the two-regime tyre model (solid line), the
semi-nonlinear (dashed line), the full-nonlinear single point contact model (dash-dotted line) and Guig-
giani’s nonlinear full contact patch (thick line) for diﬀerent value of the lateral slip input σy = 0.07 and
σy = 0.21.


  The two-regime model is instead formulated directly in state–space form. With the
travelled distance as independent variable, the system in Equation (41) becomes

                         dFt(s)
                 = C′σ  σ(s) −ˆσ ϵ  Ft(s); ϵ     ,                     (53)
                          ds
```

<a id="romano-p023"></a>

#### ROM · PDF página 023 de 037

```text
1048          L. ROMANO ET AL.


                 Table 1. Tyre parameters.

                    Parameter                Description                 Unit       Value
                    Cσ              Slip stiﬀness                   N       3 · 104
                        C′x            Longitudinal stiﬀness of the carcass   N m−1     6 · 105
                        C′y             Lateral stiﬀness of the carcass       N m−1     2.4 · 105
                         Fz              Vertical force                   N       3000
                   a            Contact patch semilength        m        0.075
             μ              Friction coeﬃcient                   –         1


where the matrix C′σ  is defined as in Equation (37) and the slip function ˆσ(·) in
Equation (31) is replaced more conveniently by ˆσ ϵ(·; ϵ).
  A first set of simulations is aimed at comparing the three approximated models given
by Equations (49), (53) and (51) with the exact solution to Equation (1) starting from an
initial undeformed configuration and subjected to the constraint imposed by Equation (5)
(Guiggiani’s nonlinear full contact patch model [1]). The equations for the complete model
are derived in Guiggiani’s book [1] and are not reported here for brevity. Furthermore, for
the sake of simplicity, the longitudinal and lateral problems are analysed in isolation in
Figure 7. Indeed, even in the case of pure longitudinal or lateral interactions, Equation (1)
does not admit a closed-form solution and need to be solved numerically4. Generally, it
may be observed that the two-regime model succeeds better in replicating the exact trend,
whereas the single contact point models exhibit larger discrepancies especially at higher
values of the slip inputs. The parameters for the simulation are listed in Table 1.
  A second comparison between the three approximated models is shown for differ-
ent combinations of constant slip inputs σx and σy in Figure 8. The slip value is set to
σ = 0.17 < σχ. It may be noticed that the agreement between the two-regime tyre model
and the semi-nonlinear single contact point is particularly good in the beginning, when
the forces are in the linear region of the diagram Ft −σ and the assumption of constant
relaxation length holds fairly. As the travelled distance increases and the tyre characteristics
develop fully, the two-regime transient model exhibits a faster convergence to the steady-
state solution, and tends to the full-nonlinear one. Clearly, all the models converge in any
case to the same asymptotic value.
   Finally, a third set of simulation, whose results are shown in Figure 9, is carried out to
illustrate the models behaviour when the tyres are subjected to a sinusoidal forcing term
modelled as σ(s) = ¯σ[0.9 + 0.1 sin(ωs)]. For both excitation frequencies, ω = 10π and
50π, the slip amplitudes are limited to ¯σx = ¯σy = 0.12. Generally speaking, there is again
a very encouraging match between the two-regime tyre transient and the full-nonlinear
one, whilst the discrepancy with the semi-nonlinear single contact point is due to the
approximation of constant relaxation length.


5.2. Comparison with experimental data

In this paper, the transient response of the tyre predicted using the two-regime model was
partially validated against a series of consecutive deceleration manoeuvres. To the purpose
of experimental validation, two different braking tests were considered in turn: Test 1 was
characterised by a sequence of relatively small, continuously time-varying slips inputs (up
to |σx| = 0.3), whilst Test 2 was aimed at stressing the tyre up to the saturated region of
```

<a id="romano-p024"></a>

#### ROM · PDF página 024 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1049





Figure 8. Comparison between the two-regime tyre model (solid line), the semi-nonlinear (dashed line)
and the full-nonlinear single point contact model (dash-dotted line) for diﬀerent longitudinal and lateral
slip inputs σx and σy, respectively. It may be noticed a better agreement between the ﬁrst two models
for low values of the slip force, where the approximation of constant relaxation length holds for both
formulations. Conversely, as the value of the force increases, the two-regime tyre model exhibits a similar
behaviour to the full-nonlinear single contact point one. The black dashed line represent the steady-
state values for the longitudinal and tangential forces. (a) Slip inputs: σx = σy = 0.12. (b) Slip inputs:
σx = 0.085, σy = 0.14. (c) Slip inputs: σx = 0.14, σy = 0.085.
```

<a id="romano-p025"></a>

#### ROM · PDF página 025 de 037

```text
1050          L. ROMANO ET AL.





Figure 9. Comparison between the two-regime tyre model (solid line), the semi-nonlinear (dashed line)
and the full-nonlinear single point contact model (dash-dotted line) for diﬀerent excitation frequencies
ω.Itmaybenoticedthatboththetwo-regimeandthefull-nonlinearsinglecontactpointmodelssucceed
in following the oscillatory trend of the input, whilst the lateral tyre characteristic predicted by the semi-
nonlinear model seem to be almost unaﬀected by the harmonic component of the forcing term. (a) Slip
amplitude: ¯σx = ¯σy = 0.12; excitation frequency ω = 10π m−1. (b) Slip amplitude: ¯σx = ¯σy = 0.12;
excitation frequency ω = 50π m−1.




the slip curve. Both tests involved the VTI friction test vehicle BV12 shown in Figure 10: a
specially equipped truck with a test wheel suspension for passenger car wheels (in the fol-
lowing example, the wheel mounted a 4 D1 Continental/Ice Contact 2 tyre, with a nominal
radius of 0.36 m).
```

<a id="romano-p026"></a>

#### ROM · PDF página 026 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1051





Figure 10. The VTI friction test vehicle BV12.

  Some peculiar features of the BV12 are as follows. The test wheel can be forced to rotate
at the constant slip that produces maximum braking performance by means of a cardan
shaft connected to a gearbox with continuously variable slip. The wheel can also be locked
by means of a disk brake after being decoupled from the gearbox. The braking force is
calculated from wheel torque, which is measured by a transducer on the cardan shaft to the
wheel. The wheel load is obtained by means of dead weights acting on a spring/damper.
Furthermore, wheel rotational velocity and travelled distance are measured. From these
values, the longitudinal slip σx and rolling radius can be easily calculated.
  Data collected during Test 1 were specifically employed to parametrise the transient
longitudinal model according to the two-regime formulation of Equation (41) (again with
ˆσ(·) in Equation (31) modified as ˆσ ϵ(·; ϵ)), whilst Test 2 was mainly used for valida-
tion. In this regard, it is worth emphasising that, due to the intrinsic nature of the tests
considered in the paper, only the pure longitudinal transient behaviour of the tyre could
be validated. Moreover, since the longitudinal dynamics was considered in isolation and
the influence of the lateral slip was systematically disregarded, the transient critical slip
σχ was fairly assumed to be coincident with the corresponding steady-state value, i.e.
σχ ≡σ cr.
  For both tests, large variation in the normal force acting on the tyre could also be
observed during the braking phases. To cope with this aspect, the explicit dependence
upon the vertical load acting on the tyre was also taken into account in the parametri-
sation and validation processes. In particular, a simple linear model for the slip stiffness
Cσ as a function of Fz was assumed in this paper of the type Cσ = ¯CσFz. Starting from
```

<a id="romano-p027"></a>

#### ROM · PDF página 027 de 037

```text
1052          L. ROMANO ET AL.

the relationship Cσ = 4a2bk, the contact patch semilength was derived to be dependent         '         √
upon Fz as a = ¯a  ¯CσFz, with  ¯a = 1/  4bk. For the sake of simplicity, the semiwidth
b was instead assumed to be constant over time. According to the brush theory, the
critical slip value was also modelled explicitly as a function of the friction coefficient
μ as σ cr = 3μFz/Cσ ≡3μ/ ¯Cσ. Therefore, defining the longitudinal friction as μx ≜
Fx/Fz and ˆμx(·) ≜ˆFx(·)/Fz, it was possible to restate the longitudinal component of
Equation (30) as

                                   |σx|   1     σx   2
 μx = ˆμx(σx) = ¯Cσσx  1 −¯Cσ   +     ¯Cσ               ,   σx ∈(−3μ/ ¯Cσ, 3μ/ ¯Cσ).  (54)
                       3μ   3    3μ

Owing to these premises, the tyre model was parameterised in a two-step process:
first, initial values for the structural parameter ¯Cσ and the friction coefficient μ in
Equation (54) were estimated employing an iterative procedure based on the least-square


                  Table 2. Tyre parameters optimised from experimental data.

                     Parameter             Description               Unit        Value
                               ¯Cσ           Normalised slip stiﬀness           –          7.6
              μ              Friction coeﬃcient               –         0.48
                         C′x             Lateral stiﬀness of the carcass    N m−1      8.61 · 105
                                       ¯a            Contact patch parameter     m N−1/2      0.0003





Figure 11. Tyre-slip curve ﬁtted from experimental data. Parameter values: ¯Cσ = 7.6, μ = 0.48.
```

<a id="romano-p028"></a>

#### ROM · PDF página 028 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1053


fitting technique. Specifically, these quantities were optimised restricting the attention
to the nonlinear region of the tyre-slip curve in the range σx ∈(−σ cr, −σ cr/3] =
(−3μ/ ¯Cσ, −μ/ ¯Cσ], where the transient dynamics of the tyre was expected to play a minor
role due to the fast excitation caused by large longitudinal slips (slip was defined with
negative sign for braking). Since ¯Cσ and μ were also unknown, the procedure was iter-
ated multiple times until convergence to ensure that only the desired points were included
when optimising both parameters. At each iteration, the previous estimate for σ cr was used
to select the appropriate samples. The normalised slip stiffness was constrained between
0 ≤¯Cσ ≤10, whilst lower and upper bounds for the friction coefficient were specified as
0 ≤μ ≤0.6. The upper limit on μ was motivated by the fact that the maximum observed
ratio μx between the longitudinal tyre force and the normal load was around 0.62 (as
already pointed out, in the brush model the peak force equals μFz, and therefore the maxi-
mum value for μx in Equation (30) is μpeak ≡μ). For both quantities, the optimised values
are reported in Table 2, whereas a comparison between the measured data points and the
predicted steady-state behaviour of the tyre is shown graphically in Figure 11.





Figure 12. Transient longitudinal response of the tyre to a time-varying slip input σx (red line) predicted
according to the two-regime model (blue line) and the steady-state Equation (54) (cyan line). The com-
parison is performed against the measured tyre force (grey line). (a) Transient longitudinal tyre response
during Test 1. (b) Transient longitudinal tyre response during Test 2.
```

<a id="romano-p029"></a>

#### ROM · PDF página 029 de 037

```text
1054          L. ROMANO ET AL.

   In a second step, the optimal values for the tyre carcass stiffness C′x and the quantity
¯a were found by minimising the error between the transient simulation results and the
measured tyre data, using the travelled distance s as independent variable. In this process,
the dynamic model described by Equation (41) was implemented by saturating the output
Fx(s), which was always constrained below the friction limit μFz(s). The dynamical sys-
tem was then re-initialised to the measured longitudinal force every time that the latter fell
below the estimated peak force. Again, the values extracted from the optimisation routine
are listed in Table 2. A comparison between the transient behaviour predicted by the two-
regime model (blue line), the steady-state Equation (54) (cyan line) and the experimental
data (grey line) is shown in Figure 12 for both Test 1 and Test 2. Obviously, the match is far
from being perfect due to the simplistic nature of the brush model, but a good qualitative
agreement towards the measured tyre force may be observed overall, thus corroborating
the potential of the proposed formulation. It should be noticed that the steady-state model
for the longitudinal tyre force given by Equation (54) yields very similar results to the tran-
sient one, especially during severe braking phases, whereas larger discrepancies may be
observed for relatively low values of the slip (in magnitude), where the slower dynamics of
the two-regime formulation better matches the experimental results. The calculated root
mean square errors (RMS) for the steady-state and transient models were 368.14 and 356.40
for Test 1 and 329.57 and 329.48 for Test 2, respectively.

6. Conclusion

In this paper, the two-regime theory recently introduced by Romano et al. [5] has been
refined. First, a complete set of relationships – renamed sliding functions – existing between
the slip variables and the tyre forces and moment at low rolling speeds has been derived.
The notion of slip functions has also been introduced to formalise mathematically the
corresponding steady-state relationships.
   Second, a class of nonlinear dynamic models for combined translational slips has been
developed that allows to describe the transient generation of tyre forces in a relatively sim-
ple way. The model generalises further the one presented in [5] and allows to circumvent
the need to solve the complicated PDEs governing the tyre dynamics.
  A formal analysis has been then conducted in Section 4 by resorting to the well-
established theory for nonlinear systems. The investigation has been restricted to the case
in which the steady-state tyre forces and the translational slips have the same direction.
In this context, an estimate σχ on the supremum of the transient slip that always ensures
partial adhesion in the contact patch has been provided. Indeed, any value σ < σχ implies
automatically that the tyre forces never exploit the maximum friction available. The tyre
carcass anisotropy plays a pivotal role in the determination of the parameter σχ through
the relaxation ratio χλ. This is defined mathematically as the ratio between the smallest
and largest eigenvalues of the enhanced stiffness matrix. For a tyre with a diagonal matrix
for the carcass stiffnesses, this also coincides with the ratio between the minimum and
maximum enhanced relaxation lengths. In transient conditions, low values of χλ corre-
spond to smaller admissible slip inputs σχ compared to the critical slip value σ cr that
guarantees adhesion in steady-state conditions. This result may be exploited for control
or optimisation purposes.
  Then the focus has been shifted towards the physical properties of the new model. It
has been demonstrated that the dynamical system used to describe the transient forces
```

<a id="romano-p030"></a>

#### ROM · PDF página 030 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1055


dissipates power at the tyre–road interface. As a consequence, the frictional power gener-
ated due to finite slip is not stored completely in the elastic carcass.
   Finally, in Section 5 the validation of the two-regime model introduced in this paper has
been addressed. Specifically, in Section 5.1, the complete two-regime model for combined
slip has been compared against the semi-nonlinear and the full-nonlinear single contact
point models, and partially against Guiggiani’s nonlinear full contact patch. The agree-
ment between the first two formulations is especially good at a low value of the slip inputs,
where the approximation of constant relaxation lengths holds fairly; as the slips increase,
the two-regime transient model matches the full-nonlinear one. On the other hand, it has
been also demonstrated that the two-regime model exhibits a better agreement to Guig-
giani’s complete model compared to the single contact points. The two-regime tyre models
and the single contact point represent an approximation of the pure brush and the brush-
string models, respectively. In the former, the tyre carcass is modelled using a linear spring,
whilst the latter is based on a distributed description whose governing differential equa-
tions are the ones of a stretched string. Simulation results seem to suggest that the specific
constitutive relationship chosen to model the carcass extensibility does not play a funda-
mental role for accurate prediction of the transient generation of forces and moment, since
the two models behave very similarly. Additionally, the transient longitudinal dynamics of
the tyre according to the two-regime formulation has been validated in isolation against
experimental data in Section 5.2. In spite of the simplistic nature of the brush models, the
behaviour predicted using the two-regime theory appears to agree qualitatively with the
measured quantities, confirming the potential of the approach outlined in the paper.
  Compared to the single contact point models, the two-regime model is simpler to imple-
ment and is more general from a theoretical viewpoint, since it considers the transient
due to the bristle deflection. However, a closed-form expression for the two-regime for-
mulation has only been possible to derive for the case of isotropic tread, whilst the single
contact point models can use the transient slip as input to any tyre equation. The possi-
bility of extending the two-regime theory to account for the tyre tread anisotropy must be
therefore explored in future studies.


Forces and moments    Unit                      Description

Ft            N        Planar force vector
Ft0            N          Initial conditions for the planar force vector
Fx, Fy          N        Longitudinal and lateral tyre forces
Fx0, Fy0         N          Initial conditions for the longitudinal and lateral tyre
                                      forces
Fz            N        Vertical force
Mz            N m      Self-aligning moment
qt            N m−2   Tangential shear stress vector
qt            N m−2   Total tangential shear stress
qx, qy          N m−2   Longitudinal and lateral shear stress
qz            N m−2   Vertical pressure
Displacements         Unit                      Description
ut           m       Displacement vector of the bristle
```

<a id="romano-p031"></a>

#### ROM · PDF página 031 de 037

```text
1056          L. ROMANO ET AL.


ux, uy        m         Longitudinal and lateral displacement of the bristle
ut0         m            Initial tangential displacement vector of the bristle
                                   (IC)
ux0, uy0       m            Initial longitudinal and lateral displacement (IC)
s          m          Travelled distance
x          m         Coordinate vector
x, y         m         Longitudinal and lateral coordinates
δt          m         Tyre carcass tangential displacement vector
δx, δy        m         Tyre carcass longitudinal and lateral displacements
Speeds              Unit                       Description
Vr          m s−1      Rolling speed
vt          m s−1      Tangential velocity field
vx, vy        m s−1      Longitudinal and lateral components of the velocity
                                           field
vs          m s−1      Micro-sliding velocity
vsx, vsy        m s−1      Longitudinal and lateral micro-sliding speeds
Slip                 Unit                       Description
Parameters
σ                –           Translational slip vector
σ                –           Total translational slip
σx, σy             –          Longitudinal and lateral slip
σ cr               –          Global critical slip
σχ               –          Transient critical slip
σ ′               –          Transient slip vector
σ ′               –           Total transient slip
σ x,′ σ y′             –          Transient longitudinal and lateral slip
ϕ              m−1       Rotational slip or spin parameter
Rotation matrices     Unit                       Description
and tensors
Aϕ             m−1       Spin tensor
Geometric           Unit                       Description
parameters
a, b         m         Contact patch semilength and semiwidth
¯a          m N−1/2   Contact patch parameter
Rr          m          Rolling radius
           m         Matrix of relaxation lengths
 ˜  σ         m         Matrix of generalised relaxation lengths
    ′
  σ         m         Matrix of enhanced relaxation lengths
λxσx, λyσy      m         Longitudinal and lateral relaxation length
λxσy, λyσx      m         Cross relaxation lengths
˜λxσx, ˜λyσy      m         Generalised longitudinal and lateral relaxation length
˜λyσx, ˜λxσy      m         Generalised cross relaxation lengths
λ′xσx, λ′yσy      m        Enhanced longitudinal and lateral relaxation length
λ′max, λ′min      m       Maximum and minimum enhanced relaxation length
χλ               –          Relaxation ratio
```

<a id="romano-p032"></a>

#### ROM · PDF página 032 de 037

```text
                                                               VEHICLE SYSTEM DYNAMICS       1057


Stiffnesses         Unit                       Description
and compliances
Kt          N m−3   Matrix of the bristle tangential stiffnesses
kxx, kyy       N m−3   Longitudinal and lateral stiffness of the tread bristle
kxy, kyx       N m−3   Cross stiffnesses of the tread bristle
Cσ          N        Matrix of slip stiffnesses
˜Cσ          N        Matrix of generalised slip stiffnesses
C′  σ          N m−1   Enhanced matrix of slip stiffnesses
Cxσx, Cyσy      N        Longitudinal and lateral slip stiffnesses
˜Cxσx, ˜Cyσy      N        Generalised longitudinal and lateral slip stiffnesses
C′xσx, C′yσy      N m−1   Enhanced longitudinal and lateral slip stiffnesses
Cxσy, Cyσx      N        Cross slip stiffnesses
˜Cxσy, ˜Cyσx      N        Generalised cross slip stiffnesses
C′xσy, C′yσx      N m−1   Enhanced longitudinal and lateral cross slip stiffnesses
Cσ          N         Slip stiffness
¯Cσ              –        Normalised slip stiffness
C′max = C′min    N m−1  Maximum and minimum enhanced stiffness
C′          N m−1   Matrix of tyre carcass stiffnesses
C′xx, C′yy       N m−1   Longitudinal and lateral stiffness of the tyre carcass
C′xy, C′yx       N m−1   Cross stiffnesses of the tyre carcass
S′         m N−1   Matrix of tyre carcass compliances
S′xx, S′yy       m N−1   Longitudinal and lateral compliance of the tyre carcass
S′xy, S′yx       m N−1   Cross compliances of the tyre carcass
Friction           Unit                       Description
parameters
μ               –         Friction coefficient
μpeak            –        Peak friction coefficient
μx              –        Normalised longitudinal force
Functions          Unit                       Description
and operators
∇t             m−1      Tangential gradient
ˇσ(·, ·), ˇϕ(·, ·)           -, m−1    Sliding functions
ˆσ(·, ·), ˆϕ(·, ·)           -, m−1    Slip functions
ˆσ ϵ(·; ·)           –        Modified slip functions
Sets               Unit                       Description
Cμ             N2        Friction circle
◦
C μ            N2        Interior of the friction circle
Cχ             N2       Relaxation circle
◦
C χ             N2        Interior of the relaxation circle
Eχ             N2       Relaxation ellipse
Eχ             N2        Interior of the relaxation ellipse
P            m2       Contact patch
P            m2        Interior of P
∂P        m       Boundary of P
L         m       Leading edge
```

<a id="romano-p033"></a>

#### ROM · PDF página 033 de 037

```text
1058          L. ROMANO ET AL.


     N   m   Neutral edge
     T   m    Trailing edge
        R≥0   –    Set of positive real numbers (including 0)
       R>0   –    Set of strictly positive real numbers (excluding 0)


Notes

 1. For example, Pacejka’s Magic Formula.
 2. For a definition of input-to-state stability, the reader is referred to Khalil [45] (Chapter 4).
 3. For a mathematical definition of passivity, the reader is again redirected to Khalil [45, Chapter
     6].
 4. For example, using Euler’s forward scheme.


Acknowledgments

The authors gratefully acknowledge financial support from the COVER project (44929-1), funded
by the Swedish energy agency and the Swedish vehicle research and innovation program (FFI).


Disclosure statement

The authors declare that they have no conflict of interest.


Funding

The authors gratefully acknowledge financial support from the COVER project (44929-1),
funded by the Swedish energy agency and the Swedish vehicle research and innovation
program (FFI)

ORCID

Luigi Romano    http://orcid.org/0000-0001-8435-7696
Bengt Jacobson    http://orcid.org/0000-0002-5798-5651


References

 [1] Guiggiani M. The science of vehicle dynamics. 2nd ed. Cham (Switzerland): Springer Interna-
      tional; 2018.
 [2] Genovese A, Garofano D, Sakhnevych A, et al. Static and dynamic analysis of non-Pneumatic
      tires based on experimental and numerical methods. Appl Sci. 2021;11(23):11232.
 [3] Sakhnevych A. Multiphysical MF-based tyre modelling and parametrisation for vehicle setup
    and control strategies optimisation. Vehicle Syst Dyn. 2021;21:1–22. 10.1080/00423114.2021.
     1977833.
 [4] O’Neill A, Prins J, Watts JF, et al. Enhancing brush tyre model accuracy through friction
     measurements. Vehicle Syst Dyn. 2021;115:1–23. 10.1080/00423114.2021.1893766.
 [5] Romano  L, Bruzelius  F, Jacobson  B. Unsteady-state brush  theory. Vehicle  Syst Dyn.
     2020;59(11):1643–1671. 10.1080/00423114.2020.1774625.
 [6] Romano L, Bruzelius F, Jacobson B. Brush tyre models for large camber angles and steering
     speeds. Vehicle Syst Dyn. 2020;21:1–52. 10.1080/00423114.2020.1854320.
 [7] Romano L, Timpone F, Bruzelius F, et al. Analytical results in transient brush tyre mod-
       els: theory for large camber angles and classic solutions with limited friction. Meccanica.
     10.1007/s11012-021-01422-3.
```

<a id="romano-p034"></a>

#### ROM · PDF página 034 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1059


 [8] Pacejka HB. Tire and vehicle dynamics. 3rd ed. Amsterdam: Elsevier/BH; 2012.
 [9] Bengt JHJ. Vehicle Dynamics Compendium. 2020. Available from: https://research.chalmers.se
     /en/publication/520229.
[10] von Schlippe B, Dietrich R. Das flattern eined bepneuten rades. Bericht 140 der Lilienthal
      Gesellschaft. 1941: NACA TM 1365.
[11] Segel L. Force and moment response of pneumatic tires to lateral motion inputs. Trans ASME
       J Eng Industry. 1966;88: 10.1076/vesd.41.1.1.23406.
[12] Pauwelussen JP. The local contact between tyre and road under steady state combined slip
     conditions. Vehicle Syst Dyn. 2004;41(1):1–26. 10.1076/vesd.41.1.1.23406.
[13] Takács D, Orosz G, Stépán G. Delay effects in shimmy dynamics of wheels with stretched string-
      like tyres. Eur J Mech- A/Solids. 2009;28(3):516–525.
[14] Takács D, Stépán G. Micro-shimmy of towed structures in experimentally uncharted unstable
     parameter domain. Vehicle Syst Dyn. 2012;50(11):1613–1630.
[15] Takács D, Stépán G, Hogan SJ. Isolated large amplitude periodic motions of towed rigid wheels.
     Nonlinear Dyn. 2008;52:27–34. 10.1007/s11071-007-9253-y.
[16] Takács D, Stépán G. Experiments on quasiperiodic wheel shimmy. ASME J Comput Nonlinear
    Dynam. 2009;4(3):031007. 10.1115/1.3124786.
[17] Takács D, Stépán G. Contact patch memory of tyres leading to lateral vibrations of four-
     wheeled vehicles. Phil Trans R Soc A. 2013; 37120120427. 10.1098/rsta.2012.0427.
[18] Besselink IJM. Shimmy of aircraft main landing gears [doctoral thesis]. Delft; 2000.
[19] Ran S. Tyre models for shimmy analysis: from linear to nonlinear [doctoral thesis]. Eindhoven;
     2016.
[20] Pacejka HB. The wheel shimmy phenomenon: a theoretical and experimental investigation
     with particular reference to the non-linear problem [doctoral thesis]. Delft; 1966.
[21] Pacejka HB, Bakker E. The magic formula tire model. Int J Vehicle Mechanics and Mobility.
     1992;21(1):1–18.
[22] Higuchi A. Transient response of tyres at large wheel slip and camber [doctoral thesis]. Delft;
     1997.
[23] Higuchi A, Pacejka HB. The relaxation length concept at large wheel slip and camber. Vehicle
     Syst Dyn. 1997;25(sup001):50–64. 10.1080/00423119708969644.
[24] Maurice JP, Berzeri M, Pacejka HB. Pragmatic tyre model for short wavelength side slip
      variations. Vehicle Syst Dyn. 1999;31(2):65–94. 10.1076/vesd.31.2.65.2096.
[25] Rill G. Sophisticated but quite simple contact calculation for handling tire models. Multibody
     Syst Dyn. 2019;45:131–153. 10.1007/s11044-018-9629-4.
[26] Rill G. Road vehicle dynamics: fundamentals and modeling with MATLAB®. 2nd ed. Abingdon:
   CRC Press; 2020.
[27] Svendenius J, Wittenmark B. Brush tire model with increased flexibility. European Control
     Conference; Cambridge, UK: 2015. 10.23919/ECC.2003.7085237.
[28] Svendenius J. Tire modelling and friction estimation [dissertation]. Lund; 2007.
[29] Svendenius J, Gäfvert M, Bruzelius F, et al. Experimental validation of the brush tire model.
     Tire Sci Technol. 2009;37(2):122–137.
[30] Pacejka HB, Besselink IJM. Magic formula tyre model with transient properties. Vehicle Syst
    Dynam. 1997;27(Sup001):234–249. 10.1080/00423119708969658.
[31] Romano L, Sakhnevych A, Strano S, et al. A novel brush-model with flexible carcass for
     transient interactions. Meccanica. 2019;54:1663–1679. 10.1007/s11012-019-01040-0.
[32] Shaju A, Pandey AK. Modelling transient response using PAC 2002-based tyre model. Vehicle
     Syst Dyn. 2020; 10.1080/00423114.2020.1802048:
[33] Canudas-de-Wit C, Olsson H, Astrom KJ, et al. A new model for control of systems with
      friction. IEEE Trans Automat Contr. 1995;40(3):419–425. 10.1109/9.376053.
[34] Canudas-de-Wit C, Tsiotras P, Velenis E, et al. Dynamic friction models for road/tire longitu-
     dinal interaction. Vehicle Syst Dyn. 2003;39(3):189–226. 10.1076/vesd.39.3.189.14152.
[35] Canudas-de-Wit C, Tsiotras P. Dynamic tire friction models for vehicle traction control. Pro-
     ceedings of the 38th IEEE Conference on Decision and Control (Cat. No. 99CH36304); Vol. 4,
     Phoenix, AZ, USA: 1999. p. 3746–3751. 10.1109/CDC.1999.827937.
```

<a id="romano-p035"></a>

#### ROM · PDF página 035 de 037

```text
1060          L. ROMANO ET AL.


[36] Velenis  E,  Tsiotras  P, Canudas-de-Wit  C,  et  al. Dynamic  tyre  friction models  for
    combined  longitudinal and  lateral  vehicle  motion.  Vehi  Syst Dyn.  2005;43(1):3–29.
     10.1080/00423110412331290464.
[37] Deur J. Modeling and analysis of longitudinal tire dynamics based on the Lugre friction model.
    IFAC Proceedings Volumes. 2001;34(1):91–96. 10.1016/S1474-6670(17)34383-5.
[38] Deur J, Asgari J, Hrovat D. A 3D brush-type dynamic tire friction model. Vehicle Syst Dyn.
     2004;42(3):133–173. 10.1080/00423110412331282887.
[39] Deur J, Ivanovic V, Troulis M, et al. Extensions of the LuGre tyre friction model related to
     variable slip speed along the contact patch length. Vehicle Syst Dyn. 2005;43(supp):508–524.
     10.1080/00423110500229808.
[40] Xu N, Guo K, Zhang X, et al. An analytical tire model with flexible carcass for combined slips.
    Math Probl Eng. 2014; 397538. Article ID 9 pages.10.1155/2014/397538.
[41] Liang W, Medanic J, Ruhl R. Analytical dynamic tire model. Vehicle Syst Dyn. 2008;46(3):
     197–227. 10.1080/00423110701267466.
[42] Kikuuwe R. A brush-type tire model with nonsmooth representation. Math Probl Eng. 2019;
     9747605. Article ID 13 Pages.10.1155/2019/9747605.
[43] Edwards CH. Advanced Calculus of Several Variables, III – Successive Approximations and
     Implicit Functions. Academic Press; 1973. P. 160–202.
[44] Bruzelius  F, Hjort M, Svendenius  J. Validation  of a  basic combined-slip  tyre model
      for use in friction estimation applications. Proc Inst Mech Eng D  J Automobile Eng.
     2014;228(13):1622–1629. 10.1177/0954407013511797.
[45] Khalil HK. Nonlinear systems. 3rd. Upper Saddle River: Prentice Hall; 2002.


Appendices

Derivation of the sliding functions

The derivation of the sliding functions appearing in Equation (24) may be worked out similarly
as in Romano et al. [5]. Substituting Equation (1) into Equation (18a), assuming vanishing sliding
and disregarding the partial derivative with respect to the longitudinal coordinate yields, after some
manipulations,
                              % &     %            & %  &
                                                       0                               σ                                                        I + APKtS′                                                                                                          ˙Ft              APKt                            Kt˜ISP                   ≈1                                                                                                                                        .             (A1)                              ϕ                                                          Iσ S′                         Iσ                                    Iϕ                                                         1   ˙Mz                                         Vr

where

        AP ≜      dx,                                                     (A2a)
             P

          SP ≜     x dx                                                  (A2b)
             P
                                 ((                 ((
                  Iσ =  Iσx    Iσy ≜  P kyxx −kxxy dx   P kyyx −kxyy dx   ,         (A2c)

                   Iϕ ≜      kyyx2 −(kxy + kyx)xy + kxxy2 dx,                         (A2d)
             P
and the matrix ˜I reading
                                  %     &
                                       0  −1                                                                                        ˜I ≜                   .                                (A3)
                                       1   0
Solving Equation (A1) for the theoretical slip and spin variables yields
                  ⎡      ⎤
                % &                                %  &                                                                        S′                                                     ˇσ   ˙Ft, ˙Mz              S′                  σ                                                         F           ≈1  ⎢⎣      ⎥⎦≜1       M      ˙Ft    ,               (A4)
                 ϕ    Vr    ˇϕ   ˙Ft, ˙Mz      Vr   S′ϕF    S′ϕM    ˙Mz
```

<a id="romano-p036"></a>

#### ROM · PDF página 036 de 037

```text
                                                                 VEHICLE SYSTEM DYNAMICS       1061


where the matrix of generalised compliances is defined as

                                               −1 %            &                        S′                               S′
                      F                  M     APKt   Kt˜ISP      I + APKtS′  0                 ≜                        S′                                         Iσ        Iϕ             Iσ S′      1   .            (A5)                    ϕF    S′ϕM


Proof of Theorem 4.1

The proof of Theorem 4.1 is given here.


Proof: The result follows from an application of Theorem 4.19 ([45, Chapter 4]). For the problem
under consideration, the Lyapunov function may be defined as V(Ft(t)) ≜12F2t (t). The above func-
tion satisfies the first assumption of Theorem 4.19 with α1(Ft) = α2(Ft) = V(Ft), which implies
ρ(·) ≡γ (·). Taking the derivative yields
                    ˙V  Ft(t) = −Vr(t)C′σ ˆσ ϵ  Ft(t); ϵ  · Ft(t) + Vr(t)C′σ σ(t) · Ft(t).          (A6)

Since V(Ft(t)) is radially unbounded and ˙V(Ft(t)) is negative definite for σ = 0, it may be eas-
ily concluded that the origin is asymptotically stable. Recalling that ˆσ ϵ(Ft; ϵ) = ˆσ(Ft)Ft/(Ft + ϵ),
Equation (A6) yields

                             C′                                 σ Ft(t) · Ft(t)   ˙V  Ft(t) = −Vr(t)ˆσ  Ft(t)         + Vr(t)C′σ σ(t) · Ft(t)
                                    Ft(t) + ϵ
                             C′                                 σ Ft(t) · Ft(t)
          ≤−Vr(t)ˆσ  Ft(t)         + Vr(t)  v′σ σ(t) · Ft(t) 2                                    Ft(t) + ϵ
          ≤−Vr(t)λmin C′σ  ˆσϵ  Ft(t); ϵ  Ft(t) + Vr(t)σ(t)  v′σ Ft(t) 2
          ≤−Vr(t)λmin C′σ  ˆσϵ  Ft(t); ϵ  Ft(t) + Vr(t)λmax C′σ  σ(t)Ft(t)
      = −W  Ft(t), t −Vr(t)ψλmin C′σ  ˆσϵ  Ft(t); ϵ  Ft(t) + Vr(t)λmax C′σ  σ(t)Ft(t),
                                                                                (A7)

where W(Ft(t), t) ≜Vr(t)(1 −ψ)λmin(C′σ )ˆσϵ(Ft(t); ϵ)Ft(t) is positive definite for some positive
constant ψ ∈(0, 1). Therefore, any value of Ft(t) such that
                                                      ˆσϵ  Ft(t); ϵ ≥σ(t)                              (A8)
                                    ψχλ

ensures that ˙V(Ft(t)) is negative definite. Reversing the inequality, it may be deduced that if

                                                              peak      1              sup σ(t) < θχλ ˆσϵ  μpeakFz; ϵ = ψχλσ              ≜σχ,         (A9)
                   t≥t0                                  1 + ϵ μpeakFz

then Ft(t) < μpeakFz for all t ≥t0.
   Furthermore, since ˆσϵ  : [0, μpeakFz) →[0, σχ/(ψχλ)) is a K-class function in (0, μpeakFz) by
Assumption 3.2, its inverse σ ϵ−1 (·; ϵ) may be computed in (0, μpeakFz). The function
                   ⎧
                                      ⎪⎨0,             σ = 0,
                     γ (σ; ϵ) ≜    −1   σ                                     (A10)                                      ⎪⎩σ ϵ                ; ϵ    ,  σ ∈  0, σχ   ,
                               ψχλ

is continuous in [0, σχ), since ˆσϵ(0; ϵ) = 0 and limFt→μpeakFz ˆσϵ(Ft; ϵ) = σχ/(ψχλ). Additionally,
it is strictly increasing. Therefore, it may be concluded that γ ∈K. Then, it may be observed that,
when σ = 0, Ft = 0 satisfies automatically the inequality, since it is σϵ(0; ϵ) = 0 ≡γ (0; ϵ). On the
```

<a id="romano-p037"></a>

#### ROM · PDF página 037 de 037

```text
1062          L. ROMANO ET AL.


other hand, for σ ∈(0, σχ) it holds that

                          −1             −1   σ
                         Ft = ˆσ ϵ   ◦ˆσϵ(Ft; ϵ) ≥ˆσϵ                ; ϵ ≜γ (σ; ϵ).               (A11)
                                    ψχλ

Therefore, it may be inferred that the system (41) is input-to-state-stable with γ (·; ϵ) defined as in
Equation (A10). It remains to prove that γ (σ; ϵ) →F−1t  (σ/(ψχλ)) as ϵ →0+. The result for σ = 0
is trivial. In any other case:

                                −1   σ         −1   σ          σ              lim γ (σ; ϵ) = lim  ˆσ ϵ               ; ϵ = ˆσ      = ˆFt                 .       (A12)
            ϵ→0+         ϵ→0+      θχλ          ψχλ       ψχλ

                                               ■
```

---
