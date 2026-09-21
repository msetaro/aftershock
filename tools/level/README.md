# Declarative levels (version 1)

For related commands, allocate one root first: `export AFTERSHOCK_SCRATCH="$(mktemp -d)"`.
Otherwise each invocation gets a fresh root; children inherit it. Retain the root
for logs/build reuse, then remove it when its evidence is no longer needed.

The provisional format is JSON, matching the asset cooker and #18's data-format
decision. The compiler emits ordinary Quake 3 MAP, BSP and AAS content. Existing
Radiant maps remain usable. `tests/assets/levels/two_lane.json` is the complete
three-room, two-lane example; its art is owned GPL content.

```
python3 tools/level level.json --output $AFTERSHOCK_SCRATCH/my-level/baseq3
python3 tools/level level.json --output $AFTERSHOCK_SCRATCH/my-level/baseq3 --map-only
```

The first command also runs pinned q3map2 and MBSPC; the second only emits the MAP
and validates design rules. Successful stdout is one JSON object containing
`version`, `name`, `map`, `bsp`, `aas`, `sha256` and `report`. Paths are relative to
the output content directory. Errors use stderr and a nonzero exit status; no
success report is emitted. CI never records or replaces fixtures.

All coordinates/dimensions are integer Quake units. Rooms and passages are aligned
with the world axes. A room's `origin` is its horizontal center and floor height;
`size` is width, depth and clear height. Walls/floors/ceilings are 16 units thick.
Entity `origin` values are world coordinates. A player spawn origin is 24 units
above its floor. Unknown fields, duplicate IDs and non-finite values are errors.

The root object contains:

- `version: 1`, and a lowercase `name` used for output filenames.
- `materials`: role-to-qpath mapping for `floor`, `wall`, `trim`, `cover`, `prop`
  and `sky`. Each qpath resolves to an owned image in the adjacent `assets/textures`
  directory. Generated shaders supply sky/lighting intent and collision-only faces.
- `rules`: `min_corridor_width`, `min_door_height`, `max_sightline` and
  `max_cover_gap`, all positive distances. Physical player clearance is always
  enforced even when an authored rule requests less.
- `rooms`: `{id, origin: [x,y,floor], size: [width,depth,height]}` records.
  Room interiors must not overlap.
- `connections`: `{id, from, to, axis, at, width, height}`. `axis` is `x` or `y`;
  `from` is on the lower end of that axis. `at` is the absolute center along the
  perpendicular axis. Both openings must fit their rooms. Corridors are generated
  across the gap. An optional `door: true` places a sliding door at the first
  opening. Different floor heights require `transition: "stairs"` or `"ramp"`;
  stairs use steps no higher than 16 units, and ramps stay within walking slope.
- `spawns`: `{team: "ffa"|"red"|"blue", origin, angle}`. Team records emit the
  standard CTF start/respawn entities. All spawns must reach the same walkable area.
- `cover`: `{id, origin, kit: "low"|"tall"}`. Origin is the floor center;
  the kits are 64 by 32 units and 48/96 units tall. Optional `size` overrides the
  kit dimensions. These are solid brush pieces.
- `props`: `{id, model, origin, size, material, solid}`. `model` is a relative
  path under `assets`; `size` gives the centered validation/collision box.
  `material` names a material role. Meshes are baked by q3map2; solid props also
  emit a clip box. The sample supplies an owned OBJ cube.
- `pickups`: `{classname, origin}` using supported native Quake pickup names.
- `lighting`: `ambient`, optional `sun: {direction, color, intensity}` and
  `lights: [{id, origin, color, intensity}]`. Colors are linear RGB in 0..1;
  sun direction points from the sky toward the map. Named lights remain editable
  in generated MAP source. Optional `directional: true` bakes paired intensity and
  model-space light-direction pages with pinned q3map2 `-deluxe -deluxemode 0`.
  It retains the BSP light grid for dynamic-object probes. Omitted/false preserves
  the accepted default bake. Existing MAP projects opt in with worldspawn key
  `_aftershock_deluxe` set to `1`; `0` disables it. Both compiler entry points read
  that key from the compiled worldspawn before lighting. The output remains IBSP
  46, with even surface lightmap indices and the corresponding direction page
  immediately following each intensity page. The #14 renderer consumes these
  directions separately from color data; ordinary Quake renderers still use the
  intensity pages. `python3 tests/lighting.py --compile` checks repeated output and
  unchanged default fixtures without recording references.

Compiler acceptance includes containment, player clearances, connected spawn
navigation, sightline limits, cover distance, asset existence, and raw repeated
output equality. Named viewpoints, fly-through screenshots and the extended
headless report belong to #27.

The sample images and cube are committed sources. Their explicit authoring command
is `python3 tests/assets/levels/export.py`; CI only verifies their provenance.

The permanent gates are `python3 tests/level.py` for the language/design rules and
`python3 tests/level.py --compile` for repeated pinned MAP/BSP/AAS output plus
committed fixture comparison. Initial fixture authoring is explicit:
`python3 tests/level.py --compile --record-fixtures`. It writes only
`tests/golden/levels/two_lane.{map,bsp,aas}` and is refused in CI. Review generated
geometry and actual bot pathing before accepting those new artifacts.

