"""Public descriptions must never expose the internal development catalog."""
import sys
import importlib.util
import unittest
from unittest.mock import patch
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
                validator.player_feature_claims(mod, text, mod + "." + lang)
        public = {entry["package"] for entry in sections.public_mcm_catalog()}
        for entry in sections.mcm_catalog():
            if entry["package"] in public:
                continue
            for alias in (entry["package"], entry["id"], entry["name"].lower()):
                with self.subTest(alias=alias):
                    with self.assertRaisesRegex(ValueError, "Private"):
                        validator.validate_public_mcm_text("[b]" + alias + "[/b]", "nexus.txt")

    def test_keo_release_preparation_is_self_only_and_requires_explicit_authorization(self):
        private = [dict(package="KEO", id="KEO", name="KEO: Kenshi Engine Optimizations",
                        public=False, release_authorized=True),
                   dict(package="DynamicFactions", id="Dynamic Factions", name="Dynamic Factions",
                        public=False, release_authorized=True),
                   dict(package="GlobalSatelliteMap", id="Global Satellite Map", name="Global Satellite Map",
                        public=False, release_authorized=True)]
        with patch.object(validator, "mcm_catalog", return_value=private), patch.object(validator, "public_mcm_catalog", return_value=[]):
            validator.player_feature_claims("KEO", "KEO: Kenshi Engine Optimizations - our fork.", "KEO.en")
            for other in ("AssignedWorkers", "ModConfigMenu"):
                with self.subTest(other=other), self.assertRaisesRegex(ValueError, "Private"):
                    validator.player_feature_claims(other, "KEO: Kenshi Engine Optimizations - our fork.", "other.en")
            for name in ("Dynamic Factions", "Global Satellite Map"):
                with self.subTest(name=name), self.assertRaisesRegex(ValueError, "Private"):
                    validator.player_feature_claims("KEO", "KEO " + name, "KEO.en")
                with self.subTest(self_project=name), self.assertRaisesRegex(ValueError, "Private"):
                    validator.player_feature_claims(name, name, "other.en")
            for unauthorized in (False, None, "true", 1):
                private[0]["release_authorized"] = unauthorized
                with self.subTest(unauthorized=unauthorized), self.assertRaisesRegex(ValueError, "Private"):
                    validator.player_feature_claims("KEO", "KEO: Kenshi Engine Optimizations", "KEO.en")
            private[0]["release_authorized"] = True
            with self.assertRaisesRegex(ValueError, "Private"):
                validator.validate_public_mcm_text("KEO", "raw-public-text")
        published = {"KEO": dict(workshop_id=123, nexus=456, version="1")}
        self.assertEqual(sections.public_mcm_catalog(private, published), [])

    def test_keo_release_input_uses_canonical_fork_and_commented_defaults(self):
        build = validator.build
        items = [item for item in build.ITEMS if item["mod"] == "KEO"]
        self.assertEqual(len(items), 1)
        self.assertEqual(items[0]["title"], "KEO (MCM fork) (QoL)")
        self.assertEqual(items[0]["version"], "1")
        self.assertEqual(items[0]["nexus"], 2271)
        self.assertIs(items[0]["lang"], True)
        self.assertEqual(Path(build.source_dir("KEO")).resolve(), Path("D:/DEV/Kenshi/forks/KEO").resolve())
        self.assertEqual(Path(build.defaults_ini("KEO")).resolve(), Path("D:/DEV/Kenshi/forks/KEO/KEO.ini").resolve())
        canonical = Path(build.defaults_ini("KEO")).read_text(encoding="utf-8-sig")
        active = [line for line in canonical.splitlines() if "=" in line and not line.lstrip().startswith((";", "#"))]
        self.assertEqual(active, [], "KEO release defaults must remain commented, never personal overrides")
        with patch.object(build.os.path, "isfile", return_value=False):
            self.assertIsNone(build.defaults_ini("KEO"))
        with patch.object(build.os.path, "isdir", return_value=False):
            self.assertIsNone(build.source_dir("KEO"))

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
