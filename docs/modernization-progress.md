# Modernization checkpoint

Integration: `modernization`. Issue branches: `issue/<number>-<slug>`, one bug per
#31 PR and one warning class per #8 PR. Merge commits only after gates/self-review.
Never push main, force-push, rewrite history, or touch port-evidence. Stop after #8
and a design-only `docs/design/rhi.md` for #6; no #6/#7 implementation.

Maintainer ruling (2026-09-19): keep all future changes and PRs in
`msetaro/aftershock`. Do not create PRs against ec-/Quake3e or another parent
repository. This replaces the earlier requirement to submit applicable #31 fixes
upstream; historical upstream PR references below are completed past work.

## Next action

Active: issue/8-unreachable-code. PR #106 at bea6347c passes build 35462676712
and regression 35462676707, with self-review on #106/#8; merged c4047adf.
The build's sole initial failure was an MSYS mirror timeout before compilation;
its isolated retry passed. Check the merged-tree regression next.

Applied the reviewed C4702 preview to 15 source/header files and promoted C4702
on owned C++ sources. Real MSVC x64/ARM64 Debug/Release diagnostic run 35462960543
passes with C4244/C4702 promoted. All 151 affected syntax configurations and four
game helper hashes/layouts pass. Of 169 production objects, 127 are raw/native
identical and 24 differ only in debug information. The final 18 debug objects
change only allocation diagnostic source-line immediates; every other stripped
byte is identical (unreachable-line-review.json). No reachable arithmetic change,
new OS calls, allocation, non-trivial lifetime, layout, accepted fixture or golden
change. Source 6bd5c657 records the cleanup; original GPL import hashes retain the
transformation for both native files. Run hosted gates, then self-review before merging.
Optional MISSIONPACK object comparison also compiles all nine configurations:
five release objects are identical; two Clang release objects share the lightning
bounce decrement/check between continuing paths, and two GCC debug objects move
the same increment onto its sole continue edge. Ten-bounce limit and arithmetic
are retained. This cache-only comparison demotes the pre-existing enum arithmetic
warning at g_weapon.cpp:1099; no production warning policy changed. Evidence:
unreachable-missionpack/results.json and unreachable-missionpack-*.diff.
Next warning: review the single C4701 debug bot diagnostic initialization guard.

The C4611 annotation is merged: only standard-MSVC Q_setjmp is annotated, the #1
trivial-lifetime gate remains, and C4611 is an error on owned C++ sources. All 28
local production objects remain raw/native-identical; real MSVC x64/ARM64 controls
confirm the bare call fails and the annotation passes. No exception-model change.

Applied the reviewed narrowing-v5-preview to 174 source/header files and promoted
C4244 on owned C++ sources, removing both inherited C4244 suppressions. Source
6709136c records the change; original GPL import hashes are retained with this
transformation attached to all 70 changed native files. Only inherited trailing
whitespace on 58 touched lines was trimmed after preview verification. PR #106 head bea6347c passes
regression 35462676707. Build 35462676712 passed all compiled legs; the MinGW
release leg failed before compilation while downloading a ccache package signature
from an MSYS mirror. Only that failed leg was rerun and passed. All twelve committed-tree
helper hashes/layouts match the post-formatter baseline (numeric-final-native.json).

All conversions remain at their original arithmetic boundaries; RHS temporaries
preserve compound-assignment evaluation order where needed. No dynamic allocation,
new OS calls, non-trivial lifetime or wire/file layout change. Accepted fixtures
and goldens are unchanged. One Vulkan error diagnostic now stringifies the explicit
(uint64_t) fence timeout cast; timeout value and normal rendering are unchanged.

The merged C4324 declaration preserves real MSVC x64 JPEG jump offset 176 and
record size/alignment 432/16; ARM64 remains offset 168 and size/alignment 360/8.
Six local production objects are raw/native-identical; three debug objects differ
by exactly one allocation source-line byte (207 -> 214), with every other stripped
byte identical. Artifacts: alignment-padding-* and msvc-jpeg-layout-*.log.

Next finish remaining warnings/Apple deprecations, format/tidy/layout/assert rules,
and #6 design only. All work and PRs stay in msetaro/aftershock.

Read-only layout inventory at 63615d82: GCC/Clang agree on sizes, alignment and
trivial standard layout for 59 central wire/model/BSP/AAS records. Cache evidence:
layout-inventory/results.json. This is a baseline, not completed #8 coverage; local
image records, platform cache records and native mirrored declarations still need
review. No source assertions/type changes applied by the inventory.
A fresh diagnostic-only branch issue/8-warning-inventory-current at a05b6ccf
is based on 63615d82, includes the verified C4611 preview, preserves source line
counts while exposing inherited header diagnostics, and uses /W4 for Debug.
Never merge that branch/workflow; harvest its warning inventory for the next
C4244 class. All four inventory legs pass in run 35460712936: 1,734 unique
C4244 sites, 37 C4702 sites, and one C4701 site in debug bot diagnostics.
The latter two classes need separate review; the potentially-uninitialized warning
appears to involve repeated botDeveloper guards, not established runtime failure.
Current warning inventory and refreshed Clang AST evidence use current-msvc-* and
current-narrowing-ast* in cache. All actions remain inside msetaro/aftershock.
The refreshed AST covers 165 source files (one transient Clang crash in tr_noise
passed an isolated retry). Seven Windows/header files and inactive debug paths
need explicit coverage. First cache-only current-narrowing-preview: 1,667 casts
in 154 files; all 1,527 syntax configurations and twelve native helper hashes/
layouts match the post-formatter baseline. All 1,709 production objects preserve
code/data: 1,361 raw/native, 342 debug-only, six verified symbol-table/equivalent
unwind-record ordering/padding differences in be_aas_reach. Section bytes,
relocations, symbol values/sizes and normalized unwind records were checked.
Evidence: current-narrowing-object-review.json, current-narrowing-metadata-review.json.
Diagnostic-only 04daa92a passes inventory 35461209732 with 371 remaining C4244
sites (the AST estimate had predicted 361); never merge that branch.

Second preview narrowing-v2-preview adds scalar-macro conversions and 97 compound
assignments: all 1,611 syntax configurations pass. It exposed compound-assignment
evaluation/load-order changes and must not be applied. Third preview uses RHS
scalar temporaries at four native and 34 engine sites, restoring all twelve native
helper hashes. GCC/MinGW/AArch64 release refinements match; Clang still schedules
some engine operations differently, requiring source/codegen review and replay.

Applied candidate narrowing-v5-preview includes 43 local vector-macro expansions
that cast only each final component, explicit float conversion in mirrored
SnapVector declarations, and 87 Windows/header/inactive/typedef sites. All 2,380
syntax configurations pass; all twelve GCC/Clang C/C++ helper hashes/layouts match
post-formatter-native.json. All four real MSVC configurations pass diagnostic run
35462301210 at 972dcedb with zero C4244 warnings; C4702/C4701 remain separate work.

Full object comparison: 2,667 owned-source configurations, 2,243 raw/native-identical,
335 debug-only. Remaining 89 include debug scalar-temporary/symbol/unwind metadata
changes, Clang engine instruction scheduling/register allocation in seven source
files, and the Vulkan error string above (plus relocation offsets). No claim that
all objects are byte-identical. Evidence: narrowing-v2-objects/v5-results.json and
narrowing-v5-object-review.json. Original full arithmetic expressions and target
conversions are retained; full regressions are required by the #8 oracle rule.
Clang checks from the diagnostic tree pass unchanged Q3 frame golden b38004b1,
collision differential 9674cd22, and both bot-smoke goldens fea77580/14c8ee7d. The
smoke uses the already documented cache-only host-IP metadata filter on expected
and actual logs; gameplay output is unchanged. Artifacts: narrowing-clang-demo.log,
narrowing-clang-differential.log, narrowing-clang-runtime.log. No accepted fixture
or golden regeneration. Hosted CI will run the ordinary OpenArena gates unmodified.

All twelve GCC/Clang C/C++ native helper builds/layouts pass after formatter fixes
#99/#100; post-formatter-native.json at 4b159f7e is the cache reference for future
warning comparisons. Pre-fix #94 helper hashes are obsolete for that purpose.
No accepted fixture/golden was changed.

#97 local-shadow source 91a4341b preserves 19 production objects (15 raw/native,
four debug-only) and all four edited-tree cgame helper hashes/layouts. #96 global
shadow source af07f013 preserves 30 objects (24 raw/native, six debug-only) and
all four edited-tree game helper hashes/layouts. Original GPL hashes remain.

#95 merged-tree regression
35456678897 passes. #96 global-shadow source af07f013 preserves all 30 production
objects (24 raw/native, six debug-only) and all four final game helper hashes/layouts.

Source 91a4341b applies the C4456 local-shadow preview in three files: flat particle
width/height, fog pipeline definition, Vulkan result/memory/descriptor locals.
MSVC C4456 becomes an error on owned C++ sources. Nineteen production objects
preserve code/data (15 raw/native, four debug-only); four cgame helper libraries
retain preview hashes/layouts. All four final edited-tree cgame helper hashes/layouts match #94.
PR #97 passed all hosted gates and is merged. No FP expression, OS access, allocation, lifetime, layout, fixture or
golden changes. Artifacts: local-shadow-* in persistent cache. Record source and
GPL provenance, then hosted gates/self-review before merging.

#95 merged 5e343bd5: seven local alpha token renames preserve nine production
objects and four cgame helpers. Its generated-VS correction scopes promoted MSVC
warnings to owned C++ source properties; all 358 GNU/361 MinGW commands were
byte-identical. Add future warning promotions to that source-property list.

Completed size conversions #94: source 3eaba64d/provenance 36d419dd records 53
existing narrowing casts in 25 GPL files, with MSVC C4267 promoted to an error.
All 269 production objects preserve code/data (211 raw/native, 58 debug-only).
Nine helpers retain hashes; three GCC C libraries have reviewed equivalent
low-32-bit selections/subtractions, addresses and padding. All twelve edited-tree
helpers/layouts reproduce reviewed hashes. 1,584 before/after qsort cases pass
(4705a47e). Original expression evaluation precedes each cast. No FP expressions
or accepted fixtures/goldens changed. Artifacts: size-conversion-*.

Completed default-only switches #93: source f9dbcb07/provenance a5e141d9 removes
two AAS wrappers and one UI wrapper, retaining the exact unconditional statements.
MSVC C4065 is an error. Nineteen release objects are raw/native-identical; nine
debug objects differ only in no-ops and addresses, with branch target instruction
indices and all remaining instructions/relocations verified. Four native UI
libraries/layouts retain hashes. Artifacts: default-switch-*.

Completed standard offset #92: source cf4f6f1e replaces the DirectInput wheel
macro with standard offsetof and promotes MSVC C4644 to an error. All three
MinGW release/debug production objects retain raw/native hashes; hosted MSVC
x64/ARM64 debug/release pass. Artifacts: offsetof-preview and offsetof-objects.

Completed string constness #91: sources c08ed1e9/4a47cf0c qualify sixteen
read-only declarations/fields across thirteen engine files and enable GCC/Clang
and MSVC strict string checking. All 2,667 syntax configurations pass; all 578
production objects preserve code/data (310 raw/native, 214 debug/six approved
const-parameter manglings, 54 MinGW symbol-order-only). Fixed Q3 replay retains
b38004b1. The default local smoke failed only on rotated host IPv6 addresses;
a retained cache-only wrapper excludes exactly `^IP6?: .*` lines from expected
and actual logs, and both maps pass (fea77580/14c8ee7d). No harness or accepted
golden changes. Hosted OA runtime passes unmodified. Artifacts: write-strings-*.

Retained warning previews (global/local shadowing merged; engine-size applied):
- global-shadow C4459: four files, 30 production objects preserve code/data
  (24 raw/native, six debug-only); four native game libraries/layouts unchanged.
- local-shadow C4456: three files, 19 production objects preserve code/data
  (15 raw/native, four debug-only); four native cgame libraries/layouts unchanged.
