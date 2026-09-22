# Recast/Detour 1.6.0

Unmodified source/header subsets for offline Recast generation and runtime
Detour/DetourCrowd. The original zlib License.txt and per-file copyright notices
are retained. provenance.json pins the upstream commit, downloaded archive and
every copied file. Demo, sample assets, debug drawing and tile-cache modules are
not imported; Aftershock owns its bounded tooling and debug display.

Integration compiles these sources statically. Recast belongs to the offline
cooker; Detour runtime storage uses the engine's explicit arena owner. No system
package, exception/RTTI dependency or runtime shared library is required.
