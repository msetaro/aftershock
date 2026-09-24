# One match per server

For related commands, allocate one root first: `export AFTERSHOCK_SCRATCH="$(mktemp -d)"`.
Otherwise each invocation gets a fresh root; children inherit it. Retain the root
for logs/build reuse, then remove it when its evidence is no longer needed.

Build the dedicated-server image from the repository root:

```
docker build -f tools/match/Dockerfile -t aftershock-match:issue28 .
python3 tools/match/dev.py --matches 2 --output $AFTERSHOCK_SCRATCH/my-aftershock-matches
docker compose -p my-aftershock-matches -f $AFTERSHOCK_SCRATCH/my-aftershock-matches/compose.json up
```

The generator gives every match a separate password/token and writable home,
with UDP ports starting at localhost:27960. `matches.json` contains the local
connection details. Choose a new output directory and Compose project for every
run: match homes/cursors must never be reused for a different match. After inspecting
results, `docker compose ... down --volumes` removes that project's disposable data.
The ingest container's `/app/match records` command prints its acknowledged JSONL
records. The stub accepts only its configured per-match tokens and fsyncs before ACK.
It is a development stand-in for #30, not a database or production service.

The image contains the static native game/server, its libc/libm runtime, the Go
controller, and only the owned two-lane sample. `content.py` packages the accepted
MAP/BSP/AAS plus project textures/model/shaders; it never compiles or regenerates
accepted fixtures and never searches installed game content. Use
`python3 tests/match_content.py` to check reproducible bytes. The sample is enough
for a dedicated server; a complete playable game's character/UI/sound assets are
separate from this server packaging. CI mounts OpenArena to test a real client.
No Quake 3 pak is copied, published or included in any image.

The process runs as 65532:65532 with read-only root, dropped capabilities and no
privilege escalation. `/home/match` is the sole writable match volume, backed by
`emptyDir` in Kubernetes. The image precreates the legacy default-home directory
so its initial mkdir probe succeeds without writing to the root filesystem.
`fs_basegame=aftershock` selects the project's content without legacy asset checks;
the OpenArena acceptance profile explicitly selects `baseoa` instead.

## Launch and lifetime

`/app/match run` reads JSON from `MATCH_SPEC` or `MATCH_SPEC_FILE`:

```
{"id":"match-123","map":"two_lane","mode":0,"frag_limit":10,"time_limit":10,
 "players":8,"password":"replace-with-a-random-password","token":"replace-with-an-allocation-token"}
```

Identifiers, secrets, counts, modes and limits are bounded; unknown fields and
extra JSON fail. There is no arbitrary console-command field or shell expansion.
`MATCH_PORT` defaults to 27960, `MATCH_CONTENT` to `/content`, `MATCH_GAME` to
`aftershock`, `MATCH_HOME` to `/home/match`, and `MATCH_SERVER` to `/app/server`.
At least one of the frag/time limits must be positive. Time limits are in minutes.
`players` is the maximum slot count, not a minimum-start quorum.

`sv_exitOnMatchEnd=1` exits the native game after its five-second intermission.
Default servers retain their existing lifetime and ready-vote behavior. The
wrapper's readiness probe verifies the actual loaded map through local UDP;
`/app/match probe` exposes that same check. A process exit writes `engine.done`;
SDK Shutdown waits for `results.done`, which only the acknowledged shipper creates.
A failed final ingest does not claim successful persistence or invoke SDK Shutdown.
Unacknowledged bytes remain in the pod until the orchestrator removes it; the last
acknowledged checkpoint is the durable outcome after node/pod loss.

The original #28 development generator uses its per-match password fallback.
The #29 backend instead allocates a versioned MatchSpec with expected account IDs,
a private join-signing key and an ingest token. The controller writes a private
join configuration before readiness; native admission verifies short-lived,
match-scoped tickets and retains used nonces across reconnect/config reload.
Pure content validation stays enabled. Live Steam SDK/provider acceptance is #180;
owned fixture authentication does not establish Steam support.

## Results are outside the frame loop

`/app/match ship` tails `games.log` in a separate process/container. It sends
version-1 batches through the checked-in `ingest.proto` service using standard
protobuf Struct/Empty messages. Batches contain match ID, contiguous byte offsets,
event lines, accumulated joins/kills/per-player kills/deaths/scores, elapsed game
seconds, and completion/final-stream flags. The allocation token travels only in
gRPC authorization metadata. The backend must bind that token to the match ID.

A pending batch is persisted before sending. Cursor/checkpoint updates are fsynced
and renamed only after acknowledgement. Retries preserve the exact pending batch;
the stub deduplicates identical retries and refuses gaps or changed retry payloads.
Final acknowledgement requires the cursor to cover the closed game log. The
shipper sends partial progress at least every two seconds when events exist and
flushes the final record after process exit. An empty startup failure can finish
its event stream without claiming a completed match.

TLS is the default for `MATCH_INGEST` (default `ingest:50051`). Only isolated local
Compose/kind examples set `MATCH_DEV_INSECURE=1`; the stub requires it. Its
`MATCH_TOKENS` JSON map is supplied by local generation or a Kubernetes Secret.
Production #30 must supply authenticated TLS ingest and durable storage. No database
driver, credentials or blocking persistence call is added to the game process.
The stub keeps a digest per batch in memory: this is intentionally a development
store, with #30 owning retention and scalable persistence. Full memory-image
resume and server-side demo recording are outside this issue.

## Kubernetes acceptance

