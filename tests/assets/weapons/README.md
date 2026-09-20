Original GPL-2.0-or-later weapon parameters and range props for #11. The rifle
model reuses the unchanged owned #10 source. The new weapon graph aligns reload
notifies to weapon stages and adds a melee state using its existing fire clip.
`../range.json` cooks the complete owned range presentation. No installed game content is
copied here.

Weapon definition/cooked format version 2 adds an explicit projectile model and
collision half-size; it supersedes the unmerged first slice's version 1 payload.
`assets.json` cooks definitions. `presentation.json` cooks original impact
materials, optic/grenade box props and synthetic shot/reload sounds. The latter
sources and authoring script are hash-recorded in `provenance.json`.

Explicit authoring of these new props/sounds (follow with visual/audio review):

```
python3 tests/assets/weapons/export.py
python3 tools/cook tests/assets/weapons/presentation.json --output /tmp/range-assets
```

CI consumes committed sources and never runs the authoring command. Existing
animation exports, accepted demos and frame goldens are not regenerated.
