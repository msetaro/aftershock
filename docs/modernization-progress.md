# Modernization checkpoint

Integration: `modernization`. Issue branches: `issue/<number>-<slug>`, one bug per
#31 PR and one warning class per #8 PR. Merge commits only after gates/self-review.
Never push main, force-push, rewrite history, or touch port-evidence.

Maintainer continuation (2026-09-19): continue the modernization roadmap through
completion or a dependency requiring the maintainer. This supersedes the earlier
stop after #8/design-only #6. Implement #6 next, then follow #25; preserve the
separate-session #35 scope and the completed network-test evidence.

Maintainer ruling (2026-09-19): keep all future changes and PRs in
`msetaro/aftershock`. Do not create PRs against ec-/Quake3e or another parent
repository. This replaces the earlier requirement to submit applicable #31 fixes
upstream; historical upstream PR references below are completed past work.

## Next action

Current branch: `issue/11-weapons`; draft PR #150, based on #12 merge 3bb04837.
Continue #11 with test-first weapon snapshot/prediction and gameplay integration,
then the target range using the existing owned rifle/body assets. #12 integration
regression 35523091952 passed. Complete the full #11 scope, wire tests into CI, run all gates/self-review
and merge through its own PR. #11 was read; #10/#12 are
its prerequisites. Reuse existing fixed-tick, asset and animation APIs.

#12 PR #149 merged with a merge commit as
3bb048375ccb3b7497ffd536eba37fc5cf1dbe8a. Its tree
93a79e9b32ddfca56f57702fe6476a69478aeb37 equals the fully tested 5078d3ab tree.
Exact-head build 35522266447 and regression 35522266448 passed after committed
AGENTS self-review. Hosted budgeted loopback: 634/634 shots agree (31 hits), 64
uncompensated differences, median view age 154 ms, maximum prediction error
8.875 units. Matching/version-2 connection tests and fixed static/module animation
replays (253 authoritative hashes each) passed. The reviewed local final run
passed 385/385 shots (20 hits), 39 uncompensated differences, median age 146 ms
and the same prediction bound. No fixture/golden changed.
Acceptance comment: https://github.com/msetaro/aftershock/issues/12#issuecomment-5751108810
Merged-tree regression 35523091952 passed; #12 is closed and checked in #25.
Preserve the provider/transport boundary recorded below.

## #11 test-first scope

The first portable contract covers a versioned weapon asset in the existing
cooker, a second rifle authored only in JSON, incremental dependencies, exact
20 ms ticks, 1000 repeatable seeded shots, alternate-seed divergence, recoil
patterns, automatic/semi/burst cadence, staged tactical reload and cancellation,
magazine/chamber/reserve conservation, ADS interpolation, data attachments,
range falloff/material penetration response and projectile step/bounce math.
The initial probe failed on the absent weapons_public.h and weapons.cpp
(weapons-before.log), before any weapon runtime/cooker implementation.
This is the first slice, not #11 acceptance. Server rewind integration, replicated
projectiles/grenades and prediction, switching/dual-wield/melee, animation/sound
notifies, real target-range/ImGui controls and fixed-demo parity remain required.
The original #10 rifle/body sources and accepted fixtures remain unchanged.

The initial weapon payload/runtime now passes GCC and Clang/libc++ UBSan,
including the independent 1000-shot integer/binary32 reference. Both trace hashes
are 0c1b0e259650e6c5c6c155244100b3e194abbfc75fe7d10717cdb25b919c746f
(weapons-reference-gcc.log, weapons-reference-clang.log). Client/server production
build passes (weapons-core-build.log); only the new weapon translation unit gains
strict FP flags. Payload size is 3936 bytes, plain state 56 bytes, fixed arrays and
20 ms stepping with no per-frame allocation. Tactical/empty reload conservation,
cancel points, melee cadence, clock wrap and projectile fuse checks pass. The
existing unsigned form of Q_rand's recurrence supplies this new seeded state;
legacy RNG code is untouched. New tests 125c8016/9a047470 preceded implementation.
This is not #11 acceptance: data-to-gameplay and rendering/audio integration,
replication/prediction, target-range tooling and replay gates remain outstanding.

Follow-up test-first checks fail separately on two unfinished parts of this new
feature: native cooked-index acceptance of kind 7 (weapons-index-before.log), and
30 ms fire intervals retaining their phase across 20 ms ticks rather than slowing
to 40 ms (weapons-cadence-before.log). Extend index registration and carry the
sub-tick remainder while discarding stale idle/reload backlog. Existing 80 ms
1000-shot trace must remain identical.
Both follow-ups now pass GCC and Clang/libc++ UBSan after ca557339's failing
assertions: kind 7 is registered, a 30 ms cadence alternates quantized intervals
without slowing its mean rate, and old idle/reload backlog is not emitted as a
burst. The 80 ms 1000-shot digest remains unchanged (weapons-cadence-gcc.log,
weapons-cadence-clang.log). Next: auxiliary weapon state round-tripped through
the actual delta codec, then server/client fixed-tick gameplay integration.


## #12 implemented feature evidence

Implemented so far: generated replication descriptions beside state members,
strict Aftershock version/schema agreement in the existing challenge/connect
flow, bounded per-frame hit-box history and opt-in hitscan rewind. `g_rewind`
defaults off; `g_maxRewind` defaults to 200 ms (hard ceiling 1000 ms). Storage is
64 frames, 1024 entities, 2048 boxes/frame, under 5 MiB from the level hunk with
restart reuse. Owned animated actors use #10 boxes; other eligible actors use
bounds. World/brush collision stays live, and no live actor transform/link is
mutated. Generation checks prevent spawn/respawn/teleport interpolation.

Passed: GCC/Clang+UBSan byte/schema/compatibility/history/game-integration probes,
current C/game C++/engine ABI, format/boundaries, and production client/server build.
Real loopback Q3: 425/425 unambiguous shots agree, including 21 hits; 45 decisions
would differ without rewind. Model: 100 ms RTT, +/-15 ms combined jitter, 5% packet
loss; median view age 149 ms; largest prediction error 8.875 units. Gate thresholds
are >=99% over >=100 shots with >=5 hits and >=5 uncompensated differences, and
prediction error <=32 units including initial spawn. Portable linear-target test:
950/950 at 100 ms one-way delay, +/-15 ms jitter, 5% loss; error <=0.000488 units.
No simulation-affecting legacy floating-point expression or accepted fixture changed.

Real network test first failed on the absent cheats-only developer target, then
exposed harness ordering (inventory not yet received; spawn not settled). It now
waits for an in-game ready message before placement and firing, without weakening
hit thresholds. Logs: netcode-runtime-before.log, netcode-runtime-target.log,
netcode-runtime-synced.log under ~/.cache/aftershock-modernization. OpenArena real
loopback passed 374/374 shots (19 hits), 38 uncompensated differences, median
view age 148 ms and prediction error <=8.875 (netcode-runtime-oa.log). Classic Q3 fixed replay still
matches 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4
(netcode-classic-demo.log); the fixed owned animation demo still matches all 253
received authoritative boxes and repeated frames (netcode-animation-demo.log).

Test-first commits: 013b13d2 schema byte oracle, 0dae55bf version agreement,
adc86e49 portable rewind, 59d11780 game integration, 6e7ccf89 real delayed transport.
Implementations: 531cf0d5 generated tables, 2c9b461e handshake, 54555ea7 history,
7d66955a game integration. New schema baseline (107002 bytes from e1ff877f):
26a5fc0d8e5afbfcc1634ddbfcf67155c6a2c6156066b86d20088690dff7d496. No accepted
golden was regenerated. Connections lacking Aftershock agreement are refused;
legacy demo decoding remains independent. Actual Steamworks SDK/platform service
implementation belongs to #23; #12 must establish the checked asynchronous seam
and must never treat a claimed identity or absent provider as authenticated.

#10 is complete, closed and checked in #25. PR #148 merged as
e1ff877f87d7cb17d62f41977428e08c10cb3920 (tree
c04729f603ae4ede3814e93c8ad69e85688c55d2), exactly matching tested head acc12bf3.
Full build 35516551419 and regression 35516551423 passed before merge;
merged-tree regression 35517250474 passed all required jobs after AGENTS review.
Continue #12 before #11, then the remaining #25 roadmap, only in this repository.

Telemetry test-first commit 5deecae7 failed on absent prediction aggregates and
rewind reports (netcode-metrics-before.log). These now pass GCC/Clang developer
telemetry checks. The server sends at most four reports per second per firing
client through reliable game commands; the remote overlay shows age/budget,
sampled hits/clamping and prediction last/peak/mean. Invalid reports are ignored.
OpenArena with report delivery verified passes 371/371 shots (19 hits), 40
uncompensated differences, median view age 147 ms, prediction error <=8.875
(netcode-metrics-runtime.log). Full build d381167e passed 35519899293; regression
35519899276 passed on that earlier head. Current work is not yet accepted.

Replication-policy test-first now fails on absent sv_replication.cpp
(netcode-policy-before.log). It specifies priority and age within an optional
update budget derived per client, retention of last acknowledged state for
unaffordable changes, immediate removal outside interest, preservation of retained
storage lifetime, range controls and admission of high-priority entities beyond
the legacy first-256 candidate slots. Budget 0 must preserve the old exact path;
wire capacity stays 256. Required playerstate/reliable/removal traffic remains
mandatory even if it exceeds a tiny optional-update budget; existing rate limiting
continues to account for actual transmitted bytes.

Replication policy now passes GCC and Clang/libc++ UBSan probes. Radius interest
narrows existing visibility, priorities select up to 256 entities from the full
candidate set, and age schedules optional deltas against the client rate budget.
Unchanged defaults preserve the old path. The real OpenArena 48-byte budget run
verified both deferred updates and receipt of the target state: 393/393 shots
agree (20 hits), 48 differ without rewind, median age 150 ms and prediction error
<=8.875 units (netcode-policy-received.log). The initial 128-byte trial did not
exercise deferral, so the test retained its assertion and reduced the budget.
Server status reports cumulative deferrals and mandatory-traffic overruns.
Test-first commit 417e130f precedes implementation (52b73467).

Identity seam test-first now fails on the absent sv_identity.cpp
(netcode-identity-before.log). It requires anonymous/null behavior, asynchronous
Steam verification, revocation/timeout cleanup, rejection of stale connection
callbacks, immutable provider ownership once used, and generation-tagged browser
and matchmaking results. #12 exposes bounded main-thread hooks; #23 supplies the
SDK, ticket transport and platform UI. No null backend or claimed ID can report
an authenticated identity. Provider hooks must be exercised through the actual
server lifecycle, not just an isolated mock of the state machine.

Identity implementation now passes GCC and Clang/libc++ UBSan
(netcode-identity-gcc.log, netcode-identity-clang.log), following a331a836's failing
tests. Server connect/free/frame paths open/close/poll connection generations;
game imports return account IDs only after verification. Expiry and revocation
end the provider session and drop the client. Provider absence stays anonymous.
Native UI search imports copy bounded results and cancel on shutdown. OS/SDK
ownership stays in platform; no SDK dependency is introduced. Tests cover both
server identity code and the production platform dispatch. Decision: #12 delivers
the requested interface; #23 supplies real Steam ticket transport, SDK callbacks
and platform UI. There is no ticket wire command or requirement for authenticated
players before that backend exists. This boundary is explicit in tests/README.md.

All new probes are wired into both compiler CI jobs. Runtime CI builds matching
and version-2 servers, runs actual protocol acceptance/refusal, then the OpenArena
48-byte-budget delayed hitscan gate; fixed classic and animation replays remain.
AGENTS commands and tests/README.md document controls, limits and replay policy.
The initial identity client/server production build passes; final local/hosted
acceptance and complete self-review are still required.

Full local #12 probes/ABI/developer data pass on e64e214f. Matching/mismatched
real connections pass; the final OpenArena budget run passes 680/680 shots
(33 hits), 78 uncompensated differences, median age 155 ms and prediction error
<=8.875 (netcode-final-runtime.log). Classic Q3 replay retains the accepted frame
hash. Tidy passes 1238 configurations; lifetime analysis is still running.
Self-review found a defect in this PR's new rewind trace plane metadata: a
negative axial normal must use PLANE_NON_AXIAL, matching PlaneTypeForNormal.
The new assertion fails before correction (netcode-plane-before.log); correct
that new feature code before acceptance. Existing collision arithmetic is unchanged.
Hosted e64e214f regression 35521897618 exposed added serverinfo metadata in the
unchanged OpenArena bot log: sv_snapshotBudget plus its 20-byte length increase.
The budget is server-only and no client consumes it, so remove CVAR_SERVERINFO
from the new cvar rather than changing accepted goldens or widening normalization.
Compiler unit, sanitizers and both cross jobs already pass on that head.
The plane assertion in 47b4ec60 now passes under both compilers after using the
existing PlaneTypeForNormal macro in the new trace adapter. Positive and negative
impact normals/distances/signbits are covered. Rebuild and rerun the affected
runtime/fixed replay gates; the pending full gates must target the corrected head.

## #12 self-review and acceptance checkpoint

Scope: #12's version/schema agreement, generated descriptions, opt-in bounded
rewind, replication policy, telemetry and checked identity/discovery seams. Steam
SDK/ticket transport remains #23's implementation responsibility, explicitly
reported on #12. No parent-repository change, unrelated engine fix, accepted
golden/fixture regeneration or legacy simulation FP restructuring is included.
The new trace adapter now uses the existing plane classification convention.

Server-only snapshot budget registration avoids adding unused wire/serverinfo
metadata. The resulting OpenArena oa_dm1/oa_dm7 bot logs match accepted goldens
(netcode-bot-oa.log). Required traffic remains mandatory; existing address,
challenge, command and transmit rate controls are retained. Interest narrows PVS;
priority/budget selection stays bounded and keeps old snapshot-storage generation
checks when retaining acknowledged state.

New runtime storage is fixed POD or level-hunk owned. There is no per-frame heap
allocation, non-trivial core destructor, or OS/SDK call outside platform. Wire
and module layouts still agree across C/game C++/engine C++; codec digest is
unchanged. New sampling arithmetic uses strict FP. Provider callbacks are
main-thread, generation-checked and bounded; null/claimed identities are never
verified. The real UI wrapper probe checks safe copies, stale handles and terminal
buffer failure cancellation under both compilers (netcode-ui-discovery*.log).

Local evidence: unit plus one-ULP negative control, sanitized units, collision
differential, both OpenArena bot goldens, classic Q3 replay, fixed Q3 animation
replay (253 authoritative boxes), native ABI and all new GCC/Clang UBSan contracts
pass. After plane correction, live budgeted OA passes 487/487 shots (25 hits),
50 uncompensated differences, median view age 149 ms and prediction error
<=8.875 (netcode-plane-runtime.log). Tidy passes 1238 configurations; lifetime
analysis passes 1184 commands. Hosted eb017dc5 build 35522039819 passes all Linux,
macOS, MinGW and MSVC x64/ARM64 legs. Its regression repeats the now-corrected
serverinfo-only bot mismatch; it is not acceptance. Push this reviewed correction
and require a fresh full current-head build/regression before ready/merge.

## #10 accepted implementation

Implemented: cooked graphs and compressed pose sampling, blend trees/masks/additive
layers, fixed-step events/root motion/IK, copied renderer poses, authored rifle/body
controllers, replicated hit boxes, automatic body facing, ADS/recoil/sway, and an
ImGui source editor/compiled-table inspector/preview. Editor and native GCC/Clang
UBSan tests pass. Current C/game C++/engine C++ ABI checks retain 29 types, three
offsets and the extension value; the whole-game C/DLL port oracles are historical
at 7f4d43a7, with classic/shared-math gates retained. No accepted golden changed.

Two separate #10 demos were recorded once from 3fbc0a62 (q3dm17, oa_dm1). Each
replay matched 253 received hit-box hashes against the saved authoritative trace,
and three frame hashes repeated. All six new captures were visually reviewed.
Fixtures/manifests are under `tests/golden/animation`; do not record them again.
Current verification covers the fixed-tick publication guard, owner visibility
bounds and reused-entity slot checks. Passed: unchanged classic Q3 frame projection, fixed Q3 and OpenArena module
replays (253 matching boxes each), unit/one-ULP gate, sanitized units, collision
differential, old asset-editor workflow, and lifetime analysis (1156 commands).
Classic bot smoke initially differed only in this host’s rotated IPv6 addresses;
the shared log normalizer now removes IP/IP6 enumeration from both sides, without
changing goldens or gameplay lines. Both-map bot smoke now passes with that metadata-only normalization
(animation-runtime-classic.log).
Tidy passed 1202 configurations before the final small publication/ownership edits.

All PRs stay in this repository; no parent-fork PRs or main pushes.

## #10 local self-review

Scope matches #10's cooked runtime, authored rifle/body graphs, game notifies,
fixed-step replicated pose state, ImGui authoring and fixed-demo parity. New
animation arithmetic is strictly compiled; existing simulation expressions and
accepted classic fixture bytes are unchanged. No engine bug fix outside the
feature is included. The native port-era C/DLL comparison is explicitly historical;
current ABI/shared math and classic replay checks remain in CI. Host interface
address normalization removes metadata only, from both expected and actual logs.

No new OS calls occur outside the filesystem layer. Animation file storage has
explicit load/shutdown ownership; simulation/render poses use bounded POD arrays
and no per-frame allocation. Lifetime analysis passes 1156 commands, including
the new subsystem; type/boundary/format checks pass. Wire structs retain their
layouts (C/game C++/engine C++ agreement), and cooked animation records assert
layout/trivial-copy properties. Renderer ABI is 13 shipping / 17 development.

Native GCC/Clang+UBSan, the real-input editor, fixed-step live gameplay (including
100 Hz server), new Q3/static and OA/module fixed replays, unchanged classic Q3
frames/collision/bot smoke, unit/negative control, sanitized units, old asset editor,
known-bug classification, tidy and lifetime gates pass locally. Exact-head hosted build/regression and merged-tree verification passed. The original
assets and new demos contain no copied game paks; both new source/demo manifests
record provenance and exact hashes. All changes and PRs remain in this repository.

## #10 implementation evidence (chronological)

#9 PR #145 merged as c195f798 and passed integration 35507482742; #9 is closed
and checked in #25. Accounting PR #146 merged as 3d104d0c after exact-head build
35508534162/regression 35508533987; integration 35508970698 passed.
Scale PR #147 merged as 7f4d43a7 after exact head dd8f7f79 passed build
35509606176/regression 35509606177 with its committed self-review. Its merge tree
matches the tested tree (796b6c532309846d4913439d74dfc751c4ca4ae1).
Merged-tree regression 35510058541 passed. Both IQM fixes are accepted; no active
known-bug entry or UBSan suppression remains. #31 can close at this checkpoint.

Current branch is issue/10-animation, with modernization merged forward. #31 is
closed after both IQM fixes passed merged-tree regression. Test-first commits
57838d59/b0c95261/47b1c807 now pass the initial cooked-graph/native runtime slice
(animation-core-first.log): sampling, transitions, loop event boundaries,
translation root motion, masked/additive transforms and two-bone/look-at IK.
The new graph reuses IQM quantized poses and binds the exact cooked model hash;
source manifests track graph/glTF/buffer dependencies and graph-only edits.
Original Blender rifle/body assets and the earlier native viewer checks remain
unchanged. This is feature development, not #10 acceptance or gameplay parity.

Initial portable/cooker implementation is cdc87445; GCC and Clang/libc++ with
UBSan pass (animation-core-first.log, animation-core-clang.log), as do format,
boundary and type checks. New source-only tests now expose the unimplemented
turning-loop composition (animation-root-events-before.log, expected assertion).
They also specify initial time-zero events, bounded transactional event overflow
and unsigned clock wrap. These now pass GCC/Clang+UBSan after ordered rigid root
composition and explicit initial-entry bookkeeping (animation-root-events-after.log,
animation-root-events-clang.log). State/event output remains atomic on overflow;
repeated ticks cannot double-deliver or cascade transitions. Next: data-authored
blend trees/masked additive layers, then production/gameplay integration.
New tree test now fails on the unchanged runtime (animation-trees-before.log):
a numeric parameter blends idle/wave while a second drives additive wave motion
only on the tip subtree. It checks both composed rotation and untouched root
translation. Implement flat topologically ordered nodes and fixed bone masks;
reuse the already-tested transform operators. Implemented: up to 64 flat nodes,
16 masks with 128 weights, parameter blends and reference-relative additive
layers. The tree contract passes GCC and Clang/libc++ with UBSan
(animation-trees-after.log, animation-trees-clang.log). Production client/server
build passes (animation-build.log); the new core has explicit strict FP flags.
SHA ownership is shared core plus a separate copy only for optional renderer
modules. A full cooker check overlapped the source-list edit and invalidated its
tool hash mid-run; the stable rerun passed (animation-cook-regression-stable.log). Next: renderer copied-pose submission, authored rifle/body
graphs, gameplay/replication and ImGui authoring. Renderer submission test-first
now fails to compile against the absent API (animation-render-before.log). It
requires graph/model revision agreement, copied skin matrices and culling bounds,
128 poses per renderer frame, capacity rejection and frame reset. Legacy entity
submission must clear a reused pose pointer. This is ordinary render ownership
coverage using a small constructed skeleton; gameplay parity remains outstanding.
Implemented renderer pose submission passes GCC/Clang+UBSan and the client build
(animation-render-after.log, animation-render-clang.log, animation-render-build.log).
Skin matrices live in renderer frame storage; custom bounds cover the transformed
bind mesh, and legacy frame sampling remains on its existing path. Renderer ABI
is now 13 shipping / 17 development; modules must be rebuilt together. Verified
cooked model hashes persist across initial registration and development reload.
The new hand-authored rifle/body graphs pass native state/event/socket/layer
checks (animation-rigs-test.log). Rifle: idle/ADS/fire/reload/sprint/jump, shot,
shell and four reload stages. Body: idle/walk/run blend, masked aim/lean,
crouch/prone, turn and alternating footsteps. `rigs.json` adds graph recipes;
original Blender files, original project and provenance hashes are unchanged.
Classic fixed Q3 replays still match projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4
(animation-classic-demo-fresh.log); the default output directory was from another
worktree, so validation used a fresh build directory. No fixture/golden changes.
Next: replicated gameplay pose state and real presentation/ImGui tests.
Snapshot test-first now fails on the absent shared adapter
(animation-snapshot-before.log). Decision: explicit auxiliary animation entities
(type 255, outside the existing event range), using the existing entity delta
codec and unchanged wire structs. Their own fields carry nine state words,
sixteen float parameters, owner/rig and origin/view angles. Legacy player fields
are untouched. Trace event consumers and exclude this new type before use;
prove the full state/float round trip through production MSG functions.
The adapter now passes GCC and Clang/libc++ with UBSan: all nine state words,
16 float parameters, owner/rig and origin/angles survive the real delta codec
(92-byte initial / 10-byte time-only delta for the test record). Native client/
server builds pass; public animation constants use inline constexpr to satisfy
the existing native unused-constant policy. CG_CheckEvents and BotCheckEvents
explicitly exclude the new auxiliary type; the render dispatcher/botlib already
skip the event-range types, and auxiliary solidity/loop sound/event stay zero.
No player snapshot field or codec expression changed. Logs:
animation-snapshot-after.log, animation-snapshot-clang.log,
animation-snapshot-build.log. Gameplay publication/loading is the next step.
The new live gameplay test runs the real client with owned native game code and
owned cooked rigs, then checks states/events/render counts and matching server/
client hit-box digests. It currently fails as expected with no animation states
(animation-gameplay-before.log). A native test also specifies authored hit-box
bounds and in-place root translation (animation-boxes-before.log, absent API).
Implement the hit-box output and game paths before accepting either test. These
are new #10 artifacts; no existing fixture is regenerated.
Authored boxes/in-place poses pass GCC and Clang+UBSan (animation-boxes-after.log,
animation-boxes-clang.log). Game publication/loading and client presentation are
now implemented in the worktree and build (animation-game-build.log); the first
live gameplay run passed (animation-gameplay-first.log). Graph files are owned zone allocations loaded at
module init and freed on shutdown. Server animation advances at 20 ms, publishes
auxiliary snapshots, and derives boxes from the same immutable graph/parameters
used by the client. Rifle/body presentation copies poses through the renderer;
first-person FOV uses the existing entity transform. The smoke saw all six rifle
states and expected reload/shot/shell/footstep events, 434 body/294 rifle render
submissions, and matching received client/server box hashes. ADS and third-person
captures were visually reviewed; these are the intended original block rigs.
Remaining: actual recorded-demo parity, integrated IK/sway/aim behavior,
ImGui graph authoring/inspection, content/runtime docs and complete gates.
Live gameplay is committed as ab6283ed. The next pose-IK test fails on the absent
bone-chain application API (animation-pose-ik-before.log); it uses the actual
owned rifle arm and requires the hand to reach a nearby target. Integrate the
existing analytical solver with local rotations and descendant matrices, then
apply hands/feet/look-at in the same pose path used by hit boxes.
This integration now passes the native arm target test and live Q3 smoke
(animation-pose-ik-after.log, animation-gameplay-ik.log). Hands follow weapon
grips/the reload magazine; feet use server collision-derived, replicated height
offsets; head look-at and upper aim use view/input parameters. Server/client
boxes still match after those operations. The generic IK application rejects
non-uniform/sheared ancestor frames without changing the input pose; the owned
rigs use supported uniform frames. Clang+UBSan and OpenArena live gameplay now
pass too (animation-pose-ik-clang.log, animation-gameplay-oa.log). New gameplay
input overrides reset on respawn; authoritative ground offsets cannot be set by
client animation commands. Next: ADS sight placement, automatic body turning,
cosmetic sway, then ImGui authoring and fixed-demo capture/replay.
The automatic turning/ADS test now fails as expected: only idle/move body
states appear when the player rotates (animation-facing-ads-before.log). The
test also requires a centered settled optic, measured from the rendered socket.
Implement authoritative body facing plus client-only sight placement/sway next.
Automatic turning and settled ADS now pass the live test
(animation-facing-ads-after.log): idle/move/turn all occur, the body facing
is replicated, and 40 settled optic samples are centered within 0.000001 units.
Server/client hit-box digests still agree; native checks pass
(animation-facing-core.log). Root rotation is extracted into body facing,
head yaw follows replicated view offsets, and bob/sway fade through ADS.
A rifle additive recoil node now retains the ADS base while firing.
The final graph passes live gameplay and Clang/libc++ UBSan
(animation-facing-final.log, animation-facing-clang.log), including retained
body facing after the turn; format/type/boundary checks pass.
Graph authoring test-first now fails against the missing developer command
(animation-editor-before.log). It will edit initial_state in ImGui, preserve
a byte-identical backup, observe the external cooker revision and render the
changed graph. Implement a bounded source editor and compiled graph inspector;
reuse the existing ImGui and filesystem rather than a new JSON/UI dependency.
Implemented editor now passes the real-input edit/cook/preview check
(animation-editor-after.log): idle changed to ADS, source backup matches exactly,
external cook revision changes and the new pose renders. The UI also exposes
state/event/transition/node/mask tables and fixed-step parameter playback.
Gameplay assets stay immutable until map restart. The ordinary cooked index now
accepts graph entries (kind 6); the existing animation probe checks that valid
project index. Next: complete this slice verification, then record the separate
fixed #10 demo and prove server/client hit-box parity on replay.
The final editor run and GCC/Clang UBSan native checks pass
(animation-editor-final.log, animation-editor-core.log, animation-editor-clang.log).
Current ABI checks pass GCC and Clang/libc++; focused C bot/flag/voter checks also
pass. Lifetime analysis now includes the new engine/animation directory.
The new replay driver correctly fails while its separate #10 fixture is absent
(animation-demo-before.log). Next: record each content-set fixture once from this
implementation, review it, and compare replayed client boxes with the saved server
trace plus repeat frame hashes. No accepted classic fixture will change.
Both new fixtures were recorded once from 3fbc0a62 and replay twice successfully:
253 client hit-box hashes exactly match each saved authoritative trace, and all
three sampled frame hashes repeat (animation-demo-q3-record.log,
animation-demo-oa-record.log). Visual review/fixture commit is next. A higher-rate
server check now exposes a new-feature timing gap: at sv_fps=100, consecutive
server frames can publish different transforms for the same 20 ms animation tick
(animation-fast-server-before.log). Publish pose/transform inputs only when that
fixed animation clock advances. The 100 Hz check now passes
(animation-fast-server-after.log). All six new frame captures were visually
reviewed: the owned block rifle/arms render through the recorded actions.
Fixture hashes: Q3 1895aaf34d32be3330eeb4a728389ddb138cdeac37fdcc565788485abaa8d57b;
OA cc28ee474ca3acfcb493850e680cc8a4f680725057cfc8285c5ab62a0cc71d89.
Tidy passes all 1202 production configurations (animation-tidy.log); lifetime
analysis is still running. Final integration self-review also keeps auxiliary
visibility bounds equal to the owning player and rejects reused non-animation
entity slots on the client. Next: replay these same fixtures with that review
change, finish classic/full gates, then open the #10 PR.
Current ABI checks are extracted as tests/native_abi.py: the same 29 types, three
offsets and extension value agree for C, game C++ and engine C++. Shared C/C++
math checks remain. Full-game C/DLL source gates remain historical at 7f4d43a7;
current game service code is C++ and builds through production CMake. The game
command return type follows existing qboolean declarations so focused C helper
checks remain usable without hiding any feature behind language conditionals.
The old C/C++ DLL import/source-comparison CI steps are port-era oracles and will
need explicit treatment now that the owned native game calls the C++ animation
service. Preserve their accepted pre-#10 evidence; do not add production-only
language conditionals to hide new features from those tests. Retain current wire/
module ABI checks and fixed demo/math parity when updating this CI step.
Full scope remains data-authored state machines/blend trees,
masked/additive layers, events, root motion, IK/aim offsets, rifle idle/ADS/fire/
reload/sprint/jump and sockets, third-person split/aim/footsteps/crouch/prone/lean/
turn, ImGui authoring/inspection, and deterministic fixed-timestep replicated
hit-box parity for a recorded demo. Preserve all classic accepted fixtures/hashes;
new acceptance artifacts stay separate. Continue the complete #25 roadmap after
#10 (#12 before #11's replication dependency). No upstream PRs or main pushes.

