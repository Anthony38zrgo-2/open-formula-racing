# Oscilación en reposo del Williams: Etapa B, Puerta 1

Fecha: 20 de octubre de 2026. Rama: `codex/initial-port`. HEAD: `8531ccb1de40a79390f6140ee2eae4fb08f8cf54`.

Este informe cubre únicamente la Puerta 1 del encargo de Etapa B: geometría y unidades del adaptador vertical. No declara resuelto el solver de siete grados de libertad, no conecta la ruta nueva a `wheel.cpp` y no modifica la producción. La referencia de Etapa A sigue siendo la configuración efectiva.

## 1. Resultado

- Adaptador geométrico puro creado en `src/modules/simu/simuv5/tire_vertical_geometry.cpp/.h`.
- Banco `tools/validate_formula_tire_geometry.cpp`: **144/144 PASS**.
- Valores cargados del XML del Williams y recentrado al centro de gravedad comprobados.
- `simuv5` compila con el módulo nuevo.
- Los bancos existentes siguen pasando: selector 6/6, contacto 13/13, fuerzas 21/21, térmica 25/25, energía 27/27, vertical 10/10 y chasis 6/6.
- DLL y XML de producción sin cambios: SHA-256 `B14FB36D...9927` y `66FD0A8D...A426`, respectivamente.

## 2. Contrato geométrico

Por rueda, en metros, segundos y radianes:

- `H`: altura de la referencia del chasis que publica `wheel->pos.z`.
- `G`: altura real de carretera, sin desplazarla por deflexión.
- `R`: radio geométrico descargado (`wheel->radius`).
- `L`: extensión física de suspensión, positiva cuando la rueda baja respecto al chasis.
- `C`: altura mundial del centro físico de rueda.
- `d`: deformación radial con signo, positiva al comprimir.
- `b`: relación de bellcrank.

Identidades implementadas:

    C = H + R - L
    d_signed = G + R - C = L - (H - G)
    d_publicada = max(d_signed, 0)
    C_velocity = H_velocity + R_velocity - L_velocity
    d_velocity = G_velocity + L_velocity - H_velocity

La transformación de `H` replica la de `SimCarUpdateWheelPos`:

    H = centro_mundial_del_GC - altura_local_del_GC
        - wheel_x * sin(pitch) + wheel_y * sin(roll)

    H_velocity = velocidad_vertical_del_GC
        - wheel_x * cos(pitch) * pitch_velocity
        + wheel_y * cos(roll) * roll_velocity

El banco comprueba la derivada por diferencia finita usando la misma transformación. No se usa `wheel_x`/`wheel_y` brutos: son los `staticPos` ya recentrados al GC.

## 3. Adaptador hacia simuv5

Salidas del adaptador:

    susp.x = b * L
    susp.v = -b * L_velocity
    susp.a legacy = +b * L_acceleration
    aceleración física de compresión = -b * L_acceleration

`L` y `L_velocity` son el estado autoritativo. `d`, `C` y sus velocidades se calculan desde la misma geometría; el adaptador no integra ni recorta estados. En vuelo conserva `L` y `L_velocity` y publica `d = 0` sin borrar el estado. `geometric_contact` es `d_signed >= 0`.

## 4. Valores cargados del Williams

Leídos con `GfParmReadFile` desde `implementation/candidates/williams-nineteen-ninety-seven/williams-nineteen-ninety-seven.xml`:

| Dato | Valor |
| --- | ---: |
| Reparto delantero-trasero | 0.445 |
| Vía delantera, `ypos` | ±0.834987 m |
| Vía trasera, `ypos` | ±0.800000 m |
| Eje delantero, `xpos` | +1.146992 m |
| Eje trasero, `xpos` | -1.743008 m |
| `statGC.x` calculado | -0.456958 m |
| `statGC.y` calculado | 0.000000 m |
| `wheel_x` delantero recentrado | +1.603950 m |
| `wheel_x` trasero recentrado | -1.286050 m |
| Radio geométrico delantero | 0.311949 m |
| Radio geométrico trasero | 0.322016 m |
| Bellcrank cargado | 1.0 |
| Ride height de suspensión, `L` de referencia | 0.050000 m |

