# Comparación del neumático simuv5 con Rust

> Actualización posterior: el modelo acoplado seleccionable ya está implementado en `implementation/reports/williams_coupled_tire_model.md`. Este documento conserva la comparación previa al cambio y no describe el estado actual.

Revisión de código y pruebas nativas del 20 de octubre de 2026. Sin equivalencia histórica ni conducción validadas.

simuv5 usa una curva de fuerza común sobre la magnitud del deslizamiento, dividida entre ejes longitudinal y lateral. Rust usa curvas independientes y un presupuesto combinado. Rust Standard incluye memoria de pérdida/recuperación, migración de picos y rampa de contacto; los backends simplificados eliminan parte de esos estados.

El port aproxima el pico lateral a 0,115 rad y la caída asintótica a 0,88, conservando mu=2,7. Eso no reproduce la curva combinada, las diferencias longitudinales, el camber ni la respuesta transitoria de Rust. Las pruebas nativas actuales dan picos efectivos de aproximadamente 2,32 a 3,55 según carga/eje, sin penalización térmica. No demuestran aceleraciones del coche completo.

Presión: el usuario solicitó literalmente 160 psi; se guardaron 1103161,16691 Pa. El solver simuv5 interpreta presión absoluta y resta la atmosférica en sus ecuaciones térmicas: aproximadamente 145,3 psi manométricos con 101300 Pa atmosféricos. El perfil Rust usa 130/120 kPa manométricos delante/detrás en frío y 160/150 kPa como referencia caliente. No se ha cambiado el ajuste solicitado ni resuelto la diferencia de contrato.

La rigidez radial tireSpringRate se calcula en simuv5 a partir de presión y geometría, pero no se consume en el solver revisado. No atribuir un endurecimiento vertical real a ese valor calculado. Rust acopla presión, rigidez/amortiguación radial, deformación, radio efectivo y contacto. Su modelo térmico separa tres zonas de banda, carcasa y gas; simuv5 mantiene una temperatura global.

Con 160 psi, las pruebas de fuerzas y energía siguen pasando. En el escenario sintético de frenada de 60 s, el delantero termina aproximadamente a 102 C frente a 82 C con la presión anterior. El banco controla fuerzas y deslizamiento: no es una vuelta conducida ni una validación de estabilidad del contacto.

Pendientes: definir unidades absolutas/manométricas coherentes con la interfaz, revisar acoplamiento de carcasa, comparar curvas combinadas y transitorios bajo iguales cargas/temperaturas/presiones y validar en pista. No aumentar mu sin esa evidencia.

Fuentes: src/modules/simu/simuv5/wheel.cpp; D:/Formula90s/game/crates/vehicle-physics-engine/src/tire.rs; tire_thermals.rs; wheel_mechanics.rs y perfil Williams hockenheim/tires.json. La revisión de Rust fue estática; no se afirma haber recompilado ni ejecutado ese proyecto.
