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
        compose.compose_branches(fake_run, "foundation", branch_set, "compose msg")

        new_or_describe_calls = [
            c for c in calls if c[:2] == ["jj", "new"] or c[:2] == ["jj", "describe"]
        ]
        self.assertEqual(
            new_or_describe_calls,
            [
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", "compose msg"],
                ["jj", "new", "@", "bugfix-a"],
                ["jj", "describe", "-r", "@", "-m", "compose msg"],
                ["jj", "new", "@", "bugfix-b"],
                ["jj", "describe", "-r", "@", "-m", "compose msg"],
                ["jj", "new", "@", "feature-a"],
                ["jj", "describe", "-r", "@", "-m", "compose msg"],
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
            compose.compose_branches(fake_run, "foundation", branch_set, "compose msg")

        merged_branches = [c[3] for c in calls if c[:3] == ["jj", "new", "@"]]
        self.assertEqual(merged_branches, ["bugfix-a"])

    def test_empty_branch_set_still_creates_and_describes_base_commit(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            return ""

        compose.compose_branches(
            fake_run, "foundation", compose.ComposeSet([], []), "compose msg"
        )
        self.assertEqual(
            calls,
            [
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", "compose msg"],
            ],
        )

    def test_each_describe_call_targets_only_at_symbol(self):
        """The describe target must never be a range: a range would also
        match the composed branches' own original commits, corrupting
        their messages and bookmarks (see round 2's regression)."""
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        branch_set = compose.ComposeSet(bugfixes=["bugfix-a"], features=[])
        compose.compose_branches(fake_run, "foundation", branch_set, "compose msg")

        describe_calls = [c for c in calls if c[:2] == ["jj", "describe"]]
        for call in describe_calls:
            self.assertEqual(call, ["jj", "describe", "-r", "@", "-m", "compose msg"])


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

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

        self.assertEqual(result, 0)
        description = "build/1.2.3: compose bugfix-a, feature-a"
        self.assertEqual(
            calls,
            [
                ["jj", "bookmark", "list"],
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", description],
                ["jj", "new", "@", "bugfix-a"],
                ["jj", "describe", "-r", "@", "-m", description],
                ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"],
                ["jj", "new", "@", "feature-a"],
                ["jj", "describe", "-r", "@", "-m", description],
                ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"],
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
            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

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

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

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

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

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

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

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
                    "@",
                    "-m",
                    "build/1.2.3: compose no additional branches",
                ],
                ["jj", "bookmark", "create", "build/1.2.3", "-r", "@"],
            ],
        )

    def test_boss_feature_drift_aborts_before_bookmark_creation(self):
        def fake_run(args):
            if args[:2] == ["jj", "bookmark"] and args[2] == "list":
                return ""
            return ""

        with tempfile.TemporaryDirectory() as manifest_tmp, tempfile.TemporaryDirectory() as repo_tmp:
            manifest_path = Path(manifest_tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            repo = Path(repo_tmp)
            feature_dir = repo / "src" / "boss" / "include" / "boss" / "features" / "alpha"
            feature_dir.mkdir(parents=True)
            (feature_dir / "boss-feature.json").write_text(
                json.dumps({
                    "name": "alpha",
                    "id": 99,
                    "key": "alpha",
                    "trait": "Slic3r::Boss::AlphaFeature",
                    "header": "boss/features/alpha/AlphaFeature.hpp",
                    "components": {"slic3r-domain": {"capabilities": ["fdm_config"], "sources": []}},
                }),
                encoding="utf-8",
            )
            (feature_dir / "AlphaFeature.hpp").write_text(
                "struct AlphaFeature {\n    static constexpr int id = 1;\n};\n",
                encoding="utf-8",
            )

            calls = []

            def recording_run(args):
                calls.append(list(args))
                return fake_run(args)

            result = compose.run_compose(recording_run, manifest_path, "foundation", "1.2.3", repo)

        self.assertEqual(result, 1)
        self.assertNotIn(["jj", "bookmark", "create", "build/1.2.3", "-r", "@"], calls)


@unittest.skipUnless(shutil.which("jj"), "jj binary not available")
class RunComposeRealJjTests(unittest.TestCase):
    """Exercises run_compose against a real jj repo.

    Fake-runner tests only check the command list run_compose issues; they
    can't tell what `jj describe` actually touches. Round 1's fake-runner
    tests passed while `jj git push` still failed on undescribed commits.
    Round 2's fake-runner tests (and even round 2's own real-jj test) passed
    while `jj describe -r "<base>..@"` silently overwrote a composed
    branch's own commit message and moved its bookmark, because that range
    includes the branch's own tip commit as well as the merge commits.

    This test proves both properties round 3 requires: every commit
    compose_branches itself creates gets a real description, AND a composed
    branch's own commit/bookmark is byte-for-byte unchanged afterward.
    """

    def _run_jj(self, cwd, *args):
        result = subprocess.run(
            ["jj", *args], cwd=cwd, capture_output=True, text=True
        )
        self.assertEqual(
            result.returncode, 0, msg=f"jj {args} failed: {result.stderr}"
        )
        return result.stdout

    def _commit_id(self, repo, revision):
        return self._run_jj(
            repo, "log", "--no-graph", "-r", revision, "-T", "commit_id"
        ).strip()

    def _description(self, repo, revision):
        return self._run_jj(
            repo, "log", "--no-graph", "-r", revision, "-T", "description"
        )

    def test_composed_commits_described_and_branch_tips_untouched(self):
        with tempfile.TemporaryDirectory() as manifest_tmp:
            with tempfile.TemporaryDirectory() as tmp:
                repo = Path(tmp)
                self._run_jj(repo, "git", "init", "--colocate")
                self._run_jj(repo, "config", "set", "--repo", "user.name", "Test User")
                self._run_jj(
                    repo, "config", "set", "--repo", "user.email", "test@example.com"
                )

                # foundation, plus two branches each composed onto it,
                # mirroring what a real BOSS workspace looks like before
                # composition. @ is left on feature-a (the last-created
                # branch) on purpose: that is the position most exposed to
                # a describe call that reaches too far, so it is the
                # scenario round 2's regression would have hit hardest.
                self._run_jj(repo, "new", "-m", "foundation", "root()")
                self._run_jj(repo, "bookmark", "create", "foundation", "-r", "@")

                self._run_jj(repo, "new", "-m", "bugfix a", "foundation")
                self._run_jj(repo, "bookmark", "create", "bugfix-a", "-r", "@")

                self._run_jj(repo, "new", "-m", "feature a", "foundation")
                self._run_jj(repo, "bookmark", "create", "feature-a", "-r", "@")

                # Record each branch's own commit id, description, and
                # bookmark target BEFORE composing, to compare byte-for-byte
                # afterward.
                before = {}
                for branch in ("bugfix-a", "feature-a"):
                    before[branch] = (
                        self._commit_id(repo, branch),
                        self._description(repo, branch),
                        self._run_jj(repo, "bookmark", "list", "-r", branch),
                    )
                self.assertIn("bugfix a", before["bugfix-a"][1])
                self.assertIn("feature a", before["feature-a"][1])

                # The manifest is written outside the jj repo on purpose:
                # writing it inside the repo, while @ sits on a branch's own
                # commit, would get folded into that commit by jj's ordinary
                # working-copy auto-snapshot on the next jj invocation --
                # a test-harness artifact unrelated to compose.py, but one
                # that would make this test's own "before" snapshot wrong.
                manifest_path = Path(manifest_tmp) / "manifest.json"
                manifest_path.write_text(
                    json.dumps({"excluded": []}), encoding="utf-8"
                )

                run = compose.make_subprocess_runner(repo)
                result = compose.run_compose(run, manifest_path, "foundation", "9.9.9", repo)
                self.assertEqual(result, 0)

                # Property (a): every commit compose_branches itself created
                # (the base commit plus one merge per branch) has a
                # non-empty description naming the composition. This range
                # intentionally excludes the branch tips, since checking
                # their descriptions is property (b) below, not this one.
                descriptions = self._run_jj(
                    repo,
                    "log",
                    "--no-graph",
                    "-r",
                    "(foundation..build/9.9.9) ~ (bugfix-a | feature-a)",
                    "-T",
                    'description ++ "\x1f"',
                )
                entries = descriptions.split("\x1f")[:-1]
                self.assertEqual(len(entries), 3, msg=f"entries={entries!r}")
                for entry in entries:
                    self.assertNotEqual(
                        entry.strip(), "", msg=f"empty description in {entries!r}"
                    )
                    self.assertIn("build/9.9.9: compose bugfix-a, feature-a", entry)

                # Property (b): each branch's own commit id, description,
                # and bookmark target are completely unchanged after
                # composition. This is exactly the check round 2 skipped.
                for branch, (commit_before, desc_before, bookmark_before) in before.items():
                    commit_after = self._commit_id(repo, branch)
                    desc_after = self._description(repo, branch)
                    bookmark_after = self._run_jj(
                        repo, "bookmark", "list", "-r", branch
                    )
                    self.assertEqual(commit_before, commit_after, msg=branch)
                    self.assertEqual(desc_before, desc_after, msg=branch)
                    self.assertEqual(bookmark_before, bookmark_after, msg=branch)


if __name__ == "__main__":
    unittest.main()
