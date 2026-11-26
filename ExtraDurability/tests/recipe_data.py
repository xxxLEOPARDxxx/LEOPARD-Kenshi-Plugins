"""Check the actual FCS records consumed by smithies and finished research."""
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE.parents[1] / "tools"))
from moddiff import fields, records

MOD = "Extra Durability (HARD).mod"
LEGACY_KIT = f"1-{MOD}"
TOOLS = "43404-changes_otto.mod"
rows = {row["string_id"]: row for row in records(str(SOURCE / "package" / MOD))}

smiths = ("2035-gamedata.base", "2103-gamedata.base", "2104-gamedata.base", "2296-gamedata.base")
research = ("52323-rebirth.mod", "2297-gamedata.base")
assert set(rows) == {LEGACY_KIT, TOOLS, *smiths, *research}
assert rows[TOOLS]["type"] == 4
assert rows[LEGACY_KIT]["type"] == 4
assert fields(rows[LEGACY_KIT])["float", "charges"] == 100.0
kit = fields(rows[TOOLS])
assert ("int", "item function") not in kit
assert ("float", "quality") not in kit
assert kit["float", "charges"] == 100.0
assert {key[2]: value[0] for key, value in kit.items() if key[:2] == ("ref", "ingredients")} == {
    "42159-gamedata.base": 300, "2292-gamedata.base": 100,
}
for sid in smiths:
    assert rows[sid]["type"] == 62
    assert fields(rows[sid]) == {("ref", "item crafts", TOOLS): (5, 0, 0)}
for sid in research:
    assert rows[sid]["type"] == 21
    assert fields(rows[sid]) == {("ref", "enable item", TOOLS): (0, 0, 0)}
print("Repair kit and four smiths verified")
