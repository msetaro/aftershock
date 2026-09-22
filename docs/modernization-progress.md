# Modernization checkpoint

Integration: `main` (maintainer workflow change, 2026-09-20). Issue branches:
`issue/<number>-<slug>`, one bug per #31 PR and one warning class per #8 PR.
Merge commits only after all required gates pass and the AGENTS self-review.
No human review step. Never merge with a red/skipped required check, force-push,
rewrite history, move/delete known-good-* tags, or touch port-evidence. Nothing is
published or commented outside msetaro/aftershock. `modernization` is retired;
its history below is an evidence record, not the current PR target.

Maintainer continuation (2026-09-19): continue the modernization roadmap through
completion or a dependency requiring the maintainer. This supersedes the earlier
stop after #8/design-only #6. Implement #6 next, then follow #25; preserve the
separate-session #35 scope and the completed network-test evidence.

Maintainer ruling (2026-09-19): keep all future changes and PRs in
`msetaro/aftershock`. Do not create PRs against ec-/Quake3e or another parent
repository. This replaces the earlier requirement to submit applicable #31 fixes
upstream; historical upstream PR references below are completed past work.

## Maintainer Steam deferral — 2026-09-22

Steam SDK setup, provider implementation, authenticated ticket transport and
live presence/invite acceptance move to follow-up #180 at the maintainer's
request. #23 now completes the bounded existing-provider interface and null
backend with deterministic-provider contract tests. Do not claim Steam support
from those tests. This dependency no longer blocks modernization; continue #24's
existing SDK dependency checkpoint, then #29 and #30. No SDK/account answer is
needed for the current scope. #23 and tracking issue #25 record the same ruling.

## Next action


Active isolated preparation is issue/29-backend-services in backend-tree, branched
from main 13106135 while the earlier required checks finish. Preserve merge order
#21 -> #22 -> #23, record #24's existing SDK dependency, then #29 -> #30. #180 owns
the maintainer-deferred Steam SDK/live acceptance and does not block this work.
The #29 issue and existing Go match controller/spec, identity lifecycle and
client HTTP ownership are reviewed. Start with versioned JoinTicket, MatchSpec
and Loadout contracts; reuse the existing Go module and standard crypto/HTTP.
The initial schema/API checks fail before implementation (c5217257). The v1
schema, bounded decoders and domain-separated HMAC join-ticket contract now pass
with an independent Python signature oracle. Existing match-controller Go race
tests also pass after moving its unchanged bounded JSON decoder into the shared
contract package. The native verifier and nonce store now pass both compiler UBSan probes, using
the same Python signature oracle and correctly signed invalid-claim controls.
Server identity/connect, persistent authentication/profile/parties and durable queue
recovery now pass their local tests. Next: read-only results and verified account
attribution, native HTTPS/UI, deployment and real client/kind acceptance.

Resume the active predecessor gates before any later issue can merge:
- #31 PR174 is fully accepted on main 5caa2c1c; #31 is closed again.
- #19 PR175 is fully accepted at main b92b6ef5. Merged build 35728186900 and
  regression 35728186799 pass all 26 required jobs. Merge tree ac9a6cd4 matches
  tested head 312048f0. Issue #19 and #25 are updated for integrated acceptance.
- #20 PR176 merged as main 131061351efc14483aeb7da3f623f79ea884d640 at
  2026-09-22 14:59:32 UTC after all 26 exact-head jobs passed at 4d06f25b.
  Self-review and fresh main/base/head/tag checks passed. Merge tree 11bc602c
  equals the tested head. Integrated build 35744183979 and regression 35744184060
  pass all 26 required jobs. #20 is fully accepted; issue #20 and #25 are updated.

#21 PR177 is fully accepted at main 7c24808f16f9a72d9ed21ba9af736cd54adba386.
All 26 exact-head and integrated jobs pass (merged build 35757322676 and regression
35757322732). Merge tree c14c33d3 equals the tested head; issue #21 and #25 are updated.

#22 PR178 merged as main aa96932abb13f761d0a71dde89f95d33893a45c8 at
2026-09-22 18:28:14 UTC after all 26 exact-head jobs passed on b23ef158. Final
self-review and current-main/base/head/tag checks pass; merge tree 309bac591a8225478a52321b0205a547f885a996
matches the tested tree. Integrated build 35767363858/regression 35767363841 are running.

#23 draft PR179 final head 1f395716 includes merged #22 main aa96932a. Combined
GCC/Clang UBSan services, identity/discovery, format and affected/suite catalog
checks pass. Final build 35767480303/regression 35767480297 are running; require all
26 jobs and integrated #22 acceptance, then fresh main/head/base/tag checks and
self-review before merge. Steam integration/live acceptance is deferred to #180.

Private Python: /home/matt/.cache/aftershock-modernization/sketch-python/bin/python.
Private Go: PATH=/home/matt/.cache/aftershock-match-tools/go/bin:$PATH.
Continue through #24's SDK dependency, #29 and #30 per #25. The #23 live Steam work is deferred to #180. Continue #21/#22/#23 final gates,
then #24's dependency checkpoint and #29/#30; no maintainer input is needed.

## #29 acknowledged results read API

The existing development ingest owner now exposes an authenticated read-only gRPC
method with a separate reader credential. Only final, completed and acknowledged
checkpoints appear as results. History pages contain at most 20 matches with a stable
match-ID cursor; score-ordered leaderboards contain at most 100 accounts. Private
allocation credentials, slot ownership and raw events never leave this API.

Go race tests pass for account isolation, unfinished exclusion, duplicate ACKs,
restart recovery and complete traversal of 105 results through bounded pages
(backend-results-after.log). The development stub intentionally reuses its existing
in-memory/log storage; #30 owns replacing it with indexed transactional storage.
Next: player-facing HTTPS consumer, native UI and kind acceptance.

## #29 read-only results contract, test first

The new read contract requires separate reader authorization, canonical account
ownership, completed-and-acknowledged results only, bounded history/leaderboard
responses, retry deduplication and durable-stub restart recovery. It fails before
implementation on missing Read/response fields (backend-results-before.log).
Reuse #28's development log owner for this acceptance read API; #30 will implement
its production transactional storage. The player-facing tier never writes match data.

A native attribution review also found that a departing player's final score must
be logged before ClientDisconnect clears ownership. The new feature retains only a
bounded log-ownership flag until that callback, since server identity is already
closed by then; anonymous logs stay unchanged. Native/runtime checks remain required.

## #29 verified result ownership

The native game emits ClientIdentity only from the server's existing verified
identity import at ClientBegin; no client name/userinfo claim is used. The sidecar
keeps bounded account statistics separate from legacy slot statistics, clears
ownership on disconnect/connect, accumulates reconnect score deltas, and preserves
its baseline across checkpoint restart. Anonymous game logs are unchanged.
Go race checks and the full native build pass (backend-owner-after.log,
backend-owner-build.log). Actual authenticated-client log acceptance follows with
the HTTPS/UI runtime gate; this is not yet end-to-end backend acceptance.

## #29 verified result ownership, test first

The checkpoint contract now requires server-verified account attribution through
slot reuse, anonymous clients, suicide, team changes and checkpoint restart.
It fails before implementation on missing account statistics (backend-owner-before.log).
Existing slot-oriented development statistics remain compatible; authenticated
results must never infer account IDs from names or client-supplied userinfo.

## #29 durable queue and allocation recovery

The database commits expected-player membership and a private per-match key before
calling Agones. A per-match row lock serializes allocation attempts; the match label
recovers an allocation after its response is lost. Party edits cannot change an active
assignment. Tickets are issued only after the existing UDP probe confirms both the
loaded map and configured match ID, with each authenticated member's own identity
and a fresh nonce. Cluster HTTPS trust and service-account credentials are explicit;
redirects and unbounded responses are refused.

The real PostgreSQL/TLS/UDP recovery test passes, including a committed allocation
whose first response is lost, delayed engine readiness, two member identities,
replica restart and exactly one allocation POST (backend-queue/contracts.log).
The full Go race suite and vet pass. Next: verified account attribution and read-only
results, native HTTPS/UI, deployment and real kind acceptance. Queue cancellation
and production ingestion are not implemented at this checkpoint; #30 owns ingestion.

## #29 queue recovery contract, test first

The new real-database/TLS/UDP gate requires leader-owned group admission, a durable
assignment before allocation, recovery after a committed-but-lost Agones response,
exact configured-match readiness before issuing tickets, separate signed identities
for each member and fresh nonces on repeat polls. It requires one allocation across
replica restart and rejects party changes while assigned. It fails on missing queue
configuration/API fields before implementation (backend-queue-before.log).

Initial matchmaking decision: one existing party (or solo caller) allocates one pod
for the owned test mode. This satisfies the first-game login/queue/join loop without
a speculative skill matcher or additional queue service. The shared database owns
membership and assignment; a later game rule can pool distinct parties when needed.
Do not hold the global membership lock across allocation HTTP; use the per-match
row lock and recover by the match label. Results remain read-only in #29.

## #29 persistent parties

Explicit party create/join/leave now uses the shared database. Invite codes are
random 128-bit values returned only on creation and stored as digests; party IDs
are separate. Membership is unique per account, the leader leaving disbands the
party, and old codes stop working. A short database advisory lock serializes
membership mutations across replicas; it holds no HTTP/network allocation work.
This deliberately simple ceiling is documented for measured future sharding.
The engine's 64-player limit is enforced before adding a member.

Real PostgreSQL/race checks pass for concurrent joins to different parties, identity
ownership, capacity, disband/revoked code, explicit leave and fresh-pool persistence,
alongside the full auth/profile/TLS contracts (backend-parties/contracts.log). The
existing Go race suite and vet also pass. No invitations are sent to any account.
Next: durable queue ownership and Agones allocation recovery, then results/client UI.

## #29 party contract, test first

The real-database gate now requires persistent explicit-code party create/join/leave,
leader disbanding, revoked old codes and one-party-per-account membership across
concurrent replica attempts. It fails with the absent party endpoint before its
implementation (backend-parties-before/contracts.log). Codes are returned for an
explicit user action; no invitations or messages are sent to other accounts.

## #29 HTTPS service entry point

The match binary now has a backend mode with explicit TLS certificate/key, canonical
AppID, private publisher-key file, trusted weapon catalog and database configuration.
Production defaults use the documented Steam HTTPS endpoint; the optional additional
CA file supports the private acceptance PKI without disabling verification. Request
and header sizes/deadlines are bounded. Health checks the database, and fixed service
labels expose request/error totals and duration sums. JSON request logs include only
service/status/duration, never paths, query strings, headers or bodies. Shutdown
waits for bounded in-flight requests before closing the database pool.

The real PostgreSQL/HTTPS lifecycle gate passes (backend-services-https/contracts.log):
private trust setup, actual TLS login, expected metrics and graceful shutdown, together
with all earlier persistent owner/replay/redirect/concurrency checks. Full Go race and
vet checks pass. The server process is not yet a complete backend deployment: parties,
queue/Agones, read-only results, native client HTTPS/UI and kind acceptance remain.

## #29 HTTPS process contract, test first

Extend the real-database gate to start the actual service entry point with private
TLS material, a trusted local HTTPS identity endpoint and an explicit weapon catalog.
Require database-backed health, a real HTTPS login, bounded service metrics without
tokens, and graceful shutdown. The test fails on missing serveBackend before wiring
(backend-https-before.log). Live Steam application/account setup remains deferred.

## #29 persistent authentication and profiles

The bounded HTTP handler verifies identity using Steam's documented publisher-side
AuthenticateUserTicket GET API and the aftershock identity purpose. Its endpoint
must use HTTPS; redirects are refused and upstream URLs/errors containing credentials
are never returned/logged. The local integration fixture is HTTPS too. A caller
cannot choose the authenticated account. Sessions use random 256-bit opaque bearer
tokens, store only token/ticket digests, expire after one hour and retain exchanged-
ticket fingerprints across logout/expiry. Retention is one row per exchanged ticket;
provider-proven cleanup lifetime is deferred until #180 supplies that guarantee.
PostgreSQL owns session/profile state and the unique exchange constraint. Startup
schema installation is serialized across replicas; no ORM or custom DB protocol.
Profile updates are atomic, bounded UTF-8 names plus catalog-validated weapon paths.

The real private PostgreSQL gate passes, including restart persistence, account
isolation, expiry/logout, oversized and unknown-field rejection, no followed auth
redirects, and eight concurrent exchanges yielding exactly one accepted session.
Full existing Go race tests also pass (backend-services-db/contracts.log and
backend-go-service.log). The initial nil-loadout scan needed the standard []byte
SQL destination before JSON encoding; the final gate passes. The service process/
TLS entry point, parties/queue/Agones, read-only results and native client UI are
still outstanding; this handler alone is not #29 completion or live Steam acceptance.
Primary API reference: https://partner.steamgames.com/doc/webapi/ISteamUserAuth .

## #29 persistent authentication/profile contract, test first

A real PostgreSQL integration test requires upstream-verified account identity,
bounded opaque sessions, ticket-exchange replay rejection, strict request/loadout
validation, per-account profile ownership, persistence across pool/service restart,
logout and expiry. The test fails on missing openBackendDB/backendService before
implementation (backend-service-before.log). tests/backend_services.py owns a
private randomly named loopback database container with temporary credentials;
missing Docker/database is a failure, not skipped acceptance. No live Steam calls
are made: its HTTPS exchange fixture is explicitly local and SDK acceptance remains #180.

## #29 controller authenticated allocation

The controller consumes the v1 MatchSpec inside a private allocation envelope,
writes its join key/expected players through the existing atomic 0600 file writer,
and loads join configuration before clearing the warm-server password. Its existing
loaded-map UDP readiness probe now also checks the configured match ID; writing
stdin alone never marks a backend allocation ready. Legacy #28 password specs and
private persisted-spec revalidation remain supported. The match image now copies
the new shared contracts package explicitly.
Full Go race tests, including invalid private allocations and wrong/missing match
readiness, pass. The real warm controller/dedicated server gate passes with a local
SDK endpoint, followed by unsigned rejection/signed admission. Existing legacy
controller -> engine -> shipper -> durable stub completion also passes. The complete
match image builds as aftershock-match:issue29; production tidy and format/type/
boundary checks pass. Evidence: backend-allocation-after.log, backend-controller.log,
backend-legacy-controller.log, backend-container-build.log. No accepted fixtures changed.

Next persistent-tier decision: reuse Go net/http and database/sql, with PostgreSQL
for shared durable profile/session/party/queue state across HPA replicas. Use pgx's
existing database/sql driver rather than an ORM or custom protocol. Primary references:
https://github.com/jackc/pgx/wiki/Getting-started-with-pgx-through-database-sql and
https://www.postgresql.org/docs/18/transaction-iso.html. Driver metadata resolves to
v5.11.0; the reviewed postgres:18-bookworm manifest is
sha256:3725f4e2499eef5134592b3b4ab79a543ed7f8e533b05b5b637af926630f6650 (18.6).
No database service/driver implementation is added yet; write its failing integration
contract next. #29 remains read-only for match results; durable match writes stay #30.

## #29 controller allocation contract, test first

The existing Go controller test now requires a private allocation envelope carrying
the versioned MatchSpec, a per-match 32-byte key and separate ingest credential.
It checks canonical full-width expected players, required mode/version validation,
no key in process arguments, private 0600 join configuration and revalidation of
the persisted spec. It fails before controller support (backend-allocation-before.log).
Legacy password-based #28 specifications stay compatible. Authentication readiness
will be checked through the existing loaded-map UDP probe, not inferred from stdin.

## #29 signed UDP admission

SV_DirectConnect now validates the configured ticket after protocol/challenge checks,
rejects unavailable accounts/nonces before altering any existing slot, binds lost-
response retries to the same endpoint/qport/challenge and connected identity, and
applies the verified backend identity before sending connectResponse. The temporary
ticket is removed from saved userinfo and subsequent userinfo changes. No simulation
arithmetic or accepted golden changes. The probe uses the existing shared per-address
rate limits and filters ordinary sequenced packets out of handshake responses.
The actual loopback UDP gate passes with owned content, including endpoint rejection
without disturbing the original session, disconnect/reload replay rejection and a
fresh-ticket reconnect (backend-udp.log). A normal OpenArena pure-server/client join
also passes (backend-anonymous.log). Both compiler UBSan owners, existing identity
checks, production tidy and format/type/boundary checks pass. Next: connect the
versioned allocation contract to controller configuration/readiness and real client
HTTPS/UI; complete persistent services/kind acceptance before any #29 PR merge.

## #29 live admission contract, test first

The new backend_join_runtime.py launches our dedicated server on loopback with owned
content and encodes connect packets with the production Huffman implementation.
It requires fail-closed initial configuration, expected-player/time/signature checks,
identical lost-response retries, peer binding, rejection after disconnect/reload and
fresh-ticket reconnect. It also checks logs/userinfo do not retain credentials.
Before connection wiring, the test fails because an anonymous connect receives
connectResponse despite failed join configuration (backend-udp-before.log).

## #29 private join configuration

The server console command joinconfig reads bounded match-join.json through the
existing filesystem owner. Invalid initial input closes admission; invalid reload
preserves the last valid configuration; same-match reload preserves replay history.
Configuration accepts exactly the v1 match/key/expected-player fields, with unique
keys and full-width canonical account IDs. Secrets are never printed. The existing
agent JSON grammar/string helpers now have a shared qcommon implementation, retaining
the same validation behavior and 16-level nesting bound for development requests.
Both GCC and Clang UBSan backend/config and existing agent-protocol checks pass.
The full developer client/server build, production-flags tidy for all three affected
owners and format/type/boundary gates pass (backend-config-* and backend-json-agent-*
logs in the private modernization cache). Live UDP admission is the next test-first step.

## #29 join configuration contract, test first

Extend the existing backend server probe with private-file configuration checks:
invalid initial input must require authenticated admission and accept nobody;
valid v1 configuration installs atomically; unknown/duplicate fields are rejected;
failed reload preserves prior configuration; successful same-match reload preserves
used nonces. This is the next failing check before adding the console/file path.
Reuse the existing agent JSON validation/string routines in qcommon for both owners.

## #29 server identity owner

The new server join owner validates and atomically installs bounded match/key/
expected-player configuration. Reinstalling the same match/key preserves used
nonces; invalid configuration preserves the previous valid state. Verified joins
use SERVICE_BACKEND and retain the nonce for idempotent connection-response retries.
The identity owner distinguishes SDK and backend sessions for callback polling,
end-session cleanup and public identity reporting. It rejects a duplicate active
account and keeps the existing SDK timeout/revocation behavior. All actual callers
of SV_PlayerIdentity/GetPlayerIdentity are reviewed. Checkpoint serialization uses
its existing named checkpointClient_t fields and rejects authenticated identities;
no checkpoint file layout changes are introduced by the in-memory client fields.

Both compiler backend/server UBSan probes and existing identity/discovery probes
pass (backend-server-{gcc,clang}.log, backend-identity-{gcc,clang}.log). A full
developer client/server build and production-flags tidy for join.cpp, sv_join.cpp
and sv_identity.cpp pass. Format (580), types (441) and boundaries (443) pass.
This owner is not wired to the live UDP connect path or match controller yet;
actual configuration loading, handshake/retry behavior, client HTTPS/UI and kind
acceptance remain next. No accepted fixture changes.

## #29 server identity contract, test first

A new probe includes the actual server identity and new join owner. It requires
validated expected-player configuration, verified backend identity distinct from
Steam, duplicate-account rejection, same-handshake retry eligibility only while
connected, expiry and replay retention across disconnect and same-match config
reload. An SDK callback matching a backend session must not revoke it, and closing
a backend session must not invoke SDK EndAuth. Invalid configuration preserves
the prior valid configuration. This fails on absent sv_join.cpp before integration
(backend-server-before.log). Actual UDP handshake wiring remains a later gate;
the caller must also bind a retry to the same peer address/qport/challenge.

#24's existing console SDK gate is checkpointed on comment 5780638857: no console
SDK/dev kit is supplied, desktop/proxy checks remain enforced, and actual console
boot/replay stays outstanding. Continue #29/#30 as recorded; #180 separately owns
the maintainer-deferred Steam SDK/provider/live acceptance.

## #29 native ticket verification

The new portable qcommon owner verifies the exact shared compact contract using
the existing vendored SHA-256 implementation and bounded stack buffers. It checks
canonical decimal/hex encodings, full-width identities, key width, match and time
bounds, then compares every MAC byte before returning copied POD claims. Rejected
outputs are cleared. A fixed 256-entry per-match nonce store rejects duplicates,
fails closed when all entries are live and reuses only expired entries. It adds
no allocation or platform call and is registered in the existing source list.
GCC and Clang/libc++ UBSan probes pass against the independent Python signature,
including correctly signed invalid versions, IDs/overflow, times/lifetimes,
matches and nonce encodings (backend-native-{gcc,clang}.log). Targeted tidy is
clean; format/type/boundary checks pass. The signer/native verifier/nonce store
are not yet connected to actual client/server authentication. That integration
must check expected players and handle handshake retransmission without allowing
a new connection to reuse a consumed ticket.

## #29 native join-ticket contract, test first

Extend the same backend driver with a native UBSan probe using the independent
Python HMAC signature already checked by Go. It requires exact uint64 identity,
match/time/signature checks, cleared rejected outputs, trivial POD records and
bounded nonce retention. The replay store must reject duplicates and fail closed
when all 256 entries are live, then reuse expired entries without accepting the
same new nonce twice. This fails on the absent join_public.h/join.cpp before any
native implementation (backend-native-before.log). A server must preserve the
replay store across restarts within the same match and handle retransmitted
connect handshakes idempotently; those integration checks remain to be written.

## Current predecessor gate update

#21 PR177 is merged as main 7c24808f16f9a72d9ed21ba9af736cd54adba386 after
all 26 final-head jobs passed. Its tree c14c33d3 equals the tested 2cefd7e3 tree;
main build 35757322676/regression 35757322732 pass all 26 required jobs. #22 final head b23ef158
includes that main and starts build 35757405874/regression 35757405820. Earlier
#22 regression 35749778962 is cancelled as superseded. The initial #23 head's
16 compiler builds and nine completed regression jobs passed; remaining runtime
35752232064 is cancelled because the required final #22 main integration will
need all fresh checks. Neither cancelled run is merge acceptance. #23 must still
merge accepted #22 main and pass every final check before merging. No red/skipped
required gate is waived.

## #29 v1 contract implementation

The three versioned contracts now validate schema shape and semantic limits.
The Go ticket signer/verifier uses standard HMAC-SHA256 with a join-specific
prefix, canonical integer/hex encodings, match binding and an exclusive expiry.
An independently computed Python HMAC literal checks the actual bytes. A further
test first catches missing mode/rule fields being silently decoded as zero;
pointer presence checks distinguish required zero-valued fields from absence or
null. Player IDs reject zero, leading zeros and uint64 overflow; loadouts require
both bounded weapon paths and membership in the server-owned catalog. Expected
players must be unique and fit the match slots. Semantic cross-field constraints
are checked by Go in addition to JSON Schema shape checks.

The existing match controller delegates its unchanged bounded/unknown-field/
trailing-data JSON checks to the shared contracts package. Full Go race tests
pass, as does tests/backend.py with the private Python and Go environments
(backend-contract-after.log, backend-match-go.log). The missing-required-field
negative result is backend-required-before.log. No engine, service deployment,
account exchange or accepted replay fixture is changed by this slice.

## #29 initial contracts, test first

The public v1 contracts specify bounded match/rules/expected-player data, loadouts
that reference a server-owned weapon catalog and short-lived signed join tickets.
Player IDs are canonical decimal strings to preserve the full uint64 range in
JSON clients. Join tickets use a fixed canonical field sequence, match-scoped
HMAC-SHA256 with a 32-byte key, a 128-bit nonce and at most 120 seconds of lifetime.
The native server will reject reuse separately; signature verification alone is
not a replay guard. Session/backend and allocation secrets must remain separate.
The first tests require strict version/field/bounds validation, signature/time/
match/key rejection and weapon-catalog membership. Evidence:
backend-contract-before.log (missing schema), backend-go-before.log (missing API).
No accepted fixture changes or production service calls occur.

References reviewed: Go crypto/hmac (https://pkg.go.dev/crypto/hmac), the Agones
GameServerAllocation specification (https://agones.dev/site/docs/reference/gameserverallocation/)
and Steam ISteamUserAuth (https://partner.steamgames.com/doc/webapi/ISteamUserAuth).
Real Steam identity acceptance remains #180; CI identities must be explicitly
isolated test-provider identities, never represented as live Steam verification.

## #23 bounded service implementation

The existing provider table now exposes optional user/ticket, presence,
lobby/invite, achievements, cloud-file and workshop operations. The absent
provider stays anonymous/unavailable. Inputs and copied POD outputs are checked;
lobby request generations reject superseded/duplicate/late completions. Invites
never join automatically. No allocation, OS call, simulation or wire-layout
change is introduced. A fixed uint32_t event enum permits defined validation of
unknown provider event values under UBSan; the first implementation's unspecified
underlying enum triggered UBSan before its rejecting switch.

GCC and Clang/libc++ service probes and existing authenticated-identity/UI
discovery probes pass (services-{gcc,clang}.log and
services-identity-{gcc,clang}.log). Format (577 files), fixed-width policy (439)
and boundaries (440) pass. The new probe is included in both CI unit variants,
the workflow-derived local suite, affected-path selection and verification docs.
Affected-path selection, suite catalog, isolation policy and workflow lint also
pass. Targeted clang-tidy reports only advisory enum-size findings (four existing
enums and the explicit-width event enum). All new result records are checked as
trivially copyable. No accepted fixture is modified. This is interface preparation, not completed
Steam integration; SDK adapter, ticket transport and actual presence/invite
acceptance remain outstanding.

## #23 service interface contract, test first

Add one standalone probe that runs the same bounded user/ticket, presence,
lobby/invite, achievement, cloud and workshop operations against an absent
provider and a deterministic installed provider. Invalid caller arguments must
not dispatch; malformed provider records must not escape. Provider replacement
remains forbidden after use. The first run fails on missing serviceUser_t and
related interface functions (services-before.log). Actual Steam acceptance is
still separate and cannot be claimed from this probe.

## #23 preparation and external SDK boundary

The official Steamworks SDK download page requires a Steamworks login; no installed
SDK header was found in the project/cache locations. Valve's public Source SDK
repository also publishes the Steam API headers and redistributable libraries.
Keep that official reference only in private cache for API inspection, pinned at
b8cfb12c0e083a2ef5b2f9f9b50f3902fa034474 with verified Git blob hashes and a local
SHA256 manifest. No SDK files, credentials or binaries are vendored/published.
The public Source SDK license is scoped to Source-engine modifications, so this
reference is not used as the Aftershock build SDK. The adapter needs a proper
external Steamworks SDK root; default/null CI needs none. Manual callback dispatch keeps SDK events queued as bounded POD data
and never reenters Com_Error. Real Steam acceptance still needs an authorized AppID,
running logged-in clients and a designated invite recipient; do not invent them.
References: https://partner.steamgames.com/doc/sdk/api and the ISteamUser and
ISteamMatchmaking API pages. No external repository writes were performed.

## #22 combined AI validation

Local merge a203c2e4 incorporates final #21 branch 2cefd7e3 without accepting it
before hosted checks. The production merge is automatic; the progress conflict
retains both issues' evidence. Complete developer client/server rebuild, agent
protocol and developer data probes pass, followed serially by OpenArena overlay
controls/renderer restart/idle allocation/shutdown reporting and AI combat with
same/fresh-process checkpoint continuation. Evidence is
profiling-navigation-{build,agent,data,ui,combat}.log. The actual selected peak
capture attributes 370.939 ms of a 378.521 ms frame to events/commands; the
hierarchy remains readable after the AI inspector merge. Formatting (589 files),
type policy (448), boundaries (449) and targeted production-flags tidy on both
shared developer owners pass. No fixture changes. The accepted main merge and
all final-head/integrated hosted gates remain required.

## #22 local gate evidence

Full tidy passes 1426 production configurations (profiling-tidy.log). GCC/Clang
units and Clang sanitizer known-bug classification pass the unchanged unit hash
8d44421dfd5f31912bb7ffc942c6f0e1f32cd9a445e1dbcf38b658f555598ede. The one-ULP
negative control passes; no finished network command is rerun. Isolation, suite
contract, workflow lint and agent transport checks pass. The extended developer
runtime also passes local Quake 3 (profiling-runtime-q3.log). Lifetime analysis,
developer renderer modules pass (profiling-runtime-modules.log), as does the
unchanged OpenArena fixed replay 17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96.
The unchanged Quake 3 fixed replay also passes
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
Lifetime analysis passes 1356 commands across 151 paths. Final
UI review gives the plot its full labeled row and accounts for frame padding,
so click-to-retain indexes the plotted area instead of including the label width.
The final plot owner passes both static/module tidy configurations and a fresh
OpenArena runtime check; its capture is reviewed. Prepare a draft PR against main
for hosted feedback while #21 finishes; merge accepted #21 forward and rerun all
final checks before any #22 merge.

## #22 renderer and shutdown accounting contract, test first

Reuse the existing RHI draw counter and frontend triangle/surface counters; retain
them before the normal frame reset, alongside submitted entity count. Existing
GPU geometry/staging budgets expose their real allocations. Hunk tags reflect its
actual low/high permanent/temporary lifetime regions instead of inventing per-object
ownership. Extend existing developer probes for these snapshots and an explicit
shutdown retained-allocation report; process-lifetime cvars/arenas must not be
misreported as proven leaks. Test-first 3ff812a4 fails on missing renderer APIs.
The implementation passes both compiler developer probes, native command tests
and full developer build. A real OpenArena quit reports zero BOTLIB, RENDERER,
CLIENTS and DEVTOOLS blocks; remaining cached packs/process strings and permanent
hunk regions are shown rather than silently discarded. Runtime verifies the
report after clean exit. The developer renderer ABI advances 31 to 32; shipping
ABI 25 stays unchanged. Explicit frame selection is shared by the overlay and
JSON command, with retention/reset covered by probes and a reviewed capture.

## #22 packet and field contract, test first

Extend the same CPU/network probe with bounded packet history, timestamps/direction,
exact compressed-bit accounting for transmitted entity/player fields and player
arrays, reset and disabled instrumentation. Encode/decode real production deltas,
require matching read/write field totals and identical bytes with telemetry off.
This captures field payload/control bits, excluding message/header framing; it is
not a second network serializer. The missing API contract fails first at
8798763b (profiling-network-before.log). The owner now retains 256 packet records
and 128 fixed field slots without allocations; both compiler probes pass after
implementation. Production delta bytes match with telemetry enabled/disabled;
the accepted replication fixture also passes unchanged. The overlay exposes all
retained packet metadata and field bit totals; the command exposes the latest
64 packets plus all observed fields. Reset clears the counters explicitly. The
command probe and developer client/server build pass.

## #22 profile command contract, test first

Extend the existing native command probe: select the retained peak, expose parent
and self-time fields plus bounded history summaries, reject out-of-range age and
invalid booleans before a requested reset, and clear history explicitly. The old
handler ignores those arguments; the new check fails before implementation
(profiling-agent-before.log, a8ead056). The handler now exposes the retained
frame/parent/self-time/history and validates before reset. GCC protocol probe
passes (profiling-agent-gcc.log). The overlay consumes the same history, supports
click-to-retain, peak and live selection, and displays indented inclusive/self
timings. Explicit frame/pacing/event-command scopes extend existing server/client
scopes; no RAII or simulation expressions change. GCC and Clang/libc++ protocol checks and a complete developer client/server
build pass. Runtime overlay validation and remaining issue scope are next.

## #22 profiling contract, test first

The issue and existing instrumentation are reviewed. The engine already has
explicit generation-checked CPU scope tokens, a flat previous-frame view, RHI GPU
timestamps, zone tags/hunk totals, packet/snapshot/prediction totals, renderer
subsystem counters and lifetime-bounded debug primitives. Extend those owners;
do not introduce RAII guards across Com_Error or a second profiler framework.
The first contract adds parent indexes/self time and retained bounded CPU frame
history/peak attribution. Existing incomplete/stale/overflow/backwards-clock and
network controls remain. Actual overlay spike attribution without rebuilding is
required before #22 acceptance; optional Tracy must not become a mandatory build
dependency. The existing asset/memory probes pass, then the extended CPU probe fails on its
missing parent/self-time/frame APIs (profiling-history-before.log). No #22
production changes existed at that checkpoint (c4d6a9ff). The initial hierarchy
implementation passes that contract. Extend it before completion: external idle
time between frames must not inflate CPU duration, history must wrap at 240
frames while retaining the peak, and a finished child of an abandoned parent must
remain inspectable. The idle-duration assertion fails first
(profiling-idle-before.log); finalize time at the last completed scope instead of
the next frame boundary. The correction passes GCC and Clang/libc++ developer-data probes
(profiling-history-{gcc,clang}.log). The owner retains 240 POD frames, inclusive
and self durations, parent indexes, drop counts and the worst complete frame.
It uses existing explicit tokens and no dynamic allocation. Overlay/agent
consumption and production scope coverage remain next.

## #21 Windows public-header macro contract, test first

Corrected head d24ba4f4 reaches the developer client compilation and exposes
Windows min/max macro expansion inside the new shared sound-distance header
(C2589/C2059). Add the hostile-header condition to the existing audio spatial
probe before correcting the shared helper. Both actual callers (authored audio
and AI hearing) remain on the same implementation. Parenthesizing the function
name prevents macro expansion without changing arithmetic; GCC optimized
snd_spatial.o is byte-identical before/after. Both compiler spatial probes and
the complete developer client/server rebuild pass. The same hosted run then
reaches retained C99 bot/team-leader probes: new bool declarations require
stdbool.h when g_local.h is consumed as C. Add that conditional standard include
and verify bot byte conversion on GCC/Clang plus the team-leader check. All
remaining hosted native probe steps also pass on both compiler configurations
(navigation-native-tail.log). No accepted golden changes. Push the corrected
head and require every fresh required check before readiness/merge.

## #21 hosted portability and cold tool setup

Head 9fd94edb fails MSVC C4244 on two integer ternaries assigned/passed as
floats. Use exact float constants (0/1 and 8/16), preserving values. Both hosted
unit legs fail preparing the owned compiled level. Reproducing with a clean
cook-only Python venv and an unextracted pinned archive confirms the missing
level-tool Python dependency. Install the existing tools/level/requirements.txt
and libarchive-dev on those CI runners, matching runtime setup. The navigation
driver now prints its saved level log when compilation fails. No local system
packages or accepted fixtures change. Clean-cache full navigation cooking/native
checks pass after the dependency install (navigation-cold-after.log). Client/server
rebuild, format/types, workflow lint and all four native-controller tidy/lifetime
configurations pass. Fresh hosted checks remain required before PR177 can merge.

## #21 main integration

Merge #20 main 13106135 forward. Production changes merge automatically; resolve
adjacent verification/catalog entries by retaining both packages and navigation.
Keep both README sections and the earlier content checkpoint as history beneath
the current next action. No accepted fixtures change. Rebuild the combined
client/server and package/AI checkpoint integration pass, as do catalog/policy
checks. Draft PR177 is open against current main for its required hosted gates.

## #21 cooker header dependency contract, test first

Final review of the actual Ninja dependency records finds 12 engine headers in
the new collision cooker, but its content-hash input list only included one.
Extend the existing navigation test to simulate changed collision/platform header
bytes in memory and require tool-hash invalidation. No production file or accepted
fixture is modified. Test-first 27133f59 fails on the missing cm_local.h dependency
(navigation-header-before.log). Adding all 12 verified header dependencies makes
the complete navigation driver pass (navigation-header-after.log).

## #21 actual q3dm17 acceptance

The final private native run passes (navigation-q3-combat5.log and
navigation-q3-combat/actors.json): a complete six-corner route traverses two actual
push triggers, the data rifle damages the player, a return hit selects cover, and
the bot moves roughly 668 units into trace-protected cover against the relocated
hostile's last sensed position. The authored cover dwell is 20 seconds so the
inspector can observe completion. Both actor movement and weapon damage use the
unchanged native paths. The live AI inspector screenshot was reviewed.

All map extraction, observed landing data, cooked navmesh and trajectories remain
in the persistent private cache; no installed game archive or derived map asset
is committed/uploaded. Reproduction scripts there are navigation-q3-offline.py,
navigation-pads-observe.py, navigation-q3-combat.py. Owned CI geometry exercises
the same path/perception/weapon/checkpoint owners without proprietary content.
GCC/Clang navigation UBSan and both legacy bot smoke content sets pass unchanged.
Full tidy passed 1446 configurations, with the final two-line cover correction
rechecked for tidy/lifetimes in all four configurations (navigation-cover-recheck.log).
Full lifetimes pass all 1380 commands. Fixed demos retain Q3 frame hash
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4 and OpenArena
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(navigation-demo-{q3,oa}.log). The owned combat/checkpoint test also passes with
installed Q3 content (navigation-combat-q3.log). No accepted golden changed.

## #21 cover completion after memory expiry, test first

q3dm17's longer cover route takes more than five seconds at native crouch speed.
The bot reaches the selected protected point but the transient observation has
expired, incorrectly clearing covered. Extend the owned runtime contract to hold
cover for six seconds beyond arrival: target acquisition must expire, while
protection against the stored last-known threat remains true. The current client
fails that exact assertion (navigation-cover-memory-before.log). Reuse the
already checkpointed sense position; no new owner or movement change is needed.
Test-first e6ebaa21 fails before the two-line correction; the full owned combat
and same/fresh checkpoint run now passes (navigation-cover-memory-after.log).
GCC/Clang units retain hash 8d44421dfd5f31912bb7ffc942c6f0e1f32cd9a445e1dbcf38b658f555598ede.
Full pre-correction tidy passed 1446 configurations; recheck the changed game
owner after this final correction. Lifetimes and the remaining legacy gates run.

## #21 launch-trigger continuation contract, test first

The first actual q3dm17 AI run launches but its route cursor stays at the source:
the native trigger overlaps the actor before the endpoint arrival radius. Add a
contract that consumes the existing authoritative pad-touch bounds and advances
only the current matching launch link (including unrelated/invalid bounds and
ordinary-jump rejection). Also allow native ground steering after an off-center
landing; preserving pad velocity only applies while airborne. No movement or
impulse expression changes. Evidence: navigation-q3-runtime.log/actors.json. Test-first 2305f981 failed
on the missing trigger API before implementation. Both compiler UBSan navigation
suites now pass (navigation-trigger-{gcc,clang}.log). The native q3dm17 route
now finishes after two pad flights: 38 sampled airborne/landing phases and arrival
at sample 83 (navigation-q3-trigger-runtime.log). Inspector capture reviewed;
map-derived assets and trajectories remain private. Combat/cover acceptance on
q3dm17 is still running.

## #21 native combat and cover

After the committed failing combat contract in 1b248ac6, the controller feeds real
trace facts into sight/target memory and shared audio distance/occlusion sensing.
Only actual weapon-shot events mark audible sources. New attack commands release
the existing respawn latch before firing; an empty data-weapon magazine requests
its existing reload command. Behavior-driven cover selects reachable nav-boundary
points, crouches through unchanged Pmove, and reports covered only after reaching
the selected point with a current blocked threat trace. Perception, cover/replan
state and recent shot clocks have named checkpoint fields; no per-frame allocation.

OpenArena combat acceptance passes (navigation-combat-5.log): rifle damage to the
player, low-health rule after a player hit, more than 32 units of real movement
into protected cover, and exact same/fresh-process continuation. The controlled
spawns are baked into the owned scratch BSP so fresh reconstruction sees identical
content; a temporary developer entity override correctly failed that identity
check. Frozen pre-AI migration still passes. Inspector capture reviewed. GCC state
suite and format/type/boundary gates pass. The explicit occluded-shot runtime also passes (navigation-hearing.log): gain
0.2996 at 260 units matches shared linear attenuation times 0.35 occlusion, a
complete investigation route causes real movement, and checkpoint continuation
remains exact. Full post-combat analysis and q3dm17 acceptance remain pending.

## #21 navigation checkpoint owner

After a67d9c69's failing full-game contract, typed records own navmesh/behavior
content identities, controller clock, each bot's behavior state, route corners,
authored link kinds, cursor/phase and goal. Both latched asset selections use the
existing cached-cvar owner. Decode validates all actors before publication;
navmesh bots have their own connection/spawn reference validation and no legacy
bot handles. Missing pre-AI records migrate to the legacy controller; present
invalid records fail through the tested presence API.

Same-process and fresh-process paused restore and 25-tick continuation pass with
exact actor/weapon/route data (navigation-checkpoint-fresh.log). The full existing
state suite passes GCC and Clang UBSan (navigation-state-{gcc,clang}.log), and the
legacy runtime suite still passes interrupted/replaced reconnect, fresh-process
continuation and the unchanged frozen-v1 fixture (navigation-legacy-checkpoint.log).
Tidy passes 1446 production configurations and lifetimes pass all 1380 commands.
The full MinGW client/server/devtools build passes. Loading the unchanged frozen
v1 fixture from an active AI session also passes and clears both selected AI
assets (navigation-checkpoint-migration.log). Combat/cover and q3dm17 AI traversal
remain pending.

The extended --combat runtime contract fails as intended on the patrol-only
client: the visible hostile does not trigger data-weapon fire
(navigation-combat-before.log). It requires real player damage, a low-health
transition, movement into trace-protected cover and exact checkpoint continuation.
Commit this test before connecting the existing perception and weapon services.

## #21 live AI checkpoint contract, test first

Extend native patrol acceptance with a paused full-game save, 25 live ticks,
restore, and exact actor/weapon/route/behavior continuation. The new AI owner and
its two latched cvars are not yet serialized. Run this contract against the
patrol client before implementing the owner. It fails at checkpoint capture
(navigation-checkpoint-before.log); the existing owner validation cannot account
for a connected bot without a legacy bot-state allocation. Preserve all accepted
fixtures and add explicit navmesh actor ownership.

## #21 optional owner lookup

After e6ba42de's failing contract, State_Find can optionally report whether the
named slot exists even when its schema is rejected. Existing callers retain
identical behavior. The engine/native field/migration probes pass under GCC and
Clang UBSan (navigation-state-core.log), including missing versus invalid owner
records. This supports pre-AI fixture migration without changing old schemas or
regenerating fixtures. Full AI owner/cvar continuation is still being connected.

## #21 checkpoint migration presence contract, test first

The new AI owner needs an explicit absent-record migration for frozen pre-AI
checkpoints. Extend the existing state probe so lookup reports whether a named
record exists independently of schema success: a missing owner may migrate,
but an existing undecodable owner must fail. This avoids treating malformed new
state as an older save. Commit the failing optional-presence API contract before
implementation; syntax compilation fails at the absent presence argument
(navigation-state-presence-compile-before.log). The full state driver separately
stops at its expected unowned g_navigation/g_behavior guard until AI cvar
ownership is connected. No fixture regeneration or existing schema change is
needed.

## #21 first native patrol integration

Wire the existing navigation/behavior/perception owners and pinned Detour sources
into client/server builds with strict FP options. Cook index kinds 15/16 are
recognized; navigation and behavior load through existing filesystem services.
Latched g_navigation/g_behavior opt into the new controller; ordinary bot matches
keep their existing path. The controller emits the existing bot input/usercmd
format and Pmove remains movement authority. New bots bypass legacy AAS client
setup; a runtime run with the owned map's AAS removed passes.

After 7d40e273's failing runtime contract, the OpenArena native patrol moves about
1195 units over complete three/four-corner routes with the data-driven rifle
active. Actor telemetry and the AI ImGui panel expose state age, transitions,
route cursor and colored path segments. The inspector capture was viewed.
Evidence: navigation-runtime-patrol2/ and navigation-runtime-no-aas/; Linux client
and server build and format/type/boundary checks pass. This is initial patrol,
not combat/cover, q3dm17 traversal or checkpoint acceptance; those are next.

## #21 private map-pad observations

Observed all 13 existing q3dm17 pads through current-main native movement. Twelve
produce a measured landing; pad *14 has no settled landing within 500 sampled
frames and is excluded from this temporary link recipe. Cooking those twelve
local observations increases complete spawn-pair routes from 27 to 58 (52 remain
partial), with 531 cover candidates, 11 valid steering queries and no query
allocations. Evidence: navigation-pads-observe/ and navigation-q3-pads-{cook,query}.log.
BSP, trajectories and derived navmesh stay private; no map bytes are committed or
uploaded. These are preparation measurements, not native AI gameplay acceptance.

## #21 native runtime contract, test first

Add tests/navigation_runtime.py using the existing owned two_lane level compiler,
a cooked patrol behavior/navmesh, and the accepted weapon/animation asset sources.
The OpenArena current-main client starts the map and native bot, then fails at
absent actor AI state as intended (navigation-runtime-before.log). The remaining
contract requires actual Pmove displacement over 128 units, a complete navmesh
route, the configured data weapon and an AI inspector capture. Commit before
connecting the existing core owners to game lifecycle and developer telemetry.
Combat/cover, q3dm17 traversal and checkpoint continuation remain later acceptance
steps; this initial patrol test does not claim them.

## #21 authoritative route following

After 872f868f's failing contract, Nav_Follow keeps an eight-byte POD corner/
phase cursor. It selects bounded ground/off-mesh steering targets from actual
feet and grounded state, waits for a real map-pad launch and landing, and never
moves an entity. Jump commands end in flight; door/drop targets remain explicit;
partial routes never report arrival. Both compiler UBSan suites pass copied-
cursor continuation and zero tick allocations (navigation-follow-{gcc,clang}.log).
Isolated lifetime and the configured tidy error policy pass for the owner; the
existing advisory warnings remain. Full production integration gates are pending.

## #21 authoritative route-following test first

Extend the native allocation-counted probe with authoritative foot positions and
POD route cursor continuation. Ground corners advance normally; a pad source
must wait for a real airborne transition, then its landing; ordinary jump/drop/
door actions keep distinct output. A partial route must never report arrival.
The contract is deliberately independent of Pmove: it selects a steering target,
while existing native user commands retain movement authority. Compilation fails at the absent navFollowState_t/Nav_Follow API
(navigation-follow-before.log). Commit this contract before implementation.

## #21 checkpoint-enabled main forward merge

Merge main b92b6ef5 after the isolated core preparation. Source lists and the
collision public API combine cleanly; retain current PR174 acceptance and both
issues' detailed histories when resolving the documentation conflicts. Recheck
the cooker/native driver against the new CM/shared sources before committing the
merge. The first combined cooker link fails because CM's new portal checkpoint
helpers depend on State_Append/State_Find (navigation-state-base.log). Link the
existing small state/SHA owners into the offline helper, and apply C++-only
compiler options by language so the vendored SHA file remains C. Include those
sources in the existing tool dependency hash. This does not bypass #20 or claim
runtime gameplay integration. The combined native/cook driver passes
(navigation-state-base-after.log); format (582), types (442) and boundaries (443)
also pass before the merge commit.

## #21 off-mesh action metadata and observed pad link

After 6bf8f3b9's failing test, native waypoints expose the cooked jump/drop/door/
launch action alongside the authored ID. The added launch kind represents an
existing map impulse rather than synthesizing a gameplay jump. Both compiler
UBSan drivers pass ordinary-jump and incrementally recooked launch cases. Cooker
limits now reserve capacity for off-mesh polygons/vertices and the outer envelope,
so accepted output stays within the native tile/file bounds.

A private link for the observed q3dm17 *5 pad increases complete spawn-pair routes
from 24 to 27 (83 still partial). All 11 steering queries and 531 cover candidates
remain available with zero tick allocations (navigation-q3-launch-{cook,query}.log).
The derived assets and trajectory stay private. This is route/action preparation;
actual native bot movement, pad execution, combat and checkpoint continuation
are still required and no #21 PR is open.

## #21 off-mesh action identity, test first

A private current-main client observation confirms q3dm17's *5 jump pad lands
on the upper platform after 85 sampled frames at dt=20 ms. Source pad bounds
center at (-32,-672,338); the observed first landing origin is approximately
(-125.100327,-772.106201,600.125), with floor at 576. Game movement was untouched.
Trajectory/log evidence stays in navigation-pad-observe/; no map bytes are
committed or uploaded. The first telemetry row predates queued teleport execution;
use the first positive launch-velocity row, not that stale row, as launch evidence.

Native routes currently expose link IDs but not their action kind. The extended
probe requires jump versus existing-pad launch identity, and the driver adds a
launch-kind incremental cook. Compilation fails on absent link kind metadata
(navigation-link-kind-before.log). Commit first, then expose the existing cooked
area kind and add an explicit launch kind; ordinary jump/drop/door remain distinct.

## #21 stateless steering and real-map query evidence

After 7461cf58's failing contract, Nav_Avoid uses a preallocated Detour avoidance
query reset for each authoritative actor snapshot. Nearby navmesh wall segments
and up to 64 actor circles feed the existing crowd sampling parameters. Both
compiler UBSan tests pass oncoming-actor avoidance, identical fresh-world and
intervening-query results, and zero allocations after initialization
(navigation-avoid-{gcc,clang}.log). Tidy, lifetime and strict MinGW/aarch64
compilation pass for the changed owner.

Private q3dm17 queries cover all 11 installed spawn positions: 24 complete routes,
86 explicitly partial routes, 531 nearby cover candidates and 11 valid steering
queries, with zero query allocations (navigation-q3-query.log). The disconnected
platform routes still need authored off-mesh traversal; do not describe these
numbers as full-map traversal or gameplay acceptance. No game asset/derived map
output is committed or uploaded.

## #21 checkpoint-compatible steering decision and test

Gameplay will use Detour local obstacle avoidance from current authoritative
positions/velocities, alongside bounded explicit paths and POD behavior/perception
state. Do not introduce hidden asynchronous crowd/path-queue state into full-game
checkpoint continuation. The existing persistent crowd API remains available and
tested; gameplay movement still goes through native usercmd/Pmove.

The new probe requires avoidance of an oncoming actor, bit-identical output in a
fresh nav world, and identical output after an unrelated intervening query. Both
worlds initialize before the allocation counter; no tick allocation is allowed.
Compilation fails at missing navObstacle_t/Nav_Avoid
(navigation-avoid-before.log). Commit before the direct avoidance implementation.

## #21 navmesh cover candidates and real-map offline cook

After 209a8623's failing query contract, nearby ground polygons expose inward
boundary points with stable polygon/edge identities. Results are distance-sorted
and bounded to 256 polygons/points; capacity exhaustion is explicit. The caller
still tests current threat occlusion and route reachability. Both compiler UBSan
runs find reachable cover behind the owned collision brush and retain identical
query IDs/positions with no allocations (navigation-cover-{gcc,clang}.log).
Isolated navigation tidy passes; all three navigation/behavior/perception owners
also compile with strict warnings on local MinGW x86_64 and aarch64 compilers.

A separate private cook reads installed q3dm17 BSP content and produces 1,282
polygons, 2,087 vertices, no authored links yet, and a 189,236-byte .asnav file
SHA256 d61e7f20fa8663669b3236c42d9ce55e3e3f2c58ce648f8670cc9e8dbcff97aa
(navigation-q3-offline.log). These derived bytes and the temporary BSP stay in
the private cache and are not committed/uploaded. This proves real-map cooking,
not native bot gameplay or map traversal acceptance.

## #21 collision-derived cover candidate test

Extend the existing native navigation probe to require bounded, repeatable cover
points from nearby navmesh boundaries, including a reachable point behind the
owned low-cover collision brush. Candidates must remain within the requested
range and retain stable identities on repeated queries. Existing allocation
counters also cover this query. Compilation fails on missing navCoverQuery_t/
Nav_CoverPoints (navigation-cover-before.log); commit before implementation.

## #21 isolated perception and cover selection

After 1d4b1d6f's missing-owner failure, bounded POD perception selects the nearest
visible hostile with stable identity ties, then the loudest audible hostile,
then a time-limited remembered position. Supplied trace facts combine with the
sight cone; dead/friendly observations invalidate remembered targets. Hearing
shares authored audio's linear/inverse gain and settled occlusion amplitude.
Cover selection chooses the nearest reachable point protected from the threat.
Game trace generation and navmesh cover candidates remain to be integrated.

Both compiler UBSan navigation/behavior/perception suites pass, as do isolated
perception tidy/lifetime checks. Existing audio spatial probes pass GCC and
Clang/libc++, and the authored sound-event suite passes. Sharing the occlusion
helper leaves GCC -O2 snd_event.o byte-identical. Spatial instruction scheduling
changes after helper extraction, but 100,000 original/current spatial outputs
are bit-identical, including both attenuation models and randomized positions/
velocities. Evidence: navigation-perception-{gcc,clang}.log, navigation-audio-
compare.log and navigation-audio-events.log. No accepted sound/demo artifact,
authoritative simulation expression, allocation or OS access changed.

## #21 perception/cover contract, test first

The native probe requires sight cone plus supplied collision visibility,
nearest-hostile selection with stable identity tie breaks, audio-model hearing
through occlusion, exact memory expiry/copy continuation, and nearest reachable
cover protected from the threat. It fails on absent perception_public.h/
perception.cpp after existing navigation/behavior tests pass
(navigation-perception-before.log). These helpers consume bounded trace facts;
real game trace production and movement remain separate acceptance requirements.
Before sharing audio math, retain original GCC -O2 spatial/event objects in the
private cache for a before/after code-generation comparison. No audio model or
accepted sound fixture change is intended.

## #21 isolated hierarchical behavior execution

After 7f7a3f60's missing-runtime failure, fixed-capacity POD behavior/state records
load the authored hierarchy and execute one ordered transition per tick. Leaf
rules precede inherited parent rules; dwell time is measured in the current leaf,
and state elapsed/transition counters saturate without wrapping. Loader validation
checks table extents, names, hierarchy depth/cycles, leaf targets and comparison
ranges before publication. No allocation or OS calls are introduced.

GCC and Clang/libc++ UBSan now pass the complete navigation/behavior driver,
including exact dwell/timeout boundaries and identical copied-state continuation
(navigation-behavior-{gcc,clang}.log). Isolated owner clang-tidy and AST lifetime
checks pass (navigation-owner-{tidy,lifetime}.log). These isolated checks do not
replace final production-configuration gates after engine/gameplay integration.
Perception, cover/target selection, real bot movement/weapons, ImGui inspection
and #19 checkpoint integration are still outstanding. No #21 PR yet.

## #21 native behavior contract, test first

The cooked hierarchy now has a native probe requiring leaf-before-parent
transition priority, inherited loss-of-sight behavior, minimum dwell times,
exact timeout boundaries and identical continuation from a copied POD state.
The initial compile fails on missing behavior_public.h/behavior.cpp after all
existing cook/navmesh/crowd assertions pass (navigation-behavior-native-before.log).
Commit those assertions before the runtime state-machine implementation.

## #21 behavior cooker

After 078aebcd's missing-kind failure, the behavior cooker emits the existing
version/hash envelope around bounded named states and ordered transition tables.
Parent identities, cycles/depth, leaf targets and field-specific comparison
ranges are validated. The agent schema/describe API includes the format, and
the development index reserves kind 16. Native transition execution is not yet
implemented. The complete navigation driver passes its existing query/crowd
checks and the new incremental behavior edit (navigation-behavior-after.log).

## #21 behavior source contract, test first

Extend the navigation acceptance driver with a data-authored guard hierarchy:
patrol/investigate leaves and attack/cover children of a combat parent. Ordered
leaf transitions handle health/cover, while both combat children inherit the
lost-target transition. The source also carries minimum dwell times and a timed
return to patrol. It must cook incrementally without rebuilding the collision
navmesh; an edited threshold must change only its own asset.

Existing native navigation assertions pass first, then cooking fails at the
missing behavior asset kind (navigation-behavior-before.log). Commit this source
contract before the cooker; native transition/perception/cover assertions remain
necessary before those runtime pieces. #19 is now in its final UBSan runtime step.

## #21 isolated native navigation owner

After e8a12794's missing-owner failure, the opaque POD owner copies/validates the
pinned single-tile envelope and initializes Detour query/crowd capacity through
zone allocator hooks. Public coordinates remain engine Z-up. Ground paths and
explicit authored off-mesh IDs use bounded caller results; crowds provide
steering only, with authoritative usercmd/Pmove integration still outstanding.
No engine build or gameplay owner uses this preparation yet.

The native test exposed Detour omitting a link marker when the route starts at
the link's exact endpoint. Funnel ground segments separately around each
corridor off-mesh polygon so the authored action survives a zero-length approach.
The same test now passes, without weakening its assertion. GCC and Clang/libc++
UBSan runs pass routes across the owned map, explicit link IDs, two opposing
agents reaching their destinations without interpenetration, slot reuse and
complete cleanup. Allocation counters remain unchanged after world creation.
Evidence: navigation-native-{gcc,clang}.log. No accepted golden changes.

Limits: one tile up to 16 MiB, 32,768 polygons, 256 authored links, 256 returned
waypoints and 64 preallocated agents. Tile counts, section sizes, geometry/detail
references, BV escapes and off-mesh identities are validated before Detour pointer
fixups. Partial routes/capacity exhaustion are explicit in the result. Compiler,
format/type/boundary checks cover this slice; full integration/tidy/lifetime and
real gameplay acceptance remain after accepted #19/#20 main is merged forward.

## #21 native navigation contract, test first

The existing navigation driver now compiles a native functional probe after its
owned collision cook. It requires a route across both lanes/height transition,
the authored off-mesh link ID, two opposing agents reaching their goals without
interpenetration, reusable slots, explicit cleanup and no allocations after
world creation. The owner must copy the caller's asset bytes before mutable
Detour initialization. Public result values must remain trivially copyable.
The initial build fails on missing navigation_public.h/navigation.cpp
(navigation-native-before.log). Vendored compilation retains its warning policy;
owned probe/runtime compilation requires -Wall -Wextra -Werror and UBSan.
No gameplay integration or completed #21 acceptance is claimed.

## PR174 merged-tree acceptance complete

Fresh job evidence confirms main 5caa2c1c passes all 16 compiler builds in
35715942160 and all ten active regressions in 35715942143 attempt 2. #31 is closed
again after its acceptance comment; docs/bugs.md records PR174 and both exact-head
and integration runs. The runtime-only rerun preserved all tests after the first
attempt's advancing-gameplay timeout. No suppression or accepted golden changes.
PR175 is still running; do not substitute predecessor gates for its own runtime.

## #21 first collision-navmesh cooker

The initial cook contract now passes (navigation-first-after.log). The offline
helper links the actual CM loader, uses the explicit solid/player-clip exporter,
rotates engine Z-up coordinates into Detour Y-up, then runs Recast voxelization,
clearance/regions/contours/detail generation and Detour tile creation. Source JSON
has bounded agent settings and named off-mesh link IDs/kinds; the existing agent
schema/diagnostic API exposes it. The cooker records both source and collision
BSP hashes and skips/recooks reproducibly. Owned map outputs only; no accepted
fixture or proprietary archive is copied into the repository.

The .asnav envelope retains the shared 48-byte version/hash header, followed by
64 bytes of BSP SHA256, six agent/cell floats, tile length and CM checksum, then
the pinned Detour tile. Native inputs have explicit sizes/offset assertions.
The cooker development index reserves kind 15; runtime loading/reload handling
is not implemented yet. Existing tests remain the acceptance boundary, not the
mere presence of a cooked file. Next add native query/crowd/off-mesh tests before
runtime owners, then gameplay/perception/behavior/tooling on accepted #19/#20 main.

## #21 collision export mask implementation

After 83a9fdd4's failing compile, add the explicit-mask overload and preserve the
old two-argument solid-only wrapper. The same brush/patch clipping code now serves
navigation's solid/player-clip mask; no expressions or native movement/trace code
are restructured. GCC and Clang/libc++ UBSan probes pass the original and added
mask assertions (navigation-collision-after.log, navigation-collision-clang.log).
The navigation asset cooker remains unimplemented; no gameplay acceptance yet.

## #21 collision export mask, test first

The existing physics collision probe now requires an explicit navigation contents
mask. A player-clip brush must stay excluded from the existing cosmetic default
but be included alongside solid patches when requested. Original winding area,
inline-model exclusion, deduplication and cleanup assertions remain. The targeted
compile fails on the absent three-argument export overload
(navigation-collision-before.log). Keep the old two-argument symbol/default and
reuse the same clipping implementation; no authoritative CM trace code changes.

Predecessor update: #20's first runtime failure is an extra search-path line for
an absent optional engine directory, not gameplay divergence. af524559 fixes it;
both unchanged OA bot hashes and complete package/pure/render acceptance pass.
New build 35725046713/regression 35725046710 are running/queued. PR174 integration
is in its final sanitizer/replay checks; PR175 remains in level authoring.

## #21 pinned dependency preparation

After the missing-kind failure was committed at 4c7fa084, import unmodified
Recast/Detour/DetourCrowd source/header subsets at the verified v1.6.0 commit.
third_party/recast/provenance.json records every copied hash and archive identity;
the original zlib license/notices remain. No demos, sample assets or unneeded
TileCache/DebugUtils modules are imported. No build/runtime integration exists
yet, and the initial cooker test still fails as expected.

## #21 collision-navmesh source contract, test first

Issue #21 has no additional comments. tests/navigation.py compiles the owned
three-room/two-lane map using the existing level tool, then requires a navigation
asset whose source includes that actual collision BSP plus agent/off-mesh settings.
It checks transitive manifests, incremental skipping and deterministic recooking.
The map compiles; cooking fails at `asset kind is not implemented yet: navigation`.
This is the initial source/output contract only; it does not yet prove native
pathfinding, off-mesh execution, crowd avoidance or gameplay acceptance.

Inspection found existing CM_PhysicsTriangles world brush/patch export. Reuse it
instead of deriving navigation from visible render surfaces; account for playerclip
explicitly without modifying authoritative CM/Pmove calculations. Recast/Detour is
explicitly requested by the issue. The official latest tagged release is v1.6.0,
peeled commit 6dc1667f580357e8a2154c28b7867bea7e8ad3a7, tag object
b4554541b658630816dba41466eb1cefb624519e. Its downloaded source archive SHA256 is
f565cc91b85df95a656cfc672e41c02e8aa44ba2363905aa8277ce20ea87491d; the zlib license
is retained in the private source cache. No dependency is vendored yet.
Sources: https://github.com/recastnavigation/recastnavigation/releases/tag/v1.6.0
and https://github.com/recastnavigation/recastnavigation/tree/6dc1667f580357e8a2154c28b7867bea7e8ad3a7.

## #20 draft-feedback workflow decision

Both complete local unit variants pass (content-unit-suite/suite-report.json
ok:true, full:false because only unit jobs were selected). Linux/MinGW builds,
full lifetime/tidy coverage, both-content package/pure/render acceptance, existing
bot goldens and both fixed replays have passed. No accepted golden changed.

Earlier notes kept #20 local while waiting for #19. With local acceptance complete,
open a draft into current main for early hosted compiler feedback instead of
waiting idle. This does not change merge order: #19 must merge first, then its
current main must be merged forward into #20 and fresh final gates must pass.
Any checks on the older base are preliminary evidence only. Self-review finds
only #20 package/root/test/docs changes, no authoritative simulation edits, no
new non-trivial core owners and no new per-read/seek allocations.

## #20 CI evidence retention

The runtime job retains only package logs, screenshots and size JSON, including
pure-session logs; package contents and game archives are not uploaded. The
AGENTS/tests/design acceptance commands include the dedicated server so the
pure-session portion is explicit. GCC's complete unit-job variant passed; Clang
is still running. PR174 integration has advanced beyond level authoring; PR175
remains in its level-authoring step. Continue monitoring both, with no red/skipped
required-check merge or main-base shortcut.

## #20 offline/native limit agreement

Self-review aligns offline mount limits with the native 64-package/65,536-visible-
asset limits and uses a set for removal membership during writing. The complete
functional package check still passes (content-native-final.log); artifact bytes
and accepted fixtures are unchanged. GCC's full unit job variant has passed; the
Clang/libc++ variant is running in content-unit-suite. No #20 PR is open yet.

## #20 final local gate checkpoint

Full lifetime analysis passes 1,320 production compilation commands/147 paths,
including shipping/devtools, static/module and all controls. Tidy covers 1,382
configurations with the separately recorded eight-filesystem recheck after the
single tool crash. Fixed Q3 and OpenArena replay hashes remain exactly
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4 and
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96.
Evidence: content-lifetimes.log, content-tidy/results.json,
content-tidy/recheck-results.json, content-demo.log, content-demo-oa.log.
The runtime test also now proves that packaged autoexec/q3config entries do not
replace loose user configuration; full OA package/pure/render acceptance passes
again. The full local unit job variants run via tests/suite.py under
content-unit-suite. PR175 and PR174 integration still await hosted runtime.

## #20 filesystem implementation and local acceptance

Native packages reuse the existing search-pack/hash/handle interface. File reads,
seeks and explicit close use the platform stream; modern metadata is excluded
from the legacy zip cache. Sorted independent/patch/DLC/mod mounts validate their
composed SHA256 view. Removal entries stop lower package/loose lookup and listing.
Reserved user configs keep the legacy archive exclusion. Existing pure checksums
bind modern identity/metadata; missing modern artifacts require installation,
while legacy pk3 downloading and reading retain their existing path.

Engine data is under fs_enginepath/engine, game data under fs_basepath/game, writes
under fs_homepath/active-game. Windows now uses the static OS application-data API
for its user default, replacing the inactive optional profile path. Linux/macOS
keep their existing home defaults. Missing home requires an explicit override,
never an implicit install-directory write. Explicit portable equal-root launches
remain supported. No simulation arithmetic or accepted fixture/golden changes.

Both Quake 3 and OpenArena pass roots, patch/removal/DLC/mod precedence, filesystem
restart, user writes, legacy gameplay and matching-package sv_pure client/server
sessions (content-mount-final-q3.log, content-mount-oa.log). The actual owned
character packages into 4,373 bytes; a 1,455-byte texture delta changes 1,518 native
preview pixels in fresh clients. Screenshot reviewed. The first native mount run
caught a missing legacy-cache exclusion; corrected within this unmerged feature.
The test setup also needed normal console `set` for new cvars and session setup
before stepping. Its corrected test fails the old binary specifically at the
missing engine mount (content-mount-before.log).

Linux client/server and MinGW client/server builds pass. Core unit golden plus
one-ULP negative control and both Q3 bot hashes pass unchanged (content-unit.log,
content-bots.log). Format/type/boundary and suite/affected/actionlint pass. Tidy's
full run had one tool crash while files.cpp was being edited; after stabilizing
that file, all eight of its production configurations pass (content-retidy.log).
The original full report and separate recheck report are retained. Lifetime and
fixed replay gates remain outstanding; no hosted acceptance is claimed.

CI registers the package probe on both compilers and real OA package rendering/
pure checks in runtime. AGENTS/tests docs and docs/design/packages.md describe
commands, roots, mounting, limits and installer-only modern content. Keep #20
local until #19 merges; final checks must include that current main.

## #20 filesystem acceptance, test first

The real-engine test packages owned configuration files in separate engine/game/
user roots. It requires a base/patch view, tombstone hiding of lower loose files,
additional DLC mounts, filesystem restart, user-only writes and playable legacy
pk3 maps. The accepted binary fails at the absent engine package mount
(content-mount-before.log). Commit the test before filesystem integration.
Implementation may proceed while #19's last runtime job runs; #20 final gates
must include accepted #19 main before its own PR can merge.

## #20 native stream implementation

The production reader validates fixed layouts, canonical metadata hashes and
ordered patch identities, then verifies each complete asset before exposing bytes.
Stored assets use 64-bit platform offset reads; compressed assets reuse puff with
a bounded zone buffer. Read/seek do not allocate. Explicit close/free releases all
resources. GCC and Clang/libc++ UBSan pass (content-native-after.log,
content-native-clang.log). Raw DEFLATE packages measure 14,949 bytes for the base,
2,077 for the one-texture delta and 460 for removal. No new dependency.
The filesystem has not mounted these packages yet; this is not #20 acceptance.

## #20 native stream contract, test first

The package test now compiles the future native reader and platform file stream,
requiring stored/compressed bytes to match cooked files, seek/tell accuracy,
matching patch identities, removed entries, no read/seek zone allocations and
complete handle/allocation release. Its initial compilation fails on absent
package_public.h/package.cpp/sys_content_file.cpp (content-native-before.log).
Commit this contract before implementing those owners.

Inspection found that the engine ships puff, not a public incremental zlib API.
Use raw DEFLATE through that existing decoder: compressed assets decode/hash once
at open into a bounded buffer; stored assets stream through platform offsets.
Audio can explicitly remain stored. The offline codec and format doc now agree,
and the tool-side exact patch assertions still pass before native compilation.
This changes only new, unaccepted #20 artifacts; no accepted fixtures are touched.

## #20 offline package and manifest-diff implementation

The writer uses Python's existing SHA256/zlib facilities, a fixed 128-byte header
and index entries with 64-bit extents, per-asset uncompressed hashes and canonical
JSON manifest. New/changed payloads and explicit removals form patches; required
base and resulting identities bind each diff to the current mounted view.
Stored and compressed assets coexist; source recipe manifests stay offline.
Atomic publication preserves previous output on failure, and extraction refuses
an existing destination. No new dependency or accepted asset/golden changes.

The owned-content test passes reproducible packaging, exact texture patch and
removal views. This is not native mount acceptance. The implementation/spec remain
local until #19 is accepted and main is merged forward.

## #20 first packaging acceptance test

Reuse tests/cook.py's owned source generator and the actual cooker. Patch a
separate texture source so exactly one cooked texture and its index/revision
change. The delta must omit unchanged model/audio payloads and stay below half
the full package size. Applying base plus patch must reproduce every cooked file
hash; a following deletion patch must hide the removed configuration file.
The first run fails on the missing packaging CLI after cooking succeeds.
This is tool-side preparation only; platform streams, native mounts, precedence,
directory separation and legacy compatibility remain required by #20.

## Historical #20 pre-acceptance checkpoint

#19 PR175 is merged at main b92b6ef5 after all 26 exact-head jobs passed; its
merged build 35728186900 passed and regression 35728186799 is still running.
Require merged acceptance before updating #25's checkbox or merging #20.

#20 PR176 includes that main at b2c2ae5e. Self-review found possible signed overflow
in the new modern-package relative-seek branch: on 64-bit-offset platforms,
INT64_MAX plus a nonzero current position overflows before its bounds check.
Test-first 8702f1d3 extends the existing allocation-counted package probe. The
package owner now checks the signed offset against its bounded extent before
addition; the filesystem delegates all modern-package seek origins to that owner.
GCC and Clang/libc++ UBSan, all 16 changed-owner tidy configurations, format/type/
boundary checks, client/server build and full OpenArena package runtime pass
(content-seek-*.log). Invalid seeks retain position and allocate nothing.
Push the new head and require all 26 fresh exact-head jobs plus accepted #19
integration before self-review/readiness/merge. Do not reuse b2c2ae5e gates as
final acceptance. No accepted content fixture changes.

#21 remains isolated in navigation-tree (issue/21-ai-navigation), with tested
cooking/routes/crowd/cover/stateless steering/behavior/perception and a failing
native runtime patrol contract. Merge accepted #20 main forward before gameplay,
ImGui and checkpoint integration. Continue the full roadmap; no maintainer input
is needed and a CI wait is not a stopping point.

## #19 merge and #20 combined-tree checkpoint

PR175 self-review is recorded at issue19 comment 5776614749. All 16 compiler and
ten active regression jobs pass, including checkpoint/profile migration, native
bot/replay goldens and sanitizer checks. The merge commit preserves exactly the
tested tree. #19 is closed by the PR; #25's acceptance checkbox remains pending
until the merged-tree gates complete. No accepted golden or fixture changed.

The #20 forward merge retains package mounts/streaming, loose-user save storage,
both state/package affected-test routing and both acceptance commands. Filesystem
conflict is only the package header versus <cerrno> includes; both are needed.
The combined client/server build and policy/test-routing checks pass. Runtime
package/pure/render, profile and frozen checkpoint migration are running in that
order. Push this merge for final hosted gates while these finish; neither the
preliminary head nor a partial local result is final acceptance.

## #19 MSVC profile output initialization

Build 35719871675 reaches a remaining C4701 in profile loading: MSVC does not
prove that the successful State_Read out-parameter is initialized through the
stored valid flag. Initialize the version output to zero; valid reads still
replace it and rejected reads never publish the profile. Rerun the real profile
and frozen v1 migration, then require another exact-head hosted matrix.

## #19 combined CI corrections verified locally

Both full unit-job command sets now pass: initial suite reports retain all passed
prefix steps, and resume reports/logs retain the remaining commands after each
adapter correction. This includes both compiler state suites, format diagnostics
under ASan/UBSan, native math, protocol/replication/rewind and differential goldens
with the one-ULP negative control. No test assertion or sanitizer was removed.

The 68 changed production source configurations also pass an extra shadowing
review. Format, types, boundaries and native ABI pass. Rebuilt profile migration
passes after the local name change. Local MinGW compiles the corrected save
message. Quake 3 and OpenArena bot/replay goldens remain unchanged; the frozen
profile/game gzip and raw hashes still match their committed provenance.
Origin main remains 5caa2c1c and the known-good tag object/target are unchanged.
Push the combined corrections to PR175 and require fresh hosted acceptance.

## #19 legacy probe compiler integration

Both full local unit jobs pass cooking, state owners, level/protocol/RHI and ABI
steps, then expose the bot-command driver compiling the new C++-only owner tail
of ai_main.cpp as C. Extend that file's existing __cplusplus owner guard through
its newly added cvar/reference helpers, and guard the new g_main.cpp cvar owner
likewise. This preserves the existing C regression probes and produces identical
production C++ code. No assertion, test language, fixture or golden changes.
Resume the unit jobs from their first failed step instead of repeating passed
asset cooking. The full exact-head hosted jobs remain required.

ASan keeps the new callback-identity tables live even in isolated formatting
probes. Their table-referenced gameplay functions now need explicit aborting test
doubles for unrelated engine services/bot nodes. Add only those doubles and the
normal native service header; all original formatter assertions and ASan/UBSan
instrumentation stay enabled. Base/missionpack team formatting and all four
native diagnostic owners pass after adaptation.

## #19 first hosted matrix findings

MSVC parameter-shadowing correction is pushed at 79308321. The first full matrix
also found MinGW's legacy printf annotation rejecting %zu; checkpoint size is
bounded to 64 MiB, so print its checked value as unsigned. The second MSVC
pass also finds the profile validator local keys array shadowing the engine
global; rename it keyNumbers without altering the profile schema. A direct local MinGW
compilation passes after that correction.

Hosted GCC reached the rewind probe and exposed a test comparison of four bytes
of alignment padding between teleport and generation. Compare all four owned
fields exactly instead; no production rewind behavior or accepted data changes.
The hosted legacy OpenArena C smoke also needs its test adapter to reject the four
new checkpoint exports explicitly, since that pinned C game has no state owners.
The native Aftershock game remains the implementation exercised by full game
save/load tests on OpenArena content. Reproduce the old-adapter compilation failure
locally, then both legacy OpenArena bot goldens pass unchanged after adapting it
(state-final-oa-smoke.log). Both OpenArena fixed replay frame goldens also pass
unchanged (state-final-oa-demo.log).

The Clang/libc++ unit job also exposed an old direct-include hitscan probe
missing the <cmath> prelude that game/module.cpp normally supplies. Add that
standard header to the probe so the new saved-weapon finite check compiles under
the runner's libc++ too. Run both complete local unit job variants to catch
additional test integrations before the next push.

PR174's integration diagnostics show 865 rewind traces and 45.06 seconds of
advancing simulation when the existing client-frame script hit its 45-second
wall deadline. No engine error/disconnect appears. Rerun only the failed runtime
job (35715942143 attempt 2); acceptance still requires it to finish green.

## #19 hosted portability correction

Draft PR175 targets main at cb46c7dd. Hosted MSVC x64/ARM64 Debug/Release
reject three new level-state helper parameters named level because they shadow
the native game's global (C4459). Rename only those parameters to savedLevel;
the reported compiler failure is the failing check. No layout/data behavior or
fixture changes. Local lifetime analysis passed all 1,344 compilation commands,
150 paths, shipping/development and static/module controls.

PR174's merged-tree runtime reached the existing delayed hitscan test, then timed
out waiting for netcode_done (35715942143). Inspect its runtime diagnostics before
rerunning; the exact-head run passed. Keep #31 open until integration is green.

## #19 final lifecycle and CI registration

The pending-load gate now honors shutdown, explicit map replacement releases its
archive, and map_restart rejects an unfinished reconnect. The real OpenArena
runtime passes shutdown/reload/map replacement, exact same/fresh-process restore
and continuation, and frozen full-game v1 migration (state-final-runtime.log).
Full GCC and Clang/libc++ state suites pass (state-final-{gcc,clang}.log); full tidy
passes 1,410 production configurations (state-final-tidy.log). Lifetime analysis
continues. No existing fixture or golden was regenerated.

CI now runs all state owners with both compilers and real profile/checkpoint
migration with OpenArena in the runtime job; evidence uploads only logs and entity
JSON. Affected-test routing, AGENTS commands and tests/README describe storage,
supported local-game context, migration and immutable fixture provenance.
Actionlint, affected-contract and suite-contract checks pass. Normal Quake 3 fixed replay and both bot-smoke hashes remain identical
(state-final-demo.log, state-final-smoke.log). Final Quake 3 checkpoint and
OpenArena profile/frozen-profile migration pass (state-final-{q3,profile}.log).
Format, types and boundaries pass. Open the draft PR for hosted acceptance;
finish lifetime analysis and self-review before ready/merge.

## #19 interrupted-load lifecycle test first

The runtime now interrupts a pending local reconnect with sv_killserver, requires
shutdown, and loads again; it also requires a map command to replace a pending
load normally. The first case fails before correction: the new checkpoint early
return in SV_Frame runs before the existing shutdown request
(state-interrupt-before.log). Move only that new load gate behind shutdown priority
and clear pending archive ownership before a normal explicit map change. This is
a lifecycle gap in new #19 code, not a normal engine bug fix.

## #19 full-game N-to-N+1 migration passes

The v2 checkpoint header removes redundant pure mode (owned by server cvars) and
adds the native protocol revision. Explicit v1 migration assigns its known
protocol revision 2 and validates its legacy pure value; a later protocol change
cannot silently relabel old archives. The frozen v1 bytes are unchanged.

The rebuilt client passes newly written v2 same/fresh-process restore and exact
continuation, then loads the frozen v1 game in another fresh process with seed 456
and matches both saved and continued entity projections
(state-migration-runtime.log). Build and focused tidy pass. Finish lifecycle review,
CI/command documentation, and final combined-tree gates before opening #19's PR.

## #19 full-game v2 migration test first

The runtime now requires newly written headers to add protocol and remove pure,
and loads the frozen v1 archive in a fresh process with seed 456, requiring its
recorded exact entity state and subsequent continuation. The unchanged v1 engine
fails the new-writer assertion as expected (state-migration-before.log). The v2
header will obtain pure mode from its already-owned server cvar; v1 migration
retains its legacy check and assigns that format's known protocol revision 2.
The migration fixture bytes remain unchanged.

## #19 full-game v1 fixture frozen

The explicit OpenArena-only command passed all same/fresh-process exact assertions
and created checkpoint-v1.asstate.gz once at writer 308b70c2. Real-client screenshot
review confirms the restored oa_dm1 scene, local player HUD and Sarge. The archive
contains 86 saved/83 continued entity rows; raw size 17,407,471, gzip size 340,020.
Raw SHA256: 1d3d0762caf2f464e242d0e485de429f5a9a53e91e2327efff4f2fcf742fa977.
Gzip SHA256: 946d1991efd407e7eebd6182485ebe1325a369a7366bac9f1fb436e165a99b74.
Source/binary provenance and entity projections are checkpoint-v1.json. The fixture
README records OpenArena GPL attribution from the installed Debian data package.
No proprietary content is included; no accepted fixture is regenerated. Freeze
this commit before v2 removes redundant pure metadata and adds protocol context.

## #19 full-game v1 fixture creation command

Quake 3 also passes the unchanged same/fresh-process exact continuation gate.
The explicit --record-v1-fixture command now requires OpenArena and a version-1
writer, refuses existing gzip/metadata paths, and writes only after the full runtime
assertions pass. It records binary/source and raw/compressed hashes plus exact
saved/continued entity projections. A private screenshot supports review. Run it
once at this committed writer/driver, review, then freeze the bytes; CI never
invokes the creation flag. Implement v2 migration only after that reviewed fixture.

## #19 full OpenArena checkpoint continuation passes

A client checkpoint owner retains the actual input clock rebased against the new
process clock, old frame/time cursors, extrapolation/new-snapshot flags, exact view
angles, selected weapon/sensitivity and pending loopback command. Transport sequence
numbers remain fresh. This resolves the 100 ms input discrepancy without changing
normal clock adjustment, movement arithmetic or the exact equality assertions.

The small owner probe passes GCC/Clang libc++ UBSan. Full OpenArena runtime now
passes same-process paused restore and continuation, then fresh-process restore
and the same continuation with a different initial RNG seed. Earlier numbered
revisions remain unchanged. Build, focused tidy, format/types/boundaries pass
(state-client-clock-*); the isolated scratch executable initially collided with
an existing directory, then ran successfully under a unique name. No accepted
fixture changed. Quake 3 runtime and full-game migration/hosted acceptance remain.

## #19 local input clock test first

The private archive diff shows equal server time/residual, bot clocks and RNG
records, with the first significant continuation difference in local command time.
A client-side input owner must retain the paused clock lead, time-adjustment flags,
view angles, selected weapon/sensitivity and pending loopback command. The new
state_client probe fails on its absent implementation before the change
(state-client-before.log); it requires rebasing against a different process clock
without losing the saved lead. Fresh transport sequences stay connection-owned.

## #19 exact spatial restore; continuation investigation

The spatial owner now preserves sector heads, exact entity list order, cluster
membership and map-derived sector geometry with checked indices. It validates
whole lists and rejects cycles/mismatched native linked flags before publication.
Native saved absolute bounds are retained without rerunning collision math; the
superseded native relink helper is removed. GCC/Clang libc++ UBSan, build, native
ABI, focused tidy and policy gates pass (state-world-*). The full OpenArena runtime
now restores all 86 entity rows exactly, including snapped player bounds.

The continuation assertion still fails on small player/bot position differences
after resuming; no tolerance was added. Investigate first differing clocks/input
and RNG records using private saved archive comparisons. Full #19 remains open;
no fixture has been frozen or regenerated. PR174 remains gated on hosted runtime.

## #19 live restore spatial correction, test first

The first full runtime reaches load completion and restores all 86 OpenArena
entities, but the equality gate fails on the two players' absolute collision
bounds. Normal player code links at snapped positions and then restores precise
currentOrigin; deriving links again from currentOrigin changes those saved bounds.
Spatial list order also cannot be inferred from entity indices. This is a gap in
new #19 restore, not a change to normal collision behavior. Preserve actual server
sector lists/cluster membership and keep native saved bounds untouched.

The new state_world probe fails compilation before owner implementation. It
requires the original area-query order, snapped bounds distinct from precise
origins, clusters, and cycle rejection without mutation (state-world-before.log).
The runtime driver now retains entity diffs on failure. No goldens were changed.

## #19 first live restore coordinator

The local load coordinator now builds and passes native ABI, focused tidy and
policy checks. Both full GCC/Clang libc++ UBSan state suites pass
(state-load-full-{gcc,clang}.log). The coordinator validates engine records and
cvar capacity before shutdown, loads matching content without gameplay startup
frames or automatic bots, reconstructs native/botlib owners, spatial links,
configstrings and baselines, then reconnects only the recorded local human slot.
Simulation/input remain frozen through its first snapshot. Final cvar/input and
RNG restoration happens after client startup; bot cache ages exclude reconnect
latency. Errors discard the fresh world and shutdown releases pending archive data.

Full runtime acceptance is now being exercised; no success is claimed yet. Its
handshake wait steps only the frozen connection, ending on explicit completion.
This small local-player-plus-one-bot case does not reach the separately reproduced
MAX_CLIENTS shutdown boundary, so it can run independently while PR174 checks
continue. #31 must still merge green and be merged forward before #19 acceptance;
repeat final gates on that combined tree. The original prerequisite ordering was
conservative; no #31 fix is copied into this branch. N-to-N+1 fullgame fixture and
final hosted gates remain unfinished.

## #19 server record preflight

Server capture now round-trips and validates its complete engine records: bounded
map/game context and clocks, exactly one designated local human among anonymous
bot/free slots, required input records, aggregate configstring capacity, and actual
replication priority/radius limits. The probe covers invalid metadata/ownership,
missing engine records, a complete valid archive and a bad final interest record.
GCC/Clang libc++ UBSan, build, focused tidy and format/type/boundary checks pass.
Live OpenArena capture/read validation passes at 17,313,632 bytes
(state-server-*). The unused sv_gametype alias record was removed; actual
g_gametype is already owned by native game cvars. No accepted fixture changed.
Full load coordination and acceptance remain next; PR174 runtime is still running.

## #19 server restore preflight test first

The new state_server probe requires bounded map/game names, valid server clocks,
and a single designated local human slot with anonymous local/bot identities.
It fails on missing ValidCheckpointHeader/ValidCheckpointClient before their
implementation (state-server-before.log). These checks precede any destructive
map reload; full owner validation still occurs against freshly loaded content.

## #19 platform save routing

Profiles and full-game captures now route through Sys_SaveRevision/Sys_ReadSave.
The bounded main-thread provider installs before first IO; console SDK integration
can supply its active-user/title storage callbacks without portable code using OS
APIs. No SDK implementation or console acceptance is claimed. Provider errors
never fall back to PC storage. PC reads only loose active-game user-directory
files; exclusive creation tries numbered revisions without replacing existing
bytes, and failed writes/close errors remove the newly created partial file.

GCC/Clang libc++ UBSan provider probes cover routing, collision retries, invalid
paths, size bounds, immutable installation and failure without fallback. Real
OpenArena profile save/reload, fresh-process reload and frozen-v1 migration pass;
full paused game capture still validates at 17,314,861 bytes. Client/server build,
focused tidy and format/type/boundary gates pass (state-storage-*). The first
build exposed a missing cerrno include, corrected before these successful gates.
No accepted fixture changed. Full live load coordination and its runtime and
N-to-N+1 game fixture acceptance remain unfinished.

## #19 platform storage test first

The new state_storage probe requires a platform save provider, immutable numbered
revisions, bounded profile/game paths, and errors without fallback to PC storage.
It fails compilation on the absent save_public.h before implementation
(state-storage-before.log). Existing profiles and checkpoints still call the
filesystem directly; route both through this boundary next. PC storage must stay
in the active game's user directory and never overwrite an existing revision.

## #19 cross-owner cvar capacity

Checkpoint preflight now counts missing owner-selected cvar names across the
engine and native game together, deduplicating shared names. It reserves only
currently empty slots, not slots that later phases might release, so preparation
fits regardless of owner ordering. A mutating callback is rejected. Both GCC and
Clang libc++ UBSan probes demonstrate aggregate exhaustion, duplicate-name reuse,
rejection without registry mutation, and successful preparation after capacity is
freed. Client/server build, focused tidy, formatting, types and boundaries pass;
the live OpenArena capture still validates at 17,314,861 bytes
(state-capacity-{probes,build,runtime,tidy}.log). The load coordinator must invoke
this preflight before preparation. Platform routing and full restore remain next.

## #19 bot ownership validation

Native checkpoint validation now cross-checks every active bot against its saved
player/entity slot and the reconstructed live character/movement/goal/weapon/chat
pools. Actor-local handles cannot be shared; cached characters may be shared.
Missing handles, mismatched bot flags/connection state and duplicate ownership
reject before native publication. GCC/Clang UBSan cover those cases and sharing,
and the real paused OpenArena capture still passes full read validation at the
same 17,314,861-byte size. Build, native ABI, focused tidy and policy checks pass
(state-bot-bindings-*). No normal simulation path or accepted fixture changed.

## #19 live capture integration in progress

Native checkpoint orchestration and the server savegame command now build with
the native ABI gate and focused tidy. The native phase stages all
entity/client/level drafts and owner validation before publishing; string storage
uses the existing level arena, spatial relinking preserves link generations, and
native RNG restoration remains an explicit final phase. Server capture owns map
context, client slots/input, configstrings, replication interest, selected cvars,
portal counts and the libc random stream. Save revisions use the user directory
and never replace a previous numbered path. Platform save-provider routing and
loadgame orchestration remain unfinished.

Live OpenArena oa_dm1 capture with a paused local player and Sarge now writes a
17,314,861-byte checkpoint. Before writing it, the command opens the archive and
validates the complete native, botlib and portal state against the live world.
New validation was corrected for the unused native level.gentitySize (map setup
leaves zero) and predictable temporary events carrying EV_EVENT_BITS in eType.
The reference and semantic probes cover those cases. No gameplay code changed.

Client/server build, native ABI, focused tidy and format/type/boundary checks pass.
Full GCC/Clang state suites pass (state-capture-full-{gcc,clang}.log).
The mixed engine/native cvar probe now explicitly declares the native RNG helper
because the two legacy q_shared headers share an include guard; production builds
already compile those namespaces separately. Captured files are private temporary
artifacts, not accepted fixtures. Next: cross-owner bot handle/cvar capacity
validation, platform save routing and loadgame coordination, then the committed
full runtime acceptance and frozen N-to-N+1 OpenArena fixture. Do not claim capture
alone as #19 completion.

## #19 botlib checkpoint phases

BotLib_WriteState now groups every owner behind a setup/navigation header.
PrepareSettings restores library variables before normal setup/map loading;
PrepareState reconstructs saved actor/cache slots in the freshly loaded world.
ReadState validates all owners before applying their values, then restores parser
and variable state after content loading. Preparation failure requires discarding
the fresh world; no simulation may run during restore. Disabled botlib and worlds
without AAS are supported explicitly; a loaded but unfinished AAS is not savable.

A real-library probe links all production botlib sources, with owned empty config
text and test memory/filesystem services. Disabled capture and normal setup,
shutdown, preparation and restoration pass GCC/Clang libc++ UBSan. Saved queued
chat and pending input survive; validation leaves live input unchanged; all zone
and hunk allocations are accounted for at shutdown. Navigation owner probes remain
separate until the full runtime integration. Client/server build, focused tidy and
format/type/boundary gates pass (state-aggregate-*). Native/engine coordinator,
cross-owner handle checks and fullgame acceptance remain unfinished.

## #19 collision portal reconstruction

The collision owner now records the BSP checksum, area count and symmetric portal
reference matrix. Restore validates the entire matrix before mutation and rebuilds
derived flood labels before snapshot visibility. GCC/Clang libc++ UBSan prove the
same area bits and subsequent door-close sequence with multiple references, plus
mismatched-map/count and invalid-matrix rejection. Client/server build, focused
tidy and format/type/boundary checks pass (state-portals-*). This is additive
checkpoint handling; ordinary collision and door arithmetic remain unchanged.
Live restore still needs to call it before publishing the first snapshot.

#31 prerequisite PR174 is merged as 5caa2c1c after all 26 required gates passed;
merged-tree workflows are recorded in Next action.

## #19 gameplay draft validation; #31 cleanup prerequisite

Draft validators now check finite scalar/vector fields, raw enum representations,
trajectory kinds, active entity/client slot identity, bounding-box order, player
movement/weapon/team states and level client counts/sorted-client uniqueness.
They inspect legacy enum representations with memcpy before evaluating enum
values, so invalid serialized enums reject without undefined behavior. Existing
user-command handling still owns raw input validation. Auxiliary animation/weapon
state retains its separate owner validation. GCC/Clang libc++ UBSan, client/server
build, focused tidy and policy checks pass (state-semantics-{gcc,clang,build,tidy,policy}.log).
The coordinator still must call these validators before publishing decoded drafts.

Ordering refinement: take the recorded last-chat-handle shutdown bug in its own
#31 PR now, before #19 live coordination. Full checkpoint reload uses normal
library shutdown, which must release every handle including MAX_CLIENTS. This
turns the recorded bug into a prerequisite; do not work around or fix it inside
#19. Continue independent #19 coordination work while that PR's gates run.

## #19 engine/native cvar restore boundary

Engine records now take trusted owner-selected names and preserve current/reset/
latched strings, optional bounds, validators, flags and cached numeric/revision
state. Existing registry addresses remain stable; absent records can remove only
eligible dynamic variables or explicitly owned transient session names. Private
variables reject capture/restore. Server notification flags/group state restore
without replacing renderer notification state. Registry capacity must be checked
across the complete restore by the coordinator before applying new names.

Native cached-cvar owners now include their engine records through the existing
native service boundary. Modes are 0 validate, 1 restore, 2 prepare: preparation
withholds pending latched values so normal map initialization loads the saved
current content; final restore reinstates the pending values. Additional records
cover authoring paths, bot initialization settings, session/botsession names and
per-item disable cvars. Current filesystem/process context remains separately
owned. The literal cvar call inventory now has a source ownership check.

Focused GCC/Clang libc++ UBSan prove real native/engine handoff, pending cached
updates, fresh registry handles, latch/bound behavior, session cleanup and content
path preparation (state-engine-cvar-bridge-{gcc,clang}.log). Full combined GCC/
Clang suites, the client/server build and focused tidy pass after the preparation
followup (state-bridge-full-{gcc,clang,build,tidy}.log). #19 comment 5773458340 records
the preceding complete native-owner checkpoint. Full checkpoint integration is
still incomplete; no accepted fixture changed.

## #19 cached native cvars and change counters

All persistent native-game cvars now have named owner records, including the
main table, bot scheduling/navigation/minimum players, animation, weapons and
rewind. Restore retains current registration handles while applying cached
values, strings and modification counters. Main-table tracking and password
change detection also restore. The source ownership check covers every persistent
vmCvar declaration in those owners. GCC/Clang libc++ UBSan pass fresh-handle
rebinding, complete-group validation before mutation and missing tracking-record
rejection (state-cached-cvars-{gcc,clang}.log). Build, focused tidy and policy
checks pass. Combined state suites pass on both compilers after native-owner and
cvar additions (state-native-complete-{gcc,clang}.log).

Next: engine cvar values must be prepared before map initialization and restored
with native caches afterward. Use explicit owner-selected names, preserving
runtime registration handles and current platform/filesystem context. The native
cvar call inventory is state-native-cvar-callers.txt in the private cache. Include
uncached map-authoring paths, bot setup settings, session strings and per-item
disable flags when coordinating owner selection. Then semantic validation and
live server/client/RNG restoration, save-provider routing, frozen OpenArena full-
game migration and final local/hosted gates. No accepted fixture changed.

## #19 native bot slots and remaining native owners

Native bot preparation validates all actors first, then uses the existing level
arena to reconstruct exact active and allocated-inactive slots. Inactive partial
setup rejects capture. Activation links target the final allocation. Legacy
G_Alloc accounting restores its saved used count while retaining the normally
loaded map-info prefix; entity strings and actors will use level memory. This
preserves the next legacy allocation offset and remaining budget without saving
raw pointer-bearing memory. Full coordination must start from a fresh map.

Item registration retains the complete boolean table; IP filters retain all
1,024 entries independently of the short presentation cvar. Loaded bot/arena info
strings and order are verified by digest. GCC/Clang libc++ UBSan pass active and
inactive bot reconstruction, missing-actor rejection before allocation, same-next
arena allocation, complete item/filter tables and relocated/changed content
(state-native-owners-{gcc,clang}.log; state-bot-info-{gcc,clang}.log). The initial
owner client/server build and focused tidy pass. Bot-info build/tidy and all
policy checks also pass (state-native-info-{build,tidy,policy}.log). Cached
cvars/change counters and
semantic/coordinator work remain next. No accepted fixture changed.

## #19 chat reconstruction and combined preparation gates

Chat records now retain the source identity for private as well as cached content.
Preparation reuses the existing chat loader, verifies content, restores line
timers, and reconstructs exact cache sharing/private ownership. Actor preparation
validates queue records before publishing exact saved handles into the normally
initialized message heap. Command-time staging cleans up on failure.
GCC/Clang libc++ UBSan exercise real loading of a small owned chat source, sparse
slots through MAX_CLIENTS, same-next chat selection, pending message order and
failed restore cleanup. Full state suites pass on both compilers, as do the
client/server build and focused tidy (state-full-prepare-{gcc,clang,build,tidy}.log).
No accepted fixture changed. #19 comment 5773155394 records the preceding handle
checkpoint. Remaining work starts with other native globals/semantic validation,
then live restore coordination, both RNGs, save-provider routing and full-game
OpenArena migration acceptance; final integration gates still remain.

## #19 goal and weapon handle reconstruction

Goal and weapon preparation stage exact saved handles, reuse the shared weight
cache, rebuild private weights and derive content indices with the existing
index builders. Loaded content/index identities must match before publication;
failed staging frees its private weights, indices and handles. GCC and
Clang/libc++ UBSan pass sparse slots including MAX_CLIENTS, shared/private
ownership, learned values, same-next goal pop and rollback without touching the
shared cache (state-{goal,weapon,weight}-handles-{gcc,clang}.log). Client/server build and focused tidy
pass (state-owner-prepare-{build,tidy}.log). Chat reconstruction is next, followed by native/server coordination.

## #19 shared and private weight reconstruction

Weight preparation reuses ReadWeightConfig in its existing private-load mode,
restores the libvar's cached value on return, verifies the saved topology, and
applies learned values. The shared cache publishes only after all slots succeed;
failed staging frees its allocations. Parser search-folder restoration must run
after all content loading. The new test first failed on missing preparation APIs
(state-weight-prepare-before.log), then passed with the real script/precompiler
and fixed owned source on GCC and Clang/libc++ UBSan. It covers saved cache gaps,
private ownership, unchanged libvar fields, changed content and missing final
records with no leaked allocations. Build and focused tidy pass
(state-weight-prepare-{gcc,clang,build,tidy}.log). Goal/weapon/chat preparation and
full live coordination remain; no accepted fixture changed.

## #19 movement-pool reconstruction

Movement preparation validates every saved record before allocating exact saved
handle slots into an empty pool. GCC and Clang/libc++ UBSan prove gaps, relocated
storage and rejection of incomplete records without publication. Client/server
build and focused tidy pass (state-move-reconstruct-{gcc,clang,build,tidy}.log).
Character and movement construction are complete locally; shared/private weight,
goal, weapon and chat reconstruction remain next. No accepted fixture changed.

## #19 exact character-pool reconstruction and stable owner gates

At f8a4e000, the complete state suite passes on GCC and Clang/libc++ UBSan
(state-full-aas-{gcc,clang}.log). Full tidy passes 1,390 production configurations;
full lifetime analysis passes 1,332 commands/148 paths and its positive/seven-object
negative controls (state-full-aas-{tidy,lifetimes}.log). Issue19 comment 5772911231
records this owner checkpoint; all those gates are now complete.

Characters now also retain resolved typed attributes, because filename/skill alone
cannot reliably reproduce fallback/default merges and interpolated cache entries.
Bot_PrepareCharacterState requires an empty pool, validates every record first,
then creates exact saved slots with owned string copies and rebased clock ages.
The existing attribute hash checks the resolved payload; full checkpoint content
identity still belongs to the coordinator. GCC/Clang UBSan pass reconstruction,
owned-string relocation and missing-later-payload rejection before publication
(state-character-reconstruct-{gcc,clang}.log). Client/server build and focused tidy
pass. No accepted fixture changes; eventual full-game fixtures use OpenArena.

## #19 bot content and synchronous parser boundary

BSP checkpoints verify both the source entity text and parsed epair order/content
against hashes; no map content is embedded. The parser retains its search folder
and requires no active source handles, outstanding copied tokens or global macro
definitions at the command boundary. Current native game/UI code never installs
persistent global macros; adding that use will require a state migration rather
than silently omitting it. GCC/Clang libc++ UBSan pass content relocation/change
checks and exact folder restoration with boundary rejection
(state-bot-content-{gcc,clang}.log). No script-loader behavior changed.
Routing build and focused tidy pass; its source ownership gate now covers every
AAS world/cache field. Combined state/lifetime/tidy gates will be rerun after this
stable owner checkpoint before reconstruction work.

## #19 AAS routing cache ownership

Routing checkpoints retain cache metadata, 16-bit travel times, reachability
choices, per-bucket list order and oldest-to-newest eviction order. They verify
rebuilt reverse links, intra-area travel times, portal maximum times, travel flags
and reachability pass-area lists before replacing caches. Synchronous update
scratch must have no in-list work; stale scratch pointers are never serialized.
Explicit ceilings are 4,096 caches, 65,536 travel entries per cache and 256 MiB of
cache allocations; excess rejects. Pointer lookup is bounded quadratic command
work, documented in source. GCC/Clang libc++ UBSan prove cache-hit payload, exact
bucket/LRU order and same-next-eviction after fresh allocation; changed derived
routing, missing final records and over-wide travel times leave current caches
unchanged (state-aas-routing-{gcc,clang}.log). World/settings build and focused
tidy also pass. Full live reconstruction/coordination is still open.

## #19 AAS world clocks, geometry and physics settings

World records retain clocks, frame counters, initialization bookkeeping and
per-area disabled bits against a digest of all loaded geometry/cluster arrays.
The digest masks the dynamic disabled bit and omits the two reachability-record
tail padding bytes, so pointer relocation and padding cannot change identity.
Capture requires a fully initialized AAS map and the same 65,536-area ceiling as
spatial links. All AAS physics settings have named float descriptors and finite
validation. GCC and Clang/libc++ UBSan verify changed geometry/content rejection,
missing disabled-area records, clock restore, tail-padding independence and
bit-identical AAS_HorizontalVelocityForJump results after settings restore
(state-aas-world-{gcc,clang}.log). Routing caches must be restored with world state
before navigation resumes; their owner and the full coordinator remain open.

## #19 AAS entity history and spatial links

AAS entity records preserve history even for entities invalidated this frame;
spatial links remain separately owned, and the unused Quake 3 BSP leaf pointer
must be null. Named scalar/vector fields validate finite values, model indices
and bounds before application. The AAS link owner retains both list orders
(entities in areas and areas touched by entities), their back-links, the free-list
and free count. Every link must belong exactly once to both live views or once to
the free list. Free payload is omitted. The capture ceiling is 65,536 links and
areas; excess fails explicitly. GCC/Clang libc++ UBSan cover relocation, actual
unlink/relink with identical next slots, maximum-size heap, invalid/incomplete
records, cycles and live/free overlap (state-aas-{entities,links}-{gcc,clang}.log).
Client/server build and focused tidy pass.

A separate existing bug was reproduced: BotShutdownChatAI omits allocated handle
64. Issue31 reopened with comment 5772659684; docs/bugs.md records the reproducer
and exit 1. No fix is mixed into #19. Take a separate test-first #31 PR after #19
unless full restore makes it a prerequisite. Private reproducer is retained under
the modernization cache. No fixture or suppression changed.

## #19 botlib globals and immutable map information

Botlib records now retain the global clock and developer setting against matching
initialized client/entity dimensions. Map locations and camp spots are verified
by hashes of named, pointer-free fields in list order. They are reloaded from map
content, not copied as pointer-bearing records. GCC and Clang/libc++ UBSan pass
relocation, changed content/dimensions and cyclic-map-list rejection
(state-bot-global-map-{gcc,clang}.log); build and focused tidy pass.
AAS inventory confirms entity spatial-link ordering, disabled routing areas and
routing cache contents/order need explicit ownership; cold cache regeneration
alone is not yet proven to preserve continuation.

## #19 named bot variables and combined verification

Libvar checkpoints retain name/value strings, flags, modified bits, cached numeric
values and list order. Loads preserve existing cached libvar_t handles and create
missing names through the existing allocator. Unexpected initialized names reject
rather than deleting an owner's pointer. All records and names validate before
application; the explicit ceiling is 256 variables with engine string limits.
GCC/Clang libc++ UBSan cover fresh/partial reconstruction, cached-handle identity,
incomplete/duplicate records and list cycles (state-libvars-{gcc,clang}.log).
Client/server build and focused tidy pass. The complete state driver through chat
content also passes on both compilers (state-full-chat-{gcc,clang}.log); chat build,
tidy and format/types/boundaries pass. Issue19 comment 5772554241 records the
weapon/character/chat decisions. AAS, global clocks, map info identity and the
full-game reconstruction/coordinator still remain; no partial acceptance.

## #19 chat content and dialogue timers

Shared chat caches, private actor chats and global reply lines now save reuse times
against immutable content hashes. Hashes cover line/type order, reply keys,
matching templates, synonym weights and random expansion lists; cached filenames
and chat names and actor-to-cache ownership must also match. Capture has an
explicit 8,192-line / 65,536-syntax-node ceiling per owner and rejects excess,
cycles, aliases and non-finite timers. It never truncates. All records validate
before timers change. GCC and Clang/libc++ UBSan pass relocated shared/private
chat, identical next-line selection, changed-content and missing-last-record
checks (state-chat-content-{gcc,clang}.log). Reconstructed content/cache slots are
still prerequisites; coordinator and complete runtime acceptance remain open.

## #19 chat queue ownership

Chat actor scalars and pending console messages now retain pool slot/queue/free-list
order. Back-links are validated at capture and rebuilt from validated, disjoint
lists at load. Message text is chunked in groups of 64; a maximum-capacity 65,536
slot pool with one queued message needs four records, not one per free slot.
Free payload is discarded because enqueue overwrites it. GCC and Clang/libc++
UBSan prove same-next-allocation and wrapped message handles after relocation,
reject overlap/cycles and missing later text without mutation, and exercise the
last slot of the maximum pool (state-chat-queue-{gcc,clang}.log). Client/server
build and focused tidy pass. Botlib edits now select the state regression in the
affected-test map. Chat content/timers and full reconstruction remain open.

## #19 character-cache state and process-clock ages

Character records verify immutable typed attributes, filename and skill, then
restore reference counts and unreferenced ages. The coordinator supplies one
process-clock reading for capture and one for load; timestamps are rebased with
unsigned wrap-preserving arithmetic so a fresh process evicts the same oldest
unreferenced handle. Original cache/eviction expressions are unchanged. GCC and
Clang/libc++ UBSan pass changed-content, incomplete-pool, clock-wrap and actual
same-next-eviction checks with relocated characters/strings
(state-bot-characters-{gcc,clang}.log). Build and focused tidy pass. Cache handles
still require reconstruction before application; chat/AAS/libvars and the full
checkpoint coordinator remain open.

## #19 bot weapon-weight ownership

Weapon-pool checkpoints verify pointer-free weapon/projectile descriptors and the
weapon-to-weight index map, retain shared cache-slot references and restore private
weight values. The loaded descriptors have explicit no-padding size assertions.
GCC and Clang/libc++ UBSan prove the same best-weapon choice after relocation and
reject changed definitions, changed indices and missing values before mutation
(state-bot-weapons-{gcc,clang}.log). Build and focused tidy pass. Character/chat,
immutable location/camp identity, AAS/libvars and full reconstruction remain open.

## #19 bot level-item allocation state

Level-item checkpoints retain live payload, doubly linked live order, free-list
order and the map item-number base. The allocator now records its existing bounded
capacity for serialization and clears that bookkeeping on shutdown. Free payload
is omitted because AllocLevelItem clears it before reuse. Every allocated slot
must belong to exactly one terminating live/free list; overlaps, cycles and
invalid item references reject before publication. GCC and Clang/libc++ UBSan
prove that relocated heaps make the same next allocation, including a deliberately
invalid unreachable payload that is correctly discarded (state-bot-items-{gcc,clang}.log).
Build and focused tidy pass. This is checkpoint ownership, not a change to allocation
or item simulation behavior. Immutable location/camp identity and the remaining
botlib owners still need integration.

## #19 bot goal pools and shared weight cache

Goal records preserve stack entries, avoided-goal timers and reachability memory.
Pool identity verifies loaded item descriptors and the item-to-weight index map;
pointers are rebound to the recreated owners. Shared cached weights are saved once
by cache slot, while uncached private configurations accompany their owning goal.
Unexpected private aliases and duplicate cache slots reject rather than silently
apply conflicting copies. GCC and Clang/libc++ UBSan pass full-state relocation,
identical goal-pop continuation, content mismatch and missing-later-record tests
(state-bot-goals-{gcc,clang}.log). The cache reader validates all slots before any
application. Client/server build and focused tidy pass. Level-item lists, weapon
weight owners, character/chat/AAS/libvars and reconstruction remain open.

## #19 mutable bot weight records

Weight records retain current weight/minimum/maximum values against a digest of
reloaded names and separator topology. Pointer identity never enters the digest;
separate trees with identical content restore and evaluate bit-identically.
GCC and Clang/libc++ UBSan pass relocation, changed-topology, missing-values,
non-finite, cyclic and oversized-tree checks (state-bot-weights-{gcc,clang}.log).
The command-only ceiling is 4,096 nodes per configuration: excess rejects the
save, never truncates it. Only live array prefixes are stored. Existing weight
math is unchanged. Build and focused tidy pass. Cache/goal/weapon ownership and
reconstruction still need wiring; these per-configuration records are groundwork.

## #19 botlib input and movement ownership

Botlib inputs preserve jump-edge flags and previous input; movement records retain
all movement/avoidance fields, saved handle occupancy and model classification.
Readers require coordinator-recreated handle slots, validate every record first,
and rebind data to newly allocated storage. GCC and Clang/libc++ UBSan prove
identical next jump/avoidance updates and reject incomplete pools before mutation
(state-botlib-move-{gcc,clang}.log). Clang exposed missing explicit <cmath> includes
in the new validation code; those are fixed. Client/server build, focused tidy
and member/avoidance-slot source coverage pass. Full library reconstruction and
checkpoint integration are still pending. No original movement expressions changed.

## #19 bot map-navigation ownership

Navigation records retain last-teleport memory, map/team goals and alternative
route goals/costs. Reloaded game type, client count and BSP model identity must
match before publication. Live route costs widen to uint32 in the save record
and validate back into their original uint16 range. Unused route slots are not
read from stale storage. GCC and Clang/libc++ UBSan pass normal and optional
mission-pack probes (state-navigation-{gcc,clang}.log), including changed-map
identity and missing-later-record rejection. The optional legacy variant needs
its existing writable-string and unused-parameter warnings tolerated in the
probe only; production gates are unchanged. Build and focused tidy pass.
Bot node-switch diagnostics reset at the start of BotDeathmatchAI and are frame
scratch. Full botlib owners and checkpoint coordination remain open.

## #19 bot actor and activation drafts

Actor records describe all 134 scalar fields, shared player/input state, goals,
nullable typed AI-node identities and checked activation/waypoint references.
All eleven AI-node identities pass GCC and Clang/libc++ UBSan round-trips with
full actor byte equality after explicit pointer relocation. Invalid active-stack
cycles, unknown callbacks and incomplete actors reject before publication
(state-bot-actor-{gcc,clang}.log). Member/callback source coverage passes, as do
the client/server build and focused tidy. Botlib handle identities are retained
as draft integers; final restore must coordinate their corresponding botlib
objects and verify live actor counts before application. These are not complete
checkpoint acceptance. The combined state driver through waypoint support also
passes with both compilers (state-combined-{gcc,clang}.log).

Next: all botlib mutable owners and immutable-content
checks; then global cvar/configstring/content state and server/game/client restore
coordination, both RNG integrations, platform save routing and full saved-game
N-to-N+1 fixture acceptance. No accepted fixture has been regenerated.

## #19 bot waypoint ownership

Waypoint saves retain stable pool indices, names, goals and the exact free-list
order. Shared named bot-goal metadata accounts for every goal member. Links are
range-checked and next chains must terminate before restore; inactive free-node
prev pointers retain their historical values. GCC and Clang/libc++ UBSan prove
that restored chains allocate the same next waypoint and reject cyclic/foreign
links (state-waypoints-{gcc,clang}.log). Full bot actors, activation stacks, map
navigation globals and botlib remain next. Scheduler/queue/team records are
committed in aeedbf68; definition/editor records in 508bcdaa.

## #19 bot scheduler, queue and team ownership

Bot checkpoints now retain delayed spawn queue entries, minimum-player check time,
AI scheduling clocks/residuals, interbreeding counters, team task preferences and
leader exclusions. GCC and Clang/libc++ UBSan probes pass continuation and reject
invalid/incomplete archives before applying any owner (state-bot-globals-{gcc,clang}.log).
Client/server build, focused tidy and the existing bot command-byte control pass.
Full bot actor/waypoint/activation and botlib ownership remain open; these records
do not reset bots and are not full-game acceptance.

## #19 definition and editor checkpoint ownership

Authored definition records use the shared schemas and existing ASENT validation.
Restore requires the registry topology reloaded for the map to match before applying
edited values to either the authored or combined native table. The combined table
is deliberately not treated as ASENT: native rows precede authored replacements.
Editor saves retain complete/degraded status, unknown-key source documents,
deleted document slots and entity-to-document mappings. Capture/spawn temporaries
are reset; shipping builds validate and ignore development documents. Every
record validates before publication. Command-only fixed scratch avoids large
stack frames. GCC and Clang/libc++ UBSan pass both build variants, including changed
topology, missing records, malformed documents and no-partial-application checks
(state-definitions-{gcc,clang}.log). Build and focused tidy pass. Full checkpoint
coordination, remaining globals/bots and migration acceptance remain open.

#18 merged-tree regression 35687912012 completed successfully; together with build
35687912038, all active merged-tree checks are green for 2010b077.

## #19 utility, combat, team and podium state

Checkpoint records retain shader remaps, the death-animation cycle, team flag and
capture clocks, neutral obelisk and victory-podium references. The function-local
nextRewindSpawn counter is now owner-local so saves retain it; the increment and
zero-skip expressions are unchanged. A restored UINT32_MAX counter produces the
same next entity, with both GCC and Clang UBSan. Invalid enums, references and
incomplete remap archives reject before application. Dead remaps after reset are
not serialized. Tests initially combined legacy translation units and hit repeated
header declarations; separate owner probes correct the harness. Both compiler
probes pass (state-small-globals-{gcc,clang}.log), as do the utility probes,
client/server build and focused tidy (state-globals-build.log and -tidy.log).
Full #19 checkpoint coordination remains open. No accepted fixture changed.

## #19 rewind history state

Rewind checkpoints retain live ring frames, cursor/count, 64-bit spawn generations
and report clocks. Reset does not clear old ring storage, so unreachable frames
are intentionally never read or serialized; unused box storage is zeroed in the
save representation. Validation reuses the existing history-frame rules without
changing query/interpolation math. A fixed command scratch record avoids two
64 KiB stack frames; the existing per-frame scratch is safe to reuse during load.
GCC/Clang UBSan proves clock-wrap restore, identical subsequent history queries,
no partial application on a missing later frame, and compact empty-history saves
(state-rewind-{gcc,clang}.log). The old gameplay probe now discards unused checkpoint
sections when linking, as the other focused probes do. Original rewind acceptance
still passes 950/950 delayed hits with max error 0.000488, plus live-world/view-time
and reused-slot checks (state-rewind-control.log). Client/server build, focused
tidy, format/types/boundaries pass. No accepted fixture or authoritative FP
expression changed. Remaining owners and full checkpoint coordination stay open.

## #19 authored weapon state

Weapon checkpoints retain both-hand inventory, RNG/cooldowns, attachment masks,
selected definitions, animation state/parameters and checked auxiliary entity
slots. Definition and graph digests must match the reloaded content. Configured
attachments are rebuilt by the existing Weapon_Configure and must compare byte-
identically before capture. Inactive/unconfigured inventory is omitted only after
checking exact zero; active inventory uses shared named weaponState_t fields.
GCC/Clang/libc++ UBSan continuation and changed-content/incomplete-owner checks
pass (state-weapons-{gcc,clang}.log). Client/server build, focused tidy and source
policies pass. Existing weapon lifecycle, deltas, hitscan and animation tests pass;
the 1,000-shot trace remains
0c1b0e259650e6c5c6c155244100b3e194abbfc75fe7d10717cdb25b919c746f
(state-weapon-control.log). No original simulation expression changed.

Next: rewind history and remaining global/bot/definition/editor owners, then the
full save/load coordinator and fresh-process acceptance. Core records remain
drafts until that acceptance passes. No accepted fixture was regenerated.

## #19 authored animation state

Animation checkpoints retain rig hashes/calibration, actor clocks, manual inputs,
parameters, state-machine event cursors, hit boxes and checked entity slots. Asset
storage is reloaded by normal map setup and must match the recorded graph hash.
animState_t uses one shared named description for subsequent animation users.
The immutable archive is fully validated before a second pass applies actor state.
GCC/Clang UBSan probes using the owned body/rifle graphs resume identical state,
event and pose words, reject changed content and prove missing later actor records
do not partially apply earlier actors (state-animation-{gcc,clang}.log). The
client/server build passes. Source coverage accounts for all composed/animation
members. No authoritative animation expression or accepted fixture changed.

Remaining #19 integration includes weapon/rewind/other game globals, all bot state,
entity-definition and editor state ownership, cvars/configstrings/content identity,
server/client clocks and reconstruction, platform save routing, full-game migration
and final gates. Continue from these owners; the common serializer and completed
entity/client/level records do not need restarting.

## #19 composed side state

Composed entity state now saves animation start/frame timing, trigger cooldowns,
sound state, damage radius and explicit flag words. Column records retain the
fixed 1,024-slot state without writing bool representation or padding. The reader
validates the whole record before optional application. GCC/Clang UBSan checks
compare every slot and execute the original animation callback after restore,
including wrapped clock bits (state-composed-{gcc,clang}.log). Invalid animation
timing prevents a save from being finalized. Gameplay callback bodies are unchanged.

## #19 level draft records

Level archives retain clocks, team/voting state, intermission data, location links,
and corpse queue slots. Runtime pool addresses, native struct size and log handles
come from the new map setup; spawn-parser scratch is frame-local and reset. Saving
mid-spawn is rejected. A byte comparison after explicit handle/reference rebinding
passes with GCC and Clang/libc++ UBSan (state-level-{gcc,clang}.log). Source coverage
now accounts for each entity, client/session/team and level member, including the
explicit transient ownership rules. Full server/game/bot coordination remains open.

## #19 client draft records

Client archives reuse the generated player/usercmd descriptions and explicitly
name all persistent, session, team, damage and timer fields. Grapple references
reload into relocated entity pools. The currently unused areabits pointer is
required to be null; unexpected ownership fails the save instead of disappearing.
GCC and Clang/libc++ UBSan pass (state-client-{gcc,clang}.log), including complete
client byte comparison after pointer relocation. This remains draft loading;
level/subsystem/bot records and full-game coordination are still required.

## #19 complete entity draft records

Entity numeric/spatial fields, both entityState_t records (including the legacy
shared r.s), checked references, callback identities and nullable strings now
compose one archive draft. The source gate requires an explicit description for
every gentity_t/entityShared_t member. Numeric tests preserve negative values,
high bits and signed zero; string tests preserve null versus empty and reject
truncation before mutation. Strings use caller-owned level-lifetime storage.
GCC/Clang/libc++ UBSan pass (state-entity-archive-{gcc,clang}.log); the missing
archive APIs failed first. Client/server build and focused clang-tidy pass; format,
types, boundaries, affected selection and the accepted replication wire digest
also pass. The first focused tidy invocation inherited a GCC-only warning flag;
rerunning with the Clang equivalent passes. No live-world load or full checkpoint
is claimed.

The generated replication state header is now state_replication_public.h so game
code can consume the shared descriptors through the enforced public boundary.
The archive record bound is 16,384: seven records per maximum entity population
plus 2,305 definition records already exceed the previous 8,192 bound. No format
bytes or accepted fixture changes. Game/client/bot/subsystem coordination, load
validation, clocks/transport, save-provider routing and full-game migration remain
required. Next implement client/level records and state owners, then wire the
existing failing fresh-process test through a complete checkpoint coordinator.

## #19 entity references

Checked slot indices now describe all entity/client links; items use classname
identity so a reordered item table still loads. GCC/Clang UBSan probes restore
into different entity/client pools, preserve unrelated fields, reject out-of-pool
pointers and invalid slots/items without mutation, and round-trip null references
(state-refs-{gcc,clang}.log). Capture and restore use bounded equality scans, not
pointer subtraction across unrelated objects. Full checkpoint records, strings,
subsystems, bots, transport reconstruction and frozen game migration remain open.

## #19 callback identities

Entity callbacks now have stable names in typed, owner-local tables, including
static callbacks and build-conditional entries. The state probe covers all seven
signatures, null callbacks, unknown names/functions, wrong-signature names and
no mutation on failure. A source gate accounts for all 77 callback assignments.
GCC and Clang/libc++ UBSan pass; the devtools client/server builds pass
(state-callback-{gcc,clang,build}.log). Existing callback bodies are unchanged.
Full entity references and checkpoint integration remain in progress; this is not
savegame acceptance. #18 merged-tree regression is still being monitored.

## #19 entity definitions share the named state format

The existing definition header, prefab and component-field POD records now expose
stateSchema_t descriptions beside their declarations. Edited pickup definitions
and composed model/collision/damage/audio definitions round-trip through the same
versioned archive used by replication state, then pass the existing cooked reader.
GCC and Clang/libc++ UBSan pass (entity-state-{gcc,clang}.log); the missing-metadata
probe failed first (entity-state-before.log). ASENT bytes and accepted assets do
not change. This completes shared metadata, not full savegame integration.

## #18 acceptance and #19 main merge

PR173 merged as 2010b077 after all 16 compiler legs and all 10 active regression
jobs passed at e0c6a57d. Self-review found the final diff within #18 scope, with
bounded POD state, existing callback semantics, no per-frame allocation or new
portable OS access, and preserved layout assertions. Exact head/base/main and
known-good tag checks were repeated after marking ready, immediately before the
merge. Tree 85cc5b3ce64af5c1e8aea281149cc527d0b71dc1 is shared by head and merge.
Issue #18 closed; the tracking issue is updated at this checkpoint. New main
workflows 35687912038/35687912012 are running and must be monitored.

The #19 branch merges this accepted main without rewriting history. Conflicts are
only additive test selection and progress prose; both feature entries are retained.
Combined local checks and full game checkpoint implementation follow. This merge
is not #19 acceptance. All accepted demos/goldens and rollback tags stay unchanged.

## #20 first hosted feedback: optional engine root

Draft PR176 at c95761ed passes all 16 hosted compiler builds (35724337548).
Regression 35724337593 runtime fails before the new package test: the existing
OpenArena bot golden contains one fewer search-path line. #20 unconditionally
added the absent engine directory to legacy installs; gameplay/bot rows are
unchanged. The failure reproduces locally (content-oa-bots-before.log).

Only mount the optional engine directory when present, using existing platform
file-stat access. Both unchanged OA bot goldens now pass, with hashes
51d66d9a8104db0cbc972a48a7911e00ca410db6ce675af7665947bd95a7bef9 and
0f2e6b686b63df73a054db39b886e7f82c5cf79078f3cbbf5cab0122ed87e3bd
(content-oa-bots-after.log). Complete OA package/root/config/mod/render/pure checks
also pass again, and all eight filesystem tidy configurations pass. No golden
regeneration or gameplay edit. Push the correction and require new exact-head
checks; current-base feedback remains preliminary until #19 main is merged forward.

## #20 draft-feedback workflow decision

Both complete local unit variants pass (content-unit-suite/suite-report.json
ok:true, full:false because only unit jobs were selected). Linux/MinGW builds,
full lifetime/tidy coverage, both-content package/pure/render acceptance, existing
bot goldens and both fixed replays have passed. No accepted golden changed.

Earlier notes kept #20 local while waiting for #19. With local acceptance complete,
open a draft into current main for early hosted compiler feedback instead of
waiting idle. This does not change merge order: #19 must merge first, then its
current main must be merged forward into #20 and fresh final gates must pass.
Any checks on the older base are preliminary evidence only. Self-review finds
only #20 package/root/test/docs changes, no authoritative simulation edits, no
new non-trivial core owners and no new per-read/seek allocations.

## #20 CI evidence retention

The runtime job retains only package logs, screenshots and size JSON, including
pure-session logs; package contents and game archives are not uploaded. The
AGENTS/tests/design acceptance commands include the dedicated server so the
pure-session portion is explicit. GCC's complete unit-job variant passed; Clang
is still running. PR174 integration has advanced beyond level authoring; PR175
remains in its level-authoring step. Continue monitoring both, with no red/skipped
required-check merge or main-base shortcut.

## #20 offline/native limit agreement

Self-review aligns offline mount limits with the native 64-package/65,536-visible-
asset limits and uses a set for removal membership during writing. The complete
functional package check still passes (content-native-final.log); artifact bytes
and accepted fixtures are unchanged. GCC's full unit job variant has passed; the
Clang/libc++ variant is running in content-unit-suite. No #20 PR is open yet.

## #20 final local gate checkpoint

Full lifetime analysis passes 1,320 production compilation commands/147 paths,
including shipping/devtools, static/module and all controls. Tidy covers 1,382
configurations with the separately recorded eight-filesystem recheck after the
single tool crash. Fixed Q3 and OpenArena replay hashes remain exactly
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4 and
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96.
Evidence: content-lifetimes.log, content-tidy/results.json,
content-tidy/recheck-results.json, content-demo.log, content-demo-oa.log.
The runtime test also now proves that packaged autoexec/q3config entries do not
replace loose user configuration; full OA package/pure/render acceptance passes
again. The full local unit job variants run via tests/suite.py under
content-unit-suite. PR175 and PR174 integration still await hosted runtime.

## #20 filesystem implementation and local acceptance

Native packages reuse the existing search-pack/hash/handle interface. File reads,
seeks and explicit close use the platform stream; modern metadata is excluded
from the legacy zip cache. Sorted independent/patch/DLC/mod mounts validate their
composed SHA256 view. Removal entries stop lower package/loose lookup and listing.
Reserved user configs keep the legacy archive exclusion. Existing pure checksums
bind modern identity/metadata; missing modern artifacts require installation,
while legacy pk3 downloading and reading retain their existing path.

Engine data is under fs_enginepath/engine, game data under fs_basepath/game, writes
under fs_homepath/active-game. Windows now uses the static OS application-data API
for its user default, replacing the inactive optional profile path. Linux/macOS
keep their existing home defaults. Missing home requires an explicit override,
never an implicit install-directory write. Explicit portable equal-root launches
remain supported. No simulation arithmetic or accepted fixture/golden changes.

Both Quake 3 and OpenArena pass roots, patch/removal/DLC/mod precedence, filesystem
restart, user writes, legacy gameplay and matching-package sv_pure client/server
sessions (content-mount-final-q3.log, content-mount-oa.log). The actual owned
character packages into 4,373 bytes; a 1,455-byte texture delta changes 1,518 native
preview pixels in fresh clients. Screenshot reviewed. The first native mount run
caught a missing legacy-cache exclusion; corrected within this unmerged feature.
The test setup also needed normal console `set` for new cvars and session setup
before stepping. Its corrected test fails the old binary specifically at the
missing engine mount (content-mount-before.log).

Linux client/server and MinGW client/server builds pass. Core unit golden plus
one-ULP negative control and both Q3 bot hashes pass unchanged (content-unit.log,
content-bots.log). Format/type/boundary and suite/affected/actionlint pass. Tidy's
full run had one tool crash while files.cpp was being edited; after stabilizing
that file, all eight of its production configurations pass (content-retidy.log).
The original full report and separate recheck report are retained. Lifetime and
fixed replay gates remain outstanding; no hosted acceptance is claimed.

CI registers the package probe on both compilers and real OA package rendering/
pure checks in runtime. AGENTS/tests docs and docs/design/packages.md describe
commands, roots, mounting, limits and installer-only modern content. Keep #20
local until #19 merges; final checks must include that current main.

## #20 filesystem acceptance, test first

The real-engine test packages owned configuration files in separate engine/game/
user roots. It requires a base/patch view, tombstone hiding of lower loose files,
additional DLC mounts, filesystem restart, user-only writes and playable legacy
pk3 maps. The accepted binary fails at the absent engine package mount
(content-mount-before.log). Commit the test before filesystem integration.
Implementation may proceed while #19's last runtime job runs; #20 final gates
must include accepted #19 main before its own PR can merge.

## #20 native stream implementation

The production reader validates fixed layouts, canonical metadata hashes and
ordered patch identities, then verifies each complete asset before exposing bytes.
Stored assets use 64-bit platform offset reads; compressed assets reuse puff with
a bounded zone buffer. Read/seek do not allocate. Explicit close/free releases all
resources. GCC and Clang/libc++ UBSan pass (content-native-after.log,
content-native-clang.log). Raw DEFLATE packages measure 14,949 bytes for the base,
2,077 for the one-texture delta and 460 for removal. No new dependency.
The filesystem has not mounted these packages yet; this is not #20 acceptance.

## #20 native stream contract, test first

The package test now compiles the future native reader and platform file stream,
requiring stored/compressed bytes to match cooked files, seek/tell accuracy,
matching patch identities, removed entries, no read/seek zone allocations and
complete handle/allocation release. Its initial compilation fails on absent
package_public.h/package.cpp/sys_content_file.cpp (content-native-before.log).
Commit this contract before implementing those owners.

Inspection found that the engine ships puff, not a public incremental zlib API.
Use raw DEFLATE through that existing decoder: compressed assets decode/hash once
at open into a bounded buffer; stored assets stream through platform offsets.
Audio can explicitly remain stored. The offline codec and format doc now agree,
and the tool-side exact patch assertions still pass before native compilation.
This changes only new, unaccepted #20 artifacts; no accepted fixtures are touched.

## #20 offline package and manifest-diff implementation

The writer uses Python's existing SHA256/zlib facilities, a fixed 128-byte header
and index entries with 64-bit extents, per-asset uncompressed hashes and canonical
JSON manifest. New/changed payloads and explicit removals form patches; required
base and resulting identities bind each diff to the current mounted view.
Stored and compressed assets coexist; source recipe manifests stay offline.
Atomic publication preserves previous output on failure, and extraction refuses
an existing destination. No new dependency or accepted asset/golden changes.

The owned-content test passes reproducible packaging, exact texture patch and
removal views. This is not native mount acceptance. The implementation/spec remain
local until #19 is accepted and main is merged forward.

## #20 first packaging acceptance test

Reuse tests/cook.py's owned source generator and the actual cooker. Patch a
separate texture source so exactly one cooked texture and its index/revision
change. The delta must omit unchanged model/audio payloads and stay below half
the full package size. Applying base plus patch must reproduce every cooked file
hash; a following deletion patch must hide the removed configuration file.
The first run fails on the missing packaging CLI after cooking succeeds.
This is tool-side preparation only; platform streams, native mounts, precedence,
directory separation and legacy compatibility remain required by #20.

## #18 hosted standard-library compatibility

Initial cb84dadb build 35681818460 fails Linux libc++ and all four Apple legs:
those libraries do not expose floating-point from_chars. The local libc++ 21
probe alone did not cover that older-library interface. Use standard strtof with
explicit fixed-decimal spelling, finite/range and delimiter validation; integer
from_chars is supported and unchanged. Field values remain the same decimal
strings expected by classic spawn parsing. GCC and Clang/libc++ probes pass again;
no fixture or existing engine math changes. The old regression is cancelled to
free runners and is never acceptance. MSVC ARM64 and release x64 already pass the
new entity feature at cb84dadb; require fresh full gates for this correction.

## #18 alias compatibility review

The completed local checks at 731bd28d pass: full tidy 1,358 configurations,
lifetimes 1,300 commands/144 paths, MinGW client/server, both-content editor and
composed acceptance, and unchanged fixed Quake 3 replay/restart hashes
(entities-{tidy,lifetimes,mingw-build,demo,editor-q3}.log). Self-review then found
a gap in the new alias feature: legacy CTF and path/target callbacks compare
native classname strings. Keeping the authored alias in that field breaks those
callbacks. The added real flag-prefab test fails enemy pickup as expected
(entities-flags-before.log). Preserve native identity in the existing field and
keep prefab identity separately for generic inspection. This is a correction to
unmerged #18 code, not a change to the accepted classic callbacks.
The first flag assertion mistakenly expected unlinking; classic pickups stay
linked with zero collision contents. Corrected the assertion and rebuilt the old
3b7071f0 in a detached private worktree: it fails the corrected pickup check
(entities-alias-before.log). The new alias dispatch passes pickup and capture
(entities-alias-after.log). Keep native classname for callbacks and a separate
private definitionName for authored inspection, cleared when a slot is reused.
Quake 3 also passes all three scenarios (entities-alias-q3.log). Fixed replay and
both classic bot hashes pass again after the correction (entities-alias-demo.log,
entities-alias-bots.log). Full tidy (1,358 configurations) and lifetime (1,300 commands) reruns pass; final gates must
include accepted #17 main. PR173 is draft, initial hosted head cb84dadb.

#19 test-only preparation is 10c1bee0 in the separate state-tree worktree, branched
from main. Its named-field added/removed/reordered migration test fails on the
missing serializer. The pure serializer now passes GCC and Clang/libc++ UBSan at 96db1f93. No game
checkpoint integration or #19 acceptance is claimed.

## #18 generic inspector and serialization

9b40aebb records the missing generic native editing/serialization API and real
save/reload requirements before implementation. Reflected component fields now
feed the Definitions panel and dev_definition command, using one validated native
edit path. Replication edits update their typed policy fields. Edits affect new
spawns, while numbered hashed cooked revisions preserve saved values across map
reload; JSON remains the inheritance/component source. Apply/save run after ImGui
returns, local cheats required, no new allocations or OS calls.

OpenArena full pickup/editor/composed sequence passes (entities-editor-runtime.log),
and the Definitions screenshot was reviewed. GCC and Clang/libc++ UBSan round trips
pass (entities-editor-{first,clang}.log). Format, type, boundary, suite/affected
contracts and actionlint pass. CI now includes both compiler probes and real
runtime acceptance with log/PNG artifacts. Quake 3 editor runtime and full
lifetime/tidy checks are running; not acceptance yet. docs/design/entities.md
records the format, component and save/source decisions. No accepted fixture changed.

## #18 initial prefab and real-pickup acceptance

JSON source/schema reuses the existing cooker. No ECS or scripting language.
32c39936 records missing entities cook; 679e94f5 supplies inherited bounded POD
records with component metadata and derived priority/radius. 8cf5cf24 records
missing native lookup; 738a895e passes GCC and Clang/libc++ UBSan with conversion/
shadow warnings. Fields are bounded/validated before publication, no native IO
or allocation. The current schema covers transform/pickup/hooks/replication;
model/animation/collision/trigger/damage/audio coverage remains unfinished.

acd59b9e records the real-client missing-prefab failure. 8ae8a5e3 routes map and
runtime prefabs through existing spawn/item callbacks, applies definition fields
before explicit instance fields, and reuses generic field editing. Classic
registered spawn/item behaviors get transparent compatibility definitions.
OpenArena real collection passes map amount 45 and runtime inspector-edited
amount 20 (entities-runtime-second.log); accepted old callbacks/arithmetic are
unchanged. The first runtime fixture assigned targetname, correctly leaving a
legacy pickup dormant until triggered; removed that hook from this collection
fixture, not engine behavior. First link needed entities_public.h included before
the game namespace in game/module.cpp. Both-content pickup collection now passes, and unchanged Quake 3 bot goldens
pass q3dm17 and q3dm7 (entities-runtime-q3.log, entities-classic-runtime.log).
The next component fixture requires model/animation/collision/trigger/damage/audio
metadata and fails on the missing schema fields (entities-components-before.log,
7571d023). Its schema/cooker/native metadata now passes GCC and Clang/libc++
UBSan (entities-components-{first,clang}.log). Runtime behavior for the new
composed backend remains to be implemented and tested; metadata alone is not
acceptance. Continuous audio loops are PCM WAV; authored events are one-shot.
Reuse cooked IQM frame animation through the existing general-entity renderer;
no new model format is needed. The real composed-entity test now fails at the missing composed behavior on a
rebuilt client (entities-components-runtime-before2.log); its earlier run used
an older reader and was not the intended backend negative control. It requires
visible owned IQM animation, solid collision, trigger damage/target activation,
sound-emission state and destruction by the real weapon (9ae1769c). The native
composed backend now passes this full OpenArena sequence
(entities-composed-second.log), including visible owned IQM animation, movement
blocked at the authored box, once-only 7 damage, target pickup activation and
real weapon destruction. The fixture respects the existing five-second team
switch cooldown. Splash radius lives in the new POD side state; the legacy
integer field is unchanged. Quake 3 composed acceptance also passes (entities-composed-q3.log); implementation
commit 685e379c. The next test requires generic default editing, reflected
replication fields and hashed serialization/reload; it fails compiling absent
Entity_SetField/Entity_WriteDefinitions (entities-editor-before.log). Generic definition
preview/edit still remains.
No partial acceptance of remaining #18 scope is claimed. Schema, format/types/boundaries and affected contract pass.


## #19 complete user-command description

The committed missing-schema test fb1246bc passes GCC and Clang/libc++ UBSan with
both engine and native game declarations (state-usercmd-{gcc,clang}.log). The
existing generator now reads the complete usercmd_t declaration, including its
separate byte movement fields, into the same named-field description. Unsupported
member declarations fail instead of being omitted. Existing replication.inc is
byte-identical and the accepted wire digest still passes (state-usercmd-wire.log).
Formatting passes. These descriptions are ready for the full game-state records;
server save/load integration remains outstanding.

## #19 user-command description test before implementation

Full client state includes its last usercmd_t, not just playerState_t. The existing
round-trip probe now requires that description, including byte-sized weapon and
signed movement storage, and fails on the missing schema (state-usercmd-before.log).
Extend the shared declaration inventory and verify unchanged replication output;
do not change user-command encoding or movement arithmetic.

## #19 libc RNG capture and preserved behavior

Owned engine rand/srand calls now route through Q_Rand/Q_Srand, retaining libc's
generator and tracking its seed, draw count and an eight-value signature. Explicit
restore replays at most 100,000,000 draws; a mismatched generator signature rejects
and restores the running stream. No per-frame allocation or FP arithmetic change.
The existing token stripper enforces ownership of raw libc calls in tests/state.py.

A follow-up pre-fix assertion exposed an initial-wrapper error: its first draw
reset an inherited libc stream to seed 1. The corrected wrapper preserves all
pre-seed draws and marks their state unavailable until the engine explicitly seeds
it (as server map startup already does). state-libc-initial-before.log records the
failure; both compiler tests now cover inherited state, same-sequence operation,
restore, signature rejection and the replay bound. Capture never reseeds a stream.

GCC/Clang UBSan, client/dedicated build, full tidy (1370 configurations),
format/types/boundaries pass. Both accepted bot hashes remain unchanged:
q3dm17 fea77580629db3b6d8b0130a64d41716e93b786b7d06b1ffea85e200eeb343e6;
q3dm7 14c8ee7d86fd8712533e75cfc44462045714c143208412a440eb119ceab83dd1.
Both fixed demos retain frame hash
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
Evidence: state-libc-{gcc,clang,build,tidy,format,types,boundaries,runtime,demo}.log.
Full game-state loading is still unimplemented; these are supporting primitives.

## #19 libc RNG checkpoint test before implementation

The common state probe now requires engine RNG wrappers to match libc's original
256-value sequence and resume after restoring seed/draw position. A saved-generator
signature mismatch must reject while preserving the running stream. The test
fails on the missing API (state-libc-before.log). Compile existing q_shared with
its production warning policy; keep strict conversion/shadow checks on new state
code. Do not alter random arithmetic or switch generators. Capture will track
owned engine calls, with a bounded replay count and a generator signature to
reject incompatible libc streams. Full real-client continuation remains mandatory.

## #19 bounded text implementation and local rebuild

The committed missing-String test 62c880e7 now passes GCC and Clang/libc++ UBSan
(state-string-{gcc,clang}.log). Named string fields retain a declared capacity but
store only terminated used bytes. Loading into a larger capacity is supported;
unterminated text and insufficient destination capacity reject before mutation.
Existing numeric/Bytes encoding remains unchanged. Focused tidy, formatting,
MinGW/AArch64 compilation, client/dedicated build and the real old-profile migration
pass (state-string-{tidy,format,build,profile}.log).

A private read-only rand/srand call-origin trace on the real local bot scenario
reports only the engine executable as a caller (checkpoint-rng-audit/checkpoint.log).
This is local evidence for investigating libc RNG capture, not a cross-platform
restoration guarantee. The native generator and libc generator remain distinct;
full checkpoint commands are still unimplemented.

## #19 bounded text state test before implementation

Entity state contains many separately referenced strings. The state probe now
requires a bounded string field to serialize only its used bytes and load after
the destination capacity grows from 32 to 64 bytes. It fails on the missing
String field kind (state-string-before.log). This avoids serializing native
pointers or filling each entity's file record with maximum-capacity empty text.
Existing profile Bytes fields and the frozen v1 profile remain unchanged.

## #19 native RNG seed exposure

The committed missing-getter test dca6282c passes GCC and Clang/libc++ UBSan
(state-rng-{gcc,clang}.log). Q_GetRandomSeed reads the existing full-width seed;
restoration uses existing srand. Neither original RNG function was edited, and
compiled instruction comparison confirms rand/srand are unchanged with both
compilers (state-rng-codegen.log). Format/types pass. This is native RNG exposure
only; botlib's libc state and full checkpoints remain outstanding.

## #19 native RNG state test before implementation

The state test now also compiles the real native bg_lib through its module
wrapper and production strict/wrap flags. It verifies 4096 original integer RNG
steps, serializes the full seed and requires the next 256 draws after restore to
match. It fails on the missing read-only Q_GetRandomSeed API (state-rng-before.log).
Expose the existing seed only; retain srand/rand arithmetic and use the existing
srand entry point for restoration. Botlib's separate libc RNG remains outstanding.

## #19 unsigned trajectory metadata correction test

Review of q_shared.h found trType_t has explicit uint32_t storage. The new save
metadata initially labeled its bytes Int32, although bit round trips passed. The
probe now requires UInt32 for both trajectory type fields and fails on that label
(state-enum-before.log). The generator correction now passes GCC and Clang/libc++ UBSan with both engine
and game declarations (state-enum-{gcc,clang}.log). The pre-existing network format
remains untouched.

## #19 additional local verification

The archive module compiles under MinGW and AArch64 with conversion/shadow errors
enabled. Client/dedicated rebuild and the real OpenArena profile/migration test
pass (state-archive-build.log, state-archive-profile.log). Workspace coverage now
also checks world overlay flags and fresh-process panel/cvar/filter restoration;
both content sets pass (profile-workspace-world-{oa,q3}.log). No profile fixture was
regenerated. Full lifetime analysis passes all 1312 configurations (state-profile-lifetimes.log); hosted #18 still has only its
normal authored-level/runtime block outstanding.

## #19 checkpoint record grouping implementation

The committed archive test b68b7dcd now passes GCC and Clang/libc++ UBSan for
engine and game types (state-archive-{gcc,clang}.log). The bounded caller-owned
container groups existing field records by schema name and slot, hashes the whole
payload, checks duplicate identities and preserves per-record versions. Failed
appends make finalization fail, so an incomplete archive cannot be published.
An initial magic-string spelling mismatch was caught by the first round trip and
corrected before acceptance. Focused tidy and format pass.

Before this addition, full profile/shared-metadata tidy passed 1370 production
configurations (state-profile-tidy.log). The lifetime run remains in progress;
full final analysis must cover the completed #19 implementation. No game save/load
implementation or merge acceptance is claimed yet.

## #19 checkpoint record grouping test before implementation

The existing state probe now requires multiple independently versioned records
in one caller-owned buffer, addressed by schema name and slot, and reads them in a
different order. It fails on the missing archive API (state-archive-before.log).
This is the minimal grouping needed for all entity/client/subsystem records in a
single checkpoint; reuse the existing field format rather than add another field
serializer. No game integration or acceptance is claimed.

## #19 real checkpoint test before implementation

New tests/checkpoint_runtime.py starts a real local game with a live bot, pauses
through the existing menu, and requires numbered savegame files. It then requires
exact paused-world restoration, identical resumed simulation and the same results
in a fresh process with a different initial seed. The pre-implementation run fails
at the missing save file (checkpoint-before.log). The first draft tried to set the
readonly pause cvar; the test was corrected to use actual Escape input before
recording the missing-feature failure. No engine change was needed for that.

This is an initial acceptance test, still to be extended for authored state and
version migration. Implement only after accepted #18 main is merged forward.
Bot/game/server clocks, RNG and callback/reference restoration belong in scope;
restoring only player fields cannot pass the continuation requirement.

## #19 shared replication descriptions

The committed missing-description test 8d1299b9 now passes with both engine and
native-game declarations under GCC and Clang/libc++ UBSan. tools/replication.py
emits public named, typed state fields from its existing exhaustive member
inventory, including local members and arrays; stale-output checks cover both
outputs. Every byte of entityState_t/playerState_t round-trips. The existing
replication.inc remains byte-identical and both compilers retain accepted wire
digest 26a5fc0d8e5afbfcc1634ddbfcf67155c6a2c6156066b86d20088690dff7d496.
Evidence: state-shared-{gcc,clang,wire-gcc,wire-clang,format,affected}.log.

This describes only the two shared network-state structs, not all game state.
Entity-definition metadata and complete checkpoint integration remain required.

## #19 shared replication metadata test before implementation

The state probe now requires full entityState_t/playerState_t round trips,
including local-only members and separately encoded arrays. It fails on the
missing public state_replication.h (state-shared-before.log). Generate its typed
named-field description from the same q_shared annotations already used by the
wire table; retain the existing wire output and digest unchanged. This avoids a
second manually maintained player/entity inventory. Full game state still needs
non-networked game and subsystem records beyond these two structs.

## #19 workspace implementation and migration verification

The committed workspace failure a12b78a8 now passes on both content sets
(profile-editor-first.log, profile-editor-q3.log). Version 2 adds named fields for
panel, selected cvar, filters, window layout and world-display preferences through
the same serializer. ImGui's existing memory settings API handles window layout;
no parallel layout format or OS access is added. Version 1 explicitly initializes
the new fields to Console, empty selection/filters/layout, disabled overlays and a
512-unit radius. Shipping/dedicated builds can read and ignore editor preferences.

Client/server build and format/types/boundaries pass. The runtime test checks
same-process restoration, fresh-process bindings/settings and the immutable v1
fixture. All data remains bounded POD; no per-frame allocation is introduced.
Full game checkpoints, shared entity/replication metadata and hosted final gates
are still outstanding; this is not #19 acceptance.

## #19 editor/profile migration test before implementation

Created one new frozen version-1 profile fixture using the e199e8bc client,
OpenArena and an isolated home. Review confirms 239 archived cvars, 51 bindings,
volume 0.31, no private/CD-key values or absolute paths; raw size 4,735,504 bytes,
gzip 8,508 bytes. Hashes/provenance accompany it under tests/assets/state. Its
creation command refuses to overwrite existing data; no accepted old fixture changed.

The extended real-client test requires panel/cvar/filter restoration and loading
that old file with explicit defaults for the newly added workspace. It fails on
unchanged Textures/r_mode/r_ selection after profile load (profile-editor-before.log).
Add workspace metadata through the same versioned serializer, with version-1
migration defaults. Full game checkpoints and shared replication/entity metadata
remain outstanding.

## #19 settings profile implementation

The committed real-client failure 63fcdb68 now passes with both content sets
(profile-first.log, profile-q3.log), including a fresh process and byte-identical
older revision. saveprofile/loadprofile use the shared named-field envelope,
archive cvar values (including latched values), stable key names and existing
filesystem/binding services. Private cvars/CD keys are excluded. Version-1 record
capacities are fixed, strings/counts/names/keys/registry capacity are checked before
applying values, and existing readonly/latch behavior is respected. Numbered user
files never overwrite an earlier revision. Legacy q3config.cfg remains readable.

Temporary profile/file buffers use the existing zone allocator only during an
explicit command and are freed afterward; no per-frame allocation or new OS access.
The fixed tables use under 5 MiB per revision. Initial build corrections supplied
the existing shared-type include and actual Cvar_Flags API name. Client/server
build, format/types/boundaries pass. Editor workspace and full checkpoint state
remain required, and this is not #19 acceptance.

## #19 profile test before implementation

`tests/profile_runtime.py` starts an isolated real devtools client, changes an
archived volume setting and a real F8 binding, and requires numbered profile
saves plus reload in the same and a fresh process. Earlier revisions must remain
byte-identical. It fails at the absent saveprofile file (profile-before.log) on
the accepted UI client. Profiles will use the common named-field serializer and
existing cvar/binding/filesystem services; numbered saves avoid overwriting user
data. No automatic replacement of legacy q3config.cfg is intended. Editor workspace
and full game-state acceptance remain separate required slices of #19.

## #19 field serializer implementation

The committed missing-API test (10c1bee0, introduction-version extension 26e2824d)
now passes GCC and Clang/libc++ UBSan (state-{first,clang}.log). The small shared
serializer emits a version/hash envelope and named typed fields, excluding native
offsets and padding. It validates schema/record bounds, types, duplicates and
required fields before mutating output. Added fields retain caller defaults until
an explicit version migration; removed fields are consumed without publication.
The probe preserves common bits through reordering/removal/addition and v2 reload.
Storage is caller-owned and core state is trivial; no IO or allocation occurs.
Production CMake and affected-test selection include the module. This is only the
common format, not checkpoint/settings/editor/replication integration or acceptance.
Focused tidy and MinGW/aarch64 compilation also pass. Source review finds 64-bit
generation identities in existing rewind state, so the next probe requires their
exact preservation; it fails on the missing UInt64 field kind
(state-identity-before.log). The added UInt64 field kind now passes GCC and
Clang/libc++ UBSan, including high-bit identities through migration/reload
(state-identity-{gcc,clang}.log).

## #19 initial migration test

`tests/state.py` compiles the bounded POD serializer probe under strict conversion/
shadow diagnostics and UBSan. Version 1 contains health, a removed item field,
position and name; version 2 reorders fields, removes that item and adds armor.
It requires bit-preserved common values, an explicit armor-default migration,
and a version-2 round trip. The initial command fails on the absent
engine/public/state_public.h and engine/qcommon/state.cpp (state-before.log).
New fields record their introduction version so missing required old fields cannot
be mistaken for a migration default. This test fixes only the serialization
contract; it is not full savegame acceptance.

## #16 weapon acceptance and final gates

ff0eab9a records the missing mixed-layer evidence before implementation. Role
counters now track actual nonzero PCM contributions. GCC and Clang/libc++ UBSan
component checks pass. Real two-client firing passes Quake 3 and OpenArena:
near roles 9592/9584/0 frames, peak 10783.330, room wet peak 1104.528;
far roles 0/0/9568, peak about 264, outdoor wet peak 0. Each phase receives exactly
one remote shot notify and all voices retire. Prepared storage stays at three
samples/57,600 bytes. Evidence: audio-weapons-{oa,q3}/ and matching logs.
The systeminfo capability passes real voice reconnect acceptance and both
content sets' unchanged bot goldens. Local MinGW rebuild, format/types/boundaries,
suite/affected contracts and actionlint pass. Added weapon acceptance to runtime
CI and corrected the new audio artifact paths to a YAML literal list.
Final hosted compiler/regression checks remain required; PR171 is still draft.
Head 9708869b fixed the meter warning and passed hosted bot goldens; MSVC then
reached a C4457 shadowed axis parameter in the new reverb-volume loop. Renamed
that loop index to component; behavior is unchanged. The pinned Opus source
manifest verifies byte-for-byte (upstream generated-file whitespace is retained).
No accepted golden or known-good tag changed. Full checks must use the new head.

## #16 hosted feedback and weapon acceptance in progress

Head 201bb883 hosted checks exposed two new-path failures: MSVC C4267 in the
small voice meter (bounded string length now explicitly converted to int), and
bot-log metadata drift from advertising sv_voip in gameplay serverinfo. Voice
capability now travels in engine systeminfo, like other transport settings; the
client reads the same configstring. Accepted goldens remain untouched. Both content sets now pass unchanged bot goldens
(audio-bot-capability-{oa,q3}.log); fresh hosted compiler gates remain. Runs: build 35672903379, regression
35672903380; private audio-ci-{msvc,runtime}-201bb883.log retain diagnostics.
The first two-client weapon test attempted explicit agent stepping; that mode
skips network polling, so the listener never joined. This is not weapon playback
evidence. Use ordinary clocked clients for UDP acceptance and retain offline agent
stepping for local simulation. No engine stepping change belongs in #16.

The ordinary two-client weapon run now fires through real remote animation
notifies: one near shot, three prepared layers, peak 10783.330, room wet peak
1104.528. It intentionally fails because per-role mix counters are absent
(audio-weapons-before3/). The component probe likewise fails compiling its
near/far layerFrames assertions (audio-layers-before.log). Add those counters
at actual layer mixing, then rerun near/far and room/outdoor acceptance.

## #16 voice transport/capture checkpoint

eaeff8af records the missing voice-wire API; 8ca2828b records real two-client
failure with zero encoded packets. The new client/server path keeps the reserved
Opus packet IDs and field layout, negotiates sv_voip/cl_voip, bounds server queues
and rates, and mixes decoded speech into the voice bus. The real OpenArena loopback
passes: 100 packets encoded/decoded, zero rejection/concealment/overrun, receiver
peak 8246.129, and no microphone opened (audio-voice-runtime-oa/).
Capture is explicit push-to-talk, with bounded SDL callback, native ALSA and Windows
wave-input storage; close on release/disconnect/shutdown. bf39439e records missing
native capture functions. GCC/Clang ALSA null-device and SDL dummy-device checks
pass, including buffer overflow/reuse and close/reopen. MinGW client/server build
passes (audio-mingw-build.log); native Windows device execution is not claimed.
The codec/wire probes also pass Clang/libc++ UBSan. Reconnect coverage now limits
the test server to two clients and waits out its one-second minimum zombie period
(the allocator deliberately prefers never-used slots). This reproduced the new
voice-path defect: second speaker in slot 1 decoded 0/rejected 100 because its
sequence restarted in the old generation (audio-voice-reconnect-before4/).
The relay now assigns a server-owned generation on each new speaker stream;
reconnect acceptance passes on OpenArena and Quake 3: 200 decoded packets, zero
rejection/concealment/overrun, empty final queues, and no microphone opened
(audio-voice-reconnect-oa/ and audio-voice-reconnect-q3/). Hosted MSVC also exposed a CRT include-order
warning in the new temporary-file helper: win_local.h's close macro renamed a
deprecated CRT declaration to _close. win_shared.cpp now includes io.h before
those macros, preserving strict warnings; all four hosted MSVC legs must rerun.
Final MinGW rebuild plus format/types/boundaries (502/418/419) pass. Actual layered
weapon acceptance and complete hosted gates remain. PR171 remains draft; no final merge approval.

## #16 bounded Opus component checkpoint

1e77cb27 records the missing Opus build/component contract. The verified 1.6.1
release is imported unchanged under third_party/opus with a per-file SHA manifest
and upstream notices; a static C build uses no optional neural features and never
fetches dependencies. The scalar codec uses stack scratch plus one prepared zone
allocation for one encoder/64 decoders. Each speaker has a fixed 120 ms PCM ring.
GCC and Clang/libc++ probes pass 200 real encode/decode frames with malloc/calloc/realloc
interposition rejecting any playback allocation, duplicate/duration rejection,
one-frame concealment and bounded overflow. This is component evidence; network
transport, explicit microphone capture and real loopback remain to be implemented.
Streaming acceptance now passes both content sets including 60-frame teleport
settling (audio-streaming-settle-oa/ and audio-streaming-settle-q3/).

## #16 stream playback checkpoint

5cb833ed records the missing stream-command runtime failure; 6e5238f3 records the
missing incoming-bus mixer API. The bounded adapter now prepares PCM once into a
private temporary file and reads through retained 16 KiB PCM/4 KiB stdio buffers.
Four slots support stereo music/ambient, loop and one-shot playback; restart reuses
prepared content. The real OpenArena client read an owned compressed pk3 source,
completed 122 combined loops with two preparations, 40,960 buffer bytes and zero
I/O failures (audio-streaming-oa/). GCC/Clang UBSan bus probes pass, including
stereo routing and muted-voice ducking. Format/types/boundaries pass (496/414/415).
No engine allocation/reopen occurs in MixStreams; synchronous disk reads remain a
known latency ceiling. No claim of general real-time storage latency is made.
Both content sets pass restart reuse and one-shot retirement. A concurrent Quake 3
run exposed an insufficient 10-frame teleport wait in the new test (outdoor zone
was reached only after the sound began); the scenario now settles for 60 frames,
and Quake 3 passes with zero outdoor wet output (audio-streaming-settle-q3/).
Remote head 646dec73 passed all 16 compiler legs and nine active regression jobs.
Runtime 35668445085 timed out in the existing delayed-hitscan scenario before the
audio test ran; logs/artifacts are being inspected, and this is not merge acceptance.

## #16 reusable stream file checkpoint

a2e53b29 records the missing temporary-storage API failure
(audio-stream-file-before.log). The platform helper now exclusively creates a
private temporary file and removes it on close (immediately unlinked on Unix;
Windows uses CREATE_NEW plus DELETE_ON_CLOSE). The filesystem opens it below
the writable home stream-cache directory with a caller-owned stdio buffer.
The small functional probe passes GCC/Clang UBSan: preserve an existing file,
200 rewinds with identical bytes and automatic removal. This is reusable I/O
infrastructure, not yet streaming playback acceptance. Ordinary CRT tmpfile was
rejected because Microsoft's documented implementation can require root-directory
permissions; no local permissions/package changes are needed.
Next spool decoded PCM once during stream preparation, retain bounded read buffers
for playback/loops, and route music/ambient frames through the same bus/duck mix.

## #16 authored room and occlusion acceptance

29f17b9d records the optional level-field failure before audio_zones existed
(audio-level-before.log). The v1/v2 level contracts now accept up to 32 AABB audio
volumes and emit audio_zone_N worldspawn keys. First matching volume wins; outside
volumes is dry. The existing two_lane MAP remains byte-identical when no zone is
authored (tests/level.py). The native reader rejects invalid bounds/parameters.
Both content sets now pass the extended real-client test with an owned compiled
room: wet peak 414.676 PCM units indoors, 0 outdoors; wall traces are blocked
(10/10 OpenArena, 7/7 Quake 3). A fixed 28,800-byte prepared buffer is reused
throughout, and every voice retires. Logs: audio-acoustics-oa/ and
 audio-acoustics-q3/, plus their matching .log summaries. The room level is a new
private derivative of owned source; accepted maps/goldens were not regenerated.
`s_event path.asevt x y z` exercises world-position playback; s_audioInfo includes
zone, wet peak and trace counters. This validates native static-world occlusion
and data-authored reverb, not yet actual weapon near/far or streaming/VoIP.
Next address streamed music/ambient with bounded reusable I/O, then VoIP and final
weapon acceptance. PR171 remains draft; final gates must include current main.

## #16 acoustic DSP checkpoint

Draft PR171 is open against main at 646dec73 for early compiler/regression feedback;
it is explicitly incomplete. 6ef49949 records the missing reverb API failure
(audio-reverb-before.log). GCC/Clang component probes now show a decaying room
response, dry outdoor output, frequency-dependent occlusion and recovery.
The reverb uses four fixed damped combs with rate-bounded delay storage and a
smoothed wet gain; no convolution/personal acoustic calibration is claimed.
Native acoustic wiring reads bounded audio_zone_N worldspawn metadata after CM
load and spends at most eight round-robin static-world traces per spatial update.
Dynamic occluders are not yet represented. The first build caught COM_Parse's
const cursor contract; corrected before continuing. Authored level emission and
real room/occlusion runtime acceptance remain next, followed by streams/VoIP.

## #16 both-content playback verification

3b84dd1d passes the real-client playback test for both Quake 3 and OpenArena
(audio-runtime-q3.log / audio-runtime-oa.log). Direct PCM checks also cover bus
gain and voice-driven duck/release. Added the runtime command and log artifact to
CI; spatial tests now run on both unit compilers. Workflow self-review caught a
duplicate run key in the earlier spatial-test insertion, which would have replaced
the ALSA command. It is removed; the existing ALSA command remains and the new
spatial command is in the unit step. No affected head was merged. Native source
format/types/boundaries pass at 495/414/415 before the next acoustic changes.

## #16 client playback checkpoint

2366b157 records the real-client failure before registration/playback existed
(audio-runtime-before.log and audio-runtime-before/client.log). OpenArena now
passes tests/audio_runtime.py through the SDL dummy output backend: one cooked
PCM buffer stays at 28,800 bytes across three plays (including HRTF), every voice
retires, and peak mixed output is 5,656.854 PCM units. The private client log is
in audio-runtime-oa/. GCC development client/server builds pass. Authored events
use the existing sound handle/animation-notify path and platform output. Up to
128 event records, 512 prepared PCM samples and 64 MiB are retained until sound
shutdown; individual PCM resources are capped at 16 MiB. Registration failure
rolls back newly prepared storage. Mixing, position updates and voice admission
make no allocation/file calls. No legacy sample path or accepted fixture changes.
This is an intermediate check: real weapon near/far, occlusion/reverb, streams,
VoIP, allocation instrumentation and final multi-platform gates remain.

## #16 mixed PCM checkpoint

76876e91 records the missing mixer API failure (audio-playback-before.log).
The bounded mixer now passes actual PCM checks: near mechanical+tail summation,
far-only selection with inverse attenuation, sample exhaustion, HRTF tail
retirement, group limits and priority rejection/stealing. GCC and Clang/libc++
UBSan both pass. The mixer holds at most 96 event voices, each with four prepared
PCM views and fixed HRTF state. No allocation/file/device calls occur in its
sample loop. Bus gains and a voice-driven music/ambient duck envelope are present;
additional direct bus/duck checks and engine registration/runtime wiring remain.

## #16 latest verification

Through bcdabe5d, both new component probes pass with GCC and Clang/libc++ under
UBSan. The development client/server rebuild passes (audio-build-events.log),
as do the existing full cooker and authored-schema regressions (audio-cook.log,
audio-formats.log). Agent-format checking first failed because Go was absent
from PATH; rerun with the documented private Go toolchain passed. Format (494),
types (413), boundaries (414), affected/suite contracts and diff whitespace pass.
No runtime feature claim is made: authored registration, mixing, buses/ducking,
occlusion, map reverb, bounded streams and functional VoIP still need implementation
and full gates. No accepted golden/demo changed. Issue #16 checkpoint comment
5768068520 records scope and evidence.

Next trace registration lifetime and layer sample ownership before wiring playback.
Existing legacy sample chunks can be evicted and loaded during playback; do not
claim that reusing a handle guarantees no frame allocation. Reuse existing codec
and platform paths, but keep prepared authored PCM bounded and retained for active
voices. Preserve ordinary sample behavior and the existing animation-notify dedup.
Private current development outputs are in audio-client-build/release-linux-x86_64.
All private evidence paths here are relative to the modernization cache.

## #16 authored event cooking

The real cooker now passes tests/audio_events.py after the missing-kind failure
recorded in 4a22cbdc. New sound-event schema and agent example describe a bounded
four-layer .asevt record: 400-byte payload plus existing 48-byte hash envelope,
resource index kind 12. Fields cover bus/group/priority/voice limit, distance
model, Doppler/occlusion flags, reverb send, and mechanical/tail/distant PCM
paths with gains and distance ranges. Incremental no-op and edited-layer cooks
pass. Renderer resource-index validation admits the new kind; no renderer
behavior changes. Existing weapon sound path strings can reference this record
without changing weapon layout. Native event decoding and layer selection now pass the real cooked round trip
following the missing-API failure in 6704d956 (audio-event-native-before.log).
The decoder checks the existing hash envelope, fields and sample qpaths into
a fixed POD record with layout assertions. Native playback is not wired yet; no #16 completion or runtime acceptance is claimed.

## #16 authored event contract

The full development client/server build passes with the spatial component
(private audio-configure.log and audio-build.log). tests/audio_events.py now
exercises the real cooker for an authored mechanical/tail/distant event, existing
hash envelope, bus/group/priority/voice limit, spatial settings and incremental
edits. The first run fails because sound-event is not an implemented asset kind
(private audio-events-before.log). Next implement cooking/schema and native
registration/playback. #15 merged-tree lifetime analysis has passed; runtime is
the only active regression job still running.

## #16 binaural component checkpoint

5981cabe records the missing-API failure for the binaural test
(private audio-hrtf-before.log). The implementation uses Brown/Duda equations
2–5, verified against the primary paper, with a causal fractional delay and
one-pole/zero shadow filter. Per-voice storage is a fixed 512-sample ring and
trivial state; coefficients are configured outside the sample loop. Supported
rates are 8–192 kHz and head radius is tunable within 5–15 cm. No pinna/elevation
or subjective listening validation is claimed. GCC and Clang/libc++ UBSan probes
pass: impulse arrival, shadow energy, mirrored channels and DC stability.
Format (491), types (411) and boundaries (412) pass. The component is in CMake,
CI and affected-test selection; actual mixer integration remains next.
Merged-tree #15 build/publication 35658235970 passed; regression still has
lifetime/runtime jobs running, with the other eight active jobs passing.

## #16 initial spatial contract

Decision recorded on #16 in comment 5767944870; #15 merge report is comment
5767944695. tests/audio_spatial.py now specifies equal-power stereo placement,
linear/inverse distance attenuation, radial Doppler and invalid-input rejection.
The first run fails at compilation because the new snd_spatial API does not yet
exist (private audio-spatial-before.log). This is an API-first failure, not a
claim that legacy playback failed. The implementation now passes with GCC and Clang/libc++ under UBSan. It uses
no allocation or engine state and is listed in the client CMake sources. Engine
integration and the remaining #16 scope are still pending.

## #15 merge checkpoint

PR170 merged into main as 0561e0f0446e96f6dca51f86bae56337d6cd23f5, with parents
0928be35 and tested head b4de8b69c1cf46aa132565ebeb1dd780fa7812c9. All sixteen compiler
legs passed in 35650410977 and all ten active regression jobs in 35650410983,
including runtime 106501160022. Final audit checked zero required failures/skips,
current main/base and exact head immediately before merge, and no accepted fixture
changes. Tested and merged trees both equal c233fb96ac2679d4a66e316d2bce6c14bb4b8309.
Known-good remains object 8bc8c94c75e7c9ae59ee3fe1277e084942dda5f4 pointing to
81a0f9dc05c340f30182c34134bde67290c21774, verified against origin after merge.

Self-review: #15 scope; no authoritative arithmetic or wire/file layout changes;
no new direct OS calls; POD owned core; caller-owned arena; no step, activation,
query or recycle allocations. Dedicated-server symbols contain no Jolt linkage.
Both content runtime sets, Physics panel, fixed demos and native animation hit-box
parity pass. Full local lifetime analysis (1,268 commands), tidy policy (1,326
configurations), format/types/boundaries and affected/suite/workflow checks pass.

Private physics evidence: physics-ragdoll-after.log, physics-ragdoll-clang.log,
physics-death-q3.log, physics-death-oa.log, physics-demo.log, physics-demo-oa.log,
physics-lifetimes.log and physics-tidy.log under the modernization cache. Earlier
hosted run 35649848654 also passed all ten active jobs. Its selected artifact is
in physics-hosted-evidence/: death/restart retain arena=29498800, allocations=573,
live=424, then retire the ragdoll and finish with live=0. Captures were reviewed.
Issue checkpoint comments: 5766814918 and 5766901466; final merge report follows.

## #161 accepted merged tree

PR169 merged as 0928be35e47616d17bf8a36475f86952c11f0bbb. Final head b9ec426c passed
all 16 compiler legs (35631254291) and all ten active regression jobs (35631254305).
Main/base 07304b32 and the exact head were rechecked before merge. Tested and merged
trees both equal f21454591ae494c1f7d023b6fd5d063c005a4b29. Merged-tree publication
35638549208 and all ten active regression jobs in 35638548512 passed, including
runtime 106461943478. #161 is accepted and checked in #25.

## #15 hosted MSVC header failure

Initial hosted build 35649848657 failed both Windows ARM64 configurations. Debug
job 106499263710 reports C4530 promoted to C2220: including Jolt/Jolt.h from the
owned adapter instantiates Vec4's ostream formatter under /EHs-c-. The adapter
needs only allocator declarations, so replace the umbrella include with
Jolt/Core/Core.h plus the existing Memory.h; do not enable exceptions or suppress
the warning. Local client rebuild and both compiler probes precede the next push;
hosted MSVC builds remain required evidence. Log: physics-msvc-arm64.log.
Release Windows-MinGW job 106499262967 also rejects %zu in the new Com_Printf
world diagnostic. Both new world/status diagnostics now use unsigned values
within the fixed 128-MiB client arena limit; no engine-wide formatter change.
Log: physics-mingw-release.log. All MSVC configurations report the same umbrella
header problem. Client rebuild and both compiler component probes pass with the
narrow allocator include; hosted confirmation follows.

Draft PR170 is still unmerged. The documentation-only head 1f4a66d9 queued runs
35650061544/35650061617 before this failure was diagnosed.

## #15 death/runtime and unchanged replay checkpoint

Both complete content runtime commands now pass: four prop/contact map lifetimes
plus normal fall-damage deaths on q3dm1 and oa_dm7, rendered ragdoll captures,
fixed allocation/live-block counters through activation and map_restart, and
zero live blocks on disconnect. The failing restart assertion now passes after
CG_ClearPhysics resets props/ragdolls on both map restart and time discontinuity.
Quake 3 uses a 256-unit drop and OpenArena 320 units, below/within their respective
map geometry. Captures were reviewed; the owned articulated body renders.

Both accepted fixed-demo sets are unchanged: Q3 digest 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4,
OpenArena 17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96.
Format, affected/suite contracts and workflow syntax pass. Tidy source selection
had the same generated dependency PCH failure as lifetimes (physics-tidy-before.log);
apply the same owned-source filter and rerun. Full hosted gates remain pending.
Issue checkpoint: comment 5766814918. Next finish local analysis, open #15 PR into
main and require all hosted checks green before merge.

## #15 replicated death and restart test first

A normal fall-damage death activates/renders the 16-joint skeleton on q3dm1 and
OpenArena oa_dm7. The permanent runtime driver now requires this path and capture;
its Quake 3 restart assertion fails because map_restart leaves cosmetic deaths
active (physics-death-q3.log). Fix cosmetic reset centrally and reuse it for map
restart and time discontinuities. The OpenArena fall setup uses oa_dm7 and 320
units of height; oa_dm1's roof/low ceiling did not produce the required fall.
Unchanged Quake 3 fixed-demo replay passes frames-mesa-26.0.8.json, digest
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.

## #15 skeleton ragdoll implementation checkpoint

The owned ragdoll gate now passes GCC and Clang/libc++ UBSan: four 16-joint
ragdolls, three activation/pose/recycle cycles, two full lifetimes, no allocations
after setup and complete cleanup. Logs: physics-ragdoll-after.log and
physics-ragdoll-clang.log in the private modernization cache. The client and
server build succeeds; format, types and boundaries pass. Existing animation
runtime still passes server/client hit-box equality and rendered body/rifle poses.

Cgame now prepares a four-ragdoll pool at map load and samples cosmetic death
poses separately from authoritative animation hit boxes. Real death/runtime
acceptance is still pending; do not treat the component gate as that evidence.
Unsupported non-rigid/scaled skeletons retain native death presentation. The
lifetime check initially rejected an out-of-tree generated Jolt PCH source before
analysis; source selection now excludes generated dependency translation units,
with the full gate rerunning. No accepted fixture was regenerated.

## #15 skeleton ragdoll test first

The permanent physics driver now requires four prepared ragdolls from the owned
16-joint body animation asset, three activation/pose/recycle cycles, two complete
lifetimes and unchanged allocation counters after setup. It fails at the absent
ragdoll API (physics-ragdoll-before.log). Use the pinned binding's existing
Skeleton/Ragdoll APIs behind POD records, with load-time preparation and excluded
inactive layers; do not recreate a foreign RAII implementation in engine code.
Cgame death/corpse presentation and runtime death evidence follow the owned gate.

## #15 Physics panel and gate wiring

The shared developer test now opens Physics, throws a box, verifies twelve
projected collision edges, and captures the panel. It passes together with idle
allocation, input-release and renderer-restart checks (physics-devtools.log,
physics-devtools/runtime/physics.png). The original dropped-at-camera fixture
counted only visible/projected lines, so it was replaced by a thrown-in-view box
without weakening the twelve-edge assertion. The checkbox now reads the cvar
before drawing, so external console/agent changes remain visible in the UI.

Both unit compiler variants run physics.py; hosted runtime runs physics_runtime.py
against OpenArena and preserves its client log. The local suite derives these
steps from the workflow. Affected-path coverage includes physics source, binding,
CMake, probes and fixtures; the contract, suite shell catalog and actionlint pass.
AGENTS and tests/README document the commands. Skeleton deaths and all final
acceptance gates remain. Nothing is merged for #15 yet.

## #15 outward winding passes all four runtime maps

The fan export now swaps its second/third vertices, leaving the original CM
winding and trace code untouched. The outward-normal check fails before the fix
and passes after it. GCC and Clang/libc++ UBSan pass all component checks
(physics-winding-after.log / physics-winding-clang.log). Both Quake 3 maps and
both OpenArena maps pass real client prop/grenade motion, floor bounce, unchanged
allocation/high-water/live counters and zero live blocks at every map teardown
(physics-runtime-q3.log / physics-runtime-oa.log, adjacent client.log artifacts).
Continuous collision detection remains enabled for small dynamic cosmetics;
it alone was insufficient to fix the reversed faces. The ImGui Physics tab is
built and its shared panel/collision-bound visualization test is running.
Skeleton deaths, fixture/demo gates and hosted acceptance remain.

## #15 runtime found reversed exported triangle winding

OpenArena passed with continuous collision detection, but a subsequent Quake 3
map settled the grenade below the native floor instead of bouncing. The isolated
flat-mesh drop passes, isolating map export. Source inspection found the cause:
BaseWindingForPlane emits clockwise faces; Jolt MeshShape requires counter-clockwise
faces. The synthetic geometry test now checks outward normals as well as count
and area, and fails on the current export (physics-winding-before.log). Reverse
fan indices in this new export, then rerun both content sets before acceptance.
The existing CM winding/trace code is unchanged. This is #15 integration code,
not a previously merged engine defect.

The uncommitted client runtime/panel work is otherwise built: an ImGui Physics
tab queues prop/drop actions after vendor calls and toggles collision bounds.
It still needs runtime panel verification, skeleton deaths and full gates.

## #15 first client prop runtime passes

The client now owns a 128 MiB map-lifetime arena, exports the loaded solid world,
and retires Jolt before freeing its storage. Foreign exhaustion uses process-fatal
handling with reentrant physics cleanup disabled, never a Com_Error unwind.
Cgame prepares 32 cosmetic box/grenade slots and steps at 60 Hz with bounded
catch-up; seek/stall retires cosmetic props. physics_prop and physics_status expose
local presentation only. No live weapon trajectory or damage path changed.

The runtime driver passes both installed Quake 3 maps: props move, dropped inert
grenades fall and bounce, frame/spawn allocator/high-water/live counters remain
fixed, and map replacement works. First world: 5,436 triangles, 30,173,824 arena
bytes before prop preparation. Evidence: physics-runtime-q3/client.log and
physics-runtime-q3.log. The earlier resize/hidden-window lifecycle check also
passed with world loading (physics-window.log). Explicit zero-block teardown
logging, OpenArena, full demo gates, ragdolls and tooling remain next.

## #15 static map geometry foundation passes

The owned world accepts bounded static triangle meshes. GCC and Clang/libc++
UBSan pass the full contacts/ray/sweep/recycle suite using a two-triangle floor.
CM_PhysicsTriangles is a separate load-time export: solid world brushes/patches,
deduplicated leaf references, inline models excluded, callback capacity stop with
all temporary storage freed. Its synthetic cube/patch fixture checks 14 triangles,
area 28 and inline exclusion, without loading game content. Both compilers pass
(physics-collision-test.log / physics-collision-clang.log). Client/server rebuild
passes (physics-map-build.log). No native trace expressions changed.
Next connect map allocation/teardown and cosmetic client presentation; no runtime
caller or #15 acceptance claim yet.

## #15 static mesh test first

The owned boundary test now uses two static floor triangles rather than a box.
The first run fails at the missing mesh API (physics-mesh-before.log); the same
full-capacity contact, ray, sweep and recycle checks will exercise the mesh.
No game content or accepted fixture is involved.

## #15 client target builds with the owned module

CMake links Jolt/joltc and engine/physics only into the client. Strict FP options
apply to the owned module; MSVC runtime selection matches engine static runtime.
The development client and dedicated server build successfully in
physics-client-build. Format, type and subsystem gates pass. clang-query-21 with
the real client compilation command finds no owned non-trivial stack/global or
temporary destructors (physics-lifetimes.log). The lifetime core list now includes
physics. No runtime entry point calls the module yet; map loading/presentation
and full required hosted gates remain.

## #15 owned boundary and query allocator checks pass

GCC and Clang/libc++ UBSan pass the full owned-boundary driver, including missing-
joint control and zero independent C++ allocation calls during setup/steps.
Managed object-layer/body filter wrappers now use Jolt's registered allocation
operators; provenance retains the original hash and current adapted hash.
Evidence: physics-filter-after.log / physics-filter-clang.log. The recorded
foreign prop scene hash remains unchanged. Client linking/integration is next.

## #15 prepared joint test first

The full-capacity boundary scene now requires a swing/twist joint between two
prepared body slots, including invalid-slot rejection. Its existing four-cycle
and shutdown checks cover activation, reuse and lifetime. Initial compilation
failed at the absent joint API (physics-joint-before.log). The implementation
passes full-capacity recycle with opposing velocities; omitting the joint fails
the separation assertion (physics-joint-negative.log). A weak initial negative
control did not fail, so the fixture now pulls the pair apart with opposite
10 m/s velocities. The driver retains and requires this negative control.

Independent C++ allocation counting then exposed two setup allocations outside
the supplied arena, in JPH_ObjectLayerFilter_Create and JPH_BodyFilter_Create.
Debugger stacks and physics-joint-final.log identify the managed filter wrappers.
The test currently fails on that ownership violation; adapt their allocation
operators before claiming the owned boundary passes.

## #15 shape sweep test first

The boundary probe now requires a prepared sphere sweep against the floor,
checking its contact fraction separately from the ray and rejecting an invalid
slot. This first run fails to compile because Phys_Sweep is not implemented
(physics-sweep-before.log). GCC and Clang/libc++ UBSan now pass the implemented convex sweep with zero
post-setup allocation (physics-sweep-after.log / physics-sweep-clang.log).
The query reuses a prepared shape and excludes its body and inactive slots;
it does not allocate a query shape or collector container. Boundary checks pass.

## #15 prepared slots use collision-layer activation

The initial owned POD module caught a real recycle allocation: AddBody allocates
BroadPhaseQuadTree LayerState after a RemoveBody. Evidence is
physics-boundary-after.log and a debugger stack through AddBodiesPrepare.
The module now retains prepared bodies in the broadphase, deactivating them on
an object layer with no collision pairs and excluding that layer from queries.
No additional vendor patch is needed. GCC UBSan passes all 256 prepared slots,
four complete recycle cycles, ray queries and two arena-owned lifetimes with no
post-setup allocation. Inactive-query coverage caught a callback-table lifetime error during development;
the table now has static POD storage, as required by the C binding's retained
pointer. GCC and Clang/libc++ UBSan both pass the active/inactive query checks
and repeat lifetimes (physics-boundary-final.log / physics-boundary-clang.log).
The module remains standalone, not linked to client targets; constraints, shape
queries, map collision and presentation integration still remain.

## #15 owned-boundary failing test

The permanent driver now requires the owned physics module. Its probe prepares
all 256 slots, exercises four full spawn/step/despawn cycles, casts a ray, compares
allocation/high-water counters after setup, and tears down/reinitializes twice.
The first run fails at the absent engine/physics/physics.cpp, retained in
physics-boundary-before.log. Implementation follows this test; no runtime claim.

## #15 standard temporary storage is bounded

The C binding's standard explicit buffer and legacy default now use fixed
TempAllocatorImpl storage. The 1 KiB child-process case reports explicit exhaustion
instead of allocating; GCC and Clang/libc++ UBSan both pass the normal/restart,
changed-impulse and exhausted-buffer checks. The scene hash remains be15e66a...
(the complete hash is recorded below). Evidence: physics-temp-after.log /
physics-temp-clang.log; the driver retains temporary-exhaustion.log.

Next implement/test the owned POD physics boundary, using caller-owned arena
storage and prepared body/shape pools. Prove full capacity, spawn/recycle and
queries before connecting client props, skeleton death presentation and tooling.
The dependency is still not linked into engine targets. #15 has no PR yet.

## #15 temporary-buffer negative control fails before fallback removal

The permanent probe now runs a 1 KiB temporary-buffer case in a child process.
It requires an explicit TempAllocator exhaustion diagnostic and rejects a heap
fallback. The baseline instead reports five allocation calls on step 0 and fails
(physics-temp-before.log). Normal-size runs still pass. Next replace standard
and legacy-default fallback buffers with fixed-capacity TempAllocatorImpl and
verify the negative control on both compilers.

## #15 caller ownership and full restart pass

JPH_Init now preserves a complete caller allocator configuration and rejects a
partial one before allocation. The opaque world wrapper uses Jolt's allocation
operators. Destroying the final world clears the lookup table's backing storage.
The intermediate test caught exactly one retained block before that cleanup
(physics-ownership-reset-before.log); initialization callback replacement is
retained separately in physics-ownership-before.log.

GCC and Clang/libc++ UBSan tests now pass full setup/600-step/teardown twice per
process for each replay variant. Setup makes no C++ allocation outside registered
callbacks, stepping makes no allocation, shutdown has zero outstanding blocks,
and reinitialization produces identical results. Partial allocator configuration
is rejected. The recorded-scene hash is unchanged. Evidence:
physics-ownership-after.log / physics-ownership-clang.log.

Next: prove temporary-buffer exhaustion cannot fall back to allocation, then
implement the owned POD physics boundary and fixed-capacity lifecycle. No engine
target links physics yet, and #15 acceptance remains open.

## #15 allocator ownership test fails before initialization changes

The permanent probe now installs all five allocation/free callbacks before
initialization, requires all dependency storage to use them, counts outstanding
blocks at shutdown, and repeats the scene twice in the same process. It fails
because JPH_Init overwrites the supplied callbacks (physics-ownership-before.log).
This is the next test-first integration change. Full reset must release the
world lookup table before the caller retires its arena. No engine physics code
has been introduced yet.

## #15 allocation regression passes on both compilers

The reviewed adaptation reserves NodeID and center scratch per broadphase layer
at initialization; only serialized UpdatePrepare uses that storage, and concurrent
body insertion retains separate storage. The callback job pool prepares its one
fixed page before stepping. Original file hashes remain in provenance alongside
explicit hashes for changed files. No simulation arithmetic was changed.

The permanent test passes with GCC and Clang/libc++, both under UBSan. It checks
Jolt allocator calls and independent C++ new/new[] calls (including aligned forms),
FP control, 600 steps of recorded impulses on 32 bodies/16 constraints, replay
agreement and a changed-impulse negative control. Both produce SHA256
be15e66ae62c57bcd803c017095f9b5f7e4e30466b6d6bdb2a849bd06b78c438 for this scene.
This is measured agreement, not a claim about every platform/scene. Evidence:
physics-reserved-allocation-test.log / physics-clang-test.log. Format passes.

Next: allocator ownership, no-fallback temporary storage and teardown must be
proven before engine integration. The upstream wrapper's world lookup table
retains capacity and its small wrapper allocation uses global new; inspect and
cover complete reset/reinitialization with a counted allocator. Full body/node
capacity, recycle, queries, ragdolls, tools and unchanged movement remain open.

## #15 pinned original dependency and concrete failing gate

Imported 561 Jolt files from 5.6.0/e77f175595e64cb44218cc9d9d56fc365ad0e36a and
six joltc files from 886e088675bae3a086f8318c7803f8ee962c2f2c. MIT licenses and
archive/per-file SHA256 provenance are retained. Sources are still original.
cmake/Physics.cmake scopes conservative static-library options, disables GPU
compute/newer x86 instructions/exceptions/RTTI and refuses absent local Jolt
sources. It is not yet linked into engine targets.

The permanent probe builds with GCC/UBSan, then reports `physics step 0: 1
allocation calls` and fails its zero-allocation assertion. Evidence is
physics-original-test.log; this is the real baseline failure before vendor
adaptation. The probe's mutable impulse argument was corrected to match the C
API. Format passes (479 owned files). No engine behavior or accepted fixture changed.

## #15 first permanent test

Added tests/physics.py, its native probe and a four-command recorded prop scene.
The probe drives 32 bodies with 16 constraints over 600 fixed steps, checks Jolt
allocator calls and FP control state, and writes ordered final transforms. The
driver compares independent replays and changes the final impulse as a negative
control. No accepted golden is created or overwritten. The initial command fails
at the absent CMake integration, as expected; physics-test-before.log retains it.
Dependency import and actual allocation failure/fix are next; this is not acceptance.

## #161 merge verification

PR169 was marked ready and merged only after all 26 required checks succeeded.
The exact head and current main/base were rechecked immediately before the merge.
The tested/merged tree identity and unchanged remote known-good tag were then
verified. Issue comment 5765472685 records final measurements and self-review.
Presentation artifact 10656828055 retains the final-head frames and metrics.
Merged-tree workflow runs are recorded above; acceptance is pending those runs.

## #161 final-head hosted acceptance (completed before merge)

Final head b9ec426c passed build 35631254291 (all 16 compiler legs) and
regression 35631254305 (all ten active jobs). Runtime completed all presentation,
animation, weapon, network, level, module and UBSan controls. Prior-head results
were supporting evidence only and were not substituted for final-head gates.

Preliminary reading for #15 is retained privately in physics-research.md beside
the evidence. No physics implementation, branch, vendoring or integration decision has started.
A private pinned Jolt/joltc static-build and link smoke passes; it is dependency
research only. Allocator fallback and steady-state allocation controls still need
implementation and verification in #15. Sources/builds remain outside the repo. A private 600-step allocation probe
first reports 90 allocating steps (lazy job page and recurring broadphase scratch);
reserving scratch and warming the job page reduces this limited scene to zero
with identical final transforms. A 32-body expansion exposed another recurring
center-array allocation; reserving update-only center scratch also removes that
allocation with the same reported final transform. Full capacity, allocator and ragdoll coverage
remain #15 work; this experiment does not accept or implement that issue.
The 32-body/16-distance-constraint probe also has zero calls over 600 steps.
Preliminary evidence and limitations are recorded on #15 in comment 5765263819.
The private dependency also passes MinGW compilation/static linking (not Windows
execution); experimental-allocations.patch preserves the four-file scratch changes.

## #161 hosted/reference agreement and local content repeat

All four PNGs downloaded from presentation-metrics artifact 10653769125 on run
35629405076 match the independently generated Mesa 25.2.8 references pixel-for-pixel.
Artifacts are retained under fidelity-ci-presentation/. The local Quake 3 / Mesa
26.0.8 captures were reviewed and a fresh comparison passes
(fidelity-quake3-reference.log / fidelity-quake3-repeat.log). Exact references now
cover OpenArena on both supported Mesa versions and installed Quake 3 on local
Mesa 26.0.8. Only new version/content variants were added; no accepted PNG changed.
At 8d737daf, all compiler legs and eight active regression jobs pass; runtime and
lifetimes are still running. The content-selector checkpoint requires its own
final-head hosted success before merge. No engine or shader code changed.

## #161 local Quake 3 content control

The documented default local invocation was also checked against ~/.q3a. Its
owned-scene baseline differs from OpenArena at a few edge pixels (mean channel
error below 0.0033, maximum 84). The new test therefore follows the existing replay
content separation as well: new Quake 3 / Mesa 26.0.8 frames live under
fidelity/quake3, while all OpenArena references stay unchanged. Installed paks are
only linked into the private engine home; none are copied or uploaded. New local
content references still require review and an independent exact repeat.

## #161 per-Mesa exact references pass locally

The separately reviewed Mesa 25.2.8 frames pass a fresh exact comparison, and
Mesa 26.0.8 still passes its unchanged original four PNGs. Evidence:
fidelity-mesa25-reference.log, fidelity-mesa25-verify.log and
fidelity-mesa26-verify.log. Between drivers the maximum per-channel difference is
two RGB code values; mean channel error is below 0.022. This explains why a single
cross-driver exact reference failed, but the gate remains exact per driver.
New 25.2.8 captures are under tests/golden/fidelity/mesa-25.2.8; hosted artifact
comparison remains required. No shader, engine behavior or accepted golden changed.

## #161 hosted software-driver reference mismatch

Initial runtime 35628249523 passes streaming and the individual presentation
controls, then fails the combined baseline's exact comparison. Hosted replay logs
identify Mesa 25.2.8; the existing new local fidelity frames use 26.0.8. The same
failure is reproduced locally with privately extracted Ubuntu Mesa 25.2.8 and
LLVM 20 packages (fidelity-mesa25-before.log); no system package is installed.
Driver package SHA256 is b3be471db0ec27eb28c1ea75a4d2e6bb3207c5904bdc3823dbba9ce530026939,
verified against packages.ubuntu.com/noble-updates/amd64/mesa-vulkan-drivers/download.

Follow the existing replay gate's per-Mesa exact-reference convention: retain the
four accepted 26.0.8 PNGs byte-for-byte, add separately reviewed 25.2.8 references,
and fail unknown/missing driver references. No tolerance is relaxed and CI cannot
record frames. New references must repeat exactly locally and match the retained
hosted captures before acceptance. Current head 8213e810 otherwise passes all
16 compiler legs and eight active regression jobs; runtime and lifetimes remain.

## #161 latest checks and supplemental street capture

At 8213e810, build 35629405112 passes all 16 compiler legs. Regression 35629405076
has passed GCC unit, format, sanitizers and both cross variants; remaining jobs
are running. Only this head's complete required checks count for merge.

A supplemental real-GPU capture reuses the accepted sketch_reference street and
all nine reference effects with TAA/post/decal/soft-particle controls enabled.
Reviewed captures and report are retained in fidelity-street-visual/ and its
adjacent script/log. It reports zero effect/light/upload/decal/temporal drops.
This is additional visual evidence, not a replacement for exact software
references or previous hardware performance measurements; no fixtures changed.

## #161 all compiler legs pass; retain hosted presentation evidence

At b2f545c7, build 35628975625 passes all 16 compiler legs. Publication jobs are
conditional/non-required and skip on PRs. Regression format, sanitizer and both
cross jobs pass; longer jobs remain running. The workflow now uploads owned
presentation PNGs, JSON reports and logs (no paks/content archives) even on failure,
so hosted visual results are reviewable. Reference creation is explicitly rejected
in CI, in addition to refusing existing-file replacement. This final gate/evidence
checkpoint will trigger all checks on its own head; prior results remain evidence,
not a substitute for final-head success.

## #161 self-review and current hosted head

AGENTS self-review: changes match #161 presentation scope; authoritative movement,
collision, snapshot and damage arithmetic are unchanged. The separate descriptor
bug is already on main through PR168. No new OS calls escape platform/filesystem
interfaces; frame state is bounded/POD and no engine heap allocation was added to
frame evaluation. Cooked layout assertions and new native ABI checks are retained.
Effects are included in the passing lifetime scan. Existing goldens are unchanged:
only four new fidelity PNGs are added. Existing generated shader data is unchanged:
4589 appended lines, zero removed. Known-good object 8bc8c94c still targets 81a0f9dc.
Issue comments record measurements, failed attempts, limitations and decisions.

Current head b2f545c7 runs build 35628975625 and regression 35628975619. Superseded
intermediate regression 35628745426 was canceled to free runner slots; its build
35628745374 had already completed with the corrected-next-head MSVC diagnostics.
Initial runtime 35628249523 is retained for earlier feedback on presentation tests.
Only final-head checks count for merge. Main must be rechecked immediately before
ready/merge; all 16 compiler legs and ten active regression jobs must succeed.

## #161 second hosted MSVC diagnostics

After the decal name correction, MSVC reaches the new motion submission code
and reports C4459 (local `uniform` shadows a legacy global) and C4244 (integer
conditional values assigned to a float flag). Rename the local `motionUniform`
and spell the exact zero/one flag literals as floats. No expression ordering or
simulation behavior changes. Evidence: fidelity-ci-msvc-arm64-second.log from
build 35628745374 at ba86c89a. All required checks will rerun on the corrected head.

Expanded lifetime analysis now passes all 1248 commands / 135 source paths,
shipping/development and static/module with positive/seven-object controls.

## #161 MSVC shadow diagnostic

Build 35628249560 fails MSVC C4457 because the new decal half-size loop named its
local `size`, hiding DCL_Open's byte-count parameter. The loop variable is renamed
to `extent`; validation/arithmetic are unchanged and warning policy is retained.
This is a correction to unmerged #161 code. Log: fidelity-ci-msvc.log.

## #161 first hosted unit failures and local gates

PR169 regression 35628249523 fails both unit legs in the cook publication probe:
GCC/Clang retain references to new streaming RHI functions that the mock did not
define. Logs: fidelity-ci-unit-gcc.log / fidelity-ci-unit-clang.log. The probe now
supplies abort-on-call implementations so unchanged publications still prove zero
GPU access. No engine behavior changes. Full cooker reruns now pass under GCC and Clang/libc++
(fidelity-final-cook-gcc.log and fidelity-final-cook-clang.log).

Local tidy passes all 1306 configurations. Final legacy OpenArena replay matches
accepted hash 17a172f7 (fidelity-final-demo.log). RHI/render-graph, temporal,
residency-policy and affected-contract controls pass; all 101 shader binaries and
interfaces reproduce. Known-bug harness controls pass. Expanded lifetime scan is
still running. Hosted runs at 31049488 are evidence only until all failures are
resolved and the final head is green.

## #161 draft PR169 and final gate status

Draft PR169 is open against main at 31049488. All writes stay in this repository.
RHI checks pass, and all 101 shader binaries/interfaces match package d533f8bf;
no accepted array changed. The expanded 1248-command lifetime scan and tidy gates
are running. Hosted compiler/regression checks must pass before ready/merge;
recheck main immediately before acceptance. No red or skipped required job is
acceptable. The new software references pass exact comparison.

## #161 final gate integration review

Self-review found the new effects directory missing from the lifetime scanner's
explicit core list; it is now included and the expanded 1248-command scan is
running. Boundary/type/tidy scanners already discover owned sources. Subsystem
ownership and the concise current AGENTS verification section now name the new
presentation/streaming controls. Residency also participates in affected-test
selection. Final software combined capture comparison passes with timing and
hardware-command additions (fidelity-final-software.log). Main remains 07304b32.
Next: finish final local gates and start the exact-head hosted PR checks.

## #161 combined hardware budget pass

The permanent `tests/fidelity_runtime.py --measure-gpu` passes serially on RTX
3080 Ti / 595.91.07 at 2560x1440 offscreen, 640x360 present. After 4096 warm frames
and 64 effect warm frames, all 400 consecutive samples pass unchanged budgets.
CPU p95 ms: effects 0.140, decals 0.011, soft/decal backend 0.101, LOD 0.002.
GPU p95 ms: inclusive effects 0.784384, nested decals 0.446464, post 0.314368,
post copy 0.143360, camera motion 0.144384, object motion 0.090112, resolve
0.400384, temporal copy 0.144384. Final frame CPU p50/p95/p99 is
4.154/5.475/8.746 ms. The workload repeats all nine effects and three decals every
20 frames, ending with 478 particles / 26 instances / 72 decals, 1215 collisions,
and zero pool/light/upload/decal drops. Hunk and zone-tag memory remain identical
to baseline. The 1440p capture was reviewed. Evidence: fidelity-combined-hardware/
gpu-report.json, gpu-engine.log, hardware.png and fidelity-combined-hardware.log.
The earlier street-scene post-copy miss remains; the post path stays default off.
Optional upscaling is not required for this issue and is deferred; no new upscaler
or dependency is introduced. Remaining: exact-head/current-main gates and review.

## #161 presentation timing implementation

The failing timing assertion now passes with the four software references still
pixel-identical (fidelity-timing-after.log). Renderer API 31 development / 25
shipping exposes cumulative microseconds for effect preparation, decal preparation,
combined soft-particle/decal backend submission and model LOD selection. Agent
`profile.presentationCpuUsec` and the profiler show these totals; hardware sampling
differences consecutive totals. The existing no-wait GPU query pool also records
a nested `decals` scope. No allocations or authoritative simulation edits were
introduced. Native LOD timing uses a deterministic clock control and passes with
six measured calls (fidelity-presentation-lod.log). Client build, format/types/
boundaries and the native ABI gate pass. Next: serial combined hardware workload.

## #161 combined visual scene and remaining declared budgets

The new `tests/fidelity_runtime.py` combines the generated two_lane level, nine
unchanged reference effects, three projected decals, an owned PBR sphere, TAA and
filmic post. Two independent initial runs were byte-identical. New baseline,
combined, settling and reduced-LOD frames were reviewed; existing goldens and
assets are untouched. The sphere's authored `lod_error=.1` permits 576/288/144
triangles (75% savings at the lowest level); the default .01 held both requested
reductions at 470, so only this new test recipe opts into the larger error.

Remaining budgets declared on #161 before measurement at 1440p / RTX 3080 Ti:
effects frontend CPU p95 0.50 ms; combined soft-particle/decal backend CPU 0.50 ms;
decal frontend CPU 0.25 ms; LOD selection CPU 0.10 ms; inclusive effects GPU pass
1.50 ms, decal GPU subset 0.75 ms. Nested decal time is not counted twice. Existing
post/temporal/streaming thresholds remain unchanged. The fresh comparison run passes all four references exactly
(fidelity-combined-verify.log); the runtime command is added to hosted CI and
AGENTS.md. The new timing assertion fails first with missing `presentationCpuUsec`
(fidelity-timing-before.log). Next: implement cumulative CPU and nested decal GPU
timing, then measure the combined workload.

## #161 permanent streaming measurement command

`tests/streaming_runtime.py --measure-gpu` now reproduces the serial reference
hardware gate using the same generated assets and runtime setup as software CI.
It requires an explicit Vulkan driver and verifies RTX 3080 Ti in the engine log,
retains every-frame profiles and completed-upload samples before budget assertions,
and then runs the ordinary fly-through/reload/restart controls. The permanent
command passes: active CPU p95 0.081 ms (244 samples), GPU p95 0.327328 ms
(156 samples), against the unchanged 0.25/0.50 ms limits. Evidence:
fidelity-stream-permanent-hardware.log and fidelity-streaming-runtime/gpu-report.json
plus gpu-engine.log. Software CI does not claim hardware performance. No golden
or authored asset changed. Next: combined scene and per-system acceptance.

## #161 one-MiB streaming hardware budget pass

With the fixed staging/submission bound reduced to 1 MiB, the identical serial
RTX workload passes the unchanged budgets. Active CPU p50/p95/p99 is
0.044/0.117/0.181 ms (244 samples, max 0.196); upload GPU p50/p95/p99 is
0.323216/0.325984/0.330272 ms (156 individually observed samples, max 0.338208).
Final frame CPU p50/p95/p99 is 3.080/6.452/8.088 ms. Hardware, resolution, source
set, budgets, 4096 warm frames and four cold/visible cycles are unchanged.
Evidence: fidelity-hardware-streaming-1m/report.json, capture and .log. The failed
4 MiB report is retained separately. The native same-byte/row/timestamp test now
passes with 22 submissions per 4K BC7 chain; the client rebuild passes. Smaller
batches trade a longer promotion for bounded frame work. The software hot-reload/
restart gate passes at this final bound (fidelity-streaming-runtime-1m.log), as do
GCC/Clang native probes and formatting. The hardware baseline frame CPU was
3.044/3.387/5.853 ms p50/p95/p99; the measured transition workload ends at
3.080/6.452/8.088 ms. Combined-scene acceptance and exact-head/current-main gates
are still outstanding.

## #161 smaller transfer test-first

The native probe now requires 1 MiB fixed staging and 22 submissions for the same
4K BC7 chain, preserving every mip row/byte and every GPU timing sample. It fails
against the 4 MiB implementation at the staging-size assertion
(fidelity-stream-1m-before.log). This changes the throughput/latency tradeoff to
meet the existing measured budgets; the budget thresholds stay unchanged.

## #161 first streaming hardware measurement: budget miss

Nonblocking GPU timestamp reporting passes native GCC/Clang probes and real RTX
execution. The renderer owns two upload queries and reads them only after the
existing transfer fence, without a query wait. Profiler/ImGui expose cumulative
submission bytes/count and uniquely numbered GPU samples. Renderer API is now
30 development / 24 shipping. The frontend lifetime scan passed all 1244 commands
(shipping/development, static/module, positive/seven-object negative controls).

Serial RTX 3080 Ti / 595.91.07 measurement at 2560x1440 offscreen / 640x360 present,
4096 warm frames and four cold/visible cycles samples every frame. Five owned 4K
BC7 textures (~107 MiB) use 32 MiB residency / 128 MiB source. All 64 GPU submission
samples are observed. With 4 MiB staging/batches, active CPU p50/p95/p99 is
0.0135/0.291/0.348 ms (152 samples, max 0.485); GPU p50/p95/p99 is
0.432784/1.286912/1.296640 ms (max 1.298080). Final frame CPU p50/p95/p99 is
3.058/4.632/7.474 ms. Both declared 0.25 ms CPU and 0.50 ms GPU p95 budgets fail.
The script fails its assertion after writing the full report; this is not accepted
performance. Evidence stays in fidelity-hardware-streaming/report.json and .log.
Next: reduce the fixed batch to 1 MiB and rerun the identical serial workload;
do not change budgets or erase this result. Defaults remain off.

## #161 upload timing test-first

The native upload probe now requires per-submission GPU timestamps and cumulative
submitted-byte/submission/sample counters. Readback must happen after the existing
transfer fence without a query wait; an eight-bit counter-wrap case requires 11
ticks at 2 ns to report 0.022 microseconds. Initial compilation fails on the absent
upload-statistics API (fidelity-stream-timing-before.log). The frontend component's
lifetime scan is still running against unchanged engine sources before this timing
implementation begins.

## #161 frontend residency integration under test

The first software larger-than-budget run passes: five original 4K textures,
111848240 compressed source bytes, 33554432-byte residency pool, peak occupied
regions 32506880 bytes, 13 promotions/7 demotions, no deferred requests/failures.
Cold demand falls to 28160 resident bytes; rewarming and vid_restart restore
quality (fidelity-streaming-runtime-after.log/report.json). The arena reserves its
configured device budget; used/peak report occupied regions, not additional pools.

The initial integration run rejected all five textures at the intentional legacy
2048-pixel resampler/chunk cap. Streamed cooked mip chains now use the separate
physical compressed-image limit (up to 16384); the legacy path retains its cap.
CPU source storage is allocated once at renderer initialization, async work runs
only after frame submission, and profiler/ImGui counters expose residency/source
usage and transitions. Renderer API is 29 development / 23 shipping. Source slots
are reused for same-size or smaller reloads; growth consumes arena space until
renderer restart. Hot reload passes: a 4K floor is replaced by a differently colored
512-square texture, the source slot is reused, and the captures visibly change
(fidelity-streaming-runtime-reload.log). Final pre-restart counters are 17 promotions,
7 demotions, one reload, no failures/deferrals. Native GCC/Clang RHI, native ABI,
format, types, boundaries and workflow syntax pass. Disabled OpenArena replay is
still 17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(fidelity-stream-legacy-demo.log). The full lifetime scan remains in progress.
Real GPU transfer/CPU budget measurements remain outstanding. No fixture regenerated.

## #161 streaming runtime test-first and source-storage decision

`tests/streaming_runtime.py` cooks five original solid-color 4096-square BC7
textures (about 107 MiB including mips), overlays them on the owned generated
two_lane level and requests a 32 MiB residency budget with 128 MiB source storage.
The initial run reaches the map then fails on missing textureStreaming profiler
counters (fidelity-streaming-runtime-before.log). It requires promotion, coarse
tail eviction/restoration and vid_restart while peak residency stays bounded.
Generated artifacts live only in the private cache; accepted fixtures are unchanged.

Use a preallocated, explicitly bounded compressed-source arena to keep filesystem
I/O out of frames. This first residency implementation streams from that arena to
VRAM; it is not an unbounded CPU cache or background disk loader. Exceeding source
capacity must report an explicit error rather than silently bypassing the VRAM
budget. A future out-of-core disk path is needed only for content exceeding the
configured source budget. Runtime policy/transfer CPU budget is 0.25 ms p95 and
upload GPU budget 0.50 ms p95, declared before hardware measurement at 1440p.
Defaults remain off pending the full gate.

## #161 reclaimable residency component

The RHI owns a fixed-capacity residency arena: one device-memory allocation,
2049 image records (2048 resident plus one transition), a reusable descriptor
pool and one retirement fence. Allocations use actual Vulkan requirements and
aligned free gaps; pending/retired images remain charged. Each replacement gets
its own descriptor. Adoption between submitted frames inserts a last-use fence;
zero-timeout polling releases the old image only after completion. Shutdown
cancels uploads before freeing residency. Driver image/view objects are still
created for replacements; no engine heap or new device-memory allocation occurs
per promotion. Fragmentation may refuse a request; it cannot overrun the budget.

GCC/Clang native probes, upload ASan/UBSan, client build, formatting, types and
boundaries pass (fidelity-residency-{after,clang,sanitized,build,format,types,boundaries}).
Tests require full-pool refusal while retirement is pending, byte accounting,
distinct descriptors, no adoption of unfinished uploads, exact reuse after the
fence, rollback of a failed view and full cleanup. The generic texture creation
and staging helpers are shared; no accepted shader/fixture changed. This remains
unconnected to image loading/binding; bounded source storage, hot reload and
large-set fly-through/real GPU acceptance are next.

## #161 residency ownership test-first

The native RHI probe now requires a reusable device-memory budget with actual
Vulkan requirements, independent replacement descriptors and a retirement fence.
A full pool must refuse new images until old use completes, then reuse the freed
space without another device-memory allocation. Failed creation must release its
partial image, and shutdown must reclaim every handle. Initial compilation fails
on the absent residency API (fidelity-residency-before.log). Renderer/source-cache
integration and hardware/software fly-through acceptance still remain.

## #161 asynchronous upload component

The RHI now has explicit map-load initialization, queue, nonblocking poll and
shutdown calls. It reuses staging allocation and BC validation helpers; one fixed
4 MiB buffer, command buffer and fence stream complete rows through at most sixteen
regions per submission. Source storage is borrowed until final-fence completion.
No GPU idle wait or resource allocation occurs in queue/poll. Map release/shutdown
cleans the resources. Descriptor adoption and residency integration are still next.

GCC/Clang RHI probes pass, including a 4K chain in six submissions, exact destination
mip/row and source bytes, non-block-aligned BC4 dimensions, busy refusal, final-fence
completion and device loss (fidelity-stream-upload-{after,clang}.log). The first
runtime probe needed its temporary Vulkan observers restored before the older
replacement test; production behavior was unchanged. The native upload probe also
passes ASan/UBSan; running the entire RHI driver with those extra flags reached an
unrelated image-probe linker failure because ASan retains its unused loader table.
Only the upload binary is claimed as sanitized acceptance. Client build, graph,
format, types and boundaries pass. Shader package/accepted fixtures stay unchanged.

## #161 asynchronous upload test-first

The existing native RHI upload probe now requires a preallocated 4 MiB staging
buffer and one reusable command/fence. A 4096-square BC7 mip chain must transfer
in six bounded submissions, preserve every source byte, poll fences with timeout
zero, and publish completion only after the final fence. A busy uploader refuses
a second request; malformed sizes and device loss cannot report success. Queue
and device idle waits are rejected by the probe during uploads. Initial compile
fails on the absent asynchronous API (fidelity-stream-upload-before.log).

## #161 bounded residency policy component

The pure planner now handles at most 2048 records with stack-only scratch and no
allocation. It reserves replacement-tail headroom, counts pending/retired bytes,
keeps coarse tails, prioritizes recent bindings, retains quality on ties and
serializes transitions. A downgrade can temporarily use a smaller tail when its
preferred replacement would exceed peak budget. GCC/Clang UBSan pass, including
settling repeated decisions and rejecting malformed/overflowing costs
(fidelity-stream-policy-{after,clang}.log); formatting passes. The compiler matrix
runs this test. The policy is not wired into rendering yet; native fence-safe
uploads, bounded source storage and the larger-than-VRAM runtime set remain next.

## #161 streaming policy test-first

The existing synchronous replacement is unsuitable for streaming: it waits idle
and updates a live descriptor. Reuse its format/sampler helpers, but require
separate image ownership and fence-safe adoption/retirement for the new path.
The first policy test fails on absent tr_stream.h (fidelity-stream-policy-before.log).
It requires fixed-capacity recent-use residency, retained coarse tails, eviction
before promotion, stable equal-priority views, frame-counter wrap and peak VRAM
accounting including pending/retired allocations. Costs are device allocation
requirements, not compressed file sizes. Serial transitions deliberately bound
staging and retirement. This is not an async-upload implementation or acceptance.

## #161 first temporal GPU budgets and disocclusion pixels

At 1440p on the RTX 3080 Ti, serial real-clock timing controls with an owned native
view weapon pass their declared p95 budgets. After 4096 warm frames and 50 profile
samples: camera motion 0.128000 ms, geometry motion 0.030720 ms, resolve 0.370688 ms,
copy 0.128000 ms; CPU p50/p95/p99 4.377/4.986/8.751 ms. A continuously turning
camera with authored motion_blur 1 measures 0.101376/0.023552/0.288768/0.285696 ms
for the same GPU passes; CPU 4.333/5.083/8.820 ms. Captures were inspected: scene
blur follows the turn while the view weapon and HUD remain sharp. Reports/logs:
fidelity-hardware-temporal and fidelity-hardware-temporal-blur. These controls
do not replace final combined PBR/effects/decals scene measurement, and the earlier
post-copy budget miss remains recorded rather than erased by this cheaper view.

A stronger software test removes the owned body without a camera cut, then
compares newly revealed pixels to the settled background: 14174 covered pixels,
mean max-channel difference 3.2753 codes and p95 8 (limits 5/16). The test passes
(fidelity-motion-disocclusion/models.json). No accepted golden was regenerated.

Test-first d5678f1a adds reactive classification for animated images, video,
waveforms, scrolling/deforming materials and remapped shaders. GCC/Clang UBSan
checks pass (fidelity-reactive-{after,clang}.log). The classifier is used both to
select world geometry for the reactive overlay and to reject tracked-model
history. Constant UV scales remain eligible. The rebuilt runtime/pixel gate passes
(fidelity-reactive-runtime): the same 14174 pixels, mean 3.2753/p95 8, with zero
dropped frames or overflow. Final combined quality/reference checks remain outstanding.

## #161 native geometry motion and visible counters

The motion traversal records submitted entity identities once, draws current
geometry against scene depth and pairs native IQM vertices with copied prior
skin positions. Rigid unchanged local geometry uses prior transforms. Unknown
identities, changed legacy MD3/MDR animation, deforms and translucent inputs
conservatively reject history; this is deliberate until those inputs have
reliable prior vertices. Alpha-mask setup is shared with shadow rendering, and
motion pipelines preserve culling and polygon-offset variants. No simulation
arithmetic or per-frame allocation was added.

Test-first 944a20ec requires rejection after geometry/uniform exhaustion; the
implementation restores the main pass without rotating/committing history.
GCC/Clang graph tests pass (fidelity-motion-overflow-{after,clang}.log); the test
initially needed its two-case loop changed to avoid an undeclared initializer-list
header. GCC/Clang skin probes verify the motion vertex stream retains old values
when the current pose changes (fidelity-motion-skin{,-clang}.log). The owned
rifle/body gameplay run passes with --taa, including replicated hit-box equality
(fidelity-motion-animation-final.log); ADS/body captures were inspected.

The runtime --models gate now observes actual profiler counters through ADS,
fire/reload and third-person movement. Final report: 130 temporal frames, 1994
motion draws, 680 reactive draws, one stored/matched entity, no rejected records,
no overflow and no dropped temporal frame (fidelity-motion-models-runtime).
An initial harness attempt used cvar.set before the game registered its settings;
using the existing console set command before map load fixed the test setup.
Renderer API is 28 development / 22 shipping for the expanded statistics record.

Client builds, shadow-view tests, native ABI, format, boundaries and types pass.
Disabled-path OpenArena replay remains exactly
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(fidelity-motion-legacy-demo.log). No accepted fixture changed. Stronger pixel
checks for moving silhouettes/disocclusion, reactive animated materials, final
reference goldens and GPU budgets remain required before accepting TAA.

Declared temporal pass budgets before hardware measurement at 1440p:
camera motion 0.30 ms, geometry motion 0.50 ms, resolve 0.80 ms, display copy
0.30 ms (0.80 ms with optional motion blur). Over-budget paths stay default off.

## #161 geometry-motion pipeline component

Test-first 07cb7ec8 fails on the missing motion pipeline/modules. The new pipeline
uses the retained scene depth with reverse-depth comparison and no depth writes,
plus current position/color/UV and a separate previous-position vertex stream.
Its shaders preserve alpha-test discard and encode prior clip position with the
weapon viewport depth scale/bias. Untracked or rejected geometry writes invalid
history instead of borrowing camera-only motion. Surface submission is next;
this pipeline component does not yet draw moving objects.

GCC/Clang native graph tests pass (fidelity-motion-pipeline-{after,clang}.log),
and the client builds. Two new shader arrays are appended via bin2hex; all prior
arrays remain unchanged. Fresh/cached compilation matches all 101 shaders
(package d533f8bf7dfe1d7aef4c5b3d319c928bfe52357d1541c4e9e292abcc562c0335;
fidelity-motion-shader-check.log). Next wire the geometry traversal and previous
skin/rigid positions, preserving masks and conservative rejection for unsupported
animated/deformed inputs, then test actual moving/skinned objects and disocclusion.

## #161 camera-view temporal integration

The opt-in r_taa path now jitters only the backend presentation projection,
records camera history around a rendered main view, dispatches motion/resolve
before effects, and consumes authored motion_blur in the display copy. It forces
single-sample rendering while enabled; disabling it restores the saved MSAA
configuration. Uniform reservation covers the actual 192-byte camera record
without inflating ordinary draws to the shadow path's 1024-byte range.

The first private smoke omitted required r_fbo 1 and correctly failed its missing
pass assertion; adding the prerequisite made camera-only captures/profiling pass
(fidelity-temporal-camera-smoke.log). Captures were inspected. The permanent
camera-only test passes motion, large camera/FOV cuts, restart and explicit
disable on an owned generated map (fidelity-temporal-view-runtime-final.log).
It is wired into runtime CI. Client build, format, boundaries, types and actionlint
pass. Fixed OpenArena replay still matches accepted hash
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(fidelity-temporal-legacy-demo.log); no accepted fixture was regenerated.

Moving/skinned entity motion is NOT wired yet: the runtime test deliberately
hides entities to isolate camera reconstruction. Next add the geometry overlay
using copied previous skin positions and entity transforms, preserve alpha/depth
masks, test moving models and disocclusion, then measure all new GPU passes.
No final TAA quality or combined-scene acceptance is claimed.

## #161 temporal fullscreen dispatch component

Test-first e46a0ec8 fails on missing temporal uniform/dispatch declarations.
The RHI component initializes camera motion from depth, retains a geometry-motion
pass, resolves against distinct previous history and copies the new history to
scene color. It alternates physical histories only after dispatch, rejects a
failed uniform upload and restores initialized descriptor slots/scissor state.
All temporal dependencies cover the full image, because reprojection/blur read
neighboring pixels. Optional nine-tap motion blur affects only the display copy,
not the sharp history; disoccluded pixels reject prior depth and neighborhood
clamping bounds accumulated color. First-frame/cut paths skip history fetches.

GCC/Clang graph probes pass (fidelity-temporal-commands-{after,clang}.log), including
three frame rotations, first-frame invalidation, sparse descriptor restoration
and upload exhaustion. The initial run needed the probe's pass observer extended
to recognize new passes; production dispatch did not bypass that assertion.
Client build and format pass. Three appended shader arrays were generated with
bin2hex; every prior array is byte-identical. All 99 fresh/cached shaders match
(package 84f4d10d2f9b35d40cb4171e9a95fbd34c76b9ae5d74f441a2e05dccb6220b9f;
fidelity-temporal-shader-check.log). This component is not enabled: frontend
view lifecycle, moving/skinned geometry, visual tests and budgets remain next.
Main 07304b32 build/publication 35613793896 passed; regression 35613793817 remains
in its long level/bot runtime step. No maintainer dependency exists.

## #161 jitter and restored software controls

Test-first 06b43fd4 requires an eight-frame centered, subpixel projection jitter
sequence which changes only projection entries 8/9 and preserves invalid-size
inputs. GCC/Clang UBSan temporal probes pass (fidelity-jitter-{after,clang}.log).
This presentation helper is not enabled until motion/resolve wiring is complete;
authoritative camera/simulation state and culling stay untouched.

After descriptor adoption, software 4x MSAA post controls (exposure, LUT, sharpen,
vignette, grain, depth blur, reload/restart) and projected decal/editor/HUD controls
pass (fidelity-restored-{post,decals}-runtime.log). No accepted fixture changed.

## #161 restored RTX post measurements

The descriptor correction removes the RTX crash: 4096 warm frames plus 50 profile
samples complete on the RTX 3080 Ti at 2560x1440 offscreen / 640x360 presentation.
The owned street capture was reviewed; these are timing controls, not the final
combined PBR/effects acceptance scene. Baseline post p50/p95 is 0.137216/0.514048 ms;
copy is 0.108544/0.547840 ms. CPU p50/p95/p99 is 3.613/5.391/8.500 ms.
With LUT/sharpen/vignette/grain and nine-tap depth blur enabled, post is
0.510976/0.630784 ms, copy 0.115712/0.625664 ms; CPU 3.686/5.484/9.149 ms.
Evidence: fidelity-hardware-post-restored and fidelity-hardware-post-all report.json,
captures and logs. Runs are serial and use real platform/GPU clocks.

Filmic p95 passes its declared 0.75 ms budget; copy p95 exceeds its 0.20 ms budget.
Therefore r_postProcess stays default off. No budget was increased to pass the
measurement. Final combined-scene measurements remain required; TAA, blur using
motion vectors and streaming are not implemented by this timing checkpoint.

## #161 descriptor restoration adoption

Main's accepted helper now restores cached descriptors after the new effects and
post passes. Test-first d35a53ed fails the sparse restoration mask in
particleCommands (fidelity-restore-before.log/fidelity-restore-assert.log).
The correction replaces the device-limit dirty range with the shared bounded
helper, retaining scissor invalidation. GCC/Clang graph tests and client build
pass (fidelity-restore-{after,clang,build}.log); format passes. Tests cover live
slots 0/1/3, skipped null slots and saved dynamic offset on a 32-set-capable device.
RTX post timing and software controls are next. PR168's merged-tree runs are
build/publication 35613793896 and regression 35613793817, still in progress.

## #161 temporal graph resources

Test-first c13573bd/4a0ecc1e require distinct persistent history images and four
ordered motion/resolve/copy passes. The implementation allocates bounded RGBA16F
motion/history attachments through the existing graph pool, creates both history
write framebuffers, and releases all images/views/passes on restart. Temporal
configuration requires the post path and single-sample rendering. No runtime
switch, jitter, motion shader or resolve dispatch is enabled at this checkpoint.

GCC and Clang native graph probes pass 36 legacy, 32 post and 16 temporal
configurations, including both physical history framebuffer views and dependency
checks (fidelity-temporal-graph{,-clang}.log). The first implementation run exposed
a stale test pass-count assertion and a descriptor assertion before descriptor
initialization; correcting those assertions produced passing checks. Client build,
format (475 files), boundaries and fixed-width checks also pass
(fidelity-temporal-graph-{build,format,boundaries,types}.log). Accepted fixtures
remain unchanged. PR168 still awaits its last runtime job before merge.

## #161 previous-pose vertex component

Test-first 022fb33b fails on missing R_IQMPreviousPositions
(fidelity-temporal-skin-before.log). The implementation reuses the current IQM
influence-matrix arithmetic for both ordinary drawing and saved history; no
expression order is changed. Previous local positions come from the copied skin
pose, or the saved frame/oldframe/backlerp for legacy IQM clips, before the
separate object/world transform. Output is bounded by the supplied capacity.

GCC/Clang UBSan probes pass analytical two-bone blended positions, float/byte
weights, static vertices and legacy clip interpolation, and match the ordinary
renderer vertex bytes exactly (fidelity-temporal-skin{,-clang}.log). Actual fixed
OpenArena animation replay reports are identical between accepted #164's binary
and the rebuilt feature binary: all 253 authoritative box hashes and sampled
frames 050/100/200 agree (fidelity-temporal-demo-{before,after}.log/replay.json).
No fixture or shader array is regenerated. This still needs motion-pass wiring;
no temporal anti-aliasing or hardware budget is claimed yet.

## #161 explicit temporal submission identities

Test-first ecc74e8d extends the existing animation-render probe; it fails on the
absent submission API/identity field (fidelity-temporal-submit-before.log).
The new wrapper reuses existing material/pose validation and assigns identity
only after successful submission. Ordinary submissions clear it; failed graph
bindings cannot leak an ID to the next entity. Native body and view-weapon calls
now supply separate stable owner/part IDs including the teleport toggle.
The legacy refEntity and game-state layouts are unchanged. Renderer API is now
27 development / 21 shipping for the additional submission function.

GCC/Clang animation/render probes, native owned body/rifle gameplay and replicated
hit-box runtime, C/game/engine ABI, format and boundary gates pass
(fidelity-temporal-submit{,-clang,-runtime,-abi,-format,-boundaries}.log).
This only carries identity; GPU motion and TAA remain unimplemented. Legacy
models/brush movers still need IDs, and the history cache is not yet connected
to the rendered-view lifecycle. PR168 continues its expected long runtime gate.

## #161 temporal history component implemented

The pure renderer cache now keeps two bounded views and up to 256 entity/skin
records each, statically under 4 MiB with trivial storage and no allocation.
GCC/Clang UBSan probes pass copied poses, consecutive rendered frames (including
unsigned wrap), sparse/missing entities, model/hash changes, teleport/FOV/axis
cuts, viewport changes, failed resolves, invalid numeric input and overflow
(fidelity-temporal-{after,clang}.log). It does not use oldorigin/oldframe as
previous-rendered state. Existing legacy records and authoritative code are
unchanged. CMake, affected selection and unit CI include the new component.

This is not wired into rendering yet. Next independent #161 work: submit stable
presentation identities, capture real previous posed vertices/view matrices and
add motion/jitter/TAA passes with visual controls. PR168 still waits for hosted
runtime; adopt its accepted descriptor correction and remeasure RTX post before
claiming hardware acceptance. All current work remains in msetaro/aftershock.

## #161 temporal history test-first contract

PR168's last runtime gate is still running; the previous accepted runtime took
57 minutes, including 37 minutes in its authored-level/bot step. Independent
#161 work continues while the isolated #31 fix awaits acceptance. The new
`tests/temporal.py`/native probe specify bounded previous-rendered-view/entity
history: copied skin poses, stable identity/model/hash checks, camera and object
teleport rejection, resize/FOV/cut invalidation, missing/failed-frame rejection,
capacity and unsigned frame wrap. The pre-implementation compiler fails on the
absent temporal types/functions (fidelity-temporal-before.log). This is not TAA
or motion-vector acceptance: actual submission identities, camera jitter,
animated previous vertices, GPU resolve and visual/performance gates remain.

The strengthened native decal lifecycle also verifies visible notification text
after a small projected decal; it passes with the scissor correction
(fidelity-scissor-hud.log). No accepted fixture changed.

## #161 effects scissor restoration

While #31 PR168 runs required gates, a separate test on the unmerged #161 code
reproduces stale scissor state (70f14376, fidelity-scissor-before.log and
fidelity-scissor-assert.log). Direct effects/post calls change the GPU scissor
without changing its ordinary draw cache. After a small decal rectangle, the
next full-screen draw sees its old cached rectangle and skips the required GPU
update. RHI_EndEffects now invalidates that cache alongside depth range, so the
next ordinary draw installs its actual rectangle. This correction is confined
to new #161 code; main's #31 descriptor fix remains isolated in PR168.
Native GCC/Clang graph checks pass (fidelity-scissor-{after,clang}.log), as do
the decal editor/reload/restart lifecycle and four-sample MSAA post/HUD checks
(fidelity-scissor-{decals,post}.log). Format passes all 472 owned files. No
accepted fixtures change. #31 PR168 now has every required check except runtime
green; its descriptor correction is still awaiting merge.

## #161 RTX failure isolated; #31 fix in progress

Initial post-enabled RTX run crashes in RHI_PrepareDraw before producing timing
(fidelity-hardware-post.log, fidelity-post-gdb.log). The identical post-disabled
control passes: CPU p50/p95/p99 4.688/6.207/6.739 ms, GPU main p50/p95
1.099824/1.137120 ms and gamma 0.275456/0.278528 ms. These are control results,
not acceptance of the new post path.

Descriptor restoration uses the device's supported-set count as an index into
the engine's five-entry cache. Main's existing SSAO path has the same defect;
a separate issue/31-descriptor-restore branch at main 81e40b0c now reproduces it
with a 32-set device in the native graph probe. #31 reopened; comment 5761425851.
The new effects/post restoration will adopt the same correction after the #31
fix passes its required gates and merges. Do not accept #161 timing before that.

## #161 filmic component gates pass

Lifetimes pass all 1236 commands / 132 source paths across shipping/development
and static/module configurations, including positive/seven-object negative
controls (fidelity-post-lifetimes.log). The strengthened MSAA test also passes
HUD preservation, zero upload drops and stable memory
(fidelity-post-hud-msaa-final.log). Its near-opaque antialiased edge comparison
allows two 8-bit code values: one MSAA edge retains a small background fraction;
fully covered text remains white while the whole scene becomes orange.

GCC/Clang final graph pipeline/upload checks, all 96 shader regeneration checks,
472-file formatting, tracked boundaries, types, native ABI, authored schemas,
agent protocol, isolation and workflow syntax pass. Renderer ABI is now 26 in
development / 20 in shipping for the added post counters. Main was refreshed and
remains 81e40b0c. Initial RTX post timing is running; no PR or default switch yet.
Self-review: new presentation paths only, bounded uploads/fixed graph targets,
no authoritative FP, per-frame allocation, OS access or accepted-fixture edits.

## #161 post preservation checks pass

The strengthened single-sample native check passes profile draw/drop/load
counters, stable hunk/tag memory and unchanged fully covered notification text
under a constant orange LUT (fidelity-post-hud-final.log). White is 254 in this
legacy vertex-modulated HUD; antialiased edge pixels intentionally depend on the
background. The OpenArena fixed replay still matches
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(fidelity-post-legacy-demo.log), and soft depth/MSAA/SSAO still pass
(fidelity-post-soft-depth.log). The graph probe also checks both new pipeline
blend/sample states, descriptor restoration and bounded upload exhaustion
(fidelity-post-final-graph.log). ABI, agent protocol, boundaries/types, workflow
syntax and isolation pass. Lifetimes and strengthened MSAA/HUD are running.

## #161 initial native filmic/lens controls pass

Three new shader arrays were appended through bin2hex; all previous 93 remain
byte-identical. Fresh pinned compilation reproduces all 96 shaders
(fidelity-post-shader-check.log). Post processes each world view after effects
and before HUD, using one fixed float target plus copy-back; no per-frame heap.
The existing watcher selects finite validated profiles and a 16-slice display
LUT. The initial test correctly rejected its accidentally sRGB-cooked LUT;
setting the authored recipe to srgb:false fixes the setup, with no loader bypass.

Single and 4x-MSAA native tests pass exposure, LUT, sharpening, vignette, grain,
depth blur, watched edits, exact restoration and restart
(fidelity-post-native-{linear-lut,msaa}.log). Captures reviewed: bright exposure,
constant orange grading, defocused floor. New profiler draw/drop/load counters
are built; strengthened HUD/memory controls and legacy replay/lifetime gates are
running. The HUD check initially included partially transparent glyph-edge pixels;
those correctly blend with the changed scene. It now selects fully opaque white
pixels. No engine workaround. Temporal vectors/TAA/motion blur and streaming are
still absent; these are component passes, not final #161 acceptance.

## #161 native post resources pass; image contract first

The graph/native backend now create the separate post target and two passes;
32 combinations cover MSAA, retained depth/stencil, bloom, capture and shadows.
GCC/Clang graph checks pass and the disabled descriptor oracle remains unchanged
(fidelity-post-graph-{native,clang}.log). The development client builds. There
are no post draw calls/shaders yet. The native image test first fails because
editing exposure leaves the frame unchanged (fidelity-post-native-before.log).
It next requires LUT, sharpen/vignette/grain/depth blur, reload and restart.
Implement shaders and per-world-view dispatch before HUD, then run both sample
modes; proper temporal vectors/TAA and asynchronous streaming remain outstanding.

## #161 post graph contract first

The graph test now requires a separate sampled post-color target, a scene/depth
read pass, and a copy-back pass compatible with single-sample and MSAA color.
Post output precedes gamma/capture; HUD can be composed after the copy-back.
The first run fails on the absent graph IDs/config field
(fidelity-post-graph-before.log). Implement these declarations/native resources
before shaders; legacy disabled descriptor hashes remain mandatory.

## #161 post profile data passes

The cooker and existing native cooked-data reader now agree on the version-1
132-byte .aspost payload (kind 11). Optional controls have documented defaults,
finite range/path checks and the existing SHA envelope. GCC and Clang/libc++
UBSan pass the authored round trip and incremental exposure edit
(fidelity-post-{pure,clang}.log). No GPU behavior is changed by this data slice.
Implement graph/passes with native image controls next; temporal vectors/history
and streaming remain separate required work within #161.

## #161 post profile contract first

The first post test authors exposure/sharpen/vignette/grain/LUT/focus/blur settings
and requires a fixed 132-byte native record in the existing hash envelope, plus
an incremental exposure edit. It fails before implementation because kind post
is absent (fidelity-post-before.log). This is authoring/layout coverage only;
actual graph passes, LUT sampling, depth effects and temporal history still need
native visual tests. Blur/grain remain zero by default. Use the published CC0
ACES-fitted curve (Krzysztof Narkowicz, 2016) with exposure in linear space; do
not claim full ACES color management. Existing display-encoded HDR scene output
must be decoded before the filmic operation, and HUD composition must remain
after it. No existing shader arrays or accepted frames will be regenerated.

## #161 original projected decal references pass

New original CC0 bullet-hole, scorch and blood sources are frozen under
tests/assets/decals (256-square RGBA plus height-derived normal maps). The
initial exporter refuses populated output; provenance pins every source. Existing
effect artwork and all accepted fixtures remain unchanged. The existing reference
check now validates/cooks both sets and repeated cooking makes no changes
(fidelity-reference-decals.log). Native review runs all nine effects and all three
marks; each mark visibly projects onto the floor and clearing restores the exact
baseline (fidelity-reference-decals-runtime.log). Captures reviewed: small dark
recessed hole/rim, broad radial scorch, red splatter. Component captures only.

Format, boundary/type and authored-schema checks pass after the material-hit and
editor work (fidelity-hit-decals-{format,boundaries,types,formats}.log). Full
post/TAA/motion vectors, streaming, combined scene, budgets and exact-head/main
hosted checks remain before #161 acceptance. No PR is open yet.

## #161 live decal editor passes

The Effects panel now loads .asdc through its existing asset control, projects at
the aimed collision surface, clears the fixed ring, and reports decal counters.
Source editing reuses the existing path/backup/conflict protections. The native
editor mode passes projection, guarded save, cooked reload and clear
(fidelity-decals-editor.log); the existing particle editor still passes
(fidelity-effects-editor-v3.log). CI runs the editor variant plus the separate
MSAA lifecycle. Original bullet/scorch/blood artwork is next; final #161
post/TAA/streaming, scene and budget acceptance remain outstanding.

## #161 material-hit decal binding passes; editor test first

The version-3 optional effect decal field passes GCC and Clang/libc++ UBSan
(fidelity-hit-decals-{pure,clang}.log). The renderer preloads the mark and projects
it once when an effect starts, rotating the effect's normal axis into decal Z.
Actual per-material weapon impacts register all referenced marks, draw them on
the floor, retain them after particle expiry and visibly clear them
(fidelity-hit-decals-runtime.log). No weapon payload/game import or simulation
changes; existing reference source bytes stay frozen. This native mode joins CI.

The existing projected-decal lifecycle test now also has an editor mode. Before
implementation it fails because effects.edit cannot load an .asdc
(fidelity-decals-editor-before.log). Reuse the same guarded source editor and
trace the aimed surface for preview; keep all work presentation-only.

## #161 material-hit decal contract first

The effect definition gains an optional decal reference, so existing per-material
weapon effect selection can spawn particles and one projected mark together.
No weapon payload, authoritative hit result or game import changes are needed.
The cooker/native layout assertions now require a version-3 effect envelope and
100-byte header. Before implementation the schema rejects the new field
(fidelity-hit-decals-before.log). Existing reference sources remain untouched;
only new cooked feature data changes. Implement renderer binding, then prove
actual weapon impacts leave visible marks after their particles expire.

## #161 projected decal verification checkpoint

The MSAA lifecycle and strengthened native ring/memory checks pass
(fidelity-decals-native-{msaa,ring}.log). The same test places 129 marks, retains
128, reports one replacement and zero drops, then clears them. GCC/Clang graph,
format/boundary/type, agent protocol and all 93 pinned shader checks pass.
Most importantly, the actual OpenArena fixed-demo replay retains the unchanged
accepted frame hash 17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(fidelity-decals-legacy-demo.log). No fixture regeneration. Native decal commands
are now included in runtime CI; main/exact-head hosted gates still belong to the
complete #161 PR, which is not open yet. Finish material-hit/editor integration
and owned bullet/scorch/blood references next; post/TAA/streaming remain required.

## #161 native projected decals: initial component pass

The depth-detached pass is now shared by projected decals and soft particles.
Decals reconstruct world positions from scene depth, clip to an oriented volume,
apply color and normal textures with light-grid illumination, and use a conservative
projected scissor. Frame-owned records copy at most four full rings; overflow and
upload drops are counted. Pool order retains newer marks over older ones.

The native test passes projection, fade/expiry, the volume-misses-floor negative
control, changed normal lighting, watched texture/definition edits and restart
(fidelity-decals-native-watch.log). Active/tilted captures were reviewed: the
same floor patch changes lighting with its tangent-space normal. The first reload
comparison omitted dev_reloadAssets; enabling the existing watcher corrected the
test setup. Initial missing captures exposed command-buffer misalignment caused
by my new draw-count field. Reordering only the new fields and asserting the
command offset restores capture (fidelity-decals-capture-debug.log). No legacy
command-reader or unrelated engine code changed.

Native graph checks pass for both pipelines and bounded uniform uploads while the
legacy descriptor oracle remains identical (fidelity-decals-graph.log). Two new
shaders are generated with bin2hex; all previous 91 arrays stay unchanged. MSAA,
shader/compiler checks and the new profiler counter build are next. Material-hit
dispatch, live editor preview and original decal reference artwork remain pending.

## #161 native projection test first

The new real-client decal test requires a projected textured floor patch, lifetime
fade/expiry, no floating billboard when its volume misses the floor, normal-map
lighting changes, watched texture/definition edits and renderer restart. It fails
at the absent decals.load command before native implementation
(fidelity-decals-native-before.log). Implement through the retained-depth effects
pass; keep sampled depth detached and copy bounded draw records per scene.

## #161 decal data and fixed ring pass

The existing effects module now accepts a 200-byte .asdc payload and owns a
128-entry copied-definition insertion ring, with replacement counts, orthonormal
projection axes, fade and overflow-safe expiry. The cooker and agent schema expose
color/normal maps, full volume size, lifetime/fade, color and normal strength.
GCC and Clang/libc++ UBSan pass (fidelity-decals-{pure,clang}.log); authored format
examples/diagnostics pass (fidelity-decals-formats.log). No GPU projection is
implemented yet. Next: native depth-projection/normal-map capture test first,
then renderer bindings, material-hit dispatch, reload/editor and reference art.

## #161 projected-decal data/ring test first

The existing effect test now requires a versioned .asdc definition with color and
normal-map paths, volume size, lifetime/fade, color and normal strength. Its native
contract requires a copied definition in a fixed 128-entry insertion ring,
overwrite counters, bounded aging and fade-to-expiry. It fails before implementation
on the absent decal cook kind (fidelity-decals-before.log). These are new feature
contracts in the existing effect target, not changes to accepted content.

## #161 sampled-depth particle pass passes component gates

Soft sprite/trail quads reuse the bounded scene polygon storage and sort their
references back to front. A separate color-only graph pass samples retained depth,
then resumes loaded scene color/MSAA/depth/stencil. Single static vertex-color
materials use alpha/additive pipelines; complex legacy materials keep their stage
iterator. r_softParticles defaults off pending final budgets. Fade distance comes
from authored particle size. GPU scope and softDraws/softDrops expose work/drops.

The live floor test passes single-sample and 4x MSAA+SSAO, complete occlusion,
zero upload drops and vid_restart (fidelity-soft-runtime-final2.log). Capture
review shows solid distant cyan, faint floor intersection and no below-floor
contribution. Initial implementation rejected 160-byte draws in the old 128-byte
uniform range; only the opt-in configuration enlarges it. A stricter material
eligibility check initially missed the existing AGEN_SKIP optimization of vertex
alpha; the final condition recognizes it without changing the legacy optimizer.

GCC/Clang native graph tests retain the exact disabled descriptor hash and cover
32 new combinations, blend state, upload exhaustion and state restoration. Pinned
shader compilation reproduces all 91 shaders; the prior 88 arrays are unchanged
(fidelity-soft-graph-{native,clang}.log, fidelity-soft-shader-check.log). Release
build, formatting, boundary/type and agent protocol checks pass. Lifetime
analysis passed all 1232 compilation commands (fidelity-soft-lifetimes.log). This is component evidence;
final reference-scene goldens and hardware budgets remain outstanding.

## #161 live soft-depth contract fails first

The native test places a cyan sprite at 32 units above, 1 unit above and 4 units
below the same floor. The previous binary gives identical near/far center values
(146536 each), so the near-depth fade assertion fails as intended; the occluded
control is zero (fidelity-soft-runtime-before.log). Commit this test before the
new depth-pass implementation. Initial new shader compilation appends only three
new arrays through bin2hex; all 88 existing arrays remain byte-identical.

## #161 soft-particle graph test first

The graph contract now requires retained sampled scene depth, a particle pass
without a depth attachment, and a compatible loaded scene resume. It covers
single/multisample color with SSAO, stencil, bloom and shadows. Before implementation
it fails compiling the absent softParticles/pass declarations
(fidelity-soft-before.log). Keep the disabled descriptor oracle unchanged.

## #161 original reference effect set passes

The nine original CC0 reference definitions, procedural PNG atlases and brass
shell source are now frozen with per-file provenance. The authoring exporter
refuses an existing populated directory and is never run by CI. Initial authoring
validation caught texture/effect name collisions and inward shell winding/normals;
these were corrected before freezing the new sources. PNG bytes were not rerun
or regenerated; no existing fixture changed. Geometry assertions require outward
triangle winding and normals (fidelity-reference-geometry.log).

The native renderer draws all nine effects, produces visible image changes and
expires every pool entry (fidelity-reference-runtime.log). All captures were
reviewed: distinct flash/light, sparks, dusty/smoky puffs, brass shell, explosion
and tracer. These are component captures, not accepted frame goldens or a final
artistic scene. Pure reference and native lifecycle commands are in regression CI.
Soft-depth flags remain metadata until their renderer path is implemented.

## #161 single-sided impact visual control now passes

Correcting only the new effect quad order/UV orientation makes the stricter native
weapon test pass (fidelity-weapon-effect-runtime-winding.log). Capture review now
shows the single-sided brown impact particles directly over the floor, then the
same empty floor after expiry; no animated player is visible. This is accepted
component evidence, superseding the earlier obscured comparisons. The original
owned material is deliberately plain; final reference art remains separate.

The reference-set test now requires frozen original CC0 sources for muzzle flash,
metal/stone impacts, smoke, sparks, dust, shell, explosion and tracer, with pinned
provenance and required sprite/mesh/trail/light/collision fields. It fails on the
absent provenance before authoring (fidelity-reference-before.log). Author this
new asset set once, review it, then retain its bytes; no existing fixture changes.

## #161 visual control caught new particle winding error

Removing the player from the comparison made the stricter visual test fail. Using
actual floor-hit event positions still showed no particles
(fidelity-weapon-effect-runtime-{overhead,hit-view}.log). Review against the existing
sprite tessellator found the new #161 particle quads wound backwards; earlier
white-marker tests used two-sided shaders and masked it. Commit the stronger test
before correcting this unmerged feature. Fix only the new effect quad order/UVs;
no legacy renderer or accepted shader changes are needed.

## #161 live weapon material effects pass

The new typed cgame imports register/start effects at the existing client renderer
boundary. Map-start registration selects .asfx material references; hit presentation
uses the existing hit normal/origin and a presentation-only seed. Legacy material
shader references keep their previous mark/explosion behavior. GCC/Clang UBSan,
Release build, formatting and C/game/engine ABI checks pass.

The real client fires 20 material hits on the unchanged owned level and reports
two registered effects, 5464 submitted particle draws, no pool drops. A tightened
run uses degree-based input, disables HUD/gun/console notifications, fixes the
camera and compares visible impacts with the expired frame. Numeric comparison passes, but capture
review found the animated player obscuring the impact area; that run is not
accepted visual evidence (fidelity-weapon-effect-runtime-visible.log). The next
run uses a narrow overhead view of the floor impact to exclude the player. The first test
used raw 16-bit angles; it exercised dispatch but was insufficient visual evidence.
Both versions are retained. The visible lifecycle command is now in runtime CI.
No authoritative damage, weapon arithmetic, hit selection or snapshot change.

## #161 weapon-effect native boundary checkpoint

The actual cgame material-hit probe now passes GCC and Clang/libc++ UBSan, including
unchanged legacy mark/explosion behavior (fidelity-weapon-effect{,-clang}.log).
Only typed presentation imports and effect dispatch change; simulation is untouched.
The new real-client test fires a cooked weapon whose material paths end in .asfx.
Against the prior binary it reaches weapon hits but reports zero registered/drawn
effects, failing as intended (fidelity-weapon-effect-runtime-before.log). Commit
this live contract before rebuilding the client with the native import change.

## #161 weapon material-effect contract fails first

A new probe calls the actual cgame material-hit presentation function. Authored
.asfx material effects must start a native burst with the hit origin/normal;
legacy shader references retain marks/explosions. The first executable fails on
missing burst dispatch (fidelity-weapon-effect-before-2.log), after correcting its
standalone native include/build setup. No damage, hit selection, weapon simulation
or snapshot representation changes are needed. Add typed presentation imports next.

## #161 live effect source editor passes

The Effects ImGui panel and effects.edit share queued source/preview actions.
The existing source-saving logic now serves both animation and effects: bounded
loose JSON paths, numbered backups, read-back validation and refusal to overwrite
external changes. Effect and Profile panels expose fixed-pool/light counters.
Both effect editing/replay and the unchanged animation editor pass
(fidelity-effect-editor.log, fidelity-shared-source-editor.log). The Effects
panel capture was reviewed. Renderer reset clears its preview handles.

The regression workflow now includes GCC/Clang pure effects/LOD checks and native
effect, effect-editor and LOD lifecycle commands. Full #161 hosted/current-main
gates remain later; no PR is open yet. Next: reference effect art and weapon
material-hit bindings, then depth-soft particles/projected decals/post/TAA and
budgeted streaming. Final reviewed scene and measured hardware budgets remain
mandatory, and none of these component checks claim full issue completion.

## #161 shaped native effects and light hooks pass

Native sprite size evolution/rotation and bounded per-emitter light submission
pass visible frame, expiry, stable memory, watched reload and restart controls
(fidelity-effect-lights.log). The active capture was reviewed: rotated larger
marker plus colored light on the nearby floor. The scene light pool bounds work;
agent counters expose accepted and dropped light submissions. The build initially
caught a missing declaration for the existing scene light count; corrected before
passing. This remains component evidence, not the final reference effect set.

The next test requires an Effects ImGui source panel shared with native agent
edit actions, save backups/conflict protection, then recook/replay. Its initial
run fails on absent effects.edit (fidelity-effect-editor-before.log). Implement
by reusing the existing source editor's file helpers and preservation rules.

## #161 seeded effect shaping passes

GCC and Clang/libc++ UBSan pass version-2 cooked records, stable seeded spread,
rotation/spin and all existing pool/lifetime/collision controls
(fidelity-effect-shaping{,-clang}.log). New optional authoring fields retain the
old behavior when omitted. The live light-hook test now fails explicitly on absent
lightDraws after loading its rendered map (fidelity-effect-lights-before.log).
The first probe attempted the stats request before loading a map and was corrected;
that invalid-state response was test setup, not a renderer failure.

## #161 reference-effect shaping contract

The next effect test requires seeded velocity/origin spread and rotation/spin,
plus fixed cooked fields for size evolution and light emission. These are needed
for distinct sparks/smoke/shells rather than coincident identical particles. The
new cooked envelope is version 2; no accepted effect asset exists or is regenerated.
The test fails on the old version before implementation
(fidelity-effect-shaping-before.log). Add shaping and native light hooks next;
soft depth and full reference/material/editor coverage remain outstanding.

## #161 native LOD lifecycle passes

The owned grid now passes both GCC and Clang/libc++ with UBSan, including unchanged
animated matrices and scaled screen selection (fidelity-lod-native{,-clang}.log).
The real client passes near/far LOD draws, unchanged hunk/tag memory, removal of
lod_ratios while stale sibling files remain, restoration, and renderer restart
(fidelity-lod-runtime.log). Near/far captures were reviewed: the same white grid
is visible at both distances. These controls are not final artistic goldens.

The first live selection attempt retained the historical r_lodbias=-2 and put the
far camera outside the room. The test now explicitly selects r_lodbias=0, scales
the grid to four units and uses two in-room cameras; no legacy default changed.
Model asset diagnostics expose lods and four draw counts. Release build and source
format/boundary/type checks pass. Performance savings and larger animated scene
coverage still belong to final #161 acceptance. No accepted asset regenerated.

## #161 native LOD implementation checkpoint

The native GCC/UBSan probe passes unchanged animated matrices and screen-size
selection (fidelity-lod-native.log). Registration now reads the hash-bound set,
checks shared skeleton/animation/material compatibility and retains cached model
handles. All model replacements complete before LOD links refresh. Release client
build passes (fidelity-lod-build.log). Live selection/reload is not accepted yet.

The new engine-level test reuses the same owned animated grid and requires visible
near/far draws, stable memory, disabling stale siblings after removing lod_ratios,
reenabling after republishing, and vid_restart. Its initial run fails because the
model registry has no lods/lodDraws counters (fidelity-lod-runtime-before.log).
Add these development diagnostics and run the full lifecycle next.

## #161 native LOD contract

The cooker manifest now passes its initial assertions. A new native LOD probe
requires unchanged animated joint matrices, compatible skeletons, projected-size
selection, scaled-entity handling, bias clamps and no selection-time allocation.
Its pre-implementation compile fails on absent R_IQMLodCompatible; the IQM branch
of the existing LOD selector is also still absent. Native registration/reload and
visual lifecycle coverage remain to follow.

## #161 LOD manifest contract fails before implementation

The extended cooker test requires a versioned .aslod sidecar binding the original
IQM SHA256 to every named reduced IQM and its SHA256. The pre-implementation run
fails on missing models/grid.aslod (fidelity-lod-manifest-before.log). This keeps
stale sibling files from silently activating after a recipe changes. Commit the
contract before implementing the cooker and native screen-size selection.

Main build/publication 35589995081 passed all 16 compiler legs and create-testing;
merged regression 35589995016 is still running, so #164 acceptance remains pending.

## #164 merge and #161 compatibility checkpoint

PR167 is ready/merged after every required final check passed. The conditional
PR publication jobs are not required PR gates; the actual main publication must
now pass. Merge 81e40b0c has the exact tested source tree. Main is merged forward
into #161 without rewriting either branch. The old authored branch remains clean.

The initial #161 effects/HDR/LOD tree passes both fixed OpenArena replay fixtures
and accepted Mesa frame hash 17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96.
GCC/Clang LOD payloads are byte-identical. Protocol, format/boundary/type and authored
schema controls pass; the schema command needs the private Go bin directory.
This does not replace #161's eventual full exact-head/current-main gates.

## #161 offline mesh LODs pass

Pinned meshoptimizer 1.2 at 9d9890c73011d75920af614485296d1e03e95448 supplies
only its allocator/simplifier/header and MIT license; provenance records all file
and source archive SHA256 hashes. It is linked only into the offline cooker helper.
Optional lod_ratios produces up to three compact native IQMs, using position,
normal, UV and skin-weight error with locked joint-set and mesh boundaries.
The original full-detail geometry stays unchanged; all vertex attributes are
retained from original vertices. Unsupported reductions may stop at the error
limit rather than discard geometry arbitrarily.

The owned animated grid reduces 512/289 triangles/vertices to 256/161 and 128/97;
files shrink from 23320 to 13080 and 7960 bytes. Joint/pose/animation bytes remain
identical and repeated output is deterministic (fidelity-lod.log). Native screen
selection, animated visual comparisons and performance remain outstanding. No
accepted asset or existing shader was regenerated. Official pin/source:
https://github.com/zeux/meshoptimizer/tree/v1.2.

## #161 initial native sprite/reload/restart pass

Release client/server build succeeds with the fixed effect code linked into the
renderer only. Shared renderer API controls register/start/stop/report; the native
agent calls the existing renderer accessor. The renderer submits sprite/mesh/trail
primitives, copied material/model bindings and optional static-world collision,
with fixed pools and no play-time allocations. Authored effects retain their data
and bindings across reload. Watched reload validates the published whole-file hash.

The new native test passes on lavapipe/OpenArena: eight visible sprites, expiry,
unchanged memory counters, a watched edit from eight to three particles, and
renderer restart/re-registration (fidelity-effects-runtime.log). The initial
square-marker capture was reviewed; it is a rendering control, not the final
reference effect art or an accepted golden. Own MAP/BSP/AAS remain unchanged.
Build integration initially caught two renderer API type mismatches, corrected
before the successful build. Mesh/trail/soft/light/reference/editor/performance
coverage is still outstanding; nothing here claims full #161 acceptance.

## #161 native effect rendering contract

The client-level test now requests effects/load/start/stop through the native
agent, then requires visible sprites, counters, expiry, stable engine memory,
watched definition reload and renderer restart on the existing owned two-lane
level. Its pre-implementation run fails immediately because those commands are
absent (fidelity-effects-runtime-before.log). No new golden is accepted yet;
soft particles, mesh/trail/light paths and reviewed reference effects remain
additional required coverage. Implement the shared renderer/public controls next.

## #161 floating target component pass

hdr=2 now selects RGBA16F for offscreen rendering and reports its format; existing
hdr=-1/0/1 and direct modes stay unchanged. Device setup requires sampled/blended
linear-filtered float targets and the current RGBA8 capture conversion, rejecting
unsupported hardware explicitly. The native graph test passes, including its
unchanged descriptor hash, shadows and SSAO (fidelity-graph.log). No shader
regeneration. This is not filmic/post acceptance: real captures and full post/TAA
integration remain pending, and defaults remain at their accepted values.

## #161 initial effect cooker and fixed-pool runtime pass

The existing cooker now accepts validated effect definitions, emits .asfx in the
version/hash envelope and publishes kind 8. New engine/effects code uses 4096
particles/128 instances with a free list, bounded emission, copied active definitions,
rate carry, lifetime/flipbook updates, local emission axes, gravity/drag and optional
caller-provided collision. It owns no clock, world state or heap memory.
The original committed probe passes GCC and Clang/libc++ with strict FP and UBSan
(fidelity-effects{,-clang}.log). This is pure component acceptance only: CMake,
client rendering, public services, material/soft/light/trail paths, reference effects,
editor/hot reload, full gates and all other #161 deliverables remain outstanding.
No authoritative simulation or existing asset bytes changed.

## #161 independent implementation sequencing

The previous checkpoint conservatively deferred all implementation until #164's
merged-tree acceptance. The pure effect cooker/runtime does not depend on #164,
so continue it in this isolated issue branch while the unchanged #164 head runs
CI. This avoids idle gating time without mixing source changes or weakening any
merge gate. Integrate accepted #164 main before opening the #161 PR, and require
current-main exact-head plus merged-tree checks as usual. Effects, HDR and LOD
failing contracts are committed first. No #164 check is bypassed or restarted.

## #161 initial mesh LOD contract

The owned two-joint source generator now supplies a smooth-weighted 17x17 grid
for tests/lod.py. The initial lod_ratios recipe requires two smaller compact IQMs,
unchanged full-detail geometry, preserved joints/poses/compressed animation and
byte-repeatable cooking. Recipe provenance hashes are intentionally excluded only
from the original-geometry comparison; every payload still verifies its stamp.
Before implementation the original geometry passes and grid_lod1.iqm is absent
(fidelity-lod-before.log). No meshoptimizer import, cooker or native LOD changes
yet; screen-size selection and actual runtime cost remain future acceptance gates.

## #161 initial floating HDR contract

The existing native graph probe now checks that a new hdr=2 offscreen mode uses
R16G16B16A16_SFLOAT, while direct/legacy modes retain their exact formats. The
pre-implementation run passes the accepted 36-configuration descriptor hash and
all existing shadow/SSAO configurations, then fails the floating-format assertion
(fidelity-graph-before.log). No shader bytes or accepted oracle were regenerated.
This fixes the native format expectation only; post passes, history/motion vectors,
feature-on software references and measured budgets are still outstanding.

## #161 hardware measurement preparation

The existing renderer runs on the local RTX 3080 Ti at verified 2560x1440
capture/render resolution. Presenting a full-size image to private Xvfb adds
substantial CPU cost (44.033 ms median); this is not evidence of GPU overload.
Using existing r_fbo/r_renderScale with a 2560x1440 offscreen target and 640x360
presentation gives CPU frame p50/p95/p99 4.689/6.386/9.009 ms after 4096 warm
frames. Across 50 samples, GPU main p50/p95 is 1.096/1.100 ms and gamma is
0.271/0.275 ms. NVIDIA ICD/device and the saved 1440p PNG dimensions are checked.
Evidence: fidelity-hardware-{baseline,small-present}.py/.log and the corresponding
report.json/capture directories in the private modernization cache. This is a
static existing-renderer baseline, not #161 feature/streaming acceptance. The two
runs use different offscreen settings and are not a feature A/B comparison.
A matched large-window offscreen run (fidelity-hardware-large-present) measures
CPU p50/p95/p99 44.697/54.831/87.905 ms; GPU main p95 1.324 ms and gamma p95
0.586 ms. Both runs render/capture 1440p with the same offscreen settings, which
isolates the costly Xvfb presentation size. These are hardware/device-dependent
baseline measurements, not acceptance thresholds. All later hardware budgets
must use a consistent render/presentation configuration.
No repository renderer or accepted frame changes were made for this experiment.

## #161 native fixed-pool effect contract

The native probe specifies trivial fixed storage, valid cooked definition opening,
constant-velocity/rotated-axis movement, gravity/drag, per-emitter capacity,
flipbook progression, lifetime expiry, fractional rate
carry, stop/stale-handle behavior including expired-slot reuse, collision reflection through a caller callback,
independent presentation state and visible fixed-pool overflow counters. The
pre-implementation compile fails on engine/effects/effects_public.h. The effects
driver will compile it with strict FP/UBSan and the existing SHA256 implementation
after a valid cook. No implementation has started while #164 awaits acceptance.

## #161 initial effect authoring contract

The existing agent effect schema is currently authoring-only. tests/effects.py
now requires it to cook into the common version/hash envelope and a fixed native
record layout, appear as kind 8 in the content index, skip unchanged input and
recook an edited definition. Capacity, mesh binding and duplicate emitter authoring
errors must fail. Initial run fails with "asset kind is not implemented yet:
effect" (effects-cook-before.log). No runtime, renderer, game, shader, cooker or
accepted fixture was changed for this preparation.

Reuse the existing cooker/envelope, client render frontend, fixed native data,
ImGui tooling and render graph. #161 is presentation-only: no authoritative
movement/snapshot arithmetic or per-frame allocation. Reference effects, projected
decals, post/TAA, LOD/streaming, live data editing and real 1440p GPU measurements
are still required; this first contract is not implementation or acceptance.

## #31 descriptor restoration local verification

Test-first commit 629bd78b advertises a device with 32 descriptor sets while the
engine cache/layout has five; the native SSAO assertion fails before the fix
(descriptor-before.log). Actual accepted #164 binary also crashes with r_ssao 1
on the owned street on RTX 3080 Ti (descriptor-rtx-before.log). The correction
rebinds only initialized cache slots with the saved uniform dynamic offset and
clears the dirty range. No invalid trailing slot or hardware-limit array access.

GCC/Clang native graph checks now cover device capacities 4/5/32 with sparse
bindings, unchanged descriptors, exact dynamic offsets and cache reset. Both
pass (descriptor-after.log, descriptor-clang.log). Repeated mocked pipeline
creation initially needed the test's fake handle reset; no production workaround
was added. RTX SSAO then passes (descriptor-rtx-after.log). Software half/full
SSAO and 4x MSAA+bloom pass exact disable/restart comparisons
(descriptor-ssao.log). Fixed OpenArena replay hash remains
17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(descriptor-demo.log). Format, boundaries and type checks pass.

Self-review: one descriptor-state bug, fixed-capacity stack state only, no new
OS access, allocation, nontrivial lifetime, ABI/layout or simulation FP changes.
No accepted fixture, expected-failure entry or UBSan suppression is changed.
All writes stay in msetaro/aftershock; hosted gates remain mandatory before merge.

## #164 default command and final self-review

The exact documented build command (no supplied binaries or prepared kits) passes
with locally installed Quake 3 content: automatic pinned material fetch, Blender
kit, Release development client/server build, repeated MAP/BSP/AAS, 1.0 per-class
overlap, safe spawns, 5.2-second route, 6000 bot frames, 18 kills/51 pickups, no
observed stuck bots, two named/eighteen fly-through captures and no warnings.
Evidence: sketch-default-command/report.json/build.log and its private kit/build
outputs. No installed paks enter the published output; engine homes use symlinks.

V1 compatibility narrowing passes both compiled gates (sketch-v1-final.log,
sketch-v2-final.log): accepted MAP/BSP/AAS hashes remain 555965db / 0d0fcf2c /
12889533. Native agent protocol, strict source format, tracing/ambiguity and shooter
controls pass again. The full build ambiguity negative also fails explicitly
with its retained assumptions/overlay (sketch-ambiguous-build.log). Supplied-retarget PBR/byte-repeatability/license controls
pass (sketch-retarget-pbr.log). Workflow lint/isolation passed; final hosted gates
remain mandatory. Known-good tag object/target remain 8bc8c94c / 81a0f9dc.

Self-review: changes match #164's geometry/sketch/licensed-theme/agent acceptance
scope. The only native addition is a development-only static collision query using
existing CM_BoxTrace and trivial stack fields. No simulation FP, wire/file layout,
core ownership/allocation, production OS access or gameplay behavior changes.
V1 keeps its generator, material lookup and exact OBJ bounds. Theme publication
still permits only maintainer-listed licenses; private provided-rig processing
preserves licenses and cannot enter a theme when disallowed. No accepted fixture,
shader data, rollback tag or external repository is changed. Original drawing and
new tool controls are test-first; intermediate failures remain in the checkpoint.

Declared limits: the agent supplies semantic readings; unresolved marks remain
assumptions and fail integrated acceptance. Overhead is an honest CPU geometry
projection of compiled BSP. Contact/cover metrics are sampled estimates; route
times are native movement measurements. Full bot acceptance currently uses FFA.
The reference theme has no ambient/decal/effect assets; those sets are empty.
Supplied retargeting requires applied object transforms, one mapped root and
rotation/root-motion clips; unsupported animation scale fails explicitly.

## #164 full iteration passes; v1 compatibility review

Both complete reference/agent builds pass in sketch-reference-resting{,-edited}.
The two-storey/window variant records 16 kills, 49 pickups, no observed stuck bots,
1.0 overlap in all classes, 5.2-second resting-start route, two named and eighteen
fly-through captures. MAP source groups differ only for building_7. A separate
native review camera confirms the upper window/storey and stairs
(sketch-window-review/building_7.png). The exact default command is now running
with automatic kit preparation/build and locally installed Quake 3 content.

Self-review found shared material discovery was also reading ASMAT sidecars for
version 1, which historically ignores them. A new v1 compatibility test fails
(sketch-v1-sidecar-before.log). Restrict the new cooked-material staging to v2;
preserve v1's exact historical OBJ bound tolerance as well. This is correction of
the unmerged #164 feature path, not an engine bug fix. Re-run the v1/v2 compiled
fixture checks after narrowing the shared helper. No golden regeneration.

## #164 resting-start timing and provided retarget pass

The corrected native route test returns through normal input to the annotated
start and settles before recording time. It records zero initial velocity and
5.7 seconds over the 1800-unit lane; the impossible-time negative and subsequent
6000-frame OpenArena bot/capture checks pass (sketch-route-resting.log, 22 kills,
49 pickups, no observed stuck bots). The reference drawing's corrected build also
passes: 1.0 overlap for all classes, spawn safety, 5.2-second route, 17 kills,
53 pickups, 240 samples, no stuck bots, two named and 18 fly-through captures.
Reviewed captures show open doors, upright modules and the authored street.
The two-storey/window iteration is now collecting its native bot/capture evidence.

Provided-rig retarget passes with pinned Blender 5.0.1 and unchanged existing GPL
inputs (sketch-retarget-exported.log). A sixteen-joint body walk drives the supplied
three-joint character through explicit mappings. Exported inverse-bind matrices
match the supplied target, exported root displacement matches the supplied clip,
pose/bind error limits pass, native cooking succeeds, and two fresh output trees
are byte-identical. Missing mapped bones and changed input hashes fail without
publishing partial output. The CC0 publication validator rejects the private GPL
result, whose licenses remain unchanged. Theme publication policy did not expand.
The ordinary license/unlisted/hash negative suite also passes.

The regression workflow now shares one prepared material/module kit between the
component and full reference/agent iteration tests and runs the supplied-retarget
gate. Workflow lint and invocation-isolation checks pass. Full exact-head hosted
checks still must run; PR167 stays draft until all #164 acceptance is complete.

## #164 supplied-rig retarget contract

The existing owned three-joint character and sixteen-joint body/walk clip are
inputs to a new private conversion test; their accepted glTF/binary bytes remain
unchanged. The test fails first on the absent `tools/blender retarget` command
(sketch-retarget-before.log). It requires explicit bone mapping, rest-offset
preservation, transferred motion/root displacement, native IQM cook, repeated
bytes, source hash rejection and complete retained source licenses.

Decision: import/retarget is private processing, not theme publication. Existing
GPL inputs retain GPL in output provenance; `tools/assets validate` and theme
assembly must still reject them under the unchanged CC0 publication policy. No
new character, rig, weapon, hands or weighted animation is authored or sourced.
Use only the supplied existing fixtures for this test. The command must stage
verified dependencies and publish no partial output after an import/mapping error.

## #164 route-start review catches teleport launch

The initial native reference run passes compiled geometry overlap (all three
classes 1.0), spawn safety (656 queries, no errors) and its provisional timed lane.
Review of its starting state found that the existing setviewpos command launches
players at 400 units/s: the timer begins about 32 units beyond the annotation.
This is a setup flaw in the new #164 tool, not an engine movement bug. A new
resting-start assertion fails on the retained report (sketch-route-start-before.log).
Return to the annotated start through normal walking and allow velocity to settle
before measuring. Prior 5.5/5.06-second numbers are provisional, not final timing
acceptance. Repeat native timing/negative controls after correcting the harness.

The first integrated native build is still collecting its bot/capture evidence.
The one-command implementation and agent alias retain fresh output ownership,
notes/overlays, complete kit manifests, compiled IoU, shooter/routes and repeated
compile/native reports. The follow-up native iteration contract compares source-ID
MAP groups and requires the agent entry point. Full #164 acceptance remains open.

## #164 native route negative control passes

sketch-route-native-4 passes the complete combined OpenArena gate. Actual 1800-unit
walking takes 5.5 simulation seconds: accepted at 6 +/- 2 seconds and rejected at
0 +/- 0.1 seconds. The deliberately exposed spawn pair is rejected by native
collision sight queries. The subsequent strict 6000-frame bot/capture gate passes.
No duplicate screenshots or existing evidence are overwritten.

The owned drawing now converts explicit spawn markers, preserves pickups/cameras
and full notes, and strips drawing-only fields from playable intents. Theme
assembly supplies a recorded 96-unit entrance facing the boundary centroid only
when openings were not explicitly supplied. Existing explicit/empty openings
remain authoritative. The reference and two-storey building-7 variant pass schema,
physical geometry and connected-spawn checks; other shapes and dressing remain
identical (sketch-build-semantics.log). The integrated native command is running
in sketch-reference-native. Its compiled overhead scores 1.0 for building, solid
and wall classes against a 0.93 threshold; native shooter/bot results are pending.

## #164 route playback and owned reference contract

The native lane walk measures 5.5 simulation seconds for 1800 units. The existing
agent playtest CLI passes unchanged default-session behavior. Static controls now
also reject a hold inside collision and annotations outside the boundary; explicit
floor coordinates survive tracing. The combined route negative/runtime gate was
interrupted (exit 143) after its two movement reports; it is not accepted. Repeat
is running in sketch-route-native-4, including the impossible-time target and
strict OpenArena bot/capture checks. No accepted fixture changed.

A new original CC0 reference drawing and its manual export source are under
 tests/assets/sketch (source/hash manifest and CREDITS validated). Visual review
confirms three numbered buildings, rotated cover/building, curved wall, two
indoor spawn markers, a timed route and a hold annotation. tests/sketch_build.py
fails first on drawn spawn markers not becoming player spawns
(sketch-build-before.log). The same contract will require preserved notes/goals,
recorded default door decisions, only-building-7 edits, and the one-command native
pipeline. Continue this implementation; this remains partial #164 work.

## #164 static shooter report passes; movement contract added

The controlled-world trace tests pass (sketch-intents.log): exposed spawns,
incorrect sightline claims and exposed/missing objectives fail with suggestions;
lane widths, eight-direction sightline histograms, waist-height cover proximity
and symmetric first-contact estimates are reported. Travel estimates are labeled,
and routes remain explicitly pending movement playback.

The themed native test now requires the real engine to reject its deliberately
exposed spawn pair and then physically walk the annotated lane within its stated
time tolerance. It fails first on the absent play_routes function
(sketch-route-play-before.log). Reuse the existing agent playtest controller,
record simulation elapsed time and retain the script/report evidence.

## #164 shooter report contract

The controlled-world report test requires agent-traced spawn/objective visibility,
explicit blocked/clear sightline checks, lane/cover/sightline metrics and a labeled
first-contact estimate. It must reject exposed spawns and an incorrectly claimed
clear sightline, with suggested fixes. Routes explicitly require a subsequent
movement playtest. The contract fails on the absent intents module
(sketch-intents-before.log). Use the actual native trace channel for integration;
keep estimated travel time distinct from measured movement playback.

## #164 oriented modules and active OpenArena bots pass

Both fresh Blender kits pass native OBJ-axis bounds and byte-repeatability checks
(sketch-obj-axes.log). Retained corrected kit: manhattan-modules-yup in the user
cache. V2 validation converts Y-up OBJ coordinates with the same x,-z,y convention
as the pinned compiler; v1 validation/output remains untouched. Compiled facades
reach their declared 128-unit height, and the actual module image was reviewed.

The reference sample now gives bots reachable weapon/ammo/health/armor goals.
Its strict OpenArena run passes 6000 frames with 22 kills, 49 pickups, 240 samples,
no observed stuck bots and no warnings (sketch-theme-oriented.log/report.json).
A separate control caught below-origin crate trim being omitted; placement now
accounts for the measured lowest vertex. Crates appear in the reviewed capture.
Polygon/overhead/v1 compiled gates pass (sketch-obj-polygons.log). No accepted
fixture or game package changed. Continue shooter intent reporting and integrated
sketch build; supplied-rig retargeting remains open.

## #164 native review exposes OBJ axes and idle OpenArena bots

The OpenArena trace/capture process completed, but it is NOT reference-map
acceptance: its report contains zero kills and two inactivity warnings. The test
now requires at least two kills and no observed stuck bots. The earlier Quake 3
runs measured 13 kills/zero stuck; content-specific behavior cannot be assumed.
The authored sample has no pickups; give bots deliberate reachable item goals.

The new module camera also shows frames lying flat. q3map2's documented source
converts OBJ (x,y,z) to (x,-z,y), while the procedural export used Z-up. New controls
compare the exported OBJ after that conversion against native bounds and require
compiled facade surfaces to reach z=128. Fix the #164 export/validation convention
and repeat; do not change the compiler or v1 goldens. Primary source:
https://github.com/Garux/netradiant-custom/blob/master/libs/picomodel/pm_obj.c
The old failing evidence remains in sketch-theme-agent/images/modules.png.

## #164 compiled-world agent traces pass

The protocol gate and fresh client/dedicated release builds pass after making the
existing vector reader available to both command dispatchers. Real OpenArena
native-map queries pass: the doorway sightline is clear, the adjacent wall stops
at y=63.875, and the standard player box contacts the floor at z=24.125. Evidence:
sketch-theme-agent/agent-traces.json; sketch-trace-query.log; sketch-build.log.
Boundary/type/format checks pass. The associated bot/capture process finished but exposed the acceptance problems
recorded above. After correcting them, use the query for shooter metrics and explicit intent checks. This command
only exposes existing static-world collision in development builds.

## #164 agent collision-query contract

Shooter intent validation needs compiled-world sight and player-clearance queries
through the existing local agent channel. The dispatcher test fails first because
`trace` is absent (sketch-trace-query-before.log). Add a development-only command
that calls the existing CM_BoxTrace with either a point/solid mask or the standard
player box/player-solid mask. This is a new authoring query, not an engine bug fix
or a simulation change. Actual owned-map clear/blocked traces will verify it.

## #164 module material correction passes

The material-role assertion fails before the binding change
(sketch-theme-module-material-before.log). V2 now allows additional material roles
of at most 12 characters, keeping native shader paths inside the existing limit.
Facade/cornice props use their own baked PBR materials instead of wall UVs. A fresh
native run passes and its street image was reviewed (sketch-theme-baked.log and
sketch-theme-baked/images in user cache). Brush texture scales remain explicit;
no engine change or accepted fixture regeneration was needed for this correction.

## #164 themed PBR staging and scale pass

The scale assertion fails before implementation (sketch-theme-scale-before.log),
then passes with explicit 0.0625 brush-role scales. Native facade/street captures
were reviewed at /home/matt/.cache/aftershock-modernization/sketch-theme-scaled.
The repeated 6000-frame run again reports 13 kills, 240 samples, no stuck bots and
no warnings. Schema/trace controls and compiled polygon/v1 identity gates pass
(sketch-theme-formats.log, sketch-theme-trace.log, sketch-theme-polygons.log).
CI now exercises real pinned theme fetch/module assembly; it continues to use
owned game content for its later runtime gates. Next integrate the command and
add agent-backed intent/shooter checks; supplied-rig retargeting is also open.

## #164 initial themed native run passes

Real pinned material and Blender kits assemble repeatably with complete source
license manifests. Door/lane/spawn keep-clear checks and native PBR staging pass
(sketch-theme.log). Actual native compilation, 6000 bot frames and fixed-camera
captures pass: 13 kills, 240 samples, no observed stuck bots, no warnings
(sketch-theme-native.log; retained sketch-theme-native/report.json and images).
Visual review found the brush material tiling too large. Add an explicit per-role
texture scale and useful facade viewpoints before accepting the reference scene.
This is still component acceptance, not the completed one-command sketch pipeline.
Hosted build 35575309830 and regression 35575309836 passed the prior 5e65ed2a
checkpoint; all final-head checks remain required for PR167.

## #164 theme assembly contract

The next test requires the real pinned material/module kits to assemble seeded
props without changing shapes, preserve doorway/lane/spawn clearances, retain
complete license manifests and stage PBR material bindings through the level
compiler. It fails on the absent theme module (sketch-theme-before.log). Native
shader lookup checks explicit scripts before ASMAT, so compiler-only source-ID
aliases must not shadow PBR at runtime. Reuse the existing cooker payloads.

## #164 compiled overlap and v2 camera paths pass

Repeated compiled geometry, the 0.93 overhead gate and its displaced-building
negative control pass. The owned fixture measures 1.0 building/solid IoU; shader
aliases carry IDs but classification pixels come only from compiled triangles.
V2 fly-through paths now reuse the clearance BFS, reach the roof and remain out of
compiled solid interiors (sketch-flythrough.log). Accepted v1 MAP/BSP/AAS bytes
still match. These components still need the integrated sketch/theme build and
actual native bot/capture acceptance; #164 remains a draft.

## #164 v2 fly-through contract

The multi-storey fixture now requires automatic camera samples reaching the roof
without entering compiled brush interiors. It fails because the old fly-through
reads v1 rooms (sketch-flythrough-before.log). Reuse the existing player-clearance
search paths for v2 instead of inventing a second navigation sampler.

Compiled-overhead implementation has passed its independent control: building and
solid class IoUs are 1.0 at threshold 0.93; displaced expected geometry fails.
The actual 30-triangle class image was reviewed. Retained report/images:
/home/matt/.cache/aftershock-modernization/sketch-overhead-evidence.

## #164 compiled-overhead contract

The compiled polygon test now requires a CPU orthographic projection of actual
BSP surface triangles, class IoU against the independent interpretation pixels,
a colored overhead and a difference image. A displaced expected building must
fail the stated 0.93 threshold. The contract fails on the absent overhead module
(sketch-overhead-before.log). Keep source IDs in v2 surface shader aliases for
classification; do not infer actual footprints from authored polygons.

## #164 photographed/styled tracing passes

Both original playbook controls and the new dark/perspective drawing pass
(sketch-measure.log). Explicit regions split touching outlines; thin/dashed paths
and curves remain distinct geometry; player-height scale and measured-grid angle
snapping work. Antialiased key-color matching avoids false cross-color contours
introduced by perspective correction. One-pixel stroke expansion and inferred
angles are recorded. Semantic recognition remains the agent's explicit reading;
unread marks and uncertainty stay visible. No engine or accepted asset changed.

## #164 additional tracing contract

A new owned dark-background drawing specifies touching-outline splits, thin and
dashed straight/curved marks, player-height scale, nearby rotated-grid snapping
and phone-perspective rectification. It fails first on the unsupported scale
reference (sketch-measure-before.log). Implement measured geometry while retaining
explicit semantic regions and visible assumptions; no commercial drawing is used.

## #164 v2 props pass

Rotated props compile through pinned q3map2/MBSPC and solid props correctly reject
blocked spawns (sketch-props.log). Local `bounds_center` supports foot-origin
modules without changing mesh geometry; its independent MAP occupancy check passes
(sketch-props-offset.log). Shared OBJ source validation retains the v1 contract;
legacy controls and accepted MAP bytes pass (sketch-props-v1.log). No accepted
fixture, engine code or game package changed.

## #164 v2 prop contract

The rotated OBJ prop contract fails first because the v2 schema lacks yaw
(sketch-props-before.log). It also requires solid props to participate in spawn
clearance. Reuse the existing self-contained OBJ validation, preserve v1 outputs,
and add v2 placement/collision without changing engine code or accepted fixtures.

## #164 procedural overlap correction passes

The new coplanar-triangle control passes for every module, and both complete kits
remain byte-identical (sketch-module-overlap.log). Facade/doorway frames now meet
their caps instead of overlapping them. A fresh actual contact-sheet render was
reviewed: the black top-face artifacts are gone. Corrected retained kit and image:
/home/matt/.cache/aftershock-modernization/manhattan-modules-clean
/home/matt/.cache/aftershock-modernization/manhattan-modules-clean-preview.png.
Continue integrating these verified modules into v2 themes and actual map playtests;
supplied-rig retargeting and remaining tracing/intent/overhead work remain open.

## #164 procedural visual review / coplanar control

A retained kit at /home/matt/.cache/aftershock-modernization/manhattan-modules
validates 190 source/cooked files. An actual Blender contact-sheet render exposed
black coplanar-overlap artifacts on facade/doorway caps. Bake colors themselves
are correct. The new glTF geometry check fails on overlapping upward-facing
triangles in the facade (sketch-module-overlap-before.log). Lower the shared frame
to meet the cap without overlap, then repeat kit generation and visual review.
The retained preview is manhattan-modules-preview.png in the user cache.

## #164 procedural kit reproducibility passes

Two fresh pinned Blender 5.0.1 builds pass with byte-identical published kits
(sketch-blender.log). Nine module types have UVs, actual CPU diffuse-color bakes,
strictly reduced triangle-count LODs, map-compiler OBJs and production IQM/PBR
outputs. Complete manifests record CC0, generator version and script/parameter
hashes, with every source/cooked file covered. The CLI refuses existing output.
Hosted runtime now runs this contract and caches immutable pinned compiler,
Blender archive/extraction and downloaded CC0 source bytes. No engine changes.
Next retain/review a reference kit, accept supplied glTF/rig animation inputs,
then finish tracing, themes and the full agent/overhead/intent acceptance.

## #164 procedural reference-kit contract

The new Blender test requires pinned 5.0.1, deterministic parameterized facade,
doorway, cornice, curb, stairs, fence, barrier, crate and sign modules; real UVs,
LOD exports and baked images; production IQM cooking; and complete licensed
provenance with script/parameter hashes. Two fresh builds must match byte-for-byte
apart from diagnostic logs. It fails first because tools/blender is absent
(sketch-blender-before.log). The verified user-cache Blender binary is available;
use it for this implementation, without installing system packages.

## #164 asset preparation acceptance / CI wiring

The preparation fingerprint control passes along with repeated offline kits and
cache hash verification (sketch-preparation.log). Fingerprints include fetch and
manifest sources, the pinned Pillow version and the existing cooker's tool hash.
The runtime job now runs sketch interpretation, manifest/license controls, API
adapter controls and offline fetch/cook reproducibility after level dependencies
are installed. All are owned/offline CI controls; actual two-provider sourcing was
also measured locally above. Next: pinned Blender reference kit, remaining tracing
measurements, shooter intent/overhead/runtime integration and final #164 gates.

## #164 preparation fingerprint control

The new fetch test corrupts an otherwise valid kit's preparation fingerprint.
It fails because the existing-output fast path reuses the stale kit
(sketch-preparation-before.log). Include the fetch/manifest source, pinned Pillow
version and the existing cooker's own tool hash in that fingerprint, and refuse
reuse after a mismatch. A fresh output remains the explicit upgrade path.

## #164 source-range correction passes

The owned 16-bit gradient contract passes after normalization before RGBA conversion
(sketch-height16.log). A fresh real Manhattan kit also validates all 36 files;
its prepared ambientCG height map retains range 10..219 instead of saturation
(sketch-fetch-normalized.log). Originals remain byte-identical. Corrected kit:
/home/matt/.cache/aftershock-modernization/manhattan-kit-normalized.
Review also identified that existing-output reuse only checks the lock and file
hashes; add a preparation/cooker fingerprint control before accepting reuse across
tool updates. This is still unmerged #164 tooling, with no engine changes.

## #164 16-bit preparation control

The fetch contract now includes an owned I;16 gradient and requires normalized
prepared values 0..223, not saturation at 255. It fails as expected on the current
RGBA conversion (sketch-height16-before.log). Normalize unsigned 16-bit grayscale
before conversion while retaining the original file and its original SHA256.
Then rebuild the real kit into a fresh output and revalidate all source/cooked hashes.

## #164 real two-provider kit checkpoint

Live API searches and both adapters pass. The reviewed reference lock pins Poly
Haven brick_wall_001 (authors Dimitrios Savva/Rob Tuytel, revision fd60577b...) and
ambientCG PavingStones036 (2018-11-28 release plus archive SHA256 256824ad...).
All ten PBR channel files have exact SHA256 pins; provider metadata/attribution and
license URLs are retained. No art payload is committed: only the lock lives in
tools/assets/themes/manhattan.json. The actual kit in the user cache cooks both
materials and validates 36 covered files plus CREDITS (sketch-fetch-live.log).
Color textures were visually reviewed. Before accepting source preparation, add
and fix a 16-bit grayscale control: the ambientCG displacement original is I;16,
and the current convenience RGBA conversion clamps its prepared copy. Original
bytes are preserved; this is an unmerged #164 tooling correction, not an engine bug.

## #164 provider adapter contract

A new offline API-shape test requires tag search, explicit provider/license filters,
normalization of Poly Haven and ambientCG PBR channels, author/license URLs, asset
revision identifiers and exact downloaded SHA256 pins. It fails because the
provider module is absent (sketch-providers-before.log). Primary provider license
and API docs were checked; include Poly Haven's API service credit in CREDITS.
The two requested libraries are the only enabled network providers.

## #164 pinned fetch/cook passes; Blender verified

The offline pin/cache contract passes (sketch-fetch.log): complete PBR source
channels are retained, preparation reuses the production cooker's linear/normal
mipmap filtering when a texture budget requires reduction, and ASMAT/KTX2 outputs
come from tools/cook. Repeated kits are byte-identical and corrupted cached files
fail SHA256 verification. AO/displacement sources remain in the kit; the current
material runtime has no such channels, so their retention does not claim rendering.
No partial kit is published after a failed cook or manifest check. Existing output
is reused only after validation and an identical lock; another lock needs a fresh
output directory. Next implement real Poly Haven/ambientCG discovery and pins.

Blender 5.0.1 finished downloading from the official RWTH mirror. Archive SHA256
8019580ee1b7262e505f4196a00237ccf743c88d205b38d34201510676e60b09 passed before
extraction/execution. Binary:
/home/matt/.cache/aftershock-modernization/blender-5.0.1/blender-5.0.1-linux-x64/blender
reports version 5.0.1 / build a3db93c5b259. No local system packages installed.

## #164 pinned theme fetch contract

The next test uses owned pixel inputs in a private content-hash cache and a pinned
Poly-Haven-shaped lock. It requires offline fetch, retention of complete PBR source
channels, production ASMAT cooking, valid manifest/CREDITS, byte-identical repeated
kits and refusal of corrupted cache content. It fails first because fetch is not
implemented (sketch-fetch-before.log). Provider adapters must produce this same
lock contract; cache content is verified even on offline hits.

## #164 theme-license enforcement passes

The initial manifest validator passes all required negative controls and emits
CREDITS from validated entries (sketch-license.log). Source/cooked files must have
matching SHA256 records; every kit file is covered, with metadata files explicitly
excluded. Policy lives in the repository, not in untrusted kit data: CC0-1.0 only,
no image generators, and only explicitly requested Blender 5.0.1 for procedural
art with script/parameter hashes. Fetch/search and real provider integration are
next; no external art has been imported. Blender's verified archive is still
downloading into user cache; do not claim installation before its hash passes.

## #164 theme-license contract

Before fetching art, a new test requires tools/assets validate to enforce complete
file coverage, source URL/author/license/retrieval metadata, original and cooked
SHA256 hashes and generated CREDITS. Controls reject an unlisted file, CC-BY without
an explicit maintainer allowlist, incorrect hashes and an unapproved image
provider. It fails first because the tool does not exist (sketch-license-before.log).
Default external asset license is CC0-1.0; no new non-CC0 permission is inferred.

## #164 unread-mark accounting passes

Every remaining known-color component now becomes a numbered unread mark with
confidence, measured line segments, nearest geometry and an explicit assumption.
Unknown ink also receives provisional annotated regions instead of disappearing.
Agent-read route endpoints account for their arrowheads; explicit annotations and
geometry regions remain separate. The new negative control and original tracing
contract pass (sketch-marks.log). Unread semantic marks are not compiled into
geometry or silently turned into gameplay assertions. Remaining scope includes
complete line/curve/split inference, theme/license tooling and full integration.

## #164 unmatched-mark negative control

The next tracing test adds a known-color intent circle and annotation without
agent associations. It fails because those marks were silently omitted despite
the color being recognized (sketch-marks-before.log). Every remaining component
must receive a numbered unread mark and an assumption; an authoritative key is
not evidence that all text/arrows have been interpreted. Fix this shared marking
pass before extending curve/line measurement and the build pipeline.

## #164 initial tracing checkpoint; #163 integration accepted

The original playbook test passes with pinned OpenCV color separation, stroke-gap
closing, contour measurement/rectangle regularization, explicit agent-read marks,
class overlays, uncertainty assumptions and stable per-ID edits. The drawing's
own key overrides any global convention. The tool emits draft geometry and intent
records, not a claim of general handwriting recognition. Next: account for every
unmatched mark, line/arrow geometry and dominant-angle/curve measurements, connect
intent/schema/build validation and theme assets, then full runtime acceptance.

#163 is fully accepted on merged main ee4e95fc: 35571273115 passes all 16 compiler
jobs and actual create-testing publication; 35571273155 passes all ten required
active regression variants. Six archives are published in this repository only.
#163 is closed and checked in #25 (issue comment 5756883521). Nothing is pending
for #163. Continue #164 PR167, then #161 and the remaining roadmap.

## #164 sketch interpretation contract

The next test draws an original multicolor playbook in its private scratch root:
wobbly/gapped red building outlines and hatching, blue route/sight arrows, green
labels and a key. Notes contain the agent's explicit readings/confidence. It
requires interpretation.json, numbered class overlays, geometry/annotation/intent
separation, authoritative per-drawing key changes, intent records, uncertainty
assumptions and stable building_7 overrides. It fails before implementation because
tools/level trace is absent (sketch-trace-before.log). The tracer measures the
agent's reading; it must not claim to recognize arbitrary handwriting itself.

## #164 sunken geometry and edit isolation pass

Stable source-ID MAP labels prove that changing building_7 floors/windows leaves
every other shape's brush group unchanged. A sunken zone with a descending ramp
now passes clearance, leak detection and BSP/AAS compilation. The full compiler
caught a leak at an unnecessary retaining-wall cut; retaining the complete ring
seals it while the ramp's high end meets its top. No engine bug or engine change.
All repeated v2 and accepted v1 comparisons pass (sketch-sunken.log). The retained
owned debug MAP/log/leak evidence is under sketch-sunken-debug in the user cache.
No game packages were copied. Next implement image interpretation/tracing and
licensed theme assets, then the integrated agent/runtime/overhead acceptance.

## #164 stable-edit / sunken-zone contract

A new test requires source-ID-labelled MAP brushes, confirms that a building_7
storey/window edit changes only that shape, and requires a ramp into a sunken
floor without the original ground sealing it. It first fails on absent source-ID
labels (sketch-sunken-before.log). #163 merged publication is confirmed: repository
prerelease build-ee4e95fc contains six platform archives. Its regression is pending.

## #164 connected-storey compile passes

The ground/second-floor/roof spawn reachability contract now passes, including
repeated street-facing window rules and compiled BSP/AAS output. Interior stairs
reuse the transition generator with 16-unit slabs: an initial solid support
blocked stacked-flight headroom and was caught by the new clearance test, then
fixed before acceptance. Floor/roof slabs subtract the same stairwell. All v1
fixtures remain unchanged. Log: sketch-storeys.log in the user cache. Next add
sunken zones and stable per-shape source output, then trace/interpretation.

## #164 connected-storey contract

Draft PR167 now holds #164. A new contract requires navigation from ground to the
second floor and roof, plus repeated street-facing windows. It fails first on
unsupported roof_access/opening_rules fields (sketch-storeys-before.log). Reuse
the physical stair generator for interior switchbacks and subtract stairwells
from floor/roof slabs; no isolated upper-floor geometry counts as completion.
Merged #163 build/publication 35571273115 passes; regression 35571273155 still runs.

## #164 schema and compiled elevation checkpoint

V2 now has a structural JSON Schema exposed through the existing agent format
commands, without loading Shapely for v1/schema-only callers. The full polygon
contract passes with terrace stairs/ramp, a player-clipping fence shader and an
overhead canopy; repeated base/elevated BSP/AAS builds succeed. The runtime job
now executes that contract after installing pinned level requirements. All v1
language controls and original MAP/BSP/AAS fixture bytes remain unchanged.
Schema/production-loader agreement passes; its first invocation lacked Go on PATH,
then passed with the existing cached Go toolchain. No toolchain was installed.
Logs: sketch-elevation-compile.log, sketch-v1.log, sketch-formats.log in user cache.
Next: connected multi-floor shell/roof access, rule-based openings and sunken
zones, followed by the sketch/theme/intent pipeline. PR remains incomplete.

## #163 merged; #164 transition draft

PR166 merged into main as ee4e95fcdd21c6a67cdac2372a931d8f0ef1ad24 after final
head 9db00c0c passed all 16 build / ten required regression jobs. Current main
was rechecked immediately before merge and had not advanced. Merge and tested
head trees both equal fe1c5fd9e67fee7ba0894386c6f3508d2552c57f. Merged build
35571273115 and regression 35571273155 are now queued; verify publication too.
Known-good object/target remain unchanged. #163 can be checked in #25 after
those integration gates pass. Merge main forward into this #164 branch now.

The elevation MAP test passes with generated physical stair steps and ramp planes,
separate non-bullet-solid fence material and overheads. Navigation samples square
player clearance on all surfaces, retaining multiple floors at the same XY.
This draft still needs its full compile pass and schema integration; no #164
acceptance is claimed. Log: sketch-elevation.log in the user cache.

## #164 elevation contract

The next contract adds a terrace reached by stairs/ramp, bullet-transparent fence
material, and an overhead canopy. It currently fails on disconnected navigation,
as expected before transition generation. The failing log is
/home/matt/.cache/aftershock-modernization/sketch-elevation-before.log. Implement
physical steps/slopes and clearance traversal before treating this slice as done.

## #164 initial polygon compiler

Pinned Shapely performs boundary-hole triangulation and shell/opening subtraction.
The new v2 generator handles rotated rectangles, circles/arcs, thick paths and
concave polygons; v1 retains its original generator. Material resolution is shared.
Physical MAP plane occupancy checks pass, as do two identical pinned BSP/AAS
compilations and byte comparisons against every original v1 level fixture.
Logs: sketch-polygons.log and sketch-polygons-compile.log in the user cache.
This is partial implementation, not #164 acceptance: formal v2 schema integration,
elevation/connected floors and the sketch/theme/intent pipeline remain unfinished.


## #163 full concurrency acceptance

Both c56bde87 full-suite processes exited 0, with full=true, ok=true, dirty=false
and ten successful jobs each. Sequential step time totals are 1724.5s / 1723.7s;
the runs started simultaneously and shared only immutable/pinned tool caches.
Report SHA-256 values:

- A: 5e0bf78d8038a3f0d3f6e2c0484e9187b9e5c2ca13fbee0e4868f9b7ad72b045
- B: 639c3966d41184ec502f3cef6f28e158d7079fbc0318e9dade78f8e86d40b741

The runtime recipe steps pass after installing their prerequisites; no system
packages were installed locally. GCC/clang, sanitizer, lifetime, boundary, shader,
unchanged golden, headless runtime, cross-build and match lifecycle gates all ran.
All original accepted goldens and the immutable known-good tag remain unchanged.
Self-review is recorded below. Final documentation-only head still needs its own
hosted checks before readiness/merge; a passing older head is not substituted.

## #163 corrected full runs started

Both completed 7342518d runs passed format, tidy, lifetimes, sanitizers, both cross
builds and simultaneous private-kind match lifecycle, but each failed both unit
variants and runtime on the duplicate interpreter path. Corrected independent
GCC and clang job runs pass (agent-suite-python-gcc.log and clang result in
agent-suite-python-unit/suite-report.json); these are explicitly partial runs.

Fresh complete runs started together in clean detached worktrees at c56bde87.
Their reports and logs are under agent-suite-{a,b}-c56bde87. PR166 remains draft;
monitor these and fresh hosted gates, record acceptance, then recheck current main
and known-good before readiness/merge. No accepted golden changed.

## #163 local runner correction and hosted prerequisite order

The interpreter execution contract now passes: only bare python3 tokens are
replaced, leaving selected absolute interpreter paths intact. Both corrected
unit variants get through cooking; clang's complete local job passes. The GCC
spot run omitted the documented --glslang argument and failed its pinned compiler
check; rerun with the installed 16.6.0 path, without altering shader references.

Hosted 7342518d passes every active job except runtime's handbook step, which
runs before the existing level requirements installation and lacks libarchive-c
(agent-recipes-ci.log). Move that step after the headless level checks; the same
commands then have their prerequisites. No engine change is needed. Earlier
concurrent runs also pass both cross builds and are completing private kind;
three jobs in each failed on the already reproduced interpreter substitution.
Fresh complete concurrent/hosted runs are still required; PR166 remains draft.

## #163 full-run runner failure and reproducer

Both simultaneous clean 7342518d runs pass format, tidy, lifetime analysis and
sanitizers, but both unit variants fail when the local runner transforms the
cooker Python command into /usr/bin//usr/bin/python3. Their remaining jobs continue
for diagnostics; these failed suites are not acceptance. The new suite contract
executes the rendered Python command and fails first (agent-suite-python-before.log).
Limit bare-python substitution so an already selected absolute interpreter path
is not rewritten. Fresh full concurrent runs remain required after correction.

Hosted older 2626372d runtime also failed its handbook step on system Pillow,
confirming the selected-interpreter defect already fixed in 7342518d
(agent-old-recipes-ci.log). Current 7342518d has all 16 compiler legs green;
its latest hosted runtime remains pending. No red/skipped required check is accepted.

## #163 final self-review checkpoint

The client uses one request deadline across events, and recipe subprocesses inherit
the selected Python environment. All seven recipes pass from a private venv
(agent-recipe-python.log); OA seeded play and the CLI route/fire/capture/error
controls pass (agent-reviewed-play.log / agent-reviewed-cli.log).

Self-review: changes match #163; channel/PNG/assertion hooks are development-only;
input uses normal usercmds; simulation FP expressions and accepted goldens remain
unchanged; no core non-trivial lifetime or per-frame allocation is added; platform
owns pipe I/O. Shared UI commands and typed queries replace pixel-click tests.
Authoring effect support remains explicitly schema-only until #161.

The two 2626372d local runs passed format/tidy and were stopped during lifetimes
because review changed their head. They are not acceptance. Restart both complete
runs on the reviewed head in clean worktrees, retaining these earlier logs. Main
and immutable known-good tag were rechecked unchanged; repeat that check before
merge. Fresh hosted gates and the two final full passes remain required.

## #163 pinned recipe interpreter control

The recipe harness launched through a private venv fails the new prefix check:
its shell commands fall back to system python3 (agent-recipe-python-before.log).
Prepend the selected interpreter's bin directory to the recipe PATH so hosted
recipes inherit the same pinned cooker environment. The client total-deadline
control now passes with the ordinary stdout/stderr launcher contract.

## #163 client deadline test-first checkpoint

The client contract proves that queued events currently restart request timeout
instead of consuming its total budget (agent-client-deadline-before.log). Use one
monotonic deadline per request. Both concurrent baseline runs have now passed
format and tidy and are in lifetime analysis; latest hosted 2626372d compiler
workflow 35565832968 passes. No later head has final full-suite acceptance yet.

## #163 critical-event correction verified

The shared event function now flushes a full queue before accepting either error
or assert. Both GCC and clang/libc++ controls pass with exactly one dropped warning
and the critical event retained; release assertion objects still match. Pending
concurrency runs use the previous head and are not final acceptance.

## #163 concurrent runs and shared critical-event review

Two clean detached worktrees agent-suite-a and agent-suite-b are running the full
ten-variant suite concurrently at 2626372d, with separate roots ending
agent-suite-{a,b}-2626372d. Both format jobs pass; static analysis is running.
No accepted golden diff exists and known-good tag object/target remain unchanged.

Review of every critical-event caller found the same full-queue loss for engine
errors. The new error control fails first (agent-error-queue-before.log). Move
queue flushing into the shared error/assert path rather than treating assertions
specially. Fresh final-head concurrency acceptance remains required after this
correction; current runs are useful prerequisite/coverage evidence only.

## #163 pre-concurrency review checkpoint

Fatal assertions now flush the pending event queue before their own report. GCC
and clang/libc++ full-queue controls pass, preserve one dropped-warning report,
and retain identical release assertion objects. The unused XInput click helper
and pointer-coordinate bookkeeping are removed; no active test calls click.
Formatting and diff whitespace checks pass. Next run all ten local regression
variants simultaneously in two clean worktrees, then review fresh hosted gates.

## #163 assertion delivery self-review

A new full-queue assertion control fails first: 257 queued warnings cause the
fatal assertion event to be dropped (agent-assert-queue-before.log). This is an
unmerged #163 channel defect. Flush pending events before queuing the fatal report,
then flush it before abort. Preserve the explicit overflow count and release
assertion object identity. This does not alter shipping error handling.

## #163 executable handbook checkpoint

All seven handbook recipes pass with OA development binaries (agent-recipes.log):
new weapon cook/probe, owned level compile/headless report, glTF import, effect
schema-only contract, deterministic prediction report, private Git bisect, and
local annotated tag creation after recorded green main checks. Red/skipped jobs
and existing tags fail without tag replacement. No real repository tags change.

AGENTS links to the handbook and tests reference instead of retaining its growing
command list. Format CI checks every shell block; runtime CI executes them after
building the development server in the pinned cooker environment. Workflow lint,
suite catalog and affected-selector contracts pass. Full simultaneous worktree
runs remain pending; the recipe smoke is not full-suite acceptance.

## #163 handbook test-first checkpoint

The handbook contract requires executable shell blocks for seven task recipes and
fails first because docs/agents/add-weapon.md is absent (agent-recipes-before.log).
Runtime CI will execute authoring/playtest commands with owned source fixtures and
installed OA. Git bisect/tag commands run against a private tiny Git history and
recorded API shapes, including red/skipped-check and existing-tag negative controls;
the test never mutates repository rollback tags or publishes anything.

## #163 fast-feedback selector

`python3 tests/affected.py BASE_REF` includes committed, staged, unstaged and
untracked paths. Composing prefix mappings select existing asset-independent
probes; unknown paths are named and fall back to core/boundary checks. The default
600-second total budget retains logs/report, exits 1 on failure and 2 on timeout
or remaining checks. `--list` explains commands without running. This is explicitly
partial feedback, never merge acceptance. The contract passes including process
timeout and nonzero exit controls; actual HEAD feedback passes (agent-affected.log).
Format CI executes the selector contract. Executable handbook recipes remain next.

## #163 graph metadata and affected-test checkpoint

Graph Preview/Source/Tables selection and all displayed record tables now share
loaded editor state. Queries page typed parameters/states/transitions/conditions/
events/nodes/masks/joints; no new asset parser or source reload. Q3 and OA editor
round trips pass, as do GCC and clang/libc++ protocol/assertion/release-object
checks and formatting. No accepted asset or golden changes.

The affected-test contract fails first on missing tests/affected.py
(agent-affected-before.log). Implement conservative changed-path mapping, a stated
600-second default total budget, retained logs, and explicit incomplete status.
Full suites and hosted gates remain mandatory for merges.

## #163 graph tables test-first checkpoint

The graph editor contract now checks subtab selection and every displayed table
with one-record pagination. It first fails on the rejected tab action against
185b6723 (agent-graph-table-before.log). Implement read-only typed views over the
already loaded graph; do not reload or reinterpret the authoring source.

## #163 shared cvar controls

Cvar listing now returns bounded pages with values/defaults/descriptions/flags.
The channel and UI share cvar selection and cvar/image/material filter state.
Material asset replies include each stage's presence, state bits and texture IDs.
Native protocol/assertion/release-object checks and Q3 devtools runtime pass;
OpenArena devtools and material UI runtime checks also pass. Graph table queries/subtabs
remain next, followed by affected-test selection and executable handbook recipes.

## #163 editor metadata test-first checkpoint

The extended devtools runtime check fails against the previous development binary
with unknown_operation for cvar.list (agent-metadata-before.log). Add paged cvar
metadata, shared filter/selection controls, and material stage metadata to the
existing command layer; preserve normal Cvar_Set2 permission handling. Latest
2d013965 compiler workflow 35564020843 passes; active regression legs pass except
runtime, which is still running. PR166 remains draft.

## #163 local-suite catalog checkpoint

`tests/suite.py` derives all ten active regression job variants and their actual
test commands from regression.yml. Local prerequisite installation is omitted;
tests are retained, conditional matrix legs are selected explicitly, and commands
are parsed as Bash before acceptance by suite_contract.py. Each job gets its own
root and per-step logs/report. Optional --job filtering marks full=false. Timeouts
terminate the step's process group. The local format job passes through this
runner (agent-suite-format.log); no full concurrent-run claim is made yet.

## #163 full-suite runner test-first checkpoint

The local suite catalog contract fails first because tests/suite.py is absent
(agent-suite-before.log). Reuse the active regression workflow's actual commands
and ten job variants, with local preinstalled prerequisites and isolated roots,
rather than maintaining a second divergent test list. The catalog must retain all
test commands. Use it for the required simultaneous runs in two worktrees after
remaining feature edits are complete; hosted compiler gates still remain required.

## #163 schema CI correction

Schema head 84da36a8 build 35563076825 passes. Runtime 35563076817 fails in
schema-to-animation parity because its early system Pillow lacks ImageMath's
lambda_eval. Run that contract in the existing pinned cooker venv after installing
its requirements. The local pinned-Pillow contract passes; this is a CI dependency
ordering correction. No engine or source-data change is needed. Fresh hosted gates
remain required; the failed runtime is not acceptance.

## #163 isolation migration in progress

The scratch root helper passes concurrent creation, inherited temporary paths,
explicit roots with spaces and exclusive pinned-tool installation. Fixed defaults
in 61 Python test modules now derive from it. CI exports one root per job; test
subprocesses share it. Mutable cooker builds also live beneath that root, while
pinned tool installs are serialized before reuse. The new check_isolation gate
rejects active code's fixed temporary paths and tests its negative control.

Local Compose now chooses private project names and Docker-assigned published
ports; explicit --port remains available. Kubernetes generation accepts/chooses a
private namespace and its acceptance driver passes that namespace consistently.
Unit goldens/negative control, complete cooker, OA CLI, formatting and workflow
lint pass (agent-isolation-*.log). The local private kind acceptance passes
(agent-isolation-kind.log): allocation, native player, final ingest and replacement. All legacy accepted artifacts
remain unchanged. The two-full-suite concurrency acceptance and handbook remain;
this is not final #163 acceptance.

## #163 isolation test-first checkpoint

The shared scratch-root contract fails first because tools.scratch does not yet
exist (agent-isolation-before.log). It requires separate roots for concurrent
invocations and inherited temporary directories below an explicitly supplied root,
including paths with spaces. Next replace fixed test outputs, preserve intentional
within-suite build reuse through one exported root, and verify simultaneous suites.
Xvfb already uses automatic displays and loopback tests mostly allocate OS ports;
audit remaining defaults and writable shared build caches as part of this change.

## #163 authored-schema checkpoint

`tools/agent describe` emits schemas/examples for level, weapon, animation,
material, effect and match-spec. `validate` reports file/JSON-path/type/range/hint
errors. Structural/range boundaries are checked against production Python loaders
and the actual Go match-spec source; existing owned sources validate. The cooker
and level tools use these schemas before compiling, retain semantic/resource
checks, and emit structured diagnostics. The watcher test now checks the error
object and verifies the previous published revision remains intact.

Effects are explicitly authoring-only until #161 supplies the runtime/cooker;
this is a schema contract, not a claim that effects render. Material recipes
still choose legacy/PBR. Schemas cannot encode resource relationships or all
cross-field geometry checks; the real loaders retain that responsibility.

Local agent-formats, cooker, weapon, animation, PBR material, level and owned match
package checks pass (agent-format-*.log). The accepted MAP and owned package bytes
are unchanged. The match Docker image builds locally as aftershock-match:issue163
with the schema dependency; it is not published. CI installs the dependency in
each consuming environment and runs the six-format/loader check. Changes to the
schema source participate in the cook recipe hash. Fresh full hosted gates remain
required before readiness; no fixture regeneration or shipping engine changes.

## #163 schemas test-first checkpoint

The six-format describe/validate contract fails first on the missing describe
subcommand (agent-formats-before.log). It requires a valid JSON Schema and minimal
example for each kind, plus a deliberately invalid file reported with its file,
JSON path and expected type/range. Schemas will be checked against existing
loaders; semantic asset/geometry checks remain in those loaders. Effects have no
runtime loader until #161: define an explicitly authoring-only versioned contract
here and keep runtime/cooker support in #161, without claiming effects render.

## #163 CLI and gameplay checkpoint

The CLI now builds a Debug developer client when --binary is omitted, validates
playtest JSON before launch, walks bounded waypoints through normal input, tracks
live targets while firing, captures PNGs and checks hit/kill/error/assert/warning/
p99 limits. It retains report.json and engine.log on failure. Its JSON Schema
uses the existing distribution jsonschema package; hosted runtime installs it.
The example and tests share tools/agent/examples/playtest.json. Q3 and OA pass
three captures, route displacement, real moving-target hits and both negative
controls (agent-cli.log / agent-cli-openarena.log). The exact one-command default
build also passes in a fresh directory (agent-cli-built.log and report.json).

The permanent playthrough additionally shoots a paused ordinary bot using normal
rail damage/death; Q3 module and OA static runs confirm correlated hit/kill actor,
target, frame and time (agent-hit-modules.log / agent-hit-openarena.log). OA's
spawn has an obstruction on the +X side; its encounter setup teleports to the
clear -X side instead. No simulation expressions change. CI runs the CLI on its
existing developer build. actionlint reports only the three pre-existing
matrix.cc references in non-matrix cache keys; this checkpoint does not claim a
clean actionlint run. Full new-head hosted acceptance remains pending.

## #163 CLI test-first checkpoint

The one-command driver contract now requires a walked waypoint, tracked target
fire, hit/error/assert/p99 assertions, three real PNGs, a retained failure report,
and file/JSON-path diagnostics. Before implementation it fails because tools/agent
has no __main__ (agent-cli-before.log). Gameplay event acceptance independently
confirms normal rail damage/death on Q3; the OA encounter setup still needs a
clear firing position. No engine behavior or accepted fixtures are changed.

## #163 assertion checkpoint

The test-first native assertion contract failed with no event before abort
(agent-assert-before.log). Development debug Q_ASSERT now reports through a
trivial callback before standard assert aborts. Renderer modules receive that
callback through their existing import table (development ABI 20; shipping 15
unchanged). No allocation or OS access is added. GCC and Clang/libc++ verify one
flushed event and SIGABRT, and compare release object bytes against standard
assert exactly. The tidy controls use the actual macro and reject side effects.
Full tidy passes 1,286 configurations (agent-tidy.log). Debug seeded playthroughs
pass with static and module renderers (agent-debug-play.log and
agent-debug-modules-play.log). No accepted fixtures change.

## #163 character reload / telemetry checkpoint

The last pixel-click driver, cook_runtime.py, now uses explicit frames, shared
animation controls and structured asset reload/memory counters. It preserves
watched texture-to-render latency below one second, changed model pose at the
same handle/frame, renamed clip, repeated material reloads without memory growth,
idle UI allocation checks and video restart. Captures sample the reported preview
rectangle with prior pixel thresholds. Q3/OA static and OA module runs pass
(agent-cook.log, agent-cook-openarena.log, agent-cook-modules.log).

Profile replies now include GPU scopes, all tagged/hunk memory and remaining
bounded network telemetry. GCC -Wshadow and Clang/libc++ command probes pass.
MSVC's conservative analysis on f7c3d616 flagged registry records behind the
kind-selected callback; explicit zero initialization resolves its C4701 warning.
The new GPU count and preview-enabled parameter also avoid shadowing. Hosted
runtime 35560464259 has advanced beyond the prior startup failure; final fresh
compiler/regression gates remain required. No screen-click calls remain in test
drivers. Dedicated channel checks are now wired after the CI server build.

## #163 material controls checkpoint

Assets queries copy the existing image/material/model registries with bounded
pagination and filtering. Selection and PBR shared/preview edits use the same
functions as ImGui. Model preview state includes its viewport rectangle.
Both material test variants pass on Q3 and OA: source metallic/roughness/normal/
emission/mask/blend/unlit changes, exact restoration, and shared/instance factor
round trips (agent-material-{ui,runtime}*.log). Original image thresholds remain;
the sample disc derives from viewport/framing. Offline recooking waits for
structured reload counters rather than wall-time guesses or console regex.

The temporary agent_panels.py contract is folded into devtools.py and
dev_world_ui.py; coverage for explicit frame selection remains in devtools.
Both native command probes, boundaries, format and the wrapper contract pass.
CI now explicitly installs Pillow for new early runtime PNG checks and runs the
seeded local playthrough on its already-built developer client. Output artifact
patterns use PNG for migrated tests. Remaining pixel input is cook_runtime.py;
netcode_runtime uses named keys, not screen clicks. Final full gates still pending.

## #163 inspector lifecycle checkpoint

Named key requests queue ordinary SE_KEY events; actual key handling, bindings
and held-key release on overlay reopen are unchanged. Structured editor state
exposes arena/allocation counters and capture/enabled flags. The rewritten
devtools test passes on Q3 and OA with panel captures, 80 allocation-free idle
frames, video restart, animation and bound-key release. Its full OA rebuild also
verifies shipping excludes UI/channel symbols and development includes them
(agent-devtools.log / agent-devtools-full.log). Native protocol GCC/Clang-libc++,
format and boundaries pass. All five original editor drivers are now migrated;
material runtime still has pixel input and needs shared material/asset controls.

Hosted runs 35559027970 and 35559673071 fail before the first channel reply.
The new diagnostic identifies the ordinary Q3 startup banner. Root cause:
Ubuntu Noble's official xvfb-run redirects its child's stderr to stdout; the
local newer script preserves them. Running the Noble script locally reproduces
the identical error (agent-xvfb-before.log). tests/agent_client.py also fails first
with a tiny wrapper reproducing that documented stream redirection. The launcher now redirects engine stderr from inside the wrapper, with quoted
arguments and append-only logs. The permanent wrapper test passes, as does the
full seeded playthrough under the actual Noble script (agent-xvfb.log). This
corrects the launch integration; strict JSON validation stays enabled. The unit
CI legs now run the wrapper contract too. Official source inspected:
https://git.launchpad.net/ubuntu/+source/xorg-server/plain/debian/local/xvfb-run?h=ubuntu/noble

Build 35559673078 catches MSVC int-to-float conversions in the graph/animation
play ternaries. Use explicit float literals. These hosted heads remain unaccepted.
Material UI and recook variants now pass locally, including normal-map response.
The migrated driver waits for structured reload counters after offline cooking.
Its sampled disc derives from the reported viewport and preview FOV/framing,
retaining the original pixel-difference and exact-round-trip checks. OA validation also passes for both material variants; the material commands are
ready for their implementation checkpoint. Continue the remaining cooker test.

## #163 Range and actor-state checkpoint

Range actions now share the existing queued control path with ImGui, including
inspect/target/slot/fire/reload/melee/offhand/ADS/attachments/restart/capture.
The actor query uses optional native-game read callbacks: both hands' complete
weapon state, selected definition/attachments and graph state, plus both body/rig
animation states. Responses identify server authority; inactive records are null.
These read-only copies are development-only; no simulation expressions change.

Q3 and OA pass the rewritten Range test: panel render, existing moving target,
data-defined second slot, spent ammo, active/completed reload, ADS and animation
state (agent-range*.log). GCC/Clang-libc++ UBSan protocol, boundaries and format
pass. Hosted panel head 5dfdf535 caught MSVC C4456 for nested numeric locals;
renaming the request-id parse result removes that shadow. A fresh full build is
required; this failed head is not accepted. The test probe also gained the new
read-only view-client stub. CI retains Range output explicitly until the global
scratch migration, matching the other migrated editor tests.

## #163 world controls checkpoint

World/entity placement, crosshair picking, selection and reload now use shared
functions; the picker and simulation math are unchanged. Q3/OA World checks pass
with visible lines and labels and 640x480 PNGs (agent-world*.log). Entity reload
passes using its dedicated shared action. Native GCC/Clang-libc++ UBSan,
boundaries and formatting pass. CI explicitly supplies its existing retained
output locations for the migrated tests, preserving dependent build reuse until
the required workflow-wide scratch migration; PNG artifact patterns replace TGA.
No accepted image goldens are changed. Next: Range/remaining panel actions and
complete runtime queries, then finish #163 isolation/tools/schemas/recipes.

## #163 entity test checkpoint

tests/dev_entities.py now uses channel requests and structured entity fields.
Q3 and OA pass (agent-entity-reload*.log), including all original/unknown map keys,
spawn/edit/delete, angle/angles aliases, numbered saves and reload equivalence.
The World panel rewrite is test-first: shared placement, crosshair picking,
selection, wireframes and projected labels; absent entity.at_camera fails before
implementation (agent-world-before.log). No fixed screen coordinates remain in
that rewritten test. The earlier 5af3024b hosted regression has passed nine
required jobs; runtime is still running, and none is accepted as final #163 gates.

## #163 graph editor checkpoint

Graph commands now share load/source/text/save/undo/play/reset/parameter functions
with ImGui and the existing developer console command. IO stays queued inside
Com_Frame, outside ImGui calls. Structured graph state includes preview state,
event, time, dirty/play flags and stable IO result codes, avoiding log parsing.
The request limit is 512 KiB so the existing 65535-byte source editor fits even
when JSON escaping expands each source byte; buffers remain static and bounded.
No game simulation arithmetic or accepted fixture changes.

The rewritten tests/animation_editor.py passes for Q3 and OA, preserving the
original source backup, watched cook revision and changed ADS preview. The PNG
was visually reviewed. GCC/Clang-libc++ UBSan protocol, boundaries and formatting
pass. Original click coordinates, console regex and fixed output default are
removed from this test; legacy tests/panels remain to migrate.

## #163 shared panel checkpoint

World and Animation controls now share bounded functions between the panel and
JSON channel. Asset loading and world rebuilding remain queued inside the normal
frame, outside ImGui calls. Structured editor state reports the selected panel,
rendered frames/lines/labels, world flags and animation model/frame/previews.
The pre-implementation panel test failed on missing panel; it now passes on Q3
and OA without X11 clicks. Native GCC and Clang/libc++ UBSan contracts, boundaries
and formatting pass. This is the first panel slice; Graph, Range, asset controls
and the existing five click-driven test rewrites remain required.

## #163 raw input / camera checkpoint

Raw commands accept bounded movement/buttons/weapon/16-bit angles while the
normal usercmd path owns serverTime. The same seeded snapshots remain identical.
Camera pose control overrides cgame view construction before entity presentation,
and the local server snapshot uses that camera for visibility/relevance. The
player's simulation position is unchanged; switching back restores its view.
Renderer area bits are computed for the camera immediately, even before the next
snapshot arrives. Q3 and OA extended playthroughs pass
(agent-camera.log and agent-camera-openarena.log). Exact camera assertions use
integer coordinates so the test does not compare Python float64 addition with
expected engine float32 rounding. No existing simulation expression is rewritten.

The GCC 14 CI probe exposed an argument modified after setjmp in Com_Frame. The
new mode now uses a separate immutable condition, leaving noDelay untouched.
The developer-data reproducer passes (agent-devtools-data.log), as do the current
Clang/libc++ UBSan protocol, boundaries and formatting. Fresh hosted gates remain.

## #163 structured events / compiler checkpoint

Native error, renderer-warning, applied-damage and player-death hooks feed a
bounded event queue only in agent mode; subscribers receive JSON events between
request/reply messages. Overflow is explicitly reported. A Com_Error fails the
pending step and stops its remaining frames; fatal errors flush events before
shutdown. The ordinary developer `error drop` test emits exactly one error event,
fails the step with engine_error, and leaves the process queryable with no active
player. This test requires +set developer 1 because that ordinary command is
registered only in developer mode. No new error-injection command was added.

PNG/TGA images captured in the same frame match pixel-for-pixel. The full Q3
playthrough, Clang/libc++ UBSan protocol, boundaries and format pass
(agent-events.log). The raw-input/camera extension fails first on missing usercmd
(agent-camera-before.log). It requires direct command fields and a pose override
that restores the player camera. Hit/kill hooks still need gameplay acceptance; assert
events remain to implement. Draft PR166 remains incomplete. Portable conversion
head 5691795f passes all 16 compiler jobs in build 35557138685; its regression
35557139029 remains running. Earlier build 35556998279 is not accepted.

## #163 PNG capture checkpoint

The capture command schedules the existing renderer readback and returns a
structured relative path. Two explicit frames complete a populated 640x480 PNG,
validated by Pillow in the full deterministic playthrough (agent-capture.log).
The native writer reuses the engine's CRC helper and emits stored DEFLATE, avoiding
an added encoder dependency. It allocates temporary hunk memory only for an
explicit capture and is excluded from shipping builds. PNG captures refuse
existing output names. GCC/Clang protocol, boundaries and formatting pass.
The macOS --agent argument removal now preserves argv[0] for bundle discovery.
OpenArena also passes the full play/entity/profile/PNG test
(agent-capture-openarena.log). The native command contract is now in both CI
unit compiler legs; build Debug legs already compile developer tools on all
platforms. Draft PR #166 is open at 20c92d52. Initial build 35556998279 caught libc++
floating-point from_chars availability and MSVC width diagnostics in the
previously unused json.h implementation. Correct those integration issues and
rerun all gates. The dispatcher now uses the existing bounded JSON numeric
helper; explicit safe length conversions remove MSVC narrowing diagnostics.
Native protocol passes GCC and Clang/libc++ locally. Keep the PR draft
until every remaining acceptance requirement and final gate is complete.
The next error-stream assertion fails first on missing subscribe
(agent-events-before.log); the new PNG/TGA pixel-equivalence assertion already
passes. Next: camera poses/raw usercmds/events and shared UI commands, then the remaining
#163 tools/schemas/isolation/recipes and final full gates.

## #163 entity and profiler checkpoint

The playthrough now also spawns, edits, reads, lists and deletes an entity through
the native game's existing developer callbacks. It reads frame-time p50/p95/p99,
CPU scopes and network counters as JSON. Frame samples use real microseconds in
a bounded 4096-entry ring; queries sort a POD copy. Mutating entity replies reserve
capacity before invoking callbacks, lists page at 32 entries. GCC/Clang UBSan
protocol and the complete two-run Q3 playthrough pass (agent-entities.log).
The PNG extension fails first on unknown capture operation
(agent-capture-before.log); it will decode the result using Pillow and require
a populated 640x480 PNG. Next: capture/camera, raw commands and structured events, then shared panel
controls and the remaining #163 tooling/CI requirements.

## #163 first deterministic local playthrough

Map loading, structured player/camera snapshots and high-level input are wired.
Input fills the usual client usercmd immediately after normal command creation;
server movement/prediction arithmetic is unchanged. Renderer time follows the
explicit clock while profiler time stays real. Explicit frames also service the
queued packets normally sent during the wait loop, and their rate timestamps use
the same agent clock. This corrected an incomplete new step loop: the first
playthrough stalled after handshake because queued fragments were never sent.

The entity/profile extension is test-first: agent-entities-before.log records
unknown operation entity.spawn before implementation. It specifies spawn/edit/
inspect/list/delete and real frame-time percentiles plus CPU/network counters.

Two independent seed-123, dt-8 runs now return exactly identical player snapshots
before movement, after 30 movement frames, and after 30 released-input frames.
The test passes on both installed Quake 3 and OpenArena (agent-play.log and
agent-play-openarena.log); no faketime or accepted fixture changes. Dedicated
pipe stepping, GCC/Clang protocol, formatting and boundaries also pass. Next
extend commands to entities, raw input, profiling/events/captures and shared UI
actions, then complete tools/schemas/isolation/recipes and all required gates.

## #163 pipe and explicit-step checkpoint

Development builds accept --agent as the first argument. Platform code owns
blocking stdin and complete stdout writes; ordinary logs remain on stderr. EOF
quits cleanly. The parent waits for each response. A session configures dt/seed
before stepping, and step replies only after the requested Com_Frame calls.
Agent events use the explicit clock; the usual real profiler clock is retained.
The native game gets the session seed, and idle dedicated-server waiting yields
to the parent pipe in this mode. Shipping/default branches retain their behavior.

Native command GCC/Clang UBSan, dedicated pipe/idle-clock/EOF tests, boundary and
format checks pass. MinGW compiles the dispatcher and platform transport. Evidence:
agent-channel.log and agent-protocol-{gcc,clang}.log. This is not full playthrough
determinism evidence yet. The next test, tests/agent_play.py, uses the new small
Python pipe client and unique home/display to compare two seeded local-player
trajectories. It fails on unknown operation map before command implementation
(agent-play-before.log). Client/server Release development builds pass locally.
Main #14 regression now has only runtime still running; lifetimes passed.

## #163 initial command implementation

The development-only dispatcher uses the existing JSON member helpers after a
bounded syntax/depth check, decodes escaped strings, and writes correlated JSON
into the caller's buffer. Cvar updates respect startup/read-only/cheat/developer
flags and use Cvar_Set2 without forcing; exec queues the ordinary console path.
An insufficient reply buffer prevents mutation. No OS calls or new allocations.
The existing json.h utility is explicitly recognized as a shared public header
by the include boundary gate. Native protocol tests pass GCC and Clang/UBSan.
The test's Cvar_Flags stub now matches the production unsigned signature.
The next test, tests/agent_channel.py, launches a private-home dedicated server
over pipes, requests a seed/dt and two frame batches separated by wall-clock
sleep, and checks exact engine times plus EOF shutdown. Before transport it
fails with no JSON response as expected (agent-channel-before.log).
This is only the first command slice, not #163 acceptance. Transport, stepping,
shared UI actions, tools/schemas/isolation/recipes and full gates remain.

## #14 accepted on main

PR165 was marked ready and merged 2026-09-21 after all required exact-head jobs
passed on bda5ed1f. Fresh main and PR base both equaled 782c0dbc immediately
before the merge. Merge commit 4ade5c3a9cad9cd71a04ca2641db2f74b8355774 has
parents 782c0dbc and bda5ed1f and exactly the tested tree
4088ff6eea6cb060f109a9b7e33fe9f1bd9919f2. Known-good-2026-09-20 remains unchanged.
Merged build/publication 35555156611 and regression 35555156633 both PASS:
16 compiler legs, 10 required regression jobs, and actual six-platform prerelease.
#14 is checked in #25. #163 implementation is underway as recorded above; its
initial failing command contract was d429929d. Earlier superseded 607855b9/c9dc3835 regression runs were cancelled
to free runners; neither was accepted. Fresh #25 ordering remains #163, #164,
#161, then #15 and the remaining roadmap.

## #14 final local self-review / hosted gates pending

Local verification is complete. The corrected developer module configuration
passes all point/spot/sun, alpha-mask, animated-pose and restart checks, matching
the static results (lighting-ci-module-runtime.log). A shipping-client control
contains the expected missing dev_light/dev_view diagnostics. The final Q3 demo
replay retains 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4
(lighting-final-classic.log). GCC/Clang native contracts, RHI, pinned shaders,
format, types and subsystem boundaries pass. No accepted fixture changes; all
76 shader arrays accepted on main remain byte-identical, with 12 new programs.

Self-review: all changes implement #14 lighting or its tests/tooling. New shadow
and probe paths use bounded POD storage and prewarmed pipelines; no per-frame
heap allocation, non-trivial engine-core destructors or new portable OS calls.
Light/probe/uniform layouts have assertions; the offline probe loader validates
lengths/capacities and finite bounds. No authoritative simulation, collision,
movement or snapshot arithmetic changes. No unrelated engine bug fixes. LDR
probe capture and composed-color SSAO limits are explicit and assigned to #161.

PR165 stays draft until full current-head build/regression succeed. c9dc3835
started build 35553623895 and regression 35553623871; this final documentation
checkpoint must receive fresh gates as well. Recheck main/PR base immediately
before merge, merge forward and rerun if it advances, then verify the merged tree.
Main is still 782c0dbc at this checkpoint. Known-good tags remain untouched.
Issue14 evidence: issuecomment-5754550155; subsequent final gate IDs go on the
issue and in the post-merge continuation checkpoint.

Fresh tracking issue #25 inserts #163 (agent-native interface) and #164
(sketch-to-level) immediately after #14 and before #161. Both issue specs were
read. Continue #163 -> #164 -> #161 -> #15 and the remaining recorded sequence;
the earlier direct #14 -> #161 note is superseded. #163 owns structured local
control, deterministic stepping and isolation; no speculative #161 work started.

## #14 reference GPU acceptance measurement

The committed `tests/lighting_gpu.py` reproduces the final q3dm17 run with installed
Q3 pak symlinks only. RTX 3080 Ti, driver 595.91.07, 1280x720, shadow quality 2,
half-resolution SSAO, bloom, point light plus four sun cascades, 200 warm frames,
100 samples per baseline/point/combined phase. Camera (488,1096,416), angles
(15,270,0); point (488,1096,512), radius 768, RGB (1,.8,.5), intensity 3. GPU clock
is real; simulation uses the existing fixed tick. Combined capture was reviewed.

Combined recorded GPU scopes: median 1.933 ms, p95 1.969 ms, below the stated
16.67 ms reference budget. Median local/sun shadow 0.259/0.345 ms (3 ms combined
budget); SSAO evaluate/filter/apply 0.155/0.070/0.054 ms (1 ms budget);
main resumed 0.620 ms (10 ms main budget). Bloom extraction/eight blur passes/blend
remain below the 1 ms allocation. All repeated blur labels are summed, not
silently overwritten. Raw JSON/logs: lighting-gpu-final/timings.json and client.log
in the persistent modernization cache. Captures stay local. These are GPU pass
intervals, excluding presentation waits, and do not predict console performance.
Reflection/directional quality is covered separately on owned assets; q3dm17
retains its original baked lightmap content. Hosted gates/self-review still remain.

## #14 hosted module configuration correction

35c68d79 regression 35552046263 completed with the module shadow round-trip test
failing; static shadow/probe checks passed. Its preceding animation-demo command
builds a shipping client (AFTERSHOCK_DEVTOOLS defaults OFF), so dev_light is absent.
The local module build that passed had developer tools ON. The workflow now
explicitly rebuilds that module client with developer tools ON after the shipping
demo check, and the shadow test rejects the game's lowercase unknown-command
message directly. Diagnostics now retain the module lighting outputs. This is
#14 test configuration, not an engine bug fix. Required skipped successors on
that failed run are not acceptance. The newer 607855b9 gates are also superseded
by this correction and the native test's formatting correction; full checks rerun.

## #14 SSAO rendered checkpoint

Owned OA scene checks pass: half resolution changes 197438 channel bytes, full
resolution 176117, half resolution with 4x MSAA+bloom 197208. Every strength toggle
and renderer restart restores its expected image exactly. Full-resolution output
was reviewed (lighting-ssao-runtime.log). GCC/Clang native graph checks cover 32
SSAO configurations; existing 36 disabled and 36 shadow configurations retain their
contracts. Fresh pinned compilation matches 88 cached programs, package
4518cf5bca779b8ddd83778b7d3e8d9feacc926c0d1a2452c2bc5900c694af71. Prior 83 arrays
are unchanged. Static and module runtime coverage is wired into hosted CI.

Local module/RHI checks now pass: Q3 module SSAO has the same channel counts and
exact restart behavior; OA module probes match smooth/rough static results.
Remaining: measure the complete reference-GPU lighting path, rerun current-main
hosted gates and self-review. SSAO currently
attenuates forward-composed scene color; HDR ambient/direct separation is #161.
No accepted classic fixture, simulation expression or external repository changes.

## #14 SSAO native work in progress

Test-first 6a40804e fails on absent graph declarations. The implementation now
passes the graph contract and unchanged 36-configuration native descriptor hash.
Two R8 targets retain sampled main depth, including MSAA/stencil, and apply before
bloom through a compatible load pass. A depth-only view keeps stencil out of the
sampled descriptor. Existing resource teardown handles restart/resize.

New rendered test initially fails against 35c68d79 with zero changed bytes
(lighting-ssao-runtime-before.log). SSAO is opt-in (r_ssao 1 half / 2 full,
requires r_fbo 1), with live radius/strength controls. Five new shader programs
append through bin2hex; all previous 83 arrays remain untouched. CMake build
passes. Rendered output/lifecycle, expanded native declarations, final GPU budget
and current-head hosted gates remain outstanding; no acceptance is claimed.

## #14 SSAO graph test-first checkpoint

The graph contract now requires retained sampled scene depth, separate occlusion
and bilateral-filter targets, and a load-only scene application pass before bloom.
It covers full/half resolution, single/multisampled depth, stencil, bloom and both
shadow configurations. The initial contract fails to compile on the absent SSAO
graph declarations; implementation follows. Default-path descriptors and accepted
images remain unchanged. Current 35c68d79 build 35552046249 passes all compiler
legs; regression 35552046263 still awaits runtime/lifetimes, so it is not acceptance.

## #14 native reflection checkpoint

The baked atlas loader validates the version/length/capacity/finite positions,
then creates map-lifetime images and prewarms additive material pipelines. Dynamic
opaque/masked PBR objects blend their strongest two spherical probe influences;
normal/roughness/metallic inputs control sampling. The existing diffuse light grid
is unchanged. r_reflectionProbes defaults to 0 and can toggle live. No frame heap
allocation or classic-shader change. New reflection program reuses the existing
PBR vertex shader; all previous 82 arrays remain identical. Pinned fresh/cache
compilation agrees on 83 programs, package
d19c0c492f5a4c341dfade123d90f9a5546f373224cc660aa3b8922dff495e0e.

Actual OA owned-scene test now changes 100501 smooth / 108624 rough dynamic-model
channel bytes, distinguishes roughness contributions and restores exactly on
both disable and renderer restart (lighting-probe-lifecycle.log). The capture was
reviewed. GCC/Clang native graph/pipeline checks, RHI checks, baker unit checks and
CMake build pass. Tests and explicit bake command are documented and wired to CI.
Limitations are recorded: linear 8-bit radiance from LDR captures, spherical
influence without parallax correction; #161 owns HDR composition/capture.
SSAO and final GPU/hosted acceptance still remain; #14 is not complete.

## #14 reflection baker / rendered test-first checkpoint

The bounded offline baker now captures six native square views and filters linear
radiance into five GGX roughness levels using deterministic Hammersley samples.
Camera-direction/constant-color/filtering/binary-format checks pass; an actual
32px OA owned-level bake also passes (lighting-probe-bake.log). The versioned
ASPROBE file holds 1..32 bounded sphere probes with packed linear RGBA atlases.
Captures use private paths and symlink installed content; nothing is uploaded.

The new native sphere acceptance test successfully bakes the owned map, then
fails as intended with zero changed reflection pixels because native loading and
sampling are absent (lighting-probe-runtime-before.log). Implement those next.
The diffuse source remains the existing q3map2 light grid; HDR capture/composition
is deferred to #161's renderer transition, not claimed by this LDR baker.

## #14 reflection bake test-first checkpoint

The new offline producer contract (tests/probes.py) currently fails because
tools/level/probes.py is absent (lighting-probes-before.log). It requires all
six engine camera directions, deterministic GGX roughness filtering in linear
radiance and a fixed versioned atlas format. This is new feature coverage; no
accepted fixture or legacy shader changes. Implement the baker, then native
probe loading/material sampling and actual owned-scene acceptance. Existing
q3map2 light-grid probes remain the dynamic-object diffuse source.

## #14 caster/lifecycle and initial GPU checkpoint

The extended owned-level runtime compares opaque, checker-cutout and empty
casters only on common receiver pixels. Cutout versus empty changes 16518 channel
bytes. Two owned skinned idle/prone poses change 7033 common receiver bytes, with
exact light/shadow round trips. The first pose test placed the model before the
camera command had settled; increasing that wait produced the intended placement
and passes. No engine change was needed. Optional renderer module plus restart
recreates the exact shadowed image, both Q3 and OA content (lighting-shadow-module.log,
lighting-shadow-animated-settled.log). The latter includes the final pose coverage.

A real-clock RTX 3080 Ti / driver 595.91.07 q3dm17 sample at 1280x720, quality 2,
200 warm frames and 100 samples: baseline main median/p95 161.120/165.632 us;
point local shadow 260.096/261.120 us, resumed main 435.200/450.560 us, summed GPU
scopes 717.520/732.640 us. Camera (488,1096,416), angles (15,270,0); light
(488,1096,512), radius 768, RGB (1,.8,.5), intensity 3. Captures were reviewed.
Persistent JSON/logs: lighting-reference-gpu/timings.json and client.log;
reproducer /tmp/aftershock-lighting-gpu.py. This is partial lighting evidence,
not full #14/SSAO/probe acceptance or a console-hardware performance claim.

Draft PR #165 at 62c2745c started build 35550910077 and regression 35550910304.
Runtime failed compiling the pinned OA C adapter because the new owned wrapper's
sceneLight_t pointer lacked a C declaration. The adapter now forward-declares
that unused opaque type; it does not import the C++ layout. Rebuilt OA classic
demos pass unchanged 17a172f7ef8899a4b9ed21d754e7af71fb44234ad281a12eeefe27f60d06eb96
(lighting-oa-classic.log). Q3 classic replay also remains unchanged 43c52e51...
(lighting-direct-classic.log). All final current-main gates remain mandatory.

## #14 receiver implementation checkpoint

Test-first a613524c caught window-sized clipping of larger atlas tiles. The
shadow-only raster branch now uses the complete atlas viewport; all tile checks
pass GCC and Clang/libc++ under UBSan. Native graph/pipeline observations, RHI,
format/type/boundary checks and the CMake client build pass. Fresh pinned shader
compilation matches all 82 cached programs and every previous 80-array byte is
unchanged. The shaders use prewarmed additive/depth-equal pipelines; new receiver
draws run before fog and bounded native light storage is reused without heap work.

Corrected actual OA image evidence (lighting-direct-full-atlas.log): point
407783 lit / 42922 shadowed channel bytes; spot 88245 / 3133; sun 425643 /
188040. All three restore the unshadowed image exactly. Captures were reviewed:
point cover projects a visible floor shadow; sun also attenuates wall/floor areas.
The earlier low occlusion counts below were incomplete-atlas evidence and are
superseded. These are feature checks, not reference-GPU performance acceptance.
Mask/animated geometry/lifecycle and remaining #14 lighting features still remain.

## #14 receiver / atlas scissor test-first checkpoint

The new receiver path adds bounded per-light forward draws before fog, with world
normal/tangent transforms, point-face selection, spot falloff, four sun cascades
and tile-clamped manual depth comparison. Two new shader programs preserve all
previous 80 arrays; pinned fresh/cache compilation matches 82 programs, package
d86e9f0063a51f5a097f18dec310c5b27659d58ca86217f8646773aa4ae6d29e.
The native additive/depth-equal pipeline check passes GCC/Clang; CMake and RHI
checks pass. The initial rendered run caught the new pass incorrectly honoring
PBR's legacy-only SURF_NODLIGHT flag. Admitting PBR materials corrected that.

OA point/spot/sun now change 407783/88245/425643 channel bytes with exact restored
unshadowed images (lighting-direct-runtime.log). However, visual/source review
found the atlas reused window-clamped scissors. These early occlusion counts are
not full-atlas acceptance. tests/shadow_views.py now checks every 4x4/2x2 tile in
an atlas larger than the window and fails at the production scissor calculation.
Apply the shadow-only correction and rerun rendered evidence before committing
the receiver implementation. Display-space additive composition is transitional;
#161 owns the single HDR/tone-map resolve. No accepted shader/reference changed.

## #14 caster submission checkpoint

Test-first rendered contract is 5cf631f3. The local client now submits new native
point/spot data through dev_light (local cheats/development only), with per-frame
copies and a 16-tile local atlas. Point cameras draw six faces, spots one, sun four
cascades. Shadow views reuse existing world and MD3/MDR/IQM geometry, include
third-person bodies but omit first-person/no-shadow/depth-hacked entities. They
bypass camera PVS and restore visibility for the main view; entity lighting and
legacy stencil/projected shadows are skipped in these new depth views.

The backend records depth-only masked draws and resumes preserved scene contents.
Screen-map duplication/scanning skips caster commands. Half the surface array is
reserved for normal scene draws; shadow overflow rolls back its queued commands
and disables that scene's new lighting instead of wrapping existing geometry.
Opaque/masked caster pipelines are warmed at startup in three culling modes.
Quality 0 (default) keeps legacy behavior; qualities 1..3 allocate 1024/2048/4096
atlases and larger uniform slots. Sun distance/split/intensity, comparison toggle
and depth bias are bounded cvars. Legacy dynamic light calls are unchanged.

The initial build needed an explicit uint32_t conversion for the conditional
uniform size. Fixed CMake build, GCC/Clang UBSan view/submission checks, developer
contracts, native descriptors and format/type/boundary checks pass. Actual OA
client loads and captures all point/spot/sun cases without a reported GPU error,
then fails expectedly with zero lit/occluded pixels; receiver shading is not yet
implemented (lighting-shadow-caster-built-runtime.log). The earlier caster-runtime
log used the old binary after the first failed build and is not evidence.
Classic Q3 replay in a separate clean build directory passes unchanged
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4
(lighting-shadow-caster-classic.log). The first default output path had another
worktree's CMake cache; no reference was changed to resolve it.

Receiver integration details: shadowLight_t owns world-to-clip matrices for up to
six faces; tr.refdef.sun owns four matrices and split distances. Matrices include
the same Vulkan Y flip as RB_GetMVP; local atlas tiles are 4x4, sun 2x2 with top-row
UV indexing. New lighting can use binding 4 (five total sets required); new direct
passes can reuse base/normal/metal texture bindings without changing accepted PBR
or legacy programs. Atlas bind is forbidden during depth passes. Complete actual
occlusion/mask and optional-module tests before acceptance; no accepted goldens
or shader bytes may be regenerated.

## #14 rendered shadow test-first checkpoint

`tests/lighting_runtime.py --shadows` reuses the owned level and native client,
with a neutral normal map/darker diffuse material. It compares point, spot and sun
lighting with occlusion disabled/enabled, requires attenuation and an exact return
to the unshadowed image. Before native light controls/caster/receiver wiring it
fails with point light changed=0, occluded=0 (lighting-shadow-runtime-before.log;
client captures in /tmp/aftershock-shadow-runtime-before). No frame baseline was
accepted or regenerated. Implement the actual draw path before rerunning it.

## #14 shadow caster shader implementation checkpoint

Test-first d33b7a6b precedes two new depth-only GLSL programs with vertex alpha,
base texture and explicit opaque/GE/LT/GT mask modes. Their native pipeline uses
single-sample reversed depth, zero color attachments and no stencil/coverage.
All previous 78 SPIR-V arrays remain an identical prefix; only bin2hex appended
new arrays. Fresh pinned compilation matches all 80 cached programs/interfaces:
2380ed012d93f6cbbaa4e14c10a2c199b1147f7c4b7fcaa1c0ff606937c700d9.
Logs lighting-shadow-{shaders,shader-native,shader-clang,shader-build}.log record
shader, native descriptor/pipeline and CMake success. Native scene caster/receiver
submission and rendered mask/occlusion acceptance still remain.

## #14 shadow caster shader test-first checkpoint

The native observation now requires a dedicated caster pipeline with position,
base UV and vertex alpha, two shadow shader modules, zero color attachments,
single-sample reversed depth, and no stencil/alpha-to-coverage. Before adding the
new program it fails on absent modules/type (lighting-shadow-shader-before.log).
Existing 78 shader arrays remain accepted and must stay byte-identical.

## #14 native light submission implementation checkpoint

Test-first 368269f5 precedes the 56-byte trivial sceneLight_t API and native client/
cgame forwarding. Renderer versions advance to 15 shipping/19 development. The
existing scene/frame storage owns copies and rejects invalid point/spot values or
exhausted 16-light/16-local-tile capacities; point lights consume six tiles, spots
one. Valid spot directions normalize once. Legacy lights/wire structs are unchanged.
GCC/Clang UBSan shadow-view/submission probes, unchanged 33-line native ABI and
native CMake build pass (lighting-scene-lights-{gcc,clang,abi,build}.log). These are
submission contracts only; actual caster/receiver recording remains next.

## #14 native light submission test-first checkpoint

The same shadow-view driver now compiles a submission probe before implementation:
it requires copied point/spot data, normalized spot direction, bounded per-frame
storage and per-scene 16-tile admission (six faces per point, one per spot), valid
cones and clean frame/scene reset. It fails on the absent scene-light contract
(lighting-scene-lights-before.log). Existing legacy lights and wire structs stay
unchanged. This prepares the native caster/receiver path, not feature acceptance.

Roadmap #25 now places #161 visual fidelity immediately after #14; follow it before
#15. #160/PR #162 repairs the independent publication YAML collision on main while
this branch continues. Do not merge #14 before its final current-main gates.

## #14 shadow sampling implementation checkpoint

Test-first b5e7d5ef precedes allocation of two atlas sampler descriptors from the
existing pool. Nearest/clamped sampling supports manual depth comparison without
requiring filterable D32. Existing resize/filter refresh updates both bindings;
invalid slots/atlases and sampling during a depth pass are rejected. GCC/Clang
native graph observation, RHI upload/alternative-backend checks, CMake build and
format/type/boundary gates pass (lighting-shadow-sampling*.log). No new shader or
accepted reference changed. Caster/receiver draw submission remains next.

## #14 shadow sampling test-first checkpoint

Extend the same native graph observation to descriptor allocation, depth-read-only
layout, nearest/clamped sampling, atlas binding and existing descriptor refresh.
The pre-implementation native probe fails on missing shadow descriptors/binding
(lighting-shadow-sampling-before.log). This remains ordinary feature coverage.

## #14 shadow recording implementation checkpoint

Test-first e2bbbf9b precedes native depth-pass recording. Shadow-enabled graph
configurations retain scene color/MSAA/depth/stencil and create compatible load
continuations; disabled native descriptors still match all 36 frozen cases.
RHI_BeginShadowPass clears exactly one reversed-depth attachment and supports
local-to-sun transition; EndShadowPass restores main/screen state, viewport scale
and dirty-depth tracking. Continuation dependencies cover color and depth writes.
Native recording observation passes all 36 enabled cases on GCC and Clang/libc++,
along with the portable graph checks (lighting-shadow-record.log). CMake builds successfully
(lighting-shadow-record-build.log). No accepted fixture or shader changed.

Format/type/boundary and alternative-backend/upload checks also pass
(lighting-shadow-record-{clang,rhi}.log).

Actual caster draw/atlas descriptors, receiver shading and full lighting acceptance
remain next. No main/default scene calls these APIs until that frontend is ready.

## #14 shadow recording test-first checkpoint

The graph/native observation now requires load/store continuation passes before
implementing shadow command recording. Mocked native calls check two depth-only
clears, main/screen-map state restoration, invalid atlas rejection and duplicate
end safety. Main, MSAA and screen depth must survive a shadow interlude. The
pre-implementation build fails on absent continuation fields/API
(lighting-shadow-record-before.log). Legacy disabled descriptors remain frozen.

## #14 shadow-view implementation checkpoint

Test-first commit 8e8252ef precedes tr_shadow.cpp. The shared spot projection also
constructs six point faces. Four sun cameras mix logarithmic/linear splits, enclose
receiver frustum spheres, snap lateral centers to texels and retain bounded
upstream caster coverage. Existing scene/FP projections are untouched. GCC and
Clang/libc++ analytical UBSan checks pass, including rotated cameras/oblique sun,
with all split mixtures; logs lighting-shadow-views-{gcc,clang}.log. Native CMake
build passes (lighting-shadow-views-build.log), as do format/type/boundary gates.
The test runs on both hosted unit compiler legs. No accepted reference changed.

This is allocation-free view construction only; it is not yet called by rendering.
Next connect caster submission and depth-pass recording, then receiver lighting.
Shadow quality, reflections/SSAO and reference-GPU acceptance remain outstanding.

## #14 shadow-view test-first checkpoint

The new tests/shadow_views.py compiles an analytical probe with UBSan. It specifies
six point faces, a spot cone, reversed near/far depth, handedness, five culling
planes, four sun-slice corner coverage, off-camera casters and a stable texel grid
under small camera motion. It fails on missing view constructors before their
implementation (lighting-shadow-views-before.log). No accepted fixture is changed.
Native pass submission and receiver shading remain outstanding after this math.

## #14 shadow resource implementation checkpoint

The graph/native resource slice builds and passes both the frozen 36 legacy
configurations and 36 new shadow-enabled configurations. Two fixed sampled D32
atlases use depth-only clear/store passes and explicit depth-test write to fragment
read dependencies. A separate possible execution order places both writers before
scene consumers while preserving legacy IDs and disabled descriptor bytes.
Power-of-two atlas dimensions are bounded to 128..8192. Attachments use the
existing pooled allocator and resize/restart teardown; no frame allocation.
Logs: lighting-shadow-native.log, lighting-shadow-build.log. Format/type/boundary
checks pass. Native configuration keeps shadowMapSize zero until caster/view and
receiver submission are implemented; this is not rendered-shadow acceptance.

## #14 initial shadow graph contract

The existing graph contract now requires two opt-in sampled depth atlases
(local lights and cascaded sun), depth-only clear/store passes, explicit depth
write-to-fragment-read dependencies and shadow writers ordered before the main
scene. Existing pass/target IDs and disabled native descriptors remain fixed.
Before implementation it fails on the absent shadowSize/LocalShadow/SunShadow
contract (lighting-shadow-graph-before.log). Atlas dimensions must be bounded
powers of two. This is a resource/dependency test, not proof of rendered shadows;
frontend caster views, receiver shading and actual visual/performance acceptance
remain mandatory.

## #14 directional runtime checkpoint

The first renderer slice builds and passes actual OA/Q3 native tests with both
separate and merged lightmaps. Reversed owned normal data changes 438,166 channel
bytes on OA and 438,096 on Q3; restoring the cvar gives exact original static
pixels. Reviewed images retain baked occlusion with normal mapping disabled and
reverse the lighting with the deliberately inverted normals. Logs:
lighting-runtime-first.log, lighting-runtime-q3.log. These are new diagnostics,
not regenerated references.

The explicit BSP marker selects paired loading and even-index remapping. Intensity
retains legacy conversion; raw direction bytes use a separate half of a combined
atlas at the existing fifth descriptor slot. Unsupported four-set hardware reports
light-grid fallback. `r_directionalLightmaps` switches the normal response live.
The new material iterator supplies existing lightmap UVs; ordinary Quake materials
retain their intensity stages. All allocation is at map load, through existing
hunk/images; frame data stays bounded POD. Two new offline programs implement a
bounded dominant-direction approximation. All previous 76 shader byte arrays are
unchanged; 78-program cache/fresh compile matches package
327bdca2c2e65c383328540d3fc28f7a6e764deac4f9c7fd53d0fcc4595f1b0e
(lighting-shaders.log).

Classic Q3 replay still matches
43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4
(lighting-classic.log). RHI/alternative-backend and all 36 frozen graph descriptor
configurations pass (lighting-rhi.log, lighting-graph.log); format/type/boundary
gates pass. CI and AGENTS/tests docs include the bake/native commands. Full final
#14 checks/self-review wait for remaining scope: point/spot and cascaded sun
shadows, reflection probes, SSAO and actual reference-GPU budgets. No #14 PR yet.

#157's main-target head f638330b passed every build leg in 35545635052.
Regression 35545635067 is still running its runtime/lifetime jobs; all other gates
passed at last check. Superseded 2ed8cf56 regression 35544991093 was cancelled;
never count it as acceptance. The rollback tag remains at 81a0f9dc.

## #14 first bake implementation

The opt-in compiler slice passes `tests/lighting.py --compile`: two independent
bakes produce identical intensity/direction pairs and retain 594 light-grid
probes. `tests/level.py --compile` still matches all accepted MAP/BSP/AAS hashes.
Logs: lighting-bake-worldspawn.log and lighting-default-level-final.log.
`compile_map` reads the explicit marker from q3map2's compiled worldspawn before
its light stage, so JSON and existing MAP entry points agree. No bake container,
ray tracer, accepted fixture or compiler dependency was added. Native direction
sampling is not implemented yet; the README's renderer description is the intended
#14 contract, not a claim of completed runtime acceptance.

Maintainer workflow changed during this slice: main 81a0f9dc is the integration
baseline, known-good-2026-09-20 remains unchanged. #157 is retargeted to main;
its updated head f638330b runs build 35545635052/regression 35545635067. Preserve
these existing issue branches by merging main forward without rebasing. New issue
branches start from main. All required gates must pass before a self-merge.

The native visual contract `tests/lighting_runtime.py` compiles the owned
opt-in level and cooks ordinary wall/floor PBR materials, then runs the actual OA
client with merged/unmerged lightmaps. Before renderer implementation it fails
because native directional pages are not recognized (lighting-runtime-before.log).
The required assertions include changed static pixels with direction mapping on
and an exact off/on/off restored view; no accepted visual reference is created.

## #14 initial failing directional-bake contract

`python3 tests/lighting.py` fails before implementation with `missing or unknown
fields: directional` (lighting-before.log). The new opt-in JSON field is
`lighting.directional: true`; disabled/omitted output must retain the accepted
MAP/BSP/AAS bytes. The full `--compile` gate will compare independent bakes,
require paired intensity/model-space direction pages referenced by even surface
indices, preserve the existing baked light-grid probes and validate AAS output.
No new fixture or reference bytes are recorded.

The pinned q3map2 2.5.17n-git-68ecbed already supports `-deluxe -deluxemode 0`.
Its lightmaps_ydnar.cpp stores direction RGB immediately after each intensity page;
light.cpp confirms the mode. Reuse that ordinary IBSP 46 representation, with an
explicit `_aftershock_deluxe` worldspawn marker. A custom lighting container or
new offline ray tracer is unnecessary. Runtime direction sampling and the rest of
#14 still need implementation and tests; this bake contract alone is not acceptance.
Source: https://github.com/Garux/netradiant-custom/blob/68ecbed/tools/quake3/q3map2/lightmaps_ydnar.cpp.

Reference acceptance target is 1280x720 at 60 Hz on the recorded RTX 3080 Ti:
16.67 ms total GPU frame, with initial budgets of 3 ms shadow maps, 1 ms SSAO,
1 ms bloom, 10 ms main scene and 1.67 ms remaining work. Measure warm-frame
median and p95 per pass, with actual shadowed q3dm17 and explicit quality values.
These are targets, not measured acceptance or console-hardware claims. Software
renderers continue to supply deterministic functional gates.

## #160 publication merge repair self-review

The pre-change actionlint run failed on both duplicate keys. Removing four lines
restores exactly the build.yml bytes tested by #159; contents-write/actions-read
remain limited to publication jobs. bash syntax, the offline publisher contract,
actionlint and CRLF-aware whitespace checking pass. Final hosted gates and actual
main publication remain required. No engine FP, ABI/layout, allocation, OS access,
destructor, accepted golden or fixture changes. The merge-base check in AGENTS
addresses the missed concurrent update without changing either contributor's
intended job permissions. No tag was moved and no main commit was pushed directly.

## #158 publication self-review

The offline contract was committed first at 7d88a53a and failed because the
publisher did not exist. It now passes creation, same-tag retry, existing-tag
collision, API/create/upload failures and repository/SHA guards. Bash syntax,
actionlint for build.yml and explicit workflow permission checks pass. Existing
CRLF in build.yml is preserved; whitespace checking uses cr-at-eol for that file.

Only publication jobs receive contents-write/actions-read. The installed gh CLI
replaces the rolling-tag action; no new dependency or external destination. Build
publication uses a build-<full SHA> prerelease inside msetaro/aftershock, verifies
any existing exact tag object and retries only archive assets. It never moves a
tag or changes the stable latest release pointer. GH_HOST is fixed to github.com.
The published archives/content are the existing engine build outputs, unchanged.

Scope is #158 CI publication only. No engine code, FP arithmetic, layouts,
allocation, destructor, OS boundary or accepted fixture is changed. The test is
wired into the regression format job and documented in AGENTS/tests README.
Required full hosted build/regression remain mandatory before merge. The actual
publication job is intentionally main-event-only; require its successful merged
run before closing #158. Main a4358019's publication failed with the same baseline
permission defect in run 35546603122; its compilation jobs all passed, and its
independent regression 35546603116 is still running. Do not conflate publication
failure with #13 engine/runtime correctness or waive either acceptance step.

## #13 preparatory failing material contract

Metallic/roughness is the selected workflow, matching glTF 2.0. The explicit model
recipe `material_model: metallic-roughness` produces ASMAT v2; recipes without it
retain the accepted v1 compatibility output. Factors remain native data for live
editing and per-instance overrides. The new 240-byte payload contains base RGBA,
emissive RGB, metallic/roughness/normal scale/mask cutoff, flags and three qpaths.
The three existing material texture bindings carry base RGBA (sRGB), normal XYZ
plus roughness A (linear), and emissive RGB plus metallic A (sRGB RGB, linear A).
Packed data alpha must not premultiply normal/emissive mips. This fits the existing
RHI bindings and avoids changing legacy shader programs. Unsupported glTF texture
transforms/additional UV sets still require offline baking.

`python3 tests/materials.py` authors an ordinary owned glTF triangle with four
source textures, checks editable factors, independent BC7 decoding, mip channels,
transitive inputs/incremental rebuilding and v1 fallback. Before implementation
it fails at the existing explicit arbitrary-mask-cutoff rejection
(materials-before.log). This is only the initial cook contract; rendered lighting,
normal mapping, instance overrides, ImGui editing and unchanged classic replay
still need acceptance. No accepted fixture, source or shader bytes changed.
Reference: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials.

The native extension compiles an ordinary-output probe before cooking. It requires
native material fields and masked per-instance parameter resolution, preserving
the shared material and all unselected values. It fails before implementation on
the absent cookedPbrMaterial_t/API (materials-native-before.log). This is a normal
feature contract, not a new loader-robustness target.

The first cooker/native data slice passes both compiler families
(materials-first.log, materials-clang.log). Normal/roughness and emissive/metallic
packing preserves factors and filters data alpha independently. Explicitly reject
unsupported material extensions, occlusion (lighting owns it), non-repeat/nonlinear
samplers, extra UVs/transforms and nonconstant mismatched channel dimensions;
these require artist baking instead of silent approximation. Standalone material
recipes use the same glTF-shaped data with texture `uri` fields. Native validation
and masked factor resolution use fixed-size trivial public values, no allocation.
The full legacy cooker suite also passes (materials-legacy.log); native runtime/ImGui acceptance remains pending.

#156 exact-head gates all passed and the merge tree is identical. Hosted kind log:
match28-host-kind.log. Integration 35543218326 passed; #28 is accepted and closed.

## #13 initial rendering checkpoint

The two new authored PBR shader programs use the existing three material texture
bindings and a 112-byte uniform block inside the unchanged 128-byte allocation.
All 74 accepted shader binaries remain byte-identical; bin2hex only appended the
two new compiled programs. Fresh pinned compilation agrees for all 76 shaders
(materials-shaders.log). Native rendering builds (materials-build.log). Classic
Q3 replay remains 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4
(materials-classic.log), with no golden/fixture regeneration.

The new path uses GGX/Smith/Schlick, editable linear factors, the existing model
light-grid/dynamic direction and ambient, authored/skinned tangents or a derivative
basis, explicit mask/blend flags and the existing fog pass. This is restrained
material shading, not #14's full lighting/shadow work. Legacy shaders keep their
existing iterator. Native static/skeletal instance submission copies bounded POD
parameters without modifying refEntity_t or wire layouts. ImGui factor edits do
not change pipeline flags; optional preview overrides are separate copies.

Type/boundary gates pass. Animation tests cover static/skeletal instance copy,
rejection without consuming an entity and per-frame reset, alongside existing
animation acceptance (materials-animation.log). The owned sphere source has an
idle clip for the existing Animation tab. First visual attempt lacked that clip;
second showed every channel changing but compared the moving map behind the
preview in its exact round trip. Restrict the sample to the sphere interior:
5,521 identical restored pixels, and independent metallic/roughness/normal/emissive/
mask/blend changes. Rerun the corrected permanent test; add actual ImGui and
instance visual controls and both content sets before acceptance. Evidence:
materials-runtime-{first,idle}.log and /tmp/aftershock-material-runtime-idle images.
These are new test captures, not accepted goldens.

## #13 visual and color review

The new shader now explicitly encodes linear PBR output for the unchanged legacy
UNORM display-space target. An unlit stage independently checks sampled RGB against
the source (180,130,50), within BC7's three-byte tolerance. Only the two unaccepted
PBR programs were updated through the generator; the 74 accepted programs remain
an unchanged byte prefix. The final 76-program cache/fresh-compile package is
71ffd19cdfc660adb9c2382c409a0eb118258746aeffd2452258dd6f5e082560
(materials-shaders-display.log). Transparency uses the existing display-space
compositor; linear HDR composition belongs to subsequent lighting work.

The permanent sphere test samples its 5,521 interior pixels, not the moving map
behind it. OpenArena now passes: metallic 5,521 changed pixels, roughness 774,
normal direction 2,389, emissive/mask/blend all 5,521, correct unlit source color,
and an exact restored opaque image (materials-runtime-normal.log). A mild normal
perturbation under OA's far-side preview sun changed too few pixels for a useful
visual control; the owned source now deliberately reverses tangent-space Z. No
engine threshold/oracle was relaxed. A full-line log barrier prevents normal_flat
from being mistaken for the later normal marker. Final Q3 source validation is
running after adding glTF POSITION/time bounds and optional extension declarations.

Real ImGui shared/instance edits pass with both content sets
(materials-ui-{unique,display}.log). The shared metallic factor changes the preview;
an independent override restores the baseline; clearing it restores the shared
edit exactly. The control selects the unique models/sphere path, avoiding similarly
named legacy pickup shaders. Shader review also preserves gl_FrontFacing semantics
for mirrored PBR geometry by adjusting only that new pipeline's winding/culling.
The optional renderer module is building/running the same actual-input test.

Unit golden and its one-ULP control pass; unit.txt remains
8d44421dfd5f31912bb7ffc942c6f0e1f32cd9a445e1dbcf38b658f555598ede.
Legacy cooker/render probes, animation/instance tests, RHI and 1,270 tidy
configurations pass. Lifetime analysis passes all 1,216 commands. CI runs the new
cooker/native contract with both compilers and both visual modes with OA. AGENTS,
tests/README and tools/cook/README document commands, v2 layout, authoring and limits.
Draft #157 is open; the MSVC correction and fresh exact-head gates are recorded above.

## #13 self-review and final local gates

Final local Q3 channel/color test passes (materials-runtime-q3-final.log), as do OA
channel/color (materials-runtime-normal.log), actual ImGui shared/instance editing
(materials-ui-display.log) and the optional renderer module's actual ImGui test
(materials-ui-module.log). Both native compiler families pass the data test,
including standalone material recipes. All 1,216 lifetime configurations and 1,270
tidy configurations pass. Final format/type/boundary checks pass. Unit golden and
negative control, legacy cooker/renderer probes, animation/instance ownership and
RHI alternative-backend checks pass. Fresh shader compilation matches the final
76-program cache. Classic Q3 replay retains its accepted hash, with no regeneration.

Scope matches #13's restrained material abstraction, offline glTF/data authoring,
instance factors and live ImGui tooling. Existing recipes stay v1, Quake scripts
keep precedence and the legacy iterator/shader bytes. Only newly authored PBR
programs use the new path; shaders compile offline. Data/native layouts are fixed
and asserted. Color factors remain linear, normal/roughness and emissive/metallic
mips keep independent data alpha, and source-color/round-trip checks cover the
UNORM display conversion. Mirrored PBR winding preserves double-sided normals.

Instances use bounded per-frame POD copies and optional existing skeletal storage;
shared factor edits leave pipeline ordering/flags unchanged. No per-frame heap
allocation, non-trivial core object, OS call, renderer-private boundary crossing,
wire/refEntity layout, existing simulation arithmetic or accepted oracle change.
Initial/new cooked assets use the existing hunk/image/pipeline lifetime paths.
Unsupported authoring combinations fail explicitly; global texture-quality settings
remain authoritative. Lighting reuses the existing dominant direction/light grid,
with world batch-center sampling; direct light/shadow upgrades and linear HDR
composition remain #14's scope. No new provider/database/system package dependency.

Tests exposed setup/measurement mistakes (clip-less Animation preview, ambiguous
UI filter, substring barriers, map-background sampling, weak normal perturbation),
all corrected in the new tests rather than changing accepted references. The new
shader's display conversion was corrected before acceptance and checked against
absolute source RGB. MSVC requires an explicit enum-to-float cast for the new ternary sort selection;
the build correction keeps the same small integer values and adds no behavior.
Full exact-head hosted gates and merged-tree regression are still mandatory. No upstream PR or other-repository write is involved.

## #28 implementation record at its tested head

The following record/self-review is inherited from PR #156; final acceptance and
current integration state are recorded in Next action above.

The combined native client/server and final image build. Direct runtime and two
private kind runs pass actual OA native player acceptance with sv_pure=1, timed
exit, final acknowledged gRPC checkpoint and a fresh Ready replacement. First
measured match pod (engine/wrapper + results + SDK) uses 44,371,968 bytes working set
and 0.0039557 vCPU over 20 seconds with one connected idle player: resource
equivalents 22.54 matches/GB and 252.8/vCPU, not saturation or worst-case capacity.
A second run measures 45,154,304 bytes / 0.0040864 vCPU. Scheduler request accounting
must also include Agones' always-running init sidecar; the first added request
report missed that field (actual CRI usage already included all three containers).
The reviewed driver counts it and explicitly reserves 32 MiB/limits 128 MiB for
SDK memory. That final driver/image run is active at /tmp/aftershock-match-kind-reviewed.

Go race/vet, owned package, native lifetime, format, direct actual-client runtime
and the new CI job lint pass. Review adds shared warm/spec validation with bounded
map names and immutable final ingest streams, both covered by Go tests. AGENTS
self-review follows. Open #28's draft PR for exact-head CI while #155 integration
finishes; no #28 acceptance/merge precedes that dependency gate. After local/hosted
final gates pass, record density/decisions in #28, ready/merge it and require its
merged-tree regression before continuing #25.

Do not regenerate accepted fixtures. Do not create upstream PRs. Keep credentials
and kubeconfig out of artifacts. All cluster tools are verified user-cache binaries;
only newly created private Docker/Compose/kind resources may be removed.

Combined #28/#155 local acceptance: /tmp/aftershock-match-combined builds both
binaries; match-runtime-fixed.log passes a real OA native player with sv_pure=1,
match-end exit and final acknowledged gRPC facts. The image rebuilt successfully
(match-image-with-pure.log). The full random-cluster driver is running at
/tmp/aftershock-match-kind-first; its private credentials are not artifacts.
Go race/vet, owned content reproducibility and formatting pass. The new match-server
job passes actionlint in isolation. Whole inherited workflow lint reports three
pre-existing matrix.cc references in non-matrix jobs; this issue leaves those
unrelated cache keys unchanged. No acceptance is claimed from that full lint run.

## #28 self-review

Scope is one native server per match, its non-root read-only owned-content image,
validated match-spec launch, opt-in game lifetime, Agones allocation/health/shutdown,
external gRPC durable facts, Compose/private kind acceptance and measured density.
The only new engine change is an opt-in POD cvar and integer intermission/exit
branches; default behavior remains unchanged. The native pure defect is inherited
only from its separately tested/merged #31 PR #155. No simulation FP, layout,
allocator, core destructor, OS boundary or accepted golden changes.

Spec fields, file/line/batch sizes, identifiers, paths and offsets have bounds.
No shell/arbitrary command execution is exposed by match specs. gRPC validates
per-match tokens; TLS defaults on, explicit plaintext is confined to development
stub examples. Module password fallback is recorded until #23/provider wiring;
there is no claim of authenticated provider identity or executable attestation.
Persistence stays in a separate process. Pending data, ACK cursor and final state
have fsync/rename and retry/restart controls; startup failure never claims a
completed match, final streams cannot be extended, and EOF/final-file-size checks
avoid truncating the last events. EmptyDir loss preserves only previously ACKed
facts, as designed. The fsync-file stub is explicitly replaced by #30 later.

All images contain only owned sample content and pinned base images; fixtures
retain exact bytes. Native client acceptance mounts complete public OA test data
only into private nodes. Fresh homes prevent match ID/cursor reuse. CI-generated
credentials/kubeconfig are excluded from artifacts. Private random cluster cleanup
and bounded tool calls cannot target a pre-existing cluster. Local Composer/kind
failures informed deployment fixes, never passing missing-content runs.

Actual native runtime, two private kind lifecycles, Go race/vet, unit lifetime,
owned content and formatting pass. Review's final warm-map/final-stream guards and
SDK request accounting are under the final local run and full exact-head CI; both
remain required before merge, alongside #155 integration. Density includes all
three match containers and distinguishes measured light-load equivalents from
scheduler requests and production capacity. No database driver or simulation-loop
persistence is added; default engine behavior and all accepted goldens stay fixed.

## #31 checkpoint inherited by #13

Current main checkout: issue/31-native-pure. #27 PR #154 merged at
6a3cb22d54a1c9575adde00c1d415639cde617f3; exact-head build 35538730616,
regression 35538730598 and merged-tree regression 35539581431 all passed.
#27 is closed and checked in #25. #28 preparation remains in level-tree.

#28's ordinary native-client connection to sv_pure=1 exposed a native-port defect:
SV_VerifyPaks_f still demands vm/cgame.qvm and vm/ui.qvm checksums, while statically
linked clients never reference those files. The valid client is rejected as
unpure before ClientBegin. This fix belongs only in a separate #31 PR. The new
filesystem probe fails before any engine edit (native-pure-before.log); first
commit that test, then retain content-pak checks while replacing obsolete QVM
slots with explicit native markers. The correction now passes GCC and Clang/libc++
filesystem tests, and actual Q3/OA clients join and chat on sv_pure=1. The same
runtime script fails against pre-fix #27 binaries (native-pure-runtime-before.log).
The probe retains the existing trailing whitespace. Format/boundary/type gates
pass; unit.txt retains 8d44421dfd5f31912bb7ffc942c6f0e1f32cd9a445e1dbcf38b658f555598ede.
CI runs the unit probe in both compilers and the real OA connection. Open the
separate fix PR, require exact-head build/regression and integration, then merge
forward into #28. No upstream changes.

## #31 native pure self-review

The only behavior change is replacing obsolete QVM pak expectations with explicit
zero-valued native module slots. Native protocol/schema agreement remains in place;
data-pak membership, duplicate detection and aggregate checksum validation are
unchanged. Pure verification is not disabled. The real-client test fails before
and passes after, with both local content sets; the filesystem test keeps actual
content checksum accounting. No golden represents the previously broken native
pure connection, and existing unit/replay goldens need no regeneration.

No simulation/floating-point arithmetic, wire/file structs, OS ownership, allocation,
non-trivial lifetime or public subsystem include changes. The list uses the existing
static buffer and formatting helpers. No unrelated engine refactoring. The only
new files are focused unit/runtime tests; CI and verification documentation include
them. No active sanitizer known-bug/suppression entry exists for this functional
bug. Local builds, GCC/Clang probes, real Q3/OA clients, unit golden and format/type/
boundary checks pass. Full hosted gates and merged-tree regression remain required.

## #27 completed checkpoint

Current branch: `issue/27-headless-levels` in the main user checkout.
#26 PR #153 merged as bc1aff0d16878f3e170dab3c3aa1c1f7e79eee94 after exact-head
5f67dbf4 passed build 35537284408 and regression 35537284293. The merge and tested
head have the same tree cf77aa54884eef4140f015584e6a30fabfecb561. Its merged-tree
regression 35538219232 is running; close/check #26 only after it passes. #11/#151
are closed and #11 is checked in #25 after integration 35537127266 passed.
Modernization is merged forward through #26; no history was rewritten.
Continue #27 -> #28 then the rest of #25.

#27 test-first commit 1b69359d fails on the absent validate subcommand. The current
command passes Q3 and OA: two named PNGs and the automatic walk reproduce bytes,
with draw/triangle metrics, structure/lightmap/AAS reports and 240 bot position
samples. Q3 has 16 kills/25 pickups; OA has 13/30, with no inactivity flags. The
walk now visits all connected passages in order and returns through them (33
samples for the owned map). Named eye pose (-160,0,96) is unchanged across separated
frames in the camera probe. A deliberate open ceiling produces a clear leak error;
disconnected spawns and outside cameras fail separately. Stationary/moving/dead
sample controls verify the inactivity heuristic. Runtime warnings are retained.

Vulkan draw counters cover actual commands, including postprocessing; existing
renderer counters provide scene triangles. Read-only entity sampling uses the
existing public game tools API, initialized for development dedicated servers.
The development-only spectator camera avoids teleport launch velocity/effects;
existing setviewpos, gameplay simulation and TeleportPlayer remain unchanged.
CLI/report details, limits and conservative metric meanings are documented in
tools/level/README.md. CI and AGENTS commands are wired. No accepted fixture changes.

Local build /tmp/aftershock-level-validation-build; logs are in
~/.cache/aftershock-modernization/level-{validate-first,validate-path-q3,validate-oa,
validate-leak,camera-probe,classic-demo,tidy,lifetimes,rhi}.log. Boundary/type/native
ABI/RHI checks and tidy (1270 configurations) pass. Classic Q3 replay retains frame
hash 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4.
Lifetime analysis passes all 1216 commands (127 paths). First attempts at tidy/lifetimes
used caches for the original worktree; rerun in /tmp/aftershock-level-{tidy,lifetimes}
with the correct source root. The raw-MAP success path also passes (two automatic spawn captures), and final
OA validation passes two named plus 33 automatic PNGs across repeated runs.
Self-review is below. Draft PR #154 is open; require its exact-head hosted gates
and merged-tree regression. Do not merge before #26 integration passes.

## #27 self-review

Scope matches headless authoring validation: compile/load evidence, deterministic
PNGs, JSON/text reporting, real frame statistics and bot sampling. All source,
asset and camera identifiers are validated; JSON/MAP sizes, viewpoint counts and
engine command scripts have explicit bounds. Existing-MAP design data is marked
unavailable instead of silently passed. Inactivity is a warning heuristic with
stationary/moving/dead controls. Compiling a valid MAP with an omitted ceiling
exercises structural leak reporting; this is not a loader robustness target.

Read-only diagnostics use existing public boundaries. Fixed camera placement is
restricted to development builds, cheats and spectators; existing player movement,
teleport expressions and normal commands are untouched. Vulkan counts are bounded
POD integers, updated without allocation; no GPU call ordering or shader bytes
change. No OS access is added outside platform/filesystem code, no non-trivial core
objects or per-frame allocation is introduced, and wire/file layouts remain fixed.
Native ABI, RHI alternative-backend probes, boundary/type checks, 1270 tidy
configurations and classic replay pass. The accepted frame hash and all #26 compiled
fixtures are unchanged. Dedicated/client builds and both content sets pass headless
checks. Lifetime analysis passes 1216 configurations. Exact-head CI and merged-tree regression remain
required before acceptance. Optional orthographic/collision-only views are omitted;
named views and the automatic passage walk cover the required command.

## #26 checkpoint inherited by #27

#26 acceptance: PR #153 merged at bc1aff0d after exact-head full gates passed.
Hosted tool download/compilation matched all three owned fixture hashes. The two
bots reached the middle shotgun, east ammo and combat on the full map and each
isolated lane. Runtime screenshots passed. Integration 35538219232 remains pending.
Acceptance: https://github.com/msetaro/aftershock/issues/26#issuecomment-5752721345
Log: ~/.cache/aftershock-modernization/level26-host-runtime.log.

The first compiler implementation passes the MAP/schema/design-rule controls and
produces repeated byte-identical BSP/AAS in separate directories. Geometry review
caught duplicate coplanar room/corridor side-wall faces and a missing OBJ material
remap; both are corrected and the compiler log now has no warnings. Initial output
is disposable under /tmp/aftershock-level-{first,second,third}, not accepted fixtures.
Both Quake 3 and OpenArena native runtime checks now pass the full two-lane map,
the door/stair-only variant and the ramp-only variant: both bots reach the middle
shotgun, east ammo is collected, and combat occurs. Three room views were captured
and reviewed with correct cover, prop, sky, lighting and passages. Added blocked
room, rotated-axis, prop-bound, outside-entity, unknown-field and non-finite controls.
The initial owned MAP/BSP/AAS fixtures were explicitly authored once after review;
SHA256 MAP 555965db09c8358116557915bd1633b7b3a4dadb4b190be6e0c4313adab7b53b,
BSP 0d0fcf2ccb8cae4bd9afc19833297980613d8bfa7722ae6a481a585d7572a08c,
AAS 128895330784c535b5540c95a79f95aa511f94c93af7fff02fd479a48847bb0f.
CI, README and AGENTS commands are wired. Clean-cache AppImage extraction with
libarchive-c and normal repeated fixture comparison both pass. Self-review added
32-unit room separation so the navigation grid cannot cross an unconnected shared
wall, and world bounds before brush generation. Full empty-cache archive download,
SHA256 verification, extraction and compilation also pass. Self-review is below.
No engine edits or accepted golden changes. Continue #26 -> #27 -> #28 then #25.

## #26 self-review

Scope is the issue's declarative authoring/compiler and owned sample, with no
engine edits, runtime allocation/OS ownership, ABI/layout or simulation arithmetic
changes. Standard Python handles MAP-only generation and validation; the sole
extraction dependency is libarchive-c for a SHA256-pinned external map tool release.
Inputs are bounded, paths constrained to project assets, errors produce nonzero
status and no success JSON. Private staging excludes installed content and publishes
maps only after compilation succeeds. Output text has explicit LF bytes on every OS.
Both lanes have actual isolated-route bot acceptance in Q3 and OA, alongside rendered
three-room inspection. Whole-layout sightline and straight-line cover-distance bounds
are explicitly conservative, not claimed as precise visibility/path metrics. #27's
viewpoint/report features remain separate. Only new owned #26 fixtures were authored;
all accepted engine/gameplay/demo goldens are unchanged. Clean-cache extraction,
repeated MAP/BSP/AAS comparison, language/design controls and owned C/C++ formatting
pass locally. Exact-head full hosted gates and merged-tree regression remain required.

#11 PR #150 merged with a merge commit as
94a70b91f35acfa0636a7db473609b3aafde76e5. Its tree
d56b17d609a91ea9e84a6edfac10a7527a9c766c equals the tested 2caaa163 tree.
Exact-head build 35533785865 and regression 35533785920 passed after committed
AGENTS self-review. Hosted weapon loopback: 301/301 shots agree (18 hits), 38
uncompensated differences, median age 160 ms, prediction error <=8.875 units,
774/774 complete weapon and animation comparisons. Fixed OA replay checks 992
full-state hashes with repeatable frames under static and module renderers.
All Linux/macOS/MinGW/MSVC x64/ARM64 builds pass. Lifetime analysis passes 1216
commands. Merged-tree regression 35534705876 failed the wall-time harness deadline; #151 is the test-only correction.

## #26 preparation and failing contract test

Read #26's rooms/corridors/doors/stairs/ramps/material-role language, design rules,
deterministic MAP/BSP and real bot-pathing acceptance; #27 owns the later headless
viewpoint/fly-through validation report. #18 explicitly permits a provisional
format, so use versioned JSON consistent with tools/cook. Start with the tests.

No system packages were installed. Official NetRadiant-custom release 20260114
contains q3map2 2.5.17n-git-68ecbed and MBSPC 2.2. The Linux archive's official
SHA256 f48f6f1d0db2b910ef9cb5dc5d8a722852510f3c5c278dc17615c0466b8a7a3d was
verified, extracted in the user cache, and both tools run with their bundled
libraries. Archive/extracted tools: ~/.cache/aftershock-level-tools; extraction
helper: uv Python 3.12 venv ~/.cache/aftershock-level-python (libarchive-c 5.3).
Primary source: https://github.com/Garux/netradiant-custom/releases/tag/20260114

A disposable, owned one-room map and plain texture in cache/probe-a and probe-b
verify the toolchain. Use single-threaded BSP/VIS/light and MBSPC
-forcesidesvisible; without that MBSPC option the generated brush sides are not
marked visible for AAS. All declared BSP lump bytes agree across fresh output
directories; three alignment-padding bytes differ. Zeroing only bytes outside
declared lumps before MBSPC yields byte-identical BSP and AAS. New compiler
packaging should canonicalize that padding, with a focused check. No engine
parser or accepted golden is changed. Detailed scratch notes: /tmp/aftershock-level-next.md.

The new language contract is in tools/level/README.md. The owned two_lane.json
sample includes three rooms, four passages forming two lanes, a door, stairs,
a ramp, four FFA/team spawns, cover kits, a solid OBJ prop, pickups and lighting.
Six 16x16 procedural textures and a 64-unit cube have a source authoring script
and pinned provenance. tests/level.py requires deterministic MAP output and
specific corridor/door/containment/connectivity/material/sightline/cover/duplicate
errors. It fails on the absent CLI before implementation. The optional --compile gate also requires repeated MAP/BSP/AAS bytes and committed
fixture comparison; its explicit recording flag is refused in CI. The compiler,
reviewed fixtures and real bot-pathing acceptance now exist as recorded above.
Navigation uses a 16-unit player-clearance grid with
18-unit maximum steps and treats doors as open. Sightlines use the conservative
whole-layout diagonal bound; cover spacing is horizontal distance to nearest cover
plus a half-cell diagonal. These restrictions must be documented, not portrayed
as exact visibility/path distance. Full BSP/AAS compilation uses a verified pinned
Linux x86_64 tool bundle; MAP-only mode is portable.

## #151 acceptance and merge

PR #152 merged as 64a38d44aa50e5accca3676f7f7927be87594c25 after exact-head
7829fa97cab8b6aa6fc5f433ddd4ef80495e71f1 passed build 35536234271 and regression
35536234299. Both trees are bff0172c6eabfb0c2c663a922807faeffa8bf9c7.
Hosted normal loopback: 31.1 seconds, 592/592 shots, 37 hits, 73 uncompensated
differences, median age 153 ms. Capped 20-FPS weapon control: 54.0 seconds,
301/301 shots, 18 hits, 36 uncompensated differences, median age 200 ms,
1514/1514 full weapon and animation comparisons. Prediction error <=8.875 for
both; ignoring-child cleanup passes. Local controls were 17.7 seconds classic and
54.0 seconds weapons (329/329 and 301/301 shots). No engine or accepted fixtures
changed. Original failed integration had 1147/1147 agreeing states before its
45-second wall-time limit; graceful teardown then stalled. The replacement budgets
120 seconds for weapons inside a 240-second outer limit and kills/reaps a client
that ignores SIGTERM. Merged-tree regression 35537127266 passed. #11/#151 are closed; #11 is checked
in #25. Closure evidence: https://github.com/msetaro/aftershock/issues/11#issuecomment-5752700766
Acceptance: https://github.com/msetaro/aftershock/issues/151#issuecomment-5752604897
Logs: ~/.cache/aftershock-modernization/netcode-151-host-runtime.log.

## #151 self-review

This is exclusively test infrastructure. The same gameplay commands, fixed engine
tick, data files, accepted fixtures, hit oracle, complete state comparisons and
prediction checks remain. A low-FPS control proves the wall-time regression instead
of hiding it with a blind retry. Only the renderer-dependent view-age timing bound
accounts for one extra frame interval, never exceeding the configured 200 ms
rewind window; default 100-FPS behavior retains the old 180 ms ceiling. Cleanup
uses the existing private process group, graceful termination followed by forced
kill/reaping. No engine, simulation arithmetic, OS ownership, allocation or wire
layout changes are present. CI uses the same cooker Python environment and runs
both the ignoring-child check and the capped-FPS scenario. Full final-head gates
and merged-tree regression remain required before closing #151/#11.


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

## #11 acceptance checks in progress

The delayed/lossy weapon scenario is being added to the existing private-loopback
driver, with a 256-byte snapshot budget and a real input trigger after client
initialization. The first runs exposed harness startup/target timing problems;
the corrected scenario passes both content sets. Q3: 301/301 shots, 18 hits,
38 uncompensated differences and 786/786 full weapon/animation comparisons. OA:
301/301 shots, 19 hits, 36 uncompensated differences and 754/754 comparisons. Both
median view ages are 160 ms with prediction error bounded at 8.875 units. Loss is
enabled after the initial gamestate; the default classic scenario is unchanged.
The lifetime gate now covers the weapons
subsystem explicitly, and subsystem ownership is documented.

Full-state diagnostic hashes cover all 56 bytes (portable probe checks every byte).
The separate replay driver and CI commands are written; fixture acceptance is
recorded in the following checkpoint. The range HUD now shows actual magazine/chamber/reserve,
and followed-player commands cannot be predicted from the spectator input.
GCC and Clang/libc++ UBSan probes pass with the unchanged 1000-shot digest.
The final Q3 lifecycle/HUD run, formatting/boundary/type gates and 1270-configuration
tidy pass; full lifetime analysis also passes 1216 commands.

## #11 replay and compatibility acceptance

Both new fixtures were recorded once from 6dbe2a93: q3dm17 and oa_dm1, about 44 KiB
each, containing owned native gameplay records only. Two fixed replays per content
pass 992 full-state hashes and three identical sampled frames. The first replay
harness asked for status after the 199-frame demo ended; samples are now at frames
40/80/140, without re-recording. Reviewed captures show the owned rifle/optic,
ADS/reload, offhand/projectile presentation and magazine/chamber/reserve HUD.

Prior hosted head 16feb9f1 exposed MSVC C4701 after the animation error path and a
C witness rejecting the new bool game declaration. Feature error handling now exits
explicitly after G_Error, and game-facing booleans use qboolean. An attempted historical native C/C++ gate exposed direct C++ definition access
in legacy source files; plain
weapon-count/name queries keep those files compatible, and the HUD name follows
the acknowledged selection during demo replay. These are #11 implementation
corrections, with no legacy arithmetic or accepted golden changes. The old C bot
command witness passes. Unit digest/one-ULP control, 1216-command lifetime analysis,
and 1270-configuration tidy passed before these small compatibility corrections;
rerun affected current gates before acceptance. `tests/native_gates.py` belongs
to historical revision 7f4d43a7, as tests/README.md specifies; its current-tree
attempt then failed on existing #10 C++ declarations. Do not rerun or alter the
historical oracle. Current ABI/shared-math and production/replay checks are the
applicable gates.

## #11 self-review checkpoint

Scope matches #11: immutable data definitions and owned assets, seeded fixed ticks,
reload/cancel/ammo/ADS/attachments, data-only switching, dual-wield hooks, melee,
rewound hitscan/material effects/penetration, predicted replicated grenades, graph
notifies and an ImGui target range. Feature negotiation advances 1 to 2; demo codec
68 and existing wire layouts are retained. Protocol-version mismatch is tested.
No existing simulation FP expression, accepted fixture/golden, parent repository,
or unrelated engine bug fix is included. New sampling/weapon TUs use strict FP.

State and histories are bounded POD; projectile predictions cap at 64, effects at
128, and four auxiliary records per configured client are reserved at map start
and reused across team/spawn transitions. Definitions, graphs, models and sounds
load outside frame execution. Native hunk entities and existing arena ownership
remain in use; no new heap allocation or non-trivial destructor crosses Com_Error.
No OS calls leave platform/filesystem ownership. Struct size/triviality assertions,
real MSG codec probes and the 29-type native ABI gate retain representation checks.

Reviewed captures show actual magazine/chamber/reserve and acknowledged weapon
names. Cosmetic notify dedup handles reordering, rollback, spawn and connection
generations; capacity rejection reconciles prediction without spending server ammo.
The 8192-notify history and bounded cosmetic pool are explicit ceilings. One
speculative sound before a capacity acknowledgement cannot be unplayed; authoritative
damage/ammo and settled prediction stay exact. Offhand hooks do not impose a new
inventory policy; the supplied game mode owns the two hands and loaded definitions.

Local current-feature evidence: Q3/OA fixed #11 replays each check 992 full states;
all three frame hashes agree between repeated static and module playback. Classic
Q3 frames retain 43c52e51fbf3d2585f899737339c5e71ea14d69794be37ca1f3a5e5e80a1dbd4;
#10 Q3 replay retains 253 authoritative hit-box hashes. Both OA bot logs, collision
differential, unit/one-ULP control, sanitized units, native ABI/shared math and the
new weapon probes pass. Real range input and Q3/OA lifecycle/audio/capacity evidence
are recorded above. Full hosted acceptance remains pending.

e2d0a7b7 MSVC exposed the matching client-side C4701 after the server correction.
Both BG_WeaponAnimationStep call sites now explicitly return after their fatal
error path. This changes no successful command. Require fresh complete exact-head
build/regression after this committed self-review before ready/merge.
Acceptance progress: https://github.com/msetaro/aftershock/issues/11#issuecomment-5752242148

## #11 weapon animation snapshot test

The real entity-codec probe now fails on the absent per-hand weapon-animation
adapter (weapons-animation-snapshot-before.log). It requires all animation state,
16 parameters, full spawn/notify counters and owner/hand/definition/attachment
metadata to round-trip without changing wire layouts. Reuse the existing #10
animation adapter; reserve entity type 253 and its two unused parameter slots for
the spawn counter. Existing animation fixtures stay unchanged.
The adapter now passes GCC and Clang/libc++ UBSan
(weapons-animation-snapshot-gcc.log/-clang.log), including full counters and all
parameters; the existing 1000-shot digest and 67/10-byte weapon-state sizes remain
unchanged. Next: shared notify advancement and live graph loading/prediction.
The new graph/step probe fails on missing BG_WeaponAnimationStep
(weapons-animation-step-before.log). It checks automatic shot/shell pairs,
reload/cancel, melee, ADS and identical replay from an acknowledgement. New
range.json references the original #10 model files; only the new #11 graph has
reload notifies aligned to the existing weapon stages (200/600/800/1000 ms).

Shared graph advancement now passes GCC and Clang/libc++ UBSan
(weapons-animation-step-gcc.log/-clang.log). It flushes previous clip notifies
before restarting fire and commits bounded state/notify output together. The new
live assertion fails with zero predicted-animation comparisons
(weapons-animation-live-before.log). Load/hash-check graphs, replicate per-hand
state, and replay animation alongside weapon inputs before claiming live parity.

Live graph loading and per-hand snapshot/input replay pass
(weapons-animation-live-build.log, weapons-animation-live.log), including at least
50 main-hand comparisons of complete animation state and all parameters. Graphs
are loaded once, their hashes are checked against the server, attachment sockets
must exist, and file storage is released at shutdown. Per-tick advancement keeps
weapon and animation clocks together; split-budget records wait for a matching
pair. Spectator commands release stale weapon auxiliary entities. GCC/Clang
portable probes and format/boundary/type checks pass. This is not presentation
acceptance yet: view/attachment rendering, notify sound, resource/lifecycle/lossy
coverage, new replay, tooling and full gates remain.

The first-person presentation assertion now fails with no rendering status
(weapons-view-before.log). It requires actual skeletal and socket-attachment draws,
full ADS at the configured optic FOV (45), a centered optic, positive cosmetic
view kick and a screenshot for review. Existing gameplay checks still pass.

First-person rendering now passes the live numerical gate: skeletal model and
socket attachment draws, FOV 45 at ADS, maximum optic error 0.000011 units and
positive view kick (weapons-view-build.log, weapons-view.log). The first screenshot
caught the old model's solid placeholder sight and an incorrectly oriented new
optic. The new #11 rifle variant removes only that placeholder scene node and
references the unchanged #10 buffer; the original source stays unchanged. The
new optic is authored in its socket's local frame. Re-reviewed capture shows an
open sight with a centered blue point. GCC/Clang portable checks and style gates
pass. The test capture uses the engine's existing screenshot/TGA command.
Next: notify-driven sound and remaining target-range/acceptance scope.

The live sound assertion fails with zero server notifies/sounds
(weapons-sound-before.log), with SDL dummy audio successfully initialized. The
next implementation must map graph notify names to preloaded data sound handles,
replicate remote events and deduplicate local predicted/acknowledged events.

The new cosmetic-budget assertion fails while 128 feature effects are active
(weapons-effects-budget-before.log). Cap new weapon cosmetics without dropping
hitscan damage; this prevents notify/impact bursts from consuming the legacy
entity pool. The legacy allocator itself remains outside this feature's scope.

The initial notify path passes Q3 and OpenArena: 39 audible events each, exactly
once, and 449 matching main-hand animation comparisons. SDL dummy audio initializes
and loads the new sounds; this verifies dispatch, not physical speaker output.
Review identified that entity slot order can differ from notify order. The new
portable order/spawn/connection/wrap test fails on the absent bounded notify
window (weapons-notify-order-before.log); replace the initial high-water-only
deduplication before committing this implementation.

Notify audio and the bounded cosmetic budget now pass GCC/Clang UBSan and the
production build. The notify window handles slot reordering, respawn, connection
reuse and counter wrap; notifications older than its 8192-event window are stale.
Final Q3/OpenArena live runs both pass (weapons-notify-final-q3.log/-oa.log): 39
unique audible notifies, 449 matching main-hand animation comparisons, resident
7056-byte shot and 3968-byte reload samples. The sound-list check uses existing
s_list; the first attempted legacy soundlist command did not enumerate samples.
Format/boundary/type checks pass. New cosmetics cap at 128; at the entity
high-water ceiling they are dropped until map restart, preserving eight unopened
slots. Hitscan damage still applies when effects are dropped. Projectile capacity
and full lifecycle/lossy/replay/tooling acceptance remain outstanding.

The real ImGui range test now fails waiting for the absent dev_weapon_range
panel (weapons-range-before.log). It requires pointer-driven target spawning,
selection of a second data-only rifle, trigger and reload. Reuse existing local
rewind-target/game commands and cooked inspection; mutations run after vendor UI
calls return. Keep the range tab absent until opened so existing tool layouts
retain their positions.

The ImGui range now passes real pointer/key input on Q3 and OpenArena
(weapons-range.log/-oa.log): moving target, arbitrary loaded slot selection,
trigger and reload. The reviewed panel capture shows cooked definition inspection,
ADS/offhand/melee/attachment controls, rewind counters and restart/capture actions.
It opens with dev_weapon_range and stays absent from the tab bar otherwise.
Game mutations run after vendor UI calls. Build/style/boundary/type checks pass.
Next: bound projectiles and reserve/reuse per-client auxiliary records, then
lifecycle, lossy input and fixed replay acceptance before the complete CI gates.

The next portable assertion fails on absent WeaponProjectileAvailable
(weapons-projectile-budget-before.log); it requires a 64-actor projectile ceiling
and eight unopened entity slots. The opt-in live --lifecycle extension fails on
absent actor-record reuse evidence (weapons-lifecycle-before.log). Reserve four
auxiliary records per configured client at map initialization, reuse them across
spectator/respawn, and pause projectile firing at capacity without spending ammo.
Replicate that blocked state so client prediction follows the same rule; discard
rejected local projectiles and allow corrected notify identities to be reused.

The capacity follow-up requires the blocked bit to survive the real entity codec
and a rejected predicted notify identity to be usable again after capacity frees.
The portable check fails on absent Weapon_ForgetNotifiesAfter
(weapons-notify-rejection-before.log); only unacknowledged notify bits will be
forgotten, preserving deduplication of accepted sounds.

Lifecycle implementation review found ClientBegin intentionally resets the legacy
spawn count to 1 on team transitions. The initial pool test therefore could not
use that count as a unique connection identity. Carry the existing rewindSpawn
connection generation in unused auxiliary fields; also distinguish predicted
projectiles/notifies by it and reject late prior-generation notifications. The
portable late-generation assertion fails on current deduplication
(weapons-notify-generation-before.log). This is new #11 bookkeeping, not a change
to legacy player spawn semantics. The scenario now respects the five-second team
switch cooldown; the live 64-projectile pressure segment reaches its cap without
spending further ammo.

Projectile capacity, reusable auxiliary records and connection-generation handling
now pass GCC/Clang UBSan and both live content sets
(weapons-generation-{gcc,clang,build}.log, weapons-lifecycle.log/-oa.log).
Four records per configured client are reserved at map start and reused across
team transitions; inadequate map capacity is rejected explicitly. The owner keeps
the same record IDs while its connection generation changes, even though the
legacy spawn count returns to 1. New auxiliary constantLight fields carry that
full generation (these entity types bypass lighting); projectiles use their spare
angular-trajectory clock. No wire struct or legacy spawn semantic changes.

At 64 projectiles or the entity high-water reserve, server firing pauses without
spending ammo and the snapshot blocked bit guides prediction. Rejected local
projectiles expire; only unacknowledged notify bits are forgotten so later accepted
shots can sound. Late prior-generation/spawn notifications are discarded. The
live pressure segment holds sequence/magazine at 64/64 for at least ten samples
and reconciles prediction. Team transitions/disconnect remove owned projectiles;
ordinary respawns retain them. The final OA test also verifies post-join shot
notifications still sound. New cosmetics retain their separate 128-effect limit.
Format/boundary/type gates pass. Full #11 acceptance remains pending.

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
The new snapshot probe now fails on absent game/bg/bg_weapons.cpp
(weapons-snapshot-before.log). It requires all 56 weapon-state bytes, full-width
seed/clock/spawn counters, owner/hand/definition/attachment metadata, origin and
compact deltas to survive the production entity codec. Decision: an auxiliary
ET_WEAPON_STATE record (254) keeps existing wire structs and movement fields
unchanged; the native feature protocol must be revised before exposing the new
record to real clients. No gameplay/prediction acceptance is claimed yet.
The snapshot contract now passes GCC/Clang UBSan (67 initial bytes, 10-byte
clock delta), following ac4f90b4's missing-adapter failure. Six existing full-width
integer fields plus finite exact 16-bit float halves preserve all state bits;
there are no changed wire structs or float bit-punning. Current native ABI and
pre-#12 replication digest still pass. The production build initially caught
unused namespace constants in native wrappers; matching the existing inline
constexpr convention fixes that without relaxing warnings. The final native
client/server build passes (weapons-snapshot-build.log). Next: shared command
advancement/prediction tests, then actual game/cgame integration and owned range
assets. Protocol revision remains mandatory before publishing new auxiliary
records to real clients.

The command/prediction probe now fails on the absent Weapon_Command API and event
timestamps (weapons-command-before.log). It covers irregular usercmd intervals,
replay from older acknowledgements, duplicate commands, independent hands, clock
wrap, 50 events over one second, and atomic rejection beyond that bound. The
bound follows Pmove's existing 1000 ms catch-up window; gameplay integration must
handle longer inactivity explicitly without manufacturing an unbounded backlog.

Weapon_Command now passes that contract under GCC and Clang/libc++ UBSan
(weapons-command-gcc.log, weapons-command-clang.log). Events carry fixed-tick
timestamps, bounded batches hold the 50-tick catch-up window, and failure leaves
state/events unchanged. The original 1000-shot digest and 67/10-byte snapshot
sizes are unchanged. Next: cooked-file loading and opt-in gameplay/prediction
integration; portable replay alone is not live gameplay acceptance.

The real native-game script tests/weapons_runtime.py fails first at absent server
weapon-definition loading (weapons-live-before.log). It will check cooked data
selection, command-driven fire/reload/ADS/melee and identical authoritative state
received by cgame. This live slice does not yet assert damage, projectiles,
weapon presentation or target-range tooling; those remain required by #11.

The initial live state slice passes (weapons-live.log): cooked files load through
FS into fixed storage, g_weapons publishes content hashes, cgame checks matching
definitions, command ticks publish owner-only hand states, and classic PM_Weapon
is bypassed only when this opt-in data set is loaded. Feature protocol is now 2;
the incompatibility gate builds protocol 3. Build and portable probes pass, and
new sources pass boundary/style/type checks. No fixture or golden changed.
This slice only advances/logs events; damage, visual/audio effects, switching and
projectiles are still outstanding. Prediction replays queued inputs but needs
live acknowledgement comparison evidence and presentation consumers. Next: add
that evidence, then integrate rewind damage and remaining #11 gameplay.

Live prediction evidence is now required by a failing assertion: no comparison
with later acknowledgements exists yet (weapons-prediction-before.log, 0 samples).
Add a bounded history of actually advanced predicted states and compare all 56
bytes when their authoritative ticks arrive; never count copied baseline states.
Issue update: https://github.com/msetaro/aftershock/issues/11#issuecomment-5751395434

The live prediction assertion now passes: 410 hand/tick acknowledgements matched
all 56 previously predicted bytes, including 205 main-hand samples. Records are
stored only after input replay advances beyond the snapshot baseline, and tagged
with spawn/definition/attachments (weapons-prediction.log). Next: explicit event
time for rewind and actual weapon damage, followed by the remaining integration.

The explicit shot-time rewind check fails on missing G_TraceHitscanAtTime
(weapons-rewind-before.log). It queries the same actor at 900 and 1000 ms while
the newest usercmd stays at 1000 ms, requiring different hit decisions without
mutating live actor or command state. Keep legacy G_TraceHitscan as a wrapper.

Explicit-time rewind passes GCC/Clang UBSan while keeping the legacy wrapper
behavior and history checks unchanged (weapons-rewind-gcc.log/-clang.log).
The live damage extension now fails with no damage events against a successfully
created moving range target (weapons-damage-before.log). Implement actual
hitscan/melee damage through that trace before adding penetration/projectiles.

Live hitscan now applies the data-defined damage through fixed-event-time rewind;
melee shares that trace with its own range/damage. The owned moving target takes
40-point rifle hits and the extended state/prediction assertions pass
(weapons-damage.log). This is still incomplete #11: world penetration/material
presentation, switching/attachments, projectile actors, animation/audio integration
and range controls remain. Next: inventory-preserving switching and selection of
a second rifle authored only in data.

The switching probe fails on missing Weapon_Switch (weapons-switch-before.log).
It requires retained ammo/random state across a round trip, a data-defined equip
delay before firing, reload-stage cancellation rules and atomic rejection.

Weapon_Switch passes GCC and Clang/libc++ UBSan: equip delay, ammo/seed retention
and reload cancel boundaries (weapons-switch-gcc.log/-clang.log). The old 1000-shot
reference remains unchanged. Wire the server inventory and existing weapon
selection commands, then prove two data files can be selected in the live game.

The live two-rifle extension fails at the expected missing selection behavior
(weapons-selection-before.log): both cooked files load, but no switch occurs.
It requires 0->1->0 selection and return to the first rifle's remaining 29-round
magazine, plus the existing prediction, damage and state round-trip assertions.

Live two-rifle selection passes (weapons-selection.log). Each hand retains a
bounded per-definition inventory; existing weapon/next/previous commands select
data definitions, pickups no longer overwrite that selection, and respawn picks
the first data weapon. Switching waits for server acknowledgement while active
weapon inputs remain predicted. The first run exposed a remaining legacy
respawn selection assignment; the test also needed 130 rather than 120 frames
to empty all 31 loaded rounds before checking the 29-round empty reload. Both
are corrected, and 0->1->0 preserves ammo/seed. Production build and focused
format/boundary/type gates pass. Remaining #11 scope is listed in Next action.

The analytic game-collision probe now fails on a one-unit default-material wall
(weapons-penetration-before.log). It exercises the actual game WeaponHit path:
40 unobstructed damage, 20 through default material, 10 through thin metal,
blocking thick metal, cumulative loss across two walls and non-penetrating melee.
This is a gameplay collision test, not a loader robustness target.

Material penetration now passes both compiler/UBSan probes and the live rifle
scenario (weapons-penetration-gcc.log/-clang.log/-live.log). Four impact queries
bound a shot; one-unit occupancy probes locate an outside point and a reverse
surface trace measures thickness. Sub-unit gaps count in the same thickness
budget, an explicit resolution limit. No simulation expression outside the new
weapon implementation changed. Next: material impact presentation and remaining
projectile/animation/attachment/range integration.

The material presentation assertion fails as intended (weapons-impact-before.log).
New owned material JSON supplies distinct default/metal impact colors through the
existing cooker; this does not alter any accepted art or replay fixture. Next:
replicate the definition/material index and render that material at the impact.

Material impact presentation passes the live test and GCC/Clang portable checks
(weapons-impact-live.log, weapons-impact-gcc.log/-clang.log). New EV_WEAPON_IMPACT
appends to existing event values and carries definition/material indices; cgame
preloads all effect shaders at initialization and uses the existing fixed mark
pool. No first-hit asset loading is introduced. The thin-wall probe's direction
stub was corrected to the native game's existing non-const signature. Format and
boundary checks pass. Remaining: projectiles/grenades, attachment/view/notify/audio
integration, ImGui range tooling, lifecycle/parity tests and full CI acceptance.

Projectile/attachment presentation sources are newly authored by
 tests/assets/weapons/export.py: original box optic/grenade props and deterministic
synthetic shot/reload sounds, with hashes in provenance.json. These are new #11
assets; accepted #10 sources and fixtures remain untouched. The projectile asset
probe fails on missing model/size fields (weapons-projectile-asset-before.log).
Decision: weapon source/cooked format version 2 adds these required authoring
fields before #11 acceptance; other cooked formats keep version 1. No weapon
format has shipped or been accepted yet. Implement and validate that payload,
then shared fixed-tick collision, server projectile actors and client prediction.

The version-2 payload passes GCC/Clang UBSan with a 4004-byte definition while
weapon state/wire sizes and the 1000-shot digest stay unchanged
(weapons-projectile-asset-gcc.log/-clang.log). All six presentation resources cook
successfully (weapons-presentation-cook.log). Other asset envelopes retain their
existing default version and bytes. Provenance checks now guard the new committed
props/sounds; live rendering/audio review and actual projectile integration remain.

Shared projectile collision now has a failing contract in the existing snapshot
probe (weapons-projectile-step-before.log): fixed 20 ms flight, configured box
size, bounce loss/exit offset, fuse expiration and zero-bounce impact detonation.
Use the existing game/cgame trace signature so both sides share that step.

BG_WeaponProjectileStep now passes GCC/Clang UBSan
(weapons-projectile-step-gcc.log/-clang.log). It uses the existing game/cgame
trace signature, data collision bounds and the same fixed-step motion/bounce
math. Fuse expiry and zero-bounce impacts detonate; bounces retain the remaining
fuse. Next: actual server projectile entities, snapshot rendering, client shot
prediction and lifecycle tests. Shared physics alone is not projectile acceptance.

The extended live test loads a third, data-only grenade definition but fails on
absent projectile actors (weapons-projectile-live-before.log), after a successful
version-2 production rebuild. Reuse ET_MISSILE with generic1=255 (existing native
missiles use only team values), carrying definition/owner/hand/shot/spawn metadata
and fixed-step position/velocity/age in existing snapshot fields. No wire-layout
change. Add server actors and snapshot rendering before client shot prediction.

The server projectile/snapshot slice passes its live test
(weapons-projectile-live.log) and the portable GCC probe. Data-only grenade
selection spawns an owned model, advances shared fixed ticks, bounces and expires
its fuse; detonation uses existing direct/radius damage and bounded local effects.
Position/velocity/age stay in existing missile fields; model assets preload at
initialization. Owner disconnect removes remaining projectiles, while respawn
alone leaves them alive. Format, boundary and type gates pass. Next: client shot
prediction using each command's predicted movement pose, with deduplication against
authoritative projectile records and measured correction error. No full #11
acceptance or projectile prediction is claimed by this server-only slice.

Client projectile prediction fails its new live assertion
(weapons-projectile-prediction-before.log). A separate failing launch contract
requires server/client reuse of the command pose and data speed/spread
(weapons-launch-before.log). Record poses after each Pmove command and before
trigger prediction, matching the server's weapon hook. Use a bounded predicted
projectile pool, identify shots by spawn/hand/definition/sequence and measure the
first authoritative position correction at the same projectile age.

Client projectile prediction now passes the live test with one predicted grenade
and first authoritative position correction 0.000000 units
(weapons-projectile-prediction.log). Per-command Pmove poses feed the shared launch
helper; server/client use the same collision step. A fixed 64-shot pool falls
back to authoritative rendering when full. Acknowledged/expired identities remain
until their commands leave history, avoiding replay duplicates; authoritative
entities take over drawing. GCC/Clang probes and format/boundary/type checks pass.
Next: attachments and weapon/view/notify/audio integration, then ImGui range,
stronger lifecycle/latency/replay evidence and complete CI/self-review acceptance.

The attachment command extension fails at missing modifier/replication evidence
(weapons-attachment-before.log). Require the optic mask to produce spread 0.75
and FOV 45 on the server and reach the matching client definition. Keep masks
per hand and inventory weapon, validate data-supported bits, and apply equip
delay without changing ammunition. Socket rendering follows in presentation.

Attachment modifiers/replication pass the live scenario (weapons-attachment.log):
mask 1 gives spread 0.75 and FOV 45, reaches cgame and remains in the first rifle's
inventory across switching. The server caches configured definitions per active
hand, rejects unsupported masks/busy reload stages and applies the equip delay
without refilling ammo. Build, format and boundary checks pass. Socket/view
presentation and notify-driven audio are still outstanding.

A new failing portable assertion requires reload-start/cancel events and a second
reload press to cancel at an allowed stage (weapons-reload-events-before.log).
These events let weapon animation follow mechanical state without adding an
input bit or inferring starts from later magazine events.

Reload begin/cancel events and second-press cancellation now pass GCC/Clang UBSan
(weapons-reload-events-gcc.log/-clang.log), including ammo conservation. Shot
reference and snapshot sizes remain unchanged. Next: per-hand weapon animation
state using existing animation APIs/codec, notify-driven sound, socket/view
presentation, then range controls and complete acceptance gates.


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
