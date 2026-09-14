# Permanent regression suite (#3, in progress)

Run from the repository root. Python 3, GNU Make, GCC or Clang/libc++, and binutils
are required. Tests call real engine functions built with Make's production flags;
only the test driver provides allocator/log/file stubs, as in the port G5 harness.
No engine source is rewritten. These Linux executable checks do not yet claim
MSVC or cross-target execution coverage.

```
python3 tests/run.py unit --negative-control
python3 tests/run.py unit --cc clang --cxx 'clang++ -stdlib=libc++' --output /tmp/tests-clang
python3 tests/run.py differential
python3 tests/run.py runtime
python3 tests/run.py unit --cc clang --cxx 'clang++ -stdlib=libc++' --sanitize --output /tmp/tests-sanitized
```

`unit` runs the twelve asset-free G5 groups. `differential` adds the original
q3dm17 1,000 collision sweeps; it requires user-owned `~/.q3a/baseq3/pak0.pk3`.
`runtime` requires faketime and all installed baseq3 paks, runs q3dm17/q3dm7 with
two bots, warms the cache and requires two identical traces before golden comparison.
Use `--data /path/to/baseq3` for another installation. Pak files never enter git.

The runtime command keeps timeout outside faketime, fixes SOURCE_DATE_EPOCH, and
uses an isolated home. Normalization removes only cached-paks / working-directory
lines and replaces the two installation directory prefixes with `<HOME>`/`<DATA>`;
all gameplay events are retained. Full raw logs remain under `--output`.

The negative control compiles a temporary copy of q_math.cpp whose active GCC SSE
Q_rsqrt return moves one ULP toward infinity; golden equality must fail. It never
edits the production file. Sanitizer mode uses tools/port/ubsan.supp unchanged;
leak checks are disabled because the inherited isolated hunk/zone test allocator
retains allocations to process exit. Address/undefined behavior checks remain fatal.

## Explicit golden regeneration

Only an intentional issue-scoped behavior change may regenerate affected goldens:

```
python3 tests/run.py unit --regenerate
python3 tests/run.py differential --regenerate
python3 tests/run.py runtime --regenerate
git diff -- tests/golden
```

Explain every changed group/event in the PR. `--regenerate` is rejected whenever
`CI` is set; CI must compare committed outputs, never generate replacements.
Initial goldens were generated from the unchanged merged port, then repeated.

## Outstanding acceptance

Demo recording/replay and fixed-frame hashes, network impairment/prediction bounds,
reader fuzzers and the complete CI matrix are being added. Current sanitizer run
exposes Huff_Decode's existing unaligned load; #31 owns its fix. Hosted runtime CI
has no licensed game-data provisioning or configured self-hosted runner yet.
These gaps are blockers, not skipped tests counted as passes; #3 is not complete.
