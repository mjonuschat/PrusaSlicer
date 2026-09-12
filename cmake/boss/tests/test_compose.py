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
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
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
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            return "foundation: 111111 base\nboss: 222222 legacy\n"

        self.assertEqual(compose.list_all_branches(fake_run), [])

    def test_ignores_blank_lines(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            return "\nfeature-a: 333333 msg\n\n"

        self.assertEqual(compose.list_all_branches(fake_run), ["feature-a"])


class IsConflictedTests(unittest.TestCase):
    def test_true_output_means_conflicted(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            self.assertEqual(
                args, ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"]
            )
            return "true\n"

        self.assertTrue(compose.is_conflicted(fake_run))

    def test_false_output_means_clean(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            return "false\n"

        self.assertFalse(compose.is_conflicted(fake_run))

    def test_custom_revision_is_passed_through(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            self.assertEqual(args[4], "some-bookmark")
            return "false\n"

        compose.is_conflicted(fake_run, revision="some-bookmark")

    def test_unexpected_output_raises(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            return "maybe\n"

        with self.assertRaises(compose.ComposeError):
            compose.is_conflicted(fake_run)


class BookmarkTargetTests(unittest.TestCase):
    def test_returns_commit_id_when_bookmark_exists(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            self.assertEqual(
                args, ["jj", "log", "--no-graph", "-r", "build/nightly", "-T", "commit_id"]
            )
            return "abc123\n"

        self.assertEqual(compose.bookmark_target(fake_run, "build/nightly"), "abc123")

    def test_returns_none_when_bookmark_does_not_exist(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            raise compose.ComposeError("revision `build/nightly` doesn't exist")

        self.assertIsNone(compose.bookmark_target(fake_run, "build/nightly"))


class ComposeTitleTests(unittest.TestCase):
    def test_nightly_gets_fixed_title(self):
        self.assertEqual(compose.compose_title("nightly"), "PrusaSlicer (BOSS) Nightly Build")

    def test_versioned_release_names_the_version(self):
        self.assertEqual(
            compose.compose_title("3.0.0-alpha11"), "PrusaSlicer 3.0.0-alpha11 (BOSS)"
        )


class FormatFinalDescriptionTests(unittest.TestCase):
    def test_lists_branches_as_bullets(self):
        self.assertEqual(
            compose.format_final_description("Title", ["bugfix-a", "feature-a"]),
            "Title\n\n- bugfix-a\n- feature-a",
        )

    def test_no_branches_is_just_the_title(self):
        self.assertEqual(compose.format_final_description("Title", []), "Title")


class ComposeBranchesTests(unittest.TestCase):
    def test_merges_bugfixes_then_features_in_order(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        branch_set = compose.ComposeSet(
            bugfixes=["bugfix-a", "bugfix-b"], features=["feature-a"]
        )
        compose.compose_branches(fake_run, "foundation", branch_set, "build/1.2.3")

        new_or_describe_calls = [
            c for c in calls if c[:2] == ["jj", "new"] or c[:2] == ["jj", "describe"]
        ]
        self.assertEqual(
            new_or_describe_calls,
            [
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: base"],
                ["jj", "new", "@", "bugfix-a"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: merge bugfix-a"],
                ["jj", "new", "@", "bugfix-b"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: merge bugfix-b"],
                ["jj", "new", "@", "feature-a"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: merge feature-a"],
            ],
        )

    def test_stops_on_first_conflict_and_does_not_continue(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            if args[:2] == ["jj", "log"]:
                # Conflict on the first branch merge (bugfix-a); bugfix-b
                # and feature-a must never be attempted.
                return "true\n"
            return ""

        branch_set = compose.ComposeSet(
            bugfixes=["bugfix-a", "bugfix-b"], features=["feature-a"]
        )
        with self.assertRaisesRegex(compose.ComposeError, "bugfix-a"):
            compose.compose_branches(fake_run, "foundation", branch_set, "build/1.2.3")

        merged_branches = [c[3] for c in calls if c[:3] == ["jj", "new", "@"]]
        self.assertEqual(merged_branches, ["bugfix-a"])

    def test_empty_branch_set_still_creates_and_describes_base_commit(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            return ""

        compose.compose_branches(
            fake_run, "foundation", compose.ComposeSet([], []), "build/1.2.3"
        )
        self.assertEqual(
            calls,
            [
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: base"],
            ],
        )

    def test_each_describe_call_targets_only_at_symbol(self):
        """The describe target must never be a range: a range would also
        match the composed branches' own original commits, corrupting
        their messages and bookmarks (see round 2's regression)."""
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        branch_set = compose.ComposeSet(bugfixes=["bugfix-a"], features=[])
        compose.compose_branches(fake_run, "foundation", branch_set, "build/1.2.3")

        describe_calls = [c for c in calls if c[:2] == ["jj", "describe"]]
        self.assertEqual(len(describe_calls), 2)
        for call in describe_calls:
            self.assertEqual(call[:4], ["jj", "describe", "-r", "@"])
            self.assertTrue(call[5].startswith("build/1.2.3:"))


class RunComposeTests(unittest.TestCase):
    def test_successful_run_issues_expected_commands_in_order(self):
        calls = []

        def fake_run(args):
            if args == ["jj", "log", "--no-graph", "-r", "build/1.2.3", "-T", "commit_id"]:
                raise compose.ComposeError("revision `build/1.2.3` doesn't exist")
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
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
        final_description = "PrusaSlicer 1.2.3 (BOSS)\n\n- bugfix-a\n- feature-a"
        self.assertEqual(
            calls,
            [
                ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS,
                 "-T", 'commit_id.short() ++ "\\n"'],
                ["jj", "bookmark", "list"],
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: base"],
                ["jj", "new", "@", "bugfix-a"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: merge bugfix-a"],
                ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"],
                ["jj", "new", "@", "feature-a"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: merge feature-a"],
                ["jj", "log", "--no-graph", "-r", "@", "-T", "conflict"],
                ["jj", "describe", "-r", "@", "-m", final_description],
                ["jj", "bookmark", "set", "build/1.2.3", "-r", "@", "--allow-backwards"],
            ],
        )

    def test_missing_manifest_returns_error_without_running_jj(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "missing.json"
            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

        self.assertEqual(result, 1)
        self.assertEqual(calls, [])

    def test_stale_exclusion_returns_one_instead_of_raising(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
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

    def test_composition_conflict_returns_its_own_exit_code(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            if args[:2] == ["jj", "bookmark"]:
                return "feature-a: abc123 add a\n"
            if args[:2] == ["jj", "log"]:
                return "true\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

        self.assertEqual(result, compose.ComposeResult.CONFLICT)

    def test_empty_branch_set_still_describes_and_creates_bookmark(self):
        calls = []

        def fake_run(args):
            if args == ["jj", "log", "--no-graph", "-r", "build/1.2.3", "-T", "commit_id"]:
                raise compose.ComposeError("revision `build/1.2.3` doesn't exist")
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
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
                ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS,
                 "-T", 'commit_id.short() ++ "\\n"'],
                ["jj", "bookmark", "list"],
                ["jj", "new", "foundation"],
                ["jj", "describe", "-r", "@", "-m", "build/1.2.3: base"],
                ["jj", "describe", "-r", "@", "-m", "PrusaSlicer 1.2.3 (BOSS)"],
                ["jj", "bookmark", "set", "build/1.2.3", "-r", "@", "--allow-backwards"],
            ],
        )

    def test_first_compose_uses_bookmark_set_and_skips_abandon(self):
        calls = []

        def fake_run(args):
            if args == ["jj", "log", "--no-graph", "-r", "build/1.2.3", "-T", "commit_id"]:
                raise compose.ComposeError("revision `build/1.2.3` doesn't exist")
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            if args[:2] == ["jj", "bookmark"] and args[2] == "list":
                return "feature-a: abc123 add a\n"
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

        self.assertEqual(result, 0)
        self.assertIn(
            ["jj", "bookmark", "set", "build/1.2.3", "-r", "@", "--allow-backwards"], calls
        )
        self.assertFalse(any(c[:2] == ["jj", "abandon"] for c in calls))

    def test_recompose_abandons_previous_chain(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
            if args == ["jj", "log", "--no-graph", "-r", "build/1.2.3", "-T", "commit_id"]:
                return "oldtarget123\n"
            if args[:2] == ["jj", "bookmark"] and args[2] == "list":
                return "feature-a: abc123 add a\n"
            if args[:2] == ["jj", "log"]:
                return "false\n"
            return ""

        with tempfile.TemporaryDirectory() as tmp:
            manifest_path = Path(tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            result = compose.run_compose(fake_run, manifest_path, "foundation", "1.2.3", Path(tmp))

        self.assertEqual(result, 0)
        set_index = calls.index(
            ["jj", "bookmark", "set", "build/1.2.3", "-r", "@", "--allow-backwards"]
        )
        self.assertEqual(
            calls[set_index + 1],
            ["jj", "abandon", "-r", "::oldtarget123 ~ ::bookmarks()"],
        )

    def test_boss_feature_drift_aborts_before_bookmark_creation(self):
        def fake_run(args):
            if args[:5] == ["jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS]:
                return ""
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
        self.assertFalse(any(c[:2] == ["jj", "bookmark"] and c[2] == "set" for c in calls))


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
                joined = "\n---\n".join(entries)
                self.assertIn("build/9.9.9: base", joined)
                self.assertIn("build/9.9.9: merge bugfix-a", joined)
                self.assertIn("PrusaSlicer 9.9.9 (BOSS)", joined)
                self.assertIn("- bugfix-a", joined)
                self.assertIn("- feature-a", joined)

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

    def test_recomposing_same_bookmark_abandons_first_runs_commits(self):
        with tempfile.TemporaryDirectory() as manifest_tmp:
            with tempfile.TemporaryDirectory() as tmp:
                repo = Path(tmp)
                self._run_jj(repo, "git", "init", "--colocate")
                self._run_jj(repo, "config", "set", "--repo", "user.name", "Test User")
                self._run_jj(
                    repo, "config", "set", "--repo", "user.email", "test@example.com"
                )

                self._run_jj(repo, "new", "-m", "foundation", "root()")
                self._run_jj(repo, "bookmark", "create", "foundation", "-r", "@")
                self._run_jj(repo, "new", "-m", "feature a", "foundation")
                self._run_jj(repo, "bookmark", "create", "feature-a", "-r", "@")

                manifest_path = Path(manifest_tmp) / "manifest.json"
                manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")
                run = compose.make_subprocess_runner(repo)

                result = compose.run_compose(run, manifest_path, "foundation", "9.9.9", repo)
                self.assertEqual(result, 0)
                first_run_target = self._commit_id(repo, "build/9.9.9")

                result = compose.run_compose(run, manifest_path, "foundation", "9.9.9", repo)
                self.assertEqual(result, 0)
                second_run_target = self._commit_id(repo, "build/9.9.9")
                self.assertNotEqual(first_run_target, second_run_target)

                visible_heads = self._run_jj(
                    repo, "log", "--no-graph", "-r", "heads(all())", "-T", "commit_id ++ \"\\n\""
                ).split()
                self.assertNotIn(first_run_target, visible_heads)




class PruneOrphanedChainsTests(unittest.TestCase):
    PRUNE_LOG = [
        "jj", "log", "--no-graph", "-r", compose.ORPHANED_CHAINS,
        "-T", 'commit_id.short() ++ "\\n"',
    ]

    def test_no_orphans_issues_no_abandon(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            return ""

        self.assertEqual(compose.prune_orphaned_chains(fake_run), 0)
        self.assertEqual(calls, [self.PRUNE_LOG])

    def test_orphans_are_counted_and_abandoned(self):
        calls = []

        def fake_run(args):
            calls.append(list(args))
            if args[:2] == ["jj", "log"]:
                return "abc123\ndef456\n"
            return ""

        self.assertEqual(compose.prune_orphaned_chains(fake_run), 2)
        self.assertEqual(
            calls,
            [self.PRUNE_LOG, ["jj", "abandon", "-r", compose.ORPHANED_CHAINS]],
        )

    def test_revset_spares_bookmarked_chains_and_working_copies(self):
        self.assertIn("~ ::bookmarks()", compose.ORPHANED_CHAINS)
        self.assertIn("~ ::working_copies()", compose.ORPHANED_CHAINS)


if __name__ == "__main__":
    unittest.main()
