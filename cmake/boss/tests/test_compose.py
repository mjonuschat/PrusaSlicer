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


class ResolveBranchSetTests(unittest.TestCase):
    def test_no_exclusions_splits_by_prefix_and_sorts(self):
        result = compose.resolve_branch_set(
            ["feature-b", "bugfix-b", "feature-a", "bugfix-a"], []
        )
        self.assertEqual(result.bugfixes, ["bugfix-a", "bugfix-b"])
        self.assertEqual(result.features, ["feature-a", "feature-b"])

    def test_ordered_puts_bugfixes_before_features(self):
        result = compose.resolve_branch_set(["feature-a", "bugfix-a"], [])
        self.assertEqual(result.ordered(), ["bugfix-a", "feature-a"])

    def test_excluded_branch_is_removed(self):
        result = compose.resolve_branch_set(
            ["feature-a", "feature-b"],
            [compose.ExcludedEntry(branch="feature-a", reason="not ready")],
        )
        self.assertEqual(result.features, ["feature-b"])

    def test_non_matching_prefix_is_dropped(self):
        result = compose.resolve_branch_set(["foundation", "feature-a"], [])
        self.assertEqual(result.ordered(), ["feature-a"])

    def test_stale_exclusion_raises(self):
        with self.assertRaises(compose.ComposeError):
            compose.resolve_branch_set(
                ["feature-a"],
                [compose.ExcludedEntry(branch="feature-gone", reason="stale")],
            )


if __name__ == "__main__":
    unittest.main()
