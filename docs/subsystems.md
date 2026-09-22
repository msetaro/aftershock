# Subsystem ownership

Cross-subsystem includes use `*_public.h`. The two `q_shared.h` headers are the
existing public foundational schemas for engine and imported game code. Engine
code never includes game implementation. Game code sees only public engine
headers. `tests/check_boundaries.py` checks every engine/game C, C++, header and
include fragment, including alternate platform branches; literal `#if 0` examples
and comments are excluded. Third-party dependencies retain their own headers.

`engine/qcommon` owns commands, cvars, messages, snapshots, collision, arenas and
the filesystem. Wire/file representations and simulation expressions stay stable.
`files.cpp` owns file operations, including raw OS-path stdio used by existing
server filters and bot development logs. These raw adapters preserve stdio return
values and filename encoding; game qpaths continue through the handle API.

`engine/entities` owns bounded cooked prefab definitions, component fields and
replication metadata. It performs no IO or allocation; game owns spawn/callback
policy and the developer inspector consumes the same field metadata.

`engine/animation` owns immutable cooked graph views, fixed-step state/event
evaluation, compressed pose sampling, blend layers, root motion, IK and authored
hit-box evaluation. Its public contract contains POD records; filesystem loading
uses qcommon handles and zone ownership outside frame evaluation. Game/cgame own
replicated inputs and presentation policy; render owns copied skin matrices.

`engine/physics` owns the single cosmetic Jolt world behind a POD public contract.
Caller-supplied map storage owns all dependency allocations; shapes, slots and
constraints are prepared before fixed stepping. Unused slots remain asleep on a
non-colliding, query-excluded layer. The client alone links this subsystem; native
movement, traces, hit registration and authoritative projectiles stay in CM/game.
Foreign allocation failure terminates and must never longjmp through Jolt.

`engine/weapons` owns cooked weapon definitions, seeded fixed-tick state, reloads,
attachments, projectile math and notify deduplication. All runtime records are POD
and bounded; game owns damage/rewind and actors, cgame owns prediction/presentation,
and the development overlay owns range controls. Assets load only at map start.

`engine/effects` owns cooked effect/decal records and bounded presentation state:
seeded particles, emitter updates, collision callbacks and fading decal rings.
It never changes authoritative simulation. Render owns registration, images,
scene submission, temporal history and residency; cgame chooses authored effects
from predicted/material-hit presentation events. Cooker-only meshoptimizer code
is never linked into the engine.

`engine/server` owns server clients, authoritative snapshot assembly and the
native game lifecycle. It borrows game/entity data through `engine/public` and
owns server allocations in the existing zone/hunk lifetimes. Network sockets
belong to platform; packet sequencing and representation belong to qcommon.

`engine/client` owns connection, prediction, input interpretation, demos, console,
cinematic and AVI state. `client_public.h` exposes platform/sound services without
exporting private client structures. Sound uses frame/lifecycle operations and
client-owned AVI timing; the original sample arithmetic and ordering are retained.

`engine/sound` owns codecs, sound caches, mixing and sound registration. Its public
header defines the existing DMA contract with platform backends, raw cinematic
sample timing and the sound API. It owns sound buffers through the existing
allocator paths; this boundary change does not add allocations or alter mixing.

`engine/botlib` owns AAS navigation, bot decisions, scripts and bot development
logs. It receives engine services through its public import contract and public
qcommon/platform interfaces. Obsolete BSPC, MEQCC and SCREWUP integrations, whose
tool implementations are absent from this repository, have been removed from the
shared parser files; the BOTLIB implementation is retained.

`engine/renderercommon` owns renderer ABI types, OpenGL function declarations and
shared image/font routines. The OpenGL renderers are retired. `engine/render`
owns portable scene traversal, materials,
geometry and pass selection. `engine/rhi` owns the plain GPU contract;
`engine/renderervk` owns Vulkan objects, GPU resources and command encoding. The
frontend keeps its private state and borrows client data only through the public
renderer import/export contract. The unused renderer2 implementation and its
projects are retired, as decided in the port plan. Generated SPIR-V data remains
unchanged; `tools/shaders` owns its generator.

`engine/devtools` owns the optional development overlay and console ring. The
vendor ImGui context is reached through plain pointers; renderer commands cross
only the public draw-data contract. Input comes from existing engine events.
Cvar/console actions execute after vendor UI calls return. The allocator layer
owns the fixed UI arena, and the entire feature is absent with its CMake option OFF.

`engine/platform` owns OS access: sockets, clocks, CPU/affinity discovery, windows,
input devices, audio devices, process pipes and debug UI. Unix, Windows and SDL
implementations live below it; shared runtime code lives directly in it. Public
headers also isolate OS-dependent file types and graphics SDK declarations.
Small inline debug wrappers retain existing calls without changing renderer ABI.
Assembly stays under `platform/asm`, with its existing C-linkage boundary.

`engine/public` owns the native game/cgame/UI service contracts and shared game
ABI declarations used by the engine. This location keeps the dependency direction
from game to engine; it contains contracts, not gameplay implementation. No VM or
JIT implementation remains.

`game/game` owns authoritative game rules and bot game behavior; `game/cgame` owns
client game presentation and prediction; `game/ui` owns menus. `game/bg` owns
shared movement, gameplay helpers, imported foundational schemas, menu constants,
render types and compatibility library behavior. `game/module.cpp` preserves the
separate static module namespaces and explicit lifecycle resets. The original GPL
source hashes and transformation history remain in `docs/native-game-import.json`.

`third_party` owns vendored JPEG, Ogg, Vorbis, curl/SDL headers, Vulkan headers,
minizip, zlib, Jolt/joltc and Opus code. `cmake/Audio.cmake` builds pinned Opus
as static C; codecs use prepared state and bounded sound queues. Existing C libraries stay C; the already-ported minizip and
puff C++ files keep their language and lifetime-analysis coverage. Dependencies
are not engine subsystems and keep their upstream API conventions.

`tools` owns shader generators and historical port gates. `cmake` owns production
source lists and cross toolchains; Visual Studio projects are generated in the
build directory.
Reusable artifact gates recognize the new layout and old oracle paths. Historical
port drivers that refer to recorded source revisions remain historical evidence.
`tests` owns permanent build, runtime, fixed-replay and boundary verification;
accepted fixtures are never regenerated by CI. Recorded bugs now live in
`docs/bugs.md` and remain separate #31 changes.

The client/renderer import boundary carries platform graphics handles as opaque
64-bit values. Vulkan SDK headers belong only in `engine/renderervk` or
`engine/platform`; the include gate rejects them in portable/public headers.
Renderer module API version 10 includes filesystem cache services;
optional PC modules must be rebuilt together with the client. The scene/export
interface and native game service contracts are unchanged.

During #6 extraction, the renderer frontend consumes only `rhi_public.h` for GPU
operations; its main header and the client ABI are checked for transitive SDK
includes. Vertex/raster generation and frame command selection are frontend-owned.
Vulkan device/world records are private to `vk.cpp`. Copied device settings and
explicit host services replace frontend globals. Backend failures return status
before the frontend invokes the engine error callback. The 26 frontend files moved
unchanged into `engine/render`; `docs/rhi-frontend-move.json` records source revision,
paths and SHA-256 hashes. CMake and lifetime checks cover the new subsystem.
