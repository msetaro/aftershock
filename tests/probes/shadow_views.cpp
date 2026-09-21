// New lighting geometry: analytical coverage, not regenerated frame references.
#include "../../engine/render/tr_local.h"
#include <assert.h>
#include <cmath>
#include <initializer_list>
#include <stdio.h>

static void Close( float actual, float expected, float tolerance = 0.0001f ) {
	assert( fabsf( actual - expected ) < tolerance );
}

static void Project( const viewParms_t &view, const vec3_t point, vec3_t ndc ) {
	vec3_t relative;
	VectorSubtract( point, view.orientation.origin, relative );
	const vec4_t eye = { -DotProduct( relative, view.orientation.axis[1] ),
		DotProduct( relative, view.orientation.axis[2] ), -DotProduct( relative, view.orientation.axis[0] ), 1 };
	vec4_t clip = {};
	for ( int row = 0; row < 4; row++ )
		for ( int col = 0; col < 4; col++ )
			clip[row] += view.projectionMatrix[col * 4 + row] * eye[col];
	for ( int i = 0; i < 3; i++ )
		ndc[i] = clip[i] / clip[3];
}

static void Inside( const viewParms_t &view, const vec3_t point ) {
	vec3_t ndc;
	Project( view, point, ndc );
	assert( fabsf( ndc[0] ) <= 1.0001f && fabsf( ndc[1] ) <= 1.0001f );
	assert( ndc[2] >= -0.0001f && ndc[2] <= 1.0001f );
	for ( const auto &plane : view.frustum )
		assert( DotProduct( point, plane.normal ) >= plane.dist - 0.001f );
}

int main() {
	const vec3_t origin = { 17, -31, 63 }, direction = { 1, 2, -3 };
	viewParms_t view;
	for ( int face = 0; face < 6; face++ ) {
		assert( R_ShadowPointView( origin, 2, 512, face, 256, &view ) );
		Close( view.orientation.axis[0][face / 2], face % 2 ? -1 : 1 );
		vec3_t cross;
		CrossProduct( view.orientation.axis[0], view.orientation.axis[1], cross );
		for ( int i = 0; i < 3; i++ )
			Close( cross[i], view.orientation.axis[2][i] );
		for ( float distance : { 2.0f, 20.0f, 512.0f } ) {
			vec3_t point, ndc;
			VectorMA( origin, distance, view.orientation.axis[0], point );
			Inside( view, point );
			Project( view, point, ndc );
			Close( ndc[0], 0 );
			Close( ndc[1], 0 );
			if ( distance == 2 || distance == 512 )
				Close( ndc[2], distance == 2 ? 1 : 0 );
			for ( int side : { -1, 1 } ) {
				VectorMA( point, (float)side * distance, view.orientation.axis[1], point );
				Inside( view, point );
				Project( view, point, ndc );
				Close( ndc[0], -(float)side );
				VectorMA( origin, distance, view.orientation.axis[0], point );
			}
		}
	}
	assert( R_ShadowSpotView( origin, direction, 60, 2, 512, 256, &view ) );
	vec3_t point, ndc;
	VectorMA( origin, 100, view.orientation.axis[0], point );
	VectorMA( point, 100 * tanf( (float)M_PI / 6 ), view.orientation.axis[2], point );
	Inside( view, point );
	Project( view, point, ndc );
	Close( ndc[1], 1 );
	assert( !R_ShadowPointView( origin, 2, 512, 6, 256, &view ) );
	assert( !R_ShadowSpotView( origin, vec3_origin, 60, 2, 512, 256, &view ) );
	assert( !R_ShadowSpotView( origin, direction, 180, 2, 512, 256, &view ) );
	assert( !R_ShadowSpotView( origin, direction, 60, 512, 2, 256, &view ) );

	viewParms_t camera = {}, sun[4], moved[4];
	AxisClear( camera.orientation.axis );
	camera.fovX = 90;
	camera.fovY = 60;
	float splits[4], movedSplits[4];
	const vec3_t rays = { 0, 0, -1 };
	assert( R_ShadowSunViews( &camera, rays, 2, 2048, 0.5f, 1024, sun, splits ) );
	float nearDistance = 2;
	for ( int cascade = 0; cascade < 4; cascade++ ) {
		assert( splits[cascade] > nearDistance );
		assert( sun[cascade].viewportWidth == 1024 && sun[cascade].viewportHeight == 1024 );
		for ( float distance : { nearDistance, splits[cascade] } )
			for ( int y : { -1, 1 } )
				for ( int z : { -1, 1 } ) {
					const vec3_t corner = { distance, (float)y * distance, (float)z * distance * tanf( (float)M_PI / 6 ) };
					Inside( sun[cascade], corner );
				}
		// Off-camera geometry up the light ray must still be able to cast onto the slice.
		const vec3_t caster = { ( nearDistance + splits[cascade] ) * 0.5f, 0, 1024 };
		Inside( sun[cascade], caster );
		nearDistance = splits[cascade];
	}
	Close( splits[3], 2048 );
	// Small lateral camera motion must not move the sun's shadow texel grid.
	camera.orientation.origin[1] = 0.001f;
	assert( R_ShadowSunViews( &camera, rays, 2, 2048, 0.5f, 1024, moved, movedSplits ) );
	for ( int cascade = 0; cascade < 4; cascade++ ) {
		vec3_t a, b;
		Project( sun[cascade], vec3_origin, a );
		Project( moved[cascade], vec3_origin, b );
		Close( a[0], b[0], 0.000001f );
		Close( a[1], b[1], 0.000001f );
		Close( splits[cascade], movedSplits[cascade] );
	}
	assert( !R_ShadowSunViews( &camera, rays, 2, 2048, 2, 1024, sun, splits ) );
	assert( !R_ShadowSunViews( &camera, vec3_origin, 2, 2048, 0.5f, 1024, sun, splits ) );
	// Rotated camera and oblique sun still enclose every slice corner.
	const vec3_t angles = { 35, 127, 12 };
	AnglesToAxis( angles, camera.orientation.axis );
	VectorCopy( origin, camera.orientation.origin );
	for ( float weight : { 0.0f, 0.5f, 1.0f } ) {
		assert( R_ShadowSunViews( &camera, direction, 2, 2048, weight, 1024, sun, splits ) );
		nearDistance = 2;
		for ( int cascade = 0; cascade < 4; cascade++ ) {
			for ( float distance : { nearDistance, splits[cascade] } )
				for ( int y : { -1, 1 } )
					for ( int z : { -1, 1 } ) {
						vec3_t corner;
						VectorMA( origin, distance, camera.orientation.axis[0], corner );
						VectorMA( corner, (float)y * distance, camera.orientation.axis[1], corner );
						VectorMA( corner, (float)z * distance * tanf( (float)M_PI / 6 ), camera.orientation.axis[2], corner );
						Inside( sun[cascade], corner );
					}
			nearDistance = splits[cascade];
		}
	}
	puts( "PASS: six point faces, spot cone, reversed depth, cascade coverage and stable texel grid" );
}
