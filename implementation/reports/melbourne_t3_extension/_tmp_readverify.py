import json
from pathlib import Path

p = Path(r"D:\open-formula\implementation\reports\melbourne_t3_extension\verification\verification.json")
data = json.loads(p.read_text())
print("RIG:", data["rig"])
for k, v in data["dlls"].items():
    print(k, v["provenance"], v["sha256"][:16])
print()
for label, d in data["results"].items():
    line = f"{label:<40} maxdiff={d['maximum_absolute_difference']!r:>14} identical={d['identical_numeric_trajectory']!s:>5}"
    if "front_right_unloaded_samples" in d:
        u = d["front_right_unloaded_samples"]
        line += f" FR_unloaded {u['reference']}->{u['result']}"
    if "matches_the_equivalent_physical_element" in d:
        line += f" same_element={d['matches_the_equivalent_physical_element']}"
    print(line)
print()
reg = data["results"]["regression_design_builds_agree"]
print("regression keys:", {k: v for k, v in reg.items() if k not in ('installed_dll', 'fixed_dll')})
saved = data["results"]["saved_regraje_installed_versus_fixed"]
print("saved regraje:", {k: v for k, v in saved.items() if k != 'note'})
print("replayed:", {k: v for k, v in data["results"]["replayed_saved_regraje"].items()})
print("\ncontrols:")
for label, d in data["results"].items():
    if "control" in d:
        print(f"  {label:<16} {d['control']} = {d['requested']} -> {d['resolves_to']}")
