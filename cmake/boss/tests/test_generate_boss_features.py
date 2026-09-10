import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import generate_boss_features as gbf  # noqa: E402


def write_manifest(features_dir: Path, feature_name: str, **overrides) -> Path:
    data = {
        "name": feature_name,
        "id": overrides.pop("id", 10000),
        "key": overrides.pop("key", feature_name),
        "trait": overrides.pop("trait", f"Slic3r::Boss::{feature_name.title()}Feature"),
        "header": overrides.pop("header", f"boss/features/{feature_name}/{feature_name}.hpp"),
        "components": overrides.pop(
            "components", {"slic3r-domain": {"capabilities": ["fdm_config"], "sources": []}}
        ),
        **overrides,
    }
    feature_dir = features_dir / feature_name
    feature_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = feature_dir / "boss-feature.json"
    manifest_path.write_text(json.dumps(data), encoding="utf-8")
    return manifest_path


class DiscoverManifestsTests(unittest.TestCase):
    def test_empty_directory_returns_no_manifests(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(gbf.discover_manifests(Path(tmp)), [])

    def test_discovers_one_manifest_per_feature_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha")
            write_manifest(features_dir, "beta")
            manifests = gbf.discover_manifests(features_dir)
            self.assertEqual(sorted(m.name for m in manifests), ["alpha", "beta"])

    def test_discovery_order_is_deterministic(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "zebra")
            write_manifest(features_dir, "alpha")
            manifests = gbf.discover_manifests(features_dir)
            self.assertEqual([m.name for m in manifests], ["alpha", "zebra"])


class ValidateManifestTests(unittest.TestCase):
    def test_missing_required_key_raises(self):
        with self.assertRaises(gbf.ManifestError):
            gbf.validate_manifest(Path("x/boss-feature.json"), {"name": "x"})

    def test_bad_name_characters_raise(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "Has_Bad_Chars")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_unknown_top_level_key_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha")
            data = json.loads(path.read_text(encoding="utf-8"))
            data["unexpected"] = True
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_zero_id_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", id=0)
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_key_none_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", key="none")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_negative_id_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", id=-1)
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_id_above_int32_range_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", id=2**31)
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_key_with_disallowed_characters_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", key='x"y')
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_header_with_quote_character_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", header='boss/features/alpha/"; #include <cstdlib>')
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_header_with_backslash_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", header=r"boss\features\alpha\Alpha.hpp")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_source_with_semicolon_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={
                    "slic3r-domain": {
                        "capabilities": ["fdm_config"],
                        "sources": ["a.cpp;b.cpp"],
                    }
                },
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_non_identifier_trait_basename_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", trait="Slic3r::Boss::123Bad")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_non_string_capability_entry_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"slic3r-domain": {"capabilities": [1], "sources": []}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_non_list_sources_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"slic3r-domain": {"capabilities": ["fdm_config"], "sources": "not-a-list"}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_component_with_sources_and_empty_capabilities_is_accepted(self):
        # A plain module with no registry dispatch (e.g. PreciseWalls)
        # contributes sources to its target without joining any capability.
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"libslic3r": {"capabilities": [], "sources": ["libslic3r/src/libslic3r/Alpha.cpp"]}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            self.assertEqual(manifest.components["libslic3r"]["capabilities"], [])
            self.assertEqual(
                manifest.components["libslic3r"]["sources"], ["libslic3r/src/libslic3r/Alpha.cpp"]
            )

    def test_component_with_sources_and_omitted_capabilities_is_accepted(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"libslic3r": {"sources": ["libslic3r/src/libslic3r/Alpha.cpp"]}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            self.assertEqual(manifest.components["libslic3r"].get("capabilities", []), [])

    def test_component_with_no_capabilities_and_no_sources_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"libslic3r": {"capabilities": [], "sources": []}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_component_with_neither_key_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"libslic3r": {}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_keyword_trait_basename_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", trait="Slic3r::Boss::class")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_keyword_in_a_non_final_trait_segment_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            # Only the basename ("Feature") is a valid non-keyword
            # identifier here -- the middle segment ("class") must still
            # be caught, not just the final one.
            path = write_manifest(Path(tmp), "alpha", trait="Slic3r::Boss::class::Feature")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_absolute_header_path_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha", header="/etc/passwd")
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_parent_traversal_source_path_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={
                    "slic3r-domain": {
                        "capabilities": ["fdm_config"],
                        "sources": ["../../../etc/passwd"],
                    }
                },
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_parent_traversal_component_target_name_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"../../evil": {"capabilities": ["fdm_config"], "sources": []}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_unknown_component_key_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                components={"slic3r-domain": {"capabilities": ["fdm_config"], "sources": [], "extra": True}},
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_unknown_vendored_key_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                vendored=[{"name": "x", "path": "vendor/x", "kind": "INTERFACE", "extra": True}],
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_valid_manifest_parses(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha")
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            self.assertEqual(manifest.name, "alpha")
            self.assertEqual(manifest.id, 10000)

    def test_valid_vendored_entry_parses(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                vendored=[{"name": "fastnoiselite", "path": "vendor/fastnoiselite", "kind": "INTERFACE"}],
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            self.assertEqual(len(manifest.vendored), 1)

    def test_vendored_entry_missing_field_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(
                Path(tmp), "alpha",
                vendored=[{"name": "fastnoiselite", "path": "vendor/fastnoiselite"}],
            )
            data = json.loads(path.read_text(encoding="utf-8"))
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)


class GlobalUniquenessTests(unittest.TestCase):
    def test_duplicate_name_across_directories_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha")
            manifests = gbf.discover_manifests(features_dir)
            manifests.append(manifests[0])  # simulate a second feature reusing the same name
            with self.assertRaises(gbf.ManifestError):
                gbf.check_global_uniqueness(manifests)

    def test_duplicate_id_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha", id=1)
            write_manifest(features_dir, "beta", id=1)
            manifests = gbf.discover_manifests(features_dir)
            with self.assertRaises(gbf.ManifestError):
                gbf.check_global_uniqueness(manifests)

    def test_duplicate_key_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha", id=1, key="shared")
            write_manifest(features_dir, "beta", id=2, key="shared")
            manifests = gbf.discover_manifests(features_dir)
            with self.assertRaises(gbf.ManifestError):
                gbf.check_global_uniqueness(manifests)

    def test_distinct_manifests_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha", id=1, key="alpha")
            write_manifest(features_dir, "beta", id=2, key="beta")
            manifests = gbf.discover_manifests(features_dir)
            gbf.check_global_uniqueness(manifests)  # must not raise


class ConfigOptionsTests(unittest.TestCase):
    def _one(self, tmp, options):
        path = write_manifest(Path(tmp), "alpha", config_options=options)
        return path, json.loads(path.read_text(encoding="utf-8"))

    def test_valid_config_options_parse(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, [
                {"key": "filament_max_speed", "invalidates": ["psWipeTower", "psSkirtBrim"]},
            ])
            manifest = gbf.validate_manifest(path, data)
            self.assertEqual(manifest.config_options[0]["key"], "filament_max_speed")

    def test_empty_invalidates_is_allowed(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, [{"key": "some_option", "invalidates": []}])
            manifest = gbf.validate_manifest(path, data)
            self.assertEqual(manifest.config_options[0]["invalidates"], [])

    def test_unknown_step_name_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, [{"key": "some_option", "invalidates": ["psNotAStep"]}])
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_missing_invalidates_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, [{"key": "some_option"}])
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_config_option_key_with_hyphen_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, [{"key": "not-snake", "invalidates": []}])
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_non_string_step_in_invalidates_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, [{"key": "some_option", "invalidates": [{}]}])
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_duplicate_config_option_key_across_manifests_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha", id=1, key="alpha",
                           config_options=[{"key": "shared_opt", "invalidates": []}])
            write_manifest(features_dir, "beta", id=2, key="beta",
                           config_options=[{"key": "shared_opt", "invalidates": []}])
            manifests = gbf.discover_manifests(features_dir)
            with self.assertRaises(gbf.ManifestError):
                gbf.check_global_uniqueness(manifests)


