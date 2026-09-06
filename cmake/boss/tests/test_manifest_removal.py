# cmake/boss/tests/test_manifest_removal.py
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import generate_boss_features as gbf  # noqa: E402
from test_generate_boss_features import write_manifest  # noqa: E402


class ManifestRemovalTests(unittest.TestCase):
    def test_removing_a_feature_directory_removes_it_from_the_composition(self):
        with tempfile.TemporaryDirectory() as features_tmp, tempfile.TemporaryDirectory() as out_tmp:
            features_dir = Path(features_tmp)
            write_manifest(
                features_dir, "alpha", id=1, key="alpha",
                trait="Slic3r::Boss::AlphaFeature",
                header="boss/features/alpha/AlphaFeature.hpp",
                components={"slic3r-domain": {"capabilities": ["fdm_config"], "sources": ["alpha.cpp"]}},
            )
            write_manifest(
                features_dir, "beta", id=2, key="beta",
                trait="Slic3r::Boss::BetaFeature",
                header="boss/features/beta/BetaFeature.hpp",
                components={"slic3r-domain": {"capabilities": ["fdm_config"], "sources": ["beta.cpp"]}},
            )
            self.assertEqual(gbf.generate(features_dir, Path(out_tmp)), 0)
            header_path = Path(out_tmp, "include", "boss", "generated", "BossFdmFeatures.hpp")
            sources_path = Path(out_tmp, "slic3r-domain", "sources.cmake")
            self.assertIn("AlphaFeature", header_path.read_text())
            self.assertIn("BetaFeature", header_path.read_text())
            self.assertIn("beta.cpp", sources_path.read_text())

            shutil.rmtree(features_dir / "beta")

            self.assertEqual(gbf.generate(features_dir, Path(out_tmp)), 0)
            regenerated_header = header_path.read_text()
            self.assertIn("AlphaFeature", regenerated_header)
            self.assertNotIn("BetaFeature", regenerated_header)
            regenerated_sources = sources_path.read_text()
            self.assertIn("alpha.cpp", regenerated_sources)
            self.assertNotIn("beta.cpp", regenerated_sources)


if __name__ == "__main__":
    unittest.main()
