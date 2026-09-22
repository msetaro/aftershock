# Ironforge migration handoff — 2026-09-22

This is the restart record for the lost worktree/thread. The maintainer requested
syncing unfinished work, not merging incomplete code. Read current `origin/main:AGENTS.md`,
then this file and `docs/modernization-progress.md`. Check live GitHub state before acting.
All work stays in **msetaro/aftershock**; use explicit repository arguments with `gh`.

## Restore and resume

Clone the repository, fetch origin, and check out `issue/29-backend-services`.
The branch contains every local #29 implementation/test-first commit, including the
final intentionally failing heartbeat test. Do not restart this work from main.
The previous worktrees and `/home/matt/.cache/aftershock-modernization` are disposable;
no accepted fixture requires regeneration to move machines.

Read issues #25, #23, #24, #29, #30 and deferred #180, and draft PR179. Earlier
modernization is already accepted as recorded in the progress file. Continue until
completion or a real maintainer dependency. #180 is explicitly deferred and is not a
reason to stop or request Steam setup now.

## Exact integration state

- Main: `aa96932abb13f761d0a71dde89f95d33893a45c8`, accepted #22 PR178.
  Its 26 integrated jobs pass: build 35767363858, regression 35767363841.
  #21 PR177 is also fully accepted; issues #21/#22 and tracker #25 are updated.