## Earlier foundation/integration checkpoint

The #3 -> #31 -> #1 -> #2 -> #4 -> #5 -> #8 implementation sequence is complete on
`modernization`. Design PR #139 merged as e82eb43b after build 35480001019 and
regression 35479955499 passed; merged-tree regression 35480310184 passed.
`docs/design/rhi.md` is now the implementation plan for #6 under the renewed scope.
#6 implementation is merged. Preserved capacities: two frame slots, 4 MiB
normal / 8 MiB high geometry buffers, 2 MiB normal / 24 MiB high staging buffers,
32 samplers and 2,304 pipeline descriptions; do not change these during extraction.
Existing `vkinfo` reports peak vertex/push use, pipelines and image chunks.

## #9 initial feature-test checkpoint

`python3 tests/cook.py` builds an owned valid triangle fixture as external-buffer
glTF and embedded-buffer GLB, with two joints/two clips, plus a static variant
and a small PNG. It requires the offline CLI, native IQM geometry/clip records,
BC7/BC5/BC4 KTX2 mip chains, relative dependency/output SHA-256 manifests,
byte-stable fresh cooks and texture-only incremental invalidation. No installed
game content or accepted artifact is involved. The first run fails with exit 1
because tools/cook does not exist (cook-before.log). Commit this before adding
the cooker. Native runtime/hot reload and UI evidence remain
required; this test is only the first slice.

Decision: reuse the existing IQM v2 model payload/renderer for glTF output,
retaining legacy IQM input. Its plain records already carry assertions and its
runtime model data uses one allocation; development reload can add explicit
owned-block lifetime without changing legacy allocation. Keep named clip data
and cooker version/content metadata available to the next animation stage.
Use standard KTX2 for compressed texture output. Model basis maps glTF (x,y,z)
to engine (x,-z,y); scale is an explicit cooker setting. Use the installed Pillow
for PNG/TGA tools input and a pinned native BC encoder, not a new codec.
Primary format references: Khronos glTF 2.0/KTX2 and lsalzman/iqm iqm.h.
All OS access and source watching stay in tools/platform/filesystem ownership;
shipping code does not import glTF. No new loader-robustness targets are added.

The real Blender source fixture is tests/assets/cook-character: six textured
meshes, one three-joint skin and `idle`/`wave` clips, exported by its committed
script with no hand edits to glTF. Portable Blender 4.5.3 LTS build 67807e1800cc
was verified against official archive SHA-256
975c58fcb244273838534bba771e64ad87739216b0f9b39a888531a49a72d845.
Its provenance records every source/output hash; CI cooks committed sources and
never reauthors them. The installed Pillow 12.1.1 supplies PNG/TGA and BC5
encoding; BC4 can use the BC5 red-channel blocks. Only BC7 needs a small pinned
native encoder helper. The pinned bc7enc sources/license are staged in the
persistent cache, not yet imported into the repository.

The first implementation slice adds offline KTX2 BC7/BC5/BC4 encoding, linear
premultiplied mip filtering, explicit sRGB/data descriptors, source hashes and an
embedded content hash (computed with its own bytes zeroed). The pinned MIT BC7
encoder has only two source files and is built as a separate cached CMake tool;
Pillow supplies BC5, whose red blocks are BC4. The engine does not link this code.
An independent feature check validates all three 16x16 mip chains and block sizes
(cook-texture-check.log). The full CLI/model test still fails until the next slice.
Native compressed-texture loading and hot reload are not yet implemented.

