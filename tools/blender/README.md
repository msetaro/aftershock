# Procedural environment kit

```sh
python3 tools/blender kit --parameters parameters.json --out "$AFTERSHOCK_SCRATCH/modules"
```

```json
{"version":1,"seed":164,"bay_width":4,"storey_height":4,"wall_thickness":0.5}
```

Dimensions are meters; the existing native cooker applies 32 Quake units per meter.
The tool uses Blender **5.0.1**. Linux x86_64 downloads the official portable archive
from Blender's RWTH mirror into the user cache, verifies SHA256, and extracts with
safe data-file rules. It never installs system packages. `--blender PATH` accepts
an already installed 5.0.1 binary; other versions fail. Archive hash:
`8019580ee1b7262e505f4196a00237ccf743c88d205b38d34201510676e60b09`.

The original CC0 reference modules are facade, doorway, cornice, curb, stairs,
fence, barrier, crate and sign. Seeded parameters drive geometry. Blender applies
edge bevels, unwraps UVs, bakes material colors on the CPU with one thread, exports
glTF plus an OBJ for the map compiler, and generates a reduced triangle-count LOD.
The existing cooker emits native IQM and PBR resources for both LODs. Generated
`source/kit.json` records native bounds and measured triangle counts.

The complete kit has the same license manifest/CREDITS contract as fetched assets,
including Blender version and script/parameter hashes. Parameter hashes use sorted,
compact JSON. Source parameters, meshes, images and cooked bytes have individual
SHA256 entries. Log output sits beside the kit and is not an art asset. Existing
output directories are refused, so failed generation never replaces a previous kit.

`python3 tests/blender_kit.py` builds twice and compares every published file. The
reference art is rule-built environment content; this tool does not generate
characters, hands, first-person weapon models or weighted animation.