- Draft [PR179](https://github.com/msetaro/aftershock/pull/179), branch
  `issue/23-platform-services`, head `1f3957166a740a60d00ecb2e4d663eb136133dd6`:
  all 16 compiler jobs in [35767480303](https://github.com/msetaro/aftershock/actions/runs/35767480303)
  and all 10 active regression jobs in [35767480297](https://github.com/msetaro/aftershock/actions/runs/35767480297)
  PASS. Still draft/unmerged at migration. Self-review, fresh base/head checks,
  mark-ready, merge commit and integrated verification remain. If main changes,
  merge it forward and rerun gates first. Never merge a red/skipped required job.
- #23's revised deliverable is bounded provider/null-backend interfaces and
  deterministic-provider contract tests. Actual Steam SDK implementation, ticket
  transport and live presence/invite acceptance belong to [#180](https://github.com/msetaro/aftershock/issues/180).
  Mock identity tests are not live Steam acceptance. #24 retains its SDK dependency.
- #29 contains accepted main through merge `6ae3fbfa`, then backend work through
  `71a119d9` and this handoff/test checkpoint. It is incomplete and must stay draft.
  After #23 is accepted, merge main forward into #29, preserving `SERVICE_BACKEND = 2`
  in `engine/platform/services_public.h`. Resolve progress-file overlap by retaining
  the new handoff/current next action and accepted predecessor evidence.
- `known-good-2026-09-20` tag object is `8bc8c94c75e7c9ae59ee3fe1277e084942dda5f4`,
  target `81a0f9dc05c340f30182c34134bde67290c21774`. Never move/delete known-good tags,
  force-push, rewrite history, directly push main, or touch port-evidence.

## #29 implemented and tested locally

Implementation is in existing `tools/match`, shared `contracts`, native join/identity
code, platform HTTPS and `engine/client/cl_backend.cpp`. Read the issue as the final
spec; the following list describes progress, not completion.

- Versioned bounded JoinTicket/MatchSpec/Loadout contracts, independent Python HMAC
  oracle, native verifier/nonce retention, real UDP admission and controller readiness.
  Invalid reload preserves the previous configuration and replay protection.
- PostgreSQL-backed ticket exchange, hashed sessions/replay prevention, owned profiles
  and loadouts, parties, queue membership, persistent assignment and Agones allocation.
  Real private-DB integration tests run; no missing-database pass/skip fallback.
- Ambiguous allocation recovery persists `allocating` before posting to Agones.
  Only definite UnAllocated returns to queued; ambiguous errors recover by label lookup
  and never blindly issue a second allocation. Unknown unresolved allocation requires
  operator reconciliation; no invented timeout reset. Initial queue allocates one party
  or solo group to a pod. Queue cancellation is not implemented; review remaining scope.
- Verified account IDs feed game-log/stat attribution, including disconnect scores,
  reconnect baselines and checkpoint restore. Anonymous log behavior is unchanged.
  Parser tests pass; authenticated native-client log acceptance remains unrun.
- Read-only results RPC/HTTPS with separate reader credential, owner checks, pagination,
  exact match selection and terminal assignment cleanup even when no stats are earned.
  #28's durable development log supplies reads; production ingestion/storage is #30.
- Bounded asynchronous platform HTTPS, strict TLS/no redirects, cancellation and shared
  curl-loader ownership. Linked, dynamic and unavailable modes pass GCC/Clang UBSan
  probes. Existing download golden remains unchanged:
  `e24e0dc222e1f191920e5e9bebfbe1aed12302571b7cf70ed944583e51886963`.
- Authored backend shell schema/cooker/native validation; fixed action names and public
  values only. Client login/queue/profile/results/logout uses ephemeral credentials,
  binds the join ticket to its assigned numeric address and clears it after connection.
  No credential cvars/console arguments/UI values. Development login reads private
  `fs_homepath/backend-ticket.bin` through the same backend exchange, not an identity bypass.
- Namespaced deployment generator: service/RBAC, two replicas, TLS health probes,
  resource limits, HPA and disruption budget; only references provisioned secrets.
  Manifest contract passes. Actual deployment/scaling has NOT been tested.

Full Go race/vet, private PostgreSQL/TLS integration, native ticket probes, HTTPS
probes, authored UI checks, deployment checks, format/types/boundaries passed during
implementation. Exact results DB test last passed through `71a119d9`.
**Client probes used a preview include of PR179's header only.** Ordinary client
compilation currently lacks its ticket API. Last full native build predates the
frontend addition; never report these previews as an integrated build or live test.

## Immediate unfinished test and remaining acceptance

`tests/backend_join_runtime.py` now requires **no legacy master heartbeat**, using
only empty destinations and a private loopback receiver. It was run against the old
server and failed as intended with:

```
AssertionError: retired master heartbeat emitted to owned receiver:
 b'\xff\xff\xff\xffheartbeat QuakeArena-1\n'
```

No production retirement edit exists yet. Trace callers before removal:
`SV_MasterHeartbeat`/`SV_MasterShutdown` in sv_main.cpp, `SV_Heartbeat_f` and command
registration in sv_ccmds.cpp, spawn/init/shutdown in sv_init.cpp, connect/drop triggers
in sv_client.cpp, associated fields/prototypes in server.h and master macro in
q_shared.h. Preserve unrelated uses of local variables. Common/client `sv_master`
cvars also support legacy server browsing; decide their compatibility within #29's
scope instead of deleting them blindly. Run the existing full admission/controller
check after implementation; it must still exercise the original signed-join controls.

After integrating #23 and fixing that test:

1. Run normal GCC/Clang client probes and full native builds without preview headers.
2. Complete actual HTTPS/native-client login -> queue -> Agones match pod -> signed
   join -> verified account stats -> results in profile, using owned deterministic
   provider fixtures. No live Steam credentials are needed for this deferred scope.
3. Add kind acceptance for the deployment, health and metrics/scaling behavior. Reuse
   existing #28 Kubernetes/test helpers. The old local controller binary and Docker
   image were stale at `78701343`; rebuild all artifacts from the integrated branch.
   #28's development ingest uses static allocation-token configuration; the test must
   arrange the actual backend-generated token before match end. #30 owns production
   ingest. Do not weaken admission or result ownership to get the fixture running.
4. Finish issue scope audit, CI/affected/full-suite wiring, tests/README.md, AGENTS
   verification commands, self-review and all hosted gates. New tests are not yet
   comprehensively wired into the permanent catalog. Update #29 and the checkpoint.
5. Merge #29 only with current-main gates green; verify integrated runs, then #30.
   No unrelated engine bug fix belongs here; record it for a dedicated #31 PR/test.

## Rebuild prerequisites and useful commands

Use tests/README.md and docs/agents/README.md plus workflow versions as the source of
truth. Previous machine had GCC 15.2, Clang/libc++ 21, clang-format 21.1.8, CMake/Ninja,
Go 1.27.1, Python 3.12.13 with jsonschema/Pillow (RAQM), Docker, Xvfb, Mesa software
Vulkan and glslang 16.6.0. Match tooling pins dependencies; private PostgreSQL test
image is pinned in tests/backend_services.py. Recreate private tool environments;
do not assume the old cache paths, displays, images, certificates or binaries exist.
Existing local-machine restriction was no system package installation; use private
caches/environments unless the maintainer changes it. Hosted CI installs its packages.

From this branch, after integrating #23 for the client checks:

```sh
(cd tools/match && go test -race ./... && go vet ./...)
python3 tests/backend.py
python3 tests/backend.py --cc clang --cxx clang++ --output /tmp/backend-clang
python3 tests/backend_services.py
python3 tests/backend_http.py
python3 tests/backend_http.py --cxx clang++ --output /tmp/backend-http-clang
python3 tests/backend_ui.py
python3 tests/backend_deployment.py
python3 tests/backend_client.py
python3 tests/backend_client.py --cxx clang++ --output /tmp/backend-client-clang
python3 tests/backend_join_runtime.py --server SERVER --controller CONTROLLER
```

The last command is intentionally failing until heartbeat retirement. Runtime tests
use owned/generated content where possible; hosted game regressions use OpenArena.
Keep proprietary paks at the separately provisioned local path; never commit/copy
paks into this repo. Preserve accepted goldens, demos, shaders and frozen checkpoints.
The finished network driver's evidence is historical; do not run/edit it again.

Private caches/logs were not uploaded. Essential results and commands are recorded
here and in the progress history; all source/test fixtures are committed. Audit at
migration found all other registered worktrees clean, no stashes, and every other
local branch tip already reachable from origin. A fresh clone needs no old worktree.
