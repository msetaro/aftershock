# Opus 1.6.1

Unmodified release from https://downloads.xiph.org/releases/opus/opus-1.6.1.tar.gz
Release SHA-256: `6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1`.
`UPSTREAM.sha256` records every imported release file; `COPYING` and the PATENTS
files retain the upstream notices. Imported for issue #16 voice playback.

Builds use a static C library, no downloaded dependencies, no optional neural
features or tools/tests, and caller-owned codec state. Configuration belongs in
`cmake/Audio.cmake`; do not reformat or port this vendor code.
