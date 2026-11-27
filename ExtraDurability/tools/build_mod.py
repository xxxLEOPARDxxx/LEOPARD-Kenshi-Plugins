"""Build the native Extra Durability (HARD) item and smithing recipe."""
import argparse
import struct
from pathlib import Path

MOD_NAME = "Extra Durability (HARD).mod"
KIT_ID = f"1-{MOD_NAME}"
TOOLS_ID = "43404-changes_otto.mod"
OUTPUT = Path(__file__).resolve().parents[1] / "package" / MOD_NAME

IRON_PLATES = "42159-gamedata.base"
LEATHER = "2292-gamedata.base"
SMITHS = (
    ("weapon smith basic", "2035-gamedata.base"),
    ("weapon smith 2", "2103-gamedata.base"),
    ("weapon smith 3", "2104-gamedata.base"),
    ("armour plate", "2296-gamedata.base"),
)
RESEARCH = (
    ("Basic Weapon Smithing", "52323-rebirth.mod"),
    ("Plate Armour Crafting", "2297-gamedata.base"),
)


def text(value):
    encoded = value.encode("utf-8")
    return struct.pack("<I", len(encoded)) + encoded


def fields(values, pack):
    return struct.pack("<I", len(values)) + b"".join(
        text(name) + pack(value) for name, value in values
    )


def refs(values):
    return struct.pack("<I", len(values)) + b"".join(
        text(category) + struct.pack("<I", len(entries)) + b"".join(
            text(target) + struct.pack("<iii", first, second, third)
            for target, first, second, third in entries
        )
        for category, entries in values
    )


def record(kind, name, sid, change, *, bools=(), floats=(), ints=(),
           strings=(), files=(), references=()):
    # FCS v17 serializes record size (including itself), type and legacy ID,
    # then name, string ID, change number and the typed data sections.
    body = b"".join((
        struct.pack("<ii", kind, 0), text(name), text(sid), struct.pack("<i", change),
        fields(bools, lambda value: struct.pack("<B", value)),
        fields(floats, lambda value: struct.pack("<f", value)),
        fields(ints, lambda value: struct.pack("<i", value)),
        struct.pack("<II", 0, 0),  # vector sections
        fields(strings, text),
        fields(files, text),
        refs(references),
        struct.pack("<I", 0),  # instances
    ))
    return struct.pack("<I", len(body) + 4) + body


def mod_bytes():
    kit = record(
        4, "Equipment Repair Kit", KIT_ID, 16,
        bools=(("has collision", True),),
        floats=(("charges", 100.0), ("quality", 1.0), ("weight kg", 3.0)),
        ints=(
            ("inventory footprint height", 2),
            ("inventory footprint width", 3),
            ("inventory sound", 14),
            ("item function", 0),
            ("production time", 4),
            ("slot", 7),
            ("stackable", 1),
            ("value", 1150),
        ),
        strings=(("description", "Restores worn weapons and armour. A partly used kit keeps its remaining charge."),),
        files=(
            ("icon", r".\data\items\icons\Tools.png"),
            ("mesh", r".\data\items\meshes\Item_Tools.mesh"),
            ("physics file", r".\data\items\meshes\Item_Generic.xml"),
        ),
        references=(
            ("ingredients", ((IRON_PLATES, 300, 0, 0), (LEATHER, 100, 0, 0))),
            ("material", (("5263-lanterns_otto.mod", 0, 0, 0),)),
        ),
    )
    tools = record(
        4, "Tools", TOOLS_ID, 17,
        floats=(("charges", 100.0),),
        references=(("ingredients", ((IRON_PLATES, 300, 0, 0), (LEATHER, 100, 0, 0))),),
    )
    smiths = [
        record(62, name, sid, 17, references=(("item crafts", ((TOOLS_ID, 5, 0, 0),)),))
        for name, sid in SMITHS
    ]
    research = [
        record(21, name, sid, 17, references=(("enable item", ((TOOLS_ID, 0, 0, 0),)),))
        for name, sid in RESEARCH
    ]
    records = [kit, tools, *smiths, *research]

    # v17 header. The empty standard template has next ID 1 and flags 0x90000.
    # Record 1 belongs to this mod, so reserve ID 2 for the next FCS edit.
    header = b"".join((
        struct.pack("<I", 1),
        text("xxxLEOPARDxxx"),
        text("Equipment wear and repair."),
        text("gamedata.base,rebirth.mod"),
        text(""),
        struct.pack("<I", 2),
        bytes.fromhex("00000000000009000000"),
    ))
    return struct.pack("<II", 17, len(header) - 4) + header + struct.pack("<I", len(records)) + b"".join(records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    data = mod_bytes()
    if args.check:
        if not args.output.is_file() or args.output.read_bytes() != data:
            parser.exit(1, f"FCS data needs regeneration: {args.output}\n")
        print(f"Verified {args.output} ({len(data)} bytes)")
        return
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Wrote {args.output} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
