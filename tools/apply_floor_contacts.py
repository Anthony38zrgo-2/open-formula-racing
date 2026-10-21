#!/usr/bin/env python3
"""Write the Formula Floor section of the vehicle XML from the contact proposal.

Tier 1 and tier 2 floor points are enabled; tier 3 points and the front-wing group
are written disabled so the geometry is preserved without affecting handling.
"""

import argparse
import json
import pathlib
import sys
import xml.etree.ElementTree as element_tree


def find_section_end(lines, start_index):
    depth = 0
    for index in range(start_index, len(lines)):
        depth += lines[index].count("<section ")
        depth -= lines[index].count("</section>")
        if depth == 0:
            return index
    raise ValueError("unterminated section")


def build_block(points, section_enabled):
    lines = []
    lines.append('  <section name="Formula Floor">')
    lines.append(f'    <attnum name="enabled" val="{1 if section_enabled else 0}" />')
    lines.append(f'    <attnum name="contact count" val="{len(points)}" />')
    lines.append('  </section>')
    for index, point in enumerate(points):
        group = point["group"]
        tier = point.get("tier", 1)
        enabled = group == "floor" and tier <= 2
        solver = point["solver"]
        lines.append(f'  <section name="Formula Floor Point {index}">')
        lines.append(f'    <attstr name="id" val="{point["id"]}" />')
        lines.append(f'    <attnum name="tier" val="{tier}" />')
        lines.append(f'    <attnum name="x" val="{solver["x"]:.9f}" />')
        lines.append(f'    <attnum name="y" val="{solver["y"]:.9f}" />')
        lines.append(f'    <attnum name="z" val="{solver["z"]:.9f}" />')
        lines.append(f'    <attstr name="group" val="{group}" />')
        lines.append(f'    <attnum name="enabled" val="{1 if enabled else 0}" />')
        lines.append('  </section>')
    return lines


def apply_floor_contacts(points_path, xml_path, section_enabled=True):
    proposal = json.loads(pathlib.Path(points_path).read_text())
    points = ([dict(point, group="floor") for point in proposal["groups"]["floor"]]
              + [dict(point, group="front_wing")
                 for point in proposal["groups"]["front_wing"]])

    path = pathlib.Path(xml_path)
    lines = path.read_text(encoding="utf-8").splitlines()

    # Remove existing Formula Floor and point sections.
    index = 0
    while index < len(lines):
        stripped = lines[index].strip()
        if (stripped == '<section name="Formula Floor">'
                or stripped.startswith('<section name="Formula Floor Point ')):
            end = find_section_end(lines, index)
            del lines[index:end + 1]
        else:
            index += 1

    controls_index = None
    for index, line in enumerate(lines):
        if line.strip() == '<section name="Formula Controls">':
            controls_index = index
            break
    if controls_index is None:
        raise ValueError("Formula Controls section not found")

    controls_end = find_section_end(lines, controls_index)
    block = build_block(points, section_enabled)
    lines[controls_end + 1:controls_end + 1] = block
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")

    # Verify the written section against the proposal.
    root = element_tree.parse(path).getroot()
    floor_section = root.find("section[@name='Formula Floor']")
    if floor_section is None:
        raise ValueError("Formula Floor section missing after write")
    written_count = int(floor_section.find("attnum[@name='contact count']").get("val"))
    if written_count != len(points):
        raise ValueError("Formula Floor contact count mismatch")
    for index, point in enumerate(points):
        section = root.find(f"section[@name='Formula Floor Point {index}']")
        if section is None:
            raise ValueError(f"Formula Floor Point {index} missing")
        for axis in ("x", "y", "z"):
            expected = point["solver"][axis]
            actual = float(section.find(f"attnum[@name='{axis}']").get("val"))
            if abs(actual - expected) > 1.0e-8:
                raise ValueError(f"Formula Floor Point {index} {axis} mismatch")
        expected_enabled = point["group"] == "floor" and point.get("tier", 1) <= 2
        actual_enabled = int(section.find("attnum[@name='enabled']").get("val")) != 0
        if actual_enabled != expected_enabled:
            raise ValueError(f"Formula Floor Point {index} enabled mismatch")

    enabled = sum(1 for point in points
                  if point["group"] == "floor" and point.get("tier", 1) <= 2)
    print(f"Formula Floor: {len(points)} points, {enabled} enabled, "
          f"section_enabled={section_enabled}; XML verification passed")
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--points", required=True)
    parser.add_argument("--xml", required=True)
    parser.add_argument("--disable-section", action="store_true")
    arguments = parser.parse_args()
    return apply_floor_contacts(arguments.points, arguments.xml,
                                not arguments.disable_section)


if __name__ == "__main__":
    sys.exit(main())
