# Import a glTF character

Use glTF 2.0 with referenced buffers/images beside the source. The owned example
includes a skin and animation; copy it, then replace character.gltf and its
resources with your character. Set scale and fps in the recipe, not in engine code.

```sh
python3 - <<'PY'
import os, shutil
from pathlib import Path
shutil.copytree('tests/assets/cook-character', Path(os.environ['RECIPE_OUT'])/'source')
PY
python3 tools/cook "$RECIPE_OUT/source/assets.json" --output "$RECIPE_OUT/cooked"
python3 - <<'PY'
import os
from pathlib import Path
assert (Path(os.environ['RECIPE_OUT'])/'cooked/models/character.iqm').is_file()
PY
```

The cooker validates joints, weights and animation data and emits IQM plus its
manifest. In a development map, use animation.load with the cooked qpath, step,
then query editor.state.animation and capture its preview. See tools/cook/README.md
for scale, textures, reload and material recipes.
