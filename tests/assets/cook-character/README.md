# Owned Blender export fixture

This simple block character was authored by export.py with Blender 4.5.3 LTS.
It contains six textured meshes, one three-joint skin and two one-second clips
(`idle`, `wave`). It contains no Quake/OpenArena or other third-party game content;
the repository GPL-2.0-or-later license applies. provenance.json records the
verified official Blender archive and all source/output hashes.

CI cooks the committed glTF, binary and PNG; it does not invoke Blender or replace
these source fixtures. Explicit regeneration, followed by review and provenance
updates, is:

```
blender --background --factory-startup --python-exit-code 1 --python tests/assets/cook-character/export.py -- tests/assets/cook-character
```

The script is the complete authoring source. The exported files are not edited
by hand. This fixture supports #9's real Blender-to-engine proof; the small
stdlib-generated fixture in tests/cook.py separately tests incremental cooking.
