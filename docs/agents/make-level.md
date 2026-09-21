# Make and validate a level

CLIENT and SERVER must be development binaries. Set CONTENT/DATA for installed
Quake 3 or OpenArena. Copy our owned source; edit room, connection, cover and
spawn records while retaining stable IDs. This example renames the sample map.

```sh
python3 - <<'PY'
import json, os, shutil
from pathlib import Path
source = Path(os.environ['RECIPE_OUT'])/'source'
shutil.copytree('tests/assets/levels', source)
level = json.loads((source/'two_lane.json').read_text())
level['name'] = 'agent_level'
(source/'agent_level.json').write_text(json.dumps(level, indent=2)+'\n')
PY
python3 tools/agent validate level "$RECIPE_OUT/source/agent_level.json"
python3 tools/level validate "$RECIPE_OUT/source/agent_level.json" --output "$RECIPE_OUT/report" --client "$CLIENT" --server "$SERVER" --content "$CONTENT" --data "$DATA" --bot-frames 500
```

The command compiles MAP/BSP/AAS, validates structure and spawn paths, runs bots,
and writes named views, fly-through captures and a JSON report. Review the report
and images before accepting a design. The short bot run here checks the command;
use the default 6000 frames for design acceptance. No accepted fixtures are regenerated.

For a drawn layout, use `python3 tools/agent build --sketch IMAGE --notes NOTES
--theme manhattan --out DIR`. The [sketch build reference](../../tools/level/README.md#complete-sketch-build)
covers explicit interpretation/assumptions, licensed theme kits, recorded entrance
decisions, compiled overlap, native shooter/route checks and the full bot/capture
gate. An optional following playtest uses the same agent channel. Keep each
iteration in a fresh output directory and pass the previous interpretation for
stable IDs; no editor or accepted-fixture regeneration is required.
