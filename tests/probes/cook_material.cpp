// Repeated material edits must retain handles, ordering and bounded hunk storage.
#include "../../engine/render/tr_shader.cpp"
#include <assert.h>

refimport_t ri;
trGlobals_t tr;
static backEndData_t backend;
backEndData_t *backEndData = &backend;
static size_t allocations;
static void *storage;
static void *allocate( size_t size, ha_pref ) {
	assert( !storage );
	allocations++;
	storage = calloc( 1, size );
	assert( storage );
	return storage;
}
static void QDECL print( printParm_t, const char *, ... ) {
}
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}
void QDECL Com_Printf( const char *, ... ) {
	abort();
}
void R_DecomposeSort( unsigned, int *, shader_t **, int *, int * ) {
	abort();
}

int main() {
	ri.Hunk_Alloc = allocate;
	ri.Printf = print;
	shader_t neighbors[2] = {};
	for ( int i = 0; i < 2; i++ ) {
		neighbors[i].index = neighbors[i].sortedIndex = i;
		neighbors[i].sort = 3 + i;
		tr.shaders[i] = tr.sortedShaders[i] = &neighbors[i];
	}
	tr.numShaders = 2;
	InitShader( "models/test_material", LIGHTMAP_NONE );
	shader.reloadable = true;
	shader.sort = 2;
	shader.numUnfoggedPasses = 1;
	stages[0].active = qtrue;
	shader_t *live = GeneratePermanentShader();
	assert( live->index == 2 && live->sortedIndex == 0 && allocations == 1 );
	shaderStage_t *firstStage = live->stages[0];
	shader_t *next = live->next;
	live->remappedShader = &neighbors[0];
	live->timeOffset = 1.25;
	for ( int i = 0; i < 12; i++ ) {
		InitShader( "models/test_material", LIGHTMAP_NONE );
		shader.reloadable = true;
		shader.sort = i % 2 ? 2 : 5;
		shader.numUnfoggedPasses = i % 2 + 1;
		for ( int stage = 0; stage < shader.numUnfoggedPasses; stage++ ) {
			stages[stage].active = qtrue;
			stages[stage].stateBits = i;
		}
		assert( GeneratePermanentShader( live ) == live );
		assert( allocations == 1 && tr.numShaders == 3 && tr.shaders[2] == live );
		assert( live->stages[0] == firstStage && live->stages[0]->stateBits == (uint32_t)i );
		assert( live->next == next && live->remappedShader == &neighbors[0] && live->timeOffset == 1.25 );
		assert( live->sortedIndex == ( i % 2 ? 0 : 2 ) );
		for ( int index = 0; index < tr.numShaders; index++ ) {
			assert( tr.sortedShaders[index]->sortedIndex == index );
			if ( index )
				assert( tr.sortedShaders[index - 1]->sort <= tr.sortedShaders[index]->sort );
		}
		assert( hashTable[generateHashValue( live->name, FILE_HASH_SIZE )] == live );
	}
	free( storage );
	puts( "PASS: twelve material edits preserve handles, sorting and one hunk allocation" );
}
