# Versioned profile migration fixture

`profile-v1.asstate.gz` is an original, frozen settings-profile artifact produced
once by the real engine at commit e199e8bc. It contains default OpenArena test-home
settings, volume 0.31 and F8 bound to +attack. It contains no paks, CD keys, private
cvars or absolute paths. `profile-v1.json` records byte hashes and record counts.
Gzip only keeps this mostly empty bounded-table fixture small; the engine consumes
the decompressed ASSTATE file unchanged.

Creation command (using a devtools client built at the recorded revision):

```
python3 tests/profile_runtime.py --binary CLIENT --content openarena --data DATA --record-v1-fixture
```

The creation flag requires a version-1 writer and refuses to overwrite an existing
fixture. Normal tests and CI only read it. Never regenerate this accepted old-format
artifact to accommodate a new reader; keep explicit migration support instead.

## Frozen full-game checkpoint v1

`checkpoint-v1.asstate.gz` is the one-time OpenArena `oa_dm1` capture from writer
`308b70c2fade4461c284dd77bcc6e09fe1c23aae`, with one local player and Sarge.
`checkpoint-v1.json` records the binary/source hashes, archive hashes and exact
saved/continued entity projections. Both same-process and fresh-process restore
and continuation passed before creation; the real-client screenshot was reviewed.
The gzip is 340,020 bytes; the uncompressed archive is 17,407,471 bytes. It contains
native/botlib mutable state and parsed OpenArena bot/chat strings, not paks, textures,
sounds or proprietary Quake 3 content. Saved botlib variables include the original
private temporary home path; current filesystem services still own actual access.

OpenArena content attribution: OpenArena Team and contributors, id Software, Inc.;
GPL-2.0-or-later, as recorded in Debian `openarena-data` 0.8.5split-16 copyright
metadata. Source content is available from the OpenArena project and Debian's
openarena-data source package. This derived checkpoint uses the same GPL terms;
see the repository COPYING file. The deterministic capture recipe is its source.

Explicit one-time creation, using the recorded v1 development client and installed
OpenArena content (never Quake 3 paks):

```
python3 tests/checkpoint_runtime.py --binary CLIENT --content openarena --data DATA --record-v1-fixture
```

The command refuses an existing archive or metadata file and verifies the v1
header. It writes only after exact continuation assertions pass. Normal tests/CI
must consume these frozen bytes, not invoke creation or rewrite the old fixture
when a reader changes. `checkpoint-review.png` is private review evidence in the
command output directory, not an accepted rendering golden.
