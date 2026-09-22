# Content packages (#20)

The offline writer and native platform-stream reader share this versioned format.
Filesystem mounts preserve legacy pk3 reading and the existing pure protocol.
The package is an install artifact; no game-code module or runtime loader is added.

## Commands

```
python3 tools/package.py build --root COOKED --output base.aspack --store 'sounds/*'
python3 tools/package.py build --root RECOOKED --output texture-patch.aspack --base base.aspack
python3 tools/package.py verify base.aspack texture-patch.aspack
python3 tools/package.py extract base.aspack texture-patch.aspack --output NEW_DIRECTORY
python3 tests/packages.py
```

Inputs are ordinary files with lowercase relative qpaths of at most 63 bytes.
The cooker already emits these paths. Its source-recipe `*.manifest.json` files
are omitted; runtime payloads, `cook.index` and `cook.revision` are included.
Individual assets are bounded to 256 MiB and a package to 65,536 index entries;
archive offsets and extents are 64-bit. `--store` can repeat to select uncompressed
assets. Other assets use a raw DEFLATE stream only when it saves bytes. There is no
wall-clock metadata, so unchanged inputs and options produce identical archives.

The writer publishes a complete temporary file with an atomic replacement. It
refuses output inside its content root or over a base package. Extraction requires
a new directory and publishes it only after all visible payload hashes pass.

## Layout, version 1

All integers are little-endian. The header and each index entry are 128 bytes.
Fields are written individually with explicit widths; runtime layouts need size
and offset assertions.

| Header offset | Field | Width |
| --- | --- | --- |
| 0 | `ASPACK\0\0` magic | 8 bytes |
| 8 | version (1) | uint32 |
| 12 | index entry count | uint32 |
| 16 | manifest offset | uint64 |
| 24 | manifest size | uint64 |
| 32 | resulting content identity | SHA256 |
| 64 | required base identity, zero for independent package | SHA256 |
| 96 | SHA256 of index bytes followed by manifest bytes | SHA256 |

| Index offset | Field | Width |
| --- | --- | --- |
| 0 | zero-padded, nul-terminated qpath | 64 bytes |
| 64 | asset offset | uint64 |
| 72 | uncompressed size | uint64 |
| 80 | stored size | uint64 |
| 88 | SHA256 of uncompressed bytes | 32 bytes |
| 120 | codec: 0 stored, 1 raw DEFLATE | uint32 |
| 124 | flags: 0 asset, 1 removal | uint32 |

Entries are strictly sorted by qpath. Payloads immediately follow the index in
entry order, without gaps, and the manifest follows the final payload. Removal
entries have zero extents/hash/codec and require a base identity. No bytes follow
the manifest. Metadata and payload hashes are checked separately.

The manifest is UTF-8 canonical JSON (sorted keys, compact separators), at most
16 MiB, with `version`, `identity`, nullable `base`, `assets` (qpath to SHA256 hex),
and sorted `removed` names. It describes exactly the index. Content identity is
SHA256 over sorted visible entries, concatenating each ASCII qpath, one nul byte,
and its 32-byte uncompressed content hash. Compression does not change identity.
Version 1 is unsigned; signing is optional scope and no key service is introduced.

## Patches and mounts

A full package is independent. A patch contains only new/changed payloads and
explicit removal entries, together with its required base and resulting content
identities. A mismatched base fails before extraction. Repeating `--base` supplies
a chain or composed baseline. The offline view mounts inputs in argument order;
later independent packages override earlier entries, while patches must match
the exact current view. This supports separate DLC/mod packages without rewriting
base content. Native mounts use the same composed identity and preserve existing pure-content
restrictions. Mount validation runs at startup and after a pure-server reorder;
missing bases or incorrect order fail explicitly.

The initial test uses the owned cooker fixture, changes exactly one texture,
checks the small delta, then applies a separate configuration-file removal.
No proprietary game assets or accepted regression fixtures are packaged.

The native decoder reuses the shipped `puff` implementation. Compressed assets
are decoded and hash-checked once at open, then read/seek from their bounded
buffer; uncompressed assets use platform file reads and 64-bit offsets. Both
paths must verify hashes before exposing bytes. Keep streaming audio stored via
`--store 'sounds/*'`. Add an incremental decoder only if measured compressed
stream sizes make per-open buffering unsuitable; no new compression library is
needed for current cooked assets.

## Directories and precedence

The three roots are selected at process startup on every supported platform:

| Data | Root | Layout and writes |
| --- | --- | --- |
| Engine | `fs_enginepath`, default `fs_basepath` | `<root>/engine/`; read only |
| Game | `fs_basepath` | `<root>/<fs_basegame>/` and `<root>/<fs_game>/`; read only |
| User | `fs_homepath` | `<root>/<active-game>/`; configs, saves, captures and downloaded legacy content |

User defaults retain Linux `~/.q3a` and macOS `~/Library/Application Support/Quake3`.
Windows now uses the OS application-data directory's `Quake3` subdirectory. If
no user directory is available, startup requires an explicit `fs_homepath` rather
than writing into the install root. Launchers may explicitly select roots;
legacy portable launches with equal roots remain supported.

Search precedence is the existing filesystem order: active mod above base games,
user above installed game data within each game, engine packages last. Archives
are ahead of loose directories. Within one directory, `.aspack` archives follow
legacy pk3/pk3dir mounts and sort by filename; later names win. Use ordered names
such as `00-base.aspack`, `10-patch.aspack`, `20-dlc.aspack`. Independent DLC/mod
packages may replace matching names. A patch's repeated `--base` inputs must be
the complete lower modern-package view in that order, including engine packages;
legacy pk3/loose files are excluded from that identity. A removal hides lower
packages and loose fallback. `autoexec.cfg` and `q3config.cfg` retain the legacy
archive exclusion so user configuration is not replaced by packaged settings.
At most 64 modern packages and 65,536 visible modern assets may be mounted.

The legacy pure handshake carries checksums derived from modern content and
metadata identities; asset bytes still receive SHA256 verification at open.
Modern packages must be installed together with their required bases. Missing
modern packages produce an explicit install error; the legacy pk3 downloader
does not rename or transfer this new artifact type. Version 1 provides integrity,
not publisher authentication. Do not edit mounted packages in place: publish a
new package and restart the filesystem/session. Stored assets retain an open
platform stream after verification; compressed assets retain a bounded decoded
buffer until close. Filesystem reads and seeks allocate no zone memory.

## Acceptance

`python3 tests/packages.py` checks reproducible actual cooked content, manifest
diffs, removals, per-asset hashes, native read/seek and complete resource release.
`python3 tests/packages_runtime.py --binary CLIENT --server SERVER` checks separate roots,
patch/removal/DLC/mod precedence, restarts, user writes and legacy gameplay.
With `--server`, it also reads packaged content on both peers during a pure session. It
also cooks the owned character into one package and proves a small texture-only
delta changes the rendered preview in a fresh client. The same command accepts
`--content openarena --data PATH` for hosted CI. No accepted game demo or golden
is regenerated, and neither test packages installed third-party game content.