Each preview has source changes, commands, objects and logs in persistent cache.
Engine-size C4267 preview: 160 diagnosed lines in 45 C++ files, plus removal of
one engine header suppression. The final preview preserves all 508 production
objects (401 raw/native hashes, 107 debug-only). Casts follow complete original
expressions; compound sums retain size_t arithmetic before the final conversion.
An early text-wide preview incorrectly narrowed a same-text size_t assignment;
the debug oracle caught it. Edits now address only diagnosed line numbers, and all
objects pass. The verified preview is now applied on this branch. Evidence: engine-size-*.
C4200 preview, now applied: remove the nonstandard trailing flexible
member from pcx_t, assert its unchanged 128-byte header size, and use the address
immediately after the header for its payload. Nine production objects preserve
code/data (seven raw/native, two debug-only). No parsing behavior changes or new
test target. Remove the header suppression and add owned-source /we4200 only in
its eventual #8 PR. Artifacts: flex-array-preview, flex-array-objects/review.json.
C4127 preview, now applied: literal true loops, false disabled branches,
constexpr endian check, and compile-time glconfig size checks; remove both shared
header suppressions. All 73 objects preserve code/data: 57 raw/native, 16 debug-only. The
ABI assertion keeps its original two-line span so debug allocation __LINE__ values
remain unchanged. The first constexpr-false branch preview
made HSVtoRGB unneeded under Clang; plain literal false preserves its existing
reference. MSVC's documented trivial-constant exemption covers this form; require
hosted confirmation. Microsoft reference:
https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
Artifacts: constant-condition-preview, constant-condition-objects and
constant-condition-review.json.
C4201 preview, now applied: name the transform and scaleOffset records in both
renderer texture-modifier unions, qualify their member accesses, and remove the
engine/GL header's C4201 suppressions. All 27 changed production objects preserve
code/data (21 raw/native, six debug-only); 18 before/after compiler configurations
confirm size 28, alignment 4, member offsets 4/20/4/12 and trivial standard layout.
Add owned-source /we4201 and run hosted gates when applying. Artifacts:
anonymous-struct-preview, anonymous-struct-objects, anonymous-struct-review.json,
anonymous-struct-layout/results.json. Do not overwrite later header changes from
whole preview files; apply the recorded replacement pairs.
Formatting preview: clang-format 21.1.8 touches 398 of 407 first-party C/C++/inc
files, excluding assembly and generated shader_data.cpp. Eighteen stringifying
macros are whitespace-sensitive. All 1,810 release assembly comparisons compile;
1,802 are raw-identical (including every Clang leg). Eight GCC/MinGW sys_runtime
comparisons differ only in source-position comments on inline cpuid instructions;
all fifteen corresponding release/native objects are byte-identical, and four
GCC debug objects differ only in debug sections. Nineteen native export assembly
comparisons also pass after including .inc files. Rebase the preview on the final
warning revision and reverify before the single formatting commit; do not apply
yet. Evidence: format-* and prepared-warning-and-format-evidence.json.
A cache-only clang-tidy inventory completes 570 configurations with no compile
failures for performance-*, readability (duplicate include, misleading indentation,
redundant control flow), and one advisory modernize check (redundant void args).
Raw findings include repeated headers: enum-size 18,464; void-args 54,215;
misleading indentation 51; redundant control flow 604; int-to-ptr 20; duplicate
include 8. Unique locations are in tidy-inventory/unique.json. Enum shrinking and
integer/pointer rewrites are not authorized by these suggestions; preserve layout
and codegen. No config/source changes for tidy yet; retain advisory reports and
review the readability checks after formatting before selecting error gates.

Completed signedness #90 evidence: source 70b1f0b6/provenance 4a96ca16 records
412 edits in 91 files (15 GPL files). All 2,667 syntax configurations pass. Of
905 production objects, 755 are raw/native-identical and 150 differ only in debug
sections. Ten native helper libraries retain hashes; GCC C game/UI differ only in
reviewed register reuse, independent moves and equality operand ordering in four
functions. All twelve edited-tree helper builds reproduce reviewed hashes/layouts.
The final helper warning freeze and its reader were removed. No FP expression or
accepted fixture/golden changes. Artifacts: sign-compare-* in persistent cache.

MSVC inventory from #90 release x64 job 105922991488 (msvc-sign-release.log):
C4267 234, C4459 38, C4456 28, C4065 15, C4457 3, C4644 3. Review each class
before enabling its error gate, then /WX. Local tools include clang-query-21.

Further warning audit: shared headers still contain inherited MSVC pragma
suppression lists (engine/qcommon/q_shared.h, game/bg/q_shared.h); the platform GL
header also leaks warning disables beyond SDK includes. The visible six-class
inventory is not the complete MSVC warning inventory. #94 covers game-side C4267;
the engine header still disables that diagnostic and needs a separate follow-up.
Diagnostic-only branch issue/8-msvc-inventory at 1f5b1398 runs the unsuppressed
/W4 x64/ARM64 Release/Debug inventory in 35455896498. Its worktree is in the
persistent cache; do not merge that branch or its inventory-only workflow. All
four inventory legs pass. Retained msvc-header-*.log and msvc-header-{inventory,
unique}.json record 1,686 C4244 file/line sites, 160 C4267, 14 C4127, four C4201,
one C4200, one C4324 and two ARM64 C4611 sites, plus the prepared shadow classes.
The Debug-only extra C4456 occurrence is in VK_CHECK and is covered by the
prepared macro-local rename. Apple deprecation inventory 35457048020 passes from diagnostic commit c8029349 on the
same unmergeable branch; MSVC inventory is not rerun. All Apple configurations
report only sprintf/vsprintf deprecations (61 unique call sites in 22 files).
Retained apple-deprecations*.log/json. Review buffer ownership/bounds before
changing calls; any actual overflow fix belongs in a test-first #31 PR. The inventory does not establish that currently
quiet legacy pragmas are obsolete; consult diagnostics and test each removal.
C4244 cache-only Clang AST inventory finishes 165 available source files with no
compile failures; 3,605 conversion/compound nodes include 896 macro expansions.
Seven Windows/header paths lack matching Clang commands, and Debug-only branches
need separate coverage. No casts are applied from this inventory; distinguish
existing narrowing from widening, preserve FP expressions, and inspect macros.
Evidence: narrowing-ast.py, narrowing-ast/results.json and per-file logs. The
source baseline is 222f7439; recompute byte offsets after engine-size changes.
Continue removing applicable header suppressions
by class before /WX; review obsolete C-only diagnostics and vendor-only scopes
separately. Do not claim unrestricted MSVC warnings yet. Preserve existing numeric
conversions, layouts and FP codegen; route actual behavior fixes through #31.

Next:
The two reproduced formatter capacity defects are fixed in separate test-first
PRs #99/#100. Tests/format.py is permanent, covers six compiler/language variants,
and runs in CI. Reproduction/fix evidence is in docs/bugs.md and format-/va-*
cache artifacts. Both fixes retain accepted replay goldens.

1. Finish C4201 hosted gates/self-review and verify #102 merged-tree regression.
2. Apply verified C4324/C4611 previews separately, then finish Apple deprecations
   and remaining MSVC warning classes /WX.

3. Finish one verified tree-wide clang-format commit, tidy subsets, fixed-width
   types/layout assertions and release-identical Q_ASSERT. Update plan rules to in
   force; finish #8, write design-only docs/design/rhi.md for #6, then stop.
   No #6/#7 implementation or accepted golden regeneration for warning changes.

Recent merges (all self-reviewed; merge commits):
- #70 ignored qualifiers: 17a9dae2, build 35016076778/regression 35016076784;
  merged 9d9dc4f6, merged-tree regression 35016803376 passed. The broad *.txt CI
  ignore was removed because it incorrectly excluded CMakeLists.txt.
- #71 chat sentinel: 98b457cc, build 35017297266/regression 35017297166;
  merged c04ba916, merged-tree regression 35018077437 passed; upstream #442.
- #72 unused functions: e6dfa0ba, build 35018151616/regression 35018151602;
  merged 01a1dda8, merged-tree regression 35018822894 passed.
- #73 internal declarations: 248dca68, build 35018897499/regression 35018897591;
  merged be2186e0, merged-tree regression 35019634702 passed.
- #74 type limits: 644741e9, build 35019711765/regression 35019711780;
  merged 5fb78853, merged-tree regression 35020339038 passed.
- #75 unused constants: da94649e, build 35020481804/regression 35020481815;
  merged b28beab5, merged-tree regression 35021218286 passed.
- #76 native helper internal declarations: 4c598f3e, build 35021362798/regression
  35021362817; merged 7f5f90b8, merged-tree regression 35022204641 passed.
- #77 native helper fallthrough: 2c17b152, build 35022303013/regression 35022302958;
  merged 28b89692, merged-tree regression 35023041315 passed.
#5 is complete. #8 warning ratchet remains active; later #8 rules are not done.

Completed PR #75 evidence:
- Source 68919a83 moves the order table/type/count in cg_servercmds.cpp under the
  existing MISSIONPACK guard with its sole user, retaining source line count.
  Original GPL hashes remain; the transformation is recorded in native provenance.
- Production GCC uses -Wunused-const-variable=2 because level 1 excludes source
  files included by the native namespace wrapper. Actual-wrapper negative control
  verifies level 2. Clang omits unused constants in included files; GCC enforces
  that case. Both compiler freezes are removed from tests/native-warnings.json,
  and the standalone helper explicitly enables the class.
- All 858 native production commands pass with this class treated as an error,
  covering GCC/Clang release, GCC debug, MinGW and ARM64. All 412 captured native
  GCC/Clang release objects retain identical raw hashes. Four standalone GCC/Clang
  C/C++ helper builds and ABI layout checks pass. Artifacts:
  /tmp/aftershock-unused-constant-builds and unused-constant-native-*.
- Affected MinGW native objects match after incremental LTO. Debug .text is
  identical; remaining .rodata is an exact suffix after 88 unused bytes, and all
  71 shifted references per object preserve referenced bytes. No function changes.
  MISSIONPACK preprocessed output is identical. Its unchanged compile baseline has
  an int-to-qboolean error at cg_servercmds.cpp:936; this unsupported configuration
  limitation is recorded in docs/bugs.md/#31, not repaired in the warning PR.
  /tmp/aftershock-unused-constant-preview/{results,review}.json.
- No FP expressions, OS access, lifetimes, allocations or goldens/fixtures changed.

Completed #71 evidence:
Test-first 36410f00 exercises actual BotFindMatch, BotMatchVariable and
BotExpandChatMessage: unsigned-char returns Q instead of empty for the -1 marker;
signed-char passes. Fix c58e2751 makes both engine/game declarations signed char.
GCC/Clang ASan+UBSan pass with both defaults, preserving 8/328-byte layouts.
Across 26 production objects: 12 raw matches, six MinGW native matches, six debug
objects differ only in debug sections, and two ARM64 objects change only four
chat-offset consumers. No added/removed functions or unrelated instructions.
Unit/collision regeneration was byte-identical (8d44421d/9674cd22); Q3 smoke
6dad7c18/a15c9c91 and fixed replay b38004b1 are unchanged. Upstream C f694bbbc
reproduces the failure and passes 98691272 in ec-/Quake3e #442. No expected-failure
entry/suppression applied. Artifacts /tmp/aftershock-chat-offset-*.

Retained #8 warning evidence and upcoming previews:
- Ignored qualifiers: 590 GCC client objects match; GCC/Clang flag controls pass.
  /tmp/aftershock-ignored-qualifiers-before.
- Unused functions: 364 Clang engine objects match and flag control passes.
  /tmp/aftershock-unused-function.
- Internal declarations: 206 Clang native objects match; actual-wrapper control
  rejects a function used only by decltype. /tmp/aftershock-native-warning-check.
- Type limits: 728 GCC/Clang engine objects match. Clang needs explicit -Wtype-limits;
  both controls reject unsigned < 0. /tmp/aftershock-type-limits-{control,check}.
- Parentheses-equality preview: remove redundant inner parentheses from the
  tournament test in g_cmds.cpp. Nine objects checked: release/native objects match;
  two GCC debug objects differ only in debug sections. Applied and merged in #78.
- Self-assign preview: replace fov_x self-assignment with a comment; retain the
  existing branch, arithmetic and distinct cg.refdef.fov_x assignment. All eight
  GCC/Clang/debug and MinGW native objects match. Applied on this branch.
  Both source previews have failing/passing Clang wrapper controls:
  /tmp/aftershock-small-warning-preview. Each needs its own PR and provenance.
- Null-pointer-subtraction preview: use (uintptr_t)a for qsort alignment and
  explicitly include stdint.h; retain the existing long-sized swap algorithm.
  All 25 production objects preserve instructions/relocations; release/MinGW native
  hashes match and six debug objects differ only in debug sections. Clang controls
  fail before/pass after. /tmp/aftershock-null-subtraction-preview. Applied here.
- Address/pointer-bool preview: remove the impossible !classname stack-array guard
  in BotGetActivateGoal; preserve existing empty-classname behavior. x86 release
  and MinGW native objects match; two debug objects differ only in debug sections.
  ARM64 swaps operands of one fcmp feeding b.ne. Review confirms equality/unordered
  results are symmetric; fallthrough immediately overwrites flags with another
  fcmp, and the taken path overwrites them with the stack-canary subs before any
  further condition reads. No source FP expression changes; all other instructions
  and relocations match. Full regression remains required for the eventual PR.
  /tmp/aftershock-address-preview. Applied on this branch.
- Formatting preflight: local clang-format is 21.1.8. VK_CHECK stringifies its
  argument, so its call whitespace must be preserved by the eventual formatter
  configuration. Allocator __LINE__ macros are debug-only. Do not start the single
  tree-wide formatting commit until the warning ratchet is complete.
- MSVC release inventory: C4267, C4459, C4456, C4065, C4457 and C4644, from #69 job
  104469265974. /tmp/aftershock-msvc-warning-inventory.log. Address before /WX.

## Completed affinity fixes and #8 baseline evidence

The entries below preserve historical evidence. Only the Next action above directs
resumed work; earlier next-action wording below describes its original checkpoint.

Hex test-first 985a3f1f extends the existing affinity test with exactly 0xZ and
bare 0x and enables ASan alongside UBSan. It fails on the wrong mask and terminator
read before the fix. Keeping hex_code's result in signed int until validation
fixes all 18 cases through the private helper and intercepted public apply path.
Both common.cpp callers and recursion were already reviewed. There is no real
OS affinity change in the test. Upstream C f694bbbc independently reproduces both
failures and passes the same hex-only fix under GCC/Clang ASan+UBSan; its ten cases
exclude the separate pending operator bug. /tmp/aftershock-affinity-hex-*.log.
No expected-failure entry/suppression covers this new regression.

Hex codegen review covers nine GCC/Clang/debug/MinGW/aarch64 production objects:
only parseAffinityMask changes; no function is added/removed and all unrelated
function instructions/relocations match. Artifacts /tmp/aftershock-affinity-hex-codegen.
Explicit unit/collision regeneration is byte-identical (8d44421d / 9674cd22).
Local Q3 bot logs and fixed replay pass unchanged (6dad7c18/a15c9c91, b38004b1).
Hosted build 34995107292 and regression 34995107235 passed; #69 merged 2d9fa2ca. No source FP,
wire/file layout, allocation or OS-call change; signed int hex is trivially
destructible. No golden/fixture change is intended or accepted for this fix.

