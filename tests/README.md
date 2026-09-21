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
python3 tests/replication.py
python3 tests/protocol.py
python3 tests/rewind.py
python3 tests/replication_policy.py
python3 tests/identity.py
python3 tests/rhi.py
python3 tests/render_graph.py
python3 tests/shadow_views.py
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

## Netcode contracts (#12)

The five `replication`, `protocol`, `rewind`, `replication_policy` and `identity`
commands above accept `--cxx` and `--output`; each uses production code and UBSan
with GCC or Clang/libc++. Replication checks all annotated members and the exact
pre-change 107002-byte delta digest. `python3 tools/replication.py` explicitly
updates the generated table after a reviewed state-definition change; CI only
checks freshness. The handshake requires both `AFTERSHOCK_NET_VERSION` (default
2) and the generated schema digest. Incompatible or unversioned connections are
refused before joining; fixed legacy demo decoding is unchanged.

`g_rewind 1` opts into per-server-frame actor hit boxes. `g_maxRewind` defaults to
200 ms, capped at 1000 ms; 64 history frames can impose a smaller effective window
at high server rates. Storage holds 1024 entities and 2048 boxes per frame in less
than 5 MiB of level-hunk memory. Spawn/respawn/teleport generations never
interpolate across lifetimes. Animated actors use authoritative bone boxes;
other eligible actors use bounds. World/brush traces remain current. Four sampled
reports per second per firing client feed the development network overlay beside
prediction last/peak/mean; this is sampled telemetry, not a total shot count.

`GameImport_SetEntityReplication(number, priority, radius)` narrows existing PVS
interest (radius 0 means unlimited) and supplies priority 0..3. `sv_snapshotBudget`
defaults to 0, preserving the original selection/encoding path. Positive values
up to 16384 cap the target snapshot size, further bounded by client rate times
snapshot interval. Mandatory reliable/playerstate/removal traffic may exceed this
budget; optional changes then retain acknowledged visible states or defer newly
visible entities. Existing transmit rate limiting still charges actual bytes.
Capacity remains 256 entities; priority selects this set, then priority plus age
schedules updates without allocating per frame. `status` reports deferrals and
mandatory overruns. Radius bounds are 0..65536 world units; owned entity reuse
resets policy. This is a radius filter on PVS, not an additional spatial index.

For real loopback tests, build client/server with `AFTERSHOCK_DEVTOOLS=ON` and a
second server with `-DAFTERSHOCK_EXTRA_FLAGS=-DAFTERSHOCK_NET_VERSION=3`:

```
python3 tests/protocol_runtime.py --client /path/quake3e.x64 --server /path/quake3e.ded.x64 --other-server /path/version3/quake3e.ded.x64
python3 tests/netcode_runtime.py --client /path/quake3e.x64 --server /path/quake3e.ded.x64 --snapshot-budget 48
```

These use installed Quake 3 content by default. Hosted CI passes `--content
openarena --data /tmp/aftershock-openarena-baseoa`; both run the owned native game.
The latter also needs the cooker prerequisites, Xvfb and Mesa lavapipe. Its
private loopback relay models 100 ms RTT, +/-15 ms combined jitter and 5% loss.
The independent interpolated-box oracle requires >=99% agreement over >=100
shots, >=5 hits, >=5 decisions differing without rewind, and prediction error
<=32 units (including spawn). The 48-byte stress run must defer updates while
still delivering target state. Portable history coverage additionally models
100 ms one-way latency with 5% loss. No demo or accepted golden is regenerated;
classic and #10 fixed animation replay gates remain in the same workflow.

`engine/platform/services_public.h` is the main-thread provider seam for Steam
identity and browser/matchmaking. No provider means anonymous, not authenticated.
A successful Begin only makes a connection pending: only a matching verified
callback exposes its ID via `GameImport_GetPlayerIdentity`. Revocation drops the
client, pending checks expire after 30 seconds, disconnect ends the provider
session once, and connection tokens prevent stale callbacks authenticating a
reused slot. Ticket buffers are borrowed only during Begin (2048-byte maximum).
The provider must copy them and never log them, reenter the engine or call
`Com_Error`. Installation is immutable once used. SDK callbacks are queued by the
platform backend for bounded main-thread polling.

