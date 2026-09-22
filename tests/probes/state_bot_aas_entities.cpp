#include "../../engine/botlib/be_aas_entity.cpp"
#include <assert.h>
aas_t aasworld;
botlib_import_t botimport;
int main() {
	aas_entity_t entities[4]{};
	aasworld.entities = entities;
	aasworld.maxentities = 4;
	aasworld.initialized = 1;
	entities[2].i.valid = 1;
	entities[2].i.number = 2;
	entities[2].i.ltime = 41.25f;
	entities[2].i.update_time = 0.1f;
	entities[2].i.origin[0] = 64;
	entities[2].i.lastvisorigin[0] = 60;
	entities[2].i.old_origin[0] = 59;
	entities[2].i.modelindex = 3;
	entities[2].i.solid = SOLID_BBOX;
	entities[2].i.weapon = 4;
	entities[2].i.mins[2] = -24;
	entities[2].i.maxs[2] = 32;
	static unsigned char bytes[16384];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(AAS_WriteEntityState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	aas_entity_t loaded[4]{};
	aasworld.entities = loaded;
	aas_link_t link{};
	loaded[2].areas = &link;
	assert(AAS_ReadEntityState(reader,false) && !loaded[2].i.valid);
	assert(AAS_ReadEntityState(reader,true) && loaded[2].areas==&link);
	aas_entityinfo_t info{};
	AAS_EntityInfo( 2, &info );
	assert(!memcmp(&info,&entities[2].i,sizeof(info)));
	AAS_InvalidateEntities();
	assert(!loaded[2].i.valid && loaded[2].i.ltime==41.25f && loaded[2].i.lastvisorigin[0]==60);
	// Invalid entities still own history used by their next update.
	writer = { bytes, sizeof( bytes ) };
	assert(AAS_WriteEntityState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	loaded[2].i.ltime = 0;
	assert(AAS_ReadEntityState(reader,true) && loaded[2].i.ltime==41.25f);
	const int32_t count = 4;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,aasEntityCountSchema,0,&count));
	for ( uint32_t i = 0; i < 3; ++i )
		assert(State_Append(&writer,aasEntitySchema,i,&entities[i].i));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	loaded[2].i.ltime = 77;
	assert(!AAS_ReadEntityState(reader,true) && loaded[2].i.ltime==77);
	loaded[3].i.modelindex = MAX_MODELS;
	writer = { bytes, sizeof( bytes ) };
	assert(!AAS_WriteEntityState(&writer) && !State_Finish(&writer));
	puts( "PASS: AAS entity history survives invalidation and relocation without replacing spatial links" );
}