The offline CLI/model slice now passes tests/cook.py under GCC and Clang/libc++.
It handles ordinary static/skinned glTF/GLB, typed/sparse accessors, transforms,
bind poses, sampled named clips, IQM channel quantization, surface splitting and
dependency manifests. The real Blender fixture is consumed by production IQM
code; its root and arm motions match the source clips. New-feature corrections:
normalize clip start time (Blender's first key is at frame 1/30), and preserve
clockwise winding under mirrored static transforms. Both have failing-then-passing
feature evidence (cook-pose-before.log, cook-mirror-before.log). The pose test's
axis expectation was also corrected to the actual exported local-bone motion;
the committed source fixture was not regenerated.

GCC/Clang character outputs match byte-for-byte: IQM
07fc751de9dbff33dbaff55c2f306d12125b314ca3f81be2e66cea08e0e077c8,
material dbd8360ab541bc39cdff1719300772c831b3a6e1ee1d84cdc49e9456b8d2c053,
texture c2c0ea65275b54e97d8e7a7bfc98771d766dfcb6b8857eb4396656327f8a82e4.
Embedded hashes and manifests are independently checked. CI now runs the owned
cooking/pose test under both host compilers. No production engine code has changed
for #9 yet. Explicit WAV/OGG/material/shader project inputs, runtime BC/material
loading, bounded hot reload and ImGui clip/reload evidence remain. Generated
Python bytecode is removed from tracking and ignored; vendor bytes remain exact.

## #7 developer tooling checkpoint

Implemented: optional ImGui console/cvars; texture/material/model inspection;
animation playback; CPU/GPU/frame history and network/prediction observations;
allocator accounting; local native entity editing/save/reload/world selection;
collision/navigation overlays; game-callable debug lines/boxes/text and scopes.
Shipping defaults OFF, keeps renderer ABI 10 and contains no tooling symbols.
Enabled client/modules use ABI 11 and must be rebuilt together.

ImGui v1.92.9b (f1cc2ae15e53a861a874c3034aae6798fde194ab) retains its original
11 core/license files, verified against third_party/imgui/provenance.json.
Archive SHA-256 21d8a0a565e85dce943e375db00812c2f3f0ab21f3f0f7964e364a63422d7f99.
Vendor OS/file/shell/time defaults are disabled. The existing zone allocator backs
a fixed 16 MiB UI arena; engine mutation occurs after vendor UI returns. Owned
history and geometry buffers are bounded and plain, with no simulation arithmetic
changes. Initial/interaction allocation is distinct from the allocation-free idle
path. New-feature corrections include null inactive material stages, resetting
font resources on every renderer shutdown path, and portable numeric validation.

Entity editing reuses the native spawn field table. Original keys, including
unknown ones, are retained in an 8 MiB document; overflow disables saving.
Numbered maps/<map>.dev.NNN.ent saves never overwrite earlier revisions or BSPs.
Explicit dev_loadEntities applies the selected revision once on map restart.
Editing requires a local devmap. Spawn supports pickups and point markers;
structural class/model/team changes require replacement, and brush/mover creation
has no input in this basic editor. External OA demo modules do not register these
owned-game callbacks; entity tests use the owned game with either content set.

World tools cache at most 4,096 lines on explicit refresh using existing winding
helpers; subsequent frames allocate no cached geometry. Nearby brush selection
caps at 1,024. Actual convex/patch faces are clipped; optimized AAS without faces
shows retained area bounds/routes, explicitly labeled. Both layers reserve cache
capacity, omitted edges are counted, and rendering is x-ray. Game primitives cap
at 2,048 lines and 128 labels, with copied text and durations capped at 60 seconds.
Hunk stats report lifetime regions (it has no existing tags), while zones report
all tags. GPU samples use completed frames without waiting. Network payload stats
exclude UDP/IP headers; replay snapshots and unavailable external-game prediction
instrumentation are labeled. Full usage/limits are in tests/README.md.

Final-slice local validation (devtools-final*.log in the persistent cache):
- Real input edits cvar 0 -> 7, plays an installed model, opens inspection panels,
  checks 80 idle frames without further allocations and survives video restart;
  passes both static and optional renderer-module linkage.
- Native entity spawn/edit/delete/save/reload preserves every original map token.
  Real UI spawns/picks the same entity and enables world wireframes plus its label;
  final screenshots visually reviewed. Both collision and navigation caches fill.
- Production probes pass GCC and Clang/libc++ for registry copies, memory, scopes,
  debug expiry/wraparound/capacity, projection and occluded selection.
- Format (402 files), boundaries (367), types (366), pinned vendor hashes and owned
  whitespace pass. Tidy passes 1,162 production configurations. Lifetime analysis
  passes 1,116 compilation commands/118 source paths and all controls.
- Shipping SHA-256 remains 427e37beb867d2164294fe70f99d5bf1bf0eddcb768f3f7786cf721747f1ccfd.
  Fixed Q3 demos replay twice including video restart, matching frame projection
  43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
  Accepted fixtures, goldens and generated shaders remain unchanged.

Historical gates: test-first 7596afa9 fails on unchanged 30eeba4c because the
explicitly enabled build has no ImGui symbol (devtools-before.log). Initial slice
passed build 35492532711; its runtime exposed the test's OA/Q3 native-object
mismatch, corrected to the pinned OA objects. The next inspector runtime exposed
null inactive material stages, corrected in the new getter and covered by probes.
Profiling e24faa49 passed build/regression 35493250065/35493250038; animation
245397aa passed 35493639523/35493639540. Entity 9aca1c9f passed regression
35494279424 but failed build 35494279398 on floating from_chars; fixed above.
The latest test symbol assertion also now recognizes the drawing API's C linkage.
The retained-context input transition also has a failing-then-passing real-input
check (1899ff9d and devtools-reopen-before3.log). This corrects the new overlay,
not a pre-existing engine behavior. No existing engine bug fix was included in #7. Self-review: every change supports
#7; no new OS calls outside platform/filesystem ownership; no non-trivial core
lifetimes; no new shipping/per-frame game allocation or simulation arithmetic;
existing wire/file layouts remain asserted; new UI records have layout/copy
assertions. The optional UI's bounded arena and interaction allocations are
explicitly measured/documented. Final review additionally fixed retained-context
input capture (test 1899ff9d, fix 7f5d60f6) and saved angle aliases (test d115d52e,
fix 86e9d19e), each with failing-then-passing real interaction/save tests. Head
86e9d19e passed build 35496813498/regression 35496813511; PR #143 merged e4f2d70a.
The earlier f895997d runner shutdown was retried successfully. Merged-tree run
35497810031 passed; #7 is closed and #25 marks it complete.

## #142 test-first checkpoint (engine unchanged)

Issue/design read; the existing shipping binary is retained in
`render-graph-baseline/quake3e.x64` in the persistent modernization cache.
`render-graph-notes.md` records the traced pass/resource sequence and bounded
proposal. Preserve creation order separately from execution order; screen-map
contents may cross frames, and main/capture outputs are exported to the existing
readback path. No aliasing, pass reordering or extra waits.

`tests/render_graph.py` first observes production image/render-pass/framebuffer
creation without a GPU or window. The initial 17-case cache preparation was expanded
to 36 cases before freezing the test: direct bloom/stencil flags affect descriptors,
and screen-map MSAA is independent of main-scene MSAA. GCC and Clang/libc++ traces
agree. Frozen native trace SHA-256:
`962a3b48d9c358bde23fe52e9cb15dd9688b8c8efc083e363a2500c4680f3ddb`.
It records formats, load/store/layout/dependency fields, attachment wiring,
dimensions and allocation order. Fake memory requirements observe packing, not
hardware memory use. No accepted fixture/golden or engine file is changed.

The second probe requires explicit graph inputs/outputs, dependencies, bounded
counts, creation order and resource lifetimes, including persistent/exported
resources. On e4f2d70a the native reference passes and the graph probe fails to
compile because the API is absent (expected exit 1, render-graph-before.log).
This commit is the test-first checkpoint. Implement the public plain records and
portable compiler, then make native allocation/pass descriptors consume them;
retain exact native traces and frame gates. Add rhi to lifetime ownership when
introducing engine/rhi/*.cpp. The preceding merged-tree CI and this issue's own
gates must both pass before the next merge.

## #142 graph integration checkpoint

The portable compiler now builds fixed plain records once at target creation.
Native images, render-pass load/store/layout/dependency declarations and
framebuffers consume them through the existing allocator and handle ownership.
Both GCC and Clang/libc++ still produce the frozen 36-case trace, including the
legacy compatible-pass choice for paired blur framebuffers. No draw sequence,
shader, wait, pool capacity or accepted artifact changed. Review extended the
screen-map lifetime through post-bloom geometry; its added assertion failed first
(render-graph-screen-before.log) and passes after declaring that read. The rhi source directory
is now included in the lifetime gate. Static and renderer-module fixed Q3 demos
pass twice through video restart, retaining frame projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
Both builds pass private-Xvfb resize/hide/restore checks. Public-only stub,
upload/acquisition probes, format/types/boundaries and 1,166 tidy configurations
pass. Lifetime analysis passes 1,120 compilation commands/119 source paths
and all controls; final changed-source tidy passes all eight configurations. No command recording, waits, synchronization
or frontend arithmetic changed. Hosted acceptance remains; measurements follow below.
The first replay attempt reused a CMake directory belonging to another checkout
and was rerun in /tmp/aftershock-graph-demo.

Paired measurements are committed as docs/render-graph-measurements.json and
summarized in docs/design/rhi.md: five alternating real-clock replays per map/build;
whole-client wall medians 1.43 -> 1.37 and 1.38 -> 1.40 seconds; peak RSS
181412 -> 181008 and 215872 -> 215744 KiB; final main GPU samples 3.973 -> 4.001
and 4.837 -> 5.189 ms. No speedup claim. ELF text/data/BSS +944/+128/+3168 bytes.
The measured graph executable is 50274d80c40408004230bb86a37352eaecc90d6c0c8373df8a3d75f1574e4142.

Self-review: #142 only; no unrelated engine fixes, OS calls, core destructors,
per-frame allocation, simulation arithmetic, wire/file layouts or public renderer
ABI changed. Both creation-order mappings and attachment capacities are asserted.
Readback/persistent targets retain their old storage lifetime; command recording,
profiling markers and synchronization remain at their original sites. The fixed
graph is intentionally not an aliasing allocator or a reordered frame scheduler.
Exact-head build/regression and the subsequent merged-tree replay must pass before
closing #142. Continue with #9 after that acceptance, following #25.

## #31 Vulkan acquisition checkpoint

`tests/vulkan_acquire.py` runs the real frame method with controlled GPU callbacks
and exits at command recording, without opening a window or creating a device.
On e82eb43b the two valid result cases pass. Both timeout/not-ready cases fail:
recording is reached despite no acquired image (exit 1, expected error exit 42).
Evidence: vulkan-acquire-before.log. Engine code is still unchanged in the first
test commit. Decision: route these no-image statuses through the existing fatal
acquisition-error path; preserve success, suboptimal and out-of-date retry handling.
No fixture/golden changes are needed for valid rendering; no simulation change.
Test-first commit ea17a6ba records the failure. The engine correction accepts only
VK_SUCCESS/VK_SUBOPTIMAL_KHR before setting acquired state; other statuses retain
the existing error/retry paths. GCC and Clang/libc++ all four cases pass (the Clang
probe's error callback explicitly matches the noreturn attribute). Local video-
restart replay passes b38004b1. Format/type/boundary checks pass. AGENTS self-review:
one acquisition condition only; no new engine OS call, allocation, destructor,
layout or floating-point change. CI integration runs the new test on both unit
compilers. Head 7382d9af passed build 35484485400 and regression 35484485349;
PR #141 merged 61401e17. Integration regression 35484895454 passed.

## #6 implementation checkpoint

Self-review complete for the final extraction: changes match #6; the acquisition
fix remains the separate #31 PR #141. Frontend/backend headers are independent,
OS calls stay platform/filesystem-owned, lifetime checks cover static/modules,
wire/file assertions remain, and no simulation FP edits or per-frame allocations
were introduced. Original fixed shaders and accepted goldens are byte-identical.
Module ABI is 10; old modules require rebuilding. Device-loss status is exposed
while retaining the old continuation policy, not adding automatic recovery.
The private-Xvfb lifecycle gate tests resize and hide/restore, not desktop-WM
iconification. Mesa's exported native cache is 32 bytes, so no compilation-speed
benefit is claimed. Full current-head hosted gates remain required before merge.

Roadmap bookkeeping: #2/#4/#5 were still open despite merged PRs #50/#65/#66.
Confirmed their merged-tree runs 34937376623/34942323875/34949976994 passed and
closed those completed issues without repeating implementation. #25 now reflects
those completions and #8, and carries phase-two #142 after #7.

Final local acceptance: static/module lifetime analysis passes 549 configurations
(113 source paths), tidy 572. Five alternating fresh-process real-clock replays
per map/build are recorded in docs/rhi-measurements.json, with executable hashes.
Median whole-client wall time (including startup/software-driver work) is
1.43 -> 1.44 seconds q3dm17 and 1.40 -> 1.44 q3dm7; peak RSS medians are
181,628 -> 180,984 KiB and 215,920 -> 215,748 KiB. Final main GPU scope medians
are 4.053/4.913 ms; no baseline GPU scope exists. ELF text/data/BSS deltas are
+13,832/+80/+8,480 bytes. No performance improvement is claimed. Cache logs and
script: rhi-final-measure/, rhi-final-measure.py and rhi-final-measure.log.

The frontend move e5777895 passes build 35490205641/regression 35490205499.
Retirement 935ad6f3 passes full build 35490399717; regression 35490399721 is running.
No accepted golden/fixture/shader bytes changed. Phase-two render-pass graph scope
is explicitly carried by #142, scheduled after #7 and before Wave 2; #6 closes
only the thin-RHI extraction acceptance defined in its done-when criteria.

OpenGL retirement checkpoint (working tree): deleted engine/renderer and its
CMake source list/build selection. Vulkan remains static by default with optional
PC modules. The full build matrix now builds one backend per platform/config;
static/module lifetime and tidy configurations replace the two renderer configs.
Accepted golden files stay unchanged: frame comparison retains archived OpenGL
rows on disk and checks every Vulkan value. Evidence-policy controls pass.
Static and optional-module Q3 replay/restart pass. The Vulkan-only JSON projection
hashes to 43c52e51; every sampled pixel hash still matches the untouched accepted
golden (the former b38004b1 included archived OpenGL rows). Tidy passes 572
static/module configurations; format/type/boundary controls and explicit rejection
of obsolete OpenGL CMake selection pass. Lifetime and hosted checks remain pending.
Evidence: rhi-sole-{demo,module,tidy}.log and rhi-retired-config.log. GPU samples
from the concurrent build run are not used as a final performance baseline.
Full lifecycle regression
35489948344 passed. The frontend-move lifetime check passed 546 commands/137 paths
with its seven-object negative control (rhi-move-lifetimes.log).

Frontend directory checkpoint: hosted lifecycle head 55b3d261 passes full build
35489948399 and the regression runtime job, including OpenArena window resize,
hide/restore, fixed replay and pipeline-cache restoration. The remaining full
regression status is being verified. Moved 26 frontend files from renderervk to
engine/render without changing a byte; docs/rhi-frontend-move.json records the
source commit, both paths and each verified SHA-256. CMake/probe paths, lifetime
coverage and ownership docs now follow the split. GCC/Clang RHI controls, image
acquisition, format/type/boundary checks and Q3 replay/restart/cache all pass,
retaining b38004b1. Evidence: rhi-move-{gcc,clang,acquire,demo}.log in the persistent
cache. Lifetime analysis and hosted gates are running. No OpenGL deletion is
included in this checkpoint.

The first working slice moves uniform uploads out of `tr_shade.cpp` into the
backend through `engine/rhi/rhi_public.h`, without changing uniform generation,
alignment, descriptor ordering or frame slots. Diagnostics use a trivial public
record. The partial alternative stub compiles in every client configuration and
reports unavailable; it is not a completed alternative renderer. `tests/rhi.py`
checks public-only dependencies and production upload alignment/bytes/bindings,
capacity exhaustion and separate frame slots (GCC and Clang/libc++ pass).

Local baseline and post-extraction fixed Q3 replay pass with b38004b1, no fixture
or golden changes. Mesa 26.0.8 / llvmpipe LLVM 21.1.8, Vulkan API 1.4.335, 640x480
windowed, 32-bit textures, picmip 0, GL_LINEAR_MIPMAP_NEAREST. q3dm17 peak vertex
284 KiB / push 4,160 bytes / 67 pipelines / 183 descriptions / 1 image chunk;
q3dm7 120 KiB / 1,024 bytes / 60 pipelines / 214 descriptions / 2 chunks. Both use
pipeline world base 92. Capacities and allocation policies are unchanged. Host
wall times include startup, are informational, and are not GPU timing claims.
Cache evidence: rhi-baseline.log, rhi-uniform.log, rhi-contract.log and rhi-baseline/
in ~/.cache/aftershock-modernization. Hosted OpenArena measurements, remaining
resources/device/commands/timestamps, lifecycle gates and GL retirement are pending.

Texture slice: portable format/address enums and opaque texture handles replace
Vulkan image/view/descriptor fields in frontend records. Backend creation, uploads,
conversion, sampler selection and destruction no longer accept `image_t`. The
80-byte image record and handle offset 56 are asserted unchanged. All five format
and three sampler address mappings, descriptor binding and idempotent destruction
pass GCC and Clang/libc++ checks. Local fixed replay and video-restart lifecycle
pass b38004b1; all four plain-replay resource snapshots match the baseline. No
shader, pixel-conversion expression, allocation pool or upload barrier changes.
Legacy backend error exits still need the planned status-boundary extraction.
Evidence: rhi-textures.log, rhi-texture-lifecycle.log, rhi-texture-contract.log.

First slice b970de67: full build 35483352838 and full regression 35483352666
passed. Texture commit 401d8b74 is running build 35483644058 and regression
35483644047. Hosted OpenArena replay passed with its accepted
Mesa 25.2.8 goldens. Downloaded logs: rhi-openarena-baseline/. LLVM 20.1.2, Vulkan
API 1.4.318. oa_dm1 peak vertex 48 KiB / push 512 bytes / 67 pipelines / 216
descriptions / 2 chunks; oa_dm7 276 KiB / 1,792 bytes / 66 pipelines / 212
descriptions / 2 chunks. Both use world base 92, 8 MiB geometry per slot, five
samplers and two frame slots; staging is 2 MiB / 6 MiB respectively. These are
hosted measurements, not claims that OpenArena content is installed locally.

Command/status slice: indexed draw and render-pass ending are public RHI commands
with the same arguments and ordering. Frontend device/queue waits now receive
portable statuses and report errors only after the backend returns. The focused
production check covers successful, unavailable, out-of-memory, lost-device and
other failure returns, plus exact draw arguments and pass order. GCC and
Clang/libc++ pass; local replay retains b38004b1 (rhi-commands.log). Backend-internal
legacy error paths and initialization/presentation still require extraction.
Next: finish timestamp validation, then a separate #31 test-first correction
for image acquisition status before extracting the remaining frame/resource state. PR #140 remains a draft; nothing from #6 implementation is merged yet.

Timestamp slice (working tree): 32 bounded scopes per frame, separate query ranges
for the two frame slots, explicit begin/end commands and availability readback only
after the existing fence succeeds. Unsupported queues report no timings; no query
WAIT flag or new fence is added. Vulkan pass scopes preserve the original pass
commands, object labels and barriers. The GPU-free production check exercises
8-bit timestamp wrap, unavailable queries, capacity exhaustion and duplicate end.
Both GCC and Clang/libc++ pass. Fixed Q3 replay retains b38004b1 (rhi-timings.log).

GPU measurements must use the real clock: faketime also affects Mesa's software
query values. `tests/demo.py --measure-gpu` gates frames first, then measures two
separate Vulkan replays per map without faketime. Initial main-pass samples are
4.071/3.957 ms (q3dm17) and 4.892/4.828 ms (q3dm7), informational single completed
frame samples on lavapipe, not a before/after speedup claim. Host wall times include
startup. Measured x64 Vk_Instance grows 188,672 -> 192,104 bytes (+3,432 fixed CPU
bytes), each frame slot 320 -> 1,384 bytes. GPU storage adds 128 timestamp queries;
no per-frame CPU allocation. Evidence: rhi-timing-measurements.log,
rhi-timing-contract.log, rhi-timing-sizes.txt. Video-restart replay and real-clock timing also pass (rhi-timing-lifecycle.log).

Texture 401d8b74 full build 35483644058 and regression 35483644047 passed.
Command/status 2b43a0bb full build 35483786166 passed; regression 35483786169 is
pending final status verification. The implementation stays on draft PR #140 while the remaining RHI
boundary and acceptance work proceeds.

Pipeline description slice: engine-owned shader IDs, shadow/topology/depth modes,
cull mode and state bits now live in the SDK-independent public RHI header. The
cache key retains its 52-byte/4-byte layout and four-byte boolean fields. Find,
get-description and bind operations are public RHI entry points. Uniform layout
is renderer-owned (128 bytes, alignment 4; fog fields at offsets 64 and 112), and
uploads remain opaque bytes. No shader bytes, state values or arithmetic changed.
GCC/Clang contract checks, format/type/boundary checks and fixed replay pass;
pipeline/image counts retain their original values (rhi-pipelines.log).
Correction to the initial resource claim: q3dm7's vertex peak is 117 KiB, not
120 KiB. Rechecking timestamp checkpoint 21b44a9c before pipeline extraction also
reports 117 KiB in both replays, so the difference predates the pipeline move.
q3dm17 remains 284 KiB. Capacities and accepted sampled frames are unchanged.
Retain this measured difference; do not treat upload peaks as frame goldens.
Evidence: rhi-pre-pipeline-recheck.log and rhi-pre-pipeline-metrics/.

GPU scopes checkpoint 21b44a9c passed full build 35484267291 and regression
35484267381, including hosted real-clock OpenArena measurements. Prior command
checkpoint 2b43a0bb also passed full build 35483786166/regression 35483786169.
All #6 implementation remains unmerged in draft PR #140.

Platform boundary slice: client/renderer imports use opaque 64-bit instance/surface
handles, converted only inside the native backend/platform calls. REF_API_VERSION
is 9 so incompatible old renderer modules are rejected. The include gate rejects
Vulkan SDK headers outside the backend/platform. Static replay and GPU measurement
pass (rhi-platform.log); optional modules pass both maps/renderers twice including
video restart (rhi-module-demo.log), retaining b38004b1. The demo driver checks
executable symbols to distinguish module from static linkage. CI now exercises
this optional module lifecycle. Format/type/boundary and RHI checks pass. No shader,
fixture, golden, allocation, floating-point or scene export changes.

Pipeline checkpoint 11cbcddb passed full build 35484803600/regression 35484803591.
Merge checkpoint 18e4e887 passed full build 35484969696/regression 35484969698.
The #31 acquisition fix's integration regression 35484895454 also passed.

Frontend pipeline ownership: the built-in pipeline index table and its creation
routine moved from the Vulkan device to the frontend shader implementation. The
creation body is identical after ownership/capability identifier substitutions;
GPU objects remain private and the world-pipeline retention boundary stays at the
same call position. A trivial capability snapshot replaces direct frontend reads
of private device flags. The public stub implements both new calls. GCC and Clang
checks pass, including capability mapping/world retention; format/type/boundary
checks pass. Replay plus video restart retains b38004b1 (rhi-state-demo.log).
Real-clock main samples are 3.923/3.920 ms q3dm17 and 4.857/4.901 ms q3dm7,
informational rather than a speedup claim. No shader/fixture/golden changes.

Platform checkpoint e259fc0f is running build 35485551477/regression 35485551484.
Remaining frontend private accesses include frame state, buffer/descriptor binding,
sampler policy, transforms and diagnostics; remove these before the frontend move.

Frame/binding slice: portable frame-state queries, index-pool selection/uploads,
screen-map bindings, sampler replacement and format diagnostics now own the former
frontend device accesses. Sampler replacement preserves the wait/destroy/update
order and returns a failed wait before mutation. Descriptor slot values are
unchanged engine-owned constants. Diagnostic format strings are copied because
the legacy formatter shares a scratch buffer for unknown formats. GCC/Clang checks
cover index binding cache, upload overflow/resize scheduling, failed/successful
sampler waits and distinct format labels. Module Q3 replay/restart retains b38004b1
(rhi-frame-demo.log). Format/type/boundary checks pass; no goldens changed.

Hosted platform e259fc0f: full build 35485551477 passed; regression 35485551484
failed only the newly added combined OpenArena module/restart frame step. Static
OpenArena replay passed eeb218f3. Artifact comparisons locate differences only at
the HUD portrait/lagometer (same bounding boxes in both renderers), after the warmup
replay and restart. The pre-existing lifecycle test/README only establish restart
frame equality for Quake 3; OpenArena uses fresh processes. Correct the new hosted
module step to --modules without --lifecycle, so module linkage is checked against
the accepted fresh-process goldens. Keep local Q3 restart checks required. This
does not establish OpenArena post-restart golden equality. Evidence:
rhi-platform-runtime-ci.log and rhi-platform-runtime-artifacts/. No image regions
are masked or goldens replaced. Verify the corrected hosted step on the next head.

Transform/visibility slice: the matrix generator and modelview storage now belong
to the frontend; the generator body is identical after function/state identifier
changes. The RHI receives exactly 64 transform bytes. Bloom receives the frontend
restore matrix and retains the existing GPU command order; its final-frame path
skips restore after clearing the pipeline, as before. Visibility storage alignment
and readback are private, with the same one-frame delayed coherent read and no
extra wait. Vulkan device/world globals now live in vk.cpp. Modelview and built-in
pipeline reset points remain tied to renderer resource/context shutdown.

GCC/Clang contract checks verify exact push bytes/stage/offset and aligned visibility
reads. Replay/restart retains b38004b1 before and after global ownership moves
(rhi-transform-demo.log, rhi-transform-ownership.log). Format/type/boundary checks
pass. Real-clock main samples: 3.927/3.901 ms q3dm17, 4.890/4.903 ms q3dm7;
informational, not a speedup claim. No shader, fixture or golden changes.

Frame/binding checkpoint ea6209b5 passed full build 35486025063 and regression
35486025142, including both static and optional module OpenArena replay. This
verifies the corrected fresh-process module gate. Built-in checkpoint 1642187d
passed build 35485710232; its regression 35485710189 had the same newly introduced
OpenArena restart/HUD mismatch as e259fc0f, resolved by ea6209b5's test correction.

Vertex-stream slice: material and lighting attribute selection moved to the
frontend. The RHI receives up to eight plain stream records, selecting the existing
map or frame pool. It preserves upload order, 32-byte alignment, gaps in cached
binding offsets and deferred resize on exhaustion. No scene/shader/tess records
are read by the stream upload/bind operation. Scratch records are bounded stack
data; no heap allocation or capacity change. GCC/Clang checks cover static/frame
pool handles, masked gaps, exact copied bytes, overflow and the empty mask.
Replay/restart retains b38004b1 (rhi-stream-demo.log); format/type/boundary pass.
Real-clock main samples: 4.004/3.932 ms q3dm17, 4.919/4.888 ms q3dm7; informational.
Transform checkpoint 1a4c19b1 is running build 35486276366/regression 35486276356.

Raster/draw slice: viewport/scissor generation and indexed/static draw selection
are frontend-owned. The RHI receives plain raster records and an explicit fallback
texture; it no longer reads tess or the white-image scene record to submit draws.
Raster arithmetic is unchanged; native rectangles/viewports are constructed from
the portable values. The existing depth-range/scissor cache and descriptor-before-
viewport command order are retained. Raster records are currently computed at
each draw preparation, even if the backend cache avoids GPU state commands; this
is bounded stack work, not a heap allocation or a claimed CPU optimization.

GCC/Clang checks cover viewport/scissor values, cache/invalidation, and no state
commands after upload exhaustion. Replay/restart retains b38004b1
(rhi-raster-demo.log); format/type/boundary checks pass. Real-clock main samples:
4.171/4.014 ms q3dm17, 4.806/5.045 ms q3dm7, informational. No accepted goldens or
shader bytes changed. Transform 1a4c19b1 passed build 35486276366 and regression
35486276356. Next extract frame command-list selection/submission inputs and finish
initialization/status boundaries before moving files or retiring OpenGL.

Frame input / SDK boundary slice: command-list screen-map selection and frame
bookkeeping moved to the frontend. Begin/end receive explicit screen-map, bloom
and capture decisions; duplicate stereo begin, skipped/overflow end, bloom outcome
and pre-present CPU timing retain their existing behavior. The backend no longer
reads backEnd, backEndData, tess or tr. Overbright configuration is explicitly
copied when post-process pipelines update and reused for swapchain recreation.
Its device/world records are translation-unit private. Remaining initialization,
resource and screenshot calls are declared through the public RHI; the frontend
no longer includes vk.h. Context retention still releases map resources without
calling device shutdown. Existing shutdown callers always destroyed the context,
so the removed shutdown argument was redundant.

The public stub covers the expanded API. Both compilers verify public-only stub
dependencies and GPU-SDK-free frontend/client headers. Local static frame/restart
passes b38004b1 (rhi-submit-demo.log); module frame/restart also passes b38004b1
(rhi-sdk-demo.log). The existing acquisition regression passes after its call-site
rename (rhi-sdk-acquire.log). No shader, fixture or golden change. Legacy backend
error exits are still present: public lifecycle declarations alone do not complete
the error-status requirement. Device configuration still uses shared renderer
configuration declarations; complete that dependency before claiming separation.

Raster d9292e90 passed full build 35486615563. Regression 35486615660 passed its
runtime and other jobs but failed tidy on the moved conditional-compilation draw
branch's indentation. Explicit braces/early return remove that ambiguity. The
complete local tidy gate now passes all 570 production configurations
(rhi-sdk-tidy.log); format/type/boundary and GCC/Clang RHI checks pass.

GPU error slice: fallible RHI calls return nodiscard statuses and a borrowed
fatal/drop diagnostic. Vulkan's internal abort is contained by a standard setjmp
scope with trivially destructible captures/locals; the previous scope is restored
on normal and error returns. Frontend R_CheckRHI reports only after return. The
cached pipeline bind avoids establishing a jump scope on the normal draw path.
This preserves the #1 engine longjmp decision without making optional MSVC modules
depend on the executable's private Q_setjmp_c assembly symbol. Decision recorded
on #6 comment 5747381615. Fixed diagnostic state adds 8,220 x64 symbol bytes
(excluding linker padding); no per-frame allocation or GPU wait is introduced.

The allocator review found recoverable Hunk_AllocateTempMemory errors inside image
conversion. Conversion moved unchanged to the frontend, which frees its scratch
before checking the upload status. The backend receives the already converted mip
chain and bytes per pixel. All five format byte checks and scratch release on both
success/device-error returns pass GCC and Clang/libc++. Descriptor allocation and
pipeline-capacity checks also verify returned status, fatal/drop diagnostic and
restored jump scope without invoking ri.Error. Existing acquisition checks pass.
Remaining zone allocator failures are process-fatal host services, not recoverable
GPU statuses; initialization's legacy frontend texture-mode callback remains for
the configuration extraction. This is not a completed device-loss recovery design.

Before conversion moved, both static and module Q3 restart replays passed b38004b1
(rhi-error-demo.log, rhi-error-module-demo.log); full lifetime (546 configurations,
137 source paths), tidy (570 configurations), format/type/boundary gates passed.
After conversion, focused GCC/Clang checks, all 570 tidy configurations, static
replay/restart and module replay/restart pass b38004b1 (rhi-error-conversion.log,
rhi-error-module-conversion.log). Conversion function comparison is byte-identical
after only portable enum/type substitutions. Real-clock main samples are
4.114/5.322 ms q3dm17 and 4.931/4.950 ms q3dm7, informational. The lifetime rerun
uses a checkout-specific cache after the default /tmp cache referenced another
checkout; no engine failure occurred in that configure attempt.
No shader, fixture, golden or simulation arithmetic changes. SDK checkpoint
bee85c84 passed full build 35487036651 and regression 35487036699.

Device/configuration slice: RHI initialization copies window/render/capture sizes,
latched settings and existing frontend resource limits. It borrows an output record
only during initialization and clears that pointer before return, including error
returns. Post-process settings are copied at the original update points; expression
order is unchanged. Swap interval and minimization remain live host callbacks at
the original query sites. Seven explicit host services retain allocator, log and
platform ownership. The backend now includes only its private GPU types, the RHI
contract and public qcommon helpers; no tr_local/tr_common/tr_public dependency,
renderer globals, cvar pointers or frontend texture-mode callback remains.

Texture filter parsing stays in the frontend. The backend retains the original
initial device wait/sampler setup position and later filter-update ordering. The
uncompiled alpha-to-coverage block is removed rather than exporting its absent
cvar as a live configuration input: an initial eager read in this working change
was caught by the startup replay and corrected before commit. The acquisition
regression and GCC/Clang RHI checks pass, including missing-loader status return,
copied settings and release of the borrowed output pointer. The dependency check
now rejects private frontend headers from the backend as well as GPU SDK headers
from the frontend. No new allocation, shader/fixture/golden or simulation change.

Local static/module replay/restart retain b38004b1 (rhi-device-config.log,
rhi-config-module.log). Real-clock main samples: 4.881/4.116 ms q3dm17 and
4.946/4.905 ms q3dm7, informational. Format/type/boundary and all 570 tidy
configurations pass. Additional before/after FBO/bloom/MSAA/16-bit-texture and
supersampled/render-scaled replays pass on both maps, including live gamma,
greyscale, bloom-intensity and texture-filter updates. All 12 sampled TGA files
match the retained pre-configuration binary byte-for-byte (rhi-config-parity.log,
rhi-config-parity/hashes.json records binary/frame hashes). These temporary
comparisons do not replace goldens. The full lifetime gate passes all 546
compilation commands/137 source paths. New persistent configuration/host/output-
pointer symbols total 208 x64 bytes (excluding padding); the initialization output
is stack-only, not a second persistent copy of frontend device strings.

Error checkpoint d65b28df passed full regression 35487920089. Full build
35487920102 failed only MSVC (all four configurations): the new noreturn helper
exposes legacy unreachable fallback returns/assignments (C4702). Remove those
unreachable statements; retain the noreturn contract and warning gate. Hosted
verification of this correction is pending the configuration checkpoint push.
The checkout-specific lifetime rerun for d65b28df passed all 546 compilation
commands/137 source paths (rhi-errors-lifetimes.log).

Configuration a198b032 passed full build 35488490079 and regression 35488490074,
including all four MSVC legs and hosted static/module OpenArena replay. This
verifies the C4702 correction as well as the frontend-free backend dependency.

Shader packaging slice: downloaded the official glslang 16.6.0 Linux tool into the
user cache (no system package installation). Its 74 outputs exactly match every
committed SPIR-V byte. A portable manifest now records the explicit variants and
verified source/recipe/output hashes. The build emits an aligned shader header,
interface metadata and SHA-256 package identity; unchanged sources reuse the
committed compiled cache, changed recipes require the pinned offline compiler.
Sources/includes, compiler/options/target, payloads and interface metadata all
participate in the key. No runtime source compiler is introduced. A CMake shaders
target forces compilation, while ordinary builds verify the cache. CI's GCC unit
leg verifies the release archive digest and recompiles every variant. Dedicated
server-only configurations do not acquire shader/Python requirements.

Fresh compiler/cache comparison and the CMake shaders target pass for all 74
variants with package hash 743e9c51f75547a6577119182c0363c5829f498e1e99c8672e057fad19c7e737
(rhi-shader-tests.log, rhi-shader-target.log). Include edits, compiler identity and
options change the recipe key. GCC/Clang contracts, acquisition, format/type/
boundary checks pass. Generated-header static replay/restart retains b38004b1
(rhi-shader-demo.log). Runtime driver pipeline-cache persistence remains the next shader
step. No accepted fixture/golden or committed shader_data.cpp changes.

Shader package 8e1fdb00 passed full build 35488951387 and regression 35488951430,
including fresh 74-variant compilation, all platform builds and hosted replay.

Runtime pipeline cache (working tree): frontend startup restores a cache before
creating material/post-process pipelines; teardown exports it before destroying
the device. The key records the complete shader package hash, vendor/device/driver
and native cache UUID. Filenames abbreviate the shader hash to fit MAX_QPATH, but
the stored full key must match. Vulkan also checks its native cache header before
restoration. Filesystem-owned helpers read only the home game directory (never
pk3 search paths), bound reads to caller capacity and check lengths/checksums.
Partial or incompatible data is a miss. Cache allocation is bounded at 16 MiB,
only at initialization/shutdown; no new per-frame allocation or GPU wait. This is
new #6 cache plumbing, not an unrelated engine bug fix. Two filesystem imports
advance the optional renderer ABI to 10; scene/game services are unchanged.

GCC/Clang contract checks exercise cache export/restore and copied compatibility
identity. Q3 restart replay passes b38004b1 and logs an actual cache restoration
(rhi-cache-demo.log). The first startup attempt exposed an omitted function-loader
entry in this new code; the loader entry was added and the final tree rebuilt
before that passing replay. All 570 tidy configurations and format/type/boundary
checks pass. Separate-process static and module/restart cache tests pass b38004b1
(rhi-cache-process.log, rhi-cache-module.log), requiring cache-load evidence in each
warm Vulkan replay. The lifetime gate passes all 546 compilation commands/137
source paths. Hosted static OpenArena now uses --pipeline-cache as well. On local Mesa the exported native cache is a 32-byte
header (124 bytes including the RHI key); this verifies persistence, not a measured
pipeline compilation speedup. Persistent Vk_Instance grows by 96 bytes including
alignment. Real-clock main samples: 4.016/3.899 ms q3dm17 and 4.846/4.893 ms q3dm7,
informational. Existing buffer capacities and accepted shader/frame bytes remain.

Pipeline-cache f075e4cd passed full build 35489385818 and regression 35489385893,
including hosted fresh-process cache restoration with unchanged OpenArena frames.

Lifecycle slice: tests/window.py starts a client on its own Xvfb display and
selects only that process's window. Actual 800x600 -> 640x480 resizing triggers
swapchain recreation; unmapping/hiding exercises SDL's engine-minimized path,
requires an FBO screenshot while hidden, then maps/restores and requires another
640x480 screenshot. Local real-clock lifecycle passes (rhi-window.log/window.log).
This is not an EWMH/window-manager iconification test or a new pixel golden.
Hosted CI adds x11-utils and runs the same check with OpenArena.

Presentation statuses now expose device loss after returning from the backend;
the frontend retains the existing developer diagnostic/continuation policy and
frame-slot advance. This relocates that policy, not an engine behavior fix or a
claim of new device-loss recovery. The existing RHI check covers hidden windows,
no acquired image, no submission, normal presentation/slot advance and device-loss
return/ownership. GCC/Clang checks pass. Updated fixed Q3 replay/restart/cache gate
retains b38004b1 (rhi-lifecycle-demo.log); format/type/boundary checks pass. No shader,
fixture, golden or simulation arithmetic changes. Current-head hosted gates are
pending; after they pass, hash-verify the portable frontend move.

## Final #8 verification

PR #138 merged as e4440d85 after current-head build 35479545347 and regression
35479545353 passed. Preceding merged-tree regression 35479365759 passed. The #8
merged-tree acceptance run is 35479878500; it must pass before the design PR merges.
Source commits: 09f418cc (final byte/layout declarations), af8c818a (Q_ASSERT),
5c34725f (cache read scalar spelling); final PR head 6be86089. AGENTS self-review and
measurements are on #8 and PR #138. #8's plan rows are marked in force.

The final layout audit includes all central/shared, image/cache and journal/browser/
routing records and both copies of the eight IQM records. WAV/browser/routing scalar
widths, ADPCM samples/indexes, bot characteristic tags, signed chat offsets and qsort
bytes are explicit. 169 sampled layout/byte objects: 135 raw/native-identical,
28 debug-only, six debug sign/zero extensions before CT_STRING equality; all 256
byte values produce the same equality result. The cache-read follow-up was refreshed
in all nine affected configurations. Evidence: char-object-review.json.

Q_ASSERT aliases the nineteen existing checks without changing conditions. Its
205 sampled objects preserve code/data (159 raw, 46 debug-only). Twelve native
helper binaries retain hashes. The permanent tidy driver passes all 570 production
configurations with assertions enabled and rejects increments, mutating calls and
nested increments in allowed pure helpers. Local unit/one-ULP negative control,
signed/unsigned-char chat checks, format/type/boundary checks and fixed Q3 replay
(b38004b1) pass. Evidence: assert-final-object-review.json, assert-final-native.json,
final-rules-{unit,chat,tidy,demo}.log in ~/.cache/aftershock-modernization/.

PR #137 integer policy merged as 50d6ee48 after build 35478967343 and regression
35478967384 passed. All 1,810 release configurations compile: 1,730 assemblies are
raw-identical and two GCC Vorbis cases assemble identically; 78 Windows configurations
change internal integer code/symbols and are not claimed identical. All twelve native
helpers match. Script arithmetic, seek/config-journal widths and hash overflow retain
explicit legacy platform contracts. Foreign stdio/curl/Vorbis/minizip/Xlib ABI types
are retained. The long ban checks inactive platform branches. The pinned OpenArena
adapter keeps its own original C seek signature; all three local static OA modules
build. Hosted OpenArena runtime and both-map replay passed; local OA assets were
absent and no missing-asset run was counted as passing.

PRs #132-#136 completed the one tree-wide clang-format commit, authoritative pinned
format gate, tidy readability/performance policy, and preceding fixed-width layouts.
Formatting preserved 1,810 release assembly instruction/data streams (eight inline-asm
source-comment differences reviewed), all nineteen native export assemblies and
helper behavior. The initial helper differences were only assertion source-line
immediates/build IDs. Original GPL hashes and accepted goldens/fixtures have never
been regenerated for these code-rule changes. All future writes stay in this repo.

## Historical verification checkpoints

The entries below record completed or superseded previews; follow Next action above.

Fresh cache-only formatted-central-width and formatted-enum-unsigned previews
preserve 56/85 sampled objects after stripping debug metadata; the shared-width
candidate also retains all twelve formatted helper hashes. Do not reapply stale
pre-format candidates. Refresh inputs if an intervening source edit touches them.

PR #133 published: source f0c9acc3/head 73090b12, build 35476070964 and
regression 35476070992 running. Hosted pinned formatting and clang-tidy jobs pass;
other gates remain pending. #132 merged-tree regression is 35476004982.
Applied record-width-preview (PR #134) preserves 159 sampled objects
(125 raw/native-identical, 34 debug-only) and all twelve formatted helper hashes.
Evidence: record-width-object-review.json, record-width-native.json. Its six files
cover 52 central file records and seven shared state/font records, explicit
uint32_t C++ trajectory enum, and a global type_traits include for native modules.
C99 reference declarations and all recorded sizes/alignments are preserved.

Further cache-only previews (not yet applied):
- image-cache-width-preview adds eight image/cache record assertions. All 55
  objects preserve code/data: 41 raw-identical, eight debug-only, six containing
  only reviewed allocation source-line immediates. The PCX header uses explicit
  uint8_t fields but preserves the existing host-char diagnostic interpretation;
  BMP/TGA decoded struct layouts stay 1080/20 bytes, PNG IHDR stays 16 bytes
  (its serialized fields occupy 13). No packing policy changes. Cache version zero
  retains explicit Windows/non-Windows widths and its existing platform signature.
  Evidence: image-cache-width-{object-review,line-review}.json. No new loader tests.
- formatted-assert-preview uses Q_ASSERT as an object-like alias of assert for all
  nineteen existing calls, with arguments unchanged. Its 205 object samples are
  159 raw/native-identical and 46 debug-only; twelve native helpers retain their
  hashes. Assertion-enabled Clang analysis passes 46 configurations and rejects
  both increment and mutating-call controls. Evidence: formatted-assert-*.
Both candidates include the record-width header baseline; compare inputs before
applying after the record-width PR. Do not reuse older pre-format candidates.

PR #131 verification: source 05eca339/head a296e69a, build 35474814153,
regression 35474814201 and preceding merged-tree regression 35474771300 pass.
All macOS legs pass with deprecation errors enabled. Twelve native helper/layout
builds remove only the two unused diagnostic exports; local fixed replay keeps
b38004b1. Original GPL hashes and accepted goldens remain unchanged.

PR #130 verification: source 0eb07ced, head dc005462. Build 35474393459,
regression 35474393491 and preceding merged-tree regression 35474358929 pass.
Four base/MISSIONPACK GCC/Clang release objects are byte-identical. Permanent
GCC/Clang-libc++ contract tests pass without warnings and enforce writable-string
errors. Original GPL hashes and goldens stay unchanged.

PR #129 verification: test a3652169, fix f3facd3a, head ef18757a. Build
35473984319, regression 35473984309 and preceding merged-tree regression
35473860807 pass. Twelve diagnostic paths pass with GCC and Clang/libc++ under
ASan/UBSan. Local smoke retains fea77580/14c8ee7d with the documented host-address
exclusion; fixed replay retains b38004b1. Original GPL hashes and accepted goldens
stay unchanged. Evidence: native-diagnostics-* artifacts.

PR #128 verification: test 1ef998b0, fix e8752ef1, head 901106f1. Build
35473480437, regression 35473480441 and preceding merged-tree regression
35473365622 pass. GCC/Clang-libc++ base/MISSIONPACK contract checks pass under
ASan/UBSan. Local smoke retains fea77580/14c8ee7d with the documented host-address
exclusion, and fixed replay retains b38004b1. GPL import hashes and goldens stay
unchanged. Evidence: team-message-* artifacts.

PR #127 verification: source 7c1db108/head 44616990, build 35472963400,
regression 35472963410, preceding merged-tree regression 35472915485 all pass.
Four cgame helper/layout builds remove only the two retired exports; 18 production
samples compile. Local fixed Quake 3 replay retains b38004b1. GPL hashes and
accepted fixtures/goldens are unchanged. Evidence: obsolete-print-* artifacts.

PR #126 verification: build 35472521203, regression 35472521080 and preceding
merged-tree regression 35472483857 pass. 195 compilation configurations and the
capacity comparisons pass. Fixed Quake 3 replay retains b38004b1; both bot-smoke
logs retain fea77580/14c8ee7d with the documented cache-only host-address exclusion.
No committed golden or test-normalization changes. Source head 08c56d57.

## Recent MSVC merges

PR #125 strict MSVC policy: head e273e6eb, merged ebd40d5b.
Build 35472155040 and regression 35472154997 pass after self-review; preceding
merged-tree regression 35472119435 also passes. Owned engine/game C++ now uses
/W4 /WX in all configurations; vendor warning policy and optimization are intact.

PR #124 C4711: source 3e6d66b8, head 43605a7d, merged 613c96b8.
Build 35471369036 and regression 35471369005 pass after self-review.
85 sampled objects preserve code/data (67 raw/native-identical, 18 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4711-object-review.json and msvc-c4711-native.json.

PR #123 C4514: source f7568d21, head 8c4d2192, merged 3f6484ca.
Build 35470941999 and regression 35470941980 pass after self-review.
92 sampled objects preserve code/data (82 raw/native-identical, 10 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4514-object-review.json and msvc-c4514-native.json.

PR #122 C4214: source 231355cc, head 231355cc, merged 08682b7a.
Build 35470571769 and regression 35470571759 pass after self-review.
92 sampled objects preserve code/data (92 raw/native-identical, 0 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4214-object-review.json and msvc-c4214-native.json.

PR #121 C4136: source 0c220839, head 9da7b602, merged 3b5ae1a5.
Build 35470242725 and regression 35470242736 pass after self-review.
85 sampled objects preserve code/data (75 raw/native-identical, 10 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4136-object-review.json and msvc-c4136-native.json.

PR #120 C4115: source fae6f1a7, head 478bb626, merged be3f2b32.
Build 35469905876 and regression 35469905875 pass after self-review.
85 sampled objects preserve code/data (75 raw/native-identical, 10 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4115-object-review.json and msvc-c4115-native.json.

PR #119 C4051: source d005bccc, head b5d49d95, merged 51366ebe.
Build 35469530847 and regression 35469530821 pass after self-review.
85 sampled objects preserve code/data (75 raw/native-identical, 10 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4051-object-review.json and msvc-c4051-native.json.

PR #118 C4032: source fc7de3a5, head e0c22d71, merged 21e4c41d.
Build 35469216318 and regression 35469216328 pass after self-review.
92 sampled objects preserve code/data (82 raw/native-identical, 10 debug-only); all twelve native helper hashes/layouts match the baseline.
Evidence: msvc-c4032-object-review.json and msvc-c4032-native.json.

PR #117 C4091: head ad768fe4 merged 981c534d. Build 35468827516 and
regression 35468827553 pass after self-review. Preceding merged-tree regression
35468824605 passes. 85 sampled objects (77 raw/native, 8 debug-only) preserve
code/data; twelve helper hashes/layouts match. Evidence: msvc-typedef-*.

## Historical #8 preparation (superseded)

These notes retain earlier experiments and their original context. The Next action
section above controls current work; do not reapply a candidate already merged.

Final formatting proof is complete against #131 head a296e69a. All 410 input
hashes matched before applying the cached result to the root worktree. The
original trailing-comment alignment oscillated in l_precomp and needed a second
pass in twelve other files; disabling trailing-comment alignment resolves this.
Evidence: format-comment-alignment-review.json and format-fixedpoint-experiment.json.
The pinned clang-format 21.1.8 package is available for hosted CI; no local
package installation is needed. Accepted fixtures and goldens remain unchanged.

Baseline rule: use post-warning-native.json (a296e69a) for upcoming proofs.
post-formatter-native.json and earlier width/assertion previews are historical;
refresh those candidates against the final formatted tree before claiming results.

MISSIONPACK constness preview: missionpack-const-preview makes only the read-only
Team_FragBonuses search-name pointer const and removes its two redundant casts.
G_Find already accepts const char*. GCC and Clang base/MISSIONPACK release objects
are byte-identical before/after, and writable-string warnings disappear. Evidence:
missionpack-const-preview/{changes,results}.json and compiler logs. Applied as PR #130 after the formatter fixes; full hosted gates are running.

Unused native parser-diagnostic preview: unused-parser-diagnostics-preview removes
the two unreferenced definitions/prototypes in native q_shared. All twelve
GCC/Clang C/C++ helper builds/layouts pass and exports remove exactly COM_ParseError
and COM_ParseWarning. Evidence: unused-parser-diagnostics-{native,symbol-review}.json.
Refreshed after the native formatter fixes and constness cleanup: pre-apple-final-
native.json is the before baseline; unused-parser-diagnostics-final-native.json
and its symbol-review.json record twelve passing after builds/layouts with only
the two diagnostic exports removed. Apply final-preview/changes.json with GPL
provenance and the final Apple deprecation ratchet after #130 merges.

Completed #31 native diagnostic-output capacity fix (PR #129): twelve active calls in g_main,
cg_main, ui_atoms and ai_main. The small-text capacity/routing probe fails all
twelve pre-fix contracts; the cached candidate passes 24 GCC/Clang ASan/UBSan
cases plus twelve Clang/libc++ cases. Evidence: native-diagnostic-before.json, native-diagnostic-contract.cpp,
native-diagnostic-preview/{changes,results}.json. Decision recorded on #31:
truncate diagnostic output to the actual buffer/remainder capacity, retaining
error routing and the existing seven-byte log prefix offset. Keep this separate
from PrintMsg's reject/error contract. Test-first commit, fix, GPL provenance and full gates are complete; see the
PR #129 verification record above. No oversized write is used in these probes.
Native COM_ParseError/COM_ParseWarning have no consumers (only definitions and
prototypes); their separate #8 deletion can retire the last unused diagnostic
formatters before removing the Apple warning disable.

Completed #31 PrintMsg formatter-result contract (PR #128). A small-text, cache-only probe
substitutes return values without any oversized write. The full-capacity result
incorrectly reaches message dispatch; valid/fitting/error cases behave as expected.
Evidence: team-message-contract-before.json and team-message-contract.cpp; issue
#31 records the finding. The test-first fix, provenance and gates are complete; see PR #128 above. Cached corrected candidate passes all
16 GCC/Clang base/MISSIONPACK contract cases under ASan/UBSan. Optional C++
MISSIONPACK compilation exposes three pre-existing writable-string warnings in
Team_FragBonuses; keep their constness cleanup in #8. No engine fix is part of #126.

Unused print-test service preview: obsolete-print-preview removes only the two
unreferenced services, their declarations and forwarding wrappers in five files.
All four GCC/Clang C/C++ native cgame helpers compile with unchanged layouts, and
symbol review removes exactly testPrintInt/testPrintFloat without added exports.
18 sampled production objects compile; code hashes change from function removal.
Historical CG_TESTPRINT enum values are retained so later service numbers stay
stable. Evidence: obsolete-print-{native,symbol-review}.json and objects/results.
Completed with imported-file provenance in PR #127; see its verification above.

Fixed-capacity formatting preview: numeric-format-preview replaces 43 sprintf
calls in 16 engine files with snprintf using the actual array or remaining
capacity. Formats and value arguments stay unchanged. The greyscale shader helper
receives its caller's array size; existing pointer outputs were traced to their
owners (filterDate, reliableCommands and the checked chat destination). Reviewed
numeric maxima, bounded source strings and the fixed country table fit the
existing buffers. 195 GCC/Clang/MinGW/AArch64 compilation configurations pass;
numeric-format-capacity.cpp confirms identical lengths/text at representable
numeric extremes, maximal bounded strings and every country table entry with
GCC and Clang. Applied on the active issue branch. The library call ABI
changes, so full runtime/replay gates are required rather than claiming identical
objects. PR #125 has merged. Arbitrary-length native diagnostic
formatters and the obsolete native print-test interface remain separate work.

Real MSVC record-width diagnostic: 96bbfb58 on the never-merged
issue/8-warning-inventory-current branch combines enum-unsigned and central-width
previews with the already-verified owned-source /W4 /WX policy. Run 35471127733
checks x64/ARM64 Debug/Release, both renderer builds and generated Visual Studio
projects. All four legs passed, with zero compiler C warnings and the three
previously reviewed Visual Studio vendor-flag D9025 notices per leg. Evidence:
width-msvc-jobs.json, width-msvc-review.json and width-msvc-*.log. This validates
the enum/layout candidate on MSVC but remains a diagnostic, not a final-tree gate. Two existing trailing spaces on changed weapon-field
lines were trimmed for git diff --check. Native GPL provenance belongs to the
later integration source commit, not this diagnostic-only branch.

Trajectory enum decision preview: prefer an explicit uint32_t C++ trType_t base,
with the C99 reference enum declaration retained. enum-unsigned-preview includes
the shared primitive-width/assertion edits and preserves all 85 sampled objects
(67 raw/native, 18 debug-only) and all twelve native helper hashes/layouts. This
matches the existing GCC/Clang enum representation; final MSVC/matrix verification
is still required. Do not apply enum-width-preview: its signed int32_t candidate
changed the GCC game/cgame helper hashes and is rejected in favor of the identical
unsigned candidate. No simulation expressions or accepted goldens changed.
Evidence: enum-unsigned-object-review.json and enum-unsigned-native.json. Refresh
this generator against the final warning/formatting tree before publishing.

Final formatter preparation: format-final-stringifying-macros.json adds standard
assert and Q_ASSERT to the existing 18 macro names whose tokens are stringified.
Use that 20-name list when refreshing the final formatter preview, preserving
assertion expression text as well as engine macro text. The older format-preview
and its measurements remain historical evidence and do not cover this expanded
configuration or the current warning tree. Rebuild the final formatting proof.

assert-tidy-preview verifies clang-tidy 21's bugprone-assert-side-effect with
assert/Q_ASSERT, function-call checking enabled, and an anchored allowlist for
__builtin_expect, Q_fabs, VectorLengthSquared, isnan and __builtin_isnan. All 46
Clang configurations containing assertions pass with -UNDEBUG; positive controls
pass and both increment/mutating-call controls fail as required. Evidence:
assert-tidy-preview/{config,results}.json and control logs. Integrate this check
with the later Q_ASSERT/tidy change; no new runtime abstraction is needed.

Cache-only assert-preview replaces the 19 active engine/native assert calls with
Q_ASSERT and defines the object-like alias `#define Q_ASSERT assert` in both shared
headers. Object-like expansion preserves standard assert argument stringification.
All 205 sampled objects preserve code/data (159 raw/native, 46 debug-only), and
all twelve native helper hashes/layouts match. Evidence: assert-object-review.json
and assert-native.json. Existing arguments were reviewed for side effects: only
read-only Q_fabs, VectorLengthSquared and isnan calls occur. No root edits yet;
refresh after the warning/formatting tree, record native GPL provenance, include
Q_ASSERT in the selected clang-tidy assert-side-effect check, and run hosted gates.

Cache-only shared-width-preview converts primitive fields in the seven shared
network/font records and handle aliases to exact-width equivalents, with matching
size/alignment/trivial-copy/standard-layout assertions in engine and native headers.
The game wrapper includes type_traits before entering module namespaces. All 85
sampled objects preserve code/data (67 raw/native, 18 debug-only), and all twelve
C/C++ native helper hashes/layouts retain the baseline. Evidence: shared-width-*
artifacts. This preview leaves trType_t's enum base unchanged; enum policy still
needs its own reviewed codegen proof. Refresh against the final warning/formatting
tree before applying; native GPL provenance and full hosted gates are still needed.

Cache-only central-width-preview explores explicit-width primitive fields and
size/alignment/trivial-copy/standard-layout assertions for 52 records in the
engine qfiles_public.h, iqm.h and aasfile.h headers. All 59 baseline GCC/Clang
record sizes/alignments/traits remain identical; 56 sampled production objects
preserve code/data (44 raw/native, 12 debug-only). Evidence: central-width-preview/
{changes,counts}.json, *-sizes.txt, central-width-object-review.json. This has not
been applied: refresh it after the final warning/formatting tree, review enum and
platform-cache policies separately, and run the full matrix before publishing.
The initial cache draft mishandled `unsigned short int`; its compiler check
caught that before any root source edit, and the corrected preview passed.

Session-only helpers quiet-warning-step.py, finish-quiet-warning.py and
quiet-warning-sequence.py automate the reviewed pragma-only C4032/C4051/C4115/
C4136/C4214/C4514/C4711 sequence. The sequence waits for each PR's build/regression
and its preceding merged-tree regression, checks the exact source/flag diff and
GPL hashes, records the self-review, and merges only on success. The sequence is complete through PR #124; no runner is active. Do not restart
it from the first class. Inspect quiet-warning-sequence.log when reviewing evidence. Every class still has its own branch/PR. C4514/C4711 retain compiler defaults.
The runner's PID/session is transient; the log and msvc-c*-published.json files
record PR heads, merge IDs and gates. Stop on any unexpected failure.

## Earlier warning evidence

Merged C4206 evidence: PR #116 head 8b50fc5e removes the engine suppression and
promotes /we4206. All 85 sampled objects preserve code/data (77 raw/native,
8 debug-only), and all twelve native helper hashes/layouts retain the baseline.

Merged C4220 evidence: source e883223d/head 9b143687 removes both varargs-matching
suppressions and promotes /we4220. No declaration/call/expression changes. All 85
sampled objects preserve code/data (67 raw/native, 18 debug-only), and all twelve
helper hashes/layouts retain the baseline. Original GPL hashes retain provenance.

Merged C4142 evidence: source 229ccb64/head be5a2878 removes both type-redefinition
suppressions and promotes /we4142. No declaration changes. All 85 sampled objects
preserve code/data (67 raw/native, 18 debug-only), and all twelve helper hashes/
layouts retain the baseline. Original GPL hashes retain native-header provenance.

Merged C4152 evidence: source fe91948f/head 0618ce14 removes both function/data
pointer conversion suppressions and promotes /we4152. No pointer expression changes.
All 85 sampled objects preserve code/data (67 raw/native, 18 debug-only), and all
twelve helpers/layouts retain the baseline. Original GPL hashes retain provenance.

Merged C4125 evidence: source fe512b6e/head a2bf708c removes both octal-escape
suppression directives and promotes /we4125. No string/parser expressions changed.
All 85 sampled objects preserve code/data (67 raw/native, 18 debug-only), and all
twelve helpers/layouts retain the baseline. Original GPL hashes retain provenance.

Merged C4057 evidence: source 769685fb/head f11e7913 removes only both inherited
pointer base-type suppressions and promotes /we4057 on owned C++ files. No pointer
conversions or expressions changed. All 85 sampled objects preserve code/data
(67 raw/native, 18 debug-only), and all twelve helpers/layouts retain the baseline.
Original GPL import hashes retain the native-header transformation. No arithmetic,
allocation, OS access, lifetime, layout, fixture or golden changes.

Merged C4100 evidence: source b4696ba0/head 7dedb706 removes only both inherited
unused-parameter disables and promotes /we4100 on owned C++ sources. The #84
annotations are unchanged. All 85 sampled objects preserve code/data (67
raw/native, 18 debug-only), and all twelve helper hashes/layouts retain the
baseline. Original GPL import hashes retain the native-header transformation.
No arithmetic, allocation, OS access, lifetime, layout, fixture or golden changes.

Merged C4018 evidence: source 464d4faa/head 768cc133 removes only both inherited
signed/unsigned suppressions and promotes /we4018 on owned C++ sources. Source
fixes landed in #90. All 85 sampled objects preserve code/data (67 raw/native,
18 debug-only), and all twelve helper hashes/layouts match post-formatter-native.json.
Original GPL import hashes retain the native-header transformation. PR #109 passed
build 35464448737 and regression 35464448740; merged aaef418c. No arithmetic,
allocation, OS access, lifetime, layout, fixture or golden changes.

A read-only preview removing all
16 inherited active MSVC pragma classes preserves all 70 sampled non-MSVC
preprocessor streams: every directive is inside an MSVC-only conditional. Evidence:
msvc-quiet-preview/classes.json and msvc-quiet-preprocess/results.json.
Refreshed real MSVC inventory at a83363a2 (run 35463911574) has zero owned-source
warnings across x64/ARM64 Debug/Release with all inherited header suppressions
exposed and /W4 active. Evidence: v2-msvc-*.log and v2-msvc-warning-unique.json.
No batch removal was applied; each class still requires its own hosted gates. Microsoft documents [C4514](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4514?view=msvc-170)
and [C4711](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4711?view=msvc-170)
as off by default (C4711 is informational). Decision: remove their legacy disables
individually, retain compiler defaults, and do not promote optimizer reports or
enable /Wall merely to manufacture them.
Strict-policy diagnostic 8ed19afc passed Ninja and generated Visual Studio builds
in run 35466504710, but review found its fallback source warning levels overrode
vendor /w (D9025 and vendor warnings in msvc-strict-v1-x64-debug.log). Do not apply
that version. Revised preview 48ffcf53 removes the unnecessary fallback: /W4 /WX are
owned-C++ source properties, vendor /w stays untouched, and target-wide levels
are removed. Remaining non-C++/vendor build sources are resources/assembly.
The diagnostic branch is never merged; apply a reviewed policy-only change after
individual suppressions are gone. Revised 48ffcf53 passes all four x64/ARM64
Debug/Release legs in run 35466848347, both Ninja and generated Visual Studio.
There are zero compiler C warnings. Visual Studio emits three D9025 notices per
leg when vendor /w overrides its default /W1; the regular Visual Studio baseline emits the same three
notices for /W3 -> /w. The regular baseline also emits 144 such Ninja notices;
the revised policy emits none in Ninja. Vendor suppression is preserved. Evidence:
msvc-strict-v2-*.log, msvc-strict-baseline-x64-debug.log, msvc-strict-v2-review.json.


Merged C4701 evidence: debug-reach-v2-preview initializes missing reachability in
the existing DEBUG-only else while keeping printing conditional. All 17 syntax
configurations and 13 raw/native-identical release objects pass. Six debug objects
change one conditional branch destination around the existing zeroing operation;
other normalized instructions match. Real MSVC x64/ARM64 Debug/Release diagnostic
run 35463911574 at a83363a2 passes with C4701 promoted. Repeated before/after debug
bot smoke (bot_developer=1) matches 9fd54408 after loading-time normalization; it
exercises developer diagnostics but did not emit the rare missing-goal message.
The earlier combined-guard preview still warned in MSVC run 35463680431 and must
not be applied. Artifacts: debug-reach-v2-*; no accepted golden changes.

Merged C4702 evidence: PR #107 source 6bd5c657/head 2c6f02c5 passed build
35463628573 and regression 35463628559; merged 937d7cee. All 151 syntax checks,
four game helper hashes/layouts and real MSVC diagnostic run 35462960543 pass.
Of 169 production objects, 127 are raw/native-identical and 24 debug-only; the
remaining 18 debug objects differ only in allocation source-line immediates, with
every other stripped byte identical (unreachable-line-review.json). Nine optional
MISSIONPACK comparisons compile with their pre-existing enum warning demoted
only for comparison: five release objects identical, two Clang release objects
share the lightning decrement/check, and two GCC debug objects move the same
increment onto its continuing edge. Ten-bounce limit and arithmetic retained.

PR #106 at bea6347c passed build 35462676712 and regression 35462676707; merged
c4047adf after self-review. The sole initial build failure was an MSYS mirror
package-signature timeout before compilation; its isolated retry passed.
All work and PRs remain in msetaro/aftershock.

The C4611 annotation is merged: only standard-MSVC Q_setjmp is annotated, the #1
trivial-lifetime gate remains, and C4611 is an error on owned C++ sources. All 28
local production objects remain raw/native-identical; real MSVC x64/ARM64 controls
confirm the bare call fails and the annotation passes. No exception-model change.

Applied the reviewed narrowing-v5-preview to 174 source/header files and promoted
C4244 on owned C++ sources, removing both inherited C4244 suppressions. Source
6709136c records the change; original GPL import hashes are retained with this
transformation attached to all 70 changed native files. Only inherited trailing
whitespace on 58 touched lines was trimmed after preview verification. PR #106 head bea6347c passes
regression 35462676707. Build 35462676712 passed all compiled legs; the MinGW
release leg failed before compilation while downloading a ccache package signature
from an MSYS mirror. Only that failed leg was rerun and passed. All twelve committed-tree
helper hashes/layouts match the post-formatter baseline (numeric-final-native.json).

All conversions remain at their original arithmetic boundaries; RHS temporaries
preserve compound-assignment evaluation order where needed. No dynamic allocation,
new OS calls, non-trivial lifetime or wire/file layout change. Accepted fixtures
and goldens are unchanged. One Vulkan error diagnostic now stringifies the explicit
(uint64_t) fence timeout cast; timeout value and normal rendering are unchanged.

The merged C4324 declaration preserves real MSVC x64 JPEG jump offset 176 and
record size/alignment 432/16; ARM64 remains offset 168 and size/alignment 360/8.
Six local production objects are raw/native-identical; three debug objects differ
by exactly one allocation source-line byte (207 -> 214), with every other stripped
byte identical. Artifacts: alignment-padding-* and msvc-jpeg-layout-*.log.

Next finish remaining warnings/Apple deprecations, format/tidy/layout/assert rules,
and #6 design only.
Read-only assertion audit (assert-call-inventory.txt): 19 active owned-code calls,
12 commented examples. Arguments contain no assignments/increments; the only
called helpers are Q_fabs, VectorLengthSquared and isnan, which read inputs without
changing engine state. For the later Q_ASSERT step, an object-like alias to the
standard assert macro can retain its existing expression stringification and
release behavior. No assertion edits or implementation have been applied; verify
release objects on the final tree before adopting it.

A design-only draft is prepared in cache/rhi-design-draft.md from issue #6 and the
current renderer contracts. It covers the thin static RHI, explicit ownership and
longjmp boundaries, preserved SPIR-V/replay hashes, a compile-only second backend,
and a later render-graph phase. Do not publish it as completed work or start RHI
implementation before #8 is complete; refresh it then into docs/design/rhi.md.
 All work and PRs stay in msetaro/aftershock.

Read-only layout inventory at 63615d82: GCC/Clang agree on sizes, alignment and
trivial standard layout for 59 central wire/model/BSP/AAS records. Cache evidence:
layout-inventory/results.json. This is a baseline, not completed #8 coverage; local
image records, platform cache records and native mirrored declarations still need
review. No source assertions/type changes applied by the inventory.
Additional read-only Clang Linux inventory at 7dedb706 records seven local records:
BMPHeader_t 1080/4, pcx_t 128/2, TargaHeader 20/2, PNG_ChunkHeader 8/1,
PNG_Chunk_IHDR 16/4, pk3cacheHeader_t 44/1 and pk3cacheFileItem_t 24/1 (size/alignment).
Evidence: local-format-layout-inventory/results.json. These include decoded CPU
records, whose padding need not equal the serialized byte count; preserve their
existing layout rather than packing them to match file headers. GCC class dumps confirm cache header/item sizes 44/24 on Linux and 40/12 on
MinGW Windows, all alignment 1; the named PNG/TGA layouts agree. Evidence:
local-format-layout-gcc/results.json. Preserve the platform-tagged cache layouts
when replacing long; a single universal width would silently change Windows data. This inventory only compiles existing
translation units; it invokes no parsers and adds no new test target.

A fresh diagnostic-only branch issue/8-warning-inventory-current at a05b6ccf
is based on 63615d82, includes the verified C4611 preview, preserves source line
counts while exposing inherited header diagnostics, and uses /W4 for Debug.
Never merge that branch/workflow; harvest its warning inventory for the next
C4244 class. All four inventory legs pass in run 35460712936: 1,734 unique
C4244 sites, 37 C4702 sites, and one C4701 site in debug bot diagnostics.
The latter two classes need separate review; the potentially-uninitialized warning
appears to involve repeated botDeveloper guards, not established runtime failure.
Current warning inventory and refreshed Clang AST evidence use current-msvc-* and
current-narrowing-ast* in cache. All actions remain inside msetaro/aftershock.
The refreshed AST covers 165 source files (one transient Clang crash in tr_noise
passed an isolated retry). Seven Windows/header files and inactive debug paths
need explicit coverage. First cache-only current-narrowing-preview: 1,667 casts
in 154 files; all 1,527 syntax configurations and twelve native helper hashes/
layouts match the post-formatter baseline. All 1,709 production objects preserve
code/data: 1,361 raw/native, 342 debug-only, six verified symbol-table/equivalent
unwind-record ordering/padding differences in be_aas_reach. Section bytes,
relocations, symbol values/sizes and normalized unwind records were checked.
Evidence: current-narrowing-object-review.json, current-narrowing-metadata-review.json.
Diagnostic-only 04daa92a passes inventory 35461209732 with 371 remaining C4244
sites (the AST estimate had predicted 361); never merge that branch.

Second preview narrowing-v2-preview adds scalar-macro conversions and 97 compound
assignments: all 1,611 syntax configurations pass. It exposed compound-assignment
evaluation/load-order changes and must not be applied. Third preview uses RHS
scalar temporaries at four native and 34 engine sites, restoring all twelve native
helper hashes. GCC/MinGW/AArch64 release refinements match; Clang still schedules
some engine operations differently, requiring source/codegen review and replay.

Applied candidate narrowing-v5-preview includes 43 local vector-macro expansions
that cast only each final component, explicit float conversion in mirrored
SnapVector declarations, and 87 Windows/header/inactive/typedef sites. All 2,380
syntax configurations pass; all twelve GCC/Clang C/C++ helper hashes/layouts match
post-formatter-native.json. All four real MSVC configurations pass diagnostic run
35462301210 at 972dcedb with zero C4244 warnings; C4702/C4701 remain separate work.

Full object comparison: 2,667 owned-source configurations, 2,243 raw/native-identical,
335 debug-only. Remaining 89 include debug scalar-temporary/symbol/unwind metadata
changes, Clang engine instruction scheduling/register allocation in seven source
files, and the Vulkan error string above (plus relocation offsets). No claim that
all objects are byte-identical. Evidence: narrowing-v2-objects/v5-results.json and
narrowing-v5-object-review.json. Original full arithmetic expressions and target
conversions are retained; full regressions are required by the #8 oracle rule.
Clang checks from the diagnostic tree pass unchanged Q3 frame golden b38004b1,
collision differential 9674cd22, and both bot-smoke goldens fea77580/14c8ee7d. The
smoke uses the already documented cache-only host-IP metadata filter on expected
and actual logs; gameplay output is unchanged. Artifacts: narrowing-clang-demo.log,
narrowing-clang-differential.log, narrowing-clang-runtime.log. No accepted fixture
or golden regeneration. Hosted CI will run the ordinary OpenArena gates unmodified.

All twelve GCC/Clang C/C++ native helper builds/layouts pass after formatter fixes
#99/#100; post-formatter-native.json at 4b159f7e is the cache reference for future
warning comparisons. Pre-fix #94 helper hashes are obsolete for that purpose.
No accepted fixture/golden was changed.

#97 local-shadow source 91a4341b preserves 19 production objects (15 raw/native,
four debug-only) and all four edited-tree cgame helper hashes/layouts. #96 global
shadow source af07f013 preserves 30 objects (24 raw/native, six debug-only) and
all four edited-tree game helper hashes/layouts. Original GPL hashes remain.

#95 merged-tree regression
35456678897 passes. #96 global-shadow source af07f013 preserves all 30 production
objects (24 raw/native, six debug-only) and all four final game helper hashes/layouts.

Source 91a4341b applies the C4456 local-shadow preview in three files: flat particle
width/height, fog pipeline definition, Vulkan result/memory/descriptor locals.
MSVC C4456 becomes an error on owned C++ sources. Nineteen production objects
preserve code/data (15 raw/native, four debug-only); four cgame helper libraries
retain preview hashes/layouts. All four final edited-tree cgame helper hashes/layouts match #94.
PR #97 passed all hosted gates and is merged. No FP expression, OS access, allocation, lifetime, layout, fixture or
golden changes. Artifacts: local-shadow-* in persistent cache. Record source and
GPL provenance, then hosted gates/self-review before merging.

#95 merged 5e343bd5: seven local alpha token renames preserve nine production
objects and four cgame helpers. Its generated-VS correction scopes promoted MSVC
warnings to owned C++ source properties; all 358 GNU/361 MinGW commands were
byte-identical. Add future warning promotions to that source-property list.

Completed size conversions #94: source 3eaba64d/provenance 36d419dd records 53
existing narrowing casts in 25 GPL files, with MSVC C4267 promoted to an error.
All 269 production objects preserve code/data (211 raw/native, 58 debug-only).
Nine helpers retain hashes; three GCC C libraries have reviewed equivalent
low-32-bit selections/subtractions, addresses and padding. All twelve edited-tree
helpers/layouts reproduce reviewed hashes. 1,584 before/after qsort cases pass
(4705a47e). Original expression evaluation precedes each cast. No FP expressions
or accepted fixtures/goldens changed. Artifacts: size-conversion-*.

Completed default-only switches #93: source f9dbcb07/provenance a5e141d9 removes
two AAS wrappers and one UI wrapper, retaining the exact unconditional statements.
MSVC C4065 is an error. Nineteen release objects are raw/native-identical; nine
debug objects differ only in no-ops and addresses, with branch target instruction
indices and all remaining instructions/relocations verified. Four native UI
libraries/layouts retain hashes. Artifacts: default-switch-*.

Completed standard offset #92: source cf4f6f1e replaces the DirectInput wheel
macro with standard offsetof and promotes MSVC C4644 to an error. All three
MinGW release/debug production objects retain raw/native hashes; hosted MSVC
x64/ARM64 debug/release pass. Artifacts: offsetof-preview and offsetof-objects.

Completed string constness #91: sources c08ed1e9/4a47cf0c qualify sixteen
read-only declarations/fields across thirteen engine files and enable GCC/Clang
and MSVC strict string checking. All 2,667 syntax configurations pass; all 578
production objects preserve code/data (310 raw/native, 214 debug/six approved
const-parameter manglings, 54 MinGW symbol-order-only). Fixed Q3 replay retains
b38004b1. The default local smoke failed only on rotated host IPv6 addresses;
a retained cache-only wrapper excludes exactly `^IP6?: .*` lines from expected
and actual logs, and both maps pass (fea77580/14c8ee7d). No harness or accepted
golden changes. Hosted OA runtime passes unmodified. Artifacts: write-strings-*.

Retained warning previews (global/local shadowing merged; engine-size applied):
- global-shadow C4459: four files, 30 production objects preserve code/data
  (24 raw/native, six debug-only); four native game libraries/layouts unchanged.
- local-shadow C4456: three files, 19 production objects preserve code/data
  (15 raw/native, four debug-only); four native cgame libraries/layouts unchanged.
Each preview has source changes, commands, objects and logs in persistent cache.
Engine-size C4267 preview: 160 diagnosed lines in 45 C++ files, plus removal of
one engine header suppression. The final preview preserves all 508 production
objects (401 raw/native hashes, 107 debug-only). Casts follow complete original
expressions; compound sums retain size_t arithmetic before the final conversion.
An early text-wide preview incorrectly narrowed a same-text size_t assignment;
the debug oracle caught it. Edits now address only diagnosed line numbers, and all
objects pass. The verified preview is now applied on this branch. Evidence: engine-size-*.
C4200 preview, now applied: remove the nonstandard trailing flexible
member from pcx_t, assert its unchanged 128-byte header size, and use the address
immediately after the header for its payload. Nine production objects preserve
code/data (seven raw/native, two debug-only). No parsing behavior changes or new
test target. Remove the header suppression and add owned-source /we4200 only in
its eventual #8 PR. Artifacts: flex-array-preview, flex-array-objects/review.json.
C4127 preview, now applied: literal true loops, false disabled branches,
constexpr endian check, and compile-time glconfig size checks; remove both shared
header suppressions. All 73 objects preserve code/data: 57 raw/native, 16 debug-only. The
ABI assertion keeps its original two-line span so debug allocation __LINE__ values
remain unchanged. The first constexpr-false branch preview
made HSVtoRGB unneeded under Clang; plain literal false preserves its existing
reference. MSVC's documented trivial-constant exemption covers this form; require
hosted confirmation. Microsoft reference:
https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
Artifacts: constant-condition-preview, constant-condition-objects and
constant-condition-review.json.
C4201 preview, now applied: name the transform and scaleOffset records in both
renderer texture-modifier unions, qualify their member accesses, and remove the
engine/GL header's C4201 suppressions. All 27 changed production objects preserve
code/data (21 raw/native, six debug-only); 18 before/after compiler configurations
confirm size 28, alignment 4, member offsets 4/20/4/12 and trivial standard layout.
Add owned-source /we4201 and run hosted gates when applying. Artifacts:
anonymous-struct-preview, anonymous-struct-objects, anonymous-struct-review.json,
anonymous-struct-layout/results.json. Do not overwrite later header changes from
whole preview files; apply the recorded replacement pairs.
Formatting preview: clang-format 21.1.8 touches 398 of 407 first-party C/C++/inc
files, excluding assembly and generated shader_data.cpp. Eighteen stringifying
macros are whitespace-sensitive. All 1,810 release assembly comparisons compile;
1,802 are raw-identical (including every Clang leg). Eight GCC/MinGW sys_runtime
comparisons differ only in source-position comments on inline cpuid instructions;
all fifteen corresponding release/native objects are byte-identical, and four
GCC debug objects differ only in debug sections. Nineteen native export assembly
comparisons also pass after including .inc files. Rebase the preview on the final
warning revision and reverify before the single formatting commit; do not apply
yet. Evidence: format-* and prepared-warning-and-format-evidence.json.
A cache-only clang-tidy inventory completes 570 configurations with no compile
failures for performance-*, readability (duplicate include, misleading indentation,
redundant control flow), and one advisory modernize check (redundant void args).
Raw findings include repeated headers: enum-size 18,464; void-args 54,215;
misleading indentation 51; redundant control flow 604; int-to-ptr 20; duplicate
include 8. Unique locations are in tidy-inventory/unique.json. Enum shrinking and
integer/pointer rewrites are not authorized by these suggestions; preserve layout
and codegen. No config/source changes for tidy yet; retain advisory reports and
review the readability checks after formatting before selecting error gates.

Completed signedness #90 evidence: source 70b1f0b6/provenance 4a96ca16 records
412 edits in 91 files (15 GPL files). All 2,667 syntax configurations pass. Of
905 production objects, 755 are raw/native-identical and 150 differ only in debug
sections. Ten native helper libraries retain hashes; GCC C game/UI differ only in
reviewed register reuse, independent moves and equality operand ordering in four
functions. All twelve edited-tree helper builds reproduce reviewed hashes/layouts.
The final helper warning freeze and its reader were removed. No FP expression or
accepted fixture/golden changes. Artifacts: sign-compare-* in persistent cache.

MSVC inventory from #90 release x64 job 105922991488 (msvc-sign-release.log):
C4267 234, C4459 38, C4456 28, C4065 15, C4457 3, C4644 3. Review each class
before enabling its error gate, then /WX. Local tools include clang-query-21.

Further warning audit: shared headers still contain inherited MSVC pragma
suppression lists (engine/qcommon/q_shared.h, game/bg/q_shared.h); the platform GL
header also leaks warning disables beyond SDK includes. The visible six-class
inventory is not the complete MSVC warning inventory. #94 covers game-side C4267;
the engine header still disables that diagnostic and needs a separate follow-up.
Diagnostic-only branch issue/8-msvc-inventory at 1f5b1398 runs the unsuppressed
/W4 x64/ARM64 Release/Debug inventory in 35455896498. Its worktree is in the
persistent cache; do not merge that branch or its inventory-only workflow. All
four inventory legs pass. Retained msvc-header-*.log and msvc-header-{inventory,
unique}.json record 1,686 C4244 file/line sites, 160 C4267, 14 C4127, four C4201,
one C4200, one C4324 and two ARM64 C4611 sites, plus the prepared shadow classes.
The Debug-only extra C4456 occurrence is in VK_CHECK and is covered by the
prepared macro-local rename. Apple deprecation inventory 35457048020 passes from diagnostic commit c8029349 on the
same unmergeable branch; MSVC inventory is not rerun. All Apple configurations
report only sprintf/vsprintf deprecations (61 unique call sites in 22 files).
Retained apple-deprecations*.log/json. Review buffer ownership/bounds before
changing calls; any actual overflow fix belongs in a test-first #31 PR. The inventory does not establish that currently
quiet legacy pragmas are obsolete; consult diagnostics and test each removal.
C4244 cache-only Clang AST inventory finishes 165 available source files with no
compile failures; 3,605 conversion/compound nodes include 896 macro expansions.
Seven Windows/header paths lack matching Clang commands, and Debug-only branches
need separate coverage. No casts are applied from this inventory; distinguish
existing narrowing from widening, preserve FP expressions, and inspect macros.
Evidence: narrowing-ast.py, narrowing-ast/results.json and per-file logs. The
source baseline is 222f7439; recompute byte offsets after engine-size changes.
Continue removing applicable header suppressions
by class before /WX; review obsolete C-only diagnostics and vendor-only scopes
separately. Do not claim unrestricted MSVC warnings yet. Preserve existing numeric
conversions, layouts and FP codegen; route actual behavior fixes through #31.

Next:
The two reproduced formatter capacity defects are fixed in separate test-first
PRs #99/#100. Tests/format.py is permanent, covers six compiler/language variants,
and runs in CI. Reproduction/fix evidence is in docs/bugs.md and format-/va-*
cache artifacts. Both fixes retain accepted replay goldens.

1. Finish C4201 hosted gates/self-review and verify #102 merged-tree regression.
2. Apply verified C4324/C4611 previews separately, then finish Apple deprecations
   and remaining MSVC warning classes /WX.

3. Finish one verified tree-wide clang-format commit, tidy subsets, fixed-width
   types/layout assertions and release-identical Q_ASSERT. Update plan rules to in
   force; finish #8, write design-only docs/design/rhi.md for #6, then stop.
   No #6/#7 implementation or accepted golden regeneration for warning changes.

Recent merges (all self-reviewed; merge commits):
- #70 ignored qualifiers: 17a9dae2, build 35016076778/regression 35016076784;
  merged 9d9dc4f6, merged-tree regression 35016803376 passed. The broad *.txt CI
  ignore was removed because it incorrectly excluded CMakeLists.txt.
- #71 chat sentinel: 98b457cc, build 35017297266/regression 35017297166;
  merged c04ba916, merged-tree regression 35018077437 passed; upstream #442.
- #72 unused functions: e6dfa0ba, build 35018151616/regression 35018151602;
  merged 01a1dda8, merged-tree regression 35018822894 passed.
- #73 internal declarations: 248dca68, build 35018897499/regression 35018897591;
  merged be2186e0, merged-tree regression 35019634702 passed.
- #74 type limits: 644741e9, build 35019711765/regression 35019711780;
  merged 5fb78853, merged-tree regression 35020339038 passed.
- #75 unused constants: da94649e, build 35020481804/regression 35020481815;
  merged b28beab5, merged-tree regression 35021218286 passed.
- #76 native helper internal declarations: 4c598f3e, build 35021362798/regression
  35021362817; merged 7f5f90b8, merged-tree regression 35022204641 passed.
- #77 native helper fallthrough: 2c17b152, build 35022303013/regression 35022302958;
  merged 28b89692, merged-tree regression 35023041315 passed.
#5 is complete. #8 warning ratchet remains active; later #8 rules are not done.

Completed PR #75 evidence:
- Source 68919a83 moves the order table/type/count in cg_servercmds.cpp under the
  existing MISSIONPACK guard with its sole user, retaining source line count.
  Original GPL hashes remain; the transformation is recorded in native provenance.
- Production GCC uses -Wunused-const-variable=2 because level 1 excludes source
  files included by the native namespace wrapper. Actual-wrapper negative control
  verifies level 2. Clang omits unused constants in included files; GCC enforces
  that case. Both compiler freezes are removed from tests/native-warnings.json,
  and the standalone helper explicitly enables the class.
- All 858 native production commands pass with this class treated as an error,
  covering GCC/Clang release, GCC debug, MinGW and ARM64. All 412 captured native
  GCC/Clang release objects retain identical raw hashes. Four standalone GCC/Clang
  C/C++ helper builds and ABI layout checks pass. Artifacts:
  /tmp/aftershock-unused-constant-builds and unused-constant-native-*.
- Affected MinGW native objects match after incremental LTO. Debug .text is
  identical; remaining .rodata is an exact suffix after 88 unused bytes, and all
  71 shifted references per object preserve referenced bytes. No function changes.
  MISSIONPACK preprocessed output is identical. Its unchanged compile baseline has
  an int-to-qboolean error at cg_servercmds.cpp:936; this unsupported configuration
  limitation is recorded in docs/bugs.md/#31, not repaired in the warning PR.
  /tmp/aftershock-unused-constant-preview/{results,review}.json.
- No FP expressions, OS access, lifetimes, allocations or goldens/fixtures changed.

Completed #71 evidence:
Test-first 36410f00 exercises actual BotFindMatch, BotMatchVariable and
BotExpandChatMessage: unsigned-char returns Q instead of empty for the -1 marker;
signed-char passes. Fix c58e2751 makes both engine/game declarations signed char.
GCC/Clang ASan+UBSan pass with both defaults, preserving 8/328-byte layouts.
Across 26 production objects: 12 raw matches, six MinGW native matches, six debug
objects differ only in debug sections, and two ARM64 objects change only four
chat-offset consumers. No added/removed functions or unrelated instructions.
Unit/collision regeneration was byte-identical (8d44421d/9674cd22); Q3 smoke
6dad7c18/a15c9c91 and fixed replay b38004b1 are unchanged. Upstream C f694bbbc
reproduces the failure and passes 98691272 in ec-/Quake3e #442. No expected-failure
entry/suppression applied. Artifacts /tmp/aftershock-chat-offset-*.

Retained #8 warning evidence and upcoming previews:
- Ignored qualifiers: 590 GCC client objects match; GCC/Clang flag controls pass.
  /tmp/aftershock-ignored-qualifiers-before.
- Unused functions: 364 Clang engine objects match and flag control passes.
  /tmp/aftershock-unused-function.
- Internal declarations: 206 Clang native objects match; actual-wrapper control
  rejects a function used only by decltype. /tmp/aftershock-native-warning-check.
- Type limits: 728 GCC/Clang engine objects match. Clang needs explicit -Wtype-limits;
  both controls reject unsigned < 0. /tmp/aftershock-type-limits-{control,check}.
- Parentheses-equality preview: remove redundant inner parentheses from the
  tournament test in g_cmds.cpp. Nine objects checked: release/native objects match;
  two GCC debug objects differ only in debug sections. Applied and merged in #78.
- Self-assign preview: replace fov_x self-assignment with a comment; retain the
  existing branch, arithmetic and distinct cg.refdef.fov_x assignment. All eight
  GCC/Clang/debug and MinGW native objects match. Applied on this branch.
  Both source previews have failing/passing Clang wrapper controls:
  /tmp/aftershock-small-warning-preview. Each needs its own PR and provenance.
- Null-pointer-subtraction preview: use (uintptr_t)a for qsort alignment and
  explicitly include stdint.h; retain the existing long-sized swap algorithm.
  All 25 production objects preserve instructions/relocations; release/MinGW native
  hashes match and six debug objects differ only in debug sections. Clang controls
  fail before/pass after. /tmp/aftershock-null-subtraction-preview. Applied here.
- Address/pointer-bool preview: remove the impossible !classname stack-array guard
  in BotGetActivateGoal; preserve existing empty-classname behavior. x86 release
  and MinGW native objects match; two debug objects differ only in debug sections.
  ARM64 swaps operands of one fcmp feeding b.ne. Review confirms equality/unordered
  results are symmetric; fallthrough immediately overwrites flags with another
  fcmp, and the taken path overwrites them with the stack-canary subs before any
  further condition reads. No source FP expression changes; all other instructions
  and relocations match. Full regression remains required for the eventual PR.
  /tmp/aftershock-address-preview. Applied on this branch.
- Formatting preflight: local clang-format is 21.1.8. VK_CHECK stringifies its
  argument, so its call whitespace must be preserved by the eventual formatter
  configuration. Allocator __LINE__ macros are debug-only. Do not start the single
  tree-wide formatting commit until the warning ratchet is complete.
- MSVC release inventory: C4267, C4459, C4456, C4065, C4457 and C4644, from #69 job
  104469265974. /tmp/aftershock-msvc-warning-inventory.log. Address before /WX.

## Completed affinity fixes and #8 baseline evidence

The entries below preserve historical evidence. Only the Next action above directs
resumed work; earlier next-action wording below describes its original checkpoint.

Hex test-first 985a3f1f extends the existing affinity test with exactly 0xZ and
bare 0x and enables ASan alongside UBSan. It fails on the wrong mask and terminator
read before the fix. Keeping hex_code's result in signed int until validation
fixes all 18 cases through the private helper and intercepted public apply path.
Both common.cpp callers and recursion were already reviewed. There is no real
OS affinity change in the test. Upstream C f694bbbc independently reproduces both
failures and passes the same hex-only fix under GCC/Clang ASan+UBSan; its ten cases
exclude the separate pending operator bug. /tmp/aftershock-affinity-hex-*.log.
No expected-failure entry/suppression covers this new regression.

Hex codegen review covers nine GCC/Clang/debug/MinGW/aarch64 production objects:
only parseAffinityMask changes; no function is added/removed and all unrelated
function instructions/relocations match. Artifacts /tmp/aftershock-affinity-hex-codegen.
Explicit unit/collision regeneration is byte-identical (8d44421d / 9674cd22).
Local Q3 bot logs and fixed replay pass unchanged (6dad7c18/a15c9c91, b38004b1).
Hosted build 34995107292 and regression 34995107235 passed; #69 merged 2d9fa2ca. No source FP,
wire/file layout, allocation or OS-call change; signed int hex is trivially
destructible. No golden/fixture change is intended or accepted for this fix.

Next #8 class: ignored qualifiers. The broad warning inventory reports no existing
diagnostics in this class, so only its CMake suppression needs removal. GCC and
Clang production-flag controls accept a const-qualified scalar return while the
suppression is present and reject it when enabled. Captured 590 current client
objects across both renderers at dc3a6a81 for direct before/after hash comparison:
/tmp/aftershock-ignored-qualifiers-before. No engine source or fixture change is
needed for that class. Keep it in a separate #8 branch/PR after #69 is ready.

Operator fix: preserve the + or - before recursive operand consumption. Test-first
e84a1f6e fails; all 16 valid cases now pass GCC/Clang UBSan through both helper and
public entry point. Common initialization and cvar-update callers were reviewed.
Upstream C f694bbbc fails the same probe (with its original Com_SetAffinityMask
name) and passes the same fix under both compilers. Explicit unit and collision
golden regeneration is unchanged (8d44421d / 9674cd22). No engine FP, layout,
allocation, OS call or lifetime change. No expectation/suppression applies.

#68 local gates pass: Q3 bot hashes 6dad7c18/a15c9c91 and both-renderer fixed
replay b38004b1 are unchanged. Nine production-object configurations retain every
function symbol; only parseAffinityMask instructions change. GCC/MinGW SnapVector
constant labels/offsets and ARM64 CPU-name strings move because the now-unreachable
empty string is omitted. Seven explicit byte checks verify the referenced constants
are identical. Artifacts /tmp/aftershock-affinity-codegen/{results,constants}.json;
/tmp/aftershock-affinity-{runtime,demo}.log. Scope self-review passes: one operator
bug, no FP/layout/OS-call/allocation/lifetime changes, no applicable expectation or
suppression. Build 34951709954 and regression 34951709967 passed; PR #68 merged b4db52c4.
Verified PR head: 83899a7de9e25ff2cfa6b2c105eb322ecad7a60d.

The separate hex-sentinel reproducer now confirms both symptoms: 0xZ becomes
UINT64_MAX and bare 0x causes ASan global-buffer-overflow. Temporary two-case
extension /tmp/aftershock-affinity-hex-probe.cpp; output
/tmp/aftershock-affinity-hex-before.log. Make this a separate test-first #31 PR.

#31 operator test-first checkpoint: tests/affinity.py compiles the actual private
parser/public apply implementation with UBSan. Sixteen valid expression cases
cover constants, 64-bit values, aliases and mixed operator order. The OS setter
is intercepted. The permanent test fails on the current source as expected
(/tmp/aftershock-affinity-test-first.log); both compiler CI unit jobs now run it.
This was the failing checkpoint before the operator fix.

#8 baseline: 2,380 production C++ objects/diagnostics across GCC/Clang release,
GCC debug, MinGW and aarch64 server configurations. Both renderers covered where
applicable. /tmp/aftershock-warning-before and /tmp/aftershock-warning-cross-before
contain compiler commands, raw hashes and logs. Six full compilation controls fail
with -Werror=implicit-fallthrough, including debug game and native Windows paths;
syntax-only compilation does not emit GCC's fallthrough diagnostic and is not used
as the gate. /tmp/aftershock-fallthrough-before-*.log records those expected failures.
Potential behavior bugs from other warning classes require #31 disposition; do not
correct them in this PR. No goldens/fixtures are regenerated.

#8 fallthrough implementation: six comments document existing transitions in file
append mode, preprocessor subtraction, SDL fallback settings, Windows key dispatch,
UI radio input and the debug game error path. Existing comments suffice for the
GCC warning and preserve the retained C oracle sources; no new portability macro
is needed. Source line counts are preserved so debug metadata can also match.
The warning suppression is removed from engine and native C++ compiler lists.
Unit golden 8d44421d and the one-ULP negative control pass. Object review passes: 2,372/2,380 raw objects match; the eight MinGW LTO
containers differ, but incremental LTO linking produces byte-identical native
objects (including data and relocations) for all eight. No LTO option is removed
from production. Thirteen initial Unix differences were unpinned __TIME__; the
focused before/after repeat uses the existing test SOURCE_DATE_EPOCH and all 13
match. Original hashes/diagnostics remain recorded. Review evidence and driver:
/tmp/aftershock-fallthrough-review*, /tmp/aftershock-fallthrough-results.json.
Original imports/notices/hashes are retained; provenance records a5c199bf.
Hosted gates and final self-review remain required.

First hosted #67 run found vendored minizip still inheriting engine warnings.
The CMake correction applies the plan's -w (/w on MSVC) vendor policy through
explicit existing source lists. Owned engine/game code retains the fallthrough
gate. Full local non-SDL debug client/server build passes; vendor comparison across nine configurations passes: 433/577 raw objects
match and all 144 differing MinGW LTO containers produce byte-identical native
objects after LTO linking. Artifacts /tmp/aftershock-vendor-warning-parity and
/tmp/aftershock-vendor-lto-review. No vendored source is edited.

Two affinity-helper bugs are confirmed by direct calls that do not apply CPU
affinity: valid 1+2 and 3-1 yield 1 and 3; 0xZ yields UINT64_MAX. docs/bugs.md
records the distinct operator-consumption and unsigned-sentinel causes with their
reproducer. Fix only in separate #31 PRs with failing tests first. Other warning
candidates remain unconfirmed and must not be silently changed during the ratchet.
The operator bug's temporary UBSan probe covers 16 valid numeric/alias/compound
expressions through both the private parser and public apply path. It fails on
compound expressions before any fix. Sys_SetAffinityMask is intercepted, so the
probe changes no process affinity. /tmp/aftershock-affinity-operators-probe.cpp and
/tmp/aftershock-affinity-operators-before.log. Copy this to a permanent test in its
own #31 branch and commit the failing test before changing engine code.

#67 self-review: one warning class; six comments preserve control flow and line
counts; explicit source lists scope vendor flags. No FP/layout/OS-call/allocation
or lifetime changes; accepted goldens/fixtures unchanged. Native provenance and
object/LTO review are recorded. Corrected hosted runs 34950827218 (build) and
34950827333 (regression) passed; #67 merged as 43ad68ab.

## #5 completed verification

Final source 9c170ddd passes full build 34948420894 and regression 34948420906.
Provenance checkpoint 3f91d501 passes build 34948630311 and regression 34948630289.
CMake-only retirement e67f397e passed build 34947650438 and regression 34947650429,
including hosted MinGW curl/zlib linkage and artifact staging. Migration checkpoint
390a20f4 passed every hosted raw-object and generated MSVC gate in 34946471284.
Embedded-debug 48733686 passed migration 34946795374, regression 34946795290 and
full build 34946795283; MSVC x64 debug reports 720/720 cacheable calls, 52 hits.
Native provenance records both shared GPL file transformations in 9c170ddd,
preserving original source hashes and notices.

Self-review: #5 build/platform scope only; no new portable OS access, non-trivial
core destructors, per-frame allocation or simulation FP expression changes.
Supported function bodies and raw-object differences are reviewed below. Existing
wire/file assertions remain; all 103 native layout/symbol gates pass. No accepted
golden or fixture changed. Regression probes use production CMake objects. Generated
MSVC and every required hosted compiler/configuration pass. Final review removes
one trailing empty CMake line; no command or source setting changes.

Makefile, game/modules.mk and handwritten MSVC projects are removed after parity.
CMake-only build.yml keeps Linux/macOS/Windows release/debug binaries and release
artifact jobs, adds Clang and caches, and builds generated VS projects. Its CRLF
convention is retained. Replay the retired migration oracle at 390a20f4. Release
workflow and install staging pass locally; staged Linux binaries match build
outputs byte for byte. Bundled macOS SDL uses @executable_path for adjacent staging.
The local optional MinGW curl configuration compiles but cannot link absent target
zlib; hosted MSYS installs zlib explicitly. No local packages or assets are copied.

Inactive-platform cleanup removes x86/ARM32/PowerPC branches, unused x87 state and
helpers, old 32-bit mixers and the unused Sys_ConfigureFPU hook. Three supported
Sys_SnapVector bodies and retained MSVC setjmp/longjmp/CPUID assembly are byte-
identical to their old bodies. CMake/header checks reject unsupported architectures,
32-bit pointers and big-endian targets. Configure negative controls for i686,
armv7 and ppc64le pass and run in CI. No active FP expression is rearranged.

After cleanup, 356/358 GCC Vulkan objects and 96/97 aarch64 server objects remain
raw-byte identical to the original baselines. Only unix_main objects differ.
Function/relocation review finds exactly the removed empty Sys_ConfigureFPU;
all retained functions have identical instructions and symbolic targets. ARM64
has one changed trailing alignment nop outside function size. Reports and driver:
/tmp/aftershock-64bit-functions*. All 103 native C/C++ layout/symbol gates pass;
the same 60 advisory outcomes remain. Boundary check passes 383 files (one retired
assembly-only header fewer). Unit/one-ULP, shared math/case, both Q3 smoke logs and
fixed replay pass unchanged. Both OpenArena sanitizer smoke maps and fixed replay
also pass unchanged. Lifetime analysis passes 546 commands/137 source paths and
seven negative controls. Artifacts /tmp/aftershock-64bit-*. Goldens/fixtures remain unchanged.


## #5 migration history (completed; pending statements below are historical)

Make reference checkpoint a08e7275 fixes reproducibility; CMake repair 6987587a;
test-helper migration f111b28c. Original baseline: 11 successful Make configurations,
3,757 raw object hashes and actual compiler commands under
/tmp/aftershock-cmake-before (source 4018c08a, engine identical to 4a952854).
Driver: /tmp/aftershock-cmake-baseline.py. All 3,757 local objects match CMake:
GCC release/debug OpenGL/Vulkan/dynamic, Clang+libc++ both static renderers,
MinGW both static renderers, aarch64 dedicated. No hash normalization.

Original MinGW -flto objects change hashes even with an identical repeated command
(/tmp/aftershock-cmake-lto-repeat.json). GCC records random section IDs and the
unmapped working directory with relative LTO locations
([upstream diagnosis](https://gcc.gnu.org/pipermail/gcc-patches/2022-November/606205.html)).
Both build systems now use the same per-TU seed, absolute source/include spelling
and stable absolute debug-source prefix; separate macro mapping preserves relative
__FILE__ strings. LTO and all object sections remain intact. Deterministic Make
references: /tmp/aftershock-cmake-mingw-repro-make[-opengl]. Focused proof:
/tmp/aftershock-lto-absolute.py. Debug assembly additionally needs its compilation
directory mapped; /tmp/aftershock-cmake-mingw-debug-parity now passes 361/361.
GCC debug dynamic also preserves compiler flag order for raw DWARF equality.

Repository oracle tools/port/check_cmake_parity.py builds both systems and saves
actual commands, raw hashes and explicit differences; its local GCC Vulkan gate
passes 358/358 (/tmp/aftershock-cmake-permanent-parity). Replay this oracle at the
recorded Make-retirement checkpoint after Make disappears. Candidate artifacts:
/tmp/aftershock-cmake-*; initial comparator /tmp/aftershock-cmake-compare.py.

Hosted source 6987587a passes existing regression 34945736264 and build
34945736251. New migration 34945736467 and follow-up 34946061800 prove raw parity
on macOS Intel/ARM64 and Linux GCC/Clang/native ARM64, release/debug, both renderers.
MinGW debug's single assembly difference is fixed locally as above; hosted rerun
remains. MSVC Ninja now compiles ARM64 (359/359 cacheable calls), but automatic
CMake manifest generation duplicates the engine's existing resource manifest.
Use /MANIFEST:NO to retain that resource, and correct x64 vcvars selection to
amd64 (ARM64 uses amd64_arm64). Generated Visual Studio build still needs to pass.
No source fix, warning suppression or dropped resource is involved.

Permanent unit/download/lifetime helpers consume CMake production objects and
compile_commands.json. Engine link instrumentation is target-specific so probe
entry points do not enter CMake's compiler-identification checks. Locally pass:
unit + one-ULP negative control; Clang+libc++; ASan/UBSan with known-bug classifier;
curl options/download; complete bot result; lifetime 546 commands/137 paths with
seven negative controls; both Q3 smoke logs; both-renderer Q3 fixed replay
(b38004b1); both OpenArena UBSan smoke logs; OA fixed replay (5b89d338).
Logs/artifacts /tmp/aftershock-cmake-{unit,unit-clang,unit-sanitized,download,
bot-move,lifetimes,runtime,demo,oa-runtime,oa-demo}*. Goldens/fixtures unchanged.

Regression workflow migration now adds job caches and uses CMake for cross-server
builds and the two existing Windows SDL interface compile checks. The latter pass
locally (/tmp/aftershock-cmake-sdl-cross). The supported build.yml is being switched to CMake after the migration gates;
preserve its original CRLF.
CMake keeps explicit source lists, strict native FP, precise MSVC engine FP, fast
MSVC release renderer FP and static CRT. MSVC ARM64 curl remains disabled as in
the old projects. External OA native objects remain static test inputs.


Correction checkpoint 390a20f4 is pushed. Prior f111b28c passed regression
34946061849 and the supported full build 34946061751. Its migration proved every
Linux/macOS compiler/configuration/renderer combination; only MinGW debug metadata
and MSVC environment/manifest steps needed the corrections above. New migration
34946471284 has already passed MSVC ARM64 debug, including generated VS projects;
other legs are pending. Runtime goldens and both fixed content replays pass locally.

MSVC debug's /Zi made all 359 compilation calls uncacheable with the installed
ccache 4.9; release is 359/359 cacheable. The build now selects CMake's Embedded
MSVC debug format (/Z7), keeping symbols in objects for ccache and final linked
PDBs. This changes debug metadata format, not optimization or runtime checks.
[ccache 4.9 option handling](https://github.com/ccache/ccache/blob/v4.9/src/argprocessing.cpp#L1152)
explicitly supports /Z7 and rejects /Zi. Hosted cache statistics must confirm the
change. Build instructions and compile-database documentation are being updated;
Make/handwritten-project retirement and inactive platform cleanup remain undone.


Embedded-debug source 48733686 passes migration 34946795374 on every leg. MSVC
x64 debug now reports 720/720 cacheable compilation calls, including 52 hits,
while retaining the generated VS build. Native Windows curl needs target zlib;
the local optional curl build compiles but cannot link -lz because that cross
library is absent. Hosted MSYS now installs its target zlib explicitly; the
local cross example uses USE_CURL=OFF, matching the verified cross configuration.
No local package is installed and the optional local curl link is not claimed
passing. CMake-only workflow/artifact staging still needs its own hosted run.


## #4 completed evidence

#4 evidence: baseline /tmp/aftershock-boundary-before has 355 production objects
per renderer on 239cbc34. Pure move 85381cda moves 757 files with identical Git
blob IDs, modes and SHA256; mapping /tmp/aftershock-subsystem-moves.json. Separate
path repair 8b5264c5 preserves 355/355 Vulkan and 355/355 OpenGL object hashes,
without normalization (/tmp/aftershock-boundary-after). The numstat display check
initially rejected binary '-' fields; independent blob/mode/hash checks verified
the moves. No accepted golden or fixture changed anywhere in #4.

Boundary source 52e57708 publishes client/sound/shared game/input interfaces,
moves clock/CPU/debug/AVI process operations to platform, and routes raw filter/bot
file operations through files.cpp. The 384-file include/OS check and controls run
in CI. Five public client operations replace sound's private client-state access.
All five Sys_SnapVector bodies, clock bodies and CPU-detection bodies compare
byte-identical with their originals. The AVI arithmetic/order is retained. Inline
platform debug wrappers preserve renderer ABI. Raw stdio adapters preserve return
values and encoding; /tmp/aftershock-boundary-stream-check.cpp exercised formatted
write, item counts, seek/tell, EOF, close and missing-file behavior against files.o.
Unused renderer2 and absent-tool parser integrations were retired per the plan.
Ownership: docs/subsystems.md. Bug records: docs/bugs.md. All 130 original GPL
source hashes remain verified; e24495b2 records import path transformations.

Local final gates: unit/one-ULP control; both Q3 bot hashes; both-renderer Q3
lifecycle replay (b38004b1); OA UBSan bot hashes and fixed replay (5b89d338);
103 native C/C++ layout/symbol comparisons; ALSA callbacks/thread joins and curl
transfer; GCC client/server and MinGW Windows client; lifetimes 546 commands/137
paths across both renderers. The four-command/one-path lifetime reduction from
#2 is raw net_ip moving into platform. Artifacts: /tmp/aftershock-boundary-*.

Hosted first build found Ogg/Vorbis $(ProjectName) include directories missed by
literal path repair. ffaa1ea7 repairs them; expanded include/library directories
were checked and all MSVC legs pass. Final review restored build.yml's original
CRLF bytes (normalized text exactly equals tested ffaa1ea7); no source change.

Self-review: one issue, public/OS ownership scope, content-only move evidence kept
separate from boundary changes; no new OS calls outside platform/filesystem;
no non-trivial core lifetimes or per-frame allocation; no simulation FP expression
change; existing wire/file layout assertions retained and native layouts match;
all goldens/fixtures unchanged. CI and local gates pass. PR #65 merged as 4a952854.

## Earlier #2 integration checkpoints (historical)

Active: issue/2-native-game, draft PR #50. #3, the recorded #31 fixes through
PR #61, and #1 are merged. #2 import, C/QVM parity, catalog port, advisory review,
and byte-identical .cpp rename are complete. Static engine adapters are still
scratch-only; repository integration, OpenArena static coverage and VM/JIT removal
are next. Do not rerun finished network work or regenerate accepted fixtures.

Game lifecycle test-first 413f1ed8 fails when native module storage survives unload.
Source d6c2ac52 restores per-level arena/bot/cache/counter state. Both persistent
restart/map-change repeats match DLL reloads (dd1fe5c3); the movement-debug variant
also matches (e87382ec). Existing Q3 native bot logs and all accepted replay frames
remain unchanged. Provenance 1591cc93 passed regression 34931875059; full build
34931874993 must still be checked.

Client lifecycle test-first a1cf3223 fails after fixed replay/video restart with
retained modules. The client now resets RNG/effect history, draw/loading/prediction
state and particle rotation at module init; UI resets its state, arena and server
cache counts. Existing menu structs already reset on entry. Both maps/renderers
now pass the ordinary-versus-retained transition comparison and the separate
accepted-golden replay (b38004b1). GCC/Clang C++ modules build; all 103 C/C++ layouts
and symbol comparisons pass, with advisory codegen reports retained. No FP
expression was rearranged, no per-frame allocation or OS access was added.
These resets implement #2's new static storage lifetime, not pre-existing fixes.

Current evidence: /tmp/aftershock-native-lifecycle-{debug,smoke,demo,gates},
/tmp/aftershock-native-client-lifecycle-{before,after,golden,clang,gates}.
Mutable-state audits: /tmp/aftershock-native-game-state-inventory.txt and
/tmp/aftershock-native-client-state-inventory.txt. Module wrappers must bind all
five compatibility functions (rand/srand/qsort/atof/memmove), and lifetime analysis
must cover their generated translation units. Base-game lifecycle was exercised;
missionpack is not an enabled imported-module configuration.

PR #61 merged 35a2c75c; merged regression 34930596490 passed. Its #2 integration
1def16ce passed regression 34930663280 and full build 34930663236. Both overlapping
info-removal helpers are fixed identically in q_shared.cpp; provenance retained.

Client source 436bbab1/provenance e40ac443 are pushed; regression 34932294456
passed and full build 34932294429 remains to check. Earlier full build 34931874993
passed too.

Working-tree static integration now replaces the six server/nine client dispatch
sites with typed calls. A single module.cpp wrapper compiles each imported source
in its module namespace, with all five compatibility functions bound locally;
there are no generated translation-unit files. The first actual Make dedicated
build links (/tmp/aftershock-native-integrated). Client build and bot smoke are
running. VM code is still linked but no longer serves these direct game exports;
its removal and test-suite adaptation remain pending. These integration edits are
not committed yet, and CI still describes the previous lifecycle checkpoint.

OpenArena preflight now compiles its pinned C sources with typed imports/exports,
combines each module into a relocatable object and prefixes its internal global
symbols using objcopy. The public typed exports remain visible. All three C objects
compile, and the game links against the same direct-call server; runtime/replay
parity remains to verify. This preserves a static hosted-content test executable
without importing OA implementation into production game source. Artifacts:
/tmp/aftershock-oa-static-preflight and its driver script. This approach still
needs permanent test/build integration, state-reset audit and verification.

Next: verify static Q3/OA bot logs and client fixed replay, integrate permanent
native builds/tests (including lifetime analysis), then remove the VM/JIT code.
Keep OpenArena coverage and accepted goldens; PR #50 remains draft until static
linking, VM/JIT removal, build/lifetime/layout/runtime/replay gates and self-review
are complete. The full later #4/#5/#8/design-only #6 sequence remains outstanding.

The integrated Q3 static server passes both accepted bot logs, both static client
renderers pass fixed replay (b38004b1), and lifetime analysis passes 562 compile
commands including 206 native commands across both renderer configurations. The
Clang AST log contains private state from game, cgame and UI, confirming all
wrapper source selections were analyzed. Engine builds use the existing fixed
SOURCE_DATE_EPOCH; a first standalone build omitted it and differed only in the
version date, then passed after rebuilding the two date-bearing engine objects.

Static OA C game bot logs pass both accepted hashes; static OA clients pass both
renderers and maps (5b89d33). Permanent openarena_native.py --static now builds the
same isolated C objects. Runtime/demo now use static modules by default and the
redundant former native-DLL CI pass is removed. OA runtime --sanitize includes its
C game object as well as engine code and is running. Test arguments --game-code
and --game-language are retired; C/C++ import compiler checks remain separate.

The permanent Q3 lifecycle gate now compares against the already-recorded DLL
reference logs (new native-lifecycle*.log files, dd1fe5c3/e87382ec), preserving all
existing accepted goldens. Static movement-debug restart/map-change passes twice.
Client lifecycle frames were also checked against the original frame golden and
match it, so --lifecycle now uses that existing golden directly. No extra frame
fixture or regeneration is needed. These integration changes are still uncommitted.

Earlier checkpoints below describe how this integration was reached.


Active work: issue/2-native-game, draft PR #50. PR #60 merged as 8e3ecf78 after
regression 34927341670/full build 34927341749 passed on 0366fa06. Test-first
02a9ddeb reproduces float-to-signed-byte UB. The three complete movement expressions
now explicitly truncate through int; GCC/Clang original C objects, layouts, symbols
and assembly are byte-identical before/after. All 18 command cases pass. Temporary
complete #2 native C++ UBSan bot smoke passes both maps with identical repeats and
accepted logs. Explicit unit/collision/Q3 runtime regeneration is byte-identical.
This integration retains T22 abs casts, float suffixes and all catalog edits.
The command test uses the complete local headers; the temporary header-fetch helper
is unnecessary here and removed, retaining #2's local-header team-leader test.

#59 merged 11781f44; merged-tree regression 34926638834 passed. Its #2 integration
and job-local pahole installation d8691015 passed regression 34926764924/full build
34926765230, including the 103-object native comparison. #58 merged-tree regression
34925562788 and #57 merged-tree regression 34924317093 passed.

Strict native warning freeze 7c4f8302 passed regression 34925774876/full build
34925774880. Both C/C++ native builds use -Wall -Wextra -Werror with only classes
observed in C. Provenance checkpoint e4853819 verified all 130 original GPL hashes:
30 verbatim files, 96 modified, four retained ABI headers. Per-file transformation
references identify native ABI/catalog edits and each separate #31 fix.

Permanent native_gates.py passes all 103 G2/G3 comparisons locally and in hosted
CI; it retains 63 advisory assembly diffs. --tidy completed all objects with 1365
narrowing, 55 signed-char and nine string-result findings; no tool/compile failures.
Latest local artifacts precede #60: /tmp/aftershock-native-gates-final; function
review: /tmp/aftershock-native-function-review-current and
/tmp/aftershock-native-review-checkpoint.md. Flag initialization/command-byte bugs
found during this review have now been fixed in separate #31 PRs. No unconfirmed
G7 warning is being called a sanitizer failure or hidden.

The resolved integration passes GCC/Clang command checks and the team-leader
check; ai_main.c is byte-identical to the successful complete native UBSan preflight.

#60 merged-tree regression 34927724919 passed. The resolved #2 integration
f92ae456 passed regression 34927833819/full build 34927833852.

The advisory G4/G7/catalog review and acceptance decision are committed in
58d86036 (docs/native-port-review.md) and recorded on #2. Diagnostics remain visible;
confirmed bugs are separately tracked/fixed.

The .cpp rename now preserves all bytes of 93 implementation files (git reports
100% similarity for every rename; /tmp/aftershock-native-rename-hashes.json records
SHA256 before/after). The manifest retains original upstream source paths/hashes.
C oracles explicitly select -x c. GCC/Clang C and C++ strict module builds pass;
OpenArena C module build passes. Math/shared, team leader/voters, base/missionpack
flags and bot command checks pass. No accepted fixtures/goldens changed.

Rename 634decac passed regression 34928509506 and full build 34928509462
(the single failed MSYS2 package-download job passed on retry). Checkpoint 526a77b7
passed regression 34928947087/full build 34928947114.

Static preflight remains scratch-only. Six server objects now use typed game
exports; 183 typed imports replace game syscalls. Both bot logs still match.
Nine client objects use typed cgame/UI exports with explicit call-depth counters;
94 cgame and 87 UI direct imports compile. Fixed replay on both renderers matches
all accepted frames. This is not yet repository integration or VM removal.

Additional restart/map-change checking found two distinct issues. Static linking
must reset module state previously reset by DLL reload: botstates points into the
reset game arena, and bot timing statics persist. A scratch reset clears those
pointers/counters and restores the restart portion, but the full state audit is
unfinished. Also the unchanged DLL reference itself corrupts info strings on map
change: original GPL Info_RemoveKey and Info_RemoveKey_Big use overlapping strcpy.
ASan reproduces strcpy-param-overlap in current q_shared.cpp. That existing bug
must be fixed separately under #31 before accepting map-change parity.

The separate #31 info-string fix is now merged as #61. Resume native lifecycle
reset and static integration,
retaining OpenArena coverage. Native module library audit must also bind memmove
inside each namespace: bg_lib defines it in addition to rand/srand/qsort/atof; the
first scratch wrappers omitted that local prototype. Do not claim complete static
parity until this is corrected and verified. No accepted fixtures/goldens changed.

Scratch evidence: /tmp/aftershock-native-static-preflight (exports, client-imports,
client-exports, reset). Scripts: /tmp/aftershock-game-direct-preflight.py,
/tmp/aftershock-native-direct-preflight.py, /tmp/aftershock-game-static-link.py,
/tmp/aftershock-game-export-preflight.py, /tmp/aftershock-client-direct-link.py,
/tmp/aftershock-client-export-preflight.py, /tmp/aftershock-static-reset-preflight.py.
Runtime/replay output: /tmp/aftershock-game-exports-runtime,
/tmp/aftershock-client-exports-demo. Restart comparisons:
/tmp/aftershock-static-restart.py, /tmp/aftershock-static-reset-restart.py,
/tmp/aftershock-static-reset-original-reference.py. ASan reproducer/log:
/tmp/aftershock-native-info-overlap. Full earlier native UBSan evidence:
/tmp/aftershock-bot-command-native.py/.log. No static engine edits are committed.

Completed #2 checkpoints: permanent OpenArena native build/smoke/replay d0013d95
(regression 34922352537 passed); portable Q3 binary32 literals 5592a1eb (regression
34922727256 passed). All 103 C and 103 C++ objects stayed byte-identical in the
literal conversion. Clang native Q3 smoke/replay and GCC/Clang native OA smoke/replay
match both maps/renderers. Three T17 ui_ingame casts also preserve C/C++ objects.
Full G2/G3 before the sentinel merge: 103/103 objects match. G3 adds artifact-only
-U__OPTIMIZE__ to the existing header/optimizer isolation flags; production assembly
differences are retained. The completed G4 review is in native-port-review.md; no
functions were added/removed across 103 objects. G7 on the temporary merged UI tree completed all
103 objects without tool/compile failures: 1365 narrowing, 55 signed-char and nine
implicit strcmp-result findings. The nine strcmp comparisons are equivalent nonzero
checks. The disposition in native-port-review.md retains inherited conversions for
#8 and routes confirmed defects through #31.
Artifacts: /tmp/aftershock-native-function-review, /tmp/aftershock-native-g2-g3-headers,
/tmp/aftershock-native-warning-inventory and /tmp/aftershock-native-tidy/results.json.
Earlier #54/#55/#56 merged-tree regressions 34913731858/34914963107/34915431579 passed.

#3 is complete (PR #33, merged-tree regression 34867621821 passed). The Huffman
alignment fix merged as PR #36 / bb4474db after regression 34868566671 and full
build 34868566674 passed; its merged-tree run 34869117306 passed.

Filesystem PR #37 merged as 73108eab after regression 34877286456 and full build
34877286443 passed; merged-tree regression 34877819880 passed.

Download PR #38 merged as 02b16def after regression 34878247137 and full build
34878247337 passed; merged-tree regression 34878892522 passed.

ALSA PR #39 merged as 811c6f7a after regression 34879358567 and full build
34879358584 passed; merged-tree regression 34879983585 passed.

Curl PR #40 merged as 998f21c3 after regression 34880567812 and full build
34880567796 passed; merged-tree regression 34881337473 passed.

ZIP PR #41 merged as 2a9436fe after regression 34885119593 and full build
34885119668 passed; merged-tree regression 34885592975 passed.

VM PR #42 merged as 62dad842 after regression 34886047536 and full build
34886047431 passed; merged-tree regression 34886485915 passed.

zlib callback PR #43 merged as fed755d6 after regression 34886950022 and full
build 34886949860 passed; merged-tree regression 34887353903 passed.

Extension output PR #44 merged as bdef99f4 after regression 34887856044 and full
build 34887856072 passed; merged-tree regression 34888464336 passed.

AAS PR #45 merged as 481d008a after regression 34889484418 and full build
34889484412 passed; merged-tree regression 34889992769 passed.

PNG PR #46 merged as f46f48c7 after regression 34890358367 and full build
34890358307 passed; merged-tree regression 34890928632 passed.

JPEG PR #47 merged as 9a7c2625 after regression 34891606879 and full build
34891606634 passed on source 01f2dd40. Its merged-tree regression 34892331846 passed. The disposition audit accounts for all twelve defects; CMake remains #5.
#31 is complete; all twelve fixes are merged and linked in the closed notes ledger.

#1 merged as PR #48 / 95b418b0 after regression 34892972996 and full build
34892972994 passed on e0013d90; merged-tree regression 34893585998 passed.

#2 exact GPL C import is committed/pushed on issue/2-native-game: b3ef1acd plus
checkpoint a937d710. Its 125 imported files and four retained ABI headers have
SHA256 provenance. Native preflight found an LP64 Q_rsqrt overread; no fix was made
on #2. The source is not wired into the engine yet.

Native math PR #49 merged as 2018564f after regression 34894597080 and full build
34894597081 passed on source 152cc6e2. Its merged-tree regression 34895239211 passed. #31 is closed again.

Native transition branch: `issue/2-native-game` (draft PR #50, checkpoint f4653398). Merging modernization retains the original
GPL import commit b3ef1acd and resolves the q_math add/add to the reviewed #49 fix.
The progress conflict is resolved to the latest #31 evidence plus this #2 state.
No history rewritten. Sources still compile only in temporary native preflight,
not the permanent engine build. Original headers match seven shared ABI sizes and
three offsets. Temporary native game/cgame/UI modules compile as C after pointer
entry-point adaptation and linking original bg_lib support; all 36 q3dm17 gameplay
events match the accepted QVM log. Full log differences are implementation-loading
metadata, compile date and bot-skill printf padding. Both-renderer fixed-demo
native preflight repeats identically but fails all accepted frame hashes: differences
range from 2 pixels to 931 pixels per sample. No fixture/golden changes. Added the
required ui_shared.h header verbatim with provenance (126 imported files now).
Temporary native ABI adaptation now passes every accepted frame on both maps and
renderers: binary32 literals plus float results at math calls reproduce the QVM
compiler model (lcc/src/bytecode.c declares float/double/long double all size 4).
GCC uses -fsingle-precision-constant; Clang accepts -cl-single-precision-constant
(the GCC spelling is ignored by Clang, verified with a literal-type assertion).
This is native ABI compatibility, not a new simulation algorithm: original FP
expressions and all accepted fixtures/goldens remain unchanged. Evidence:
/tmp/aftershock-native-demo-math-boundary.log, original frame hash b38004b1.
Permanent C native build/layout/smoke/replay commands are under development.
All 29 shared ABI sizes/alignments and three offsets match with GCC and Clang.
Native syscall arguments now use intptr_t words and explicit unused tail words,
matching the transitional DLL host's fixed vararg reads. The default QVM smoke
still passes both accepted goldens. Native q3dm17 passes the normalized accepted
log; native q3dm7 repeats but adds a Major chat line at the end, so parity fails.
The gate retains that failure and all gameplay text. Temporary engine tracing
shows both modes receive seed 140, run 1,494 frames, and finish at 74,900 ms.
Player states match through 42,150 ms; the first bot movement input difference
then precedes the differing state at 42,200 ms and syscall sequence at 42,300 ms.
Trace edits live only under /tmp, outside this branch's engine sources.
Permanent native demo replay now passes all accepted frames on both maps and both
renderers (/tmp/aftershock-native-demo-permanent.log, hash b38004b1). The server
trace identifies the blocker: BotMoveToGoal initializes only its first six fields;
the obstacle early return leaves movedir/weapon/ideal_viewangles untouched.
At 42,150 ms both modes pass identical movement state and goal, return blocked by
entity 167 with flags 32, then BotAIBlocked reads stale movedir. QVM has old stack
coordinates while native has different stack contents, causing different avoidance.
This is an existing engine bug, not native arithmetic drift. No inline #2 fix.
Current branch: `issue/31-bot-move-result`, based on modernization. #2 is safely
checkpointed/pushed as f4653398 and draft PR #50. #31 is reopened.
Next: a separate #31 failing-test-first PR initializes the
complete movement result and explains any resulting golden changes. After it
merges, resume permanent native smoke/replay parity and C++/static integration. No VM/JIT
removal before full native parity. Keep all new bugs in separate #31 PRs.

Clang runtime observation classified: VM_CallCompiled's instrumented indirect
call reads metadata at codeBase-8 before entering JIT code; the mmap allocation
starts at codeBase and has no preceding metadata. Disassembly confirms that load.
A temporary relink with only vm_x86.o built with -fno-sanitize=function passes both
original Q3 smoke goldens; all other UBSan instrumentation remains. No source or
CI flag/suppression changes were made. This is an instrumentation/JIT compatibility
limit of the transition oracle; #2 removes that JIT. The allocator callback bug
is independently covered by the permanent Clang unit test. The ASan/faketime
experiment still timed out before output and is not a claimed runtime gate.

## Issue status and remaining sequence

| Order | Issue | Status |
| --- | --- | --- |
| 1 | #3 regression suite | Complete: PR #33, merge 8692b422; historical network evidence remains unchanged. |
| 2 | #31 bugs | Recorded fixes merged in individual test-first PRs, including later native and warning-audit discoveries. Current evidence and reproducers are in docs/bugs.md. No further upstream submissions under the maintainer ruling above. |
| 3 | #1 error model | Complete: PR #48, merge 95b418b0; longjmp and the lifetime gate are in force. |
| 4 | #2 native game | Complete: PR #50, merge 6069cf2a; static C++ game modules, native/QVM parity evidence retained, VMs/JITs removed. |
| 5 | #4 boundaries | Complete: PR #65, merge 4a952854; verified moves, public include/OS gates, docs/subsystems.md and docs/bugs.md. |
| 6 | #5 CMake | Complete: PR #66, merge 1412c2eb; object parity, generated MSVC projects, 64-bit-only primary CMake build. |
| 7 | #8 code rules | Complete: warning ratchet, one formatting commit, tidy subsets, integer/layout policy and release-identical Q_ASSERT merged through PR #138 (e4440d85). Final evidence is above. |
| 8 | #6 design only | docs/design/rhi.md written; the sequence ends after its gated merge. #6 implementation remains future work. |

## Rulings in force

- The user's 2026-09-14 continuation supersedes the older roadmap order and handoff.
  Preserve fixed-timestep simulation, prediction/snapshots, cvars/pk3, arena/POD data,
  no per-frame allocation. No simulation FP restructuring except tested #31 fixes.
- C++20, no exceptions/RTTI, longjmp/trivial core lifetimes. VMs/JITs are removed;
  renderer modules are optional PC-only. clang-format 21.1.8 is authoritative.
- Local system packages must not be installed. Each CI job installs prerequisites.
  Proprietary paks stay outside git and uploads. Missing content/tools fail tests.
- Network coverage is finished. This thread ran `python3 tests/network.py --max-error 0`
  exactly once: exit 1, `FAIL: prediction bound exceeded`, measured 8.875 against zero.
  Do not modify or rerun that harness in this continuation.
- Normal demo checks replay fixed fixtures; recording is intentionally not reproducible.
  Golden creation/replacement is explicit, reviewed, and forbidden in CI.
- Engine/vendor bug fixes belong only in individual #31 PRs. No production source
  changed in #3. The independent scope adjustment on issue #3 remains in force.

## #3 acceptance evidence

Source/test/golden head: **98096f9710b29fb99464ac2464b356b7a28799cc**.

- Regression workflow **34866966536: success**. GCC, Clang/libc++, aarch64/mingw,
  sanitizer expectations, OpenArena collision/both-map smoke, and both real software
  renderer replay gates pass. The independently assigned job is non-blocking per
  the issue's scope ruling.
- Full build workflow **34866966514: success**, including Linux/macOS, MSVC x64/ARM64
  Debug/Release and Windows mingw. Skipped release-publishing jobs are inapplicable.
- Local GCC/Clang unit hashes and active Q_rsqrt one-ULP controls pass. Original
  accepted unit/collision/Quake 3 smoke goldens are byte-identical to d754d683.
  Local collision, both smoke maps, and final fixed-demo replays pass again.
- The sanitizer unit run reports HuffmanGetSymbol alignment as `known, tracked in #31`
  while comparing all thirteen groups. Unknown diagnostics, ASan failures, and stale
  expectations fail. `tests/check_known_bugs.py` verifies the classification policy.
  PNG alignment and JPEG table-index defects/reproducers are already on #31 and in
  docs/cpp-port-notes.md; they are not exercised by the unit driver.
- OpenArena uses oa_dm1/oa_dm7, Sarge/Beret, fs_game=baseoa, networking disabled and
  900 waits for combat. Hosted collision/smoke match locally generated goldens.
  Ubuntu paks contain native-module markers; tests/openarena.py adds official GPL
  oaxB52 QVMs with a pinned SHA256. Source/license/provisioning are in tests/README.md.
- Review rejected initial q3dm17 idle-player death-screen samples; final Quake 3
  fixtures follow a bot. Review also corrected the inherited opengl1 Make value to
  opengl: previous two-renderer claims were incorrect. Both renderer identities are
  now asserted; tests/check_frames.py rejects mislabeled logs and unequal repeats.
- Exact frame profiles use Mesa 26.0.8 locally and Mesa 25.2.8 on Ubuntu 24.04.
  No pixel tolerance or fallback. The hosted profile was generated LOCALLY with
  tests/frames.py --regenerate after visual review of run **34866295338** on
  **145939aa**. All 24 saved frames (12 samples, two repeats), renderer identities,
  and fixed fixture hashes were checked. CI never generated a golden.
- Golden-writing modes reject CI. AGENTS.md lists permanent tests/ commands.

## Self-review

#3 contains only tests/CI/docs and initial public-content/demo fixtures. No engine
or vendor changes, new core destructors, allocations, OS calls, simulation FP edits,
or layout changes. Original accepted goldens and port-evidence remain untouched.
README documents prerequisites, both content sets, provenance, exact profiles,
known-failure policy, and explicit fixture/evidence commands. Issue #3 and PR #33
record the measurements and corrected renderer finding.

## Local recovery paths

#3 worktree: `/home/matt/.t3/worktrees/aftershock/t3code-b3a8e505`.
The supplied workspace was at the integration baseline when this thread began.
`/tmp/aftershock-openarena-baseoa` is staged public content; original downloaded
packages were extracted under `/tmp/aftershock-openarena`, never installed.
`/tmp/aftershock-demo-tests` and `/tmp/aftershock-oa-demo` contain final local replay
evidence. `/tmp/aftershock-ci-runtime4/aftershock-demo-tests` contains reviewed hosted
baseline evidence. These paths are disposable; source and README commands suffice.

## #31 Huffman validation

Fatal Clang ASan/UBSan unit run failed before the fix at huffman_static.cpp:206,
exit 1; the same command passes afterward with no diagnostic/expectation.
Production GCC assembly and symbols pass the existing port gates before/after.
The explicit unit and differential regeneration commands produce zero golden
diff. Local q3dm17/q3dm7 smoke and both-renderer fixed-demo replay pass unchanged.
An upstream C regression tests every symbol at 32 bit offsets: sanitizer fails
before, passes after, and GCC C assembly is also identical. Upstream PR: https://github.com/ec-/Quake3e/pull/424. Fork PR #36 merged after full CI and self-review; merged-tree regression passed.

## #31 filesystem validation

The permanent `--sanitize --pointer-compare` command fails before the fix with
NULL versus filename+3, and passes afterward with the original unit golden.
Separate ASan/UBSan and ASan/pointer-comparison runs retain both checks: combining
them under Clang 21 instruments generated pointer-overflow comparisons in COM_ParseExt
(disassembly shows `__sanitizer_ptr_cmp(pointer, -3)`). No suppression/expectation
covered this bug. GCC/Clang/libc++ units, the one-ULP control, both sanitizer modes,
collision, both-map smoke and both-renderer replay pass. Explicit unit/collision
regeneration changes no golden. Symbol gate passes; normalized per-function assembly
changes only FS_AllowedExtension among 99 functions, as expected for the new guard.
Upstream C test fails before and passes after: https://github.com/ec-/Quake3e/pull/425.
Fork: https://github.com/msetaro/aftershock/pull/37. Regression/full build runs above
include public-content runtime and all platform legs. The caller audit's separate
Sys_LoadLibrary uninitialized diagnostic pointer is recorded in notes and issue #31.

## #31 download validation

Permanent `python3 tests/download.py` fails before on the trailing-slash base
(`maps//map%20name.pk3`), then passes with GCC and Clang/libc++ after the one-line
guarded last-character check. `%1` substitution, escaping and empty-base behavior
are retained. Explicit regeneration created only tests/golden/download.txt.
Existing unit/one-ULP, collision, smoke and replay gates pass. One local smoke
attempt overlapped replay and hit an occupied UDP port; serial smoke passed both
original map goldens. Run those local runtime gates serially. No golden changed
to accommodate the collision. The hosted runtime URL check uses existing libcurl
packages; begin/cleanup run without a transfer.
Upstream C fix/test: https://github.com/ec-/Quake3e/pull/426.
Fork PR: https://github.com/msetaro/aftershock/pull/38.

## #31 ALSA validation

Permanent callback assignments reject both original void(void) signatures. GCC and
Clang/libc++ pass after the pthread-compatible signatures/direct registration/NULL
returns. Real ALSA null output receives positive MMAP/DIRECT submissions and both
threads join within the timeout; no physical device or fabricated audio backend.
The dynamic-ALSA production object builds. Explicit unit/collision regeneration
produces no golden diff; serial both-map smoke and both-renderer replay pass unchanged.
Upstream C test/fix: https://github.com/ec-/Quake3e/pull/427.
Fork PR: https://github.com/msetaro/aftershock/pull/39. CI runs are above.

## #31 curl varargs validation

The permanent Clang test fails before at va_start and passes after changing the last
named argument to int and retaining a CURLoption local. Removed -Wno-varargs.
GCC/Clang local-file tests verify long/pointer/offset forwarding, body suppression,
private-data identity, bounded partial output on size-limit rejection and exact
returned bytes. No network transfer. Explicit URL regeneration changes no golden.
Normalized GCC codegen/symbol gates pass; the internal mangled name changes with
its parameter type, and all callers are in cl_curl.cpp. Unit/one-ULP, collision,
serial both-map smoke and both-renderer replay pass unchanged.
Fork PR: https://github.com/msetaro/aftershock/pull/40. CI runs are above.

## #31 ZIP validation

The existing GCC UBSan bot smoke fails with the original ZIP object and passes
with CopyLittleLong through a signed int temporary. Removed only the ZIP alignment
suppression. Q3 and OpenArena both-map goldens pass unchanged. Explicit unit/collision
regeneration produces no golden diff; normal serial smoke and fixed-demo replay
pass. GCC production symbol gate passes. Codegen gate reports only stack slots
64/68 exchanged with their matching sign-extending consumers; field destinations
and operations are unchanged. This reviewed difference is acceptable under #31's
codegen ruling; the gate is not weakened or reported as identical.
Upstream C UBSan startup on valid installed paks fails before and exits cleanly
after: https://github.com/ec-/Quake3e/pull/428. No new content or loader target.
Self-review: one packed-read bug, shared function covers all callers; no simulation
FP edit, layout change, allocation, destructor or new engine OS call. Runtime
sanitizer coverage reuses the existing smoke runner and compares existing goldens.
Regression 34885119593 passed on source 09e7cc96, including hosted GCC UBSan smoke.
Full build 34885119668 passed on the same source. Final checkpoint changes only
documentation; self-review passes and no goldens changed.

Next bug reproduction is ready without a source change: the same GCC runtime
with UBSAN_OPTIONS=halt_on_error=1 (no suppressions) exits 1 at vm.cpp:1181.
Evidence: /tmp/aftershock-vm-before-runtime.log. Follow with its own branch/PR.

## #31 VM validation

Permanent GCC UBSan runtime fails before at vm.cpp:1181, passes after the packed
read uses CopyLittleLong through int32_t. Removed the final alignment suppression;
tools/port/ubsan.supp is empty. All Q3/OA smoke goldens, normal serial smoke and
both-renderer replay pass unchanged. Explicit unit/collision regeneration has no
golden diff. Production symbols and all 26 function-section bytes match. Text
codegen differs only in the compiler switch-table label CSWTCH.89/90; unchanged
instructions/data reviewed acceptable, no gate edits. Upstream C reproducer
fails before and passes after: https://github.com/ec-/Quake3e/pull/429.
Self-review: one packed-read bug in the shared caller path; no instruction format,
FP, JIT behavior, allocation, destructor, layout or engine OS changes. No new
known-bug entry or test target; existing runtime CI is the permanent regression.
Regression 34886047536 and full build 34886047431 pass on source f320dee6.
Final checkpoint changes documentation only. Self-review passes.

## #31 zlib callback validation

Permanent Clang ASan/UBSan unit fails before on zcalloc's byte-pointer callback
type; correct void-pointer signatures and direct registration pass initialization
and cleanup. Separate pointer sanitizer mode also passes. The helper is linked
into the existing unit driver and uses its allocator stubs, with no content input.
No new runner/target or expectation/suppression. Explicit unit/collision golden
regeneration produces no diff; one-ULP negative control, normal/GCC UBSan smoke
and both-renderer replay pass. Production normalized codegen/symbol gates pass;
only private callback mangled names change. All callers are in unzip.cpp.
Upstream C regression fails before and passes after:
https://github.com/ec-/Quake3e/pull/430. Self-review: one callback ABI bug; no
allocation arithmetic, per-frame allocation, layout, FP, destructor or engine
OS-access changes. Regression 34886950022 and full build 34886949860 pass
on source 74b15fd6. Final checkpoint is documentation only; self-review passes.

## #31 extension output validation

Both platform diagnostic callers receive a defined extension on true returns:
the shared function now initializes its output before classification, using an
empty string for names without a dot. Versioned .so and rejected-extension strings
remain unchanged; the other callers only consume output on false returns.
Existing unit assertions fail before and pass after for empty, extensionless,
trailing-dot, ordinary, .so.N and pk3 cases. No library is loaded by the test.
Clang sanitizer/pointer checks, one-ULP control, collision, smoke and replay pass.
Explicit unit/collision regeneration changes no golden. Symbols pass; only
FS_AllowedExtension changes bytes among 99 functions. The assembly text also
renames CSWTCH.612/613 in FS_Seek, whose bytes are unchanged. Reviewed expected
codegen difference; no gate weakening. Upstream C fix/test:
https://github.com/ec-/Quake3e/pull/431. Self-review: shared out-parameter bug only;
no extension policy, OS access, allocation, destructor, FP or layout changes.
Regression 34887856044 and full build 34887856072 pass on source 8fd57208.
Final checkpoint changes documentation only; self-review passes.

Next AAS reproduction: GCC UBSan compile of be_aas_reach.cpp with its existing
-Wno-maybe-uninitialized exception overridden by -Werror=maybe-uninitialized fails
on beststart at line 2196. The function is AAS_Reachability_Jump (the earlier note
incorrectly called it JumpArea). A temporary source copy with a bestdist==999999
return before VectorMiddle compiles cleanly; engine source is unchanged so far.
The Makefile exception explicitly names this one defect and can be removed in
its own #31 PR. Evidence: /tmp/aftershock-aas-warning-before.log and
/tmp/aftershock-aas-guard-check.log.

## #31 AAS candidate validation

The existing runtime --sanitize build fails before on the recorded beststart
warning once its specific Makefile exception is removed. AAS_Reachability_Jump
now returns false when bestdist retains its initial sentinel, before midpoint
reads. The edge selector only replaces bestdist when it supplies endpoints; no
candidate arithmetic was rewritten. Upstream C compilation likewise fails before
and passes after: https://github.com/ec-/Quake3e/pull/432.
Explicit unit/collision regeneration has no golden diff. Final Q3 and OA UBSan
both-map smoke, normal serial smoke and both-renderer replay pass unchanged.
The production symbol/codegen gates are not identical: only jump/grapple functions
change, with an added private VectorLength body and no direct sqrtf import.
Register/stack/inlining changes were reviewed; actual emitted VectorLength matches
a libm oracle for four million finite-input vectors across all four rounding modes.
No source FP expression change or gate weakening. Self-review: one missing-candidate
bug, existing compile/runtime regression, no layout/OS/allocation/destructor change.
Regression 34889484418 and full build 34889484412 pass on source b5e31b8c.
Final checkpoint is documentation only; self-review passes.

## #31 PNG header validation

The existing demo build fails before on the new chunk-header alignment assertion
(4 instead of 1). A scoped packing pragma gives this wire type byte alignment;
its eight-byte size, two uint32_t fields and BigLong conversions stay unchanged.
All six buffered header-read sites share the type. GCC production codegen/symbol
gates pass identically; both-renderer fixed replay preserves every frame hash.
Explicit unit/collision regeneration has no golden diff; both-map smoke passes.
Upstream C layout assertions fail before and pass after with GCC and Clang:
https://github.com/ec-/Quake3e/pull/433. No expected-failure entry or suppression
covered this type. Self-review: one alignment bug; persistent layout assertions
in the existing renderer build, no new content/recording, FP, allocation, OS-access
or destructor changes. Regression 34890358367 and full build 34890358307 pass
on source becd4b27. Final checkpoint changes documentation only; self-review passes.

## #31 JPEG table-index validation

The permanent Clang sanitizer unit fails before at index 5 outside JHUFF_TBL *[4].
get_dht now chooses the AC/DC base array first and applies the index after its
existing validation. All legal slot destinations and expected error code/index
values pass, with the routine and probe compiled as C. Pointer mode also passes.
Explicit unit/collision regeneration has no golden diff; one-ULP control, normal
smoke and fixed-demo replay pass unchanged. Symbols pass; only get_dht changes
normalized assembly among 15 functions, reviewed as the expected pointer-lifetime
change. No gate weakening. Upstream C test fails before and passes after:
https://github.com/ec-/Quake3e/pull/434. Known-bugs has no entries, and ubsan.supp
is empty. Self-review: one vendor bounds bug, existing unit runner, no file loading,
FP, layout, engine OS-access, allocation or destructor change. Regression
34891606879 and full build 34891606634 pass on source 01f2dd40. Final checkpoint
changes documentation only; self-review passes.

## #1 acceptance evidence

Source e0013d9085f56ddd4d726a15e5c5a724dc9272f3 passed regression 34892972996 and
full build 34892972994. Local Clang 21 and hosted Clang analysis pass 140 engine
translation units using 356 compilation commands across both renderer configurations.
Controls reject seven owning objects, accept trivial/defaulted objects and pointers,
and verify core inclusion/platform exclusion. Clang's AST `destroyed` annotation
comes directly from VarDecl::needsDestruction; the controls detect format drift.
Section 11 records the retained longjmp rationale, wrapper restriction, and inactive
preprocessor-branch/self-review limitation. AGENTS.md and README document the command.
No engine source or golden changed. Final self-review passes; docs checkpoint only.

## #31 native math acceptance evidence

Source 152cc6e294a37772c0d942fb1ce69dadb1d57c9d passed regression 34894597080 and
full build 34894597081. GCC/Clang optimized and ASan C checks pass all eight words
from the unmodified 32-bit SSE C executable. Explicit unit/collision regeneration
has zero diff. Symbols pass; only Q_rsqrt changes normalized assembly among 47
functions, with unchanged FP arithmetic order. No expectation/suppression existed.
Self-review: one LP64 native word-width defect, three prerequisite GPL source/header
imports, no production engine/FP/layout/OS/allocation/destructor change. This C
math dependency is test-only until #2. Final checkpoint changes documentation only.

Independent temporary #2 preflight: seven shared ABI sizes and three offsets match
between original GPL C headers and engine C++ headers (/tmp/aftershock-native-layout-*.txt).
Base-game native smoke with original bg_lib support reproduces all 36 accepted
q3dm17 gameplay events. Full text differs only in module-loading metadata, build
date and bot-skill padding (custom VM printf vs native libc). This is preliminary
single-map evidence, not completed parity. Temporary UI compiles; cgame additionally
requires upstream code/ui/ui_shared.h, an include omitted from the initial #2 import.
Do not remove VM/JIT paths or change accepted fixtures before full native parity.

## #31 movement result validation

The poisoned-output production-engine test fails before (3993d575) and passes after
for GCC and Clang/libc++. The upstream C test fails before with GCC and passes after
with GCC and Clang: https://github.com/ec-/Quake3e/pull/435. BotMoveToGoal now zeroes all 52 bytes before lookup/early returns;
no FP expression, layout, allocation, OS access or destructor change. Symbols pass;
only BotMoveToGoal differs among 33 assembly functions: additional zero stores and
register allocation changes, reviewed as expected for this tested bug fix.

Explicit unit/collision regeneration preserves their hashes (8d44421d / 9674cd22).
q3dm17 is unchanged. q3dm7 changes only by Major's two-line chat (one say event),
now hash 028fba42; native and QVM match after implementation metadata normalization.
OpenArena obstacle avoidance no longer depends on stale caller memory: oa_dm1 has
70 rather than 75 Item events, still four kills, one rather than two say events
(hash 9ca81956); oa_dm7 has 58 rather than 71 Item events, five rather than two kills,
and two rather than one say events (5a511a91). Both maps repeat identically before
these explicit golden writes. No recording/frame golden changes. Fixed replay
passes both maps/renderers for Quake 3 (b38004b1) and OpenArena (5b89d338).
No expected-failure entry or UBSan suppression covered uninitialized movement output.
Regression 34900480717 and full build 34900480656 pass on source
fc49615d4be82ff41110f6d521ee6945dd376739. GCC UBSan Quake 3 smoke also passes both
updated goldens. Self-review: one #31 initialization defect, production-body test
first, explained golden changes only, unchanged file/wire layout and FP expressions,
no new engine OS calls, non-trivial destructors or allocations. Issue updated;
upstream #435 open. This final checkpoint changes documentation only.

#2 preparation only: OpenArena oaxB52 source tag resolves to
331464ca396d80e91cf9be273588f2b5f4b7afc8, matching the release used by hosted QVM
fixtures. Clone is /tmp/aftershock-oa-native-source; no native OA build or code change
yet. GCC and Clang both accept their binary32 literal flags in C++20 as well as C.
Temporary C++ compilation of the base game lists expected enum/pointer/constness,
FOFS pointer-to-int and old-style definition conversions; no C++ source port begun.

## #31 native dispatch validation

The test at 6d4710b4 uses the actual production VM_Call and a native entry stub;
counts 3, 0, 1, 2 check every delivered argument, return value and restored call
depth. Clang 21 crashes before at the zero-count call. After initializing unused
slots, GCC and Clang/libc++ pass. The native Clang module from #2 then reproduces
both QVM Quake 3 logs through the transition driver (6dad7c18 / a15c9c91 normalized).
Default QVM bot smoke and both-map/both-renderer fixed replay remain unchanged.
Explicit unit/collision regeneration has zero diff. Symbols pass; only VM_Call
changes among 26 assembly functions: zero stores and native-path control flow;
no source FP, layout, OS, allocation or destructor changes. No known-bug entry or
suppression covered the native uninitialized arguments. Regression 34908517245 and
full build 34908517199 pass on source 3201b7fa8babcd54be0129fac3c0d0ab99349dcf.
Self-review: one shared dispatch initialization bug, failing test first, no unrelated
refactoring, unchanged file/wire layout, FP expressions, allocation, OS access and
trivial lifetime rules. Issue updated, upstream #436 open. Final checkpoint docs only.

Native dispatch upstream C fix/test: https://github.com/ec-/Quake3e/pull/436.

Next #31 prerequisite investigation: Clang -std=gnu99 -O2 -Werror=array-bounds
-fsyntax-only on the original pinned GPL ai_cmd.c and ai_team.c independently
rejects both index-32 teamleader writes. Other writes already use ClientName or
Q_strncpyz. These game files are absent from ec-/Quake3e, and pinned OpenArena
already terminates at sizeof(teamleader)-1. Keep this native import defect scoped
to its own #31 PR; do not introduce unrelated game imports upstream.

## #31 team-leader bounds validation

Test-first commit fc665341 imports only code/game/ai_cmd.c and ai_team.c from GPL
revision dbe4ddb10315479fc00086f08e25d968b4b43c49, retaining their notices. Full native
integration remains #2. `python3 tests/teamleader.py` checks the actual C functions
against the pinned, clean original bot-state headers with Clang bounds errors.
Both original index-32 writes fail; both bounded-copy replacements pass. The test
fetches public source headers when absent, never game assets. CI runs it on Clang.
Original source SHA256s and reproducer are in cpp-port-notes.md.

GCC -O2 -DNDEBUG defined-symbol comparison passes. Of 43 ai_cmd and 22 ai_team
functions, only BotMatch_StartTeamLeaderShip and BotTeamAI change normalized assembly:
the existing Q_strncpyz call replaces strncpy plus the out-of-bounds byte store;
the first function also reallocates one register. No FP instruction changes.
No expected-failure entry or UBSan suppression covered this compile-time failure.

Explicit unit/collision regeneration produces zero golden diff (8d44421d /
9674cd22). Gameplay/frame fixtures are unaffected by these test-only prerequisite
imports; the existing CI runtime gates remain required.

PR #53 source 0ff62c302c96e00f929f4537e7bc8559805f2c29 passed regression
34909591046. Full build 34909591164 compiled macOS release successfully but its
artifact upload timed out at CreateArtifact (ETIMEDOUT); retry only the failed job
after the remaining build job finishes. This is not a source/build failure and
is not counted as a passing full-build gate. The original GPL ai_team.c ends in
a blank line, preserved byte-for-byte in the prerequisite import; the fix itself
has no whitespace-only changes.

Full build 34909591164 attempt 2 passed on the same source; only the failed macOS
job was rerun. PR #53 self-review: one #31 bug, exact two-file prerequisite import,
only two bounded-copy changes; no new OS access, non-trivial lifetime, allocation,
wire/file layout or FP expression changes. Defined-symbol/codegen review and the
failing-before/passing-after test pass. Goldens/fixtures remain unchanged; no
expectation or suppression applies. This final checkpoint changes documentation only.

#2 resumed at merge 383c53e0. Permanent Clang C native smoke passes both Q3 maps
(6dad7c18 / a15c9c91 normalized accepted logs); Clang C native fixed replay passes
both maps/renderers with the unchanged frame hash b38004b1 and original fixtures.
Commands use --game-code native --cc clang --cxx 'clang++ -stdlib=libc++'; logs:
/tmp/aftershock-native-runtime-clang-final.log and
/tmp/aftershock-native-demo-clang-final.log. GCC parity was already measured before
this bounds-only fix; the unused team paths are now covered by tests/teamleader.py.
OpenArena C native preflight is confined to /tmp/aftershock-oa-native-work from the
pinned source: transitional intptr_t syscall words, entry signatures and the same
binary32 ABI header. No repository OA source import or golden changes yet.

OpenArena native preflight builds game/cgame C modules but oa_dm1 stops before bot
startup: SP_func_door passes NULL ent->targetname through strequals to libc strcmp.
GDB confirms __strcmp_avx2 -> SP_func_door -> G_CallSpawn -> G_InitGame. Reproducer:
/tmp/aftershock-oa-native-smoke.py and /tmp/aftershock-oa-native-gdb.py (logs alongside).
The original macro is code/qcommon/q_shared.h:712; nullable targetname also reaches
it in three g_main.c paths. This is an external OpenArena game-source #31 item;
no patch has been made. Quake 3 native compiler/replay parity is complete, so its
C++ catalog port can proceed independently while OA remains a failing prerequisite.
The permanent native OA build and UI source mapping are not yet implemented.

## #2 C++ compatibility deviations

Baseline 3306d55d compiles 103 native C release objects (module-local shared sources
included) with GCC -O2 -DNDEBUG and the established native ABI flags. Hash manifest:
/tmp/aftershock-native-c-object-gate/before/sha256.json. This precedes all C++ edits.
Two necessary syntax adaptations outside T1–T25 are isolated in their own commit:
- FOFS uses `(int)offsetof(gentity_t, x)` plus stddef.h instead of narrowing a pointer
  expression directly to int, rejected by 64-bit C++. The int field representation
  and every measured offset stay the same; no entity layout change.
- Three bg_lib sort helper definitions use prototype parameter lists with their
  original types, replacing K&R definitions that C++ cannot parse. Bodies unchanged.
All 103 C release objects remain byte-identical after these changes (zero changed
SHA256s), including the field table and sort code. Evidence command:
python3 /tmp/aftershock-native-c-object-gate.py deviations. No simulation expression
or golden change; no new algorithm or bug fix. Ordinary catalog casts/renames follow
in separate commits. C++ syntax preflight initially reports 50 of 100 module TUs
failing; bg_lib adds old-style-definition errors. No permissive flags are enabled.

First ordinary catalog pass: T1 pointer casts, T2 boolean-expression casts, T3 enum
casts, T4 delete member -> deleteButton, T14 register removal, T20 const search
results (or casts where the shared receiving pointer also mutates writable text).
All 103 native C release objects remain byte-identical to 3306d55d; including
uis.debug's T3 compound-assignment spelling, so no T25 branch is needed. The field
rename leaves the menu asset paths unchanged. During review a broad temporary
replacement also changed two string literals; those were restored before this
passing gate and are not part of the commit. Strict C++ syntax now proceeds to
string-literal constness under -Werror=write-strings; no warning suppression.
Evidence: /tmp/aftershock-native-c-catalog-final.log; 103 unchanged hashes.

T8 read-only declarations now cover the three cvar tables, item names/media, spawn
and command names, menu artwork fields and fifteen diagnosed string-pointer arrays.
Receiving locals and existing extern array declarations retain matching qualifiers.
Public function signatures are unchanged; the one parameter qualified is a static
UI helper. menutext_s.string remains mutable because several menus fill its backing
buffer; their literal assignments still need call-site casts. All 103 native C
release objects remain byte-identical (const-final log). Next: remaining T8 literal
casts at unchanged public APIs/return sites, then C++ exports and full module gates.

Remaining T8 sites retain the existing public char* interfaces and mutable UI text
fields: 1,060 diagnosed literal/macro-expression casts across 71 files. Macro
constants and concatenated string contents are unchanged; casts are at use sites.
All 103 TUs now pass GCC C++20 syntax with -Werror=write-strings and
-Werror=register, no permissive flags. All 103 C release objects still have the
original byte hashes (literal-casts gate). Next: T5 exports, Clang C++ diagnostics,
linked C++ native ABI/symbol/codegen comparison and permanent smoke/replay parity.

T5 marks exactly dllEntry/vmMain in all three modules with guarded Q_EXTERN_C.
T15 adds eleven required literal/macro separator spaces in ai_team/g_cmds. GCC
and Clang now pass all 103 C++20 syntax checks. All 103 C release objects remain
byte-identical (exports gate). No other C-linkage annotations or math edits.

Native build/check commands now expose --language c++ (module builder) and
--game-language c++ (runtime/replay). GCC and Clang/libc++ link all three modules
and match the 29 ABI layouts/three offsets. GCC C++ bot smoke matches both accepted
Q3 logs; fixed replay is running. The linked-module check uses -z defs.

Isolated build compatibility deviation: Clang C++ at -O2 rejects bg_lib.c's atof
because glibc has already defined an optimized extern-inline atof. The builder
compiles only this compatibility TU separately with -D__NO_INLINE__; this controls
glibc header definitions, not optimizer inlining. Other TUs are unchanged. Clang
C bg_lib raw object SHA256 is identical with/without the setting:
028960a967ce3710c7b994aa6743e7eb640ca8bc9a40e10d7fdd822f0b2578c0.
GCC does not receive it (its object would change). Evidence:
/tmp/aftershock-native-bg-lib-inline; all-module Clang C++ link now passes.
The function bodies, caller arithmetic and external atof symbol are retained.

GCC C++ fixed replay passes all original Q3 samples (b38004b1); Clang C++ all-module
links pass after the scoped header setting. Artifact gates identify real pending
C/C++ library/header differences plus compiler symbol/table numbering; they are
not yet marked passed. CI also exposed the standalone #31 team-leader check's
missing COM_TRAP_GETVALUE definition after #2 imported the complete local headers.
The check now uses the native ABI header and local bot-state types, removing its
obsolete external header fetch; it passes locally. No bug fix or gate suppression.

Artifact review caught the two T22 sites in ai_main: AngleDifference(...) and
forward[2] passed to abs. Explicit int casts preserve the C call's conversion;
these are port compatibility edits, not FP expression restructuring. The C++
front-end's default _GNU_SOURCE also redirected scanf/strtol to C23 symbols while
the C99 reference used C99/legacy entries. The native C++ builder now uses
-U_GNU_SOURCE -D_DEFAULT_SOURCE, matching the C feature set; focused GCC/Clang
objects both reference __isoc99_sscanf and strtol again. Seven of 103 objects
still differ in undefined library dependencies (ctype macro vs function calls,
plus strstr-to-strchr optimization); no defined-symbol difference is reported.
These require explicit review, not a blanket normalizer. Artifact reports:
/tmp/aftershock-native-cpp-gates-pinned/results.json and per-object diffs.

Permanent native_shared.py compares actual shared math (4,096 samples including
zero/quadrant angles) and Q_strlwr/Q_strupr for all nonzero bytes in the C locale.
GCC C vs C++ and Clang C vs C++ agree: math 67988592, case 676e85f5. This covers
AngleVectors' packed/scalar compiler variation and libc ctype macro/function paths;
no fixture/golden writes. CI now builds C++ modules and runs this differential in
both unit compiler jobs. Clang C++ smoke and fixed replay pass both maps/renderers,
using unchanged Q3 fixtures/frame hash b38004b1. Logs:
/tmp/aftershock-native-runtime-cpp-clang.log and
/tmp/aftershock-native-demo-cpp-clang.log. GCC C++ passed those same outputs earlier;
T22 and feature-setting changes still require its final runtime/replay rerun.
Regression 34911920499 passed on checkpoint 42675c08; current CI will validate the
new permanent shared-function and C++ build steps. Artifact G3/G4 review remains
open (seven dependency diffs; no defined-symbol mismatch; per-object reports under
/tmp/aftershock-native-cpp-gates-pinned). Do not label those gates complete yet.

Final GCC C++ rerun after T22/library-feature pinning passes both smoke logs and
all fixed replay frames; Clang C++ passes the same. Regression 34912410405 passed
on c386658a, including both C++ module builds and shared-function differentials.
All 103 defined-symbol sets and raw dllEntry/vmMain spellings match the C baseline;
the seven remaining undefined-dependency diffs are fully enumerated in the reports.
Artifact codegen review remains open before static integration. Next is the separate
#31 OpenArena nullable-target helper fix, keeping #2 checkpointed on this branch.

## #31 OpenArena absent target names

Pinned public OpenArena source: 331464ca396d80e91cf9be273588f2b5f4b7afc8 (oaxB52).
The header macro strequals calls strcmp directly. Native oa_dm1 crashes because
SP_func_door passes absent targetname; three g_main elimination-target paths use
the same helper with nullable names. All call sites were checked. The fixed helper
returns false if either name is absent and evaluates each argument once, retaining
case-sensitive comparison and equality for present empty strings. All non-null
call behavior remains strcmp equality. This fixes the shared cause once.

`python3 tests/openarena_strings.py` reads three public headers from the exact Git
revision into its own output directory, preserving notices. It applies the checked-in
patch there and compiles the real helper with UBSan; no game assets or new loader
are involved. Test-first 43a3dac3 fails (argument 2 NULL); GCC/Clang pass after.
No expected-failure entry or suppression covered this newly observed external-source
bug. ec-/Quake3e lacks the helper and corresponding game code, so no applicable
engine upstream PR exists. #2 will consume this patch for its native CI build.

Native OA smoke now passes both maps with the patch: normalized hashes 51d66d9a
(oa_dm1) and 0f2e6b68 (oa_dm7). The first startup succeeded but still used libc rand;
linking #2's already verified QVM rand/sort compatibility library restored gameplay
parity. Three additional VM-loader metadata lines are excluded in the temporary
native comparison; no gameplay text is removed. This build adaptation belongs to
#2 and is not an additional source fix in this PR. Temporary driver:
/tmp/aftershock-oa-target-fix-smoke.py; no native OA fixture recordings.

All 13 source files that call strequals pass defined/undefined-symbol comparison
before/after. Nineteen function bodies change codegen through null checks and
associated branch/register allocation; no function is added or removed at -O2.
The helper preserves strcmp equality whenever both pointers are present. The
G_FindTeams pair already guards both team pointers before comparison; missing
names never become matching team names. Explicit unit/collision regeneration has
zero golden diff. Existing engine/runtime source is unchanged by this patch-only
CI dependency fix. CI and PR self-review are still required before merge.

PR #54 source 07ea4fe3 passed regression 34913347473 and full build 34913347479.
Self-review: one external dependency bug, tested before/after; patch scoped to the
shared helper; no new engine OS access, allocation, lifetime, layout or FP edits;
all caller symbols preserved, null-guard codegen reviewed, goldens unchanged.
No expectation/suppression applies. This checkpoint is documentation only.

Additional #2 client preflight while CI ran: native OA cgame/UI compile and load.
Fixed oa_dm7 replay matches all six accepted samples; oa_dm1 differs on both
renderers in a roughly 107x108 pixel region (about 4,690 pixels at sample 50).
No replay is regenerated or claimed passing. Artifacts:
/tmp/aftershock-oa-native-demo and /tmp/aftershock-oa-native-demo-preflight.py.
Client build helper /tmp/aftershock-oa-native-client-build.py maps base UI objects
to code/q3_ui, uses code/ui/ui_syscalls.c, maps bg_* to code/game and links #2's
QVM random/sort library. Native OA frame parity remains #2 work after this fix.

#63 explicit unit/collision golden regeneration is byte-identical; static OA
smoke also matches both accepted bot logs. No golden/fixture change.

Platform checkpoint: all macOS configurations and the completed MSVC configurations
pass on 440089eb in build 34936092520; remaining build jobs are running. Local
MinGW native-Windows client (USE_CURL=0 USE_SDL=0, matching CI) links successfully.
The optional MinGW SDL/no-curl build exposed old missing Windows header context;
record it separately for #31 without changing those engine sources here.

#5 documentation correction: the untouched finished network driver belongs to
#3 merge 8692b422, before both path and build migrations, not the 390a20f4 build
parity checkpoint. It is not rerun or adapted here. README and AGENTS now identify
its historical revision explicitly.

UI skill investigation during #8: a bounded real UI_SPSkillMenu_SkillEvent call
with g_spSkill=1e38 and a valid ID_EASY event fails GCC and Clang
undefined,float-cast-overflow checks at ui_spskill.cpp:114. Q_atof rejects
NaN/Inf but accepts this large finite value. Other UI readers also cast before
validation. Record and fix separately under #31; no bug fix in this warning PR.
Local reproducer/logs: /tmp/aftershock-ui-skill-probe.cpp and
/tmp/aftershock-ui-skill-before{,-gcc}.log. Preserve each reader's existing
valid-value behavior and invalid-value policy.

UI skill fix evidence (2475e0d2): all five UI readers use UI_GetSkill, which
clamps the finite cvar value to 0..6 before conversion. Values outside 1..5 remain
invalid for existing callers, including truncation of 5.9 to 5. Level-menu reset,
score rejection, menu clamping and selected button behavior are preserved. The
new helper is UI-internal; no engine/public contract or layout changes.

Both Clang C/C++ helper builds and 103-object ABI gates pass with no layout or
symbol differences and the same 60 advisory C/C++ codegen differences. Across
32 production UI objects (GCC/Clang release, GCC debug, MinGW), only the skill
readers change instructions and UI_GetSkill is added. 122 shifted constant
references were checked against their actual bytes; unrelated instructions are
preserved. Artifacts /tmp/aftershock-ui-skill-codegen/{after,review}.json.
ARM64 server configurations do not contain UI; hosted client cross-builds remain.

Explicit unit/collision regeneration is byte-identical (8d44421d/9674cd22).
Q3 bot logs retain 6dad7c18/a15c9c91 and fixed replay retains b38004b1 with the
original two demo fixture hashes. No accepted file changed. Native provenance
records 2475e0d2 for the five imported files, preserving original GPL hashes.
Local logs /tmp/aftershock-ui-skill-{unit,differential,runtime,demo,native-*}.log.
The initial default /tmp/aftershock-tests configure encountered an old CMake
cache; the clean task-specific /tmp/aftershock-ui-skill-unit passed.

Null-subtraction source b6927331 matches the reviewed 25-object preview. The
explicit stdint.h include preserves line count. Both Clang native helper builds
pass, including their layout checks, and all six library hashes are unchanged:
/tmp/aftershock-null-subtraction-before.json and null-subtraction-{c,cpp}.log.
No accepted golden regeneration. PR #79 merged-tree run is 35025630301.

General parentheses preview: all 27 bot_moveresult_t_cleared callers pass the
simple identifier result. Removing declaration parentheses from the macro leaves
all 51 production/native objects byte-identical across GCC/Clang release, GCC
debug, MinGW and ARM64. GCC actual-source controls reject the old declaration
and accept the new one. No source change applied here; a later one-class PR can
remove -Wno-parentheses and change only that macro. Artifacts:
/tmp/aftershock-parentheses-declaration-preview/{results.json,*-control.log}.

UI skill PR #80 final: head 53569f88 passed full build 35025644781 and regression
35025644789; self-reviewed and merged 8b74ad07. Issue #31 comment 5688396029
records the complete validation and upstream applicability decision.

Unused-result preview (not applied): explicitly bind discarded console-write
results to [[maybe_unused]] auto locals. All five optimized GCC/Clang x86 and ARM
objects retain raw hashes; debug changes are confined to stores in the four
console functions. Existing output remains best-effort; no new error policy is
introduced. A separate class PR still needs review of those debug differences,
flag removal and hosted gates. /tmp/aftershock-unused-result-preview.

Address class local validation: all four GCC/Clang C/C++ native helpers/ABI
checks pass. Clang's six libraries match and GCC's game/cgame libraries match;
the GCC UI baselines predate the separately merged #80 fix. Artifacts:
/tmp/aftershock-address-before.json and address-{gcc,clang}-{c,cpp}.log.

Array-bounds preview is still under review: GCC 15 reports [0,4] outside qhandle_t[5]
on both skill-picture reads even after #80. Equivalent explicit dereference
*(skillMenuInfo.skillpics + (skill - 1)) removes the diagnostic. Clang objects
match, but GCC/MinGW/debug emit address-calculation changes requiring review.
No source change applied. /tmp/aftershock-array-bounds-preview. Do not use the
earlier ungrouped pointer variant; retain the original integer subtraction.

Fresh unused-parameter syntax inventory is running independently in
/tmp/aftershock-unused-parameter-inventory (driver .py, log .log). No source edits.
Both local GCC/Clang accept [[maybe_unused]] parameters in gnu99 helper mode;
Clang rejects nameless C definitions, so do not remove native parameter names.
A later class PR must verify hosted compiler compatibility and object hashes.

2026-09-19 resume: #81 full build/regression and #80 merged-tree run passed.
#81 merged 87907a26 after self-review; issue #8 comment 5742665588 records gates.
Unused-parameter inventory completed: 829 syntax configurations, zero compile
failures. The preview script stopped before edits because diagnostic columns
expand tabs; fix the column mapping in the temporary script before continuing.
No unused-parameter source edits have been applied to the repository.

Declaration source d0d8a7df: the existing complete movement-result regression
passes with GCC and Clang. Current production syntax checks pass across 2,380
configurations with the general parentheses warning enabled. New logs live in
/home/matt/.cache/aftershock-modernization/declaration-bot-move-{gcc,clang}.log.

Unused-parameter preview (not applied): 275 [[maybe_unused]] annotations in
88 source files, no headers or function-body changes. All 2,380 current production
syntax configurations pass with unused parameters treated as errors, after the
preview's relative vendor includes were connected to existing third_party sources.
The owned-code snapshot and changes.json are in unused-parameter-preview under
the persistent cache. Raw production/native object comparisons are running in
unused-parameter-objects; next verify C99 helper compatibility and review the diff
before its own warning-class PR. No annotations are in the repository yet.

Unused-parameter object preview complete: 665 of 833 production/native objects
match byte-for-byte. The other 168 are GCC debug objects and match after removing
only debug sections from copies. No instruction/data changes. Evidence:
unused-parameter-objects/{results,debug-review}.json in the persistent cache.
Four standalone C/C++ helper comparisons are still running.

Unused-parameter final local review: original source bytes are preserved after
removing the new parameter attributes, except trailing spaces on two touched
function-declaration lines (win_main.cpp/common.cpp). Source 05cb1e37 and GPL
provenance 3a3fca15 are committed. The UI export include's eight consuming production
objects are included in the 833-object comparison. Both compiler C/C++ helper
comparisons completed successfully, all twelve libraries retaining hashes.

Missing-initializer preview is outside the repository in the persistent cache:
15 files explicitly zero omitted members or use empty aggregate initialization.
The static allocator string blocks use a constexpr initializer to zero conditional
debug members without changing the layout. Compiler checks are running in
missing-initializers-check; object, C99 helper and conditional-build verification
remain before this separate warning-class change can be applied.

PR #84 follow-up 5623e8fe is integrated: all 281 parameter annotations retain
names and bodies, including six Windows debug validation callback parameters.
The full local MinGW debug client/server build passes and a before/after control
preserves native callback instructions/relocations. The initial regression
35449780958 passed; corrected-head full workflows are required. #83 merged-tree
regression 35449777481 passed. Persistent evidence: validation-callback/ and
validation-callback-check.log.

Initializer review: the four GCC debug common.cpp objects differ only in four
allocator __LINE__ immediates, each increasing by five source lines. Two MinGW
Sys_OpenVideoPipe objects reorder stores to distinct stack locations around a
comparison; mov does not change condition flags, and final stored bytes/branch
condition are unchanged. No added/removed functions or other instruction changes.
The explicit DWORD cast on si.cb leaves both reviewed MinGW native objects
byte-identical. All twelve native helper hashes/layouts are unchanged.

Latest long experiment review: the blanket V1 replacement was rejected. Script
integer widths feed float conversion, so V2 explicitly preserves scriptSigned_t/
scriptUnsigned_t as 32-bit on Windows and 64-bit elsewhere. Minizip file positions
retain their API-owned type via decltype(unz_file_info::uncompressed_size);
formatters use explicit int64_t arguments/PRId64 while retaining the existing
signed diagnostic interpretation. No such code is applied. Twelve experimental
native helper hashes match formatted-native.json; 1,810-assembly comparison is
being refreshed only for the filesystem rows. Review Windows changes and MSVC
conversions before proposing a PR. Foreign long declarations still require a
precise, documented policy; do not silently suppress a whole file or family.

#9 BC upload feature test: extend the existing production RHI probe with four
compressed formats and an odd-size 7x5 mip chain. Assert native format mapping,
block-rounded copy offsets and exact staging bytes, including replacement upload.
The unchanged engine fails to compile because the new formats/API are absent.

BC upload implementation now passes tests/rhi.py with GCC and Clang/libc++.
BC feature enablement and sampled/filter/transfer format support are checked at
texture creation. Compressed data uses the existing staging/copy path with 4x4
block rounding; legacy pixel uploads retain unit-sized blocks. No simulation or
accepted artifact changed. Evidence: cook-bc-{before,after,clang}.log.

Native KTX2 loading now uses plain asserted header/index records and borrowed mip
views; runtime SHA-256 verification matches the Python cooker. Pinned 0BSD
amosnier/sha-2 commit 565f65009bdd98267361b17d50cddd7c9beb3e6c supplies the
allocation-free C implementation (source/license hashes checked by tests/cook.py).
The valid owned texture check failed before the native reader (387d74af test-first)
and passes with GCC/Clang after implementation. Image registration uploads BC
blocks through the normal image table and RHI binding path. The development client
build passes. A real ImGui run loaded the cooked six-mesh character in q3dm17,
showing its BC7s texture and 62 frames; the preview was visually inspected. Named
clip controls/material flags/reload acceptance remain, so this is preliminary.

Fixed Q3 replay plus video restart passes twice per map, retaining frame projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
Evidence: cook-native-texture-{before,after,clang}.log, cook-runtime-build.log,
cook-ui-check.log, cook-demo.log; screenshots /tmp/aftershock-cook-ui. No accepted
fixtures/goldens changed. Next implement material records and reload ownership,
then named clips, source watcher and the complete runtime acceptance driver.

Cooked material records now validate their version/hash and load through shader
registration. They retain ordinary diffuse/vertex/lightmap shading and set culling,
unlit, blend or mask policy. Base-color factors are baked offline in linear color
space into each material's private texture, avoiding runtime shader variants.
Explicit material JSON sources now share the glTF material cooker. An independent
Pillow DDS decode checks the BC7 result against linear factors and alpha. The
current legacy alpha-test path accepts MASK cutoff 0.5; other cutoffs produce an
explicit offline diagnostic pending #13. The valid material test first failed on
the absent API (6370ad04); its fixture expectation was corrected to Blender's
actual doubleSided=true export, without changing source content.

GCC full cooker/native checks and the development client build pass. Material
version/flags/texture records retain 48-byte header and 88-byte payload assertions.
The new opaque-texture recipe adds metadata to #9's new artifacts; no accepted
game golden or source fixture is regenerated. Runtime reload and remaining source
kinds are still in progress. Evidence: cook-material-{before,complete,build}.log.

The reload GPU ownership step is test-first at 8f16f98f. A production RHI probe
replaces a texture twelve times, alternating dimensions, and observes exactly one
live image/view/allocation and one stable descriptor binding. Injected device-wait,
memory-allocation and view-creation failures preserve the live texture and reclaim
the incomplete replacement. The implementation adds explicitly owned memory only
to this replacement path; legacy image pools retain their existing allocation.
GCC and Clang/libc++ pass the full RHI probes. Evidence: cook-replace-*.log.
No source watcher or renderer reload polling is connected yet.

The source watcher and native texture reload are connected. Test-first commits
c885ee01/6a396dbe failed on the absent publication marker/renderer consumption.
The runtime gate then exposed a new-feature clock choice: ri.Milliseconds scales
with timescale, so polling now uses the existing real Microseconds callback.
With the preview camera settled, source PNG editing reached the captured frame
in 0.610705 seconds, changing 3,759 model-preview pixels. Before/after images were
visually inspected; the source fixture and accepted goldens were untouched.

The cooker publishes a fixed-record, hashed project index and revision marker only
after successful cooking. The enabled renderer reads the small marker without
engine allocations, checks the index/content hashes and swaps loaded texture
resources at a frame boundary. Shipping builds contain no polling or watcher.
Enabled renderer ABI is now 13 (shipping remains 10); rebuild matching modules.
GCC/Clang cooker checks, RHI ownership probes and the local development build pass.
The runtime job now installs its own venv dependency and runs the owned-character
reload gate against OpenArena. Evidence: cook-watch-*.log, cook-live-before.log,
cook-live-fixed-camera.log, cook-reload-build.log. The first 0.255857-second sample
also had a late yaw adjustment; use the fixed-camera 0.610705-second result.

Remaining #9 work: named clips; bounded material/model/animation replacement and
UI status; WAV/OGG/shader source cooking; complete static/module, restart, idle,
Q3/OpenArena and exact-head hosted gates. Existing IQM dataSize accounting reports
zero model bytes; this predates #9 and is recorded in #31/docs/bugs.md, not fixed
here. Handle that separate #31 PR after #9 before continuing #10.

Named native clips are now stored in the IQM block and copied through the public
GetModelAnimation API (shipping ABI 12, development ABI 15). Test-first 72c6ed43
failed on the absent records; GCC and Clang checks now verify `idle`/`wave` ranges,
30 FPS and non-looping flags, plus the existing production pose samples. The
inspector selects clips, clamps/scrubs within their ranges and stops non-looping
playback at the final frame. The valid fixture's loop expectation was corrected
to its actual flags=0 record; the fixture/cooker were not changed for that check.

The live owned-character test selects wave frame 46 and idle frame 15 through real
input, with screenshots visually reviewed. Its texture edit reaches the sampled
frame in 0.607583 seconds with 2,721 changed preview pixels. Evidence:
cook-clips-{before,after,clang,build}.log and cook-live-named-clips.log. Types and
boundary gates pass. Hosted build 35503080582 at preceding 23420106 found MSVC
C4701 in the new material branch; explicitly zero-initialize that local before
validation. This feature correction is included here; fresh hosted gates remain.

Next: own the cooked IQM allocation for bounded model/animation replacement,
then material replacement and UI reload observations. Keep existing dataSize
accounting untouched until the separate #31 fix recorded above.

Cooked model replacement is test-first at a71fe7d1. Twelve production IQM swaps
retain one live zone allocation (two only during replacement), reclaiming the old
block after the new model succeeds. Registered handles stay stable; existing
legacy models retain hunk ownership. Cooked IQM content hashes are checked before
registration/replacement. Existing dataSize accounting remains a separate #31 bug.

The live test edits a temporary copy of the Blender animation, changing its pose
and renaming wave to salute. Handle 78 and frame 46 survive the update; the
inspector reports salute and the screenshot shows the changed pose. It then
scrubs idle frame 15. Texture latency is 0.620578 seconds (2,721 preview pixels).
Evidence: cook-model-replace-{before,after,build}.log and
cook-live-model-reload.log. No accepted/source fixture changed. Hosted MSVC at
76287e8c found two new clip-loop shadow warnings; use clipIndex to resolve them.
Next: bounded in-place material replacement, then the remaining #9 acceptance.

Material replacement test added before implementation: production shader storage
must preserve the handle/hash chain/remap, correctly reorder changed blend sorts,
and keep one hunk allocation across twelve one/two-stage edits. It fails on the
absent reloadable flag/replacement API (cook-material-replace-before.log).

Material replacement is test-first at 822893f3. The cooked base-color recipe
reserves two stages once, reuses its shader pointer/handle and stage storage,
preserves remaps/hash chains and reorders changed sort keys at the frame boundary.
Twelve edits allocate no additional hunk storage. Development cooked materials
remain dynamic instead of baking their stage colors into static vertex buffers.
Transparency applies after texture/lightmap combination; unlit materials skip
lightmaps. Legacy shader allocation and shipping material behavior are retained.

The live test edits only its temporary source copy to change the material from
opaque to transparent. The preview disappears, with the model handle and selected
frame unchanged. Texture, material and model inspectors expose reload counts
(development renderer ABI 16, shipping ABI 12). Screenshot reviewed; texture
latency 0.608521 seconds / 2,721 changed preview pixels. GCC/Clang cooker probes,
development build, formatting, type and boundary gates pass. Evidence:
cook-material-replace-{before,after,clang,build}.log,
cook-live-material-reload.log and cook-material-{format,types,boundaries}.log.

Hosted regression 35503477810 at old head 76287e8c passed the other required
legs but the lifetime checker was terminated (exit 143), with no source diagnostic.
A fresh complete run is required; this is not an accepted gate. Continue #9 with
remaining audio/shader source kinds and the complete final acceptance checks.

Audio source feature test added before implementation. New owned 50 ms WAV/OGG
tones were authored once with libsndfile 1.2.2 (source/provenance committed); CI
reads these bytes. Test expects normalized PCM16 WAV with a versioned ASCK chunk,
source/content hashes and unchanged 22,050 Hz / 1,102 frames. The existing native
WAV path can consume this RIFF container without another runtime audio format.

WAV/OGG cooking now passes on GCC and Clang/libc++. The source feature test first
failed on the absent audio kind (dd2f171a). PCM inputs normalize to signed PCM16;
Vorbis decoding reuses the engine's vendored libogg/libvorbis in an offline helper.
Native WAV loading consumes both outputs at 22,050 Hz / 1,102 samples with the
expected tone amplitude. An ASCK RIFF chunk carries version 1, source SHA-256 and
whole-file SHA-256 (its own bytes zeroed); the ordinary native WAV decoder skips
that provenance chunk. No new runtime codec or audio resampling is introduced.

Evidence: cook-audio-{before,after,clang}.log. Source fixtures were authored once,
not regenerated. New-feature MSVC C4701 at 05192647 identified the texture reload
local; explicit zero-initialization resolves it just as for material records.
Next: offline shader source cooking using the existing pinned shader package,
then remaining final #9 acceptance and exact-head gates.

Shader source feature test added before implementation: a copied owned vertex
shader with a quoted include must cook to versioned/hash-checked SPIR-V, skip an
unchanged recipe, rebuild after the include changes, and feed the existing offline
shader package while preserving all other 73 shader bytes. It fails on the absent
shader kind (cook-shader-before.log). Runtime GLSL compilation remains excluded.

Shader cooking now passes its test-first check (5d2bf779). Quoted includes are
tracked and compiled from a confined snapshot using pinned glslang 16.6.0. The
versioned SHA-256 envelope contains native SPIR-V; CMake COOKED_SHADER_DIR feeds it
through the existing offline package builder, which requires the current exact
reflected interface. A changed include produces a changed shader; all other 73
binaries stay identical. The default package remains 743e9c51f75547a6577119182c0363c5829f498e1e99c8672e057fad19c7e737.
Evidence: cook-shader-{before,after}.log and cook-source-kinds.log. No runtime
compiler/shader hot reload is added.

Hosted runtime at 05192647 found the developer registry probe needed the three
new reload-count query stubs; it now checks those copied counts too. This is a
feature test integration correction, not a legacy engine bug. Remaining: cooker
tangent/bounds review, repeated live edits/idle/restart/module acceptance, fixed
Q3/OpenArena replays and complete exact-head hosted/self-review gates.

Final cooker review found a new-feature bounds error: enlarged source scale 32000
puts production IQM posed vertices outside bounds derived from unquantized source
frames. Test-first evidence is cook-bounds-before.log; the ordinary scale still
passes. Fix this cooker calculation within #9, not an existing engine #31 change.
Also require absent source tangents to remain absent instead of inventing a basis.
No simulation arithmetic or accepted fixture/golden is changed.

The bounds correction now passes native geometry checks at scale 32 and 32000
on GCC and Clang/libc++. It reconstructs stored float/16-bit poses and bind/vertex
values before computing bounds, with float-accumulation padding. Optional source
tangents are retained; absent/mixed tangents are omitted for #13 to generate a
material-specific basis. Clip labels are unique and fit native 63-byte storage.
New #9 artifacts change accordingly; accepted goldens and source fixtures do not.

A separate pre-existing IQM row-scale bug is recorded in docs/bugs.md and #31
comment 5749265890. The cooker rejects rotated nonuniform joint scales until
that separate fix. No engine matrix arithmetic changed. Both that bug and the
allocation-accounting bug need separate #31 PRs after #9, before #10.

Ten thousand unchanged publication polls perform no engine allocation and no GPU
operation. The live Quake 3 test now checks six additional material/model/texture
edits, idle frames and video restart: renderer storage stays at 17,032 bytes in
one zone block; permanent hunk stays at 22,959,808 bytes through all repeated
edits. Idle UI allocations stay constant. The owned character renders again
after restart. Texture latency is 0.605579 seconds / 2,721 changed pixels.
Evidence: cook-bounds-*.log, cook-final-runtime.log and the runtime screenshots.

The shader override check now compares SPIR-V type/global/decorated records in
addition to the original reflected layout; a changed vertex input width is
refused. Default package bytes/hash remain unchanged. A full shipping client
build consumed a cooked shader through COOKED_SHADER_DIR successfully
(cook-shader-build.log); compiler/contract evidence is cook-shader-contract.log.
Hosted build at f7258d82 passed (35505208309); that is an earlier head, not final
acceptance. Local tidy passed 1,170 production configurations. Remaining final
module/OpenArena/runtime/replay/hosted gates are required before PR readiness.

## #9 final local acceptance and self-review

Local static Quake 3, module Quake 3 and static OpenArena live gates pass, including
source texture/clip/material edits, six further replacement cycles, idle frames
and video restart. Measured texture latency: 0.605579 / 0.606071 / 0.605774 seconds
respectively (2,721 changed preview pixels each). Module/static renderer storage
stays bounded. Ten thousand unchanged publication polls allocate no engine memory.
The OpenArena test's standalone build now selects its matching native objects,
using the same helper as the existing demo tests.

OpenArena data was initially absent locally: that attempt failed, not skipped.
Seven SHA-256-verified Debian data archives were then extracted only into the
user cache (no system package installation), and tests/openarena.py staged their
paks plus its pinned gamecode. The source/hash list is
~/.cache/aftershock-modernization/openarena-data-packages.json. No Quake 3 pak was
copied, committed or uploaded. Evidence: openarena-data-stage.log and
cook-final-{runtime,modules,openarena}.log.

Fixed Quake 3 replays twice per map with video restart pass projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
OpenArena module replays twice per map pass
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96.
All four committed demo hashes remain unchanged. Evidence:
cook-final-demo-{q3,oa}.log. No accepted golden or fixture was regenerated.

Self-review: scope matches #9; source import/compression/compiler work stays
offline. Native geometry, PCM and shader-package paths are reused. New runtime
records retain layout/copy assertions; renderer ABI changes require matching
modules. OS access is confined to the existing filesystem layer, with tools
using their normal host APIs. Core lifetimes remain trivial, no simulation FP
expression changed, and idle polling uses fixed caller storage. Reload transactions
retain handles and bounded CPU/GPU ownership. Existing IQM bugs are documented
and deferred to separate #31 PRs; the importer reports the unsupported rotated
nonuniform scale combination.

GCC/Clang feature checks, 10,000-poll and twelve-replacement checks pass. Local
lifetimes: 1,124 compilation commands / 120 paths, positive and seven-object
negative controls pass. Tidy: 1,170 configurations pass (advisory findings retained).
Format/type/boundary gates pass. Hosted build 35505791786 at 70a31804 passed; its
regression 35505791776 has passed every required job except lifetime analysis,
which was still running at this checkpoint. Superseded regression 35505208258
was cancelled after its other jobs passed, to prioritize the final head. These
earlier-head results do not replace the required final exact-head workflows.

Final-head Clang CI found a source-watcher race (35506318169 / job106066633883):
a source edit immediately after cooking could be captured by the post-cook
snapshot without being built, leaving the published revision unchanged. A
deterministic interleaved-edit check now exercises the real cooker/watcher loop
before fixing it. Preserve the one-second live gate. Hosted lifetime checks at
70a31804 were again terminated with exit 143; batch the same AST coverage to
bound clang-query's retained translation-unit memory (source verified against
LLVM 18 clang-query/tool/ClangQuery.cpp). Final exact-head gates must rerun.

The interleaved-edit test fails before the watcher fix with `watcher lost the
edit made during cooking` (cook-watch-race-before.log). This test is committed
before the implementation. Eight-path lifetime batches passed all 1,124 commands
in 5:01, but peaked at 7,340,232 KiB RSS; reduce to one source path per process
for hosted-runner headroom. The unbatched measurement reached 20,712,276 KiB
before its intentional 180-second measurement timeout; that run is not a pass.

The watcher now compares snapshots around the complete cook and keeps a baseline
only after a stable pass. Startup/changed manifests discover dependencies, then
receive a verification pass; edits during cooking are not silently accepted.
The deterministic test fails at 03268cd1 before implementation and passes after;
GCC and Clang/libc++ full cooker checks pass, and the live texture/reload/restart
check still passes within one second (cook-watch-runtime.log and latency.txt).

Lifetime batching is by 16 compilation commands, not source paths: game/module.cpp
alone has 412 configurations. Every original command remains in the full evidence
database and is checked once through the batched database. Positive and seven-object
negative controls remain mandatory. The new full local measurement is running
(cook-lifetimes-commands.log); exact-head hosted coverage remains required.
Build 35506318131 at 5ffaabc2 passed. Regression 35506318169 was cancelled after
its Clang watcher failure, with all other completed required jobs passing and
lifetime analysis still pending; it is not an accepted final gate.
Self-review of this follow-up: only offline watcher correctness and analysis
resource use changed; native code, accepted fixtures and hashes are untouched.

## #9 accepted tree and #31 accounting preparation

#145 merged as c195f798; final build 35507057165/regression 35507057162 passed.
The final local lifetime run covered all 1,124 commands / 120 source paths and
positive/seven-object negative controls in 5:02 at 448,048 KiB peak RSS. The
normalized full compile database is identical. One-path batching had still used
6,719,332 KiB because game/module.cpp has 412 configurations; 16-command batches
bound that case too. Live watched texture latency is 0.605985 seconds with 2,721
changed pixels; repeated replacements/idle/restart and accepted fixed-demo hashes
remain passing. No accepted fixture/golden was regenerated.

Accounting test-first 85563345 fails before the fix at
`model.dataSize == (int)allocationSize`. Twelve owned replacements also require
exact current block accounting, rejecting an accumulating fix. Registration and
reload both route through R_LoadIQM; its one native block needs one assignment.
This bug is not a sanitizer finding, so it has no known-bugs/suppression entry.

## #31 accounting fix and local verification

R_LoadIQM assigns model_t::dataSize to its native block size. Test-first 85563345
failed before the assignment; GCC and Clang/libc++ complete cooker tests now pass,
including registration and twelve owned replacements (no accumulating count).
Format passes. Fixed Quake 3 demos replay twice per map and preserve projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4 and both committed
fixture hashes. Logs: iqm-accounting-{gcc,clang,format,demo}.log. No golden was
regenerated, and this non-sanitizer bug has no known-bugs/suppression entry.

Self-review: this #31 change is one metadata assignment in the shared loader.
No geometry/simulation arithmetic, allocation, OS call, destructor, ABI/layout or
unrelated refactoring changes. Initial registration and in-place replacement share
the corrected path. Full exact-head hosted build/regression still required for the
PR, after #9 integration regression 35507482742 passes. The rotated-scale test is
committed separately as 13f999df and fails before any matrix change.

#9 merged-tree regression 35507482742 passed at c195f798. The accounting PR may
now open; its own exact-head build/regression and post-merge regression remain
required. The main worktree is now on issue/31-iqm-accounting.

## #31 rotated-scale test-first preparation

13f999df tests production scale-before-rotation for all axes and signed/nonuniform/
unit scales plus inverse products. e2cd9e50 extends it through the owned two-joint
glTF triangle and native IQM poses. Its tip uses T(0,1,0), Rz(90), S(2,1,1) and
inverse bind T(0,-1,0); the engine-basis final vertices must be (0,0,0), (-1,0,3),
(-1,0,-1). This is an analytical expectation, not a regenerated accepted golden.
Both pre-fix runs fail at the first matrix point assertion. Evidence:
iqm-scale-before.log and iqm-scale-parity-before.log. No engine scale change has
been applied at this test/merge checkpoint.

## #31 rotated-scale fix and local verification

JointToMatrix changes the six off-diagonal scale indices to multiply columns,
so local scale precedes rotation. Both callers (bind construction and animated
pose sampling) use this shared correction. The cooker drops the temporary
native_local rejection and uses its existing decompose helper. The CI unit legs,
AGENTS and tests README now include tests/iqm_scale.py; the general cooker check
accepts the formerly blocked owned nonuniform source.

GCC and Clang/libc++ analytical matrix/inverse/native-glTF pose tests pass with
UBSan, as do both complete cooker suites and formatting (415 owned files).
Fixed Quake 3 demos replay twice per map with the accepted projection
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4 and unchanged
fixture hashes. Evidence: iqm-scale-{after,clang,cook-gcc,cook-clang,format,demo}.log.
No accepted artifact was regenerated. The new valid source case uses analytical
expected positions; no sanitizer suppression or expected-failure entry exists.

Self-review: one existing numerical bug, failing tests before implementation,
no unrelated refactor. The shared helper fixes both affected paths. No gameplay
simulation expression, allocator, OS call, destructor or ABI/layout changes.
Only renderer transform coefficients change; uniform-scale paths and accepted
replays retain their outputs. Exact-head hosted build/regression, the preceding
#146 integration gate, and this PR's merged-tree regression remain required.


#146 initial build 35508144036 caught MSVC C4267: the allocation size is size_t,
while model_t::dataSize retains its signed 32-bit reporting field. The correction
checks that the allocation fits before assigning with an explicit conversion;
an unrepresentable count is rejected before allocation. GCC/Clang complete cooker
checks and formatting pass again (iqm-accounting-width-{gcc,clang,format}.log).
This remains the same accounting bug and introduces no new test target. Final
exact-head build/regression must rerun; the initial failed build is not acceptance.
Self-review update: the reporting-width guard is necessary for exact accounting;
no ABI, allocation algorithm, geometry or simulation arithmetic changes.

The accounting width correction 1f1aeb8f is merged forward into the separate
scale branch before its PR. Superseded accounting regression 35508144077 was
cancelled after build 35508144036 failed; it is not an accepted gate.


#146 merged as 3d104d0c after final exact-head build 35508534162/regression
35508533987 passed. Its merge tree equals the tested tree. Integration regression
35508970698 is running; no scale PR opens before it passes. Modernization is
merged forward into the scale branch at this checkpoint. The corrected-accounting
and scale test combination passes (iqm-scale-merged-width.log). #10 test-only
preparation is 57838d59; its initial cook fails on the absent animation asset kind.

#146 integration regression 35508970698 passed at 3d104d0c. Open the scale PR at
this checkpoint; require its exact-head build/regression and post-merge regression.
#10 preparation 0704fe4c adds new original Blender rifle/body acceptance sources,
whose model cooks and neutral-layout visual review pass. The animation-state
asset test still fails as expected; there is no #10 runtime/gameplay implementation.

## #10 initial failing data-authoring test

`python3 tests/animation.py` reuses the owned two-joint source generator and a
plain JSON definition with named clip states, a numeric input, timed transition
blends and a bone-bound event. It requires an animation-kind cook in the existing
version/hash envelope, transitive source manifests and graph-only incremental
recooking. Before implementation it fails with `asset kind is not implemented
yet: animation` (animation-before.log). No accepted fixture is modified.

The test fixes only the initial authoring/envelope contract; native payload
layout and runtime/gameplay semantics still need their own test-first coverage.
This small first slice is not #10 acceptance. The full first-person/third-person,
IK/layers/events and recorded client/server hit-box gates remain mandatory.

## #10 owned acceptance-source preparation

New tests/assets/animation sources are authored once with the same verified
portable Blender 4.5.3 LTS build 67807e1800cc and its glTF exporter. The rifle/arms
has 13 joints, 13 meshes and six one-second clips (idle/ADS/fire/reload/sprint/jump),
plus muzzle/magazine/optic/grip bones. The body has 16 joints, 15 meshes and ten
clips (idle/walk/run/aim up/down/crouch/prone/lean left/right/turn), spine-mask
hierarchy, two-bone limb chains and forward walk/run root motion. Native model
cooks confirm 186/310 frames at 30 FPS. Provenance records all authoring/export
hashes; CI consumes these bytes and never invokes Blender.

The original block geometry and neutral rig layouts were reviewed in local CPU
render previews. The source export and model-only cook pass (animation-export.log,
animation-model-cook.log). The extended initial test verifies provenance/model
counts, then still fails on the absent animation asset kind
(animation-owned-before.log). No existing source or accepted demo/frame fixture
was regenerated. These are test assets, not evidence that #10 runtime/gameplay is
complete; full rifle/body in-game and recorded hit-box acceptance remain required.


#147 merged as 7f4d43a7 after exact-head build 35509606176 and regression
35509606177 passed. The merge tree equals the tested tree; integration regression
35510058541 is still running. Both #31 fixes are now merged separately, and #10
has merged modernization forward. No #10 runtime implementation begins before
that integration gate passes.

The new source rigs also load through the real developer viewer in a locally
rebuilt client under private Xvfb/lavapipe. Actual UI input selects rifle idle
(frame 62), rifle fire (31), body idle (93); model handles 78/79 and advancing
preview counts are confirmed. Screenshots were reviewed at side/oblique yaw.
The first temporary capture script quit before its last queued screenshot; adding
the existing wait-before-quit pattern fixed the script. No engine change was needed.
Evidence: animation-native-preview.py/.log and animation-native-preview/*.png in
the persistent cache. These checks validate source rigs/clips only, not the
future #10 gameplay/replication/IK acceptance.

#147 integration regression 35510058541 passed at 7f4d43a7. Both IQM fixes are
accepted, known-bugs has no active entries, and ubsan.supp is empty. #31 is
completed again; #10 native/runtime implementation may now begin after its tests.


#10 native test-first extension: tests/probes/animation.cpp now specifies plain
trivial runtime state/poses, masked and additive transform blending, reachable and
clamped two-bone IK, look-at rotation, compressed-clip sampling through the cooked
asset, timed state transitions and exactly-once bone-bound events. The driver
compiles with strict FP and UBSan and runs the native probe before its incremental
edit. The pre-implementation compile fails on the missing
engine/animation/animation_public.h (animation-native-before.log). No runtime
implementation exists at this checkpoint; commit these assertions first.

The native pre-implementation contract also covers root displacement across a
loop, events at state entry/end, multiple crossed loop events, and repeated-tick
deduplication. In particular, an end notify must be delivered before an on-end
transition changes states. These cases are committed before runtime code.