Next #8 class: ignored qualifiers. The broad warning inventory reports no existing
diagnostics in this class, so only its CMake suppression needs removal. GCC and
Clang production-flag controls accept a const-qualified scalar return while the
suppression is present and reject it when enabled. Captured 590 current client
objects across both renderers at dc3a6a81 for direct before/after hash comparison:
/tmp/aftershock-ignored-qualifiers-before. No engine source or fixture change is
needed for that class. Keep it in a separate #8 branch/PR after #69 is ready.

Operator fix: preserve the + or - before recursive operand consumption. Test-first
e84a1f6e fails; all 16 valid cases now pass GCC/Clang UBSan through both helper and
public entry point. Common initialization and cvar-update callers were reviewed.
Upstream C f694bbbc fails the same probe (with its original Com_SetAffinityMask
name) and passes the same fix under both compilers. Explicit unit and collision
golden regeneration is unchanged (8d44421d / 9674cd22). No engine FP, layout,
allocation, OS call or lifetime change. No expectation/suppression applies.

#68 local gates pass: Q3 bot hashes 6dad7c18/a15c9c91 and both-renderer fixed
replay b38004b1 are unchanged. Nine production-object configurations retain every
function symbol; only parseAffinityMask instructions change. GCC/MinGW SnapVector
constant labels/offsets and ARM64 CPU-name strings move because the now-unreachable
empty string is omitted. Seven explicit byte checks verify the referenced constants
are identical. Artifacts /tmp/aftershock-affinity-codegen/{results,constants}.json;
/tmp/aftershock-affinity-{runtime,demo}.log. Scope self-review passes: one operator
bug, no FP/layout/OS-call/allocation/lifetime changes, no applicable expectation or
suppression. Build 34951709954 and regression 34951709967 passed; PR #68 merged b4db52c4.
Verified PR head: 83899a7de9e25ff2cfa6b2c105eb322ecad7a60d.

The separate hex-sentinel reproducer now confirms both symptoms: 0xZ becomes
UINT64_MAX and bare 0x causes ASan global-buffer-overflow. Temporary two-case
extension /tmp/aftershock-affinity-hex-probe.cpp; output
/tmp/aftershock-affinity-hex-before.log. Make this a separate test-first #31 PR.

#31 operator test-first checkpoint: tests/affinity.py compiles the actual private
parser/public apply implementation with UBSan. Sixteen valid expression cases
cover constants, 64-bit values, aliases and mixed operator order. The OS setter
is intercepted. The permanent test fails on the current source as expected
(/tmp/aftershock-affinity-test-first.log); both compiler CI unit jobs now run it.
This was the failing checkpoint before the operator fix.

#8 baseline: 2,380 production C++ objects/diagnostics across GCC/Clang release,
GCC debug, MinGW and aarch64 server configurations. Both renderers covered where
applicable. /tmp/aftershock-warning-before and /tmp/aftershock-warning-cross-before
contain compiler commands, raw hashes and logs. Six full compilation controls fail
with -Werror=implicit-fallthrough, including debug game and native Windows paths;
syntax-only compilation does not emit GCC's fallthrough diagnostic and is not used
as the gate. /tmp/aftershock-fallthrough-before-*.log records those expected failures.
Potential behavior bugs from other warning classes require #31 disposition; do not
correct them in this PR. No goldens/fixtures are regenerated.

#8 fallthrough implementation: six comments document existing transitions in file
append mode, preprocessor subtraction, SDL fallback settings, Windows key dispatch,
UI radio input and the debug game error path. Existing comments suffice for the
GCC warning and preserve the retained C oracle sources; no new portability macro
is needed. Source line counts are preserved so debug metadata can also match.
The warning suppression is removed from engine and native C++ compiler lists.
Unit golden 8d44421d and the one-ULP negative control pass. Object review passes: 2,372/2,380 raw objects match; the eight MinGW LTO
containers differ, but incremental LTO linking produces byte-identical native
objects (including data and relocations) for all eight. No LTO option is removed
from production. Thirteen initial Unix differences were unpinned __TIME__; the
focused before/after repeat uses the existing test SOURCE_DATE_EPOCH and all 13
match. Original hashes/diagnostics remain recorded. Review evidence and driver:
/tmp/aftershock-fallthrough-review*, /tmp/aftershock-fallthrough-results.json.
Original imports/notices/hashes are retained; provenance records a5c199bf.
Hosted gates and final self-review remain required.

First hosted #67 run found vendored minizip still inheriting engine warnings.
The CMake correction applies the plan's -w (/w on MSVC) vendor policy through
explicit existing source lists. Owned engine/game code retains the fallthrough
gate. Full local non-SDL debug client/server build passes; vendor comparison across nine configurations passes: 433/577 raw objects
match and all 144 differing MinGW LTO containers produce byte-identical native
objects after LTO linking. Artifacts /tmp/aftershock-vendor-warning-parity and
/tmp/aftershock-vendor-lto-review. No vendored source is edited.

Two affinity-helper bugs are confirmed by direct calls that do not apply CPU
affinity: valid 1+2 and 3-1 yield 1 and 3; 0xZ yields UINT64_MAX. docs/bugs.md
records the distinct operator-consumption and unsigned-sentinel causes with their
reproducer. Fix only in separate #31 PRs with failing tests first. Other warning
candidates remain unconfirmed and must not be silently changed during the ratchet.
The operator bug's temporary UBSan probe covers 16 valid numeric/alias/compound
expressions through both the private parser and public apply path. It fails on
compound expressions before any fix. Sys_SetAffinityMask is intercepted, so the
probe changes no process affinity. /tmp/aftershock-affinity-operators-probe.cpp and
/tmp/aftershock-affinity-operators-before.log. Copy this to a permanent test in its
own #31 branch and commit the failing test before changing engine code.

#67 self-review: one warning class; six comments preserve control flow and line
counts; explicit source lists scope vendor flags. No FP/layout/OS-call/allocation
or lifetime changes; accepted goldens/fixtures unchanged. Native provenance and
object/LTO review are recorded. Corrected hosted runs 34950827218 (build) and
34950827333 (regression) passed; #67 merged as 43ad68ab.

## #5 completed verification

Final source 9c170ddd passes full build 34948420894 and regression 34948420906.
Provenance checkpoint 3f91d501 passes build 34948630311 and regression 34948630289.
CMake-only retirement e67f397e passed build 34947650438 and regression 34947650429,
including hosted MinGW curl/zlib linkage and artifact staging. Migration checkpoint
390a20f4 passed every hosted raw-object and generated MSVC gate in 34946471284.
Embedded-debug 48733686 passed migration 34946795374, regression 34946795290 and
full build 34946795283; MSVC x64 debug reports 720/720 cacheable calls, 52 hits.
Native provenance records both shared GPL file transformations in 9c170ddd,
preserving original source hashes and notices.

Self-review: #5 build/platform scope only; no new portable OS access, non-trivial
core destructors, per-frame allocation or simulation FP expression changes.
Supported function bodies and raw-object differences are reviewed below. Existing
wire/file assertions remain; all 103 native layout/symbol gates pass. No accepted
golden or fixture changed. Regression probes use production CMake objects. Generated
MSVC and every required hosted compiler/configuration pass. Final review removes
one trailing empty CMake line; no command or source setting changes.

Makefile, game/modules.mk and handwritten MSVC projects are removed after parity.
CMake-only build.yml keeps Linux/macOS/Windows release/debug binaries and release
artifact jobs, adds Clang and caches, and builds generated VS projects. Its CRLF
convention is retained. Replay the retired migration oracle at 390a20f4. Release
workflow and install staging pass locally; staged Linux binaries match build
outputs byte for byte. Bundled macOS SDL uses @executable_path for adjacent staging.
The local optional MinGW curl configuration compiles but cannot link absent target
zlib; hosted MSYS installs zlib explicitly. No local packages or assets are copied.

Inactive-platform cleanup removes x86/ARM32/PowerPC branches, unused x87 state and
helpers, old 32-bit mixers and the unused Sys_ConfigureFPU hook. Three supported
Sys_SnapVector bodies and retained MSVC setjmp/longjmp/CPUID assembly are byte-
identical to their old bodies. CMake/header checks reject unsupported architectures,
32-bit pointers and big-endian targets. Configure negative controls for i686,
armv7 and ppc64le pass and run in CI. No active FP expression is rearranged.

After cleanup, 356/358 GCC Vulkan objects and 96/97 aarch64 server objects remain
raw-byte identical to the original baselines. Only unix_main objects differ.
Function/relocation review finds exactly the removed empty Sys_ConfigureFPU;
all retained functions have identical instructions and symbolic targets. ARM64
has one changed trailing alignment nop outside function size. Reports and driver:
/tmp/aftershock-64bit-functions*. All 103 native C/C++ layout/symbol gates pass;
the same 60 advisory outcomes remain. Boundary check passes 383 files (one retired
assembly-only header fewer). Unit/one-ULP, shared math/case, both Q3 smoke logs and
fixed replay pass unchanged. Both OpenArena sanitizer smoke maps and fixed replay
also pass unchanged. Lifetime analysis passes 546 commands/137 source paths and
seven negative controls. Artifacts /tmp/aftershock-64bit-*. Goldens/fixtures remain unchanged.


## #5 migration history (completed; pending statements below are historical)

Make reference checkpoint a08e7275 fixes reproducibility; CMake repair 6987587a;
test-helper migration f111b28c. Original baseline: 11 successful Make configurations,
3,757 raw object hashes and actual compiler commands under
/tmp/aftershock-cmake-before (source 4018c08a, engine identical to 4a952854).
Driver: /tmp/aftershock-cmake-baseline.py. All 3,757 local objects match CMake:
GCC release/debug OpenGL/Vulkan/dynamic, Clang+libc++ both static renderers,
MinGW both static renderers, aarch64 dedicated. No hash normalization.

