import json
import shutil
import subprocess
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


class ListAllBranchesTests(unittest.TestCase):
    def test_parses_bugfix_and_feature_bookmarks(self):
        def fake_run(args):
            self.assertEqual(args, ["jj", "bookmark", "list"])
            return (
                "bugfix-wipe-zero: abc123 fix wipe\n"
                "feature-flowsnake-infill: def456 add flowsnake\n"
            )

        result = compose.list_all_branches(fake_run)
        self.assertEqual(
            sorted(result), ["bugfix-wipe-zero", "feature-flowsnake-infill"]
        )

    def test_ignores_non_bugfix_feature_bookmarks(self):
        def fake_run(args):
            return "foundation: 111111 base\nboss: 222222 legacy\n"

        self.assertEqual(compose.list_all_branches(fake_run), [])

    def test_ignores_blank_lines(self):
        def fake_run(args):
            return "\nfeature-a: 333333 msg\n\n"

        self.assertEqual(compose.list_all_branches(fake_run), ["feature-a"])


class IsConflictedTests(unittest.TestCase):
    def test_true_output_means_conflicted(self):
        def fake_run(args):
            self.assertEqual(
                args, ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"]
            )
            return "true\n"

        self.assertTrue(compose.is_conflicted(fake_run))

    def test_false_output_means_clean(self):
        def fake_run(args):
            return "false\n"

        self.assertFalse(compose.is_conflicted(fake_run))

    def test_custom_revision_is_passed_through(self):
        def fake_run(args):
            self.assertEqual(args[4], "some-bookmark")
            return "false\n"

        compose.is_conflicted(fake_run, revision="some-bookmark")

    def test_unexpected_output_raises(self):
        def fake_run(args):
            return "maybe\n"

        with self.assertRaises(compose.ComposeError):
            compose.is_conflicted(fake_run)


class ComposeBranchesTests(unittest.TestCase):
    def test_merges_bugfixes_then_features_in_order(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        branch_set = compose.ComposeSet(
            bugfixes=["bugfix-a", "bugfix-b"], features=["feature-a"]
        )
        compose.compose_branches(fake_run, "foundation", branch_set)

        new_calls = [c for c in calls if c[:2] == ["jj", "new"]]
        self.assertEqual(
            new_calls,
            [
                ["jj", "new", "foundation"],
                ["jj", "new", "@", "bugfix-a"],
                ["jj", "new", "@", "bugfix-b"],
                ["jj", "new", "@", "feature-a"],
            ],
        )

    def test_stops_on_first_conflict_and_does_not_continue(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:2] == ["jj", "log"]:
                # Conflict on the first branch merge (bugfix-a); bugfix-b
                # and feature-a must never be attempted.
                return "true\n"
            return ""

        branch_set = compose.ComposeSet(
            bugfixes=["bugfix-a", "bugfix-b"], features=["feature-a"]
        )
        with self.assertRaisesRegex(compose.ComposeError, "bugfix-a"):
            compose.compose_branches(fake_run, "foundation", branch_set)

        merged_branches = [c[3] for c in calls if c[:3] == ["jj", "new", "@"]]
        self.assertEqual(merged_branches, ["bugfix-a"])

    def test_empty_branch_set_still_creates_base_commit(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            return ""

        compose.compose_branches(fake_run, "foundation", compose.ComposeSet([], []))
        self.assertEqual(calls, [["jj", "new", "foundation"]])


class RunComposeTests(unittest.TestCase):
    def test_successful_run_issues_expected_commands_in_order(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:2] == ["jj", "bookmark"] and args[2] == "list":
                return (
                    "bugfix-a: abc123 fix a\n"
                    "feature-a: def456 add a\n"
                )
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3")

        self.assertEqual(result, 0)
        self.assertEqual(
            calls,
            [
                ["jj", "bookmark", "list"],
                ["jj", "new", "foundation"],
                ["jj", "new", "@", "bugfix-a"],
                ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"],
                ["jj", "new", "@", "feature-a"],
                ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"],
                [
                    "jj",
                    "describe",
                    "-r",
                    "foundation..@",
                    "-m",
                    "build/1.2.3: compose bugfix-a, feature-a",
                ],
                ["jj", "bookmark", "create", "build/1.2.3", "-r", "@"],
            ],
        )

    def test_missing_manifest_returns_error_without_running_jj(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "missing.json"
            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3")

        self.assertEqual(result, 1)
        self.assertEqual(calls, [])

    def test_stale_exclusion_returns_one_instead_of_raising(self):
        def fake_run(args):
            if args[:2] == ["jj", "bookmark"]:
                return "feature-a: abc123 add a\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(
                json.dumps({"excluded": [{"branch": "feature-gone", "reason": "stale"}]}),
                encoding="utf-8",
            )

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3")

        self.assertEqual(result, 1)

    def test_composition_conflict_returns_one_instead_of_raising(self):
        def fake_run(args):
            if args[:2] == ["jj", "bookmark"]:
                return "feature-a: abc123 add a\n"
            if args[:2] == ["jj", "log"]:
                return "true\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3")

        self.assertEqual(result, 1)

    def test_empty_branch_set_still_describes_and_creates_bookmark(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:2] == ["jj", "bookmark"] and args[2] == "list":
                return "foundation: 111111 base\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3")

        self.assertEqual(result, 0)
        self.assertEqual(
            calls,
            [
                ["jj", "bookmark", "list"],
                ["jj", "new", "foundation"],
                [
                    "jj",
                    "describe",
                    "-r",
                    "foundation..@",
                    "-m",
                    "build/1.2.3: compose no additional branches",
                ],
                ["jj", "bookmark", "create", "build/1.2.3", "-r", "@"],
            ],
        )


