# Offline cooker (#9, in progress)

Offline mesh/texture cooking and native KTX2 BC loading are available. Cooked
material loading and development reload are still being implemented; this is not
the complete #9 acceptance.

Use Python with the pinned Pillow dependency, CMake 3.25+, Ninja and a host C++
compiler. Install Python requirements in a virtual environment, not system Python.
The small pinned BC7 helper builds into the user's cache on its first invocation.
It is never linked into the engine. `CXX` selects its host compiler.

```
python3 tools/cook tests/assets/cook-character/assets.json --output /tmp/aftershock-cooked
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
texture path. Rendering these materials is part of the next runtime slice;
restrained PBR remains #13.

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
qpath. All integers are little-endian. Runtime records will retain explicit layout
and copy assertions when the runtime slice lands.

The native KTX2 reader verifies the embedded content hash using pinned
[amosnier/sha-2](https://github.com/amosnier/sha-2/tree/565f65009bdd98267361b17d50cddd7c9beb3e6c),
under its 0BSD option. The backend enables supported BC compression and checks
format capabilities before allocation, following the [Vulkan feature contract](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html).
