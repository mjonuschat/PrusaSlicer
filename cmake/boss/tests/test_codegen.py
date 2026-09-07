# cmake/boss/tests/test_codegen.py
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import generate_boss_features as gbf  # noqa: E402
from test_generate_boss_features import write_manifest  # noqa: E402


class GenerateEndToEndTests(unittest.TestCase):
    def test_empty_feature_set_emits_empty_compositions(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 0)
            config_header = Path(out_tmp, "include", "boss", "generated", "BossFdmFeatures.hpp")
            self.assertIn("BossFdmFeatures = Slic3r::Boss::BossConfigRegistry<>;", config_header.read_text())
            fill_header = Path(out_tmp, "include", "boss", "generated", "BossFills.hpp")
            self.assertIn("BossFills = Slic3r::Boss::BossFillRegistry<>;", fill_header.read_text())
            enum_header = Path(out_tmp, "include", "boss", "generated", "BossFillPatternKey.hpp")
            self.assertIn("None = 0,", enum_header.read_text())
            self.assertTrue(Path(out_tmp, "slic3r-domain", "sources.cmake").exists())
            self.assertTrue(Path(out_tmp, "libslic3r", "sources.cmake").exists())

    def test_single_config_feature_appears_in_composition(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "z-rotate",
                id=10001, key="z-rotate",
                trait="Slic3r::Boss::ZRotateFeature",
                header="boss/features/z-rotate/ZRotateFeature.hpp",
                components={"slic3r-domain": {"capabilities": ["fdm_config"], "sources": ["a.cpp"]}},
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 0)
            header = Path(out_tmp, "include", "boss", "generated", "BossFdmFeatures.hpp").read_text()
            self.assertIn("Slic3r::Boss::ZRotateFeature", header)
            sources = Path(out_tmp, "slic3r-domain", "sources.cmake").read_text()
            self.assertIn("a.cpp", sources)

    def test_duplicate_id_stops_generation_with_nonzero_exit(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(Path(features_tmp), "alpha", id=1)
            write_manifest(Path(features_tmp), "beta", id=1)
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 1)

    def test_fill_pattern_key_enum_includes_only_fill_features(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "crosshatch",
                id=20001, key="crosshatch",
                trait="Slic3r::Boss::CrossHatchFeature",
                header="boss/features/crosshatch/CrossHatchFeature.hpp",
                components={
                    "libslic3r": {"capabilities": ["fill"], "sources": ["b.cpp"]},
                },
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 0)
            enum_text = Path(out_tmp, "include", "boss", "generated", "BossFillPatternKey.hpp").read_text()
            self.assertIn("CrossHatch = 20001", enum_text)
            fill_header = Path(out_tmp, "include", "boss", "generated", "BossFills.hpp").read_text()
            self.assertIn("Slic3r::Boss::CrossHatchFeature", fill_header)
            config_header = Path(out_tmp, "include", "boss", "generated", "BossFdmFeatures.hpp").read_text()
            self.assertNotIn("Slic3r::Boss::CrossHatchFeature", config_header)

    def test_colliding_enum_member_names_stop_generation(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "alpha-fill",
                id=1, key="alpha-fill",
                trait="Slic3r::Boss::AlphaFeature",
                header="boss/features/alpha-fill/AlphaFeature.hpp",
                components={"libslic3r": {"capabilities": ["fill"], "sources": []}},
            )
            write_manifest(
                Path(features_tmp), "alpha-fill-2",
                id=2, key="alpha-fill-2",
                # Deliberately produces the same enum member name ("Alpha") as
                # the manifest above once the "Feature" suffix is stripped.
                trait="Slic3r::Boss::Second::AlphaFeature",
                header="boss/features/alpha-fill-2/AlphaFeature.hpp",
                components={"libslic3r": {"capabilities": ["fill"], "sources": []}},
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 1)

    def test_fill_feature_named_none_is_rejected(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "none-fill",
                id=1, key="none-fill",
                # Derives to the reserved enum member name "None" once the
                # "Feature" suffix is stripped -- must be rejected, not
                # silently produce a duplicate `None` enumerator.
                trait="Slic3r::Boss::NoneFeature",
                header="boss/features/none-fill/NoneFeature.hpp",
                components={"libslic3r": {"capabilities": ["fill"], "sources": []}},
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 1)

    def test_fill_feature_deriving_to_a_keyword_is_rejected(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "class-fill",
                id=1, key="class-fill",
                # "classFeature" is itself a valid identifier (passes
                # validate_manifest's trait-basename check), but strips down
                # to the reserved word "class" as an enum member name --
                # must be caught here, not just at the trait-basename stage.
                trait="Slic3r::Boss::classFeature",
                header="boss/features/class-fill/classFeature.hpp",
                components={"libslic3r": {"capabilities": ["fill"], "sources": []}},
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 1)

    def test_unknown_capability_stops_generation(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "alpha",
                components={"slic3r-domain": {"capabilities": ["not_a_real_capability"], "sources": []}},
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 1)

    def test_capability_declared_under_wrong_target_stops_generation(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            write_manifest(
                Path(features_tmp), "alpha",
                # "fill" belongs to libslic3r (CAPABILITY_REGISTRIES), not
                # slic3r-domain -- must be rejected, not silently woven
                # into BossFills while its sources link into slic3r-domain.
                components={"slic3r-domain": {"capabilities": ["fill"], "sources": []}},
            )
            rc = gbf.generate(Path(features_tmp), Path(out_tmp))
            self.assertEqual(rc, 1)


class StepInvalidationEmitTests(unittest.TestCase):
    def _parse(self, src, name, **overrides):
        path = write_manifest(Path(src), name, **overrides)
        return gbf.validate_manifest(path, json.loads(path.read_text()))

    def test_empty_feature_set_emits_empty_map(self):
        with tempfile.TemporaryDirectory() as out:
            gbf.emit_step_invalidations([], Path(out))
            text = (Path(out) / "libslic3r" / "BossStepInvalidations.cpp").read_text()
            self.assertIn("boss_step_invalidations()", text)
            self.assertIn('#include "libslic3r/StepsInvalidation.hpp"', text)
            self.assertNotIn("propagate(", text)

    def test_config_option_emits_entry(self):
        with tempfile.TemporaryDirectory() as src, tempfile.TemporaryDirectory() as out:
            m = self._parse(src, "alpha", config_options=[
                {"key": "filament_max_speed", "invalidates": ["psWipeTower", "psSkirtBrim"]}])
            gbf.emit_step_invalidations([m], Path(out))
            text = (Path(out) / "libslic3r" / "BossStepInvalidations.cpp").read_text()
            self.assertIn('{"filament_max_speed", steps({propagate(psSkirtBrim), propagate(psWipeTower)})}', text)

    def test_fill_feature_gates_boss_fill_pattern(self):
        with tempfile.TemporaryDirectory() as src:
            fill = self._parse(src, "crosshatch", id=10001, key="crosshatch",
                               components={"libslic3r": {"capabilities": ["fill"], "sources": []}})
            self.assertIn("boss_fill_pattern", gbf.boss_owned_keys([fill]))
            self.assertNotIn("boss_fill_pattern", gbf.boss_owned_keys([]))

    def test_config_option_keys_header_and_impl(self):
        with tempfile.TemporaryDirectory() as src, tempfile.TemporaryDirectory() as out:
            m = self._parse(src, "alpha", config_options=[
                {"key": "filament_max_speed", "invalidates": []}])
            gbf.emit_config_option_keys([m], Path(out))
            impl = (Path(out) / "slic3r-domain" / "BossConfigOptionKeys.cpp").read_text()
            hdr = (Path(out) / "include" / "boss" / "generated" / "BossConfigOptionKeys.hpp").read_text()
            self.assertIn('"filament_max_speed"', impl)
            self.assertIn("boss_config_option_keys();", hdr)

    def test_registry_owned_key_in_manifest_is_rejected(self):
        with tempfile.TemporaryDirectory() as src:
            m = self._parse(src, "alpha", config_options=[
                {"key": "boss_fill_pattern", "invalidates": ["posPrepareInfill"]}])
            with self.assertRaises(gbf.ManifestError):
                gbf.check_reserved_keys([m])


class GeneratedSourcesCMakeTests(unittest.TestCase):
    def test_generated_sources_var_emitted_even_when_empty(self):
        with tempfile.TemporaryDirectory() as out:
            gbf.emit_sources_cmake([], "libslic3r", Path(out))
            text = (Path(out) / "libslic3r" / "sources.cmake").read_text()
            self.assertIn("BOSS_LIBSLIC3R_GENERATED_SOURCES", text)
            self.assertIn("BossStepInvalidations.cpp", text)

    def test_domain_generated_sources_include_keys_impl(self):
        with tempfile.TemporaryDirectory() as out:
            gbf.emit_sources_cmake([], "slic3r-domain", Path(out))
            text = (Path(out) / "slic3r-domain" / "sources.cmake").read_text()
            self.assertIn("BossConfigOptionKeys.cpp", text)


if __name__ == "__main__":
    unittest.main()