@unittest.skipUnless(shutil.which("jj"), "jj binary not available")
class RunComposeRealJjTests(unittest.TestCase):
    """Exercises run_compose against a real jj repo.

    Fake-runner tests only check the command list run_compose issues; they
    can't tell whether `jj describe -r <range>` actually reaches every
    intermediate commit compose_branches creates. Round 1's fake-runner
    tests passed while `jj git push` still failed on undescribed commits, so
    this test proves the fix against real jj instead.
    """

    def _run_jj(self, cwd, *args):
        result = subprocess.run(
            ["jj", *args], cwd=cwd, capture_output=True, text=True
        )
        self.assertEqual(
            result.returncode, 0, msg=f"jj {args} failed: {result.stderr}"
        )
        return result.stdout

    def test_composed_range_has_no_undescribed_commits(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            self._run_jj(repo, "git", "init", "--colocate")
            self._run_jj(repo, "config", "set", "--repo", "user.name", "Test User")
            self._run_jj(
                repo, "config", "set", "--repo", "user.email", "test@example.com"
            )

            # foundation, plus two branches each composed onto it, mirroring
            # what a real BOSS workspace looks like before composition.
            self._run_jj(repo, "new", "-m", "foundation", "root()")
            self._run_jj(repo, "bookmark", "create", "foundation", "-r", "@")

            self._run_jj(repo, "new", "-m", "bugfix a", "foundation")
            self._run_jj(repo, "bookmark", "create", "bugfix-a", "-r", "@")

            self._run_jj(repo, "new", "-m", "feature a", "foundation")
            self._run_jj(repo, "bookmark", "create", "feature-a", "-r", "@")

            manifest_path = repo / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            run = compose.make_subprocess_runner(repo)
            result = compose.run_compose(run, manifest_path, "foundation", "9.9.9")
            self.assertEqual(result, 0)

            descriptions = self._run_jj(
                repo,
                "log",
                "--no-graph",
                "-r",
                "foundation..build/9.9.9",
                "-T",
                'description ++ "\x1f"',
            )
            entries = descriptions.split("\x1f")[:-1]
            # "foundation..@" covers every commit compose_branches created (the
            # base commit plus one merge per branch) AND the two branch tips
            # themselves, since they're ancestors of the final merge too.
            self.assertEqual(len(entries), 5)
            for entry in entries:
                self.assertNotEqual(entry.strip(), "", msg=f"empty description in {entries!r}")
                self.assertIn("build/9.9.9: compose bugfix-a, feature-a", entry)


if __name__ == "__main__":
    unittest.main()