class StoreValidationTests(unittest.TestCase):
    def _one(self, tmp, store):
        path = write_manifest(Path(tmp), "alpha", config_options=[
            {"key": "some_option", "invalidates": [], "store": store}])
        return path, json.loads(path.read_text(encoding="utf-8"))

    def test_valid_store_parses(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, {"home": "extrude_config", "field": "some_option",
                                         "kind": "per_extruder_double"})
            m = gbf.validate_manifest(path, data)
            self.assertEqual(m.config_options[0]["store"]["home"], "extrude_config")

    def test_unknown_home_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, {"home": "nope", "field": "x", "kind": "scalar_bool"})
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_unknown_kind_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, {"home": "wipe_tower", "field": "x", "kind": "nope"})
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_non_identifier_field_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path, data = self._one(tmp, {"home": "wipe_tower", "field": "1bad", "kind": "scalar_bool"})
            with self.assertRaises(gbf.ManifestError):
                gbf.validate_manifest(path, data)

    def test_duplicate_home_field_across_manifests_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            features_dir = Path(tmp)
            write_manifest(features_dir, "alpha", id=1, key="alpha", config_options=[
                {"key": "opt_a", "invalidates": [],
                 "store": {"home": "wipe_tower", "field": "dup", "kind": "scalar_bool"}}])
            write_manifest(features_dir, "beta", id=2, key="beta", config_options=[
                {"key": "opt_b", "invalidates": [],
                 "store": {"home": "wipe_tower", "field": "dup", "kind": "scalar_bool"}}])
            manifests = gbf.discover_manifests(features_dir)
            with self.assertRaises(gbf.ManifestError):
                gbf.check_global_uniqueness(manifests)


