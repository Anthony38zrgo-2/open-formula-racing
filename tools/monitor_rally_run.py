"""
Monitor de ejecucion de rally.exe para observar la carga completa de russia-stage1.acc y Rust physics
"""

import time
import subprocess
from pathlib import Path

DIST = Path(r"D:\open-formula\distribution")
RALLY_EXE = DIST / "rally.exe"
LOG_PATH = DIST / "user-settings-rally/runtime.log"

if LOG_PATH.exists():
    LOG_PATH.unlink()

print("Lanzando rally.exe...")
proc = subprocess.Popen([str(RALLY_EXE)], cwd=str(DIST))

last_len = 0
for second in range(45):
    time.sleep(1)
    if LOG_PATH.exists():
        text = LOG_PATH.read_text(encoding="utf-8", errors="replace")
        if len(text) != last_len:
            print(f"[{second}s] Log creciendo: {len(text)} bytes (ultimas lineas):")
            lines = [l.strip() for l in text.splitlines() if l.strip()]
            for l in lines[-3:]:
                print(f"    {l}")
            last_len = len(text)
            if "Rust physics: profile" in text:
                print(">>> Exito! Se alcanzo la carga del perfil y ground world de Rust!")
            if "Practice now in RUN state" in text or "race in progress" in text.lower():
                print(">>> Carrera iniciada exitosamente!")
                break

print("Finalizando proceso de prueba...")
subprocess.run(["taskkill", "/F", "/IM", "speed-dreams-2.exe"], capture_output=True)
subprocess.run(["taskkill", "/F", "/IM", "rally.exe"], capture_output=True)
