# Agent handbook

Work against main on an issue branch. Read the issue and the current checkpoint
in docs/modernization-progress.md first. Preserve accepted fixtures, existing
known-good tags and the engine rules in AGENTS.md. All PRs and publication stay
inside msetaro/aftershock; merge only when every required check passes.

Recipes are executable shell blocks, run from the repository root:

- [Add a weapon](add-weapon.md)
- [Make and validate a level](make-level.md)
- [Import a glTF character](import-character.md)
- [Add an effect authoring file](add-effect.md) (runtime pending #161)
- [Debug a prediction mismatch](debug-prediction.md)
- [Bisect a golden change](bisect-golden.md)
- [Cut a known-good tag](known-good.md)

Before each recipe, choose a fresh RECIPE_OUT directory. Set AFTERSHOCK_SCRATCH
to a private invocation root if several commands should share cached builds/logs;
otherwise each invocation creates one. Children inherit the root. Keep it until
its evidence is no longer needed. No shared fixed scratch paths, displays or ports.

Runtime recipes need CLIENT/SERVER development binaries, installed CONTENT
(`quake3` or `openarena`), DATA (its pk3 directory), and MAP where requested.
Install the prerequisites documented in tests/README.md and the pinned cooker
requirements in a Python environment. CI installs its own packages; local agents
must not install system packages. Linux headless runs use Xvfb and Mesa software
rendering. Content is symlinked into private homes and never uploaded.

`python3 tools/agent run --map q3dm17 --script tools/agent/examples/playtest.json
--out ./playtest-result` builds a development client, launches headlessly, walks,
fires, captures three frames and writes a structured report. See
[the channel and CLI guide](../../tools/agent/README.md) for commands and schemas.

For fast feedback use `python3 tests/affected.py BASE_REF`. It includes dirty and
untracked paths, prints a 600-second budget and writes selected-test logs/report.
`--list` explains the selection; `--budget SECONDS` changes the limit. Unmapped
paths use core checks and are named for review. Exit 1 is failure; exit 2 is
incomplete/timeout. This is partial feedback and cannot replace full merge gates.

`python3 tests/suite.py` runs all ten active local regression variants from the
workflow, assuming prerequisites are installed. Supply `--glslang PATH` for pinned
glslang 16.6.0 and `--openarena-data PATH` for installed original OpenArena data.
Each job gets its own root/logs; suite-report.json records the head and dirty state.
`--job NAME` is partial and is reported as such. Run two clean worktrees with distinct
AFTERSHOCK_SCRATCH roots to check concurrency. Hosted compiler legs, including
MSVC, remain mandatory. See [tests/README.md](../../tests/README.md) for individual
commands, content sets and explicit fixture-regeneration procedures.

`python3 tests/agent_recipes.py --check` validates all recipe shell blocks.
Runtime CI executes them with development binaries and owned fixtures. Bisect and
tag mechanics use isolated Git fixtures; those tests never change repository tags.
