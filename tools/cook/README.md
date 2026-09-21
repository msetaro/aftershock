# Offline cooker (#9, in progress)

For related commands, allocate one root first: `export AFTERSHOCK_SCRATCH="$(mktemp -d)"`.
Otherwise each invocation gets a fresh root; children inherit it. Retain the root
for logs/build reuse, then remove it when its evidence is no longer needed.

Offline mesh/texture cooking and native KTX2 BC loading are available. Cooked
material loading and development texture/material/model/animation reload are available.
Audio/shader inputs are available; final acceptance remains before #9 is complete.

Use Python with the pinned Pillow dependency, CMake 3.25+, Ninja and a host C++
compiler. Install Python requirements in a virtual environment, not system Python.
The small pinned BC7 helper builds into the user's cache on its first invocation.
It is never linked into the engine. `CXX` selects its host compiler.

```
python3 tools/cook tests/assets/cook-character/assets.json --output $AFTERSHOCK_SCRATCH/aftershock-cooked
python3 tests/cook.py
```

A project is JSON version 1 with an `assets` array. Each asset has a unique
lowercase relative `name`, a `kind` and a source path relative to the project.
All sources must remain inside that directory, including glTF buffers/images.

```
{"version": 1, "assets": [
  {"name": "models/character", "kind": "model", "source": "character.gltf", "scale": 32, "fps": 30},
  {"name": "textures/wall", "kind": "texture", "source": "wall.png", "format": "bc7", "srgb": true}
]}
```

Models accept glTF or GLB and emit IQM v2, retaining the existing native geometry
and skeletal path. The coordinate conversion is `(x,y,z) -> (x,-z,y)` and the
explicit scale defaults to 32 engine units per meter. Static node transforms are
baked; mirrored transforms preserve face winding. Skin bind matrices and sampled
STEP/LINEAR/CUBICSPLINE clips become plain IQM records. Each clip starts at its
first source key. Oversized surfaces are split to the existing tessellation limits.
The current native limits remain 128 joints, four influences per vertex and a
16 MiB IQM file. Skeletal shear, animated morph targets, required glTF extensions
and nonzero/transformed material UV sets are currently rejected with diagnostics.
IQM source loading remains supported by the engine; shipping code imports no glTF.

Textures emit standard KTX2 without supercompression. BC7 supports linear or sRGB
color; BC5/BC4 are linear data. PNG/TGA and glTF image bytes use Pillow decoding.
Mip filtering uses premultiplied linear values; `normal: true` renormalizes filtered
normal vectors. The BC7 encoder is pinned by source hashes; BC5 uses Pillow, and
BC4 uses its independently encoded red-channel blocks. Material imports emit a
versioned plain `.asmat` record containing base color, alpha/culling flags and its
texture path. Existing recipes retain this v1 base-color compatibility path.
New content opts into the metallic/roughness path described below.

Each asset gets a `.manifest.json` containing relative source paths, SHA-256
hashes, the recipe/tool hash and output hashes. Repeating an unchanged cook does
not rewrite outputs. Changed input or output bytes trigger only their dependent
assets. Payloads are replaced atomically, then their manifest is published. The
CLI writes one JSON result to stdout; build diagnostics go to stderr.

IQM carries an `aftershock.cook` extension: version 1, a 32-byte source hash and a
32-byte content hash. KTX2 uses equivalent named key/value records. The embedded
content hash covers the whole file with its own hash bytes zeroed. `.asmat` uses
an eight-byte magic, uint32 version/size and SHA-256 of its payload, followed by
four float color factors, float alpha cutoff, uint32 flags and a 64-byte texture
qpath. All integers are little-endian. Runtime records retain explicit layout and copy assertions.

