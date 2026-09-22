# AI and navigation

Cook collision geometry with Recast, query it through Detour, and steer native
bots through the existing usercmd/Pmove and weapon paths. Set both latched cvars
before loading a map:

```
set g_navigation navigation/level.asnav
set g_behavior behaviors/guard.asai
set g_weapons weapons/range_rifle.asweapon
map level
addbot Sarge 3
```

Empty navigation/behavior selections retain legacy botlib. The new controller
needs no AAS. Botlib remains until existing bot matches have full parity.

## Cooking

The existing `tools/cook` manifest accepts `navigation` and `behavior` kinds.
Navigation source is JSON with `version: 1`, a relative collision BSP path,
`agent: {radius: 15, height: 56, climb: 18, slope: 46}`, `cell_size: 4`,
`cell_height: 2`, and a `links` array. Each link has a unique positive `id`,
three-component `start`/`end` positions, `radius`, `bidirectional` and `kind`:
`jump`, `drop`, `door`, or `launch`. Coordinates are engine Z-up. Launch endpoints
must lie inside the corresponding native push trigger; its actual movement
notification starts traversal. The map supplies the impulse. Record its landing
with the authoritative movement test, never substitute a second movement model.
Door links steer through existing native door interactions; they do not open
locked/scripted doors. Cooked files bind the collision content/checksum.

Behavior JSON has `version: 1`, a `name`, an `initial` state name and `states`.
Each state has `name`, optional `parent`, `action` and `transitions`. Actions are
`idle`, `patrol`, `investigate`, `cover`, `attack`. Transitions specify `to`,
`field`, `op`, `value`, and `min_ms`. Fields are `visible`, `heard`, `health`,
`covered`, `time_ms`, `distance`; operations are `eq`, `ne`, `lt`, `le`, `gt`,
`ge`. Leaf rules run before parent rules, in authored order, at most one
transition per fixed tick. Health is normalized to 100 and clamped at one.

Patrol selects the farthest fully reachable deathmatch spawn among at most 64
spawns. Investigation follows the last sensed hostile position. Attack aims and
fires through existing weapon commands, with native respawn/reload rules. Cover
selects a reachable navmesh boundary point protected by the current threat trace,
then crouches and follows that route. Arrival plus protection against the stored
last-known threat drives `covered`, even after target acquisition expires.
These are bounded game actions; game-specific patrol schedules/objectives can
replace this policy without changing the navigation owner.

Sight uses native solid traces, a 120-degree field of view and 4096-unit range.
A hostile seen in range wins before the loudest heard source, with stable entity
ID ties. Memory expires after five seconds. Only real weapon shots produce the
200-ms hearing observation. The current weapon samples use linear attenuation
from 80 to 1330 units, with the same shared 0.35 blocked gain as authored audio;
hearing threshold is 0.05. Cover uses a 1024-unit search and crouched eye traces.

## Ownership and limits

The cook uses solid and player-clip collision, not render geometry. Pinned
Recast/Detour 1.6.0 keeps its original license and source hashes under
`third_party/recast`. Native worlds own their copied single tile and fixed query
storage through the zone allocator. Queries, steering and behavior ticks allocate
nothing. Detour crowd steering is available; gameplay uses its avoidance sampler
from authoritative actor snapshots, with no asynchronous path queue to restore.

Limits: 16 MiB asset, 32,768 polygons, 65,535 vertices, 256 links/route corners,
8192 query nodes, 64 agents/neighbors, and 256 cover candidates. Behavior has 32
states, 128 transitions and eight hierarchy levels. Partial paths and truncated
cover results are explicit. Asset validation checks hashes, layout and bounds
before vendor pointers are fixed up. Assets are loaded at map start.

Typed checkpoints own asset identities, selected cvars, controller clock, routes,
link phases, behavior, perception memory, cover/replan state and recent shot
clocks. Restore validates all actors and references before publishing state.
Pre-navigation checkpoints select the legacy controller; present invalid records
fail instead of silently migrating.

## Inspection and verification

The development AI panel shows state age/transitions, target, sight/hearing/gain,
cover status, route cursor and traversal phase. Green lines show ordinary route
segments; orange lines show authored links. `tools/agent` actor telemetry exposes
the same state. `tests/navigation.py` checks the bounded owners under UBSan;
`tests/navigation_runtime.py` exercises patrol, combat, hearing and exact
checkpoint continuation on owned geometry with either installed content set.
