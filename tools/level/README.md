# Declarative levels (version 1, implementation pending)

The provisional format is JSON, matching the asset cooker and #18's data-format
decision. The compiler emits ordinary Quake 3 MAP, BSP and AAS content. Existing
Radiant maps remain usable. `tests/assets/levels/two_lane.json` is the complete
three-room, two-lane example; its art is owned GPL content.

```
python3 tools/level level.json --output /tmp/my-level/baseq3
python3 tools/level level.json --output /tmp/my-level/baseq3 --map-only
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
  in generated MAP source.

Compiler acceptance includes containment, player clearances, connected spawn
navigation, sightline limits, cover distance, asset existence, and raw repeated
output equality. Named viewpoints, fly-through screenshots and the extended
headless report belong to #27.

The sample images and cube are committed sources. Their explicit authoring command
is `python3 tests/assets/levels/export.py`; CI only verifies their provenance.
