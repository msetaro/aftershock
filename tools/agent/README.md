# Local agent playtests

Build prerequisites are the same as the engine. Install the Python schema validator
with `python3 -m pip install -r tools/agent/requirements.txt` in your environment
(or the distribution's `python3-jsonschema`). Headless Linux also needs Xvfb and a
Vulkan software driver. Use installed Q3 or OpenArena content; no paks are included.

```sh
python3 tools/agent run --map q3dm17 --script tools/agent/examples/playtest.json --out ./playtest-result
```

Without `--binary`, this configures/builds a development Debug client in the output
directory. `--binary PATH` reuses a development client. OpenArena needs
`--content openarena --data PATH_TO_BASEOA --map oa_dm1`. Set `VK_DRIVER_FILES`
to use a particular GPU; otherwise the launcher chooses lavapipe. Shipping clients
do not expose the channel. An existing report is never overwritten.

A minimal script:

```json
{
  "version": 1,
  "dt": 20,
  "seed": 123,
  "steps": [
    {"op": "capture", "name": "start"},
    {"op": "walk", "offset": [32, 0, 0], "max_frames": 100},
    {"op": "capture", "name": "walked"},
    {"op": "aim", "target": {"position": [0, 0, 128]}},
    {"op": "fire", "frames": 30},
    {"op": "capture", "name": "fired"},
    {"op": "assert", "metric": "errors", "max": 0}
  ]
}
```

See `schemas/playtest.json` for every field. `cvars` supplies launch-time cvars;
`warmup` defaults to 150 frames. `walk` accepts an absolute `position` or an
`offset` from the starting player position, with tolerance and a bounded frame
budget. It steers directly toward the waypoint through normal usercmds; authors
must provide a traversable route. It is not pathfinding. `aim` and `fire` accept a
world position, entity number or classname; entity targets use live bounds.
`fire` with a target updates aim each frame. `request` forwards any channel request
(e.g. `{"op":"request","request":{"op":"exec","command":"give all"}}`).
`capture` optionally accepts a player/free-camera pose. Assertions cover hits,
kills, errors, asserts, warnings and `p99_ms`. Frame times measure the actual host,
so choose performance limits for the runner; simulation still uses fixed dt.

`report.json` retains per-step results, final profile/state, events and PNG paths.
The process exits 1 and retains an error path/hint plus `engine.log` on a playtest
failure. Invalid scripts fail before launch with file/path/type diagnostics.
Build output is in `build.log`. Each launch has a private home and automatic
X display; `AFTERSHOCK_SCRATCH` selects the parent of its unique temporary root.
The Python `Engine` context manager exposes `request()`, `step()` and `events`.

## Authoring schemas

```sh
python3 tools/agent describe weapon
python3 tools/agent validate weapon tests/assets/weapons/rifle.weapon.json
```

`describe` emits `{kind, schema, example, notes}`. Supported kinds are `level`,
`weapon`, `animation`, `material`, `effect` and `match-spec`. These are JSON Schema
2020-12 documents. `validate` checks structure, field types and ranges; it does
not cook resources or replace the loader's geometry, reference, ordering and
cross-field checks. Run the cooker/level compiler to check those too. Schema
boundaries are compared with the production loaders in `tests/agent_formats.py`.
The cooker and level compiler apply the same schemas and emit JSON error objects
with `file`, `path` and `hint`; a semantic error concerning the whole document
uses `$`. A valid level/animation example still needs its referenced source assets.

Material recipes select `legacy` or `metallic-roughness`; the material schema
describes fields from both source shapes. Effects cook to version-2 `.asfx`
records and run in fixed client-only pools. `effects.load/start/stop` control them;
`effects` reports particle/light counters. The Effects ImGui panel and
`effects.edit` share source/text/save/undo/load/start/stop actions. Source editing
uses loose `effects_source/*.json` files with backups and external-change checks;
run the cooker watcher to publish changes. Active bursts retain their parameters.
Full reference art, soft depth and final #161 scene/performance gates remain pending.
Match examples use development-only placeholder credentials.

## Editor metadata

`cvar.list` accepts a name/description `filter`, `offset` (0..65535) and `limit`
(1..16); it returns `items` with name/value/default/description/flags and `next`
(null at end). Restart pagination after cvar registration changes. `cvar.select`
takes `name`; `editor.filter` takes `kind` (cvars/images/materials) and `value`.
These use the same selection/filter state as the panels, visible in editor.state.
Material asset rows include `stageInfo` with presence/state bits/texture IDs.

`graph` action `tab` selects Preview, Source or Tables using `text`.
`graph.table` takes `section` (parameters/states/transitions/conditions/events/
nodes/masks/joints), offset and limit with the same bounds. Replies include total,
items and next. Indices refer to the loaded graph, parameter values are live, and
mask weights follow joint order. Load a cooked graph and step before querying it.

For authored-level validation, `trace` queries static compiled BSP collision:

```json
{"op":"trace","start":[0,0,64],"end":[0,0,-16],"hull":"point"}
```

`hull: "point"` uses solid contents for sightlines. `hull: "player"` uses the
standard `[-15,-15,-24]..[15,15,32]` box and solid/player-clip/body contents.
Coordinates must be finite and within world bounds; load a map first. The reply
contains `fraction`, `end`, `normal`, `start_solid`, `all_solid`, `contents` and
`surface_flags`. The existing engine collision epsilon remains in force (for
example, a floor trace can stop a standing player origin at 24.125).
This query covers static BSP collision; dynamic entities use the game queries.
It is available in development client and dedicated builds through the private
local channel. Simulation and shipping builds retain their existing behavior.


`python3 tools/agent build --sketch IMAGE --notes NOTES --theme manhattan --out DIR`
exposes the complete sketch-to-level pipeline described in
[`tools/level/README.md`](../level/README.md#complete-sketch-build). It returns the
same JSON report and nonzero failures. `--playtest FILE` follows the compiled-world
checks with a script in the same 20-ms session. Inputs, output ownership and kit
license checks are shared with `tools/level build`.