MAP-only mode needs only Python 3. The pinned full compiler currently runs on Linux
x86_64. Install `tools/level/requirements.txt` in a Python venv (libarchive-c 5.3,
using system libarchive) for the first archive extraction. The compiler downloads
NetRadiant-custom 20260114 into `$XDG_CACHE_HOME/aftershock-level-tools` (default
`~/.cache`), verifies SHA256
`f48f6f1d0db2b910ef9cb5dc5d8a722852510f3c5c278dc17615c0466b8a7a3d`, then uses
its q3map2 and MBSPC binaries and bundled libraries. Subsequent runs work offline.
No editor or tool bundle is committed. BSP/VIS/light and AAS stages use one thread;
BSP alignment padding outside all declared lumps is zeroed before the AAS checksum.
Declared lump bytes are never changed. Build diagnostics are in `compile.log`.

Version 1 deliberately restricts geometry: horizontal room dimensions and corridor
widths are even units, rooms keep at least 32 units between their interiors, passages do not intersect,
and props are self-contained OBJ meshes whose vertices fit their declared bounds.
Cover and props must fit inside one room. Walls need 16 units beside each opening;
passages need at least a 32-unit gap.
Transitions allow at most a 1:2 slope; stair risers are at most 16 units and treads
at least 32. Props are baked with their assigned role, without external MTL files.

Design rules are conservative and reported explicitly. Navigation tests a 16-unit
world grid with a 30-by-30-by-56 player box and at most an 18-unit step, including
midpoints of each traversed edge. Doors are assumed openable. All spawn cells must
be connected after cover and solid props are removed. This is an authoring check;
the compiled AAS and actual bots remain the runtime acceptance gate. Sightlines
use the whole layout's 3D diagonal as a safe upper bound, so large winding levels
may be rejected despite their walls blocking long views. Cover spacing measures
horizontal straight-line distance to the nearest cover footprint, conservatively
adding the half-cell diagonal; it is not travel distance or a promise of occlusion.
Limits cap the description at 1 MiB, each entity list at 128 and the navigation grid
at 262144 cells. These are authoring limits, not engine format limits.


## Headless validation (#27)

```
python3 tools/level validate tests/assets/levels/two_lane.json --output $AFTERSHOCK_SCRATCH/level-report --client PATH/quake3e.x64 --server PATH/quake3e.ded.x64
```

Both binaries must use `-DAFTERSHOCK_DEVTOOLS=ON`. Install the same pinned
`requirements.txt` in a venv; Pillow encodes screenshots as PNG. Xvfb, faketime and
Mesa lavapipe are required, but a display is not. Content defaults to installed
`~/.q3a/baseq3`; use `--content openarena --data $AFTERSHOCK_SCRATCH/aftershock-openarena-baseoa`
for hosted tests. Licensed packages are only symlinked into the private runtime
home after compilation and are never published as artifacts.

Optional `viewpoints` in the level JSON contain `{id, origin: [x,y,z], angles:
[pitch,yaw,roll]}` records. Origins are exact eye positions in empty world space.
Names are unique lowercase identifiers, at most 32 characters; `auto_` is reserved.
There are at most 64 named views. These fields do not change MAP/BSP/AAS output.
The command samples a deterministic walk through all connected authored passages,
returning through each visited edge, with room, approach, middle and exit captures.
The development spectator camera is motionless and does not use teleport effects
or velocity. Existing gameplay camera and teleport behavior are unchanged.

Stdout is one version-1 JSON report, also saved as `report.json`; `report.txt` is a
readable rendering of it. Failure uses nonzero status, `status: "failed"` and
explicit `errors`. Compile and native load logs are retained. The report contains
compiler/runtime warnings, design/leak/missing-asset errors, entity class counts,
spawn reachability, AAS area count, lightmap pages and the fraction of BSP draw
surfaces with lightmaps. Sky and deliberately unlit surfaces count in that
fraction's denominator. It is surface coverage, not texel occupancy. Navmesh
coverage is `null` until #21 provides that backend.

Every view lists its PNG path and SHA256, actual Vulkan draw commands in the last
submitted frame and frontend triangle count. Draw calls include indexed,
non-indexed, visibility and postprocessing commands; triangle counts use the
existing renderer's scene counters. Measurements come from a stable camera before
capture. Repeated validation uses fixed 20-ms simulation steps and the same
faketime/Mesa setup; tests compare PNG bytes and metrics directly, without recording
new demo or frame goldens.

Bot smoke defaults to 6000 frames (`--bot-frames 500..30000`). It reports kills,
item pickups and authoritative position samples every 50 frames. A possible stuck
bot stays alive within 16 units of its initial sample over ten samples. Death
breaks the window. This is a reported inactivity heuristic: stationary combat can
also trigger it, so it is a warning, not a pathfinding diagnosis or automatic
failure. Generated command scripts are capped at 60000 bytes, below the engine's
command buffer capacity; reduce level/camera complexity if that bound is reached.

