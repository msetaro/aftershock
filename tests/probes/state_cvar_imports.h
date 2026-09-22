// Engine payloads have their own real-cvar probe; these doubles exercise the
// native group's validation/apply ordering at its service boundary.
#pragma once
static int rejectCvarSlot = -1, cvarApplyCalls;
int GameImport_WriteCvarState( void *writer, const char *group, uint32_t, const char *name ) {
	assert(writer && group && name && !strncmp(group,"engine.game.cvars.",18));
	return 1;
}
int GameImport_ReadCvarState( const void *reader, const char *group, uint32_t slot, const char *name, int apply, int removable ) {
	assert(!removable);
	assert(reader && group && name && !strncmp(group,"engine.game.cvars.",18));
	if ( int( slot ) == rejectCvarSlot )
		return 0;
	cvarApplyCalls += apply != 0;
	return 1;
}