Original MinGW -flto objects change hashes even with an identical repeated command
(/tmp/aftershock-cmake-lto-repeat.json). GCC records random section IDs and the
unmapped working directory with relative LTO locations
([upstream diagnosis](https://gcc.gnu.org/pipermail/gcc-patches/2022-November/606205.html)).
Both build systems now use the same per-TU seed, absolute source/include spelling
and stable absolute debug-source prefix; separate macro mapping preserves relative
__FILE__ strings. LTO and all object sections remain intact. Deterministic Make
references: /tmp/aftershock-cmake-mingw-repro-make[-opengl]. Focused proof:
/tmp/aftershock-lto-absolute.py. Debug assembly additionally needs its compilation
directory mapped; /tmp/aftershock-cmake-mingw-debug-parity now passes 361/361.
GCC debug dynamic also preserves compiler flag order for raw DWARF equality.

Repository oracle tools/port/check_cmake_parity.py builds both systems and saves
actual commands, raw hashes and explicit differences; its local GCC Vulkan gate
passes 358/358 (/tmp/aftershock-cmake-permanent-parity). Replay this oracle at the
recorded Make-retirement checkpoint after Make disappears. Candidate artifacts:
/tmp/aftershock-cmake-*; initial comparator /tmp/aftershock-cmake-compare.py.

Hosted source 6987587a passes existing regression 34945736264 and build
34945736251. New migration 34945736467 and follow-up 34946061800 prove raw parity
on macOS Intel/ARM64 and Linux GCC/Clang/native ARM64, release/debug, both renderers.
MinGW debug's single assembly difference is fixed locally as above; hosted rerun
remains. MSVC Ninja now compiles ARM64 (359/359 cacheable calls), but automatic
CMake manifest generation duplicates the engine's existing resource manifest.
Use /MANIFEST:NO to retain that resource, and correct x64 vcvars selection to
amd64 (ARM64 uses amd64_arm64). Generated Visual Studio build still needs to pass.
No source fix, warning suppression or dropped resource is involved.

Permanent unit/download/lifetime helpers consume CMake production objects and
compile_commands.json. Engine link instrumentation is target-specific so probe
entry points do not enter CMake's compiler-identification checks. Locally pass:
unit + one-ULP negative control; Clang+libc++; ASan/UBSan with known-bug classifier;
curl options/download; complete bot result; lifetime 546 commands/137 paths with
seven negative controls; both Q3 smoke logs; both-renderer Q3 fixed replay
(b38004b1); both OpenArena UBSan smoke logs; OA fixed replay (5b89d338).
Logs/artifacts /tmp/aftershock-cmake-{unit,unit-clang,unit-sanitized,download,
bot-move,lifetimes,runtime,demo,oa-runtime,oa-demo}*. Goldens/fixtures unchanged.

Regression workflow migration now adds job caches and uses CMake for cross-server
builds and the two existing Windows SDL interface compile checks. The latter pass
locally (/tmp/aftershock-cmake-sdl-cross). The supported build.yml is being switched to CMake after the migration gates;
preserve its original CRLF.
CMake keeps explicit source lists, strict native FP, precise MSVC engine FP, fast
MSVC release renderer FP and static CRT. MSVC ARM64 curl remains disabled as in
the old projects. External OA native objects remain static test inputs.


Correction checkpoint 390a20f4 is pushed. Prior f111b28c passed regression
34946061849 and the supported full build 34946061751. Its migration proved every
Linux/macOS compiler/configuration/renderer combination; only MinGW debug metadata
and MSVC environment/manifest steps needed the corrections above. New migration
34946471284 has already passed MSVC ARM64 debug, including generated VS projects;
other legs are pending. Runtime goldens and both fixed content replays pass locally.

MSVC debug's /Zi made all 359 compilation calls uncacheable with the installed
ccache 4.9; release is 359/359 cacheable. The build now selects CMake's Embedded
MSVC debug format (/Z7), keeping symbols in objects for ccache and final linked
PDBs. This changes debug metadata format, not optimization or runtime checks.
[ccache 4.9 option handling](https://github.com/ccache/ccache/blob/v4.9/src/argprocessing.cpp#L1152)
explicitly supports /Z7 and rejects /Zi. Hosted cache statistics must confirm the
change. Build instructions and compile-database documentation are being updated;
Make/handwritten-project retirement and inactive platform cleanup remain undone.


Embedded-debug source 48733686 passes migration 34946795374 on every leg. MSVC
x64 debug now reports 720/720 cacheable compilation calls, including 52 hits,
while retaining the generated VS build. Native Windows curl needs target zlib;
the local optional curl build compiles but cannot link -lz because that cross
library is absent. Hosted MSYS now installs its target zlib explicitly; the
local cross example uses USE_CURL=OFF, matching the verified cross configuration.
No local package is installed and the optional local curl link is not claimed
passing. CMake-only workflow/artifact staging still needs its own hosted run.


## #4 completed evidence

#4 evidence: baseline /tmp/aftershock-boundary-before has 355 production objects
per renderer on 239cbc34. Pure move 85381cda moves 757 files with identical Git
blob IDs, modes and SHA256; mapping /tmp/aftershock-subsystem-moves.json. Separate
path repair 8b5264c5 preserves 355/355 Vulkan and 355/355 OpenGL object hashes,
without normalization (/tmp/aftershock-boundary-after). The numstat display check
initially rejected binary '-' fields; independent blob/mode/hash checks verified
the moves. No accepted golden or fixture changed anywhere in #4.

Boundary source 52e57708 publishes client/sound/shared game/input interfaces,
moves clock/CPU/debug/AVI process operations to platform, and routes raw filter/bot
file operations through files.cpp. The 384-file include/OS check and controls run
in CI. Five public client operations replace sound's private client-state access.
All five Sys_SnapVector bodies, clock bodies and CPU-detection bodies compare
byte-identical with their originals. The AVI arithmetic/order is retained. Inline
platform debug wrappers preserve renderer ABI. Raw stdio adapters preserve return
values and encoding; /tmp/aftershock-boundary-stream-check.cpp exercised formatted
write, item counts, seek/tell, EOF, close and missing-file behavior against files.o.
Unused renderer2 and absent-tool parser integrations were retired per the plan.
Ownership: docs/subsystems.md. Bug records: docs/bugs.md. All 130 original GPL
source hashes remain verified; e24495b2 records import path transformations.

Local final gates: unit/one-ULP control; both Q3 bot hashes; both-renderer Q3
lifecycle replay (b38004b1); OA UBSan bot hashes and fixed replay (5b89d338);
103 native C/C++ layout/symbol comparisons; ALSA callbacks/thread joins and curl
transfer; GCC client/server and MinGW Windows client; lifetimes 546 commands/137
paths across both renderers. The four-command/one-path lifetime reduction from
#2 is raw net_ip moving into platform. Artifacts: /tmp/aftershock-boundary-*.

Hosted first build found Ogg/Vorbis $(ProjectName) include directories missed by
literal path repair. ffaa1ea7 repairs them; expanded include/library directories
were checked and all MSVC legs pass. Final review restored build.yml's original
CRLF bytes (normalized text exactly equals tested ffaa1ea7); no source change.

Self-review: one issue, public/OS ownership scope, content-only move evidence kept
separate from boundary changes; no new OS calls outside platform/filesystem;
no non-trivial core lifetimes or per-frame allocation; no simulation FP expression
change; existing wire/file layout assertions retained and native layouts match;
all goldens/fixtures unchanged. CI and local gates pass. PR #65 merged as 4a952854.

## Earlier #2 integration checkpoints (historical)

Active: issue/2-native-game, draft PR #50. #3, the recorded #31 fixes through
PR #61, and #1 are merged. #2 import, C/QVM parity, catalog port, advisory review,
and byte-identical .cpp rename are complete. Static engine adapters are still
scratch-only; repository integration, OpenArena static coverage and VM/JIT removal
are next. Do not rerun finished network work or regenerate accepted fixtures.

Game lifecycle test-first 413f1ed8 fails when native module storage survives unload.
Source d6c2ac52 restores per-level arena/bot/cache/counter state. Both persistent
restart/map-change repeats match DLL reloads (dd1fe5c3); the movement-debug variant
also matches (e87382ec). Existing Q3 native bot logs and all accepted replay frames
remain unchanged. Provenance 1591cc93 passed regression 34931875059; full build
34931874993 must still be checked.

Client lifecycle test-first a1cf3223 fails after fixed replay/video restart with
retained modules. The client now resets RNG/effect history, draw/loading/prediction
state and particle rotation at module init; UI resets its state, arena and server
cache counts. Existing menu structs already reset on entry. Both maps/renderers
now pass the ordinary-versus-retained transition comparison and the separate
accepted-golden replay (b38004b1). GCC/Clang C++ modules build; all 103 C/C++ layouts
and symbol comparisons pass, with advisory codegen reports retained. No FP
expression was rearranged, no per-frame allocation or OS access was added.
These resets implement #2's new static storage lifetime, not pre-existing fixes.

Current evidence: /tmp/aftershock-native-lifecycle-{debug,smoke,demo,gates},
/tmp/aftershock-native-client-lifecycle-{before,after,golden,clang,gates}.
Mutable-state audits: /tmp/aftershock-native-game-state-inventory.txt and
/tmp/aftershock-native-client-state-inventory.txt. Module wrappers must bind all
five compatibility functions (rand/srand/qsort/atof/memmove), and lifetime analysis
must cover their generated translation units. Base-game lifecycle was exercised;
missionpack is not an enabled imported-module configuration.

PR #61 merged 35a2c75c; merged regression 34930596490 passed. Its #2 integration
1def16ce passed regression 34930663280 and full build 34930663236. Both overlapping
info-removal helpers are fixed identically in q_shared.cpp; provenance retained.

Client source 436bbab1/provenance e40ac443 are pushed; regression 34932294456
passed and full build 34932294429 remains to check. Earlier full build 34931874993
passed too.

Working-tree static integration now replaces the six server/nine client dispatch
sites with typed calls. A single module.cpp wrapper compiles each imported source
in its module namespace, with all five compatibility functions bound locally;
there are no generated translation-unit files. The first actual Make dedicated
build links (/tmp/aftershock-native-integrated). Client build and bot smoke are
running. VM code is still linked but no longer serves these direct game exports;
its removal and test-suite adaptation remain pending. These integration edits are
not committed yet, and CI still describes the previous lifecycle checkpoint.

OpenArena preflight now compiles its pinned C sources with typed imports/exports,
combines each module into a relocatable object and prefixes its internal global
symbols using objcopy. The public typed exports remain visible. All three C objects
compile, and the game links against the same direct-call server; runtime/replay
parity remains to verify. This preserves a static hosted-content test executable
without importing OA implementation into production game source. Artifacts:
/tmp/aftershock-oa-static-preflight and its driver script. This approach still
needs permanent test/build integration, state-reset audit and verification.

Next: verify static Q3/OA bot logs and client fixed replay, integrate permanent
native builds/tests (including lifetime analysis), then remove the VM/JIT code.
Keep OpenArena coverage and accepted goldens; PR #50 remains draft until static
linking, VM/JIT removal, build/lifetime/layout/runtime/replay gates and self-review
are complete. The full later #4/#5/#8/design-only #6 sequence remains outstanding.

The integrated Q3 static server passes both accepted bot logs, both static client
renderers pass fixed replay (b38004b1), and lifetime analysis passes 562 compile
commands including 206 native commands across both renderer configurations. The
Clang AST log contains private state from game, cgame and UI, confirming all
wrapper source selections were analyzed. Engine builds use the existing fixed
SOURCE_DATE_EPOCH; a first standalone build omitted it and differed only in the
version date, then passed after rebuilding the two date-bearing engine objects.

Static OA C game bot logs pass both accepted hashes; static OA clients pass both
renderers and maps (5b89d33). Permanent openarena_native.py --static now builds the
same isolated C objects. Runtime/demo now use static modules by default and the
redundant former native-DLL CI pass is removed. OA runtime --sanitize includes its
C game object as well as engine code and is running. Test arguments --game-code
and --game-language are retired; C/C++ import compiler checks remain separate.

The permanent Q3 lifecycle gate now compares against the already-recorded DLL
reference logs (new native-lifecycle*.log files, dd1fe5c3/e87382ec), preserving all
existing accepted goldens. Static movement-debug restart/map-change passes twice.
Client lifecycle frames were also checked against the original frame golden and
match it, so --lifecycle now uses that existing golden directly. No extra frame
fixture or regeneration is needed. These integration changes are still uncommitted.

Earlier checkpoints below describe how this integration was reached.


Active work: issue/2-native-game, draft PR #50. PR #60 merged as 8e3ecf78 after
regression 34927341670/full build 34927341749 passed on 0366fa06. Test-first
02a9ddeb reproduces float-to-signed-byte UB. The three complete movement expressions
now explicitly truncate through int; GCC/Clang original C objects, layouts, symbols
and assembly are byte-identical before/after. All 18 command cases pass. Temporary
complete #2 native C++ UBSan bot smoke passes both maps with identical repeats and
accepted logs. Explicit unit/collision/Q3 runtime regeneration is byte-identical.
This integration retains T22 abs casts, float suffixes and all catalog edits.
The command test uses the complete local headers; the temporary header-fetch helper
is unnecessary here and removed, retaining #2's local-header team-leader test.

#59 merged 11781f44; merged-tree regression 34926638834 passed. Its #2 integration
and job-local pahole installation d8691015 passed regression 34926764924/full build
34926765230, including the 103-object native comparison. #58 merged-tree regression
34925562788 and #57 merged-tree regression 34924317093 passed.

Strict native warning freeze 7c4f8302 passed regression 34925774876/full build
34925774880. Both C/C++ native builds use -Wall -Wextra -Werror with only classes
observed in C. Provenance checkpoint e4853819 verified all 130 original GPL hashes:
30 verbatim files, 96 modified, four retained ABI headers. Per-file transformation
references identify native ABI/catalog edits and each separate #31 fix.

Permanent native_gates.py passes all 103 G2/G3 comparisons locally and in hosted
CI; it retains 63 advisory assembly diffs. --tidy completed all objects with 1365
narrowing, 55 signed-char and nine string-result findings; no tool/compile failures.
Latest local artifacts precede #60: /tmp/aftershock-native-gates-final; function
review: /tmp/aftershock-native-function-review-current and
/tmp/aftershock-native-review-checkpoint.md. Flag initialization/command-byte bugs
found during this review have now been fixed in separate #31 PRs. No unconfirmed
G7 warning is being called a sanitizer failure or hidden.

The resolved integration passes GCC/Clang command checks and the team-leader
check; ai_main.c is byte-identical to the successful complete native UBSan preflight.

#60 merged-tree regression 34927724919 passed. The resolved #2 integration
f92ae456 passed regression 34927833819/full build 34927833852.

The advisory G4/G7/catalog review and acceptance decision are committed in
58d86036 (docs/native-port-review.md) and recorded on #2. Diagnostics remain visible;
confirmed bugs are separately tracked/fixed.

The .cpp rename now preserves all bytes of 93 implementation files (git reports
100% similarity for every rename; /tmp/aftershock-native-rename-hashes.json records
SHA256 before/after). The manifest retains original upstream source paths/hashes.
C oracles explicitly select -x c. GCC/Clang C and C++ strict module builds pass;
OpenArena C module build passes. Math/shared, team leader/voters, base/missionpack
flags and bot command checks pass. No accepted fixtures/goldens changed.

Rename 634decac passed regression 34928509506 and full build 34928509462
(the single failed MSYS2 package-download job passed on retry). Checkpoint 526a77b7
passed regression 34928947087/full build 34928947114.

Static preflight remains scratch-only. Six server objects now use typed game
exports; 183 typed imports replace game syscalls. Both bot logs still match.
Nine client objects use typed cgame/UI exports with explicit call-depth counters;
94 cgame and 87 UI direct imports compile. Fixed replay on both renderers matches
all accepted frames. This is not yet repository integration or VM removal.

Additional restart/map-change checking found two distinct issues. Static linking
must reset module state previously reset by DLL reload: botstates points into the
reset game arena, and bot timing statics persist. A scratch reset clears those
pointers/counters and restores the restart portion, but the full state audit is
unfinished. Also the unchanged DLL reference itself corrupts info strings on map
change: original GPL Info_RemoveKey and Info_RemoveKey_Big use overlapping strcpy.
ASan reproduces strcpy-param-overlap in current q_shared.cpp. That existing bug
must be fixed separately under #31 before accepting map-change parity.

The separate #31 info-string fix is now merged as #61. Resume native lifecycle
reset and static integration,
retaining OpenArena coverage. Native module library audit must also bind memmove
inside each namespace: bg_lib defines it in addition to rand/srand/qsort/atof; the
first scratch wrappers omitted that local prototype. Do not claim complete static
parity until this is corrected and verified. No accepted fixtures/goldens changed.

Scratch evidence: /tmp/aftershock-native-static-preflight (exports, client-imports,
client-exports, reset). Scripts: /tmp/aftershock-game-direct-preflight.py,
/tmp/aftershock-native-direct-preflight.py, /tmp/aftershock-game-static-link.py,
/tmp/aftershock-game-export-preflight.py, /tmp/aftershock-client-direct-link.py,
/tmp/aftershock-client-export-preflight.py, /tmp/aftershock-static-reset-preflight.py.
Runtime/replay output: /tmp/aftershock-game-exports-runtime,
/tmp/aftershock-client-exports-demo. Restart comparisons:
/tmp/aftershock-static-restart.py, /tmp/aftershock-static-reset-restart.py,
/tmp/aftershock-static-reset-original-reference.py. ASan reproducer/log:
/tmp/aftershock-native-info-overlap. Full earlier native UBSan evidence:
/tmp/aftershock-bot-command-native.py/.log. No static engine edits are committed.

Completed #2 checkpoints: permanent OpenArena native build/smoke/replay d0013d95
(regression 34922352537 passed); portable Q3 binary32 literals 5592a1eb (regression
34922727256 passed). All 103 C and 103 C++ objects stayed byte-identical in the
literal conversion. Clang native Q3 smoke/replay and GCC/Clang native OA smoke/replay
match both maps/renderers. Three T17 ui_ingame casts also preserve C/C++ objects.
Full G2/G3 before the sentinel merge: 103/103 objects match. G3 adds artifact-only
-U__OPTIMIZE__ to the existing header/optimizer isolation flags; production assembly
differences are retained. The completed G4 review is in native-port-review.md; no
functions were added/removed across 103 objects. G7 on the temporary merged UI tree completed all
103 objects without tool/compile failures: 1365 narrowing, 55 signed-char and nine
implicit strcmp-result findings. The nine strcmp comparisons are equivalent nonzero
checks. The disposition in native-port-review.md retains inherited conversions for
#8 and routes confirmed defects through #31.
Artifacts: /tmp/aftershock-native-function-review, /tmp/aftershock-native-g2-g3-headers,
/tmp/aftershock-native-warning-inventory and /tmp/aftershock-native-tidy/results.json.
Earlier #54/#55/#56 merged-tree regressions 34913731858/34914963107/34915431579 passed.

#3 is complete (PR #33, merged-tree regression 34867621821 passed). The Huffman
alignment fix merged as PR #36 / bb4474db after regression 34868566671 and full
build 34868566674 passed; its merged-tree run 34869117306 passed.

Filesystem PR #37 merged as 73108eab after regression 34877286456 and full build
34877286443 passed; merged-tree regression 34877819880 passed.

Download PR #38 merged as 02b16def after regression 34878247137 and full build
34878247337 passed; merged-tree regression 34878892522 passed.

ALSA PR #39 merged as 811c6f7a after regression 34879358567 and full build
34879358584 passed; merged-tree regression 34879983585 passed.

Curl PR #40 merged as 998f21c3 after regression 34880567812 and full build
34880567796 passed; merged-tree regression 34881337473 passed.

ZIP PR #41 merged as 2a9436fe after regression 34885119593 and full build
34885119668 passed; merged-tree regression 34885592975 passed.

VM PR #42 merged as 62dad842 after regression 34886047536 and full build
34886047431 passed; merged-tree regression 34886485915 passed.

zlib callback PR #43 merged as fed755d6 after regression 34886950022 and full
build 34886949860 passed; merged-tree regression 34887353903 passed.

Extension output PR #44 merged as bdef99f4 after regression 34887856044 and full
build 34887856072 passed; merged-tree regression 34888464336 passed.

AAS PR #45 merged as 481d008a after regression 34889484418 and full build
34889484412 passed; merged-tree regression 34889992769 passed.

PNG PR #46 merged as f46f48c7 after regression 34890358367 and full build
34890358307 passed; merged-tree regression 34890928632 passed.

JPEG PR #47 merged as 9a7c2625 after regression 34891606879 and full build
34891606634 passed on source 01f2dd40. Its merged-tree regression 34892331846 passed. The disposition audit accounts for all twelve defects; CMake remains #5.
#31 is complete; all twelve fixes are merged and linked in the closed notes ledger.

#1 merged as PR #48 / 95b418b0 after regression 34892972996 and full build
34892972994 passed on e0013d90; merged-tree regression 34893585998 passed.

#2 exact GPL C import is committed/pushed on issue/2-native-game: b3ef1acd plus
checkpoint a937d710. Its 125 imported files and four retained ABI headers have
SHA256 provenance. Native preflight found an LP64 Q_rsqrt overread; no fix was made
on #2. The source is not wired into the engine yet.

Native math PR #49 merged as 2018564f after regression 34894597080 and full build
34894597081 passed on source 152cc6e2. Its merged-tree regression 34895239211 passed. #31 is closed again.

Native transition branch: `issue/2-native-game` (draft PR #50, checkpoint f4653398). Merging modernization retains the original
GPL import commit b3ef1acd and resolves the q_math add/add to the reviewed #49 fix.
The progress conflict is resolved to the latest #31 evidence plus this #2 state.
No history rewritten. Sources still compile only in temporary native preflight,
not the permanent engine build. Original headers match seven shared ABI sizes and
three offsets. Temporary native game/cgame/UI modules compile as C after pointer
entry-point adaptation and linking original bg_lib support; all 36 q3dm17 gameplay
events match the accepted QVM log. Full log differences are implementation-loading
metadata, compile date and bot-skill printf padding. Both-renderer fixed-demo
native preflight repeats identically but fails all accepted frame hashes: differences
range from 2 pixels to 931 pixels per sample. No fixture/golden changes. Added the
required ui_shared.h header verbatim with provenance (126 imported files now).
Temporary native ABI adaptation now passes every accepted frame on both maps and
renderers: binary32 literals plus float results at math calls reproduce the QVM
compiler model (lcc/src/bytecode.c declares float/double/long double all size 4).
GCC uses -fsingle-precision-constant; Clang accepts -cl-single-precision-constant
(the GCC spelling is ignored by Clang, verified with a literal-type assertion).
This is native ABI compatibility, not a new simulation algorithm: original FP
expressions and all accepted fixtures/goldens remain unchanged. Evidence:
/tmp/aftershock-native-demo-math-boundary.log, original frame hash b38004b1.
Permanent C native build/layout/smoke/replay commands are under development.
All 29 shared ABI sizes/alignments and three offsets match with GCC and Clang.
Native syscall arguments now use intptr_t words and explicit unused tail words,
matching the transitional DLL host's fixed vararg reads. The default QVM smoke
still passes both accepted goldens. Native q3dm17 passes the normalized accepted
log; native q3dm7 repeats but adds a Major chat line at the end, so parity fails.
The gate retains that failure and all gameplay text. Temporary engine tracing
shows both modes receive seed 140, run 1,494 frames, and finish at 74,900 ms.
Player states match through 42,150 ms; the first bot movement input difference
then precedes the differing state at 42,200 ms and syscall sequence at 42,300 ms.
Trace edits live only under /tmp, outside this branch's engine sources.
Permanent native demo replay now passes all accepted frames on both maps and both
renderers (/tmp/aftershock-native-demo-permanent.log, hash b38004b1). The server
trace identifies the blocker: BotMoveToGoal initializes only its first six fields;
the obstacle early return leaves movedir/weapon/ideal_viewangles untouched.
At 42,150 ms both modes pass identical movement state and goal, return blocked by
entity 167 with flags 32, then BotAIBlocked reads stale movedir. QVM has old stack
coordinates while native has different stack contents, causing different avoidance.
This is an existing engine bug, not native arithmetic drift. No inline #2 fix.
Current branch: `issue/31-bot-move-result`, based on modernization. #2 is safely
checkpointed/pushed as f4653398 and draft PR #50. #31 is reopened.
Next: a separate #31 failing-test-first PR initializes the
complete movement result and explains any resulting golden changes. After it
merges, resume permanent native smoke/replay parity and C++/static integration. No VM/JIT
removal before full native parity. Keep all new bugs in separate #31 PRs.

Clang runtime observation classified: VM_CallCompiled's instrumented indirect
call reads metadata at codeBase-8 before entering JIT code; the mmap allocation
starts at codeBase and has no preceding metadata. Disassembly confirms that load.
A temporary relink with only vm_x86.o built with -fno-sanitize=function passes both
original Q3 smoke goldens; all other UBSan instrumentation remains. No source or
CI flag/suppression changes were made. This is an instrumentation/JIT compatibility
limit of the transition oracle; #2 removes that JIT. The allocator callback bug
is independently covered by the permanent Clang unit test. The ASan/faketime
experiment still timed out before output and is not a claimed runtime gate.

## Issue status and remaining sequence

| Order | Issue | Status / required work |
|---|---|---|
| 1 | #3 regression suite | Complete: merged PR #33, merged-tree regression passed. |
| 2 | #31 bugs | Huffman merged (#36, upstream #424); filesystem merged (#37, upstream #425); download URL merged (#38, upstream #426); ALSA merged (#39, upstream #427); curl va_start merged (#40, port-specific); ZIP alignment merged (#41, upstream #428); VM alignment merged (#42, upstream #429); zlib callbacks merged (#43, upstream #430); extension output merged (#44, upstream #431); AAS missing candidate merged (#45, upstream #432); PNG header alignment merged (#46, upstream #433); JPEG table index merged (#47, upstream #434); thirteen fixes merged through #49; merged-tree regression 34895239211 passed. Movement result (#51, upstream #435) and native dispatch (#52, upstream #436) also merged; active follow-up is GPL team-leader name bounds. Each fix needs failing-before/passing-after evidence, affected golden regeneration explained, removal of its expectation/suppression, upstream PR if not port-specific. Read docs/cpp-port-notes.md and issue #31. |
| 3 | #1 error model | Complete: merged #48, merged-tree regression 34893585998 passed. |
| 4 | #2 native game | In progress: exact C import and native preflight; prove QVM/native bot-smoke and fixed-demo parity; port with catalog T1–T25 and gates; static native modules, then remove VMs/JITs. Explicitly ends Quake 3 mod compatibility. |
| 5 | #4 boundaries | Hash-verified directory moves, include/OS-access CI checks, docs/subsystems.md; rename cpp-port-notes.md to docs/bugs.md. |
| 6 | #5 CMake | Repair as primary, object parity before removing Makefile; generated MSVC projects, 64-bit little-endian only. |
| 7 | #8 code rules | Warning class per PR; one codegen-identical tree-wide clang-format commit; tidy subsets; fixed-width wire/file types and layout traits; release-identical Q_ASSERT. |
| 8 | #6 design only | Write docs/design/rhi.md after #8, then stop. |

## Rulings in force

- The user's 2026-09-14 continuation supersedes the older roadmap order and handoff.
  Preserve fixed-timestep simulation, prediction/snapshots, cvars/pk3, arena/POD data,
  no per-frame allocation. No simulation FP restructuring except tested #31 fixes.
- C++20, no exceptions/RTTI, longjmp/trivial core lifetimes. Existing VM/dlopen paths
  are the transition oracle only. Match surrounding style until #8.
- Local system packages must not be installed. Each CI job installs prerequisites.
  Proprietary paks stay outside git and uploads. Missing content/tools fail tests.
- Network coverage is finished. This thread ran `python3 tests/network.py --max-error 0`
  exactly once: exit 1, `FAIL: prediction bound exceeded`, measured 8.875 against zero.
  Do not modify or rerun that harness in this continuation.
- Normal demo checks replay fixed fixtures; recording is intentionally not reproducible.
  Golden creation/replacement is explicit, reviewed, and forbidden in CI.
- Engine/vendor bug fixes belong only in individual #31 PRs. No production source
  changed in #3. The independent scope adjustment on issue #3 remains in force.

## #3 acceptance evidence

Source/test/golden head: **98096f9710b29fb99464ac2464b356b7a28799cc**.

- Regression workflow **34866966536: success**. GCC, Clang/libc++, aarch64/mingw,
  sanitizer expectations, OpenArena collision/both-map smoke, and both real software
  renderer replay gates pass. The independently assigned job is non-blocking per
  the issue's scope ruling.
- Full build workflow **34866966514: success**, including Linux/macOS, MSVC x64/ARM64
  Debug/Release and Windows mingw. Skipped release-publishing jobs are inapplicable.
- Local GCC/Clang unit hashes and active Q_rsqrt one-ULP controls pass. Original
  accepted unit/collision/Quake 3 smoke goldens are byte-identical to d754d683.
  Local collision, both smoke maps, and final fixed-demo replays pass again.
- The sanitizer unit run reports HuffmanGetSymbol alignment as `known, tracked in #31`
  while comparing all thirteen groups. Unknown diagnostics, ASan failures, and stale
  expectations fail. `tests/check_known_bugs.py` verifies the classification policy.
  PNG alignment and JPEG table-index defects/reproducers are already on #31 and in
  docs/cpp-port-notes.md; they are not exercised by the unit driver.
- OpenArena uses oa_dm1/oa_dm7, Sarge/Beret, fs_game=baseoa, networking disabled and
  900 waits for combat. Hosted collision/smoke match locally generated goldens.
  Ubuntu paks contain native-module markers; tests/openarena.py adds official GPL
  oaxB52 QVMs with a pinned SHA256. Source/license/provisioning are in tests/README.md.
- Review rejected initial q3dm17 idle-player death-screen samples; final Quake 3
  fixtures follow a bot. Review also corrected the inherited opengl1 Make value to
  opengl: previous two-renderer claims were incorrect. Both renderer identities are
  now asserted; tests/check_frames.py rejects mislabeled logs and unequal repeats.
- Exact frame profiles use Mesa 26.0.8 locally and Mesa 25.2.8 on Ubuntu 24.04.
  No pixel tolerance or fallback. The hosted profile was generated LOCALLY with
  tests/frames.py --regenerate after visual review of run **34866295338** on
  **145939aa**. All 24 saved frames (12 samples, two repeats), renderer identities,
  and fixed fixture hashes were checked. CI never generated a golden.
- Golden-writing modes reject CI. AGENTS.md lists permanent tests/ commands.

## Self-review

#3 contains only tests/CI/docs and initial public-content/demo fixtures. No engine
or vendor changes, new core destructors, allocations, OS calls, simulation FP edits,
or layout changes. Original accepted goldens and port-evidence remain untouched.
README documents prerequisites, both content sets, provenance, exact profiles,
known-failure policy, and explicit fixture/evidence commands. Issue #3 and PR #33
record the measurements and corrected renderer finding.

## Local recovery paths

#3 worktree: `/home/matt/.t3/worktrees/aftershock/t3code-b3a8e505`.
The supplied workspace was at the integration baseline when this thread began.
`/tmp/aftershock-openarena-baseoa` is staged public content; original downloaded
packages were extracted under `/tmp/aftershock-openarena`, never installed.
`/tmp/aftershock-demo-tests` and `/tmp/aftershock-oa-demo` contain final local replay
evidence. `/tmp/aftershock-ci-runtime4/aftershock-demo-tests` contains reviewed hosted
baseline evidence. These paths are disposable; source and README commands suffice.

## #31 Huffman validation

Fatal Clang ASan/UBSan unit run failed before the fix at huffman_static.cpp:206,
exit 1; the same command passes afterward with no diagnostic/expectation.
Production GCC assembly and symbols pass the existing port gates before/after.
The explicit unit and differential regeneration commands produce zero golden
diff. Local q3dm17/q3dm7 smoke and both-renderer fixed-demo replay pass unchanged.
An upstream C regression tests every symbol at 32 bit offsets: sanitizer fails
before, passes after, and GCC C assembly is also identical. Upstream PR: https://github.com/ec-/Quake3e/pull/424. Fork PR #36 merged after full CI and self-review; merged-tree regression passed.

## #31 filesystem validation

The permanent `--sanitize --pointer-compare` command fails before the fix with
NULL versus filename+3, and passes afterward with the original unit golden.
Separate ASan/UBSan and ASan/pointer-comparison runs retain both checks: combining
them under Clang 21 instruments generated pointer-overflow comparisons in COM_ParseExt
(disassembly shows `__sanitizer_ptr_cmp(pointer, -3)`). No suppression/expectation
covered this bug. GCC/Clang/libc++ units, the one-ULP control, both sanitizer modes,
collision, both-map smoke and both-renderer replay pass. Explicit unit/collision
regeneration changes no golden. Symbol gate passes; normalized per-function assembly
changes only FS_AllowedExtension among 99 functions, as expected for the new guard.
Upstream C test fails before and passes after: https://github.com/ec-/Quake3e/pull/425.
Fork: https://github.com/msetaro/aftershock/pull/37. Regression/full build runs above
include public-content runtime and all platform legs. The caller audit's separate
Sys_LoadLibrary uninitialized diagnostic pointer is recorded in notes and issue #31.

## #31 download validation

Permanent `python3 tests/download.py` fails before on the trailing-slash base
(`maps//map%20name.pk3`), then passes with GCC and Clang/libc++ after the one-line
guarded last-character check. `%1` substitution, escaping and empty-base behavior
are retained. Explicit regeneration created only tests/golden/download.txt.
Existing unit/one-ULP, collision, smoke and replay gates pass. One local smoke
attempt overlapped replay and hit an occupied UDP port; serial smoke passed both
original map goldens. Run those local runtime gates serially. No golden changed
to accommodate the collision. The hosted runtime URL check uses existing libcurl
packages; begin/cleanup run without a transfer.
Upstream C fix/test: https://github.com/ec-/Quake3e/pull/426.
Fork PR: https://github.com/msetaro/aftershock/pull/38.

## #31 ALSA validation

Permanent callback assignments reject both original void(void) signatures. GCC and
Clang/libc++ pass after the pthread-compatible signatures/direct registration/NULL
returns. Real ALSA null output receives positive MMAP/DIRECT submissions and both
threads join within the timeout; no physical device or fabricated audio backend.
The dynamic-ALSA production object builds. Explicit unit/collision regeneration
produces no golden diff; serial both-map smoke and both-renderer replay pass unchanged.
Upstream C test/fix: https://github.com/ec-/Quake3e/pull/427.
Fork PR: https://github.com/msetaro/aftershock/pull/39. CI runs are above.

## #31 curl varargs validation

The permanent Clang test fails before at va_start and passes after changing the last
named argument to int and retaining a CURLoption local. Removed -Wno-varargs.
GCC/Clang local-file tests verify long/pointer/offset forwarding, body suppression,
private-data identity, bounded partial output on size-limit rejection and exact
returned bytes. No network transfer. Explicit URL regeneration changes no golden.
Normalized GCC codegen/symbol gates pass; the internal mangled name changes with
its parameter type, and all callers are in cl_curl.cpp. Unit/one-ULP, collision,
serial both-map smoke and both-renderer replay pass unchanged.
Fork PR: https://github.com/msetaro/aftershock/pull/40. CI runs are above.

## #31 ZIP validation

The existing GCC UBSan bot smoke fails with the original ZIP object and passes
with CopyLittleLong through a signed int temporary. Removed only the ZIP alignment
suppression. Q3 and OpenArena both-map goldens pass unchanged. Explicit unit/collision
regeneration produces no golden diff; normal serial smoke and fixed-demo replay
pass. GCC production symbol gate passes. Codegen gate reports only stack slots
64/68 exchanged with their matching sign-extending consumers; field destinations
and operations are unchanged. This reviewed difference is acceptable under #31's
codegen ruling; the gate is not weakened or reported as identical.
Upstream C UBSan startup on valid installed paks fails before and exits cleanly
after: https://github.com/ec-/Quake3e/pull/428. No new content or loader target.
Self-review: one packed-read bug, shared function covers all callers; no simulation
FP edit, layout change, allocation, destructor or new engine OS call. Runtime
sanitizer coverage reuses the existing smoke runner and compares existing goldens.
Regression 34885119593 passed on source 09e7cc96, including hosted GCC UBSan smoke.
Full build 34885119668 passed on the same source. Final checkpoint changes only
documentation; self-review passes and no goldens changed.

Next bug reproduction is ready without a source change: the same GCC runtime
with UBSAN_OPTIONS=halt_on_error=1 (no suppressions) exits 1 at vm.cpp:1181.
Evidence: /tmp/aftershock-vm-before-runtime.log. Follow with its own branch/PR.

## #31 VM validation

Permanent GCC UBSan runtime fails before at vm.cpp:1181, passes after the packed
read uses CopyLittleLong through int32_t. Removed the final alignment suppression;
tools/port/ubsan.supp is empty. All Q3/OA smoke goldens, normal serial smoke and
both-renderer replay pass unchanged. Explicit unit/collision regeneration has no
golden diff. Production symbols and all 26 function-section bytes match. Text
codegen differs only in the compiler switch-table label CSWTCH.89/90; unchanged
instructions/data reviewed acceptable, no gate edits. Upstream C reproducer
fails before and passes after: https://github.com/ec-/Quake3e/pull/429.
Self-review: one packed-read bug in the shared caller path; no instruction format,
FP, JIT behavior, allocation, destructor, layout or engine OS changes. No new
known-bug entry or test target; existing runtime CI is the permanent regression.
Regression 34886047536 and full build 34886047431 pass on source f320dee6.
Final checkpoint changes documentation only. Self-review passes.

## #31 zlib callback validation

Permanent Clang ASan/UBSan unit fails before on zcalloc's byte-pointer callback
type; correct void-pointer signatures and direct registration pass initialization
and cleanup. Separate pointer sanitizer mode also passes. The helper is linked
into the existing unit driver and uses its allocator stubs, with no content input.
No new runner/target or expectation/suppression. Explicit unit/collision golden
regeneration produces no diff; one-ULP negative control, normal/GCC UBSan smoke
and both-renderer replay pass. Production normalized codegen/symbol gates pass;
only private callback mangled names change. All callers are in unzip.cpp.
Upstream C regression fails before and passes after:
https://github.com/ec-/Quake3e/pull/430. Self-review: one callback ABI bug; no
allocation arithmetic, per-frame allocation, layout, FP, destructor or engine
OS-access changes. Regression 34886950022 and full build 34886949860 pass
on source 74b15fd6. Final checkpoint is documentation only; self-review passes.

## #31 extension output validation

Both platform diagnostic callers receive a defined extension on true returns:
the shared function now initializes its output before classification, using an
empty string for names without a dot. Versioned .so and rejected-extension strings
remain unchanged; the other callers only consume output on false returns.
Existing unit assertions fail before and pass after for empty, extensionless,
trailing-dot, ordinary, .so.N and pk3 cases. No library is loaded by the test.
Clang sanitizer/pointer checks, one-ULP control, collision, smoke and replay pass.
Explicit unit/collision regeneration changes no golden. Symbols pass; only
FS_AllowedExtension changes bytes among 99 functions. The assembly text also
renames CSWTCH.612/613 in FS_Seek, whose bytes are unchanged. Reviewed expected
codegen difference; no gate weakening. Upstream C fix/test:
https://github.com/ec-/Quake3e/pull/431. Self-review: shared out-parameter bug only;
no extension policy, OS access, allocation, destructor, FP or layout changes.
Regression 34887856044 and full build 34887856072 pass on source 8fd57208.
Final checkpoint changes documentation only; self-review passes.

Next AAS reproduction: GCC UBSan compile of be_aas_reach.cpp with its existing
-Wno-maybe-uninitialized exception overridden by -Werror=maybe-uninitialized fails
on beststart at line 2196. The function is AAS_Reachability_Jump (the earlier note
incorrectly called it JumpArea). A temporary source copy with a bestdist==999999
return before VectorMiddle compiles cleanly; engine source is unchanged so far.
The Makefile exception explicitly names this one defect and can be removed in
its own #31 PR. Evidence: /tmp/aftershock-aas-warning-before.log and
/tmp/aftershock-aas-guard-check.log.

## #31 AAS candidate validation

The existing runtime --sanitize build fails before on the recorded beststart
warning once its specific Makefile exception is removed. AAS_Reachability_Jump
now returns false when bestdist retains its initial sentinel, before midpoint
reads. The edge selector only replaces bestdist when it supplies endpoints; no
candidate arithmetic was rewritten. Upstream C compilation likewise fails before
and passes after: https://github.com/ec-/Quake3e/pull/432.
Explicit unit/collision regeneration has no golden diff. Final Q3 and OA UBSan
both-map smoke, normal serial smoke and both-renderer replay pass unchanged.
The production symbol/codegen gates are not identical: only jump/grapple functions
change, with an added private VectorLength body and no direct sqrtf import.
Register/stack/inlining changes were reviewed; actual emitted VectorLength matches
a libm oracle for four million finite-input vectors across all four rounding modes.
No source FP expression change or gate weakening. Self-review: one missing-candidate
bug, existing compile/runtime regression, no layout/OS/allocation/destructor change.
Regression 34889484418 and full build 34889484412 pass on source b5e31b8c.
Final checkpoint is documentation only; self-review passes.

## #31 PNG header validation

The existing demo build fails before on the new chunk-header alignment assertion
(4 instead of 1). A scoped packing pragma gives this wire type byte alignment;
its eight-byte size, two uint32_t fields and BigLong conversions stay unchanged.
All six buffered header-read sites share the type. GCC production codegen/symbol
gates pass identically; both-renderer fixed replay preserves every frame hash.
Explicit unit/collision regeneration has no golden diff; both-map smoke passes.
Upstream C layout assertions fail before and pass after with GCC and Clang:
https://github.com/ec-/Quake3e/pull/433. No expected-failure entry or suppression
covered this type. Self-review: one alignment bug; persistent layout assertions
in the existing renderer build, no new content/recording, FP, allocation, OS-access
or destructor changes. Regression 34890358367 and full build 34890358307 pass
on source becd4b27. Final checkpoint changes documentation only; self-review passes.

## #31 JPEG table-index validation

The permanent Clang sanitizer unit fails before at index 5 outside JHUFF_TBL *[4].
get_dht now chooses the AC/DC base array first and applies the index after its
existing validation. All legal slot destinations and expected error code/index
values pass, with the routine and probe compiled as C. Pointer mode also passes.
Explicit unit/collision regeneration has no golden diff; one-ULP control, normal
smoke and fixed-demo replay pass unchanged. Symbols pass; only get_dht changes
normalized assembly among 15 functions, reviewed as the expected pointer-lifetime
change. No gate weakening. Upstream C test fails before and passes after:
https://github.com/ec-/Quake3e/pull/434. Known-bugs has no entries, and ubsan.supp
is empty. Self-review: one vendor bounds bug, existing unit runner, no file loading,
FP, layout, engine OS-access, allocation or destructor change. Regression
34891606879 and full build 34891606634 pass on source 01f2dd40. Final checkpoint
changes documentation only; self-review passes.

## #1 acceptance evidence

Source e0013d9085f56ddd4d726a15e5c5a724dc9272f3 passed regression 34892972996 and
full build 34892972994. Local Clang 21 and hosted Clang analysis pass 140 engine
translation units using 356 compilation commands across both renderer configurations.
Controls reject seven owning objects, accept trivial/defaulted objects and pointers,
and verify core inclusion/platform exclusion. Clang's AST `destroyed` annotation
comes directly from VarDecl::needsDestruction; the controls detect format drift.
Section 11 records the retained longjmp rationale, wrapper restriction, and inactive
preprocessor-branch/self-review limitation. AGENTS.md and README document the command.
No engine source or golden changed. Final self-review passes; docs checkpoint only.

## #31 native math acceptance evidence

Source 152cc6e294a37772c0d942fb1ce69dadb1d57c9d passed regression 34894597080 and
full build 34894597081. GCC/Clang optimized and ASan C checks pass all eight words
from the unmodified 32-bit SSE C executable. Explicit unit/collision regeneration
has zero diff. Symbols pass; only Q_rsqrt changes normalized assembly among 47
functions, with unchanged FP arithmetic order. No expectation/suppression existed.
Self-review: one LP64 native word-width defect, three prerequisite GPL source/header
imports, no production engine/FP/layout/OS/allocation/destructor change. This C
math dependency is test-only until #2. Final checkpoint changes documentation only.

Independent temporary #2 preflight: seven shared ABI sizes and three offsets match
between original GPL C headers and engine C++ headers (/tmp/aftershock-native-layout-*.txt).
Base-game native smoke with original bg_lib support reproduces all 36 accepted
q3dm17 gameplay events. Full text differs only in module-loading metadata, build
date and bot-skill padding (custom VM printf vs native libc). This is preliminary
single-map evidence, not completed parity. Temporary UI compiles; cgame additionally
requires upstream code/ui/ui_shared.h, an include omitted from the initial #2 import.
Do not remove VM/JIT paths or change accepted fixtures before full native parity.

## #31 movement result validation

The poisoned-output production-engine test fails before (3993d575) and passes after
for GCC and Clang/libc++. The upstream C test fails before with GCC and passes after
with GCC and Clang: https://github.com/ec-/Quake3e/pull/435. BotMoveToGoal now zeroes all 52 bytes before lookup/early returns;
no FP expression, layout, allocation, OS access or destructor change. Symbols pass;
only BotMoveToGoal differs among 33 assembly functions: additional zero stores and
register allocation changes, reviewed as expected for this tested bug fix.

Explicit unit/collision regeneration preserves their hashes (8d44421d / 9674cd22).
q3dm17 is unchanged. q3dm7 changes only by Major's two-line chat (one say event),
now hash 028fba42; native and QVM match after implementation metadata normalization.
OpenArena obstacle avoidance no longer depends on stale caller memory: oa_dm1 has
70 rather than 75 Item events, still four kills, one rather than two say events
(hash 9ca81956); oa_dm7 has 58 rather than 71 Item events, five rather than two kills,
and two rather than one say events (5a511a91). Both maps repeat identically before
these explicit golden writes. No recording/frame golden changes. Fixed replay
passes both maps/renderers for Quake 3 (b38004b1) and OpenArena (5b89d338).
No expected-failure entry or UBSan suppression covered uninitialized movement output.
Regression 34900480717 and full build 34900480656 pass on source
fc49615d4be82ff41110f6d521ee6945dd376739. GCC UBSan Quake 3 smoke also passes both
updated goldens. Self-review: one #31 initialization defect, production-body test
first, explained golden changes only, unchanged file/wire layout and FP expressions,
no new engine OS calls, non-trivial destructors or allocations. Issue updated;
upstream #435 open. This final checkpoint changes documentation only.

#2 preparation only: OpenArena oaxB52 source tag resolves to
331464ca396d80e91cf9be273588f2b5f4b7afc8, matching the release used by hosted QVM
fixtures. Clone is /tmp/aftershock-oa-native-source; no native OA build or code change
yet. GCC and Clang both accept their binary32 literal flags in C++20 as well as C.
Temporary C++ compilation of the base game lists expected enum/pointer/constness,
FOFS pointer-to-int and old-style definition conversions; no C++ source port begun.

## #31 native dispatch validation

The test at 6d4710b4 uses the actual production VM_Call and a native entry stub;
counts 3, 0, 1, 2 check every delivered argument, return value and restored call
depth. Clang 21 crashes before at the zero-count call. After initializing unused
slots, GCC and Clang/libc++ pass. The native Clang module from #2 then reproduces
both QVM Quake 3 logs through the transition driver (6dad7c18 / a15c9c91 normalized).
Default QVM bot smoke and both-map/both-renderer fixed replay remain unchanged.
Explicit unit/collision regeneration has zero diff. Symbols pass; only VM_Call
changes among 26 assembly functions: zero stores and native-path control flow;
no source FP, layout, OS, allocation or destructor changes. No known-bug entry or
suppression covered the native uninitialized arguments. Regression 34908517245 and
full build 34908517199 pass on source 3201b7fa8babcd54be0129fac3c0d0ab99349dcf.
Self-review: one shared dispatch initialization bug, failing test first, no unrelated
refactoring, unchanged file/wire layout, FP expressions, allocation, OS access and
trivial lifetime rules. Issue updated, upstream #436 open. Final checkpoint docs only.

Native dispatch upstream C fix/test: https://github.com/ec-/Quake3e/pull/436.

Next #31 prerequisite investigation: Clang -std=gnu99 -O2 -Werror=array-bounds
-fsyntax-only on the original pinned GPL ai_cmd.c and ai_team.c independently
rejects both index-32 teamleader writes. Other writes already use ClientName or
Q_strncpyz. These game files are absent from ec-/Quake3e, and pinned OpenArena
already terminates at sizeof(teamleader)-1. Keep this native import defect scoped
to its own #31 PR; do not introduce unrelated game imports upstream.

## #31 team-leader bounds validation

Test-first commit fc665341 imports only code/game/ai_cmd.c and ai_team.c from GPL
revision dbe4ddb10315479fc00086f08e25d968b4b43c49, retaining their notices. Full native
integration remains #2. `python3 tests/teamleader.py` checks the actual C functions
against the pinned, clean original bot-state headers with Clang bounds errors.
Both original index-32 writes fail; both bounded-copy replacements pass. The test
fetches public source headers when absent, never game assets. CI runs it on Clang.
Original source SHA256s and reproducer are in cpp-port-notes.md.

GCC -O2 -DNDEBUG defined-symbol comparison passes. Of 43 ai_cmd and 22 ai_team
functions, only BotMatch_StartTeamLeaderShip and BotTeamAI change normalized assembly:
the existing Q_strncpyz call replaces strncpy plus the out-of-bounds byte store;
the first function also reallocates one register. No FP instruction changes.
No expected-failure entry or UBSan suppression covered this compile-time failure.

Explicit unit/collision regeneration produces zero golden diff (8d44421d /
9674cd22). Gameplay/frame fixtures are unaffected by these test-only prerequisite
imports; the existing CI runtime gates remain required.

PR #53 source 0ff62c302c96e00f929f4537e7bc8559805f2c29 passed regression
34909591046. Full build 34909591164 compiled macOS release successfully but its
artifact upload timed out at CreateArtifact (ETIMEDOUT); retry only the failed job
after the remaining build job finishes. This is not a source/build failure and
is not counted as a passing full-build gate. The original GPL ai_team.c ends in
a blank line, preserved byte-for-byte in the prerequisite import; the fix itself
has no whitespace-only changes.

Full build 34909591164 attempt 2 passed on the same source; only the failed macOS
job was rerun. PR #53 self-review: one #31 bug, exact two-file prerequisite import,
only two bounded-copy changes; no new OS access, non-trivial lifetime, allocation,
wire/file layout or FP expression changes. Defined-symbol/codegen review and the
failing-before/passing-after test pass. Goldens/fixtures remain unchanged; no
expectation or suppression applies. This final checkpoint changes documentation only.

#2 resumed at merge 383c53e0. Permanent Clang C native smoke passes both Q3 maps
(6dad7c18 / a15c9c91 normalized accepted logs); Clang C native fixed replay passes
both maps/renderers with the unchanged frame hash b38004b1 and original fixtures.
Commands use --game-code native --cc clang --cxx 'clang++ -stdlib=libc++'; logs:
/tmp/aftershock-native-runtime-clang-final.log and
/tmp/aftershock-native-demo-clang-final.log. GCC parity was already measured before
this bounds-only fix; the unused team paths are now covered by tests/teamleader.py.
OpenArena C native preflight is confined to /tmp/aftershock-oa-native-work from the
pinned source: transitional intptr_t syscall words, entry signatures and the same
binary32 ABI header. No repository OA source import or golden changes yet.

OpenArena native preflight builds game/cgame C modules but oa_dm1 stops before bot
startup: SP_func_door passes NULL ent->targetname through strequals to libc strcmp.
GDB confirms __strcmp_avx2 -> SP_func_door -> G_CallSpawn -> G_InitGame. Reproducer:
/tmp/aftershock-oa-native-smoke.py and /tmp/aftershock-oa-native-gdb.py (logs alongside).
The original macro is code/qcommon/q_shared.h:712; nullable targetname also reaches
it in three g_main.c paths. This is an external OpenArena game-source #31 item;
no patch has been made. Quake 3 native compiler/replay parity is complete, so its
C++ catalog port can proceed independently while OA remains a failing prerequisite.
The permanent native OA build and UI source mapping are not yet implemented.

## #2 C++ compatibility deviations

Baseline 3306d55d compiles 103 native C release objects (module-local shared sources
included) with GCC -O2 -DNDEBUG and the established native ABI flags. Hash manifest:
/tmp/aftershock-native-c-object-gate/before/sha256.json. This precedes all C++ edits.
Two necessary syntax adaptations outside T1–T25 are isolated in their own commit:
- FOFS uses `(int)offsetof(gentity_t, x)` plus stddef.h instead of narrowing a pointer
  expression directly to int, rejected by 64-bit C++. The int field representation
  and every measured offset stay the same; no entity layout change.
- Three bg_lib sort helper definitions use prototype parameter lists with their
  original types, replacing K&R definitions that C++ cannot parse. Bodies unchanged.
All 103 C release objects remain byte-identical after these changes (zero changed
SHA256s), including the field table and sort code. Evidence command:
python3 /tmp/aftershock-native-c-object-gate.py deviations. No simulation expression
or golden change; no new algorithm or bug fix. Ordinary catalog casts/renames follow
in separate commits. C++ syntax preflight initially reports 50 of 100 module TUs
failing; bg_lib adds old-style-definition errors. No permissive flags are enabled.

First ordinary catalog pass: T1 pointer casts, T2 boolean-expression casts, T3 enum
casts, T4 delete member -> deleteButton, T14 register removal, T20 const search
results (or casts where the shared receiving pointer also mutates writable text).
All 103 native C release objects remain byte-identical to 3306d55d; including
uis.debug's T3 compound-assignment spelling, so no T25 branch is needed. The field
rename leaves the menu asset paths unchanged. During review a broad temporary
replacement also changed two string literals; those were restored before this
passing gate and are not part of the commit. Strict C++ syntax now proceeds to
string-literal constness under -Werror=write-strings; no warning suppression.
Evidence: /tmp/aftershock-native-c-catalog-final.log; 103 unchanged hashes.

T8 read-only declarations now cover the three cvar tables, item names/media, spawn
and command names, menu artwork fields and fifteen diagnosed string-pointer arrays.
Receiving locals and existing extern array declarations retain matching qualifiers.
Public function signatures are unchanged; the one parameter qualified is a static
UI helper. menutext_s.string remains mutable because several menus fill its backing
buffer; their literal assignments still need call-site casts. All 103 native C
release objects remain byte-identical (const-final log). Next: remaining T8 literal
casts at unchanged public APIs/return sites, then C++ exports and full module gates.

Remaining T8 sites retain the existing public char* interfaces and mutable UI text
fields: 1,060 diagnosed literal/macro-expression casts across 71 files. Macro
constants and concatenated string contents are unchanged; casts are at use sites.
All 103 TUs now pass GCC C++20 syntax with -Werror=write-strings and
-Werror=register, no permissive flags. All 103 C release objects still have the
original byte hashes (literal-casts gate). Next: T5 exports, Clang C++ diagnostics,
linked C++ native ABI/symbol/codegen comparison and permanent smoke/replay parity.

T5 marks exactly dllEntry/vmMain in all three modules with guarded Q_EXTERN_C.
T15 adds eleven required literal/macro separator spaces in ai_team/g_cmds. GCC
and Clang now pass all 103 C++20 syntax checks. All 103 C release objects remain
byte-identical (exports gate). No other C-linkage annotations or math edits.

Native build/check commands now expose --language c++ (module builder) and
--game-language c++ (runtime/replay). GCC and Clang/libc++ link all three modules
and match the 29 ABI layouts/three offsets. GCC C++ bot smoke matches both accepted
Q3 logs; fixed replay is running. The linked-module check uses -z defs.

Isolated build compatibility deviation: Clang C++ at -O2 rejects bg_lib.c's atof
because glibc has already defined an optimized extern-inline atof. The builder
compiles only this compatibility TU separately with -D__NO_INLINE__; this controls
glibc header definitions, not optimizer inlining. Other TUs are unchanged. Clang
C bg_lib raw object SHA256 is identical with/without the setting:
028960a967ce3710c7b994aa6743e7eb640ca8bc9a40e10d7fdd822f0b2578c0.
GCC does not receive it (its object would change). Evidence:
/tmp/aftershock-native-bg-lib-inline; all-module Clang C++ link now passes.
The function bodies, caller arithmetic and external atof symbol are retained.

GCC C++ fixed replay passes all original Q3 samples (b38004b1); Clang C++ all-module
links pass after the scoped header setting. Artifact gates identify real pending
C/C++ library/header differences plus compiler symbol/table numbering; they are
not yet marked passed. CI also exposed the standalone #31 team-leader check's
missing COM_TRAP_GETVALUE definition after #2 imported the complete local headers.
The check now uses the native ABI header and local bot-state types, removing its
obsolete external header fetch; it passes locally. No bug fix or gate suppression.

Artifact review caught the two T22 sites in ai_main: AngleDifference(...) and
forward[2] passed to abs. Explicit int casts preserve the C call's conversion;
these are port compatibility edits, not FP expression restructuring. The C++
front-end's default _GNU_SOURCE also redirected scanf/strtol to C23 symbols while
the C99 reference used C99/legacy entries. The native C++ builder now uses
-U_GNU_SOURCE -D_DEFAULT_SOURCE, matching the C feature set; focused GCC/Clang
objects both reference __isoc99_sscanf and strtol again. Seven of 103 objects
still differ in undefined library dependencies (ctype macro vs function calls,
plus strstr-to-strchr optimization); no defined-symbol difference is reported.
These require explicit review, not a blanket normalizer. Artifact reports:
/tmp/aftershock-native-cpp-gates-pinned/results.json and per-object diffs.

Permanent native_shared.py compares actual shared math (4,096 samples including
zero/quadrant angles) and Q_strlwr/Q_strupr for all nonzero bytes in the C locale.
GCC C vs C++ and Clang C vs C++ agree: math 67988592, case 676e85f5. This covers
AngleVectors' packed/scalar compiler variation and libc ctype macro/function paths;
no fixture/golden writes. CI now builds C++ modules and runs this differential in
both unit compiler jobs. Clang C++ smoke and fixed replay pass both maps/renderers,
using unchanged Q3 fixtures/frame hash b38004b1. Logs:
/tmp/aftershock-native-runtime-cpp-clang.log and
/tmp/aftershock-native-demo-cpp-clang.log. GCC C++ passed those same outputs earlier;
T22 and feature-setting changes still require its final runtime/replay rerun.
Regression 34911920499 passed on checkpoint 42675c08; current CI will validate the
new permanent shared-function and C++ build steps. Artifact G3/G4 review remains
open (seven dependency diffs; no defined-symbol mismatch; per-object reports under
/tmp/aftershock-native-cpp-gates-pinned). Do not label those gates complete yet.

Final GCC C++ rerun after T22/library-feature pinning passes both smoke logs and
all fixed replay frames; Clang C++ passes the same. Regression 34912410405 passed
on c386658a, including both C++ module builds and shared-function differentials.
All 103 defined-symbol sets and raw dllEntry/vmMain spellings match the C baseline;
the seven remaining undefined-dependency diffs are fully enumerated in the reports.
Artifact codegen review remains open before static integration. Next is the separate
#31 OpenArena nullable-target helper fix, keeping #2 checkpointed on this branch.

## #31 OpenArena absent target names

Pinned public OpenArena source: 331464ca396d80e91cf9be273588f2b5f4b7afc8 (oaxB52).
The header macro strequals calls strcmp directly. Native oa_dm1 crashes because
SP_func_door passes absent targetname; three g_main elimination-target paths use
the same helper with nullable names. All call sites were checked. The fixed helper
returns false if either name is absent and evaluates each argument once, retaining
case-sensitive comparison and equality for present empty strings. All non-null
call behavior remains strcmp equality. This fixes the shared cause once.

`python3 tests/openarena_strings.py` reads three public headers from the exact Git
revision into its own output directory, preserving notices. It applies the checked-in
patch there and compiles the real helper with UBSan; no game assets or new loader
are involved. Test-first 43a3dac3 fails (argument 2 NULL); GCC/Clang pass after.
No expected-failure entry or suppression covered this newly observed external-source
bug. ec-/Quake3e lacks the helper and corresponding game code, so no applicable
engine upstream PR exists. #2 will consume this patch for its native CI build.

Native OA smoke now passes both maps with the patch: normalized hashes 51d66d9a
(oa_dm1) and 0f2e6b68 (oa_dm7). The first startup succeeded but still used libc rand;
linking #2's already verified QVM rand/sort compatibility library restored gameplay
parity. Three additional VM-loader metadata lines are excluded in the temporary
native comparison; no gameplay text is removed. This build adaptation belongs to
#2 and is not an additional source fix in this PR. Temporary driver:
/tmp/aftershock-oa-target-fix-smoke.py; no native OA fixture recordings.

All 13 source files that call strequals pass defined/undefined-symbol comparison
before/after. Nineteen function bodies change codegen through null checks and
associated branch/register allocation; no function is added or removed at -O2.
The helper preserves strcmp equality whenever both pointers are present. The
G_FindTeams pair already guards both team pointers before comparison; missing
names never become matching team names. Explicit unit/collision regeneration has
zero golden diff. Existing engine/runtime source is unchanged by this patch-only
CI dependency fix. CI and PR self-review are still required before merge.

PR #54 source 07ea4fe3 passed regression 34913347473 and full build 34913347479.
Self-review: one external dependency bug, tested before/after; patch scoped to the
shared helper; no new engine OS access, allocation, lifetime, layout or FP edits;
all caller symbols preserved, null-guard codegen reviewed, goldens unchanged.
No expectation/suppression applies. This checkpoint is documentation only.

Additional #2 client preflight while CI ran: native OA cgame/UI compile and load.
Fixed oa_dm7 replay matches all six accepted samples; oa_dm1 differs on both
renderers in a roughly 107x108 pixel region (about 4,690 pixels at sample 50).
No replay is regenerated or claimed passing. Artifacts:
/tmp/aftershock-oa-native-demo and /tmp/aftershock-oa-native-demo-preflight.py.
Client build helper /tmp/aftershock-oa-native-client-build.py maps base UI objects
to code/q3_ui, uses code/ui/ui_syscalls.c, maps bg_* to code/game and links #2's
QVM random/sort library. Native OA frame parity remains #2 work after this fix.

#63 explicit unit/collision golden regeneration is byte-identical; static OA
smoke also matches both accepted bot logs. No golden/fixture change.

Platform checkpoint: all macOS configurations and the completed MSVC configurations
pass on 440089eb in build 34936092520; remaining build jobs are running. Local
MinGW native-Windows client (USE_CURL=0 USE_SDL=0, matching CI) links successfully.
The optional MinGW SDL/no-curl build exposed old missing Windows header context;
record it separately for #31 without changing those engine sources here.

#5 documentation correction: the untouched finished network driver belongs to
#3 merge 8692b422, before both path and build migrations, not the 390a20f4 build
parity checkpoint. It is not rerun or adapted here. README and AGENTS now identify
its historical revision explicitly.

UI skill investigation during #8: a bounded real UI_SPSkillMenu_SkillEvent call
with g_spSkill=1e38 and a valid ID_EASY event fails GCC and Clang
undefined,float-cast-overflow checks at ui_spskill.cpp:114. Q_atof rejects
NaN/Inf but accepts this large finite value. Other UI readers also cast before
validation. Record and fix separately under #31; no bug fix in this warning PR.
Local reproducer/logs: /tmp/aftershock-ui-skill-probe.cpp and
/tmp/aftershock-ui-skill-before{,-gcc}.log. Preserve each reader's existing
valid-value behavior and invalid-value policy.

UI skill fix evidence (2475e0d2): all five UI readers use UI_GetSkill, which
clamps the finite cvar value to 0..6 before conversion. Values outside 1..5 remain
invalid for existing callers, including truncation of 5.9 to 5. Level-menu reset,
score rejection, menu clamping and selected button behavior are preserved. The
new helper is UI-internal; no engine/public contract or layout changes.

Both Clang C/C++ helper builds and 103-object ABI gates pass with no layout or
symbol differences and the same 60 advisory C/C++ codegen differences. Across
32 production UI objects (GCC/Clang release, GCC debug, MinGW), only the skill
readers change instructions and UI_GetSkill is added. 122 shifted constant
references were checked against their actual bytes; unrelated instructions are
preserved. Artifacts /tmp/aftershock-ui-skill-codegen/{after,review}.json.
ARM64 server configurations do not contain UI; hosted client cross-builds remain.

Explicit unit/collision regeneration is byte-identical (8d44421d/9674cd22).
Q3 bot logs retain 6dad7c18/a15c9c91 and fixed replay retains b38004b1 with the
original two demo fixture hashes. No accepted file changed. Native provenance
records 2475e0d2 for the five imported files, preserving original GPL hashes.
Local logs /tmp/aftershock-ui-skill-{unit,differential,runtime,demo,native-*}.log.
The initial default /tmp/aftershock-tests configure encountered an old CMake
cache; the clean task-specific /tmp/aftershock-ui-skill-unit passed.

Null-subtraction source b6927331 matches the reviewed 25-object preview. The
explicit stdint.h include preserves line count. Both Clang native helper builds
pass, including their layout checks, and all six library hashes are unchanged:
/tmp/aftershock-null-subtraction-before.json and null-subtraction-{c,cpp}.log.
No accepted golden regeneration. PR #79 merged-tree run is 35025630301.

General parentheses preview: all 27 bot_moveresult_t_cleared callers pass the
simple identifier result. Removing declaration parentheses from the macro leaves
all 51 production/native objects byte-identical across GCC/Clang release, GCC
debug, MinGW and ARM64. GCC actual-source controls reject the old declaration
and accept the new one. No source change applied here; a later one-class PR can
remove -Wno-parentheses and change only that macro. Artifacts:
/tmp/aftershock-parentheses-declaration-preview/{results.json,*-control.log}.

UI skill PR #80 final: head 53569f88 passed full build 35025644781 and regression
35025644789; self-reviewed and merged 8b74ad07. Issue #31 comment 5688396029
records the complete validation and upstream applicability decision.

Unused-result preview (not applied): explicitly bind discarded console-write
results to [[maybe_unused]] auto locals. All five optimized GCC/Clang x86 and ARM
objects retain raw hashes; debug changes are confined to stores in the four
console functions. Existing output remains best-effort; no new error policy is
introduced. A separate class PR still needs review of those debug differences,
flag removal and hosted gates. /tmp/aftershock-unused-result-preview.

Address class local validation: all four GCC/Clang C/C++ native helpers/ABI
checks pass. Clang's six libraries match and GCC's game/cgame libraries match;
the GCC UI baselines predate the separately merged #80 fix. Artifacts:
/tmp/aftershock-address-before.json and address-{gcc,clang}-{c,cpp}.log.

Array-bounds preview is still under review: GCC 15 reports [0,4] outside qhandle_t[5]
on both skill-picture reads even after #80. Equivalent explicit dereference
*(skillMenuInfo.skillpics + (skill - 1)) removes the diagnostic. Clang objects
match, but GCC/MinGW/debug emit address-calculation changes requiring review.
No source change applied. /tmp/aftershock-array-bounds-preview. Do not use the
earlier ungrouped pointer variant; retain the original integer subtraction.

Fresh unused-parameter syntax inventory is running independently in
/tmp/aftershock-unused-parameter-inventory (driver .py, log .log). No source edits.
Both local GCC/Clang accept [[maybe_unused]] parameters in gnu99 helper mode;
Clang rejects nameless C definitions, so do not remove native parameter names.
A later class PR must verify hosted compiler compatibility and object hashes.

2026-09-19 resume: #81 full build/regression and #80 merged-tree run passed.
#81 merged 87907a26 after self-review; issue #8 comment 5742665588 records gates.
Unused-parameter inventory completed: 829 syntax configurations, zero compile
failures. The preview script stopped before edits because diagnostic columns
expand tabs; fix the column mapping in the temporary script before continuing.
No unused-parameter source edits have been applied to the repository.

Declaration source d0d8a7df: the existing complete movement-result regression
passes with GCC and Clang. Current production syntax checks pass across 2,380
configurations with the general parentheses warning enabled. New logs live in
/home/matt/.cache/aftershock-modernization/declaration-bot-move-{gcc,clang}.log.

Unused-parameter preview (not applied): 275 [[maybe_unused]] annotations in
88 source files, no headers or function-body changes. All 2,380 current production
syntax configurations pass with unused parameters treated as errors, after the
preview's relative vendor includes were connected to existing third_party sources.
The owned-code snapshot and changes.json are in unused-parameter-preview under
the persistent cache. Raw production/native object comparisons are running in
unused-parameter-objects; next verify C99 helper compatibility and review the diff
before its own warning-class PR. No annotations are in the repository yet.

Unused-parameter object preview complete: 665 of 833 production/native objects
match byte-for-byte. The other 168 are GCC debug objects and match after removing
only debug sections from copies. No instruction/data changes. Evidence:
unused-parameter-objects/{results,debug-review}.json in the persistent cache.
Four standalone C/C++ helper comparisons are still running.

Unused-parameter final local review: original source bytes are preserved after
removing the new parameter attributes, except trailing spaces on two touched
function-declaration lines (win_main.cpp/common.cpp). Source 05cb1e37 and GPL
provenance 3a3fca15 are committed. The UI export include's eight consuming production
objects are included in the 833-object comparison. Both compiler C/C++ helper
comparisons completed successfully, all twelve libraries retaining hashes.

Missing-initializer preview is outside the repository in the persistent cache:
15 files explicitly zero omitted members or use empty aggregate initialization.
The static allocator string blocks use a constexpr initializer to zero conditional
debug members without changing the layout. Compiler checks are running in
missing-initializers-check; object, C99 helper and conditional-build verification
remain before this separate warning-class change can be applied.

PR #84 follow-up 5623e8fe is integrated: all 281 parameter annotations retain
names and bodies, including six Windows debug validation callback parameters.
The full local MinGW debug client/server build passes and a before/after control
preserves native callback instructions/relocations. The initial regression
35449780958 passed; corrected-head full workflows are required. #83 merged-tree
regression 35449777481 passed. Persistent evidence: validation-callback/ and
validation-callback-check.log.

Initializer review: the four GCC debug common.cpp objects differ only in four
allocator __LINE__ immediates, each increasing by five source lines. Two MinGW
Sys_OpenVideoPipe objects reorder stores to distinct stack locations around a
comparison; mov does not change condition flags, and final stored bytes/branch
condition are unchanged. No added/removed functions or other instruction changes.
The explicit DWORD cast on si.cb leaves both reviewed MinGW native objects
byte-identical. All twelve native helper hashes/layouts are unchanged.
