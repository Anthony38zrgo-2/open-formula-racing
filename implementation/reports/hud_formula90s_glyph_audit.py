"""Report which characters the Libron TTF files actually provide.

Reads the cmap table of each TrueType font without external dependencies and checks the
characters the migrated Formula HUD needs, so a missing glyph cannot degrade silently
into a fallback.
"""

import pathlib
import struct
import sys

FONT_DIRECTORY = pathlib.Path(r"D:/open-formula/implementation/fonts/Libron")

REQUIRED_TEXT = (
    "0123456789"
    ":.+-/%"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "\u00b0\u00ba"      # degree sign, masculine ordinal
    "()"
    " "
    "\u00e1\u00e9\u00ed\u00f3\u00fa\u00c1\u00c9\u00cd\u00d3\u00da\u00f1\u00d1"
)


def read_cmap_codepoints(path):
    data = path.read_bytes()
    tag, _, _ = struct.unpack(">4sHH", data[0:8])
    if tag != b"true" and tag != b"OTTO" and data[0:4] != b"\x00\x01\x00\x00":
        # Accept both 'true'/0x00010000 outlines; OTTO (CFF) has the same table layout
        # for cmap, so no rejection there.
        pass
    table_count = struct.unpack(">H", data[4:6])[0]
    cmap_offset = None
    for index in range(table_count):
        entry = 12 + index * 16
        record_tag = data[entry:entry + 4]
        offset = struct.unpack(">I", data[entry + 8:entry + 12])[0]
        if record_tag == b"cmap":
            cmap_offset = offset
            break
    if cmap_offset is None:
        raise RuntimeError(f"{path} has no cmap table")
    version, subtable_count = struct.unpack(">HH", data[cmap_offset:cmap_offset + 4])
    chosen = None
    for index in range(subtable_count):
        record = cmap_offset + 4 + index * 8
        platform, encoding, offset = struct.unpack(">HHI", data[record:record + 8])
        subtable = cmap_offset + offset
        format_number = struct.unpack(">H", data[subtable:subtable + 2])[0]
        if format_number == 4 and platform in (0, 3):
            chosen = subtable
            break
    if chosen is None:
        return set()
    length = struct.unpack(">H", data[chosen + 2:chosen + 4])[0]
    seg_count = struct.unpack(">H", data[chosen + 6:chosen + 8])[0] // 2
    end_offsets = chosen + 14
    start_offsets = end_offsets + seg_count * 2 + 2
    id_delta_offsets = start_offsets + seg_count * 2
    id_range_offsets = id_delta_offsets + seg_count * 2
    codepoints = set()
    for segment in range(seg_count):
        end = struct.unpack(">H", data[end_offsets + segment * 2:end_offsets + segment * 2 + 2])[0]
        start = struct.unpack(">H", data[start_offsets + segment * 2:start_offsets + segment * 2 + 2])[0]
        delta = struct.unpack(">h", data[id_delta_offsets + segment * 2:id_delta_offsets + segment * 2 + 2])[0]
        range_offset_position = id_range_offsets + segment * 2
        range_offset = struct.unpack(">H", data[range_offset_position:range_offset_position + 2])[0]
        if start == 0xFFFF:
            continue
        for codepoint in range(start, min(end, 0xFFFE) + 1):
            if range_offset == 0:
                glyph = (codepoint + delta) & 0xFFFF
            else:
                glyph_position = (range_offset_position + range_offset
                                  + (codepoint - start) * 2)
                if glyph_position + 2 > chosen + length:
                    continue
                glyph = struct.unpack(">H", data[glyph_position:glyph_position + 2])[0]
                if glyph != 0:
                    glyph = (glyph + delta) & 0xFFFF
            if glyph != 0:
                codepoints.add(codepoint)
    return codepoints


def main():
    failures = 0
    required = sorted({ord(character) for character in REQUIRED_TEXT})
    for font_path in sorted(FONT_DIRECTORY.glob("*.ttf")):
        codepoints = read_cmap_codepoints(font_path)
        missing = [chr(value) for value in required if value not in codepoints]
        status = "OK" if not missing else "MISSING " + " ".join(
            f"U+{ord(character):04X} {character!r}" for character in missing)
        print(f"{font_path.name}: cmap={len(codepoints)} {status}")
        if missing:
            failures += 1
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
