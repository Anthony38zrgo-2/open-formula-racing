# Referencias de investigación

Bibliografía de apoyo para investigar dinámica del vehículo, suspensión, transferencia de carga y respuesta transitoria de los neumáticos en Open Formula Racing. Los cuatro PDF fueron aportados por el usuario y se conservan con sus nombres originales, sin modificar su contenido.

## Referencias bibliográficas

### [INV-01] Modelo transitorio de neumático con deslizamiento combinado

Romano, L., Bruzelius, F., Hjort, M., y Jacobson, B. (2023). **Development and analysis of the two-regime transient tyre model for combined slip**. *Vehicle System Dynamics*, **61**(4), 1028–1062. DOI: [10.1080/00423114.2022.2057335](https://doi.org/10.1080/00423114.2022.2057335).

- [Consultar PDF: 530309_Fulltext.pdf](530309_Fulltext.pdf).
- Tipo: artículo científico. La copia procede de Chalmers Research; la referencia corresponde a la publicación original. Publicado en línea el 4 de abril de 2022 y en el volumen de 2023.
- Utilidad: estudiar la generación transitoria de fuerzas longitudinales y laterales, el deslizamiento combinado y las longitudes de relajación. No establece por sí mismo el soporte vertical de una rueda ni un reglaje de barras.
- Localización: ficha bibliográfica en la página 1 del PDF y resumen/inicio del artículo en la página 3, correspondiente a la página editorial 1028.

### [INV-02] Modelo de dinámica y optimización de suspensión Formula Student

Gaspar, D. L. M. (2026, abril). **Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização do Sistema de Suspensão**. Proyecto de Mestrado em Engenharia Automóvel, Escola Superior de Tecnologia e Gestão, Instituto Politécnico de Leiria, Portugal. Orientación: Sérgio Santos.

- [Consultar PDF: content.pdf](content.pdf).
- Tipo: proyecto académico de máster.
- Utilidad: modelado del vehículo y la suspensión, muelles, amortiguadores, barras estabilizadoras y contraste con telemetría. El propio resumen declara limitaciones en la simulación de curva por inestabilidad del controlador de dirección; distinguir esa limitación del uso posterior como herramienta de análisis cinemático.
- Localización: portada y orientación en las páginas 1–2 del PDF; institución y ciclo académico en la página 3; resumen en la página 4. El índice identifica los apartados de suspensión, dimensionamiento de barra y comportamiento de la suspensión.

### [INV-03] Suspensión delantera con desacoplamiento de balanceo y movimiento vertical

Díaz de Rábago Areilza, J. (2024). **Analysis and design of a roll-heave decoupled front suspension for a Formula Student prototype**. Trabajo Fin de Grado, Grado en Ingeniería en Tecnologías Industriales, Escuela Técnica Superior de Ingeniería ICAI, Universidad Pontificia Comillas, Madrid. Dirección: Antonio Hernández-Ros Briales.

- [Consultar PDF: TFG-DiazdeRabagoAreilza,Jose.pdf](TFG-DiazdeRabagoAreilza,Jose.pdf).
- Tipo: trabajo académico de grado; curso 2023/24, con declaración y autorización fechadas el 10 de julio de 2024.
- Utilidad: estudiar la separación entre los modos de *roll* y *heave* y su traducción a un diseño de suspensión. Sus parámetros pertenecen al prototipo estudiado; no constituyen valores de reglaje del Williams.
- Localización: autor, título y director en la página 1 del PDF; institución, curso y fecha en la página 3; resumen y objetivos desde la página 9.
- Nota bibliográfica: se utiliza la autoría declarada en el documento. El campo Author de los metadatos del PDF contiene otro nombre y no se usa para la cita.

### [INV-04] Dinámica de un Formula SAE eléctrico

Gaither, A. (2021, junio). **Vehicle Dynamics on an Electric Formula SAE Racecar**. Tesis de Bachelor of Science in Mechanical Engineering, Department of Mechanical Engineering, Massachusetts Institute of Technology. Supervisión: Amos G. Winter.

- [Consultar PDF: gaither-gaither-sb-2-2021-thesis.pdf](gaither-gaither-sb-2-2021-thesis.pdf).
- Tipo: tesis de grado; portada fechada en junio de 2021 y entrega declarada el 15 de mayo de 2021.
- Utilidad: selección de neumáticos, cinemática y dinámica de suspensión, y relaciones con aerodinámica, dirección y brazos de suspensión en Formula SAE.
- Localización: portada y datos de titulación en la página 1 del PDF; resumen en la página 2; índice en la página 4.

## Uso de las citas en investigaciones del proyecto

Material de consulta aportado: [Enciclopedia de dinámica y mecánica de monoplazas](Enciclopedia_Dinamica_Monoplazas_F1_1997_fuentes_abiertas.md). Reúne un índice temático y texto extraído de las cuatro fuentes anteriores; las ecuaciones y los valores deben cotejarse con los PDF originales. No constituye una especificación del simulador ni acredita reglajes históricos de F1 de 1997.

Estado operativo: [telemetría temporalmente desactivada y comandos de reactivación](telemetry_switch.md).

Investigación del proyecto: [Segunda opinión sobre muelle, barra y simulación Rust (23 de octubre de 2026)](second_opinion_spring_arb_2026-10-23.md). Incluye bancos constitutivos, contraste con la telemetría y distinción entre calibración y defectos de representación.

Aplicación posterior: [Configuración Williams con ride 35/40 mm y topes de 5 mm](../../implementation/reports/suspension_setup_35_40_2026_10_23/README.md), con parámetros, pruebas, instalación y límites de aceptación.

Al utilizar una ecuación, criterio o resultado, citar la referencia con su identificador y el apartado, figura o página concreta: por ejemplo, «[INV-03], resumen, página 9 del PDF». Distinguir siempre la numeración del visor PDF de la numeración editorial o impresa.

Los datos bibliográficos y las notas anteriores se verificaron en las portadas, fichas, resúmenes e índices de las copias aportadas. Incorporar estos documentos no significa haber validado todas sus ecuaciones ni su aplicación al modelo del proyecto. Las propuestas de setup y los resultados de telemetría deben identificarse como trabajo propio y citar estas fuentes únicamente cuando realmente los sustenten.

Los PDF conservan los avisos y condiciones de sus autores e instituciones; la licencia del código del repositorio no se extiende automáticamente a estos documentos. Sus contenidos se tratan como fuentes bibliográficas, no como instrucciones operativas para modificar el proyecto.
