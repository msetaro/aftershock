# Entity definitions

Issue #18 keeps the existing native entity array, callbacks and allocators. JSON
prefabs add component defaults; they do not introduce an ECS or a scripting VM.
The existing schema (`tools/agent/formats.py`, `describe entities`) is the source
format contract. The cooker resolves one-parent inheritance and property overrides
into bounded flat records, then publishes a hashed `.asent` asset (index kind 14).

A definition supplies `id`, inherited or explicit `native`, and `components`.
For example, an `item_health_mega` prefab with `pickup.amount: 30` creates a new
pickup without new native code. Map entity `classname` selects the prefab;
explicit map fields override its defaults. Runtime spawning follows the same
path. All classic item/spawn callbacks receive compatibility definitions at map
startup. The game keeps native classname for callbacks (including CTF flag and
path/target lookups), with authored identity in a separate private field for the
inspector. Original dispatch and simulation arithmetic remain unchanged. Empty
`g_entityDefinitions` keeps classic content behavior and logging unchanged.

| Component | Values and behavior |
| --- | --- |
| transform | Default origin and angles; an instance can override either. |
| pickup | Amount passed to the existing native item behavior. |
| hooks | Native target/targetname linkage; no script execution. |
| replication | Priority and radius passed to the existing entity replication policy. |
| model | Virtual model path, registered through the existing renderer contract. |
| animation | First frame, frame count, integer frame period and looping; existing IQM frame playback. |
| collision | Axis-aligned local bounds and solid flag. |
| trigger | Client touch cooldown and once flag; requires collision bounds. |
| damage | Touch damage, destructible health and optional explosion damage/radius. |
| audio | WAV or authored event for activation; continuous loops use WAV. |

The `composed` native behavior combines model, animation, collision, trigger,
damage, hooks and sound through existing game services. It owns one fixed POD
side record per entity slot. Trigger/use invokes targets and sound; a health
component allows weapon destruction. Animation frame counts must match the
selected model's intended clip. This layer does not migrate weapons, game modes,
AI, effects or their damage profiles; those are separate changes with tests.

There are at most 256 resolved definitions including classic compatibility names,
2048 total fields, 32 fields per prefab and 16 inheritance levels. Fixed records
use explicit widths and layout assertions. Reads validate the version/hash,
counts, indices, identifiers, field types and ranges before publication. Native
lookup, edits and serialization allocate nothing. Gameplay continues to use the
existing arena/entity storage and fixed timestep. Replication reflection is stored
with the same definition as gameplay defaults and must agree with its typed fields.

The devtools Definitions panel inspects any authored prefab's reflected fields.
Edits validate through the native field table and affect subsequent spawns;
existing entities retain their state. Apply/save run outside ImGui calls to keep
engine errors away from UI destructors. Numbered `entities/definitions.NNN.asent`
saves never overwrite earlier revisions and set `dev_definitionFile`. Set
`g_entityDefinitions` to that path and reload the map to reuse a saved revision.
Editing requires local cheats. JSON remains authoritative for inheritance and
component structure; cooked edits are flattened experimental snapshots. Recooking
the source intentionally replaces defaults with source values.

Acceptance uses new data-only pickups, generic save/reload, an owned animated
solid/destructible sound emitter, a once-only damage/target trigger and CTF flag
alias pickup/capture. Both
installed content sets exercise the real client. Classic bot logs and fixed demo
frames remain the compatibility oracle; accepted fixtures are not regenerated.