`engine/server/identity_public.h` binds ticket submission to a connection token
obtained from the server-owned connection context, never a peer-supplied token.
#23 supplies the Steam SDK and ticket transport; #12 does not claim live Steam
verification or add a ticket command before that backend exists. The current
server permits anonymous players. The fake-provider test exercises the actual
server lifecycle, including timeout, revocation, duplicate account rejection and
slot reuse. Native UI imports expose one bounded browser or matchmaking search;
results carry request generations, terminal results end the request, and UI
shutdown cancels it. Discovery never executes commands or connects automatically.
The existing master-server browser remains available.

`python3 tests/cook.py` requires the pinned Pillow version from
`tools/cook/requirements.txt`, CMake/Ninja and a host C++ compiler. It cooks owned
static, mirrored and skinned glTF/GLB sources; checks named clips, coordinates,
BC7/BC5/BC4 KTX2 mip chains and embedded/manifest hashes; verifies no-op and
selective recooking; and feeds the committed Blender character into production
IQM pose code. `--cxx` selects GCC or Clang/libc++. Its source fixture/provenance
is in `tests/assets/cook-character`; CI never reauthors it. No game paks are used. It also
checks authored WAV/OGG sources through the production PCM codec, twelve bounded
model/material replacements and 10,000 allocation-free idle publication polls.

`python3 tests/materials.py` uses the same cooker prerequisites. It independently
decodes the owned PBR texture channels/mips, checks glTF and standalone data,
incremental dependencies, native factors and instance resolution, and retains the
v1 compatibility path. Set `CC`/`CXX` for Clang/libc++. `tests/animation.py` also
checks that static/skeletal material instances are copied into the frame, stay
independent and reset when frame storage is reused.

`python3 tests/materials_runtime.py --binary CLIENT` loads an owned glTF sphere
under Xvfb/lavapipe. It measures metallic, roughness, normal, emissive, mask and
blend source edits in the sphere interior and requires an exact restored image.
`--ui --output /tmp/material-ui` uses real ImGui input to edit a shared factor,
override only the preview instance and restore both independently. Both commands
accept the existing `--content`/`--data` options; CI uses OpenArena. Their new
captures are diagnostics, never replacements for the classic replay goldens.

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

`python3 tests/shaders.py --compiler /path/to/glslang-16.6.0` recompiles all 76
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

`python3 tests/devtools.py` builds both variants, verifies shipping excludes the
UI and agent symbols, then uses the local JSON channel to select panels and edit
cvars. It checks 80 idle frames without further ImGui allocations, bounded arena
use, renderer restart, animation and a normal game-bound key held while reopening
the overlay. Named key requests enter the normal event queue; no X11 pointer
coordinates or console regex are used. Each inspector produces a PNG capture.
`python3 tests/devtools_data.py` checks registry copies and allocator accounting
without a GPU. Runtime requires Xvfb/lavapipe and installed content. Pass
`--output DIR` to retain logs/captures, or `--binary PATH` for an existing developer
client. Both content sets now use the owned native game; this test no longer
replays a legacy demo or requires imported OpenArena game objects.

ImGui core v1.92.9b is pinned under `third_party/imgui` with its unchanged license
and source hashes. Default vendor OS, file, shell and time services are disabled.
Its allocator uses the existing zone algorithm in a fixed 16 MiB allocator-layer
arena, with no OS-heap growth during frames. Owned geometry buffers are fixed;
engine mutations occur after ImGui returns, outside vendor stack frames. Initial
or interaction-driven UI allocations are distinct from the checked idle-frame path.
Lifetime/tidy gates now cover shipping/development and static/module configurations.
The primary build matrix enables tooling in Debug and excludes it in Release.

