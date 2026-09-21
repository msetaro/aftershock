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
