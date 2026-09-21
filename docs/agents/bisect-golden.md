# Bisect a golden change

Set RECIPE_REPOSITORY to this local repository, BISECT_GOOD to a passing revision
and BISECT_BAD to a failing revision that both support tests/run.py. The clone uses
local objects and private refs. It leaves the working checkout and accepted goldens
alone. For another gate, replace the unit command with that gate's stable command.

```sh
git clone --shared --no-hardlinks "$RECIPE_REPOSITORY" "$RECIPE_OUT/bisect"
(
  cd "$RECIPE_OUT/bisect"
  git bisect start "$BISECT_BAD" "$BISECT_GOOD"
  git bisect run python3 tests/run.py unit
  git bisect log > "$RECIPE_OUT/bisect.log"
  git bisect reset
)
```

Inspect the first bad diff and raw golden difference; do not regenerate references
to make the test pass. Historical commands must be run at their recorded revisions.
CI tests the Git mechanics with a tiny private history and a deterministic changed
value, while the full suite separately verifies the real engine goldens.