`python3 tests/dev_world_ui.py` drives the live native game through the local
JSON channel and shared panel controls: spawn at the camera, select that entity in world, enable collision and
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
`tests/animation_editor.py` edits a graph through shared JSON/panel commands,
checks undo and its backup, observes the watcher revision and previews the changed
initial state. These migrated tests use fresh temporary output by default; pass
`--output DIR` to retain logs/captures. Game assets remain installed read-only.

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


## Data-driven weapon range (#11, acceptance in progress)

`python3 tests/weapons.py` cooks two rifles from data and runs the seeded 1000-shot
reference, fixed-tick lifecycle, real snapshot codec, penetration, projectile math
and graph-notify checks under UBSan. Select the other supported compiler with
`--cc clang --cxx 'clang++ -stdlib=libc++' --output /tmp/weapons-clang`.

`python3 tests/weapons_runtime.py --binary PATH` exercises native server/client
weapon and animation prediction, data-only selection, attachments, grenade
prediction, rendered ADS and exactly-once notify audio. It saves an ADS capture
for review and uses SDL dummy audio to verify dispatch and decoded resident
samples. Add `--lifecycle` for spectator/rejoin record reuse, connection-generation
audio, and the 64-projectile capacity/ammo/prediction check.
`python3 tests/weapon_range.py --binary PATH` renders the Range panel and calls
its shared controls through JSON. It checks authoritative weapon selection, ammo,
reload, ADS and animation state, then saves a PNG panel capture. Both require a development
client, Xvfb/lavapipe and installed content; hosted content uses
`--content openarena --data /tmp/aftershock-openarena-baseoa`. Local Q3 uses
`~/.q3a/baseq3`. Neither command copies game paks into the repository.

In a development client, `dev_weapon_range` opens the range panel on a local
`devmap` with `g_rewind 1` and a space-separated `g_weapons` list of cooked
`.asweapon` paths. The panel inspects a cooked definition, selects a loaded slot,
spawns the existing rewind target, pulses fire/reload/melee/offhand, controls ADS
and attachments, and restarts with an inspected weapon after offline recooking.
The active simulation keeps its map-start definition hashes. Cook the owned art
with `python3 tools/cook tests/assets/range.json --output OUTPUT` and the rifle
with `python3 tools/cook tests/assets/weapons/assets.json --output OUTPUT`.
The #11 scene variant removes the old solid placeholder sight while referencing
the unchanged #10 geometry/animation buffer. Accepted #3/#10 fixtures remain
unchanged. The data HUD shows magazine + chamber / reserve for each active hand.

`python3 tests/netcode_runtime.py --weapons --client PATH --server PATH --snapshot-budget 256`
uses the existing private loopback delay driver: 100 ms RTT, jitter and 5% loss
once the initial gamestate has loaded. Real input starts the scenario only after
client initialization. It checks rewind hits against independently interpolated
authoritative boxes, snapshot/full prediction agreement, budget deferral and
predicted grenade reconciliation. The classic scenario remains the default. `--client-fps 20` deliberately caps
rendering so the frame-counted script outlasts the old 45-second deadline; CI uses
this control. The weapon scenario has a 120-second wall budget inside a 240-second
overall limit. This changes neither the fixed simulation tick nor the hit/state
assertions. The slow control must actually exceed 45 seconds. Its view-age bound
accounts for the extra render/input interval and remains within the 200 ms rewind
window; the normal 100-FPS bound stays at 180 ms. `python3 tests/netcode_cleanup.py` verifies that a private child which
ignores graceful termination is forcibly killed and reaped; the harness uses the
same bounded cleanup on timeout.

`python3 tests/weapons_demo.py --binary PATH` replays the separate #11 fixture
twice, compares all 56 bytes of each received weapon state with its recorded
server SHA256, requires rifle/reload/ADS/melee/projectile presentation, and compares
three frame hashes. Omit `--binary` to build; add `--modules` to build the renderer
module configuration. Both installed content sets use the same owned weapon art.
The fixture manifest pins all cooked weapon/graph/model/sound/material bytes.
Recording equality is not a gate. CI never records. Explicit replacement commands:

