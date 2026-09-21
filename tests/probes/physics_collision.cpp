#include "engine/qcommon/cm_local.h"
#include "engine/qcommon/cm_patch.h"
#include "engine/qcommon/cm_public.h"
#include "engine/qcommon/surfaceflags_public.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

clipMap_t cm;
static unsigned blocks, triangles;
static double area;
static bool reject;
void QDECL Com_Error( errorParm_t, const char *, ... ) {
	std::abort();
}
void QDECL Com_Printf( const char *, ... ) {
}
void *Z_Malloc(size_t size) {
	++blocks;
	return std::calloc( 1, size ? size : 1 );
}
void Z_Free( void *block ) {
	assert(blocks);
	--blocks;
	std::free( block );
}
void *Hunk_AllocateTempMemory( size_t size ) {
	return Z_Malloc(size);
}
void Hunk_FreeTempMemory( void *block ) {
	Z_Free( block );
}
static bool Triangle( void *, const float *a, const float *b, const float *c ) {
	++triangles;
	float ab[3], ac[3], cross[3];
	VectorSubtract( b, a, ab );
	VectorSubtract( c, a, ac );
	CrossProduct( ab, ac, cross );
	area += .5 * std::sqrt( DotProduct( cross, cross ) );
	return !reject;
}
int main() {
	cplane_t planes[6]{};
	cbrushside_t sides[6]{};
	for ( int i = 0; i < 6; ++i ) {
		planes[i].normal[i / 2] = i % 2 ? -1.f : 1.f;
		planes[i].dist = 1;
		sides[i].plane = &planes[i];
	}
	cbrush_t brushes[2]{};
	for ( auto &brush : brushes ) {
		brush.contents = CONTENTS_SOLID;
		brush.numsides = 6;
		brush.sides = sides;
	}
	cLeaf_t leaves[2]{};
	for ( auto &leaf : leaves ) {
		leaf.numLeafBrushes = 2;
		leaf.numLeafSurfaces = 2;
	}
	int indices[] = { 0, 1 };
	cmodel_t models[2]{};
	models[1].leaf.firstLeafBrush = models[1].leaf.firstLeafSurface = 1;
	models[1].leaf.numLeafBrushes = models[1].leaf.numLeafSurfaces = 1;
	patchPlane_t patchPlanes[5]{};
	patchPlanes[0].plane[2] = 1;
	patchPlanes[0].plane[3] = 2;
	for ( int i = 0; i < 4; ++i ) {
		patchPlanes[i + 1].plane[i / 2] = i % 2 ? -1.f : 1.f;
		patchPlanes[i + 1].plane[3] = 1;
	}
	facet_t facet{};
	facet.numBorders = 4;
	for ( int i = 0; i < 4; ++i )
		facet.borderPlanes[i] = i + 1;
	patchCollide_t patch{};
	patch.numPlanes = 5;
	patch.planes = patchPlanes;
	patch.numFacets = 1;
	patch.facets = &facet;
	cPatch_t surface{};
	surface.contents = CONTENTS_SOLID;
	surface.pc = &patch;
	cPatch_t *surfaces[] = { &surface, &surface };
	cm.numLeafs = 2;
	cm.leafs = leaves;
	cm.leafbrushes = indices;
	cm.leafsurfaces = indices;
	cm.numBrushes = 2;
	cm.brushes = brushes;
	cm.numSurfaces = 2;
	cm.surfaces = surfaces;
	cm.numSubModels = 2;
	cm.cmodels = models;
	assert(CM_PhysicsTriangles(Triangle,nullptr));
	assert(triangles == 14 && std::fabs(area-28) < .001 && blocks == 0);
	reject = true;
	triangles = 0;
	assert(!CM_PhysicsTriangles(Triangle,nullptr));
	assert(triangles == 1 && blocks == 0);
	std::puts( "PASS: solid world brush/patch triangles, deduplication, inline exclusion and callback cleanup" );
}
