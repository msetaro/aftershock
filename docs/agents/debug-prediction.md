# Debug a prediction mismatch

Use the same content, map, fixed dt, seed and input script on both revisions.
CLIENT is a development binary. MAP, CONTENT and DATA select installed content.
The report exposes snapshots, predictions, error peak/sum, rewind statistics,
player state and per-step replies. Frame-time percentiles measure the host.

```sh
python3 tools/agent run --map "$MAP" --script docs/agents/prediction.json --out "$RECIPE_OUT/play" --binary "$CLIENT" --content "$CONTENT" --data "$DATA"
python3 - <<'PY'
import json, os
from pathlib import Path
report = json.loads((Path(os.environ['RECIPE_OUT'])/'play/report.json').read_text())
network = report['profile']['network']
assert network['snapshots'] > 0 and network['predictions'] > 0, network
print(json.dumps(network, indent=2))
PY
```

Find the first divergent state, then inspect the corresponding input, snapshot and
prediction counters. Test transport delay/loss separately with tests/netcode_runtime.py;
a local deterministic playthrough is not that transport test. Preserve the failure
as a #31 test before changing simulation expressions or accepted goldens.
