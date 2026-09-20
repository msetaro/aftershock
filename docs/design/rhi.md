# RHI extraction design (#6)

Implementation authorized by the maintainer continuation of 2026-09-19. The #8
code rules are in force. The reference is the existing Vulkan renderer. Acceptance requires
unchanged sampled-frame hashes from the committed demo fixtures. Simulation, asset interpretation, draw ordering, shader
arithmetic, blend/depth state and image conversion remain unchanged.

## Boundary and ownership

Keep the client-facing `refimport_t`/`refexport_t` interface during extraction.
It is a scene/asset API; the RHI is a separate, lower-level GPU API. Do not expose
Vulkan objects through either the client ABI or the portable RHI header.

The intended dependency direction is client -> portable renderer -> RHI backend
-> platform graphics bridge. The portable renderer owns scene traversal, material
selection, geometry generation, sorting and the existing sequence of render
passes. Vulkan owns device objects, GPU allocation, synchronization and command
encoding. Platform owns windows, native surfaces, loader access and OS calls.
Filesystem services own shader and pipeline-cache file access. The current
[ownership rules](../subsystems.md) and include/OS checks remain the starting point.

Extract in place first. Once the boundary works, move the portable `tr_*` frontend
from `renderervk` to `engine/render` using hash-verified moves. Keep shared image,
font and client ABI declarations in `renderercommon`; leave only Vulkan backend
implementation in `renderervk`. Remove the legacy OpenGL `engine/renderer` after
the Vulkan extraction passes its gates. Update the explicit CMake source lists,
subsystem ownership and include checks with the moves. Do not combine these moves
with arithmetic or material changes.

## Small public contract

Use C++20 free functions and plain records in `rhi_public.h`, with one statically
selected implementation. No inheritance hierarchy, backend factory or additional
runtime dispatch table. Optional PC renderer modules retain the existing outer
renderer module boundary; static linking is the primary configuration.

The contract covers device/capabilities, presentation, buffers, images, samplers,
shader packages, pipelines, descriptor bindings, command lists, submission fences
and timestamp queries. Add only operations required by the existing Vulkan
renderer. Use fixed-width enums/fields and opaque resource handles; resource
records and returned capabilities are trivially copyable. SDK handles, allocation
objects and platform types stay private. Explicitly describe image formats,
subresources, attachment load/store behavior, resource access and buffer ranges;
do not expose numeric Vulkan enum values as portable enums.

The initial command API records the current viewport/scissor, bindings, indexed
and non-indexed draws, copies and pass transitions. Preserve existing binding
slots, formats, attachment ordering and synchronization before attempting to
simplify them. The second backend stub implements this same header without a
Vulkan include or SDK dependency. It is compile-only and returns unsupported
initialization; it must never count as a successful rendering/replay test.

## Lifetimes, memory and errors

Preserve the current two command-buffer/frame slots (`NUM_COMMAND_BUFFERS` in
[engine/renderervk/vk.h](../../engine/renderervk/vk.h)) and their fence ownership
first. A frame slot's uploads, descriptors and deferred deletions cannot be reused
until its submission completes. Resource creation/destruction remains explicit;
CPU handles borrow backend-owned records. Device lifetime, map registration and
frame scratch have separate ownership and reset points. Keep the existing
zone/hunk interfaces for CPU storage and bounded pools for frame commands; add no
per-frame CPU allocation. Measure current geometry/upload high-water marks before
changing capacities or the existing exceptional resize behavior.

Return explicit status from backend/platform operations. Report errors through
`Com_Error` only after returning to a boundary with no live non-trivial stack
objects. GPU/OS wrappers may use RAII only within calls that cannot be crossed by
that longjmp. Aborted frames require explicit cleanup/reset; backend resource
tables retain ownership until normal teardown or a fence-safe retirement point.
Resize, minimized windows, renderer restart and device loss need explicit states;
never present an invalid image or reuse an outstanding upload allocation.