```
python3 tests/weapons_demo.py --record-fixture --binary PATH
python3 tests/weapons_demo.py --record-fixture --binary PATH --content openarena --data /tmp/aftershock-openarena-baseoa
```

These affect only `tests/golden/weapons/<content>/range.dm_68` and its manifest.
Record once, review frames and authoritative traces, and explain any replacement
in the issue/PR. Full #11 hosted acceptance remains pending.


## Directional baked lighting (#14)

`python3 tests/lighting.py --compile` uses the pinned level toolchain to bake two
independent opted-in levels. It checks paired intensity/model-space direction
pages, surface references, retained light-grid probes and unchanged disabled
MAP/BSP/AAS fixture bytes. Without `--compile` it checks only the language/MAP
contract and needs no map compiler.

`python3 tests/lighting_runtime.py --binary CLIENT` needs the same cooker, Xvfb,
Pillow and Mesa prerequisites as the material tests. The client must enable
`AFTERSHOCK_DEVTOOLS` for the fixed spectator camera. It cooks owned wall/floor
PBR materials on the opted-in level, checks separate and merged lightmap pages,
compares direction-mapped pixels and requires an exact quality-setting round trip.
Use `--content openarena --data /tmp/aftershock-openarena-baseoa` in hosted CI.
These captures are diagnostics; neither command records accepted references.

The new material path uses an intensity/direction atlas at the existing spare
texture binding; it requires five descriptor sets, otherwise it reports a
light-grid fallback. `r_directionalLightmaps 0/1` controls normal mapping live.
Intensity keeps the legacy map color conversion; direction bytes never receive
color/gamma processing. New baked shader programs leave all 76 earlier binaries
unchanged. A dominant direction approximates multi-light irradiance, with grazing
amplification bounded at 4x; it is not full spherical-harmonic irradiance.
`python3 tests/shadow_views.py` compiles the portable shadow cameras with UBSan;
`--cxx 'clang++ -stdlib=libc++'` checks the other compiler family. Analytical checks
cover six point faces, spot cone, reversed depth, culling planes and all four sun
cascades with rotated cameras and linear/logarithmic split mixtures. Sun extents
use rotation-independent receiver spheres and snap to shadow texels. Caster
extrusion is bounded by the configured shadow distance. This validates view math;
it does not claim native shadow rendering or its performance budget. The same
driver checks copied native `sceneLight_t` point/spot submission, cone validation,
per-frame capacity (16 lights) and per-scene atlas admission (16 tiles; point lights
use six, spots one). `trap_R_AddSceneLight` returns false when those bounds are
exhausted. The new POD service uses world coordinates and linear RGB/intensity,
without changing legacy dynamic-light calls or any network state. Renderer module
API versions are 15 shipping / 19 development for the new function pointer.

`python3 tests/lighting_runtime.py --shadows --binary CLIENT` adds point, spot and
sun comparisons on the owned level: light must affect the image, depth comparison
must attenuate it, and disabling comparison must restore it exactly. It compares
opaque/cutout/empty casters and two owned skinned poses on common receiver pixels.
`--lifecycle` also requires an exact image after renderer restart; the same test
accepts a development client built with `USE_RENDERER_DLOPEN=ON`. The same
`--content`/`--data` arguments apply. `tests/shadow_views.py` also verifies every
atlas tile's raster/scissor against an atlas larger than the window.

