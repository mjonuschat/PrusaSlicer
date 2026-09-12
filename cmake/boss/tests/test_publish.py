import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "release"))
import publish  # noqa: E402


class ResolveVersionTests(unittest.TestCase):
    def test_no_release_defaults_to_nightly(self):
        self.assertEqual(publish.resolve_version(None), "nightly")

    def test_release_is_used_verbatim(self):
        self.assertEqual(publish.resolve_version("2.1.0"), "2.1.0")


class WorkspaceDirForTests(unittest.TestCase):
    def test_workspace_dir_is_under_worktrees(self):
        repo = Path("/repo")
        workspace_dir = publish.workspace_dir_for(repo, "nightly")
        self.assertEqual(workspace_dir.parent, repo / ".worktrees")

    def test_workspace_dir_name_includes_version(self):
        workspace_dir = publish.workspace_dir_for(Path("/repo"), "nightly")
        self.assertIn("nightly", workspace_dir.name)

    def test_workspace_dir_is_unique_per_call(self):
        repo = Path("/repo")
        first = publish.workspace_dir_for(repo, "nightly")
        second = publish.workspace_dir_for(repo, "nightly")
        self.assertNotEqual(first, second)


@unittest.skipUnless(shutil.which("jj"), "jj binary not available")
class RunPublishRealJjTests(unittest.TestCase):
    def _run(self, cwd, *args):
        result = subprocess.run(["jj", *args], cwd=cwd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=f"jj {args} failed: {result.stderr}")
        return result.stdout

    def _run_git(self, cwd, *args):
        result = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=f"git {args} failed: {result.stderr}")
        return result.stdout

    def _make_repo_with_remote(self, repo_tmp: str, remote_tmp: str) -> Path:
        repo = Path(repo_tmp)
        remote = Path(remote_tmp)
        self._run_git(remote, "init", "--bare")

        self._run(repo, "git", "init", "--colocate")
        self._run(repo, "config", "set", "--repo", "user.name", "Test User")
        self._run(repo, "config", "set", "--repo", "user.email", "test@example.com")
        self._run_git(repo, "remote", "add", "origin", str(remote))

        self._run(repo, "new", "-m", "foundation", "root()")
        self._run(repo, "bookmark", "create", "foundation", "-r", "@")

        self._run(repo, "new", "-m", "bugfix a", "foundation")
        self._run(repo, "bookmark", "create", "bugfix-a", "-r", "@")

        self._run(repo, "edit", "foundation")
        return repo

    def test_default_version_composes_and_pushes_nightly(self):
        with tempfile.TemporaryDirectory() as repo_tmp, \
             tempfile.TemporaryDirectory() as remote_tmp, \
             tempfile.TemporaryDirectory() as manifest_tmp:
            repo = self._make_repo_with_remote(repo_tmp, remote_tmp)
            manifest_path = Path(manifest_tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            run = publish.make_subprocess_runner()
            result = publish.run_publish(
                run, repo, manifest_path, "foundation", publish.resolve_version(None)
            )

            self.assertEqual(result, 0)
            remote_bookmarks = self._run_git(remote_tmp, "for-each-ref", "--format=%(refname)")
            self.assertIn("build/nightly", remote_bookmarks)
            self.assertEqual(list(Path(repo_tmp, ".worktrees").glob("*")), [])

    def test_release_version_pushes_named_bookmark(self):
        with tempfile.TemporaryDirectory() as repo_tmp, \
             tempfile.TemporaryDirectory() as remote_tmp, \
             tempfile.TemporaryDirectory() as manifest_tmp:
            repo = self._make_repo_with_remote(repo_tmp, remote_tmp)
            manifest_path = Path(manifest_tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            run = publish.make_subprocess_runner()
            result = publish.run_publish(
                run, repo, manifest_path, "foundation", publish.resolve_version("2.1.0")
            )

            self.assertEqual(result, 0)
            remote_bookmarks = self._run_git(remote_tmp, "for-each-ref", "--format=%(refname)")
            self.assertIn("build/2.1.0", remote_bookmarks)

    def test_compose_failure_cleans_up_workspace_without_pushing(self):
        with tempfile.TemporaryDirectory() as repo_tmp, \
             tempfile.TemporaryDirectory() as remote_tmp, \
             tempfile.TemporaryDirectory() as manifest_tmp:
            repo = self._make_repo_with_remote(repo_tmp, remote_tmp)
            manifest_path = Path(manifest_tmp) / "manifest.json"
            manifest_path.write_text(
                json.dumps({"excluded": [{"branch": "feature-gone", "reason": "stale"}]}),
                encoding="utf-8",
            )

            run = publish.make_subprocess_runner()
            result = publish.run_publish(
                run, repo, manifest_path, "foundation", publish.resolve_version(None)
            )

            self.assertEqual(result, 1)
            remote_bookmarks = self._run_git(remote_tmp, "for-each-ref", "--format=%(refname)")
            self.assertNotIn("build/nightly", remote_bookmarks)
            self.assertEqual(list(Path(repo_tmp, ".worktrees").glob("*")), [])

    def test_keep_workspace_leaves_it_on_disk(self):
        with tempfile.TemporaryDirectory() as repo_tmp, \
             tempfile.TemporaryDirectory() as remote_tmp, \
             tempfile.TemporaryDirectory() as manifest_tmp:
            repo = self._make_repo_with_remote(repo_tmp, remote_tmp)
            manifest_path = Path(manifest_tmp) / "manifest.json"
            manifest_path.write_text(json.dumps({"excluded": []}), encoding="utf-8")

            run = publish.make_subprocess_runner()
            result = publish.run_publish(
                run, repo, manifest_path, "foundation", publish.resolve_version(None),
                keep_workspace=True,
            )

            self.assertEqual(result, 0)
            self.assertEqual(len(list(Path(repo_tmp, ".worktrees").glob("*"))), 1)


if __name__ == "__main__":
    unittest.main()
