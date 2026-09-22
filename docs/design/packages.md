# Content packages (#20)

Implementation checkpoint: the offline writer/reader and patch test are available.
Native mounts and platform streams remain acceptance work; this document describes
the format they must consume. Existing pk3 reading stays available.

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
assets. Other assets use a zlib stream only when it saves bytes. There is no
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
| 120 | codec: 0 stored, 1 zlib | uint32 |
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
base content. Native mount precedence and data-root policy must agree with these
rules and preserve the existing pure-content restrictions.

The initial test uses the owned cooker fixture, changes exactly one texture,
checks the small delta, then applies a separate configuration-file removal.
No proprietary game assets or accepted regression fixtures are packaged.