Existing `.map` input is also accepted with adjacent loose project `assets/`.
Its design-rule/grid reachability fields are unavailable and reported as such;
compilation, leaks, native loading and bot smoke still run. Automatic cameras come
from deathmatch spawns. `--viewpoints FILE` can supply the same camera array for a
MAP, with syntax/bounds checks but no declarative world-space containment test.
MAP input is capped at 16 MiB. No package files are copied into the tool workspace.
The acceptance test removes a ceiling from an owned valid MAP and requires a clear
leak failure; disconnected JSON spawns fail separately before engine startup.

## Reflection probes (#14)

Bake authored loose content using a development client and installed game data:

```sh
python3 tools/level/probes.py probes.json --client /path/quake3e.x64 \
  --base /path/authored/baseq3 --data ~/.q3a/baseq3 --output /path/authored/baseq3
```

```json
{"map":"two_lane","probes":[{"origin":[0,0,96],"radius":512}]}
```

Use `--content openarena --data $AFTERSHOCK_SCRATCH/aftershock-openarena-baseoa` for hosted
content. The tool symlinks installed paks only in its temporary capture directory.
It runs six square views in the native client under Xvfb/lavapipe, converts display
RGB to linear radiance, then filters five roughness levels. `--size` selects
16/32/64/128 pixels per face (default 32); `--samples` selects 32/64/128/256
GGX importance samples (default 64). The sampling equations follow
[Filament's IBL derivation](https://google.github.io/filament/main/filament.html#annex/importancesamplingfortheibl).
Captures are LDR and do not claim HDR radiance. Each probe should be placed in
free space near its intended dynamic objects; diffuse lighting still uses the
BSP light grid baked by q3map2.

The output is `maps/<map>.asprobe`: little-endian magic `ASPROBE\0`, four uint32
fields (version 1, count, face size, roughness levels 5), followed per probe by
four floats (XYZ/radius) and RGBA8 pixels. Each atlas has six face columns in
+X/-X/+Y/-Y/+Z/-Z order and five roughness rows. Count is bounded at 32, position
magnitude/radius at 32752. Renderer validation requires exact lengths and finite
coordinates; uploads keep the already-linear bytes unchanged. At the default
size each atlas consumes 120 KiB before GPU allocation alignment.

Set `r_reflectionProbes 1` to enable the bounded two-probe blend on dynamic PBR
objects. The default is off. Influence is spherical, without box parallax
correction; future authored room volumes can extend that approximation. The
#161 HDR renderer transition owns replacing LDR capture/composition.

## Version 2 polygon geometry (#164, in progress)

Version 1 retains its original generator and exact fixtures. Version 2 shares its
materials, rules, spawns, pickups and lighting, replacing rooms/connections/cover
with `boundary` and `shapes`. Discover the complete structural contract using
`python3 tools/agent describe level`. Install the pinned level requirements for v2.

The boundary has a `polygon` ring, optional `holes`, `floor` and `ceiling` heights.
A shape has a stable `id`, `kind`, `shape`, `base`, `height` and optional material
role. Footprints accept `polygon` with optional holes, `rectangle` with center,
size and angle, `circle` with center/radius/tolerance, `arc` with those fields plus
start/end/thickness, or `path` with points/thickness. Coordinates are finite Quake
units. Curve tolerance is the maximum radial chord error, with at most 256 segments.
Pinned Shapely constrained triangulation preserves concavity and holes in brushes.

`solid`, `platform`, `wall` and `overhead` extrude their footprints. A wall may set
`bullet_solid: false` for player clipping with bullet-transparent collision.
`transition` uses a straight two-point path and `transition: stairs|ramp`; its
height rises along the path unless `descending: true`. Existing minimum width,
1:2 slope, 16-unit riser and 32-unit tread limits apply. A `building` subtracts its
interior with `wall_thickness` (default 16), and accepts explicit `openings` with
zero-based edge index, distance `at` along that edge, width, sill and height.
Exterior polygon edges run counter-clockwise; rectangle edge zero is its lower
edge before rotation. Floors/roof have 16-unit slabs. Multi-floor buildings generate interior switchback stairs and subtract matching
stairwells from their floor slabs. `roof_access: true` extends the stairs through
the roof. The compiler rejects footprints that cannot fit walking clearances.
`opening_rules` accepts face_point, spacing, width, sill, height and optional
zero-based floors: edges facing that street/plaza point receive repeated openings.
Theme props and the complete sketch pipeline remain unfinished; this is not final
#164 acceptance.

`python3 tests/level_polygons.py --compile` checks physical brush occupancy,
player-clearance reachability, repeated MAP/BSP/AAS bytes and unchanged v1 fixtures.
Navigation retains separate surfaces at the same horizontal coordinate, checks
square player clearance and midpoint traversal with at most an 18-unit step.
The full sketch interpretation/theme/intent pipeline is the remaining #164 work.