El XML almacena los números como coma flotante de 32 bits. Por eso los valores recientrados difieren de los decimales nominales en hasta unos `1.7e-8 m`; el banco usa tolerancia `1e-7 m` para los valores cargados y tolerancias de `1e-15` para las identidades algebraicas.

La deflexión de 8.6 mm del ejemplo y la de 9.0 mm usada en el eje trasero son valores de prueba geométricos. No se declaran como equilibrio estático final del Williams.

## 5. Cobertura de la Puerta 1

El banco comprueba:

- Ejemplo de 50 mm y 8.6 mm: `L = 0.0586 m`, `C - G = 0.3034 m`, identidad exacta dentro de precisión numérica.
- Plano, alturas conocidas, extensión y compresión.
- `b = 1`, `b = 0.8759227555360516` y `b = 0.37049714771142384`.
- Heave, roll y pitch de ambos signos con las cuatro ruedas cargadas.
- Traslación conjunta de chasis y carretera: `d`, `d_velocity` y carga lineal sin cambio; `C` se traslada.
- Movimiento de carretera y de chasis por separado: ambos aumentan `d`, pero solo el chasis mueve `C`.
- Distinción entre velocidad de compresión `d_velocity` y velocidad de suspensión `susp.v`; una subida de carretera no genera `susp.v`.
- Vuelo con chasis quieto y con chasis moviéndose: `C` sigue su dinámica y `L`/`L_velocity` no se pierden por el signo de `H - G`.
- Frontera de contacto: `d_signed = 0` toca; `d_signed < 0` no toca y publica cero.
- Velocidad de radio: entra en `C_velocity` y se cancela en `d_velocity`.
- Adaptador: signos de `susp.x`, `susp.v`, `susp.a` legacy y aceleración física de compresión.

## 6. Comandos y evidencias

Compilación:

    cmake --build C:/Users/bill/open-formula-build/engine --target validate_formula_tire_geometry simuv5

Ejecución:

    validate_formula_tire_geometry.exe

Resultado: `checks=144 failures=0`.

Evidencias externas en `D:/open-formula-migration-evidence/vertical-stage-b-gate1-20261020/`:

- `tire-geometry-bench.log`: salida completa del banco.
- `working-tree-before.patch`, `working-tree-before-hashes.json` y `copies/`: respaldo previo a esta intervención.
- `git-status-after.txt` y `git-diff-after.patch`: estado posterior.

El `simuv5.dll` compilado en el directorio de build tiene SHA-256 `0F4E93FD6EC91FD8301977AABF6597428F0026E59F97AA0CEBF0DCAD7CD442C5`. No se copió a `distribution`.

## 7. Límites

- El adaptador no está llamado todavía desde `SimWheelUpdateRide`, `SimWheelUpdateForce`, `SimCarUpdateWheelPos` ni `SimAxleUpdate`. La producción sigue en la ruta de Etapa A.
- No existe aún el bloque conjunto de siete grados de libertad ni la evaluación pura de la suspensión nativa. La interfaz ambigua `total_travel`/`compression` de `tire_vertical_model` sigue pendiente de sustitución en la Puerta 2.
- La aproximación de ángulos pequeños no incluye acoplamiento de guiñada ni normal local de Fuji. La distancia/contacto respecto a la normal y el Jacobiano quedan para la integración completa.
- No se ha ejecutado el coche completo ni se ha instalado ninguna DLL experimental.

La Puerta 2 debe usar este adaptador como única fuente de geometría, implementar el bloque de cuarto de coche y después el de cuatro ruedas con heave/roll/pitch, y validar energía y leyes nativas antes de conectar Fuji.
