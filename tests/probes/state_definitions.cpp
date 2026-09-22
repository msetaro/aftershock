#define NATIVE_NAMESPACE game
#define NATIVE_SOURCE "game/g_spawn.cpp"
#include "../../game/module.cpp"
int main() {
	using namespace game;
	authoredDefinitions.header = { "checkpoint_definitions", 1, 1 };
	authoredDefinitions.definitions[0] = { "health_crate", "item_health_large", 0, 1, 2, 0, 0 };
	authoredDefinitions.fields[0] = { "pickup", "count", "92" };
	// Native entries precede the authored entry; the combined table is not ASENT.
	entityDefinitions.header.count = 2;
	entityDefinitions.definitions[0] = { "item_health_large", "item_health_large", 0, 0, 0, 0, 0 };
	entityDefinitions.definitions[1] = authoredDefinitions.definitions[0];
	entityDefinitions.header.fieldCount = 1;
	entityDefinitions.fields[0] = authoredDefinitions.fields[0];
	static unsigned char bytes[16384];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(G_WriteDefinitionState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	strcpy( authoredDefinitions.fields[0].value, "50" );
	entityDefinitions.fields[0] = authoredDefinitions.fields[0];
	assert(G_ReadDefinitionState(reader,false));
	assert(!strcmp(authoredDefinitions.fields[0].value,"50"));
	assert(G_ReadDefinitionState(reader,true));
	assert(!strcmp(authoredDefinitions.fields[0].value,"92"));
	assert(!strcmp(entityDefinitions.fields[0].value,"92"));
	assert(!strcmp(entityDefinitions.definitions[0].name,"item_health_large"));
	// Content topology mismatch must preserve both live registries.
	strcpy( authoredDefinitions.definitions[0].native, "item_health" );
	assert(!G_ReadDefinitionState(reader,true));
	assert(!strcmp(authoredDefinitions.definitions[0].native,"item_health"));
	assert(!strcmp(entityDefinitions.definitions[1].native,"item_health_large"));
	strcpy( authoredDefinitions.definitions[0].native, "item_health_large" );
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,entityDefinitionHeaderSchema,0,&authoredDefinitions.header));
	assert(State_Append(&writer,entityDefinitionSchema,0,&authoredDefinitions.definitions[0]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadDefinitionState(reader,true));
	strcpy( authoredDefinitions.fields[0].value, "-1" );
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteDefinitionState(&writer) && !State_Finish(&writer));
	authoredDefinitions = {};
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteDefinitionState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(G_ReadDefinitionState(reader,true));
	puts( "PASS: edited definition values restore into the matching native registry without partial updates" );

#ifdef AFTERSHOCK_DEVTOOLS
	for ( auto &source : devSource )
		source = -1;
	devSource[7] = 0;
	devDocumentCount = 2;
	devComplete = true;
	strcpy( devDocuments[0], "{\n\"classname\" \"target_position\"\n\"editor_unknown\" \"keep me\"\n}\n" );
	devDocuments[1][0] = 0;
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteEditorState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	devDocumentCount = 0;
	devComplete = false;
	devSource[7] = -1;
	strcpy( devDocuments[0], "different" );
	assert(G_ReadEditorState(reader,false) && devDocumentCount==0);
	assert(G_ReadEditorState(reader,true));
	assert(devDocumentCount==2 && devComplete && devSource[7]==0);
	assert(strstr(devDocuments[0],"editor_unknown") && !devDocuments[1][0]);
	assert(devCurrentSource==-1 && devSpawned==0);
	editorSave_t incomplete{};
	incomplete.enabled = 1;
	incomplete.complete = 1;
	incomplete.count = 2;
	for ( auto &source : incomplete.sources )
		source = -1;
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,editorSaveSchema,0,&incomplete));
	assert(State_Append(&writer,editorDocumentSchema,0,devDocuments[0]));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!G_ReadEditorState(reader,true));
	assert(devDocumentCount==2 && devSource[7]==0);
	strcpy( devDocuments[0], "{\n\"key\" missing quotes\n}\n" );
	writer = { bytes, sizeof( bytes ) };
	assert(!G_WriteEditorState(&writer) && !State_Finish(&writer));
#else
	writer = { bytes, sizeof( bytes ) };
	assert(G_WriteEditorState(&writer));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(G_ReadEditorState(reader,true));
#endif
	puts( "PASS: checkpoint editor documents and source mappings validate before publication" );
}
