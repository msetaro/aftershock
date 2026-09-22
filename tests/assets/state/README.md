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
