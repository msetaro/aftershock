# Add an effect authoring file

Effect simulation/cooking/rendering is pending #161. This recipe creates and
validates the versioned authoring contract only; it does not claim a visible effect.

```sh
python3 tools/agent describe effect > "$RECIPE_OUT/description.json"
python3 - <<'PY'
import json, os
from pathlib import Path
out = Path(os.environ['RECIPE_OUT'])
example = json.loads((out/'description.json').read_text())['example']
(out/'sparks.effect.json').write_text(json.dumps(example, indent=2)+'\n')
PY
python3 tools/agent validate effect "$RECIPE_OUT/sparks.effect.json"
```

When #161 adds the loader, extend this recipe with cooking, runtime capture and
bounded-pool checks. Existing weapon impact material recipes are separate from
this particle format.
