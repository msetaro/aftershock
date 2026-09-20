# One match per server

Build the dedicated-server image from the repository root:

```
docker build -f tools/match/Dockerfile -t aftershock-match:issue28 .
python3 tools/match/dev.py --matches 2 --output /tmp/my-aftershock-matches
docker compose -p my-aftershock-matches -f /tmp/my-aftershock-matches/compose.json up
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

The #12 identity-provider seam exists, but reliable ticket transport/provider
wiring belongs to #23. This deployment uses #28's approved per-match password
fallback until that wiring exists. Password acceptance is not provider identity
or executable attestation. Pure content validation stays enabled.

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
  --data /tmp/aftershock-openarena-baseoa --output /tmp/new-match-kind-run
```

It uses SHA256-pinned user-cache kind 0.33.0, Kubernetes 1.35.8, kubectl 1.35.8,
Helm 4.3.0 and Agones 1.60.0. Helm generates unique webhook certificates and uses
`--server-side=false` for the chart's legacy patch metadata. Configure the actual
root `gameservers.namespaces`, `minPort` and `maxPort` values; they are not under
`agones`. Only localhost UDP ports are published. The test stages resolved public
OA pak bytes into its private node, requires a real native client to enter play,
checks the acknowledged final checkpoint and waits for a fresh Ready replacement.
It deletes only its own cluster on exit. Keep generated specs, Secrets and kubeconfig
out of artifacts; publish only logs, `report.json`, `events.jsonl` and density files.

The test measures the match pod's three containers through CRI cumulative CPU and
working-set counters over 20 seconds with one connected player. The report gives
resource-equivalent matches/vCPU and matches/GB, including the wrapper, shipper and
Agones SDK; shared ingest/control-plane costs are excluded. This is a measured
baseline, not a saturation or worst-case capacity guarantee. Production sizing
needs the intended player count/maps plus operating headroom.

Additional checks (Go 1.27.1):

```
(cd tools/match && go test -race ./...)
python3 tests/match_exit.py --server SERVER --content openarena --data /tmp/aftershock-openarena-baseoa
python3 tests/match_runtime.py --controller CONTROLLER --server SERVER --client CLIENT
```

The native tests use temporary homes; existing Quake 3/OpenArena golden fixtures
are never regenerated. The runtime driver uses installed OA content for its
optional client and only owned content for server-only runs.
