# Rally Rusia — poder salir de la pista (2026-10-27)

**Síntoma:** el coche no podía salirse de la trazada; había una colisión a pocos metros que
impedía atajar, tomar los baches o llegar a los árboles cercanos. Se pedía poder salir hasta
unos 50 m (o algo menos) y que los árboles cercanos fueran sólidos.

## 1. Causa

No era un muro invisible puesto a propósito ni los árboles: era el **propio terreno**.

El clasificador convertía en obstáculo cualquier triángulo **inclinado** de un objeto
transitable:

```rust
// ac3d_track.rs, antes
} else if is_driveable_kw {
    if ny_abs >= 0.55 { driveable }
    else if height_span >= 0.35 {
        // "retaining wall or embankment" -> obstáculo
        obstacles.push(...)
    }
}
```

La hierba/terreno del arcén y el talud entre la carretera y el terreno exterior tienen triángulos
con normal inclinada. Todos esos triángulos se convertían en muros 2.5D. El resultado era un muro
**continuo a 5–10 m** de la trazada, con la misma cota que el suelo, que el coche no podía cruzar.

Medido sobre la trazada real (49 muestras, distancia al muro más cercano):

| Distancia al muro más cercano | Antes |
|---|---:|
| < 3 m | 0 |
| 3–5 m | 7 |
| **5–10 m** | **42** |
| 10–20 m | 0 |
| 20–50 m | 0 |

Las muestras de esos muros tenían `y` de 1–2 m de altura y coincidían con la cota del terreno
(p. ej. `y=[7.88, 9.11]` para una trazada a `y=8.0`): eran taludes, no árboles. Los troncos
estaban detrás del muro y por eso tampoco se podía chocar con ellos.

## 2. Corrección

### 2.1 El terreno/grava es suelo, no muro

En `ac3d_track.rs` se añadió `is_open_ground_element`: los objetos de tipo terreno, hierba,
carretera o grava son **suelo aunque el triángulo sea inclinado**. Un talud, una cuneta o un
bache ya no genera un muro; el coche puede subir/bajar por él. Las barreras de carretera
(`roadbar`) y las vallas siguen siendo obstáculos.

Resultado del mundo de colisión:

| Magnitud | Antes | Ahora |
|---|---:|---:|
| Triángulos transitables | 81.002 | **142.199** |
| Triángulos de obstáculo | 76.309 | **15.750** |
| Segmentos de muro | 60.896 | **16.564** |
| Tiempo de construcción | 1,4 s | **0,96 s** |
| Caché | 14,45 MB | **14,03 MB** |

Al dejar de convertir el terreno en muro, el campo de muros vuelve a contener solo objetos
sólidos: troncos, rocas, señales, graneros, postes, tiendas, vallas, casas.

### 2.2 Borde invisible solo donde termina el suelo

Quitar el muro del arcén permite salirse, pero al final del terreno puede haber un vacío. Para
evitar caer sin volver a poner una pared a 5 m se añadió un **borde de colisión en el límite real
del terreno**:

- Se calculan las aristas del **mesh transitable** que solo pertenecen a un triángulo.
- Se descartan las que tienen suelo a ambos lados (juntas carretera/hierba, solapes).
- Se descartan las que están a **menos de 15 m de la carretera**: así las cunetas, baches y
  taludes cercanos quedan libres y se pueden tomar.
- Las que quedan (borde exterior del stage o huecos alejados) reciben un muro 2.5D invisible de
  ~3,5 m de alto. El coche no cae al vacío, pero puede recorrer todo el terreno hasta ese borde.

Ese borde son ~2.460 segmentos; el resto (~14.100) son objetos sólidos.

### 2.3 Árboles

Los troncos (`tree_visual_russian_tree_trunks`), graneros, postes, señales, rocas y tiendas ya
se clasificaban como obstáculos desde la vuelta anterior; ahora que no hay muro de terreno delante,
el coche **sí puede llegar y chocar** con ellos. En el muestreo de la trazada los muros más
cercanos quedan a 3,7–20 m y son objetos (el más próximo, un objeto bajo de 0,65 m —señal/roca—;
los siguientes, troncos de 3–24 m de alto). No queda ningún muro continuo de terreno.

## 3. Cobertura y el límite de 50 m

La cobertura de suelo fuera de la pista no cambia respecto al informe anterior (es geometría, no
clasificación):

| Lateral | Sin suelo |
|---:|---:|
| 3 m | 0,0 % |
| 5 m | 2,6 % |
| 8 m | 7,5 % |
| 12 m | 0,6 % |
| 20 m | 14,6 % |
| 30 m | 27,4 % |
| 50 m | 32,4 % |

El `.acc` exportado no tiene terreno continuo hasta 50 m en todas las direcciones: en muchas
zonas el suelo llega a 20–50 m; en otras termina a 15–20 m. El borde invisible se coloca en el
**borde real del suelo** (a partir de 15 m de la carretera), no a 5 m. Por eso:

- puedes salirte, atajar, tomar baches y cunetas cercanas;
- puedes llegar a los árboles/rocas y chocar con ellos;
- no puedes caer al vacío en el borde;
- **no se garantizan 50 m en todas las direcciones** porque el terreno visual no existe más allá.
  Si se quieren 50 m garantizados hay que **generar/extender terreno** (o poner una zona de
  recuperación a 50 m); es un cambio de datos, no de clasificación.

## 4. Verificación

- `cargo test --test wall_field_tests`: **12/12**, incluido el test nuevo que comprueba que un
  plano transitable genera borde invisible y que un plano sin obstáculos no genera muros de
  terreno.
- `cargo test --lib ac3d_track`: 3/3.
- Validación nativa del tramo con el Ford: `TRACK_VALID` (142.199 transitables, 15.750 obstáculos,
  16.564 segmentos); 600 pasos, `rejected=0`.
- La DLL instalada carga con ABI 7 (`ofp_abi_signature=0xA91CDEBA9D3FE775`).

## 5. Instalación

| Artefacto | SHA-256 |
|---|---|
| `distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll` (off-track fix) | `6209E3E193DE01DCC9A3C88EB0666C66227C9C538E1E69CA45A5942D99A1E7CE` |
| DLL anterior (fix de clasificación/terreno) | `_baseline/vehicle_physics_engine_turn2_fixed.dll` |
| Copia de la DLL nueva | `_baseline/vehicle_physics_engine_offtrack_fixed.dll` |
| Caché del tramo (`OFP_TRACK_CACHE7`) | `_baseline/russia-stage1.acc.ofpcache` |

El juego no estaba en ejecución; **reinícialo** para cargar la DLL y la caché. Reversión: copiar
`_baseline/vehicle_physics_engine_turn2_fixed.dll` sobre la DLL instalada.

## 6. Reproducción

```powershell
# Distancia de la trazada al muro más cercano y ejemplos
cargo run --manifest-path rust/vehicle-physics-engine/Cargo.toml --example stage_wall_probe

# Cobertura de suelo
cargo run --manifest-path rust/vehicle-physics-engine/Cargo.toml --example stage_route_coverage

# Mundo y tiempos
cargo run --release --manifest-path rust/vehicle-physics-engine/Cargo.toml `
  --example wall_field_stats -- `
  distribution/share/games/speed-dreams-2/tracks/circuit/russia-stage1/russia-stage1.acc
```

La parte térmica del Ford sigue como se documentó en
`implementation/reports/rally_russia_2026_10_27/README.md` (75/20/110/65 °C).

