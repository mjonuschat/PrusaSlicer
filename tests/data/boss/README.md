# BOSS test fixtures

Each subdirectory is one feature or bugfix, built as part of porting that
feature or fix — not ahead of time. Contents:

- `config.ini` — the BOSS config that exercises the behavior
- `model.stl` (or a small synthetic geometry generated in the test itself)
- `expected.txt` — the specific assertion(s) this fixture checks, in plain
  language

A fixture must fail when its feature/fix is absent and pass when present.
Prove this with commits in this order, not a single combined commit:

1. Add and commit the failing regression test.
2. Run it and confirm it fails, for the reason you expect.
3. Implement and commit the fix, in a separate commit.
4. Run the test again and confirm it now passes.

Keeping the test and the fix as separate commits means you can `git revert`
the fix commit alone at any point and re-confirm the test still fails —
proof the test is actually exercising the fix, not passing for an unrelated
reason.

Wire new fixtures into the nearest existing Catch2 suite (`fff_print`,
`libslic3r`, etc.) rather than creating a new suite per feature.
