"""
Test script para ejecutar rally.exe por unos segundos y verificar runtime.log
"""

import os
import time
import subprocess
from pathlib import Path

DIST = Path(r"D:\open-formula\distribution")
RALLY_EXE = DIST / "rally.exe"
LOG_PATH = DIST / "user-settings-rally/runtime.log"

# Limpiar log anterior si existe
if LOG_PATH.exists():
    LOG_PATH.unlink()

print("Iniciando rally.exe...")
proc = subprocess.Popen([str(RALLY_EXE)], cwd=str(DIST))

print("Esperando 12 segundos para inicializacion de mapa, coche y fisicas Rust...")
time.sleep(12)

# Terminar speed-dreams-2 si esta corriendo
subprocess.run(["taskkill", "/F", "/IM", "speed-dreams-2.exe"], capture_output=True)
subprocess.run(["taskkill", "/F", "/IM", "rally.exe"], capture_output=True)

print("Proceso finalizado. Analizando runtime.log...")
if LOG_PATH.exists():
    log_text = LOG_PATH.read_text(encoding="utf-8", errors="replace")
    print(f"Log encontrado ({len(log_text)} caracteres).")
    
    # Buscar lineas clave
    keywords = ["Rust physics", "ford-escort", "russia-stage1", "TIRE_BACKEND_READY", "visual track", "error", "Error", "FAIL", "panicked"]
    for line in log_text.splitlines():
        if any(k.lower() in line.lower() for k in keywords):
            print("  LOG:", line)
else:
    print("ALERTA: runtime.log no fue creado.")