`r_shadowQuality` is latched: 0 (default) preserves classic rendering; 1/2/3 allocate
1024/2048/4096 square local and sun atlases. Local tiles use a 4x4 grid, sun a 2x2
grid. `r_shadowSun` (0..4, default 0) controls added sun intensity;
`r_shadowDistance` (default 2048), `r_shadowSplitWeight` (default .75),
`r_shadowBias` (default .001) and `r_shadowOcclusion` (default 1) control reach,
splits and depth comparison. Five descriptor sets are required. Casters reuse
world and native animated geometry, including alpha masks. Receiver passes use
normal/roughness/metallic channels for PBR and diffuse normals for legacy surfaces,
then existing fog. No per-frame heap allocation is added. The bounded forward
passes support the 16 local tile limit; #161 owns HDR composition and tone mapping.
Development builds provide `dev_light point x y z radius r g b intensity`,
`dev_light spot x y z radius r g b intensity dx dy dz inner outer` and
`dev_light off` on local cheat-enabled servers. Cone angles are half angles in degrees.

`python3 tests/probes.py` checks six engine camera directions, deterministic GGX
filtering in linear radiance and the packed atlas format. `python3
tests/probes_runtime.py --binary CLIENT` bakes the owned level through the native
client, applies its reflection to a dynamic metallic model, compares smooth/rough
responses and requires exact toggle/restart round trips. The content arguments
match the other runtime commands. Neither command replaces accepted references.

`tools/level/probes.py` bakes 1..32 positioned spherical reflection probes; see
`tools/level/README.md`. `r_reflectionProbes 1` enables their specular contribution
on opaque/masked dynamic PBR objects (default 0). The two strongest influence
weights blend; existing q3map2 light-grid probes still supply diffuse irradiance.
Atlas allocation/pipeline creation happens at map load, with no frame allocations.
The five roughness levels contain linear 8-bit radiance from LDR captures. This
approximation has no box parallax correction; HDR capture/composition is #161.
`python3 tests/ssao_runtime.py --binary CLIENT` measures contact attenuation on
an owned level at half/full resolution, then with 4x MSAA and bloom. Disabling
strength restores the baseline exactly; renderer restart restores the enabled
image. Both static and optional renderer modules run in hosted CI. No accepted
classic fixture changes. `r_ssao` is latched: 0 off (default), 1 half resolution,
2 full resolution; requires `r_fbo 1`. `r_ssaoRadius` is a live 1..128 world-unit
radius (default 32), and `r_ssaoStrength` is live 0..4 (default 1, 0 bypasses).

The fixed 16-sample kernel uses retained scene depth and a depth-aware filter in
separate graph passes before bloom/HUD. MSAA reads the nearest reversed-depth
sample; sky and near depth-hacked geometry are excluded. Two single-channel R8
targets use two bytes per AO pixel; the sampled main depth/MSAA attachments must
also remain resident across these passes. GPU profiler names are `ssao`,
`ssao blur`, and `ssao apply` (the last also includes subsequent HUD draws).
The current forward composition attenuates the composed scene; #161's HDR
transition owns separation of ambient and direct terms. This is a screen-space
approximation: offscreen occluders are unavailable and no temporal history is
kept. Reference-GPU measurement: `python3 tests/lighting_gpu.py --binary CLIENT --icd
/path/to/hardware-icd.json` uses installed Quake 3 content, q3dm17, 1280x720,
200 warm frames and 100 real-clock GPU samples per phase. It retains local
screenshots/logs/JSON; never upload its Quake 3 screenshots or copy paks into the
repository. RTX 3080 Ti / 595.91.07 combined point/sun shadows, half-resolution SSAO
and bloom measured 1.933 ms median / 1.969 ms p95 in recorded GPU scopes against a
16.67 ms budget. Details and per-pass limits are in modernization-progress.md;
presentation waits are excluded and this is not a console-hardware claim.

## Declarative level authoring (#26)

`python3 tests/level.py` checks the versioned JSON language, repeated MAP bytes,
room/corridor/door clearances, connected spawn navigation, blocked passages,
entity and prop bounds, asset references, sightline and cover rules, and rotated
layouts. See `tools/level/README.md` for the exact conservative design checks.
`python3 tests/level.py --compile` fetches the SHA256-pinned Linux x86_64 q3map2/
MBSPC toolchain into the user cache, compiles twice in fresh directories, and
compares raw MAP/BSP/AAS bytes with `tests/golden/levels`. First extraction requires
`pip install -r tools/level/requirements.txt` inside a venv and system libarchive
(hosted CI installs it). MAP-only validation uses Python's standard library.

