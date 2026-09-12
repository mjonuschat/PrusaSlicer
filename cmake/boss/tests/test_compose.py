import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "release"))
import compose  # noqa: E402


class LoadManifestTests(unittest.TestCase):
    def test_missing_file_returns_empty_list(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(compose.load_manifest(Path(tmp) / "missing.json"), [])

    def test_empty_excluded_list(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(json.dumps({"excluded": []}), encoding="utf-8")
            self.assertEqual(compose.load_manifest(path), [])

    def test_one_valid_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(
                json.dumps({"excluded": [{"branch": "feature-x", "reason": "not ready"}]}),
                encoding="utf-8",
            )
            self.assertEqual(
                compose.load_manifest(path),
                [compose.ExcludedEntry(branch="feature-x", reason="not ready")],
            )

    def test_invalid_json_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text("{not json", encoding="utf-8")
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)

    def test_unknown_top_level_key_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(
                json.dumps({"excluded": [], "unexpected": True}), encoding="utf-8"
            )
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)

    def test_excluded_not_a_list_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(json.dumps({"excluded": "feature-x"}), encoding="utf-8")
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)

    def test_entry_missing_reason_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(
                json.dumps({"excluded": [{"branch": "feature-x"}]}), encoding="utf-8"
            )
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)

    def test_entry_extra_key_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(
                json.dumps(
                    {"excluded": [{"branch": "feature-x", "reason": "r", "extra": 1}]}
                ),
                encoding="utf-8",
            )
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)

    def test_empty_branch_string_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(
                json.dumps({"excluded": [{"branch": "", "reason": "r"}]}), encoding="utf-8"
            )
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)

    def test_duplicate_branch_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "manifest.json"
            path.write_text(
                json.dumps(
                    {
                        "excluded": [
                            {"branch": "feature-x", "reason": "a"},
                            {"branch": "feature-x", "reason": "b"},
                        ]
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaises(compose.ManifestError):
                compose.load_manifest(path)


if __name__ == "__main__":
    unittest.main()
