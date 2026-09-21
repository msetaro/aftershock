// Bounded, allocation-free shadow cameras. Existing scene projections are unchanged.
#include "tr_local.h"
#include <cmath>

static bool ShadowFinite( const vec3_t value ) {
	return std::isfinite( value[0] ) && std::isfinite( value[1] ) && std::isfinite( value[2] );
}

static bool ShadowSize( int size ) {
	return size >= 16 && size <= 8192 && !( size & ( size - 1 ) );
}

static bool ShadowAxes( const vec3_t direction, vec3_t axes[3] ) {
	const float squared = DotProduct( direction, direction );
	if ( !std::isfinite( squared ) || squared < 1e-12f )
		return false;
	VectorScale( direction, 1.0f / sqrtf( squared ), axes[0] );
	MakeNormalVectors( axes[0], axes[1], axes[2] );
	VectorNegate( axes[1], axes[1] ); // Renderer view axes are forward, left, up.
	return true;
}

static void ShadowPlane( cplane_t *plane, const vec3_t normal, const vec3_t origin, float offset ) {
	VectorCopy( normal, plane->normal );
	plane->dist = DotProduct( origin, normal ) + offset;
	plane->type = PLANE_NON_AXIAL;
	SetPlaneSignbits( plane );
}

static void ShadowViewport( viewParms_t *view, int size ) {
	view->viewportWidth = view->viewportHeight = size;
	view->scissorWidth = view->scissorHeight = size;
	VectorCopy( view->orientation.origin, view->pvsOrigin );
}

bool R_ShadowSpotView( const vec3_t origin, const vec3_t direction, float fov, float zNear, float zFar, int size, viewParms_t *view ) {
	if ( !view || !ShadowFinite( origin ) || !std::isfinite( fov ) || fov <= 0 || fov >= 179 ||
		 !std::isfinite( zNear ) || !std::isfinite( zFar ) || zNear <= 0 || zFar <= zNear || !ShadowSize( size ) )
		return false;
	viewParms_t result = {};
	if ( !ShadowAxes( direction, result.orientation.axis ) )
		return false;
	VectorCopy( origin, result.orientation.origin );
	result.fovX = result.fovY = fov;
	result.zFar = zFar;
	ShadowViewport( &result, size );
	const float angle = DEG2RAD( fov * 0.5f );
	result.projectionMatrix[0] = result.projectionMatrix[5] = 1.0f / tanf( angle );
	result.projectionMatrix[10] = zNear / ( zFar - zNear );
	result.projectionMatrix[11] = -1;
	result.projectionMatrix[14] = zFar * result.projectionMatrix[10];
	for ( int plane = 0; plane < 4; plane++ ) {
		vec3_t normal;
		VectorScale( result.orientation.axis[0], sinf( angle ), normal );
		const float sign = plane % 2 ? -1.0f : 1.0f;
		VectorMA( normal, sign * cosf( angle ), result.orientation.axis[1 + plane / 2], normal );
		ShadowPlane( &result.frustum[plane], normal, origin, 0 );
	}
	ShadowPlane( &result.frustum[4], result.orientation.axis[0], origin, zNear );
	*view = result;
	return true;
}

bool R_ShadowPointView( const vec3_t origin, float zNear, float zFar, int face, int size, viewParms_t *view ) {
	if ( face < 0 || face >= 6 )
		return false;
	vec3_t direction = {};
	direction[face / 2] = face % 2 ? -1.0f : 1.0f;
	return R_ShadowSpotView( origin, direction, 90, zNear, zFar, size, view );
}

bool R_ShadowSunViews( const viewParms_t *camera, const vec3_t direction, float zNear, float distance, float splitWeight, int size, viewParms_t views[4], float splits[4] ) {
	if ( !camera || !views || !splits || !ShadowFinite( camera->orientation.origin ) ||
		 !std::isfinite( zNear ) || !std::isfinite( distance ) || zNear <= 0 || distance <= zNear ||
		 !std::isfinite( splitWeight ) || splitWeight < 0 || splitWeight > 1 || !ShadowSize( size ) ||
		 !std::isfinite( camera->fovX ) || !std::isfinite( camera->fovY ) ||
		 camera->fovX <= 0 || camera->fovX >= 179 || camera->fovY <= 0 || camera->fovY >= 179 )
		return false;
	vec3_t axes[3];
	if ( !ShadowAxes( direction, axes ) )
		return false;
	for ( int i = 0; i < 3; i++ ) {
		if ( !ShadowFinite( camera->orientation.axis[i] ) || fabsf( DotProduct( camera->orientation.axis[i], camera->orientation.axis[i] ) - 1 ) > 0.001f )
			return false;
		for ( int j = 0; j < i; j++ )
			if ( fabsf( DotProduct( camera->orientation.axis[i], camera->orientation.axis[j] ) ) > 0.001f )
				return false;
	}
	const float tanX = tanf( DEG2RAD( camera->fovX * 0.5f ) );
	const float tanY = tanf( DEG2RAD( camera->fovY * 0.5f ) );
	float previous = zNear;
	for ( int cascade = 0; cascade < 4; cascade++ ) {
		const float fraction = (float)( cascade + 1 ) * 0.25f;
		const float linear = zNear + ( distance - zNear ) * fraction;
		const float logarithmic = zNear * powf( distance / zNear, fraction );
		const float split = cascade == 3 ? distance : linear + splitWeight * ( logarithmic - linear );
		const float middle = ( previous + split ) * 0.5f;
		const float halfDepth = ( split - previous ) * 0.5f;
		// A sphere keeps extent independent of camera rotation. Reserve a texel
		// around the snapped center so every receiver corner stays inside.
		float radius = ceilf( sqrtf( split * split * ( tanX * tanX + tanY * tanY ) + halfDepth * halfDepth ) * 16 ) / 16;
		radius *= (float)size / (float)( size - 2 );
		const float texel = 2 * radius / (float)size;
		vec3_t center;
		VectorMA( camera->orientation.origin, middle, camera->orientation.axis[0], center );
		for ( int axis = 1; axis < 3; axis++ ) {
			const float position = DotProduct( center, axes[axis] );
			const float snapped = floorf( position / texel + 0.5f ) * texel;
			VectorMA( center, snapped - position, axes[axis], center );
		}
		viewParms_t result = {};
		AxisCopy( axes, result.orientation.axis );
		// ponytail: caster extrusion is bounded by the configured shadow distance;
		// increase that quality setting for tall off-camera occluders.
		VectorMA( center, -( radius + distance ), axes[0], result.orientation.origin );
		result.zFar = 2 * radius + distance;
		result.projectionMatrix[0] = result.projectionMatrix[5] = 1 / radius;
		result.projectionMatrix[10] = 1 / result.zFar;
		result.projectionMatrix[14] = result.projectionMatrix[15] = 1;
		ShadowViewport( &result, size );
		for ( int plane = 0; plane < 4; plane++ ) {
			vec3_t normal;
			VectorScale( axes[1 + plane / 2], plane % 2 ? -1.0f : 1.0f, normal );
			ShadowPlane( &result.frustum[plane], normal, result.orientation.origin, -radius );
		}
		ShadowPlane( &result.frustum[4], axes[0], result.orientation.origin, 0 );
		views[cascade] = result;
		splits[cascade] = split;
		previous = split;
	}
	return true;
}