`kubernetes.py` writes a private test namespace, one-replica warm Fleet, result
sidecar, ingest deployment/service/Secret and a GameServerAllocation. The Fleet
loads its map before Ready and keeps limits disabled while warm, protected by a
random unrevealed password. Allocation supplies validated `aftershock.dev/match`
metadata, applies the actual password/slots/mode/limits, and restarts that same
map. Use map-specific Fleets. Health comes from loaded-map probes; successful
match completion drains results, calls Shutdown and lets the Fleet replace the pod.

The automated check owns a randomly named kind cluster and private kubeconfig:

```
python3 tests/match_kind.py --image aftershock-match:issue28 --client /path/to/quake3e.x64 \
  --data $AFTERSHOCK_SCRATCH/aftershock-openarena-baseoa --output $AFTERSHOCK_SCRATCH/new-match-kind-run
```

It uses SHA256-pinned user-cache kind 0.33.0, Kubernetes 1.35.8, kubectl 1.35.8,
Helm 4.3.0 and Agones 1.60.0. Helm generates unique webhook certificates and uses
`--server-side=false` for the chart's legacy patch metadata. Configure the actual
root `gameservers.namespaces`, `minPort` and `maxPort` values; they are not under
`agones`. The test reserves 32 MiB and limits 128 MiB for the SDK sidecar, which
Agones injects as an always-running init container; resource accounting includes
that container as well as the two ordinary containers. Only localhost UDP ports
are published. The test stages resolved public
OA pak bytes into its private node, requires a real native client to enter play,
checks the acknowledged final checkpoint and waits for a fresh Ready replacement.
It deletes only its own cluster on exit. Keep generated specs, Secrets and kubeconfig
out of artifacts; publish only logs, `report.json`, `events.jsonl` and density files.

The test measures the match pod's three containers through CRI cumulative CPU and
working-set counters over 20 seconds with one connected player. The report gives
resource-equivalent matches/vCPU and matches/GB, including the wrapper, shipper and
Agones SDK; shared ingest/control-plane costs are excluded. The report also gives
the configured Kubernetes request budget, which limits scheduling independently
of observed idle CPU. This is a measured
baseline, not a saturation or worst-case capacity guarantee. Production sizing
needs the intended player count/maps plus operating headroom.

Additional checks (Go 1.27.1):

```
(cd tools/match && go test -race ./...)
python3 tests/match_exit.py --server SERVER --content openarena --data $AFTERSHOCK_SCRATCH/aftershock-openarena-baseoa
python3 tests/match_runtime.py --controller CONTROLLER --server SERVER --client CLIENT
```

The native tests use temporary homes; existing Quake 3/OpenArena golden fixtures
are never regenerated. The runtime driver uses installed OA content for its
optional client and only owned content for server-only runs.

Local Compose generation uses a unique project name and Docker-assigned host UDP
ports by default. Discover them with `docker compose -f PATH/compose.json port
--protocol udp match-1 27960`; pass `--port` only when deliberately reserving a
host range. Kubernetes generation likewise chooses a private namespace unless
`--namespace` is supplied. Internal container ports are unchanged.

## Player-facing backend (#29)

`/app/match backend` owns HTTPS login, profiles/loadouts, parties, queue membership
and read-only results. Provision `BACKEND_DATABASE`, `BACKEND_APP_ID`,
`BACKEND_STEAM_KEY_FILE`, `BACKEND_CATALOG`, `BACKEND_TLS_CERT` and `BACKEND_TLS_KEY`
outside the repository. The catalog is a bounded JSON array of cooked weapon paths.
`BACKEND_CA_FILE` adds private trust roots without disabling system/TLS validation.
No credentials belong in client cvars or command arguments. Live Steam setup is
explicitly deferred to #180; no SDK/AppID/account setup is needed for fixture tests.

Generate namespaced resources with `backend_kubernetes.py --namespace NAME --image
IMAGE --output NEW_FILE`. It references a separately provisioned `backend-config`
Secret, requests two replicas and supplies namespaced Agones RBAC, readiness,
resource limits, HPA and PDB. Operators install the cluster resource-metrics API
for the HPA. `/healthz` checks database availability; `/metrics` reports bounded
service counters/errors/durations and request logs are structured JSON without
credentials. `BACKEND_NAMESPACE` enables allocation using the pod service account;
the projected token file is reopened for each Kubernetes request so rotations take
effect without a restart. Missing, empty or oversized credentials fail closed.
`BACKEND_MAP`/`BACKEND_FLEET` select a map-specific warm Fleet.

The initial queue allocates one solo player or existing party per match. Membership
and assignment state persist across replicas. A lost allocation response remains
`allocating` until its labelled GameServer is recovered. Only a definite
UnAllocated result permits another allocation. A permanently unresolved outcome
requires operator reconciliation; do not reset it on a guessed timeout. Queue
cancellation and skill matching are not part of this initial queue.

The native authored shell uses `backend_url` and optional `backend_ca`, then fixed
login/queue/profile/results/logout actions. Sessions stay process-local and signed
join tickets are bound to the assigned numeric address and cleared after admission.
The developer-only `backend_dev_login` reads `fs_homepath/backend-ticket.bin` and
uses the same HTTPS verification endpoint; it cannot select a player identity.
`backend_info` exposes only the same public values displayed by the authored UI.

`BACKEND_RESULTS` configures the read-only gRPC endpoint and
`BACKEND_READER_KEY_FILE` its separate reader credential. Only the authenticated
session owner's completed results are returned, with exact-match selection and
bounded history/leaderboards. This tier never writes match data. The #28 stub is
used for development acceptance; #30 owns transactional production ingestion.
See [backend verification](../../tests/README.md#backend-services-29) for the
native and kind commands and their current acceptance status.