The native KTX2 reader verifies the embedded content hash using pinned
[amosnier/sha-2](https://github.com/amosnier/sha-2/tree/565f65009bdd98267361b17d50cddd7c9beb3e6c),
under its 0BSD option. The backend enables supported BC compression and checks
format capabilities before allocation, following the [Vulkan feature contract](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html).

Standalone `kind: material` inputs are JSON with optional `texture` (a source image
relative to the material file), `baseColorFactor`, `doubleSided`, `unlit`,
`alphaMode` and `alphaCutoff`. They emit matching `.asmat` and private `.ktx2`
files. Base-color factors are baked in linear space; opaque materials discard
source alpha. This v1 compatibility path supports only mask cutoff 0.5.

Add `"material_model": "metallic-roughness"` to a model or standalone material
recipe to emit ASMAT v2. Model recipes use glTF's base-color, metallic/roughness,
normal and emissive textures and factors, alpha modes/cutoff and double-sided
flag. Optional `KHR_materials_unlit` is supported. A standalone material uses the
same material object, with `{"uri": "image.png"}` instead of a texture index.
For example:

```json
{
  "pbrMetallicRoughness": {
    "baseColorTexture": {"uri": "paint.png"},
    "baseColorFactor": [1, 0.8, 0.5, 1],
    "metallicFactor": 0,
    "roughnessFactor": 0.6
  },
  "normalTexture": {"uri": "normal.png", "scale": 1},
  "emissiveFactor": [0, 0, 0],
  "alphaMode": "OPAQUE"
}
```

The v2 payload is 240 bytes: eleven float32 values (base RGBA, emissive RGB,
metallic, roughness, normal scale, alpha cutoff), uint32 flags, and three 64-byte
texture qpaths. It uses the same 48-byte version/hash envelope. All factors remain
editable linear values; they are not baked into the textures. BC7 packs base RGB
and opacity A in sRGB, normal XYZ and roughness A in linear space, and emissive RGB
and metallic A in sRGB RGB/linear alpha. Normal/emissive mips filter data alpha
independently, avoiding opacity weighting. Packed source channels must have equal
dimensions, except a constant 1x1 channel can expand losslessly. Supported sampling
is repeat with linear mipmaps and UV0; the engine texture-quality setting remains
authoritative at runtime. Bake other samplers, UV transforms/sets,
material extensions and resolution mismatches before import. Occlusion is left
to authored lighting; an occlusion texture is rejected, not silently discarded.

The new renderer path uses GGX/Smith/Schlick and the existing model ambient and
dominant light direction. It consumes authored/skinned tangents; meshes without
tangents use a derivative basis. World batches use their center's existing light
grid sample. This is not a replacement for #14's lighting/shadow work. Existing
Quake scripts and ASMAT v1 keep their original shader programs and draw path.
PBR computes lighting in linear space and encodes its output for the existing
display-space UNORM target. Transparency uses that existing compositor; a linear
HDR compositor is part of the later lighting/postprocessing work, not this path.

ImGui's Materials tab edits v2 factors live; source reload replaces those edits.
The preview-instance checkbox applies separate factor overrides to the Animation
or graph preview. Cgame can use `CGameImport_R_AddMaterialEntityToScene` with a
`materialOverride_t` from the public renderer contract and an optional skeletal
pose/hash. Values are copied into the owning frame, selected by mask; unselected
factors and alpha/culling flags stay with the shared material. Change pipeline
flags through the source cook/reload. Existing refEntity and snapshot layouts stay
unchanged. Legacy recipes and all accepted frame/demo fixtures are preserved.

`--watch` polls source/output timestamps every 100 ms, recooks changed dependencies
and atomically publishes `cook.index` followed by its SHA-256 `cook.revision`.
The plain index is capped at 4,096 native resources and records each qpath, hash,
size and kind. Failed cooks retain the previous published revision. The output
must be the running development client's home game directory for reload polling.
After entering a local `devmap`, set `dev_reloadAssets 1`. Development renderer ABI
16 includes reload counts, named clips and a bounded loose-home-file read callback; shipping ABI is 12.

Texture reload is implemented: polling uses the existing real microsecond clock,
so it continues while `timescale 0` pauses the scene. Idle polling allocates no
engine memory. Each texture replacement waits for completed GPU use, retains the
image registry handle/descriptor, and reclaims the prior dedicated device memory.
Size changes use the same transaction; failure preserves the live image. Named clips are exposed by the public renderer API and the Animation inspector;
non-looping clips stop at their last frame. Model/animation replacement retains handles and replaces one zone allocation;
material replacement retains its shader/stage storage and updates draw ordering.
Inspectors report reload counts. Cooked development materials stay out of the
static vertex cache; shipping and legacy asset paths retain their existing behavior.

`kind: audio` accepts PCM WAV (8/16/24/32-bit) and Ogg Vorbis, mono or stereo at
8–192 kHz. It emits native PCM16 `.wav` without resampling, capped at 256 MiB of
decoded samples. The offline Vorbis helper reuses the vendored engine codec.
The standard RIFF `ASCK` provenance chunk contains uint32 version 1, source SHA-256
and whole-file SHA-256 with its own 32 bytes zeroed. Cooking/tests verify it; the
existing native WAV decoder skips this metadata and consumes the PCM directly.
The project index identifies these native WAV resources as kind 4.

`kind: shader` compiles GLSL vertex/fragment sources with pinned glslang 16.6.0.
Set `AFTERSHOCK_GLSLANG` to its executable or put `glslang` on PATH. `stage` is
`vert`/`frag` (defaults to the file suffix); optional `defines` contains simple
`NAME` or `NAME=value` strings. Literal quoted includes stay within the project and
participate in dependency hashes. Compilation runs from a snapshot of these sources.
The `.asspv` envelope uses magic `ASSPV\0\0\0`, uint32 version 1/payload size and
SHA-256 of its native SPIR-V payload, with the same 48-byte header layout as ASMAT.

To use a cooked shader in the current renderer, name it
`shaders/<existing-package-symbol>` (for example `shaders/color_vert_spv`), then
configure CMake with `-DCOOKED_SHADER_DIR=/absolute/path/to/cooker-output`. The
existing offline package builder verifies the envelope and requires exact reflected
interface records before embedding the payload in the client/module. The current
conservative comparison includes compiler IDs. Runtime GLSL compilation and shader
hot reload are not introduced. The 76-program package retains every byte of the
74 legacy shader programs and adds the two explicitly authored PBR programs.

Frame bounds are computed from stored quantized poses, including float vertex/bind
values and accumulation padding. Source tangents are retained when every mesh
provides them; otherwise that optional IQM array is omitted and PBR uses its
derivative tangent basis. Clip names must be unique and at most 63 UTF-8
bytes. Joint transforms preserve scale-before-rotation, including rotated
nonuniform scales; static transforms remain baked.
