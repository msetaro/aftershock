# Owned animation acceptance sources

These original block rigs are authored entirely by `export.py` with Blender
4.5.3 LTS. They contain no third-party game content; the repository's
GPL-2.0-or-later license applies. `provenance.json` records the verified Blender
archive, exporter and all authoring/output hashes.

`rifle.gltf` has thirteen joints, thirteen meshes and six one-second clips:
idle, ADS, fire, reload, sprint and jump. Its arms have upper-arm/forearm/hand
chains, and its weapon has muzzle, magazine, optic and grip bones for sockets.
`body.gltf` has sixteen joints, fifteen meshes and ten clips: idle, walk, run,
aim up/down, crouch, prone, lean left/right and turn. Walk/run carry forward root
motion. Legs and arms provide two-bone IK chains; the spine subtree supports an
upper-body mask. Every clip has 31 samples at 30 FPS.

CI consumes the committed exports. It never launches Blender or reauthors them.
The explicit authoring command, followed by visual review and provenance updates,
is:

```
blender --background --factory-startup --python-exit-code 1 --python tests/assets/animation/export.py -- tests/assets/animation
```

Cook only these model sources with:

```
python3 tools/cook tests/assets/animation/assets.json --output /tmp/aftershock-animation-models
```

The sources prepare #10's acceptance content. They do not by themselves prove the
runtime state machine, events, IK, gameplay replication or demo parity. Those
require the feature's native and in-game tests. Existing #9 sources and all
accepted demo/frame fixtures remain unchanged.

The authored `rifle.animation.json` and `body.animation.json` graphs use these
unchanged exports. Cook the complete graph/model set with:

```
python3 tools/cook tests/assets/animation/rigs.json --output /tmp/aftershock-animation-rigs
python3 tests/animation.py
```

The rifle graph covers idle/ADS/fire/reload/sprint/jump and names shot, shell,
magazine and bolt events with attachment bones. The body graph blends idle/walk/
run and crouch/prone, adds aim/lean on the spine subtree, and emits alternating
footsteps. New graph sources are hand-authored data, not regenerated Blender
exports or accepted demo goldens. Native tests exercise both graphs; in-game
presentation is exercised by `tests/animation_runtime.py`;
`tests/animation_demo.py` replays the separate fixed #10 fixtures against their
recorded server hit-box traces. See tests/README.md for explicit fixture authoring.
