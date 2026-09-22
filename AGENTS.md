# Aftershock — agent instructions

Aftershock is Matt's fork of the Quake3e engine (upstream: ec-/Quake3e). The engine is now C++20
(strict port merged 2026-09-14, PR #32). Vendored libraries under `third_party/libjpeg`, `third_party/libogg`,
`third_party/libvorbis`, `third_party/libcurl`, `third_party/libsdl` and the `engine/platform/asm` sources stay C/assembly.

## Current phase: modernization

Goal: modernize the engine for building old-school Call of Duty style games (CoD 4 to Ghosts era)
while preserving the classic id Tech 3 feel: fixed-timestep simulation, snapshot netcode with
client prediction, cvars, pk3 content, flat POD data, no per-frame allocation, deterministic
movement. Consoles are a long-term target, so console constraints apply now.

The roadmap is GitHub issues #1-#31, tracked in #25. Work them in the order the tracking issue
gives: Wave 1 foundations first (#3 regression suite, #31 recorded bugs, #1 and #2 decisions,
#4 boundaries, #5 build system, #8 code rules, then #6 RHI and #7 tooling). Read the issue before
starting it; the issue is the spec. `docs/cpp-port-plan.md` sections 10 and 11 are the reference
for the constraints and code rules below.

## Constraints in force (from plan section 10)

- C++20, `-fno-exceptions -fno-rtti`. No exceptions, no RTTI, no `dynamic_cast`.
- Fixed-width integers where representation matters; `long` is banned (32-bit on MSVC/Xbox);
  explicit `char` signedness where it matters (unsigned on aarch64). Foreign ABI types
  and explicit legacy compatibility widths are documented in tests/README.md.
- Use `Q_ASSERT` with side-effect-free arguments. The tidy gate enables assertions
  for analysis; production release builds still compile them out identically.
- No JIT, no runtime `dlopen` in engine code; static linking is the primary configuration.
- OS access only inside `engine/platform` and the filesystem layer
  (`engine/qcommon/files.cpp`). Portable code never calls POSIX, Win32, or SDL directly.
- 64-bit little-endian only.
- Error handling: `Com_Error` is a `longjmp` (decision #1, option 1). Engine core uses only
  trivially destructible types on the stack and as globals. RAII is allowed only in platform
  and GPU/OS resource wrappers whose destructors never run across a `Com_Error` unwind.
- Never restructure a floating-point expression in simulation, collision, movement, message,
  or snapshot code except in a bug-fix PR with a failing-then-passing test. Cross-build
  determinism is what netcode and demos depend on.
- Memory: ownership is by arena (hunk for level lifetime, zone for tagged small allocations,
  temp hunk). No `new`/`delete`/`malloc` outside the allocator layer. No smart pointers in
  engine core.
- Keep intentional undefined-behavior patterns (float punning, file buffers cast to structs)
  unless a PR replaces them with a proven-identical form and the codegen gate agrees.

## Workflow

- Work directly against `main`: one issue per branch named `issue/<number>-<slug>`, one PR per
  issue into `main`. After your gates pass and the self-review below, merge with a merge commit
  and continue. There is no human review step; the automated gates are the quality control, so
  never merge with a red or skipped required check. Never force-push; never rewrite history;
  never delete or move `known-good-*` tags (they are the rollback points). The old
  `modernization` integration branch is retired.
- Final gates must include the current `main` commit. Recheck the PR base commit just before
  merging; if main advanced during the checks, merge it forward and rerun the gates first.
- Checkpoint in `docs/modernization-progress.md`: per-issue status, decisions, "next action".
  Update after every meaningful step; on start, resume from it.
- Bugs found while doing something else go in `docs/bugs.md` and are fixed only in their own PR with a test. No unrelated refactoring in any PR.
- Nothing leaves this repository. Never open, update, or comment on pull requests or issues on
  ec-/Quake3e or any other external repository, and never publish anything outside
  msetaro/aftershock. The maintainer decides if and when something is shared upstream.
- Style: clang-format 21.1.8 is authoritative for owned C/C++ sources. Run
  `python3 tests/check_format.py`; vendored code, platform assembly and generated
  shader data retain their original formatting.

## Definition of done for a PR

Builds on every CI leg. Regression suite green, or goldens regenerated in the same PR with the
diff explained and the issue calling for the behavior change. Self-review: change matches the
issue scope; no unrelated refactoring; no new OS calls outside platform code; no non-trivial
destructors in engine core; no allocation added to per-frame paths; layout assertions kept for
every wire and file-format struct; issue updated with what changed and what was measured.

## Layout

- `engine/qcommon` shared core: cvars, commands, filesystem, packet protocols, collision (cm_*)
- `engine/server`, `engine/client` server and client
- `engine/ui` bounded cooked menu/HUD layout and navigation
- `engine/animation` cooked skeletal sampling, graphs, root motion, IK and hit boxes
- `engine/botlib` bot AI library
- `engine/devtools` optional development UI and bounded console/debug data
- `engine/render` portable scene/material/geometry frontend; `engine/rhi` GPU contract
- `engine/renderervk` sole Vulkan backend; the legacy OpenGL renderers are retired
- `engine/renderercommon` shared image/font routines and client renderer ABI
- `engine/platform/unix`, `engine/platform/win32`, `engine/platform/sdl` platform layers
- `engine/platform/asm` hand-written assembly; symbols it references carry `Q_EXTERN_C`
- `engine/sound` sound codecs and mixing; platform owns device backends
- `engine/public` native game/cgame/UI service contracts
- `game/cgame`, `game/game`, `game/ui`, `game/bg` static native game implementation
- `engine/renderervk/shaders/spirv/shader_data.cpp` is generated by `bin2hex.cpp`; never hand-edit
- `tools/port` the port-era gates (layout, symbol, codegen, differential, math, build matrix);
  permanent regression commands live in `tests/`

## Building

CMake 3.25+ is the primary build for #5. Use Ninja on Linux/macOS and in an MSVC
developer shell. Visual Studio projects are generated from the same source lists.
The default builds the client and dedicated server with a static Vulkan renderer;
optional PC renderer modules use a separate build directory. `AFTERSHOCK_DEVTOOLS=ON`
adds the development overlay; shipping builds leave it OFF.

```
cmake --workflow --preset release
cmake --workflow --preset debug
cmake --workflow --preset msvc-x64               # Windows: generate and build VS 2022
cmake --workflow --preset msvc-arm64             # Windows ARM64 cross-build
cmake -S . -B build/modules -G Ninja -DUSE_RENDERER_DLOPEN=ON
cmake --build build/modules
cmake -S . -B build/mingw -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw64.cmake -DUSE_CURL=OFF
cmake --build build/mingw
```

Set `BUILD_CLIENT=OFF` for server-only, `BUILD_SERVER=OFF` for client-only, or
`USE_RENDERER_DLOPEN=ON` for loadable renderers. Outputs are inside each CMake build
directory under `<config>-<platform>-<arch>/`. `cmake/Sources.cmake` owns the explicit
source lists; do not add source globs. ccache is detected automatically. MinGW curl builds also require target zlib (installed in hosted MSYS CI). Game
libraries remain static in every configuration; renderer modules are optional.

The migration checkpoint in docs/modernization-progress.md records raw Make/CMake
object comparisons and hosted gates. Historical port commands must be run at
their recorded revisions. Never regenerate accepted test goldens for build changes.

Local machine: gcc 15.2, clang 21 (+libc++), cross compilers for mingw x86_64, aarch64, armhf,
ppc64le, faketime, Xvfb, Mesa software drivers, 20 cores. Agent shells have no `DISPLAY`: run
the client under `Xvfb` with Mesa's llvmpipe/lavapipe. Game data is in `~/.q3a/baseq3/` and is
found automatically; never commit or copy pak files into the repo.

## Verification commands

Start with the [agent handbook](docs/agents/README.md): executable authoring,
playtest, prediction, bisect and rollback-tag recipes, plus full-suite instructions.
[tests/README.md](tests/README.md) is the individual-command and prerequisite reference.

- Fast feedback: `python3 tests/affected.py BASE_REF` (600-second default budget;
  timeout/incomplete is a nonzero result, never merge acceptance).
- Full local regression: `python3 tests/suite.py --glslang PATH --openarena-data PATH`.
  Each invocation/job owns its scratch root. Hosted compiler checks remain required.
- Authored UI: `python3 tests/ui_framework.py` and
  `python3 tests/ui_runtime.py --binary CLIENT` (1080p, 1440p and 4K; both content sets).
- Cosmetic physics: `python3 tests/physics.py` and
  `python3 tests/physics_runtime.py --binary CLIENT` (both content sets supported).
- Presentation controls: `python3 tests/fidelity_runtime.py --binary CLIENT` and
  `python3 tests/streaming_runtime.py --binary CLIENT`; their `--measure-gpu` options
  require the documented reference GPU. `python3 tests/streaming.py` checks residency.
- Recipe syntax: `python3 tests/agent_recipes.py --check`; runtime CI executes the
  same shell blocks using owned fixtures and private Git histories.

Local runtime/differential/demo commands use installed Quake 3 paks. Hosted CI uses
OpenArena: stage with `python3 tests/openarena.py`, then pass
`--content openarena --data $AFTERSHOCK_SCRATCH/aftershock-openarena-baseoa` to those three commands.
The finished network driver is retained unchanged; its evidence belongs to #3
merge `8692b422`, before the path/build migrations. Do not rerun its negative control. `tests/README.md` documents explicit fixture
regeneration; CI never regenerates. Existing port-era layout/symbol/codegen oracles
remain available under `tools/port` for changes requiring those gates.

## Conventions

- Commit messages: `<module>: <what>` like upstream, e.g. `qcommon: add layout assertions for msg_t`.
- Subsystem prefixes (`Sys_`, `Com_`, `FS_`, `CL_`, `SV_`, `R_`, `S_`, `Cvar_`, `Cmd_`) and
  `*_public.h` vs `*_local.h` are the subsystem boundaries; include only other subsystems' public
  headers. CI enforces this; `docs/subsystems.md` records ownership.
