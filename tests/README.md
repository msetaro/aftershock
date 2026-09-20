# Permanent regression suite (#3)

Run from the repository root with Python 3, CMake 3.25+, Ninja, GCC or Clang, and binutils.
Tests call real engine functions with production flags from CMake's compile database. Test drivers provide only
isolated allocator/log/file stubs and instrumentation; production code is unchanged.

Test build directories contain `compile_commands.json` and `build.log`. Separate
output directories isolate compiler and instrumentation settings; changing a
setting refreshes the CMake cache before building. Unit and download probes select
actual client/server objects, and lifetime analysis retains every native wrapper
command across static and module configurations. ccache is used when installed.

For a normal engine build, `cmake --workflow --preset release` configures and builds
all targets. `debug`, `msvc-x64` and `msvc-arm64` presets are also available; Windows
Visual Studio projects are generated. See `AGENTS.md` for renderer/cross settings.

```
python3 tests/native_math.py
python3 tests/rhi.py
python3 tests/render_graph.py
python3 tests/cook.py
python3 tests/iqm_scale.py
python3 tests/cook_runtime.py
python3 tests/cook_runtime.py --modules --output /tmp/cook-modules
python3 tests/devtools.py
python3 tests/shaders.py --compiler /path/to/glslang-16.6.0
python3 tests/vulkan_acquire.py
python3 tests/check_format.py
python3 tests/check_types.py
python3 tests/check_tidy.py
python3 tests/check_lifetimes.py
python3 tests/check_boundaries.py
python3 tests/run.py unit --negative-control
python3 tests/run.py unit --cc clang --cxx 'clang++ -stdlib=libc++' --output /tmp/tests-clang
python3 tests/check_known_bugs.py
python3 tests/check_frames.py
python3 tests/run.py unit --cc clang --cxx clang++ --sanitize --known-bugs --output /tmp/tests-sanitized
python3 tests/run.py unit --cc clang --cxx clang++ --sanitize --pointer-compare --output /tmp/tests-pointers
```

`python3 tests/cook.py` requires the pinned Pillow version from
`tools/cook/requirements.txt`, CMake/Ninja and a host C++ compiler. It cooks owned
static, mirrored and skinned glTF/GLB sources; checks named clips, coordinates,
BC7/BC5/BC4 KTX2 mip chains and embedded/manifest hashes; verifies no-op and
selective recooking; and feeds the committed Blender character into production
IQM pose code. `--cxx` selects GCC or Clang/libc++. Its source fixture/provenance
is in `tests/assets/cook-character`; CI never reauthors it. No game paks are used. It also
checks authored WAV/OGG sources through the production PCM codec, twelve bounded
model/material replacements and 10,000 allocation-free idle publication polls.

`python3 tests/iqm_scale.py` checks native IQM scale-before-rotation around all
axes, signed/nonuniform/unit scales, inverse composition and cooked glTF/native
pose parity. It uses the same Pillow/compiler prerequisites, runs with UBSan,
and accepts `--cxx`/`--output`. Expected vertex coordinates are analytical; no
accepted model, frame or demo fixture is regenerated.

`python3 tests/cook_runtime.py` uses real ImGui input under Xvfb/lavapipe to load
the owned Blender character, select both clips and measure a watched texture
edit against a one-second gate. It edits only temporary source copies. It checks
model/clip and material changes, stable handles and storage across repeated
updates, idle UI allocations, and reload/rendering after video restart. Use
`--modules` to build the optional renderer module, or `--binary` to reuse an
existing development client. Defaults use installed Quake 3; hosted runs pass
`--content openarena --data /tmp/aftershock-openarena-baseoa`. No game paks are
committed or uploaded. Screenshot/log artifacts are separate from accepted goldens.


`python3 tests/render_graph.py` checks the portable fixed pass declarations,
dependencies and resource lifetimes, then observes production Vulkan image,
render-pass and framebuffer creation without a GPU. Its frozen 36-configuration
trace from #7 merge e4f2d70a covers direct/offscreen, bloom, stencil, independent
main/screen-map MSAA and capture. Both GCC and Clang/libc++ must match the same
trace; this migration does not regenerate accepted demo or pixel fixtures.

`python3 tests/rhi.py` checks the alternative backend against the public RHI
header alone, then exercises the production Vulkan uniform upload path without a
GPU: alignment, exact bytes, binding offsets, exhaustion and independent frame slots.
The stub reports unavailable and is never linked into a client. All CMake client
builds compile it; the two unit CI compilers also run the contract checks.
The RHI is being extracted in stages under #6; this currently covers uploads,
textures, pipeline descriptions, command/wait boundaries and timings, not a
completed second renderer. Texture checks preserve
all five format/three address mappings and binding/destruction behavior. Demo runs retain `gfxinfo`/`vkinfo`
measurements and report host wall time including startup; these are not GPU timings.
`python3 tests/demo.py --measure-gpu` first runs the unchanged frame gate, then
replays both Vulkan fixtures twice with the real clock to report completed GPU
scopes. Faketime also changes Mesa's software timestamps, so only the separate
`*-timing-*.log` values are performance measurements. These samples are
informational; hardware/driver differences are not a performance failure gate.
`python3 tests/demo.py --modules --lifecycle` also checks fixed frames through the
optional PC renderer module boundary and after video restart for Quake 3. Hosted
OpenArena runs `--modules` in fresh processes alongside the primary static renderer.
Its post-restart portrait/lagometer state differs from the fresh-process goldens;
this is not a supported restart-to-golden comparison. Renderer module API 10
requires rebuilding old modules with the client; changes cover opaque platform
handles and two filesystem cache imports, not scene or game services.
The RHI check also rejects GPU SDK dependencies in the frontend/client headers and
checks explicit stream/raster bindings, upload exhaustion and sampler wait ordering.
The backend dependency check rejects frontend headers. Missing loader entries must
return a diagnostic and release the initialization output pointer; the alternative
backend implements the same configuration and host-service records.
Controlled GPU failures must return status and fatal/drop diagnostics without
calling the engine error callback. Frontend image checks cover all five converted
byte layouts and release scratch before reporting either success or a GPU error.
The RHI retains at most 32 scopes per frame, reads available results after the
existing frame fence, and never adds a query wait. Its check covers timestamp
wrap, unavailable results, scope exhaustion, and duplicate scope completion.

