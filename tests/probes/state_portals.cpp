#include "../../engine/qcommon/cm_test.cpp"
#include <cassert>
#include <cstdlib>

clipMap_t cm;
cvar_t *cm_noAreas;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

int main() {
	cArea_t areas[4]{};
	int portals[16]{};
	cvar_t noAreas{};
	cm_noAreas = &noAreas;
	cm.numAreas = 4;
	cm.checksum = 123;
	cm.areas = areas;
	cm.areaPortals = portals;
	CM_AdjustAreaPortalState( 0, 1, qtrue );
	CM_AdjustAreaPortalState( 0, 1, qtrue );
	CM_AdjustAreaPortalState( 1, 2, qtrue );
	assert( CM_AreasConnected( 0, 2 ) && !CM_AreasConnected( 0, 3 ) );
	byte before[1]{};
	assert( CM_WriteAreaBits( before, 0 ) == 1 && before[0] == 7 );
	unsigned char bytes[4096];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert( CM_WritePortalState( &writer ) );
	stateReader_t reader;
	assert( State_Open( bytes, State_Finish( &writer ), &reader ) );
	CM_AdjustAreaPortalState( 1, 2, qfalse );
	assert( CM_ReadPortalState( reader, false ) && !CM_AreasConnected( 0, 2 ) );
	assert( CM_ReadPortalState( reader, true ) && CM_AreasConnected( 0, 2 ) );
	byte restored[1]{};
	assert( CM_WriteAreaBits( restored, 0 ) == 1 && restored[0] == before[0] );
	CM_AdjustAreaPortalState( 0, 1, qfalse );
	assert( CM_AreasConnected( 0, 2 ) );
	CM_AdjustAreaPortalState( 0, 1, qfalse );
	assert( !CM_AreasConnected( 0, 2 ) );
	cm.checksum++;
	assert( !CM_ReadPortalState( reader, true ) && !CM_AreasConnected( 0, 2 ) );
	cm.checksum--;
	assert( CM_ReadPortalState( reader, true ) );
	cm.numAreas = 3;
	assert( !CM_ReadPortalState( reader, true ) );
	cm.numAreas = 4;
	// Valid archive framing cannot turn an asymmetric matrix into live state.
	const cmPortalHeader_t header{ 4, 123 };
	const stateField_t field{ "references", 0, 16, stateType_t::Int32 };
	const stateSchema_t schema{ "engine.portalReferences", 1, 1, sizeof( portals ), &field, 1 };
	int invalid[16]{};
	invalid[1] = 1;
	writer = { bytes, sizeof( bytes ) };
	assert( State_Append( &writer, cmPortalHeaderSchema, 0, &header ) && State_Append( &writer, schema, 0, invalid ) );
	assert( State_Open( bytes, State_Finish( &writer ), &reader ) );
	assert( !CM_ReadPortalState( reader, true ) && portals[1] == 2 );
	invalid[1] = invalid[4] = -1;
	writer = { bytes, sizeof( bytes ) };
	assert( State_Append( &writer, cmPortalHeaderSchema, 0, &header ) && State_Append( &writer, schema, 0, invalid ) );
	assert( State_Open( bytes, State_Finish( &writer ), &reader ) );
	assert( !CM_ReadPortalState( reader, true ) && portals[1] == 2 );
	puts( "PASS: restored portal reference counts preserve visibility and subsequent door closes" );
}
