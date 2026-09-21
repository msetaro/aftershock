#include "cm_local.h"
#include "cm_patch.h"
#include "cm_public.h"
#include "surfaceflags_public.h"

// Load-time cosmetic geometry only. Movement and CM trace planes are untouched.
static bool CM_PhysicsWinding( winding_t *winding, bool ( *triangle )( void *, const float *, const float *, const float * ), void *context ) {
	if ( !winding )
		return true;
	bool accepted = true;
	for ( int i = 1; i + 1 < winding->numpoints && accepted; ++i )
		// CM windings are clockwise; the cosmetic mesh requires outward CCW faces.
		accepted = triangle( context, winding->p[0], winding->p[i + 1], winding->p[i] );
	FreeWinding( winding );
	return accepted;
}
bool CM_PhysicsTriangles( bool ( *triangle )( void *, const float *, const float *, const float * ), void *context ) {
	if ( !triangle || !cm.numLeafs )
		return false;
	auto *brushes = static_cast<byte *>( Hunk_AllocateTempMemory( size_t( cm.numBrushes ) + size_t( cm.numSurfaces ) ) );
	auto *surfaces = brushes + cm.numBrushes;
	memset( brushes, 0, size_t( cm.numBrushes ) + size_t( cm.numSurfaces ) );
	for ( int i = 0; i < cm.numLeafs; ++i ) {
		const auto &leaf = cm.leafs[i];
		for ( int j = 0; j < leaf.numLeafBrushes; ++j )
			brushes[cm.leafbrushes[leaf.firstLeafBrush + j]] = 1;
		for ( int j = 0; j < leaf.numLeafSurfaces; ++j )
			surfaces[cm.leafsurfaces[leaf.firstLeafSurface + j]] = 1;
	}
	// Inline entities/movers retain their native collision ownership.
	for ( int i = 1; i < cm.numSubModels; ++i ) {
		const auto &leaf = cm.cmodels[i].leaf;
		for ( int j = 0; j < leaf.numLeafBrushes; ++j )
			brushes[cm.leafbrushes[leaf.firstLeafBrush + j]] = 0;
		for ( int j = 0; j < leaf.numLeafSurfaces; ++j )
			surfaces[cm.leafsurfaces[leaf.firstLeafSurface + j]] = 0;
	}
	bool accepted = true;
	for ( int i = 0; i < cm.numBrushes && accepted; ++i ) {
		const auto &brush = cm.brushes[i];
		if ( !brushes[i] || !( brush.contents & CONTENTS_SOLID ) )
			continue;
		for ( int side = 0; side < brush.numsides && accepted; ++side ) {
			const auto *plane = brush.sides[side].plane;
			winding_t *winding = BaseWindingForPlane( plane->normal, plane->dist );
			for ( int clip = 0; winding && clip < brush.numsides; ++clip ) {
				if ( clip == side )
					continue;
				const auto *other = brush.sides[clip].plane;
				vec3_t normal;
				VectorNegate( other->normal, normal );
				ChopWindingInPlace( &winding, normal, -other->dist, .1f );
			}
			accepted = CM_PhysicsWinding( winding, triangle, context );
		}
	}
	for ( int i = 0; i < cm.numSurfaces && accepted; ++i ) {
		if ( !surfaces[i] || !cm.surfaces[i] || !( cm.surfaces[i]->contents & CONTENTS_SOLID ) )
			continue;
		const auto *patch = cm.surfaces[i]->pc;
		for ( int f = 0; f < patch->numFacets && accepted; ++f ) {
			const auto *facet = &patch->facets[f];
			const float *surface = patch->planes[facet->surfacePlane].plane;
			winding_t *winding = BaseWindingForPlane( surface, surface[3] );
			for ( int border = 0; winding && border < facet->numBorders; ++border ) {
				if ( facet->borderPlanes[border] == facet->surfacePlane )
					continue;
				const float *plane = patch->planes[facet->borderPlanes[border]].plane;
				vec3_t normal;
				const float sign = facet->borderInward[border] ? 1.f : -1.f;
				VectorScale( plane, sign, normal );
				ChopWindingInPlace( &winding, normal, plane[3] * sign, .1f );
			}
			accepted = CM_PhysicsWinding( winding, triangle, context );
		}
	}
	Hunk_FreeTempMemory( brushes );
	return accepted;
}
