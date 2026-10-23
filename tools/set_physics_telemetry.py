"""Temporarily toggle physics capture without changing HUD data or vehicle setup."""
from pathlib import Path
import argparse
import re
import shutil
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--config", type=Path, default=ROOT / "distribution/user-settings/config/rust_physics.xml")
parser.add_argument("--enable", action="store_true", help="Re-enable summaries and event traces (default: disable both).")
parser.add_argument("--check", action="store_true", help="Verify the requested state without writing.")
args = parser.parse_args()
text = args.config.read_text(encoding="utf-8")
expected = "1" if args.enable else "0"
names = ("diagnostic telemetry", "diagnostic trace")
section = ET.fromstring(text).find("./section[@name='RustPhysics']")
if section is None:
    raise SystemExit("Missing RustPhysics section")
for name in names:
    node = section.find(f"./attnum[@name='{name}']")
    if node is None:
        raise SystemExit(f"Missing switch: {name}")
    if args.check and float(node.attrib["val"]) != float(expected):
        raise SystemExit(f"Unexpected {name}: {node.attrib['val']}, expected {expected}")
if not args.check:
    updated = text
    for name in names:
        pattern = rf'(<attnum\s+name="{re.escape(name)}"\s+val=")[^"]*("\s*/>)'
        updated, count = re.subn(pattern, lambda match: match[1] + expected + match[2], updated)
        if count != 1:
            raise SystemExit(f"Ambiguous switch: {name}; no changes written")
    if updated != text:
        backup = args.config.with_name(args.config.name + f".telemetry-backup-{time.time_ns()}")
        shutil.copy2(args.config, backup)
        args.config.write_text(updated, encoding="utf-8")
        print(f"Backup: {backup}")
print(f"Physics telemetry {'enabled' if args.enable else 'disabled'}: {args.config}")
print("The next game launch reads these switches. Existing captures are preserved.")
