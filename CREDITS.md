# Créditos y procedencia del contenido

Open Formula Racing reconoce la autoría original de los recursos adaptados. Esta lista separa el contenido utilizado en el prototipo local del conjunto de colaboradores de sus paquetes de origen. Fecha de revisión: 20 de octubre de 2026. Los recursos de mods no se publican en este repositorio de código.

## Motor, herramientas y adaptación

- **Speed Dreams Team y sus colaboradores:** motor Speed Dreams 2.4.2, herramientas, interfaz, renderizado y simulación. Referencia recibida: [brhndursun/speed-dreams-simuv5](https://github.com/brhndursun/speed-dreams-simuv5), commit d8d9c179a7c944405fa4fedda4249d4703f33749. Se reconoce a brhndursun como mantenedor del repositorio de referencia, sin atribuirle individualmente todo Speed Dreams.
- **TORCS Team y colaboradores:** origen del motor del que deriva Speed Dreams. Los avisos de autor de los archivos y licencias heredadas se conservan.
- **Autores del shader de coche de Speed Dreams:** `implementation/shaders/car_material_appearance.frag` adapta `speed-dreams-data/data/data/shaders/car.frag`; se conserva la referencia al submódulo y sus avisos/licencias.
- **Autores de OpenSceneGraph, SDL, OpenAL, PLIB, FreeSOLID y demás dependencias:** según las licencias y avisos conservados en sus componentes y `packaging/3rdParty-devel/Licenses/`.
- **Anthony38zrgo-2 / Formula90s y colaboradores:** preparación intermedia de perfiles, alineación, contenido derivado y herramientas del proyecto. Esto no reemplaza los créditos de mallas, texturas ni audio originales.
- **Diego Noriega (Öggo):** GP4 Mesh Tools para Blender; atribución registrada en el `gp4form.txt` del bundle local V2.3 y en el registro de terceros de Formula90s.
- **ZaZ:** CSM, TSM y herramientas reconocidas en los créditos originales del carset GP4. No se afirma que dichas herramientas formen parte del runtime de Speed Dreams.

## Recursos utilizados en el prototipo Williams/Fuji

| Recurso | Autoría o adaptación acreditada | Evidencia y alcance |
| --- | --- | --- |
| Williams FW19, ruedas de la variante y decoración Hockenheim | **VRC, MSchumi24 y Hega; actualizaciones de SkaningeN** | `credits.txt` del F1 1997 Season Mod v2.0 y source_inventory.json del Williams |
| Organización del F1 1997 Season Mod v2.0 | **MSchumi24** | `help.txt`, versión 2.0, fechado 24/04/2024 |
| Cockpit e interior | **Enzojz**; texturas **MSchumi24** | Créditos del carset y procedencia del Williams |
| Modelo de casco Arai y texturas Williams/Villeneuve | **jvinu2000** | Créditos del carset y procedencia del piloto adaptado |
| Llantas y neumáticos del conjunto | **Ulf (Alesi_fan) y Turbo Lover** | Créditos `gy-oz-blck-yllw` y del carset; se conservan junto a los créditos del coche |
| Fuji76-77: modificación del trazado, curva 1 y nuevo AIW | **Gulag_Express** | [Publicación original](https://www.overtake.gg/downloads/fujji-1976-1977.79502/) de Gulag_Express y README incluido en la carpeta utilizada por la conversión |
| Base anterior de Fuji | **Autor original pendiente de confirmar** | El README menciona una posible procedencia de F1C/VirtuaLM, pero declara expresamente que no la conoce con certeza. No presentamos VirtuaLM como atribución confirmada |
| Material Carbon Fiber Black en los materiales UniformBlenderKitCarbonFiber | **James Middleton**, BlenderKit | Metadatos `asset_data` del GLB: nombre/autor y assetBaseId `abd33787-1667-41cb-baf3-9c7409e74517`; revisión identificada `125ce3f5-13bd-4276-b288-7014b3482af8` |
| Mapa restaurado Carbon_Fiber_diffuse.jpg | Biblioteca **BlenderKit**; autor individual del archivo pendiente de vincular | Su uso está registrado en source_inventory.json; no se presupone que sea el mismo asset de James Middleton solo por compartir nombre |
| Volante, display, fijación y levas de la adaptación | Programa de vehículo **Formula90s F1 2030**, como donante intermedio | El registro del Williams documenta ese donante; la cadena de autoría anterior de cualquier pieza de terceros debe completarse |
| Brazos, guantes y elementos HANS del piloto | Programa de piloto **Formula90s**, a partir del paquete `race-car-driver-low-poly.zip` | Autor original del paquete todavía pendiente. Su manifiesto registra la fuente y hashes, no un nombre de autor |
| Apariencia de mangas/guantes del Williams | Adaptación de Formula90s y generación de atlas a partir del donante | El inventario registra generación con image_gen; no se atribuye como pintura original del carset |
| Única muestra de motor usada en el port: engine_powered_maximum_loop.wav | **kudos_kudos**, autor del mod de sonido; **btk150**, autor del coche F2002 de referencia; preparación de banco **Formula90s** | [Ferrari F2002 Sound Mod](https://www.overtake.gg/downloads/btk150s-ferrari-f2002-sound-mod.52684/) para Assetto Corsa, procedencia confirmada por el mantenedor; la página atribuye el sonido a kudos_kudos. La versión/variante exacta del paquete utilizado aún no se ha cotejado. Derivada de `engine_on_high.wav`; hash de salida `9bec0ea0aef8424b7062ab18471e849e660c906c609848a5a3adb8ff1411414c`. No equivale al banco completo del carset |

La restauración de textura, adaptación del shader, conversión de UV y ajuste de materiales de Open Formula Racing no transfieren la autoría de esos recursos.

## Créditos completos del paquete GP4 de referencia

La siguiente relación conserva los nombres registrados en `Data/Files/Documentation/credits.txt` del **F1 1997 Season Mod v2.0**. **No implica que todos estos coches, sonidos, HUD, personal o herramientas ya estén incorporados en Open Formula Racing.** Sirve para mantener la atribución del paquete y orientar futuras adaptaciones.

| Coches | Créditos del paquete |
| --- | --- |
| Arrows | Kefla, MSchumi24 y Hega; actualizaciones de SkaningeN |
| Williams y Ferrari | VRC, MSchumi24 y Hega; actualizaciones de SkaningeN |
| Benetton, Prost, Sauber, Tyrrell, Minardi y Stewart | Conversión desde F1C 1997 por Intweb; actualizaciones de quickslick y SkaningeN. Autores originales del mod F1C no individualizados en esta fuente |
| McLaren | Soulbringer; actualizaciones de SkaningeN |
| Jordan | Hega |
| Lola | Conversión desde F1C 1997; actualización de Turbo Lover; conversor no identificado en el archivo |

| Otros recursos | Créditos del paquete |
| --- | --- |
| Cockpit y texturas | Enzojz; MSchumi24 |
| Casco Arai | jvinu2000 |
| Pinturas de casco | jvinu2000 —Ferrari y Williams—, BlackToof, Hega, SkaningeN, DaiMon, KerleyF1 y MailsonTeles —Larini— |
| Personal y accesorios de boxes | MSchumi24 |
| Comisarios | Fongu |
| Paneles de boxes | Tony (b-tone) |
| Llantas y neumáticos | Ulf (Alesi_fan), Turbo Lover |
| Volantes | Hega; volante interior genérico de Genison Kobe |
| Viseras | MSchumi24 |
| HUD 1994 | Duffer |
| Defpanel 2009 | Tony (b-tone), Ruboy |
| Teamart | Tony (b-tone), MSchumi24 |
| Pantallas de carga | SkaningeN, MSchumi24 |
| Física y rendimiento del carset | Gitanes; no se atribuye a él la calibración nativa simuv5 de este fork |
| Sonidos del carset | Mitsuro Sano; esto no identifica por sí solo al autor de la muestra V10 usada aquí |
| Sonidos de eventos | carl_gpgames |
| Bruma atmosférica | Isaint |
| GPaedia | petrus |
| Fondos de menú | MSchumi24 |
| Música de fondo | Procedente de Formula One 97, según el carset; no incorporada al port |
| Parche de memoria | Lee71 |
| Configuración CSM | MSchumi24, petrus, Flash Formula y colaboradores no individualizados |
| Trackpack | plodekk y cleberpister; no se les atribuye Fuji76-77 por esa mención |

## Evidencias y permisos pendientes

Las atribuciones se verificaron contra archivos locales, cuyo nombre y SHA-256 se conservan en [implementation/source_provenance.json](implementation/source_provenance.json): documentación GP4, inventario Williams, README Fuji, procedencia del piloto y manifiestos de audio. No se han incluido sus paquetes de medios.

El `help.txt` del carset mantiene los derechos en los autores originales. El README de Fuji declara incierta la autoría de su base. Los metadatos de Carbon Fiber Black indican `royalty_free`; esa etiqueta no es una licencia GPL ni una autorización general para subir las texturas por separado. No se declara resuelta la redistribución del paquete jugable.

Para cerrar los créditos faltan la identificación del autor original de la base Fuji, el mapa de carbono restaurado, el piloto low-poly, la cadena del donante de volante cuando corresponda y la versión/variante exacta del paquete de sonido F2002. El autor del mod de sonido sí queda identificado como kudos_kudos; no atribuimos a btk150 su autoría sonora. Cualquier futuro mod F1C/rFactor/GP4 debe traer su propia relación de autores, conversores, versión y condiciones; no basta con acreditar al último conversor.

Si eres autor o puedes aportar documentación, abre una incidencia en este repositorio para corregir la atribución o aportar la referencia correspondiente. Se conservarán los avisos originales y se actualizará esta relación sin inventar permisos o autorías.

Fuentes públicas contrastadas: [sonido F2002](https://www.overtake.gg/downloads/btk150s-ferrari-f2002-sound-mod.52684/) y [Fuji76-77](https://www.overtake.gg/downloads/fujji-1976-1977.79502/). La declaración del mantenedor vincula el audio local al mod F2002; verificar esa cadena a nivel de archivo/variante antes de preparar una distribución de medios.
