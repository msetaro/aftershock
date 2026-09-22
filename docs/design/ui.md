# Data-authored UI (#17)

Use a bounded in-house immediate UI over the existing renderer and key-binding
contracts. The renderer already provides RHI-backed textured rectangles, and
SDL already translates controller buttons and sticks into engine key events.
The runtime owns POD records and fixed interaction state; it allocates no text,
layout or widget objects while drawing. The developer overlay remains ImGui.

The cooker uses the existing Pillow FreeType/RAQM stack to shape UTF-8 localized
labels, including right-to-left and combining text, into an atlas. This keeps
font files and shaping work offline. It also prepares numeric/key-label glyphs
for changing HUD values and binding labels. Authored strings are shaped as runs;
this does not claim arbitrary runtime chat/editor text shaping. A language or
font change is a source edit and recook. Missing shaping support fails the cook.

A source document declares its design canvas, locales, named text, and pages of
labels, buttons, sliders, key bindings and numeric HUD values. Rectangle offsets
are relative to an authored viewport anchor inside a configurable safe zone.
Scale preserves aspect and typography. Navigation uses the authored item order;
keyboard arrows and controller directions share focus/activation behavior.
Rebinding captures the next supported key/button and supports cancellation.
Actions are explicit page/map/resume/quit or bounded cvar changes, not arbitrary
script text. Hot reload validates a replacement before changing the live document.

RmlUi was considered. Its document/style integration and five host interfaces
add a second live UI runtime when the initial menu/options/HUD requirement can
reuse the existing drawing/input services. Revisit that choice if rich flow layout
or an editable rich-text interface becomes a concrete requirement.

Verification starts with a failing real cook, then native layout/navigation
checks at 1080p, 1440p, 4K and differing aspect ratios. The real client must show
main menu, options/rebinding and in-game HUD from cooked data, exercise controller
input and reload edited source. Existing content opts in with its UI document;
accepted classic-content demo fixtures stay unchanged.

References: [Pillow font layout](https://pillow.readthedocs.io/en/latest/reference/ImageFont.html)
and [RmlUi host interfaces](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces.html).