`python3 tests/shaders.py --compiler /path/to/glslang-16.6.0` recompiles all 74
shader variants and compares every SPIR-V byte and reflected interface against the
committed offline cache. The pinned compiler is [Khronos glslang 16.6.0](https://github.com/KhronosGroup/glslang/releases/tag/16.6.0).
CI verifies the release archive SHA-256 before executing it. Shader sources,
quoted includes, stage/defines/options, target and compiler identity contribute to
the package hash; payload hashes and reflected bindings/locations/member offsets
are included too. Source line endings normalize to LF for identical Windows keys.

Vulkan CMake builds require Python 3.9+ and generate `shaders/shader_package.h`
and `shader_package.json` in the build directory. Unchanged inputs reuse verified
committed SPIR-V. A changed recipe/source/include requires the pinned compiler
(`-DSHADER_COMPILER=/path/to/glslang`); a missing compiler fails the build. To force
build-time recompilation, run `cmake --build build/release --target shaders` with
that compiler configured. The runtime consumes packaged bytes, never shader source.
This pipeline does not regenerate demos, frame goldens or committed shader data.
The source cache and compiled path currently produce identical shader bytes.

`python3 tests/demo.py --pipeline-cache` shares only a temporary cache directory
between otherwise fresh client processes and requires restoration on each second
Vulkan replay. `--lifecycle` also requires restoration after video restart. Frame
hash checks remain unchanged. Cache files live under the home game directory's
`cache/`, use the full shader/device/driver identity in their checked payload, and
are optional performance data. Missing, incomplete or incompatible data is a miss.
The local Mesa driver exports a 32-byte cache header; cache persistence passing on
that driver is not a pipeline compilation performance claim.

`python3 tests/window.py --binary /path/to/quake3e.x64` checks real window resize,
swapchain recreation, hidden-window FBO capture and restored capture on a private
Xvfb display. It selects only the window whose PID matches its launched client.
It needs `x11-utils` (`xwininfo`/`xprop`) and libX11, plus the normal runtime
prerequisites. Hosted CI passes OpenArena content/data arguments. Hiding generates
the SDL hidden event that uses the engine's minimized path; this is not a desktop
window-manager iconification test. Its real-clock captures check lifecycle and
size; fixed demo replay remains the separate pixel-equality gate.

`python3 tests/vulkan_acquire.py` runs the real Vulkan frame acquisition method
with controlled callbacks and stops at command recording. It needs no GPU, window
or content. Success/suboptimal must retain the acquired image index; timeout and
not-ready must report the existing fatal acquisition error before recording or
marking an image acquired. Both compiler unit jobs run these four cases.

Central model/BSP/AAS file records and the shared state/font records assert size,
alignment, trivial copyability and standard layout in their owning headers.
Their integer fields have explicit widths; the trajectory enum keeps its existing
unsigned 32-bit C++ representation. The native C reference remains buildable and
its ABI comparison runs in both compiler jobs. These changes preserve all accepted
wire, file, collision and replay goldens.

Image header and PK3 cache records also assert layout/type traits in their owning
sources. Decoded BMP, TGA and PNG IHDR structs retain their existing padding;
the assertions do not pack them to the serialized header length. Cache version zero
retains its existing platform signature and Windows/non-Windows field widths.
PCX byte fields are unsigned; the diagnostic preserves its prior host-char display.

Journal events, browser-cache address/server records and routing caches also
assert their existing record layouts. IPv4-only and IPv6 address variants retain
their distinct sizes. Existing enum promotions remain intact; their storage width
is asserted to be 32 bits. Pointer-bearing journal/routing records keep the
existing 64-bit layout, including pointer and variable-tail offsets.

`python3 tests/check_types.py` bans bare `long` types in owned engine/game
sources, including inactive platform branches. It shares the boundary check's
comment/string lexer and runs positive and negative controls. Library-facing
stdio, curl, Vorbis and Xlib values use their native ABI types; these are foreign
contracts, not portable integer storage. Internal replacements use explicit widths.
The legacy script arithmetic, seek/config-journal lengths and hash accumulators
retain explicit Windows/non-Windows widths to preserve existing behavior. Changing
those compatibility contracts requires separate behavior-change evidence.

The portable frontend IQM header carries eight file-layout contracts. WAV
scalars, browser-cache counters/size, and routing-cache size use explicit widths.
ADPCM sample/index storage, bot characteristic tags, signed chat offsets and
qsort copy bytes also have explicit types; text characters and boolean character
flags retain their existing interpretation.

The thirteen asset-free groups include wire/file layout. The negative control
moves the active GCC SSE Q_rsqrt return one ULP toward infinity in a temporary
source copy; the golden comparison must reject it. Clang requires libc++-dev and
libc++abi-dev. Cross compilation and MSVC builds run in CI; Linux executable tests
do not claim to execute on those targets.

`python3 tests/download.py` checks the real download begin/cleanup path with
libcurl, without a network transfer. It also compiles the option wrapper with
`-Werror=varargs` and checks long/pointer/offset forwarding through a local-file
transfer, including body suppression, private-data identity and a size limit. It requires libcurl development headers
and the library (`libcurl4-openssl-dev` on Ubuntu); hosted runtime CI installs them.
The URL cases cover bases with/without a trailing slash, `%1` templates, escaping,
and an empty base. File/cvar/UI operations are isolated by test stubs. Explicit
`python3 tests/download.py --regenerate` creates the reviewed URL golden; CI never
regenerates it. `--cc` and `--cxx` select the compiler as in the unit driver.

`python3 tests/audio.py` verifies the native ALSA callbacks have pthread-compatible
types, submits samples through both MMAP and DIRECT paths to ALSA's `null` output,
and joins both threads. It needs ALSA development files (`libasound2-dev` on Ubuntu)
and no physical audio device. CI installs those files in the runtime job. The test
uses the real ALSA implementation; wrappers count successful paths without replacing
the calls. `--cxx` selects the compiler. A missing device/library, no sample submission,
wrong callback type, or shutdown timeout fails the test.

`python3 tests/check_format.py` enforces clang-format 21.1.8 on tracked C/C++
headers, sources and includes in engine, game, tools/port and tests/probes.
Vendor sources, platform assembly and generated shader data are excluded.
`--clang-format` selects the command; hosted CI uses
`--clang-format 'pipx run --spec clang-format==21.1.8 clang-format'`.
Use the same version for edits. The config preserves stringified macro arguments
and disables trailing-comment alignment so one pass is stable. The initial
format commit was checked against release assembly across both renderers and
native/cross builds; only inline-assembly source-location comments differed.

`python3 tests/check_tidy.py` analyzes owned C++ sources with static and module
CMake commands, including each native module wrapper. It requires
clang-tidy and the client build headers (`clang-tidy`, `libsdl2-dev`,
`libcurl4-openssl-dev`, `mesa-common-dev`, and Ninja on hosted Ubuntu).
`--clang-tidy`, `--jobs` and `--output` select the executable, parallelism and
retained evidence. Duplicate includes, misleading indentation and assertion side
effects fail CI. `Q_ASSERT` is an object-like alias of `assert`; existing conditions
and release code are unchanged. Analysis appends `-UNDEBUG` to production flags
so assertions remain visible to the check. Positive controls allow the existing
pure math helpers; negative controls reject increments, mutating calls and an
increment nested inside an allowed helper. This does not alter build flags.
The existing bugprone/portability subset, performance-* and the single
modernize-redundant-void-arg check are advisory; CI retains every diagnostic
and a JSON summary. Enum shrinking and pointer rewrites are not automatic fixes.
The driver checks the positive case and rejects a duplicate-include control.

`python3 tests/check_lifetimes.py` checks non-trivial locals, parameters, globals,
statics and temporaries in active Linux engine code and included engine headers,
using static and module configurations. It requires clang-query (clang-tools in CI),
Clang and the client build headers. `--clang-query` selects a versioned executable;
`--output` retains the full compile database and per-batch AST evidence. Analysis
runs at most 16 compilation commands per process, retaining every configuration
while bounding clang-query memory (including the game amalgamation). Its controls reject seven
owning objects (including aliases, inheritance, arrays and std::string), and accept
trivial/defaulted destructors and pointers. Platform/vendor directories are excluded;
inactive preprocessor branches remain part of self-review. See plan section 11 for
the permanent longjmp decision and narrowly permitted resource-wrapper boundary.

`python3 tests/native_math.py --cc clang` checks the imported native game Q_rsqrt
in optimized and ASan builds. Eight fixed result words come from the unmodified
GPL routine measured in a freestanding 32-bit SSE C executable. CI needs no 32-bit
runtime: it checks those words using the selected native C compiler. This source
is a dependency of #2 and is not linked into the engine yet. Its original GPL
source and import hashes are documented in docs/bugs.md.

`python3 tests/format.py` checks the real engine C++ and native game C/C++
Com_sprintf and va helpers under ASan/UBSan. Valid text, destination truncation,
in-place formatting and va slot rotation must remain unchanged. Text that fills
a 32,000-byte formatting buffer must reach the fatal error before writing beyond it.
`--cc`, `--cxx` and `--output` select the compilers and retained binaries.

## Local Quake 3 content

```
python3 tests/run.py differential
python3 tests/run.py runtime
python3 tests/run.py runtime --sanitize --output /tmp/tests-runtime-ubsan
python3 tests/demo.py
```

These use the user's installed `~/.q3a/baseq3` paks and maps q3dm17/q3dm7.
`--data /path/to/baseq3` selects another installation. No Quake 3 paks are copied
into git or uploaded anywhere. The original unit, collision and smoke goldens
remain unchanged. Runtime needs faketime; demos also need Xvfb and Mesa's
llvmpipe/lavapipe. Tests fail when required content or tools are missing.

The completed in-process network check is `python3 tests/network.py`: both sides
use real engine loopback with `net_enabled=0`, test-only latency/loss/reordering,
and a 64-unit correction bound. Its `--max-error 0` negative control was run once
on 2026-09-14 and rejected the measured 8.875-unit correction. This requires local
Quake 3 content and is not run on the public-content hosted runtime runner. The
driver is retained unchanged by request; reproduce this completed evidence at
#3 merge `8692b422`, before the #4 path and #5 build migrations.

## Hosted OpenArena content

The runtime CI job installs `openarena-data`, `faketime`, `xvfb`, and
`mesa-vulkan-drivers`, `libsdl2-dev`, `libcurl4-openssl-dev`, and `mesa-common-dev` for client compilation. It then runs:

```
python3 tests/openarena.py
python3 tests/run.py differential --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/run.py runtime --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/demo.py --content openarena --data /tmp/aftershock-openarena-baseoa
```

OpenArena uses `+set fs_game baseoa`, oa_dm1/oa_dm7, Sarge/Beret, and separate
`tests/golden/openarena/` outputs. The collision sweep uses oa_dm1. Its smoke
runs 900 waits to exercise combat; the accepted Quake 3 scenario stays at 300.

Ubuntu/Debian data packages replace QVMs with native-module markers unsupported
by this engine. `openarena.py` symlinks installed paks into temporary storage and
adds the official OpenArena gamecode oaxB52 release, verified by SHA256
`91cb4e677d1a9f1741391ebc5fe06054ed25073addf050baf80cfc7550f98c94`.
Release/source: https://github.com/OpenArena/gamecode/releases/tag/oaxB52
(GPL-2.0-or-later, source commit 331464ca396d80e91cf9be273588f2b5f4b7afc8).
Its license is retained alongside the staged data; no downloaded paks enter git.
Use `openarena.py --data /path/to/baseoa` for data extracted without installing
packages. Local system package installation is prohibited; runner installs are
expected. A download/checksum/content failure fails CI.

## Development tooling (#7)

`AFTERSHOCK_DEVTOOLS=ON` includes the ImGui overlay; the default OFF build has no
ImGui or tool symbols. Enable it with `dev_tools 1`; Escape closes it. The overlay
provides console output/commands and cvar search, descriptions and live edit.
Texture previews, material stages, completed GPU timings/frame history and
tagged zone/hunk usage are also available. CPU scopes and client traffic/snapshot/prediction statistics are available.
The model viewer loads MD3/MDR/IQM models and optional skins, scrubs/plays frames
and rotates an existing renderer scene. Entity tools edit a local native game started with `devmap`; the World tab visualizes collision and navigation, and entity picking works at
the crosshair or by clicking outside the tools. Enabled renderer
modules use ABI 11; shipping remains ABI 10. Rebuild client/modules together.

`python3 tests/devtools.py` builds both variants, verifies symbols, then uses real
XTest input on a private Xvfb display to select/edit a cvar. It verifies 80 idle
frames without further ImGui allocations, bounded arena use and video restart.
It also holds a real game-bound key while reopening the overlay and checks that
the release command runs before input capture resumes.
It also opens each current inspector and captures its output.
`python3 tests/devtools_data.py` checks real registry copies and allocator accounting
without a GPU. The UI test requires libX11, libXtst (`libxtst6` in hosted CI), xwininfo/xprop (`x11-utils`),
Xvfb and lavapipe. The window PID must belong to the launched client. Screenshots
and logs stay under `--output`. `--binary` tests an existing development client;
`--content openarena --data /tmp/aftershock-openarena-baseoa` selects hosted assets.
The OpenArena UI check links the same pinned native OpenArena game objects as
the replay gate, matching the fixture's game protocol.

ImGui core v1.92.9b is pinned under `third_party/imgui` with its unchanged license
and source hashes. Default vendor OS, file, shell and time services are disabled.
Its allocator uses the existing zone algorithm in a fixed 16 MiB allocator-layer
arena, with no OS-heap growth during frames. Owned geometry buffers are fixed;
engine mutations occur after ImGui returns, outside vendor stack frames. Initial
or interaction-driven UI allocations are distinct from the checked idle-frame path.
Lifetime/tidy gates now cover shipping/development and static/module configurations.
The primary build matrix enables tooling in Debug and excludes it in Release.

`python3 tests/dev_world_ui.py` drives the live native game through real XTest
input: spawn at the camera, select that entity in world, enable collision and
navigation, and capture the drawn volumes/label. The World tab uses explicit
refresh, caching at most 4,096 edges around the camera (up to 1,024 brushes).
Brush/patch surfaces are exact clipped faces. Optimized AAS files that omit face
geometry show retained area bounds and reachability; unstripped files also show
ground faces. Both layers share the bounded cache with reserved capacity; omitted
edges are reported. Rendering is intentionally x-ray. Live entity bounds remain
current. Refresh cached surfaces after moving; map/video restart clears the cache.

`python3 tests/dev_entities.py` exercises native-game spawn/edit/delete and
numbered entity-string saves/reload. Unlike demo tests, it uses the owned Q3 game
with either content set. Every original map token (including unknown keys) is
compared after saving and deleting a new entity. Saved revisions are
`maps/<map>.dev.NNN.ent`; `dev_entityFile` names the latest successful save.
`dev_loadEntities 1; map_restart 0` explicitly loads that file once. Original BSPs
and paks are never written. The console `dev_entity` command exposes the same
operations; no arguments prints usage. Point markers and pickups can be spawned;
structural classname/model/team edits and creation of brush/mover geometry are
outside this basic runtime editor. Its complete map document is bounded to 8 MiB;
save fails if capture overflowed. Game callbacks are registered only by the owned
native game, so external OpenArena demo modules expose the read-only tools.

`engine/public/dev_public.h` exposes explicit `Dev_BeginScope`/`Dev_EndScope` calls
for engine-thread game code. Each frame holds at most 128 inclusive CPU scopes;
unfinished or stale tokens are discarded on frame rollover. GPU samples come from
completed frames without waiting. `Dev_PredictionError` observes the existing
calculated distance; it changes no prediction arithmetic. Datagram payload totals
exclude UDP/IP headers, and replay snapshots are explicitly distinct from traffic.
`Dev_DrawLine`, `Dev_DrawBox` and `Dev_DrawText` are engine-thread game APIs in
the same header. Colors are 0xAABBGGRR; zero duration means one frame, and positive
durations cap at 60 seconds. Storage holds 2,048 lines and 128 labels of up to
63 characters; excess primitives are counted and discarded. All telemetry/drawing
is absent from shipping and collects only while `dev_tools` is enabled.

## Runtime and fixed-demo oracle

Dedicated smoke warms the pak cache and requires two identical traces before
comparing the golden. Timeout wraps faketime, SOURCE_DATE_EPOCH is fixed, and each
map has an isolated home. Normalization removes cached-paks/working-directory
lines and replaces home/data installation prefixes. OpenArena additionally
normalizes the CPU model label and disables networking; gameplay events are kept.
Raw logs are retained in `--output`, including failed invocations.

Normal demo runs **never record**. They replay each committed `.dm_68` twice under
Vulkan lavapipe, sample TGA frames at waits 50/100/200, require changing samples
and repeated byte-identical frame hashes, then compare `frames-mesa-VERSION.json`. Unknown Mesa versions fail. Pixel hashes
are exact within each version; cross-version rasterization is not assumed identical. Screenshots
and logs remain in `--output` for review. Fixtures are small engine-generated
artifacts, not game-content archives. Recording is intentionally not reproducible:
SDL/X11/Mesa clock calls affect faketime's call count and ping-derived demo bytes.

OpenGL retired in #6. Existing OpenGL rows in the reviewed frame JSON files remain
historical evidence; normal comparison selects Vulkan rows and still requires
every Vulkan map/sample. No fixture or golden was regenerated for retirement.
`tests/check_frames.py` rejects a wrong renderer label, unequal repeats and
missing/changed Vulkan values while checking the archived-row selection. The
explicit regeneration command writes the active Vulkan measurements only.

## Sanitizer known failures

`tests/known-bugs.txt` contains regexes for UBSan diagnostics exercised by the
sanitizer unit job. Each diagnostic must match; every listed pattern must occur.
A matching error prints `known, tracked in #31`. An unknown error, an ASan crash,
a nonzero subprocess exit, a changed unit golden, or a disappeared known error
fails. The test still runs and prints its diagnostics; this is not suppression.
A #31 fix removes its entry and updates the policy self-check if applicable.
The existing bot smoke also runs under GCC UBSan (`runtime --sanitize`), comparing
the same content goldens and treating every unsuppressed diagnostic as fatal.
Both compile and link steps enable UBSan, including the hosted OpenArena C objects.
The earlier Clang/JIT metadata limitation is retired with the VM implementation;
its diagnostic history remains in the bug ledger. Unit ASan/UBSan coverage remains
required; the earlier ASan runtime experiment with faketime timed out before output.
The unit driver also initializes and frees real zlib state through its default
allocator callbacks, under the existing sanitizer modes. It reads no content and
reuses the unit allocation stubs; the ordinary unit golden stays unchanged.
The JPEG table-index check also lives in the unit driver. It compiles the actual
vendor routine and its probe as C, checking legal AC/DC table destinations and
expected index errors without file loading.
Pointer comparisons run separately from UBSan: combining both instruments Clang's
generated pointer-overflow checks and reports invalid pairs in otherwise valid
pointer increments. Both runs compare the same unit golden; neither replaces the
other. The pointer run catches extensionless names in `FS_AllowedExtension` before
the #31 fix, with `ASAN_OPTIONS=detect_invalid_pointer_pairs=2`.
PNG chunk alignment and JPEG table-index reproducers are already recorded in
issue #31 and `docs/bugs.md`; they are not exercised by the unit driver.

The inherited `tools/port/ubsan.supp` remains unchanged. Leak checks are disabled
because the inherited isolated test allocator retains hunk allocations to exit.
Omit `--known-bugs` for fatal-on-first-error UBSan behavior.

## Explicit golden regeneration

Only an intentional issue-scoped change may replace affected goldens. Initial
OpenArena and demo baselines are established in #3; accepted Quake 3 baselines
must not be regenerated for infrastructure edits.

```
python3 tests/run.py unit --regenerate
python3 tests/run.py differential --regenerate
python3 tests/run.py runtime --regenerate
python3 tests/demo.py --record-fixtures
python3 tests/demo.py --regenerate
```

`demo.py --record-fixtures` explicitly records one new demo per map and replaces
frame goldens; `--regenerate` alone replaces frame goldens using existing demos.
Initial Mesa profiles can also be established from reviewed hosted artifacts,
without ever running regeneration in CI:

```
gh run download RUN_ID -R msetaro/aftershock -n runtime-diagnostics -D /tmp/replay-evidence
python3 tests/frames.py --output /tmp/replay-evidence/aftershock-demo-tests --content openarena --regenerate
```

Review the job's fixture hashes and all screenshots first. The evidence checker
requires both repetitions, the Vulkan renderer identity, one Mesa version, and three
changing samples per map. This explicit local command hashes the downloaded TGA
files itself; it does not trust a hash manifest supplied by CI. Each initial
profile and its run/source provenance must be explained in the PR.
Add the same `--content openarena --data /tmp/aftershock-openarena-baseoa` arguments
to select that content set. Review demo logs/screenshots and explain every changed
hash or gameplay event in the PR. All golden writes are rejected when `CI` is set;
CI compares committed outputs and never regenerates them.

### Native game integration (#2)

The pinned GPL imports and provenance are in `docs/native-game-import.json`.
The engine statically links the C++20 game, cgame and UI. Each imported source
remains a separate translation unit inside its module namespace. Typed imports
and exports replace numbered calls; the module's rand/srand/qsort/atof/memmove
remain isolated from the engine and other modules.

```
python3 tests/native_abi.py
python3 tests/native_shared.py
python3 tests/run.py runtime
python3 tests/demo.py
python3 tests/demo.py --lifecycle
```

The first command compares 29 wire/module types, three offsets and the service
extension value across the C game ABI, C++ game ABI and C++ engine ABI. The second
retains the C/C++ shared-function comparison. Both run in each unit compiler job. Runtime/replay use the
static production build and require installed Quake 3 content. The native ABI
uses binary32 literals and rounds host math results to float, as the QVM compiler
did; bg_lib preserves its random sequence. Smoke normalizes module-load metadata,
build date and bot-skill printf padding when comparing accepted QVM logs; gameplay
text remains intact. Host IP/IP6 enumeration lines are removed from both sides
of log comparison because IPv6 privacy addresses change independently of the game;
connection/gameplay lines remain exact. Replay uses the unchanged demos and frame hashes.

Hosted CI uses the pinned OpenArena B52 C source and the reviewed #31 patches:

```
python3 tests/openarena_native.py --static
python3 tests/run.py runtime --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/demo.py --content openarena --data /tmp/aftershock-openarena-baseoa
```

The runtime/replay commands stage C module objects automatically and link them to
the same engine interface. GNU objcopy prefixes each module's private global
symbols; typed public exports remain visible. No game DLL is loaded. Source is
exported from revision 331464ca396d80e91cf9be273588f2b5f4b7afc8 into the test output,
retaining GPL notices. The original lists select base q3_ui sources; bg_lib
preserves the QVM random/sort behavior. These are hosted-content test objects;
the production game remains the imported Q3 C++ implementation. Runtime --sanitize
instruments both engine and OpenArena game code.

OpenArena builds as C with GCC or Clang. Its 28 structure sizes and three offsets
match the engine, including the consumed 140-byte refEntity prefix (OA appends
36 eye-vector bytes). Its optional LFX service remains unsupported, as in the
original engine; ordinary fixture settings do not call it. No game paks are
copied into the repository. The OpenArena log comparison removes its old VM-only
magic/version and jump-table compilation metadata.

### Bot movement result regression

`python3 tests/bot_move.py` calls the production `BotMoveToGoal` early return with
two poisoned output buffers and checks all result fields. It links the dedicated
server objects with a test entry point, so no content is required. `--cc`, `--cxx`
and `--output` select the same compiler matrix as the other unit checks.

`python3 tests/teamleader.py` compiles both imported GPL C team-leader paths with
Clang's bounds diagnostics as errors. It uses the imported bot-state declarations
and native ABI header. No game content or external header checkout is required.
The original #31 failing test used pinned GPL headers before #2 imported them.

Historical whole-game import checks (`python3 tests/native.py` and
`python3 tests/native.py --language c++`) run at revision `7f4d43a7`, before #10
adds the C++ animation service. They are retained as port evidence, not current
whole-game builds. At that revision, C remains the import-comparison reference;
runtime/replay use the static C++ game. Both languages compare the same 29 layouts and three
offsets with the engine; module links reject unresolved symbols. GCC and Clang
use explicit binary32 source literals without compiler-specific literal flags. For
Clang C++ only, bg_lib.cpp is
compiled separately with __NO_INLINE__ to avoid glibc's conflicting inline atof
definition; this header setting preserves the Clang C object byte-for-byte and
does not disable the optimizer's inlining. Other translation units keep their
original standard-library headers and calls.

`python3 tests/native_shared.py` compares the real shared native C and C++ functions
on the same host: 4,096 angle/vector/normalization/inverse-square-root cases (including
zero and quadrant angles), plus all nonzero byte values through Q_strlwr/Q_strupr in
the C locale. It compares raw result words, requires no assets or golden writes,
and accepts --cc/--cxx/--output. Both unit compiler jobs run it. The C++ native build
pins the C library feature set to the C99 reference, avoiding C23 scanf/strtol
redirection from the C++ compiler's default _GNU_SOURCE.

`python3 tests/openarena_strings.py` verifies the OpenArena native CI dependency's
case-sensitive name comparison (missing names and single argument evaluation) and
in-place extension stripping (model suffix paths, bounded truncation, empty input
and capacity one). It fetches pinned public source from OpenArena/gamecode
revision 331464ca396d80e91cf9be273588f2b5f4b7afc8 when the source cache is absent,
then applies the name-comparison and extension patches in tests/patches to its output
directory. The extension probe links the actual q_shared.c helper.
No game content is fetched by this check. --cc, --source and --output select the
compiler/cache/output. Both unit compiler jobs run name comparisons under UBSan and
extension stripping under ASan. The source patches are for #2's native OpenArena
configuration; the existing QVM fixtures remain unchanged. Original GPL notices remain in the fetched headers.

`python3 tests/ui_weapon.py` checks the GPL UI's negative pending-weapon sentinel
under UBSan, its signed setter signature, and its unchanged state size/offsets.
It uses the actual UI header and accepts --cxx/--output; both unit compiler jobs
run it. No content is required. Four prerequisite GPL files are imported for this
#31 fix; their pinned provenance is recorded in docs/bugs.md. The full
native integration remains #2.

`python3 tests/team_voters.py` calls the actual GPL CalculateRanks under UBSan.
It verifies zero-client reset, red/blue human counts, bot exclusion and preservation
of adjacent spawn state. --cc/--output select the compiler and output; both unit
compiler jobs run it. Only unrelated end-level notifications use test stubs.

Native Q3 helper builds use `-Wall -Wextra -Werror -Wunused-const-variable` in
both C and C++. All frozen helper warning classes were removed through separate
#8 PRs; the final signedness class removes the empty freeze configuration.
C++ literal/register errors remain enabled. OpenArena remains an external C test
dependency. Signedness edits preserve the existing integer conversions; enum
comparisons shared with C use explicit integer casts only for equality checks.
Production object comparisons and native C/C++ helper results are recorded in
`docs/modernization-progress.md`. Accepted fixtures and goldens are unchanged.

At the same historical revision `7f4d43a7`,
`python3 -B tests/native_gates.py` reproduces G2 layouts, G3 symbols and advisory G4
assembly for all 103 native Q3 objects using the existing `tools/port/gates.py`
normalizers. Layout/symbol differences fail; assembly differences remain visible
for review. --tidy also runs the three focused G7 checks, failing on tool/compile
errors and retaining every diagnostic for disposition. --jobs/--output control
concurrency/artifact location. The output records source hashes, compiler versions,
exact commands, logs and diffs. GCC/binutils and pahole are required (plus
clang-tidy for --tidy). Accepted pre-#10 CI evidence is recorded in the progress
file. Current CI checks the unchanged ABI and shared math, production builds,
fixed classic replays, plus the new animation contracts.

The G3 objects use the port oracle's optimizer/header isolation flags plus
-U__OPTIMIZE__: glibc otherwise forces single-character strstr calls into strchr in
C++ headers even with -fno-builtin. This flag is limited to symbol artifacts;
production flags and assembly retain those library transformations. The recorded
G7 report contains 1,365 narrowing, 55 signed-char and nine implicit string-result
comparisons; it is a review report, not a claim of zero findings.

`python3 tests/team_flags.py` checks actual Team_InitGame/Team_SetFlagStatus under
UBSan in base-game and MISSIONPACK builds. Initial flag configstrings are complete;
pickups, drops, repeated updates and reinitialization preserve valid flag states.
--cc/--output select compiler/output. Both unit compiler jobs run it, without assets.

The import manifest's SHA256 values always identify the pinned original GPL files.
Its per-file transformation references link the native ABI adaptations, catalog
passes and separate #31 fixes to their commits; retained engine ABI headers are
identified explicitly. The audit before renaming verified all 130 original hashes:
30 files remain verbatim, 96 carry recorded changes, and four retain engine headers.

`python3 tests/bot_command.py` exercises the real BotInputToUserCommand with
horizontal/vertical bases, byte endpoints, fractions and larger signed inputs.
UBSan float-cast-overflow checks the conversion; explicit expected bytes check
legacy truncation/wrapping. --cc/--output select compiler and output; it uses the
imported local headers and native ABI configuration.
Both unit compiler jobs run it. No assets are needed.

The 93 imported implementation files use `.cpp` names. C comparison builds select
`-x c` explicitly; the pinned OpenArena dependency remains C. The rename preserves
every source byte and the provenance manifest retains original upstream paths.

`python3 tests/native_info.py` checks both real GPL info-removal helpers under
ASan. Seven valid-string cases per helper cover removal at the beginning, middle
and end, single-pair removal and unchanged inputs. --variant small/big isolates
one helper; --cc/--output select compiler and output. Both unit compiler jobs run
it. No game assets or generated goldens are needed.

`python3 tests/native_lifecycle.py` compares static restart/map-change logs with
reviewed ordinary-DLL references captured before static integration. The added
native-lifecycle.log and native-lifecycle-debug.log baselines are those existing
reference outputs (dd1fe5c3/e87382ec), not regenerated gameplay. Both repetitions
must match; --debug-movement includes the per-module movement counter. Installed
Quake 3 content and normal runtime tools are required. Diagnostics stay in --output.
There is no regeneration mode for this reference comparison.

`python3 tests/demo.py --lifecycle` replays a fixed fixture, restarts video, then
samples another replay in the same process. On Quake 3 content both repetitions
must match the existing accepted frame goldens, just like ordinary replay. No
additional frame golden was needed. This tests the production Q3 module reset;
hosted OA parity runs each fixture in a fresh process.

`python3 tests/openarena_alloc.py` checks the real pinned OpenArena allocator with
ASan/UBSan: native structure/pointer alignment, allocation/free/reuse, preservation
of live payloads, defragmentation and reuse after completely filling the pool.
--cc/--source/--output select compiler, source
cache and diagnostics. Both unit compiler jobs run it; no game content is needed.

Runtime and replay also inspect their built binaries: all required static module
init exports must exist and no VM_* implementation symbol may remain. The old
VM_Call argument-slot probe was retired with VM_Call; its #31 history remains in
the bug ledger. Retained C/C++ import oracles are compiler evidence, not a runtime
module-loading path. Native game objects are linked into the executables.

The boundary check scans engine/game includes and OS calls, with built-in negative
controls. See `docs/subsystems.md` for the public-header and OS ownership rules.

`python3 tests/affinity.py` checks valid CPU-affinity expressions against the actual
platform helper and public apply path under ASan/UBSan. It covers decimal/hex constants,
64-bit values, core aliases, mixed `+`/`-` expressions and invalid hex prefixes. The OS setter is intercepted;
the test does not alter process affinity. Pass `--cxx 'clang++ -stdlib=libc++'` for
the second CI compiler. No content or golden files are required.

`python3 tests/chat_offset.py` checks bot chat's unmatched-variable marker with
both signed-char and unsigned-char compiler defaults under ASan/UBSan. It uses
actual template matching, variable extraction and message expansion, tests valid
offsets alongside the absent variable, and checks mirrored engine/game layouts.
`--cxx` selects GCC or Clang. It needs no content or external services.

`python3 tests/ui_skill.py` exercises the actual GPL UI skill callback and score
storage under ASan/UBSan including float-cast-overflow. Large finite cvar values,
integer conversion boundaries and invalid low values are handled without an
unbounded float-to-int conversion. Valid values (including fractional skills)
retain truncation and select the same score cvar. The shared UI reader preserves
out-of-range sentinels so menus retain their own reset/clamp policies. The engine
already filters non-finite cvar values. `--cxx` selects GCC or Clang; no assets or
expected-failure entry are needed.

`python3 tests/team_message.py` checks the real native PrintMsg function in base
and MISSIONPACK configurations under ASan/UBSan. Small ordinary text verifies
quote replacement and broadcast routing; injected formatter results verify the
fitting, full-capacity and error policies without making an oversized write.
Use `--cxx 'clang++ -stdlib=libc++'` for the Clang/libc++ CI configuration.

`python3 tests/native_diagnostics.py` checks twelve real native game, cgame, UI
and bot diagnostic paths under ASan/UBSan. The formatter seam observes the actual
destination capacity while small ordinary text verifies print/error routing and
the log prefix. No oversized write is used. The formatters use standard bounded
output, truncating diagnostic text to their existing buffer capacities. Use
`--cxx 'clang++ -stdlib=libc++'` for the second CI compiler configuration.

Cooked asset runtime check (#9): `python3 tests/cook_runtime.py` builds the enabled
client, cooks the owned Blender fixture, opens it through real ImGui input and
edits a copied source texture. The fixed-camera before/after screenshots must
show the edit within one second. `--binary` reuses a development client;
`--content openarena --data /tmp/aftershock-openarena-baseoa` selects hosted content.
Use the tools/cook Python requirements in a venv. The watcher and client use only
private temporary source/output trees; installed paks are symlinked locally.

## Animation graphs (#10)

```
python3 tests/animation.py
python3 tests/animation.py --cc clang --cxx 'clang++ -stdlib=libc++' --output /tmp/aftershock-animation-clang
python3 tests/animation_runtime.py
python3 tests/animation_editor.py
```

The native check cooks the owned Blender exports and hand-authored graphs, checks
quantized sampling, flat blend trees, bone masks/additive layers, transitions,
ordered loop notifies, rigid root motion, IK, authored hit boxes, copied renderer
poses and real snapshot delta-codec round trips under UBSan. Graphs bind the exact
cooked IQM hash. Limits are 128 joints, 64 states/nodes/events, 16 parameters/masks,
32 hit boxes, and 128 copied poses per renderer frame. Sampling and game animation
use strict floating-point flags. File storage is allocated only on graph loading;
poses and frame copies use bounded POD storage.

The live check exercises the original rifle/body in the owned native game, including
when using OpenArena art. It checks events, render submissions, retained body turns,
centered settled ADS optics and matching received client/server hit-box digests.
Use `--content openarena --data /tmp/aftershock-openarena-baseoa` for hosted content;
`--binary PATH` reuses a client, and `--modules` builds optional renderer modules.
Classic runtime/demo commands continue using their original content-specific game
implementations and accepted fixtures. No existing golden is regenerated here.

Enable both `g_animationBody animations/anim_body.asanim` and
`g_animationRifle animations/anim_rifle.asanim` before starting a map. Empty defaults
preserve classic behavior. The server advances graph state in 20 ms steps and
publishes auxiliary entities through the existing snapshot codec. The client uses
that exact state for hit boxes and samples cosmetic presentation separately.
Gameplay notifies are delivered to game code; the existing legacy weapon rules
remain the weapon-system baseline for #11. `cmd anim NAME 0|1` supplies demonstration
ADS/fire/reload/sprint/jump, stance and aim/lean inputs. Movement, view yaw/pitch and
authoritative ground contacts supply the body inputs automatically. `cg_animationFov`
and `cg_animationSway` tune first-person cosmetics; `g_animationTrace` and
`cg_animationTrace` enable diagnostic state/event/box output, and `anim_status`
reports render counts and settled optic alignment. Game graph revisions stay fixed
until map restart; the server and client must load identical graph bytes.

For ImGui authoring, stage the **owned source project** under the active game's
`fs_homepath/animation_source/` directory, including glTF buffers/materials, and run:

```
python3 tools/cook /path/to/home/baseq3/animation_source/rigs.json --output /path/to/home/baseq3 --watch
```

Use `baseoa` with OpenArena. In an enabled development build, set `dev_tools 1`,
then `dev_animation load animations/anim_rifle.asanim` and
`dev_animation source animation_source/rifle.animation.json`. The Graph tab offers
parameter sliders and fixed-step preview, compiled state/event/transition/node/mask
tables, and a JSON source editor. It accepts up to 65535 source bytes, keeps numbered
`.bak.NNN` copies before saves, refuses conflicting external edits and retains unsaved
text on failure. The external cooker validates JSON and reports errors in its own
output; load the cooked graph after a successful cook. Enable `dev_reloadAssets` to
reload changed models/materials too. Editor previews never replace live game assets.
`tests/animation_editor.py` edits a graph through real X input, verifies its backup,
observes the watcher revision and previews the changed initial state.

`python3 tests/animation_runtime.py --server-fps 100` also checks that faster server
frames never publish multiple transforms under one 20 ms animation tick.
`python3 tests/animation_demo.py` replays the separate committed #10 fixture twice,
checks each received hit-box digest against its saved authoritative server trace,
requires all rifle/body states, and compares three sampled frame hashes across
repetitions. It uses owned game code with either content set. `--modules` exercises
the optional renderer module. Normal/CI invocations never record; existing classic
frame goldens remain the visual compatibility oracle. New animation frames are a
repeatability check and do not need a new Mesa-version pixel baseline.

The only explicit animation fixture replacement command is:

```
python3 tests/animation_demo.py --record-fixture
python3 tests/animation_demo.py --record-fixture --content openarena --data /tmp/aftershock-openarena-baseoa
```

This replaces only `tests/golden/animation/<content>/rifle-body.dm_68` and its JSON
manifest. Each manifest records the source revision, exact input commands, graph/
model hashes, demo hash and server box digests. Record once, review the screenshots
and trace, and explain any replacement in the issue/PR. Recording byte equality is
not required; fixed replay is. CI rejects the recording flag. The initial fixtures
were recorded from 3fbc0a62 on q3dm17 and oa_dm1; each replay checks 253 received
poses against 265 recorded authoritative poses. The source project and demo are
owned GPL artifacts; map art remains in the user's installed content packages.
