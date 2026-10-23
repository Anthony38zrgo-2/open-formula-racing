# Telemetría temporalmente desactivada

El 23 de octubre de 2026 se desactivaron `diagnostic telemetry` y `diagnostic trace` en la configuración local `distribution/user-settings/config/rust_physics.xml`. Esto detiene los resúmenes RUSTDIAG/RUSTSUSP/RUSTAXLE, el informe diagnóstico del garaje y las capturas de eventos. La captura Rust parte desactivada y solo se habilita cuando ambos interruptores están activos. No se inicia el escritor ni se llena el anillo de trazas cuando están apagados.

Desde la raíz del repositorio:

```powershell
python tools/set_physics_telemetry.py
python tools/set_physics_telemetry.py --check
# Reactivación explícita cuando vuelva a ser necesaria:
python tools/set_physics_telemetry.py --enable
```

El cambio se lee al iniciar el juego. El aplicador conserva la configuración restante y crea una copia antes de modificarla. `--config` permite indicar otra instalación. La configuración instalada es local y no se versiona; el aplicador permite reproducir el cambio. Se conservan las capturas anteriores, los datos necesarios para HUD/sonido y los mensajes de error operativos.

Validación: ambos interruptores comprobados en cero; 51 regresiones de física y telemetría pasan, el objetivo CMake `rustphysics` compila y la DLL instalada ejecuta 2000 pasos con cero registros de traza sin habilitación explícita. No se inició una sesión de conducción.
