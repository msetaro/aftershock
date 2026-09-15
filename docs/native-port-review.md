# Native game port evidence (#2)

The native-only decision retires legacy QVM/mod compatibility. The imported GPL
base-game sources become the game, cgame and UI implementations; the engine keeps
its existing simulation expressions, allocation model and longjmp error handling.
Static integration and VM removal are implemented; final CI review remains pending.

## Source and transformation audit

`native-game-import.json` records the original repository/revision, source/destination
paths and SHA256 values. All 130 original hashes were verified against revision
dbe4ddb10315479fc00086f08e25d968b4b43c49. At the catalog-port checkpoint, 30 files were verbatim, 96 had recorded
changes, and four retained existing engine ABI headers. Later static-lifecycle
adaptations are recorded per file in the same manifest. Each changed file references
its ABI adaptation, catalog or separate #31 fix commits. GPL notices are retained.

The C native baseline is 3306d55d. The ordinary catalog changes comprise T1 pointer
casts, T2 boolean-expression casts, T3 enum conversions, T4 the deleteButton member
rename, T5 six C-linkage exports, T8 read-only declarations and 1,060 literal/macro
casts in 71 files, T14 register removal, T15 eleven literal/macro separators, T17
three enum/float comparison casts, T20 const search results and T22 two integer abs
conversions. Fifteen diagnosed string-pointer arrays were qualified read-only.
No T25 alternate implementation was necessary. Commits and per-file membership are
in the manifest; no simulation expression was rearranged.

Recorded compatibility adaptations outside the catalog:

- The native C interface carries pointer-width syscall words and three integer
  command arguments. Twenty-nine structure sizes and three offsets match the engine.
- The QVM compiler used binary32 float and double. The native ABI preserves that
  behavior at math-call boundaries. Replacing compiler literal flags with 2,783
  explicit suffixes across 57 files left all 103 C and 103 C++ objects byte-identical.
- FOFS uses offsetof; three K&R sort helper definitions use their original prototype
  types. All 103 C objects remained byte-identical after both syntax adaptations.
- C++ uses the C99 library feature set. Clang's bg_lib object alone disables glibc's
  inline atof definition; this does not disable optimizer inlining. Its C object
  remained identical with and without that header setting.

Bugs found during import/verification were fixed separately in #31 PRs #49, #51–#57,
#58, #59 and #60. Their tests and explanations are in `cpp-port-notes.md`. #2 has
changed no accepted fixture or golden. The bot-command fix also preserves both
GCC/Clang C++ objects exactly (c0f322d0 and 22ddec96 respectively).

## Gates and review decisions

G1 uses GCC and Clang, C and C++20, with -Wall -Wextra -Werror. Every disabled warning
class in `tests/native-warnings.json` occurs in the original C inventory. The frozen
counts and disposition are documented in `tests/README.md`; cleanup remains #8.

`python3 -B tests/native_gates.py --tidy` reproduces G2/G3/G4/G7 with the existing
port normalizers, compiler/source identities, exact commands, logs and diffs. G2
and G3 pass all 103 module-specific objects locally and in hosted CI. G3 isolates
header optimizations with the established oracle flags plus -U__OPTIMIZE__; the
production assembly does not receive that extra isolation.

G4 is advisory. The 63 nonempty whole-object comparisons include function order,
local-static names, register allocation and the following reviewed differences:

| Area | Difference and disposition |
| --- | --- |
| Shared math | AngleVectors uses different scalar/vector packing and static reloads; operation dependencies are preserved and the 4,096-sample word differential agrees. |
| String helpers | C ctype macros versus C++ library calls, single-character strstr versus strchr, equivalent terminator-loop layouts and size-comparison widening. All nonzero-byte case results agree in the C locale. |
| Rendering/UI | Equivalent nonnegative enum comparisons, independent integer address calculations, color-register scheduling and block placement. Fixed replay samples agree on both maps/renderers. |
| Bot movement | POD return-slot construction removes a temporary copy in BotAttackMove. The zeroed/output result paths and arithmetic dependencies remain the same. |
| Bot goals | BotCTFSeekGoals uses mask 1878 for exactly {1,2,4,6,8,9,10}; negative/out-of-range values retain the non-member path. Other large goal diffs primarily move blocks and complementary branches. |
| Commands and state | Equality-test reordering, equivalent loop exits, no-argument C/C++ call metadata, and unchanged integer-to-size_t bound checks. G_AddRandomBot still calls rand when no bot is available. |

Decision: accept these advisory compiler differences with the source audit,
layout/symbol gates and runtime evidence, retaining the original diffs for review.
The flag initializer and command-byte issues found during review were corrected
in #59/#60; they are not explained away as compiler differences. This is measured
parity for the exercised inputs, not a formal equivalence proof over arbitrary data.

