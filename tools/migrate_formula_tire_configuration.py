#!/usr/bin/env python3
"""Migrate a Formula Tires XML section from schema 1 (radial spring) to schema 2.

The migration is idempotent.  It removes the retired radial stiffness/damping
fields and writes the empirical load-compliance fields used by the rigid
contact model.  It works on the XML text so comments and formatting survive.
"""

import argparse
import math
import pathlib
import re

RETIRED = (
    'radial stiffness',
    'radial damping ratio',
    'maximum deflection',
    'partial contact stiffness floor',
    'rolling radius deflection factor',
    'contact patch deflection gain',
)


def attribute_value(block: str, name: str, default: float) -> float:
    match = re.search(
        r'<attnum\s+name="' + re.escape(name) + r'"\s+val="([^"]+)"', block)
    if not match:
        return default
    try:
        return float(match.group(1))
    except ValueError:
        return default


def remove_attribute(block: str, name: str) -> str:
    return re.sub(
        r'^\s*<attnum\s+name="' + re.escape(name) + r'"[^>]*/>\s*\n',
        '', block, flags=re.MULTILINE)


def set_attribute(block: str, name: str, value: float, unit: str | None = None) -> str:
    unit_text = f' unit="{unit}"' if unit else ''
    replacement = f'      <attnum name="{name}" val="{value:.12g}"{unit_text} />\n'
    pattern = re.compile(
        r'^\s*<attnum\s+name="' + re.escape(name) + r'"[^>]*/>\s*\n',
        re.MULTILINE)
    if pattern.search(block):
        return pattern.sub(replacement, block, count=1)
    return block.replace('    </section>', replacement + '    </section>', 1)


def migrate_section(text: str, section_name: str, static_load: float) -> tuple[str, bool]:
    pattern = re.compile(
        r'(<section\s+name="' + re.escape(section_name) + r'">)(.*?)(</section>)',
        re.DOTALL)
    match = pattern.search(text)
    if not match:
        return text, False
    block = match.group(0)
    original = block
    retired_present = any(
        re.search(r'name="' + re.escape(name) + r'"', block) for name in RETIRED)
    limit = attribute_value(block, 'maximum deflection',
                            attribute_value(block, 'empirical deformation limit', 0.0))
    rolling = attribute_value(block, 'rolling radius deflection factor', 0.68)
    patch_gain = attribute_value(block, 'contact patch deflection gain', 0.20)

    if retired_present:
        for name in RETIRED:
            block = remove_attribute(block, name)
        if limit <= 0.0:
            limit = 0.12 * 0.312
        block = set_attribute(block, 'empirical load compliance',
                              0.008 / max(static_load, 1.0), 'm/N')
        block = set_attribute(block, 'empirical deformation limit', limit, 'm')
        block = set_attribute(block, 'rolling radius load deflection factor', rolling)
        block = set_attribute(block, 'contact patch empirical deformation gain', patch_gain)
        text = text.replace(original, block, 1)
        return text, True
    return text, False


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('xml', nargs='+', type=pathlib.Path)
    arguments = parser.parse_args()
    changed_files = 0
    for path in arguments.xml:
        text = path.read_text(encoding='utf-8')
        mass_match = re.search(r'<attnum\s+name="mass"\s+val="([^"]+)"', text)
        share_match = re.search(
            r'<attnum\s+name="front-rear weight repartition"\s+val="([^"]+)"', text)
        mass = float(mass_match.group(1)) if mass_match else 612.6
        share = float(share_match.group(1)) if share_match else 0.445
        front_load = mass * 9.81 * share * 0.5
        rear_load = mass * 9.81 * (1.0 - share) * 0.5
        changed = False
        text, front_changed = migrate_section(text, 'Front Contact', front_load)
        text, rear_changed = migrate_section(text, 'Rear Contact', rear_load)
        changed = front_changed or rear_changed
        schema_pattern = re.compile(
            r'(<attnum\s+name="schema version"\s+val=")[^"]+(")')
        if schema_pattern.search(text):
            text = schema_pattern.sub(r'\g<1>2\g<2>', text, count=1)
        else:
            text = text.replace(
                '<section name="Formula Tires">',
                '<section name="Formula Tires">\n      <attnum name="schema version" val="2" />',
                1)
            changed = True
        if changed:
            path.write_text(text, encoding='utf-8', newline='')
            changed_files += 1
            print(f'migrated {path}')
        else:
            print(f'unchanged {path}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