The Vulkan implementation contains its legacy abort inside each fallible public
call using standard setjmp/longjmp and trivial automatic records. The frontend
checks the returned status and reports the preserved fatal/drop diagnostic. This
is distinct from the engine-wide Com_Error jump. The lifetime gate remains
required, including optional module builds. A cached pipeline bind takes the
existing direct path. Texture conversion and its hunk scratch belong to the
frontend; scratch is freed before reporting an upload error. Process-fatal zone
allocator callbacks remain host services. Initialization receives copied device settings and seven explicit host services.
It borrows its diagnostic/capability output only during the call, including failure
returns. Post-process settings update at the existing frontend points. The backend
has no dependency on frontend records, cvar pointers or private headers. Live
minimization and swap-interval queries keep their original call sites.

## Shader and pipeline artifacts

Initially preserve the existing generated SPIR-V bytes and generator inputs.
Do not hand-edit `shader_data.cpp`, change compiler flags, recalculate shader
expressions or recompile every shader as part of extraction. Build-time shader
packages can subsequently replace embedding in a separately verified step.

Compile GLSL/HLSL offline through the asset build. Package shader bytes with stage,
entry point and binding/layout metadata. A content hash includes sources and
includes, compiler identity/version, options, target and layout schema. A cache
miss uses the packaged shader; it does not compile source in the game process.
Pipeline descriptions and shader package hashes identify logical pipelines.
Persisted driver cache data additionally carries backend/device/driver and cache
format compatibility information; reject incompatible cache data and rebuild the
pipeline from the packaged shader. Keep cache I/O in the filesystem layer and
pipeline creation outside steady-state draw submission where possible.

## Passes and profiling

Phase one retains the current explicit main, screen-map, bloom, capture and gamma
pass sequence, including barriers and render-target formats. Do not introduce a
render graph during extraction. Preserve existing debug markers and expose bounded
GPU timestamp scopes with asynchronous readback after fence completion; profiling
must not insert a wait in each draw or change what is rendered.

Phase two may add pass declarations for inputs/outputs, transient lifetimes and
barriers. Begin by reproducing the established pass sequence exactly. Allocation
aliasing, pass reordering, new rendering features and shader optimization each
need separate measurements and replay verification; they are not prerequisites
for the thin RHI boundary.

## Implementation gates

1. Capture the current Vulkan fixture/frame hashes and record renderer settings,
   driver versions, resource counts, peak upload use and frame timings. Use both
   local Quake 3 and hosted OpenArena content; retain their separate goldens.
2. Introduce the public records and compile-only second backend stub. Build the
   static client, dedicated server, optional PC renderer module and cross targets;
   enforce public includes and platform-only OS access.
3. Wrap device/resources, then command recording and synchronization in small
   changes. Compare generated code where practical and run fixed-demo replay
   after every extraction step. Exercise renderer restart, map transitions,
   minimize/resize, upload reuse and shutdown. No fixture regeneration for this
   work; any discovered engine defect goes to a separate #31 PR.
4. Move the proven portable frontend, verify moved-file hashes, then delete the
   legacy OpenGL renderer and its build selection. Keep historical OpenGL
   comparison evidence; the continuing rendering gate is Vulkan replay against
   the accepted content-specific frame hashes. Keep the dedicated-server build
   independent of any GPU backend.
5. Require the full regression/build matrix, lifetime/boundary gates, the second
   backend compile check and unchanged demo hashes on the merged tree. Document
   measured CPU/GPU/memory differences and any remaining limitations. Rendering
   features and render-graph phase two remain separate work after this gate.

No console SDK implementation is proposed here. The stub checks that the API is
independent of Vulkan; actual platform backends still need their own SDK builds,
capability mappings, synchronization validation and hardware measurements.

## Query implementation references

Timestamp collection follows the core Vulkan 1.0 path already supported by this
backend: [timestamp writes](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdWriteTimestamp.html)
and [query result availability](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetQueryPoolResults.html).
Per-frame query ranges are read after their existing fence succeeds and reset in
the next command buffer. No query-result WAIT flag or additional fence is used.
Only the queue's valid timestamp bits participate in wraparound differences.
`tests/demo.py --measure-gpu` uses separate real-clock replays; software-driver
queries under the frame gate's faketime environment are not timing measurements.