G7 reported 1,365 narrowing, 55 signed-char and nine implicit string-result
comparisons before the last fix. The string-result checks are ordinary nonzero
strcmp tests. The remaining report identifies inherited integer/float, size and
byte conversions; no blanket casts or tidy fixes were applied. Preserve these
source conversions for the warning/type work in #8, and route confirmed defects
through #31. The full native C++ UBSan bot smoke now passes both Q3 maps after #60.
The report remains visible and is not represented as zero findings.

G5/G6 evidence includes native C/C++ shared-function hashes (math 67988592, case
676e85f5), both Q3 bot logs and fixed-demo replay on GCC/Clang. OpenArena's pinned
external C modules also match both hosted-content bot logs and fixed replay.
The permanent runtime/replay tools require successful native loading and compare
against existing QVM goldens; they prohibit native regeneration.

The ABI/type audit and advisory decisions apply before static integration. All 93 implementation files were renamed with identical source bytes (100% git
rename similarity). The later static/direct-call change must rerun the runtime, layout and build gates before PR #50 is ready.

## Resident module initialization

Game d6c2ac52 and client/UI 436bbab1 restore state previously reset by DLL reloads.
The game arena is cleared at level initialization; bot pointers, timing counters,
preferences, spawn queue and per-level counts reset at their existing entry points.
Bot client-count consumers reuse the existing map-initialized global. Death and
movement diagnostic counters retain their original per-module lifetime.

The client resets its random/effect seeds, score-plum history, draw statistics,
loading counts, prediction state and particle rotation at module initialization.
The UI clears its state/arena and cached server counts. Menu structures initialize
on entry; resource handles are refreshed by existing cache functions. Temporary
string/math buffers are written before consumption and require no blanket reset.
The enabled base-game paths are covered; this does not claim missionpack coverage.

Test-first 413f1ed8 and a1cf3223 reproduce resident game/client differences. After
the adaptations, game restart/map-change logs match ordinary reloads twice, with
and without movement diagnostics. Fixed replay after video restart matches on both
maps/renderers with normal and retained storage. The separate accepted-golden
replay remains b38004b1. GCC/Clang native C++ builds and all 103 C/C++ layout/symbol
comparisons pass. Floating-point expressions and accepted fixtures remain unchanged.

## Static integration and platform follow-through

956eebfa replaces numbered engine dispatch with typed game/cgame/UI imports and
exports. A common wrapper compiles each production translation unit in its module
namespace; rand/srand/qsort/atof/memmove remain module-local compatibility functions.
OpenArena CI builds pinned C modules as isolated static objects with objcopy symbol
prefixes. Its alignment/free-list fixes came from separate #31 PRs #62/#63.
Static Q3/OA bot logs and fixed replay retain every accepted hash. The added Q3
lifecycle log references were existing reviewed DLL outputs; original goldens are
unchanged. Static lifecycle frames use the original accepted frame golden.

090b7a3c adds 17 T8 casts in enabled DEBUG AI diagnostics. GCC and Clang -O0 DEBUG
C objects for both changed files are byte-identical before/after. Uncalled Windows
byte-swap helpers and old macOS PPC helpers were removed from the imported header;
no game/cgame/UI caller exists. _WIN32 selects Windows definitions and _MSC_VER
limits MSVC pragmas. The old MSVC-only inline int3 becomes __debugbreak on MSVC
and __builtin_trap elsewhere; this preserves a debug failure at the same overflow
condition on new toolchains (GCC's trap terminates rather than resuming int3).
Release behavior and all FP expressions are unchanged. MSVC generates separate
build-directory wrappers matching the 103 Make source selections; /fp:strict and
disabled intrinsics preserve the native arithmetic/library configuration. Apple
SDK deprecations use the existing engine platform freeze (440089eb).

VM retirement test-first a8879635 rejects the linked old implementation. The
retirement removes eight VM/interpreter/JIT files plus active Make/MSVC and
startup/unload API hooks. File-format declarations remain for historical layout
oracles; no QVM parser/interpreter/compiler or game DLL loader remains. Static
smoke and replay inspect every executable for required init exports and absence
of VM_* implementation symbols. The old VM_Call probe and retained-DLL comparison
shim are retired with those obsolete paths. Original C/C++ compiler oracles remain.

After retirement, both Q3 bot logs and both-map/both-renderer lifecycle replay
retain accepted hashes. Lifetime analysis passes 550 compile commands/138 source
paths, including all native wrapper selections. No new wire/file structure layout,
simulation FP expression, OS access or allocation was introduced by the deletion.
The bot synonym import keeps the same 1024-byte legacy limit via MAX_STRING_CHARS;
retiring its VM constant changes no service behavior.
