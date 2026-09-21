# Add a weapon

Run from the repository root after setting RECIPE_OUT to a fresh directory.
Start from our owned example, keep references to its model/graph, and change a
copy. Names are stable content identifiers; the cooker output is disposable.
This example adds `agent_rifle` with a 36-round magazine.

```sh
python3 - <<'PY'
import json, os, shutil
from pathlib import Path
out = Path(os.environ['RECIPE_OUT'])
source = out/'source'
shutil.copytree('tests/assets/weapons', source)
weapon = json.loads((source/'rifle.weapon.json').read_text())
weapon.update(name='agent_rifle', magazine=36)
(source/'agent.weapon.json').write_text(json.dumps(weapon, indent=2)+'\n')
(source/'agent.assets.json').write_text(json.dumps({'version':1,'assets':[
    {'name':'weapons/agent_rifle','kind':'weapon','source':'agent.weapon.json'}]}))
PY
python3 tools/agent validate weapon "$RECIPE_OUT/source/agent.weapon.json"
python3 tools/cook "$RECIPE_OUT/source/agent.assets.json" --output "$RECIPE_OUT/cooked"
python3 tests/weapons.py --output "$RECIPE_OUT/weapon-tests"
```

Cook referenced models/graphs from presentation.json before equipping the weapon
in a map. Use the channel's range inspect/select/fire and actor queries to inspect
it; tests/weapon_range.py is the full runtime example. A passing schema does not
replace gameplay, animation, replication or deterministic weapon gates.