`python3 tests/level_runtime.py --client CLIENT --server SERVER` compiles the owned
sample and runs two minutes of fixed-step native bot play, requiring both bots to
reach the middle-room shotgun, an east-room pickup and repeated combat. It repeats
with only the door/stair lane and only the ramp lane, so one good route cannot
hide a broken second route. The client captures fixed views of all three rooms
under Xvfb/lavapipe. Hosted CI uses `--content openarena --data
/tmp/aftershock-openarena-baseoa`; local runs default to installed Quake 3 paks.
Content paks are temporary symlinks and never enter the compiler's workspace.
Only owned art and compiler outputs are committed.

Initial #26 fixtures were authored once with
`python3 tests/level.py --compile --record-fixtures`, after repeated-build, bot-path
and rendered geometry review. That flag is refused in CI. Any future fixture
replacement requires the same explicit command and an explained behavior change;
accepted engine/demo goldens are independent and unchanged.


## Headless level reports (#27)

`python3 tests/level_validate.py --client CLIENT --server SERVER` runs the complete
`tools/level validate` command without a display. Both binaries require development
tools. It compares PNG bytes and draw/triangle metrics across two runs, checks the
JSON/text report and 240 bot position samples, and requires clear failures for an
unreachable spawn, outside camera and deliberately open ceiling. Small controls
also verify stationary/moving/dead bot inactivity classification. Add the same
OpenArena content flags as `tests/level_runtime.py` for hosted content. Artifacts
are JSON/text, engine/compiler logs and PNGs under `/tmp/aftershock-level-validation`.
No accepted demo, frame or level fixture is regenerated. See `tools/level/README.md`
for report semantics, conservative design checks and the inactivity heuristic.

#28 match-server checks live in `tests/match_content.py` (reproducible owned package),
`tests/match_exit.py` (opt-in native lifetime; Q3 default or explicit OA),
`tests/match_runtime.py` (separate wrapper/shipper/ingest with optional actual OA
client), and `tests/match_kind.py` (private Agones Fleet allocation and real-client
acceptance through acknowledged result and replacement). The Go service's
`go test -race ./...` checks validation, bounded checkpoints, durable retry/restart,
ingest outage and process exit before log creation. See tools/match/README.md for
image/Compose/spec details and the exact kind command. The kind test downloads
verified tools only into user cache, creates/deletes only its random private cluster,
and measures all three match containers' CPU/working set. Its density conversion
is a connected-player resource baseline, not a saturation/capacity guarantee.
OpenArena test paks are copied into that private node only; the image contains only
owned content. Generated secrets/specs/kubeconfig are excluded from CI artifacts.

Native pure-server regression (#31): `python3 tests/native_pure.py` exercises the
real filesystem pure list with statically linked modules and retained content-pak
checksum accounting. `python3 tests/native_pure_runtime.py --client CLIENT --server
SERVER` requires a real native client to enter play and chat on a password-protected
`sv_pure=1` server. It defaults to local Quake 3; hosted CI passes `--content openarena
--data /tmp/aftershock-openarena-baseoa`. Native cgame/UI slots are explicitly zero;
package checksum membership, duplicate and aggregate verification remain active.
This validates content agreement, not executable attestation. Existing non-pure
replay fixtures are unchanged; no fixture regeneration is required.

## Main build publication

`python3 tests/publish_build.py` replaces gh with an offline temporary stub. It
checks initial publication, asset retry, an existing tag with a different target,
API/upload/create failures and rejection of other repositories/non-SHA targets.
It never accesses a remote service or creates a real release.

Successful main builds publish `build-<full commit SHA>` prereleases inside
msetaro/aftershock. Only publication jobs have contents-write permission. Retry
can replace that build's archive assets after verifying the immutable tag target;
it never moves a tag or changes known-good rollback points. Ordinary release
uploads retain their explicit release-event workflow.
