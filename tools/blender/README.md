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

OBJ exports use Y-up, as expected by the pinned q3map2 importer. Its `(x,-z,y)`
conversion restores the native Z-up bounds recorded in `kit.json`. The gate checks
that conversion explicitly; the compiled reference facade must reach its declared
128-unit height. glTF and native IQM retain their existing cooker conventions.


## Supplied rigs and motion clips

```sh
python3 tools/blender retarget --assets provided --parameters retarget.json --out "$AFTERSHOCK_SCRATCH/retargeted"
```

```json
{"version":1,"rig":"rig/character.gltf","clip":"clip/body.gltf","animation":"walk","bones":{"root":"root","arm.L":"upperarm.L","arm.R":"upperarm.R"},"fps":30,"root_motion_scale":1,"name":"models/provided","scale":32}
```

Both inputs are provided glTF 2.0/GLB assets with exactly one armature and applied
object transforms. The input directory needs the complete source/license/hash
manifest used by `tools/assets`; external buffers/images must remain within it.
The explicit bone map covers every target bone and maps its single root to a
source root. The script transfers supplied rest-to-pose rotations, preserves the
target bind offsets, and applies the stated root-motion scale. It bakes the named
clip at the stated frame rate and cooks the result to native IQM/PBR. Animated
scale, missing bones/clips and unverified dependencies fail explicitly. This is
retargeting supplied motion, not character/rig/weighted-animation generation.

The output contains the original supplied bytes, parameters, retargeted glTF,
measured transfer report, native cooked assets and a complete manifest/CREDITS.
Every source license remains attached to the derived output. Private conversion
can process provided assets outside the publication allowlist; its JSON explicitly
reports `publishable: false` in that case. `tools/assets validate` and theme
assembly still enforce the unchanged maintainer allowlist and reject that output.
No CLI option bypasses publication validation. Purchased/proprietary working files
remain outside the public repository; sourcing/allowlisting them is the maintainer's
decision. Fresh output ownership and staged failure handling match the kit command.

`tests/blender_retarget.py` consumes the existing, unchanged repository-owned GPL
character/body fixtures in private temporary directories. It checks mapped motion,
exported target inverse-bind matrices, exported root displacement, native cooking,
repeated output bytes, missing-bone/hash failures and rejection by the CC0 theme
publication gate. It never regenerates those accepted character sources or adds
new character art to the repository.
