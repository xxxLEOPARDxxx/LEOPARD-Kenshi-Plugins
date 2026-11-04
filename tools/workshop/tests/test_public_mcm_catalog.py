"""Public descriptions must never expose the internal development catalog."""
import sys
import importlib.util
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import publication_sections as sections
import validate_descriptions as validator

REPOS = "https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks"
PRIVATE = ("Dynamic Factions", "Global Satellite Map", "KEO", "XP Overhaul", "Organize the Inventory")

class PublicCatalogTests(unittest.TestCase):
    def test_actual_supported_sections_never_disclose_private_work(self):
        for lang in ("ru", "en"):
            text = sections.supported_section(lang)
            for name in PRIVATE:
                with self.subTest(lang=lang, name=name):
                    self.assertNotIn(name, text)
            self.assertNotIn("local build", text)
            self.assertNotIn("локальная сборка", text)

    def test_validator_rejects_private_names_in_every_public_output(self):
        for mod in ("ModConfigMenu", "AssignedWorkers"):
            for name in PRIVATE:
                if name == "KEO" and mod != "ModConfigMenu":
                    continue
                with self.subTest(mod=mod, name=name):
                    with self.assertRaisesRegex(ValueError, "[Pp]rivate"):
                        validator.player_feature_claims(mod, (REPOS if mod == "ModConfigMenu" else "") + " " + name, "output.txt")

    def test_original_public_keo_compatibility_is_not_our_private_fork(self):
        for mod in ("BetterLooting", "StackSort"):
            validator.player_feature_claims(mod, "Compatible with the original KEO plugin.", "output.txt")
            for announcement in ("KEO: Kenshi Engine Optimizations", "Kenshi Engine Optimizations", "KEO is our local fork", "Наш локальный форк KEO"):
                with self.subTest(mod=mod, announcement=announcement):
                    with self.assertRaisesRegex(ValueError, "Private"):
                        validator.player_feature_claims(mod, announcement, "output.txt")

    def test_real_generator_outputs_and_private_identifiers(self):
        path = sections.HERE / "desc/make_desc.py"
        spec = importlib.util.spec_from_file_location("privacy_description_source", path)
        source = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(source)
        for mod, pair in source.T.items():
            for lang, text in zip(("ru", "en"), pair):
                validator.validate_public_mcm_text(text, mod + "." + lang,
                                                   require_all=(mod == "ModConfigMenu"),
                                                   allow_original_keo=(mod != "ModConfigMenu"))
        public = {entry["package"] for entry in sections.public_mcm_catalog()}
        for entry in sections.mcm_catalog():
            if entry["package"] in public:
                continue
            for alias in (entry["package"], entry["id"], entry["name"].lower()):
                with self.subTest(alias=alias):
                    with self.assertRaisesRegex(ValueError, "Private"):
                        validator.validate_public_mcm_text("[b]" + alias + "[/b]", "nexus.txt")

    def test_shipped_mcm_readme_keeps_private_projects_internal(self):
        readme = (Path(__file__).resolve().parents[3] / "ModConfigMenu/README.txt").read_text(encoding="utf-8")
        validator.validate_public_mcm_text(readme, "MCM README", allow_original_keo=True)

    def test_public_admission_requires_published_identity_and_respects_privacy(self):
        base = dict(package="Released", name="Released Mod", id="released", group="own")
        good = dict(workshop_id=123, nexus=456, version="2")
        self.assertEqual(sections.public_mcm_catalog([base], {"Released": good}), [base])
        self.assertEqual(sections.public_mcm_catalog([base], {}), [])
        for field in good:
            for invalid in (None, "", 0, False, "unverified"):
                record = dict(good, **{field: invalid})
                self.assertEqual(sections.public_mcm_catalog([base], {"Released": record}), [])
        for change in ({"group": "optional_experimental"}, {"public": False}, {"experimental": True}, {"group": "unknown"}):
            entry = dict(base, **change)
            self.assertEqual(sections.public_mcm_catalog([entry], {"Released": good}), [])

    def test_internal_catalog_stays_complete_public_names_are_required(self):
        internal = sections.mcm_catalog()
        self.assertTrue(all(any(e["name"] == name or e["id"] == name for e in internal) for name in PRIVATE))
        public = sections.public_mcm_catalog()
        self.assertEqual(len(public), 38)
        for lang in ("ru", "en"):
            text = sections.supported_section(lang) + REPOS
            validator.validate_public_mcm_text(text, "output.txt", require_all=True)
            for entry in public:
                if entry["id"] == "ModConfigMenu":
                    continue
                with self.subTest(lang=lang, missing=entry["name"]):
                    with self.assertRaisesRegex(ValueError, "Missing supported"):
                        validator.validate_public_mcm_text(text.replace(entry["name"], "REMOVED"), "output.txt", require_all=True)

if __name__ == "__main__":
    unittest.main()
