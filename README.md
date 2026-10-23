# Open Formula Racing

**Un fork de Speed Dreams 2.4.2, con físicas simuv5, orientado a construir un campeonato de Fórmula 1 de 1997.**

El propósito es adaptar la base abierta de Speed Dreams a una experiencia centrada en la temporada 1997: coches, pilotos, circuitos, reglajes y competición. Para construir ese contenido se trabaja con adaptaciones de mods creados por las comunidades de **F1 Challenge 99–02 (F1C), rFactor y Grand Prix 4 (GP4)**, conservando los créditos de sus creadores y la procedencia de cada recurso.

La conversión de un modelo o una textura no sustituye la autoría original. Agradecemos a los modders que hicieron posible recuperar y mantener este patrimonio del simracing. Los [créditos y procedencias](CREDITS.md) distinguen los recursos ya utilizados, los colaboradores del paquete de origen y las atribuciones que todavía faltan por verificar.

## Base tecnológica y procedencia

| Componente | Referencia |
| --- | --- |
| Motor | **Speed Dreams 2.4.2**, versión declarada en [CMakeLists.txt](CMakeLists.txt) |
| Simulación utilizada | **simuv5**, adaptada para este proyecto |
| Código de referencia recibido | [brhndursun/speed-dreams-simuv5](https://github.com/brhndursun/speed-dreams-simuv5), revisión [`d8d9c179`](https://github.com/brhndursun/speed-dreams-simuv5/commit/d8d9c179a7c944405fa4fedda4249d4703f33749) |
| Proyecto original | [Speed Dreams](https://www.speed-dreams.net/), derivado de [TORCS](https://torcs.sourceforge.net/) |
| Preparación previa de contenidos | [Formula90s](https://github.com/Anthony38zrgo-2/formula-90), mediante conversión offline a formatos nativos |

La revisión de referencia fue comprobada directamente y declara también 2.4.2. Este repositorio inició su historial local con una instantánea adaptada: el commit de referencia **no es un ancestro Git** de este historial. La procedencia se documenta sin presentar las modificaciones como una versión oficial de Speed Dreams. Véase [el registro de procedencia](implementation/source_provenance.json).

## Estado actual

El prototipo local se centra en **Williams FW19, Jacques Villeneuve, variante Hockenheim, sobre Fuji76-77**, como sesión individual de práctica. Fuji76-77 es un circuito histórico de prueba; no se presenta como el circuito japonés del campeonato de 1997.

El trabajo existente incluye conversión visual y de materiales, contacto y colisiones de Fuji, controles de teclado/mando, calibración de geometría y suspensión, diferencial con precarga y ajuste separado de aceleración/retención, alas y fondo aerodinámico, y bancos de regresión nativos.

**El campeonato completo todavía no está implementado.** Tampoco hay un importador universal que cargue directamente cualquier mod F1C, rFactor o GP4. Las conversiones actuales son específicas, preparadas offline y requieren sus recursos de origen. La equivalencia física histórica, conducción y calibración final siguen en validación.

El nuevo modelo de neumáticos y térmica inspirado en Rust está implementado como candidato seleccionable para el Williams/Fuji en [implementation/reports/williams_coupled_tire_model.md](implementation/reports/williams_coupled_tire_model.md). La especificación de origen se conserva en [instrucciones.md](instrucciones.md). La activación no implica aceptación de conducción: la validación humana, la lluvia y las vueltas completas siguen pendientes. Los resultados y límites del trabajo actual están en [implementation/reports](implementation/reports).

## Investigación y referencias bibliográficas

Las fuentes de apoyo para investigar suspensión, dinámica del vehículo y neumáticos se conservan en [docs/investigation](docs/investigation/README.md), con citas completas, enlaces a los PDF y notas sobre su aplicación:

- **[INV-01]** Romano, L., Bruzelius, F., Hjort, M., y Jacobson, B. (2023). [*Development and analysis of the two-regime transient tyre model for combined slip*](docs/investigation/530309_Fulltext.pdf). *Vehicle System Dynamics*, 61(4), 1028–1062. [DOI: 10.1080/00423114.2022.2057335](https://doi.org/10.1080/00423114.2022.2057335).
- **[INV-02]** Gaspar, D. L. M. (2026). [*Modelo de Comportamento Dinâmico do Veículo Formula Student para otimização do Sistema de Suspensão*](docs/investigation/content.pdf). Proyecto de máster, Instituto Politécnico de Leiria.
- **[INV-03]** Díaz de Rábago Areilza, J. (2024). [*Analysis and design of a roll-heave decoupled front suspension for a Formula Student prototype*](docs/investigation/TFG-DiazdeRabagoAreilza,Jose.pdf). Trabajo Fin de Grado, Universidad Pontificia Comillas, ICAI.
- **[INV-04]** Gaither, A. (2021). [*Vehicle Dynamics on an Electric Formula SAE Racecar*](docs/investigation/gaither-gaither-sb-2-2021-thesis.pdf). Tesis de grado, Massachusetts Institute of Technology.

Estas referencias documentan las bases de investigación; sus resultados no implican una calibración o validación automática del vehículo del proyecto. Los documentos conservan sus propios avisos de autoría y condiciones de uso.

## Créditos principales del contenido utilizado

- **Williams FW19 y variante visual GP4:** VRC, MSchumi24 y Hega; actualizaciones de SkaningeN. El carset *F1 1997 Season Mod v2.0* está organizado por MSchumi24.
- **Cockpit:** Enzojz; texturas de MSchumi24. **Casco Arai y pintura Williams:** jvinu2000.
- **Llantas y neumáticos:** Ulf (Alesi_fan) y Turbo Lover.
- **[Fuji76-77](https://www.overtake.gg/downloads/fujji-1976-1977.79502/):** modificación del trazado y AIW por Gulag_Express. La autoría de la base queda pendiente porque el propio README original no la confirma.
- **Audio V10 de referencia:** [Ferrari F2002 Sound Mod](https://www.overtake.gg/downloads/btk150s-ferrari-f2002-sound-mod.52684/) de **kudos_kudos**, creado para el Ferrari F2002 de **btk150** en Assetto Corsa; procedencia indicada por el mantenedor. Es una adaptación sonora provisional, no una grabación certificada del Williams de 1997.
- **Carbon Fiber Black:** James Middleton, según los metadatos del material BlenderKit incorporado al GLB de origen.
- **Herramientas GP4:** Diego Noriega (Öggo), por GP4 Mesh Tools; ZaZ, por las herramientas del ecosistema GP4.
- **Motor y dependencias:** los equipos y colaboradores de Speed Dreams, TORCS y las bibliotecas incluidas, conservando sus avisos originales.

El [registro completo](CREDITS.md) incluye la relación de créditos del carset, adaptaciones intermedias y recursos con autor pendiente. No atribuimos el audio V10 del prototipo a los autores de sonido del carset sin demostrar que sea el mismo recurso.

## Código, contenido y licencias

El código de Speed Dreams conserva su licencia **GPL-2.0-or-later** y los avisos correspondientes; consulte [LICENSE](LICENSE) y los textos específicos de cada componente. Las aportaciones de este fork sobre esa base se distribuyen con esa misma licencia, salvo indicación específica compatible en el archivo correspondiente.

Los modelos, pistas, texturas y sonidos de terceros conservan sus propias condiciones. **Acreditar un recurso no concede permiso de redistribución ni lo convierte en GPL.** Este repositorio publica el código, las herramientas y la documentación de las adaptaciones; los paquetes locales de mods y la distribución jugable generada no se incluyen en este push. Las atribuciones y permisos incompletos quedan identificados en CREDITS.md.

El contenido base oficial de Speed Dreams y FreeSOLID se referencia mediante submódulos, con sus propias licencias. El paquete Williams/Fuji de la instalación de desarrollo no forma parte de esos submódulos.

## Obtener y compilar

```sh
git clone --recurse-submodules https://github.com/Anthony38zrgo-2/open-formula-racing.git
cd open-formula-racing
```

Las dependencias y compilación del motor se describen en el [README original de Speed Dreams](README_SPEED_DREAMS.md). Como punto de partida, con las dependencias instaladas:

```sh
cmake -S . -B build
cmake --build build --parallel
```

En Windows, la instalación de desarrollo usa CMake, Ninja y MSYS2 UCRT64, además de las dependencias gráficas y de audio de Speed Dreams. Ese entorno concreto no es un instalador portable incluido en Git.

Los scripts de `tools/` preparan el vehículo, pista, sesión y distribución desde fuentes locales. **Un clon de este repositorio no contiene los recursos externos necesarios para reproducir automáticamente el Williams/Fuji.** Los enlaces de créditos conducen a las publicaciones originales de sus autores; no se presupone autorización para redistribuir sus paquetes.

Cuando ya se ha generado una distribución local completa, el punto de entrada es `distribution/OpenFormula.exe`. No necesita Godot para ejecutar el port; Formula90s es una fuente de preparación y referencia, no el motor de juego de este fork.

## Contribuir

Se agradecen correcciones de física, herramientas de conversión, pruebas, documentación de procedencia y contacto con autores. Para incorporar contenido, registrar autor original, conversores, versión del mod, archivos usados, evidencia y condiciones de uso; separar esos créditos de los trabajos de adaptación.

Este es un proyecto comunitario independiente, sin afiliación oficial con Formula 1, FIA, Williams, fabricantes o los juegos de origen. Sus nombres y marcas pertenecen a sus respectivos titulares.