class PerimeterPolicyCapabilityTests(unittest.TestCase):
    def _one(self, tmp):
        return write_manifest(
            Path(tmp), "test-perimeter-policy", id=900201, key="test-perimeter-policy",
            trait="Slic3r::Boss::Test::PerimeterPolicyFixtureFeature",
            header="boss/test-fixtures/perimeter-policy-fixture/PerimeterPolicyFixtureFeature.hpp",
            components={
                "libslic3r": {
                    "capabilities": ["perimeter_policy"],
                    "sources": ["libslic3r/src/libslic3r/boss/test_fixtures/PerimeterPolicyFixture.cpp"],
                }
            },
        )

    def test_perimeter_policy_capability_is_known(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            gbf.check_known_capabilities([manifest])
            gbf.check_capability_targets([manifest])

    def test_perimeter_policy_generates_component_alias(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)

            include_dir = Path(tmp) / "generated" / "include"
            header_path = gbf.emit_composition_header("perimeter_policy", [manifest], include_dir)
            content = header_path.read_text(encoding="utf-8")
            self.assertIn(
                "using BossPerimeterPolicies = Slic3r::Boss::BossPerimeterPolicyRegistry<"
                "Slic3r::Boss::Test::PerimeterPolicyFixtureFeature>;",
                content,
            )


class LabelObjectsCapabilityTests(unittest.TestCase):
    def _one(self, tmp):
        return write_manifest(
            Path(tmp), "test-label-objects", id=900301, key="test-label-objects",
            trait="Slic3r::Boss::Test::LabelObjectsFixtureFeature",
            header="boss/test-fixtures/label-objects-fixture/LabelObjectsFixtureFeature.hpp",
            components={
                "libslic3r": {
                    "capabilities": ["label_objects"],
                    "sources": ["libslic3r/src/libslic3r/boss/test_fixtures/LabelObjectsFixture.cpp"],
                }
            },
        )

    def test_label_objects_capability_is_known(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            gbf.check_known_capabilities([manifest])
            gbf.check_capability_targets([manifest])

    def test_label_objects_generates_component_alias(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)

            include_dir = Path(tmp) / "generated" / "include"
            header_path = gbf.emit_composition_header("label_objects", [manifest], include_dir)
            content = header_path.read_text(encoding="utf-8")
            self.assertIn(
                "using BossLabelObjects = Slic3r::Boss::BossLabelObjectsRegistry<"
                "Slic3r::Boss::Test::LabelObjectsFixtureFeature>;",
                content,
            )


class SolidFillPolicyCapabilityTests(unittest.TestCase):
    def _one(self, tmp):
        return write_manifest(
            Path(tmp), "test-solid-fill-policy", id=900501, key="test-solid-fill-policy",
            trait="Slic3r::Boss::Test::SolidFillPolicyFixtureFeature",
            header="boss/test-fixtures/solid-fill-policy-fixture/SolidFillPolicyFixtureFeature.hpp",
            components={
                "libslic3r": {
                    "capabilities": ["solid_fill_policy"],
                    "sources": ["libslic3r/src/libslic3r/boss/test_fixtures/SolidFillPolicyFixture.cpp"],
                }
            },
        )

    def test_solid_fill_policy_capability_is_known(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)
            gbf.check_known_capabilities([manifest])
            gbf.check_capability_targets([manifest])

    def test_solid_fill_policy_generates_component_alias(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)

            include_dir = Path(tmp) / "generated" / "include"
            header_path = gbf.emit_composition_header("solid_fill_policy", [manifest], include_dir)
            content = header_path.read_text(encoding="utf-8")
            self.assertIn(
                "using BossSolidFillPolicy = Slic3r::Boss::BossSolidFillPolicyRegistry<"
                "Slic3r::Boss::Test::SolidFillPolicyFixtureFeature>;",
                content,
            )


class VendoredEmissionTests(unittest.TestCase):
    def _one(self, tmp):
        return write_manifest(
            Path(tmp), "test-vendored", id=900401, key="test-vendored",
            trait="Slic3r::Boss::Test::VendoredFixtureFeature",
            header="boss/test-fixtures/vendored-fixture/VendoredFixtureFeature.hpp",
            components={
                "libslic3r": {
                    "capabilities": [],
                    "sources": ["libslic3r/src/libslic3r/boss/test_fixtures/VendoredFixture.cpp"],
                }
            },
            vendored=[{"name": "boss-test-vendor", "path": "vendor/boss-test-vendor", "kind": "INTERFACE"}],
        )

    def test_vendored_entry_emits_interface_library(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)

            output_dir = Path(tmp) / "generated"
            cmake_path = gbf.emit_vendored_cmake([manifest], "libslic3r", output_dir)
            content = cmake_path.read_text(encoding="utf-8")
            self.assertIn("add_library(boss-test-vendor INTERFACE)", content)
            self.assertIn("target_include_directories(boss-test-vendor INTERFACE", content)
            self.assertIn("vendor/boss-test-vendor", content)
            self.assertIn("target_link_libraries(libslic3r PRIVATE boss-test-vendor)", content)

    def test_vendored_entry_not_linked_into_other_targets(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = self._one(tmp)
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)

            output_dir = Path(tmp) / "generated"
            cmake_path = gbf.emit_vendored_cmake([manifest], "slic3r-domain", output_dir)
            self.assertEqual(cmake_path.read_text(encoding="utf-8"), "")

    def test_manifest_without_vendored_emits_empty_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = write_manifest(Path(tmp), "alpha")
            data = json.loads(path.read_text(encoding="utf-8"))
            manifest = gbf.validate_manifest(path, data)

            output_dir = Path(tmp) / "generated"
            cmake_path = gbf.emit_vendored_cmake([manifest], "slic3r-domain", output_dir)
            self.assertEqual(cmake_path.read_text(encoding="utf-8"), "")


if __name__ == "__main__":
    unittest.main()
