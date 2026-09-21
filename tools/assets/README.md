# Theme assets

Use the level/cooker Python environment. Kits stay outside the source tree by
default; downloaded originals are cached by SHA256 under
`$XDG_CACHE_HOME/aftershock-assets` (normally `~/.cache/aftershock-assets`).

```sh
python3 tools/assets search --provider polyhaven --tag brick
python3 tools/assets search --provider ambientcg --tag paving
python3 tools/assets fetch --theme manhattan --out "$AFTERSHOCK_SCRATCH/manhattan"
python3 tools/assets validate "$AFTERSHOCK_SCRATCH/manhattan"
```

The reference lock pins Poly Haven `brick_wall_001` and ambientCG `PavingStones036`
at a 1024-pixel budget. Fetch checks every original SHA256, prepares native material
inputs, runs the existing cooker, validates complete provenance and publishes the
kit only after success. `--offline` requires all pinned source bytes in the cache.
A repeated output is reused only after its manifest and lock match. Choose another
output directory for a changed lock; existing kits are never silently replaced.

A kit contains original PBR channels, prepared images/material definitions,
ASMAT/KTX2 resources, the source lock, a license manifest and generated `CREDITS`.
AO/displacement are retained for authoring; the current material runtime has no
AO/displacement input. This tool does not claim that those channels render.

New pins are an explicit reviewed authoring operation, separate from fetch/CI:

```sh
python3 tools/assets pin --theme custom --resolution 1024 \
  --asset polyhaven:brick_wall_001:facade \
  --asset ambientcg:PavingStones036:ground --out new-assets.lock.json
python3 tools/assets fetch --theme custom --lock new-assets.lock.json --out new-kit
```

Pin refuses to overwrite a lock. Poly Haven download size/MD5 metadata is checked
before SHA256 pinning; ambientCG archives and extracted channels are pinned by
SHA256. Later fetch never silently resolves another revision. Texture reduction
reuses the production cooker's linear-color and normal-vector mip filtering.

Both enabled providers publish CC0 assets: [Poly Haven](https://docs.polyhaven.com/en/faq)
and [ambientCG](https://docs.ambientcg.com/license/). The adapters use their documented
keyless APIs, identify this application and add Poly Haven's required API service
credit. [Poly Haven API terms](https://github.com/Poly-Haven/Public-API/blob/master/ToS.md)
and [ambientCG API](https://docs.ambientcg.com/api/v2/full_json/) describe those endpoints.
Other providers are rejected. Repository-owned `policy.json` currently permits only
CC0-1.0 and the requested pinned Blender procedural tool; it permits no generated
image service. Additional licenses/generators need an explicit maintainer entry.
Proprietary files do not belong in this public repository.

Every asset file must occur in a manifest entry with source URL, author, license,
retrieval date, original/cooked SHA256 and attribution. Symlinks, unlisted files,
wrong hashes and unapproved licenses/generators fail validation. CI controls are
`tests/theme_assets.py`, `tests/theme_fetch.py` and `tests/asset_providers.py`; they
use owned inputs and offline metadata, while the reference kit is also exercised
against its real pinned sources during acceptance.
