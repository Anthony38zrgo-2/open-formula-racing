"""Summarize the read-only Rust probe and the preserved driving session."""
from pathlib import Path
import csv
import hashlib
import json

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
curve = list(csv.DictReader((OUT / "spring_curve.csv").open()))
modal = list(csv.DictReader((OUT / "modal_stiffness.csv").open()))
telemetry = json.loads((ROOT / "implementation/reports/telemetry_2026_10_23/latest_test_102453/parsed.json").read_text())
rows = {r["time"]: r for r in telemetry["rows"]}
result = {"method": "430 production-path spring evaluations; finite difference epsilon=1e-6 m; interval=1e-5 s; no road or chassis integration", "curves": {}, "runtime": {}, "provenance": {}}
for name in ["FrontLeft", "FrontRight", "RearLeft", "RearRight"]:
    data = [{k: float(v) for k, v in r.items() if k != "wheel"} for r in curve if r["wheel"] == name]
    result["curves"][name] = {
        "samples": len(data),
        "max_spring_force_error_vs_exact_pct_above_200n": max(abs(r["spring_n"] / r["exact_spring_n"] - 1) * 100 for r in data if abs(r["exact_spring_n"]) > 200),
        "max_virtual_work_error_pct_above_200n": max(abs(r["energy_gradient_error_n"] / r["spring_n"]) * 100 for r in data if abs(r["spring_n"]) > 200),
        "max_exact_virtual_work_error_n": max(abs(r["exact_energy_gradient_error_n"]) for r in data),
        "selected": [min(data, key=lambda r: abs(r["q_input_m"] - q)) for q in [-0.055, -0.05, -0.045, 0.0, 0.015, 0.024]],
    }
for wheel, ref in [("FL", .095), ("FR", .095), ("RL", .1094), ("RR", .1094)]:
    data = [r for r in telemetry["susp"] if r["wheel"] == wheel and float(rows[r["time"]]["speed"]) > 5]
    q = sorted(float(r["comp_m"]) - ref for r in data)
    stop = sorted(float(r["stop_n"]) for r in data)
    result["runtime"][wheel] = {
        "samples": len(data),
        "q_p5_p50_p95_m": [q[int((len(q) - 1) * p)] for p in [.05, .5, .95]],
        "progressive_stop_force_above_1n_fraction": sum(float(r["stop_n"]) > 1 for r in data) / len(data),
        "progressive_stop_force_p95_n": stop[int(len(stop) * .95)],
    }
receipt = json.loads((ROOT / "implementation/reports/telemetry_2026_10_23/installation.json").read_text())
for name, expected in receipt["source_sha256"].items():
    actual = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
    assert actual == expected, name
result["provenance"]["receipt_source_hashes_matched"] = len(receipt["source_sha256"])
for name in ["rust/target/release/vehicle_physics_engine.dll", "distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll", "rust/target/release/deps/libvehicle_physics_engine.rlib"]:
    result["provenance"][name] = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
assert result["provenance"]["rust/target/release/vehicle_physics_engine.dll"] == result["provenance"]["distribution/lib/games/speed-dreams-2/modules/simu/vehicle_physics_engine.dll"]
assert all(abs(float(r["bar_sym_sum_n"])) < 1e-8 and abs(float(r["bar_heave_n"])) < 1e-8 for r in modal)
result["modal_rows"] = modal
(OUT / "metrics.json").write_text(json.dumps(result, indent=2), encoding="utf-8")

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
fig, axes = plt.subplots(2, 2, figsize=(11, 7.2), sharex="col")
fig.suptitle("Williams: interacción muelle–barra en la suspensión Rust", fontsize=15, weight="bold")
for col, name, title, kref, ratios, observed in [
    (0, "FrontLeft", "Eje delantero", 103040.422473, [1., .3, .15, .12], [-61.6, -22.0]),
    (1, "RearLeft", "Eje trasero", 19601.889887, [.8, .24, .09], [-50.9, 27.2]),
]:
    data = [r for r in curve if r["wheel"] == name]
    x = [float(r["q_end_m"]) * 1000 for r in data]
    force = [float(r["spring_n"]) / 1000 for r in data]
    k = [float(r["k_spring_fd_n_m"]) for r in data]
    axes[0, col].plot(x, force, color="#176d9c", lw=2)
    axes[0, col].set_title(title)
    axes[0, col].set_ylabel("Fuerza del muelle en rueda (kN)")
    for ratio in ratios:
        beta = [100 * 2 * kref * ratio / (v + 2 * kref * ratio) for v in k]
        axes[1, col].plot(x, beta, lw=1.9, label=f"r = {ratio:.2f}")
    axes[1, col].axhline(50, color="#b73636", lw=1, ls="--")
    axes[1, col].set_ylabel("Cuota local de barra\nen rigidez de roll (%)")
    axes[1, col].set_ylim(0, 100)
    axes[1, col].set_xlabel("Recorrido desde la posición de diseño q (mm)")
    axes[1, col].legend(fontsize=8, loc="lower right")
    for ax in axes[:, col]:
        ax.axvspan(*observed, color="#176d9c", alpha=.08)
        ax.axvline(0, color="gray", ls=":", lw=1)
        ax.grid(alpha=.2)
        ax.set_xlim(-80, 30)
fig.text(.5, .015, "Sombreado: percentiles 5–95 del recorrido de la rueda izquierda a >5 m/s.\nCuota elástica local con esquinas simétricas; excluye topes, neumáticos, amortiguación y transferencia geométrica.", ha="center", fontsize=8)
fig.tight_layout(rect=(0, .07, 1, .95))
fig.savefig(OUT / "spring_bar_comparison.png", dpi=170)
print("Wrote metrics.json and spring_bar_comparison.png")
