#include "animation_public.h"
#include "../../third_party/sha256/sha-256.h"
#include <math.h>
#include <string.h>

static const animTransform_t identity = { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } };

template <typename T>
static T Read( const animAsset_t *asset, animSectionIndex_t section, uint32_t index ) {
	T value;
	memcpy( &value, asset->data + asset->header.sections[section].offset + sizeof( T ) * index, sizeof( value ) );
	return value;
}
static uint32_t Count( const animAsset_t *asset, animSectionIndex_t section ) {
	return asset->header.sections[section].count;
}
static bool Range( uint32_t first, uint32_t count, uint32_t total ) {
	return first <= total && count <= total - first;
}
static bool Name( const char *name ) {
	return name[0] && memchr( name, 0, 64 );
}
static bool Finite( const float *values, uint32_t count ) {
	for ( uint32_t i = 0; i < count; ++i )
		if ( !isfinite( values[i] ) )
			return false;
	return true;
}
static float Dot( const float *a, const float *b, uint32_t count ) {
	float value = 0;
	for ( uint32_t i = 0; i < count; ++i )
		value += a[i] * b[i];
	return value;
}
static bool Normalize( float *v, uint32_t count ) {
	const float squared = Dot( v, v, count );
	if ( !isfinite( squared ) || squared < 1e-20f )
		return false;
	const float inverse = 1 / sqrtf( squared );
	for ( uint32_t i = 0; i < count; ++i )
		v[i] *= inverse;
	return true;
}
static float Clamp( float value, float minimum, float maximum ) {
	return fminf( maximum, fmaxf( minimum, value ) );
}
static void MultiplyQuaternion( const float *a, const float *b, float *out ) {
	const float value[4] = {
		a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
		a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
		a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
		a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]
	};
	memcpy( out, value, sizeof( value ) );
}
static void Blend( const animTransform_t *a, const animTransform_t *b, float weight, animTransform_t *out ) {
	const float sign = Dot( a->rotate, b->rotate, 4 ) < 0 ? -1.0f : 1.0f;
	for ( uint32_t i = 0; i < 3; ++i ) {
		out->translate[i] = a->translate[i] + ( b->translate[i] - a->translate[i] ) * weight;
		out->scale[i] = a->scale[i] + ( b->scale[i] - a->scale[i] ) * weight;
	}
	for ( uint32_t i = 0; i < 4; ++i )
		out->rotate[i] = a->rotate[i] + ( sign * b->rotate[i] - a->rotate[i] ) * weight;
	if ( !Normalize( out->rotate, 4 ) )
		memcpy( out->rotate, identity.rotate, sizeof( out->rotate ) );
}

bool Anim_Open( const void *bytes, size_t size, animAsset_t *asset ) {
	if ( !bytes || !asset || size < 48 + sizeof( animFileHeader_t ) || size > ( 16u << 20 ) + 48 )
		return false;
	const uint8_t *data = static_cast<const uint8_t *>( bytes );
	uint32_t version, payloadSize;
	memcpy( &version, data + 8, 4 );
	memcpy( &payloadSize, data + 12, 4 );
	if ( memcmp( data, "ASANIM\0\0", 8 ) || version != 1 || payloadSize != size - 48 )
		return false;
	uint8_t hash[32];
	calc_sha_256( hash, data + 48, payloadSize );
	if ( memcmp( hash, data + 16, sizeof( hash ) ) )
		return false;
	animAsset_t result = {};
	result.data = data + 48;
	result.size = payloadSize;
	memcpy( &result.header, result.data, sizeof( result.header ) );
	const auto &h = result.header;
	constexpr size_t sizes[] = { sizeof( animFileJoint_t ), sizeof( animFilePose_t ), sizeof( uint16_t ), sizeof( animFileClip_t ), sizeof( animFileParameter_t ), sizeof( animFileState_t ), sizeof( animFileTransition_t ), sizeof( animFileCondition_t ), sizeof( animFileEvent_t ) };
	for ( uint32_t i = 0; i < ANIM_SECTION_COUNT; ++i ) {
		const auto &s = h.sections[i];
		if ( s.offset < sizeof( h ) || s.offset > payloadSize || s.count > ( payloadSize - s.offset ) / sizes[i] )
			return false;
	}
	const uint32_t joints = Count( &result, ANIM_JOINTS );
	if ( !Name( h.model ) || !joints || joints > ANIM_MAX_JOINTS || Count( &result, ANIM_POSES ) != joints ||
		 Count( &result, ANIM_PARAMETERS ) > ANIM_MAX_PARAMETERS || !Count( &result, ANIM_STATES ) ||
		 Count( &result, ANIM_STATES ) > ANIM_MAX_STATES || h.initialState >= Count( &result, ANIM_STATES ) ||
		 !h.frameCount || h.frameChannels > joints * 10 || uint64_t( h.frameCount ) * h.frameChannels != Count( &result, ANIM_FRAMES ) )
		return false;
	uint32_t channel = 0;
	for ( uint32_t i = 0; i < joints; ++i ) {
		const auto joint = Read<animFileJoint_t>( &result, ANIM_JOINTS, i );
		const auto pose = Read<animFilePose_t>( &result, ANIM_POSES, i );
		if ( !Name( joint.name ) || joint.parent < -1 || joint.parent >= int32_t( i ) || joint.channelOffset != channel ||
			 !Finite( joint.bind.translate, 3 ) || !Finite( joint.bind.rotate, 4 ) || !Finite( joint.bind.scale, 3 ) ||
			 Dot( joint.bind.rotate, joint.bind.rotate, 4 ) < 1e-20f || pose.mask > 1023 || !Finite( pose.offset, 10 ) || !Finite( pose.scale, 10 ) )
			return false;
		for ( uint32_t c = 0; c < 10; ++c ) {
			if ( !isfinite( pose.offset[c] + pose.scale[c] * 65535.0f ) )
				return false;
			channel += ( pose.mask >> c ) & 1;
		}
	}
	if ( channel != h.frameChannels )
		return false;
	for ( uint32_t i = 0; i < Count( &result, ANIM_CLIPS ); ++i ) {
		const auto clip = Read<animFileClip_t>( &result, ANIM_CLIPS, i );
		if ( !Name( clip.name ) || !clip.frameCount || !clip.durationMs || clip.durationMs > 3600000 || !Range( clip.firstFrame, clip.frameCount, h.frameCount ) || clip.flags > ANIM_LOOP )
			return false;
	}
	for ( uint32_t i = 0; i < Count( &result, ANIM_PARAMETERS ); ++i ) {
		const auto parameter = Read<animFileParameter_t>( &result, ANIM_PARAMETERS, i );
		if ( !Name( parameter.name ) || !isfinite( parameter.value ) || !isfinite( parameter.minimum ) || !isfinite( parameter.maximum ) || parameter.value < parameter.minimum || parameter.value > parameter.maximum )
			return false;
	}
	for ( uint32_t i = 0; i < Count( &result, ANIM_STATES ); ++i ) {
		const auto state = Read<animFileState_t>( &result, ANIM_STATES, i );
		if ( !Name( state.name ) || state.clip >= Count( &result, ANIM_CLIPS ) || state.flags > ANIM_LOOP || !state.speedQ16 || state.speedQ16 > 16 * 65536 || !Range( state.firstEvent, state.eventCount, Count( &result, ANIM_EVENTS ) ) )
			return false;
		const auto clip = Read<animFileClip_t>( &result, ANIM_CLIPS, state.clip );
		uint32_t last = 0;
		for ( uint32_t e = 0; e < state.eventCount; ++e ) {
			const auto event = Read<animFileEvent_t>( &result, ANIM_EVENTS, state.firstEvent + e );
			if ( !Name( event.name ) || event.timeMs < last || event.timeMs > clip.durationMs || ( state.flags && event.timeMs == clip.durationMs ) || event.bone < -1 || event.bone >= int32_t( joints ) )
				return false;
			last = event.timeMs;
		}
	}
	for ( uint32_t i = 0; i < Count( &result, ANIM_TRANSITIONS ); ++i ) {
		const auto t = Read<animFileTransition_t>( &result, ANIM_TRANSITIONS, i );
		if ( ( t.from != ANIM_ANY_STATE && t.from >= Count( &result, ANIM_STATES ) ) || t.to >= Count( &result, ANIM_STATES ) || t.blendMs > 60000 || t.flags > ANIM_ON_END || !Range( t.firstCondition, t.conditionCount, Count( &result, ANIM_CONDITIONS ) ) )
			return false;
	}
	for ( uint32_t i = 0; i < Count( &result, ANIM_CONDITIONS ); ++i ) {
		const auto c = Read<animFileCondition_t>( &result, ANIM_CONDITIONS, i );
		if ( c.parameter >= Count( &result, ANIM_PARAMETERS ) || c.operation > 5 || !isfinite( c.value ) )
			return false;
	}
	*asset = result;
	return true;
}

template <typename T>
static int32_t Index( const animAsset_t *asset, animSectionIndex_t section, const char *name ) {
	if ( !name )
		return -1;
	for ( uint32_t i = 0; i < Count( asset, section ); ++i ) {
		const auto record = Read<T>( asset, section, i );
		if ( !strcmp( record.name, name ) )
			return int32_t( i );
	}
	return -1;
}
int32_t Anim_ParameterIndex( const animAsset_t *asset, const char *name ) {
	return Index<animFileParameter_t>( asset, ANIM_PARAMETERS, name );
}
int32_t Anim_BoneIndex( const animAsset_t *asset, const char *name ) {
	return Index<animFileJoint_t>( asset, ANIM_JOINTS, name );
}
int32_t Anim_ClipIndex( const animAsset_t *asset, const char *name ) {
	return Index<animFileClip_t>( asset, ANIM_CLIPS, name );
}
const char *Anim_StateName( const animAsset_t *asset, uint32_t state ) {
	return state < Count( asset, ANIM_STATES ) ? reinterpret_cast<const char *>( asset->data + asset->header.sections[ANIM_STATES].offset + sizeof( animFileState_t ) * state ) : "";
}
const char *Anim_EventName( const animAsset_t *asset, uint32_t event ) {
	return event < Count( asset, ANIM_EVENTS ) ? reinterpret_cast<const char *>( asset->data + asset->header.sections[ANIM_EVENTS].offset + sizeof( animFileEvent_t ) * event ) : "";
}
void Anim_DefaultParameters( const animAsset_t *asset, float *parameters ) {
	memset( parameters, 0, sizeof( float ) * ANIM_MAX_PARAMETERS );
	for ( uint32_t i = 0; i < Count( asset, ANIM_PARAMETERS ); ++i )
		parameters[i] = Read<animFileParameter_t>( asset, ANIM_PARAMETERS, i ).value;
}
void Anim_Reset( const animAsset_t *asset, uint32_t time, animState_t *state ) {
	*state = { asset->header.initialState, asset->header.initialState, time, time, time, 0, time, 0, 0 };
}
static uint64_t LocalTime( uint32_t time, uint32_t entered, uint32_t speed ) {
	return uint64_t( time - entered ) * speed / 65536;
}
static bool Parameters( const animAsset_t *asset, const float *parameters ) {
	for ( uint32_t i = 0; i < Count( asset, ANIM_PARAMETERS ); ++i ) {
		const auto parameter = Read<animFileParameter_t>( asset, ANIM_PARAMETERS, i );
		if ( !isfinite( parameters[i] ) || parameters[i] < parameter.minimum || parameters[i] > parameter.maximum )
			return false;
	}
	return true;
}
static bool Emit( const animAsset_t *asset, uint32_t id, uint32_t time, animState_t *state, animEvents_t *events ) {
	if ( events->count == ANIM_MAX_EVENTS )
		return false;
	const auto event = Read<animFileEvent_t>( asset, ANIM_EVENTS, id );
	events->items[events->count++] = { id, event.bone, time, ++state->eventSequence };
	return true;
}
// State/event output is committed together; exceeding the caller's bounded event
// buffer reports failure without advancing the simulation state.
bool Anim_Tick( const animAsset_t *asset, const float *parameters, uint32_t time, animState_t *state, animEvents_t *events ) {
	events->count = 0;
	if ( state->current >= Count( asset, ANIM_STATES ) || uint32_t( time - state->lastTime ) > INT32_MAX || !Parameters( asset, parameters ) )
		return false;
	if ( state->initialized && time == state->lastTime )
		return true;
	animState_t next = *state;
	animEvents_t pending = {};
	const auto current = Read<animFileState_t>( asset, ANIM_STATES, next.current );
	const auto clip = Read<animFileClip_t>( asset, ANIM_CLIPS, current.clip );
	if ( !next.initialized ) {
		for ( uint32_t e = 0; e < current.eventCount; ++e ) {
			if ( Read<animFileEvent_t>( asset, ANIM_EVENTS, current.firstEvent + e ).timeMs )
				break;
			if ( !Emit( asset, current.firstEvent + e, next.entered, &next, &pending ) )
				return false;
		}
		next.initialized = 1;
	}
	const uint64_t from = LocalTime( next.lastTime, next.entered, current.speedQ16 );
	const uint64_t to = LocalTime( time, next.entered, current.speedQ16 );
	if ( current.eventCount && to > from ) {
		const uint64_t firstLoop = current.flags ? from / clip.durationMs : 0;
		const uint64_t lastLoop = current.flags ? to / clip.durationMs : 0;
		if ( lastLoop - firstLoop > ANIM_MAX_EVENTS )
			return false;
		for ( uint64_t loop = firstLoop; loop <= lastLoop; ++loop ) {
			for ( uint32_t i = 0; i < current.eventCount; ++i ) {
				const auto event = Read<animFileEvent_t>( asset, ANIM_EVENTS, current.firstEvent + i );
				const uint64_t eventTime = loop * clip.durationMs + event.timeMs;
				if ( eventTime > from && eventTime <= to ) {
					const uint32_t tick = next.entered + uint32_t( ( eventTime * 65536 + current.speedQ16 - 1 ) / current.speedQ16 );
					if ( !Emit( asset, current.firstEvent + i, tick, &next, &pending ) )
						return false;
				}
			}
		}
	}
	for ( uint32_t i = 0; i < Count( asset, ANIM_TRANSITIONS ); ++i ) {
		const auto t = Read<animFileTransition_t>( asset, ANIM_TRANSITIONS, i );
		if ( ( t.from != ANIM_ANY_STATE && t.from != next.current ) || t.to == next.current || ( t.flags && to < clip.durationMs ) )
			continue;
		bool passes = true;
		for ( uint32_t c = 0; c < t.conditionCount && passes; ++c ) {
			const auto condition = Read<animFileCondition_t>( asset, ANIM_CONDITIONS, t.firstCondition + c );
			const float a = parameters[condition.parameter], b = condition.value;
			switch ( condition.operation ) {
			case 0:
				passes = a == b;
				break;
			case 1:
				passes = a != b;
				break;
			case 2:
				passes = a < b;
				break;
			case 3:
				passes = a <= b;
				break;
			case 4:
				passes = a > b;
				break;
			case 5:
				passes = a >= b;
				break;
			}
		}
		if ( !passes )
			continue;
		next.previous = next.current;
		next.previousEntered = next.entered;
		next.current = t.to;
		next.entered = next.blendStarted = time;
		next.blendDuration = t.blendMs;
		const auto entered = Read<animFileState_t>( asset, ANIM_STATES, next.current );
		for ( uint32_t e = 0; e < entered.eventCount; ++e ) {
			if ( Read<animFileEvent_t>( asset, ANIM_EVENTS, entered.firstEvent + e ).timeMs )
				break;
			if ( !Emit( asset, entered.firstEvent + e, time, &next, &pending ) )
				return false;
		}
		break;
	}
	next.lastTime = time;
	*state = next;
	*events = pending;
	return true;
}

static animTransform_t Decode( const animAsset_t *asset, uint32_t joint, uint32_t frame ) {
	const auto record = Read<animFileJoint_t>( asset, ANIM_JOINTS, joint );
	const auto pose = Read<animFilePose_t>( asset, ANIM_POSES, joint );
	float values[10];
	uint32_t channel = frame * asset->header.frameChannels + record.channelOffset;
	for ( uint32_t i = 0; i < 10; ++i ) {
		values[i] = pose.offset[i];
		if ( pose.mask & ( 1u << i ) )
			values[i] += float( Read<uint16_t>( asset, ANIM_FRAMES, channel++ ) ) * pose.scale[i];
	}
	animTransform_t result;
	memcpy( result.translate, values, sizeof( result.translate ) );
	memcpy( result.rotate, values + 3, sizeof( result.rotate ) );
	memcpy( result.scale, values + 7, sizeof( result.scale ) );
	if ( !Normalize( result.rotate, 4 ) )
		memcpy( result.rotate, identity.rotate, sizeof( result.rotate ) );
	return result;
}
static animTransform_t Sample( const animAsset_t *asset, uint32_t joint, const animFileClip_t *clip, uint64_t time, bool loop ) {
	time = loop ? time % clip->durationMs : ( time < clip->durationMs ? time : clip->durationMs );
	const uint64_t position = time * ( clip->frameCount - 1 );
	const uint32_t frame = uint32_t( position / clip->durationMs );
	const auto a = Decode( asset, joint, clip->firstFrame + frame );
	if ( frame + 1 == clip->frameCount )
		return a;
	const auto b = Decode( asset, joint, clip->firstFrame + frame + 1 );
	animTransform_t result;
	Blend( &a, &b, float( position % clip->durationMs ) / float( clip->durationMs ), &result );
	return result;
}
static void Matrix( const animTransform_t *transform, float *matrix ) {
	const float x = transform->rotate[0], y = transform->rotate[1], z = transform->rotate[2], w = transform->rotate[3];
	const float *s = transform->scale;
	matrix[0] = ( 1 - 2 * ( y * y + z * z ) ) * s[0];
	matrix[1] = 2 * ( x * y - z * w ) * s[1];
	matrix[2] = 2 * ( x * z + y * w ) * s[2];
	matrix[4] = 2 * ( x * y + z * w ) * s[0];
	matrix[5] = ( 1 - 2 * ( x * x + z * z ) ) * s[1];
	matrix[6] = 2 * ( y * z - x * w ) * s[2];
	matrix[8] = 2 * ( x * z - y * w ) * s[0];
	matrix[9] = 2 * ( y * z + x * w ) * s[1];
	matrix[10] = ( 1 - 2 * ( x * x + y * y ) ) * s[2];
	for ( uint32_t i = 0; i < 3; ++i )
		matrix[i * 4 + 3] = transform->translate[i];
}
bool Anim_Evaluate( const animAsset_t *asset, const animState_t *state, const float *parameters, uint32_t time, animPose_t *pose ) {
	if ( state->current >= Count( asset, ANIM_STATES ) || state->previous >= Count( asset, ANIM_STATES ) || uint32_t( time - state->entered ) > INT32_MAX || !Parameters( asset, parameters ) )
		return false;
	const auto current = Read<animFileState_t>( asset, ANIM_STATES, state->current );
	const auto previous = Read<animFileState_t>( asset, ANIM_STATES, state->previous );
	const auto clip = Read<animFileClip_t>( asset, ANIM_CLIPS, current.clip );
	const auto oldClip = Read<animFileClip_t>( asset, ANIM_CLIPS, previous.clip );
	const float weight = state->blendDuration ? Clamp( float( time - state->blendStarted ) / float( state->blendDuration ), 0, 1 ) : 1;
	pose->jointCount = Count( asset, ANIM_JOINTS );
	for ( uint32_t i = 0; i < pose->jointCount; ++i ) {
		const auto sampled = Sample( asset, i, &clip, LocalTime( time, state->entered, current.speedQ16 ), current.flags != 0 );
		if ( weight < 1 ) {
			const auto old = Sample( asset, i, &oldClip, LocalTime( time, state->previousEntered, previous.speedQ16 ), previous.flags != 0 );
			Blend( &old, &sampled, weight, &pose->local[i] );
		} else
			pose->local[i] = sampled;
		float local[12];
		Matrix( &pose->local[i], local );
		const auto joint = Read<animFileJoint_t>( asset, ANIM_JOINTS, i );
		if ( joint.parent < 0 )
			memcpy( pose->world[i], local, sizeof( local ) );
		else {
			const float *parent = pose->world[joint.parent];
			for ( uint32_t row = 0; row < 3; ++row )
				for ( uint32_t col = 0; col < 4; ++col )
					pose->world[i][row * 4 + col] = parent[row * 4] * local[col] + parent[row * 4 + 1] * local[4 + col] + parent[row * 4 + 2] * local[8 + col] + ( col == 3 ? parent[row * 4 + 3] : 0 );
		}
	}
	return true;
}
static animTransform_t ComposeRigid( const animTransform_t *a, const animTransform_t *b ) {
	float matrix[12];
	Matrix( a, matrix );
	animTransform_t out = identity;
	for ( uint32_t i = 0; i < 3; ++i )
		out.translate[i] = matrix[i * 4] * b->translate[0] + matrix[i * 4 + 1] * b->translate[1] + matrix[i * 4 + 2] * b->translate[2] + a->translate[i];
	MultiplyQuaternion( a->rotate, b->rotate, out.rotate );
	Normalize( out.rotate, 4 );
	return out;
}
static animTransform_t InverseRigid( const animTransform_t *value ) {
	animTransform_t out = identity;
	for ( uint32_t i = 0; i < 3; ++i )
		out.rotate[i] = -value->rotate[i];
	out.rotate[3] = value->rotate[3];
	float matrix[12];
	Matrix( &out, matrix );
	for ( uint32_t i = 0; i < 3; ++i )
		out.translate[i] = -( matrix[i * 4] * value->translate[0] + matrix[i * 4 + 1] * value->translate[1] + matrix[i * 4 + 2] * value->translate[2] );
	return out;
}
static animTransform_t RootAt( const animAsset_t *asset, const animFileClip_t *clip, uint32_t time, bool loop ) {
	animTransform_t sampled = Sample( asset, 0, clip, time, loop );
	memcpy( sampled.scale, identity.scale, sizeof( sampled.scale ) );
	uint32_t loops = loop ? time / clip->durationMs : 0;
	if ( !loops )
		return sampled;
	animTransform_t start = Sample( asset, 0, clip, 0, false );
	animTransform_t end = Sample( asset, 0, clip, clip->durationMs, false );
	memcpy( start.scale, identity.scale, sizeof( start.scale ) );
	memcpy( end.scale, identity.scale, sizeof( end.scale ) );
	const auto inverse = InverseRigid( &start );
	animTransform_t cycle = ComposeRigid( &inverse, &end ), accumulated = identity;
	// Exponentiation preserves ordered rigid composition without a loop per cycle.
	for ( ; loops; loops >>= 1 ) {
		if ( loops & 1 )
			accumulated = ComposeRigid( &accumulated, &cycle );
		cycle = ComposeRigid( &cycle, &cycle );
	}
	const auto origin = ComposeRigid( &start, &accumulated );
	const auto relative = ComposeRigid( &inverse, &sampled );
	return ComposeRigid( &origin, &relative );
}
bool Anim_RootMotion( const animAsset_t *asset, int32_t clipIndex, uint32_t from, uint32_t to, bool loop, animTransform_t *motion ) {
	if ( clipIndex < 0 || uint32_t( clipIndex ) >= Count( asset, ANIM_CLIPS ) || to < from )
		return false;
	const auto clip = Read<animFileClip_t>( asset, ANIM_CLIPS, uint32_t( clipIndex ) );
	const auto a = RootAt( asset, &clip, from, loop );
	const auto b = RootAt( asset, &clip, to, loop );
	const auto inverse = InverseRigid( &a );
	*motion = ComposeRigid( &inverse, &b );
	return true;
}
void Anim_BlendTransforms( uint32_t count, const animTransform_t *a, const animTransform_t *b, const float *mask, float weight, animTransform_t *out ) {
	for ( uint32_t i = 0; i < count; ++i )
		Blend( &a[i], &b[i], Clamp( weight * ( mask ? mask[i] : 1 ), 0, 1 ), &out[i] );
}
void Anim_AdditiveTransforms( uint32_t count, const animTransform_t *base, const animTransform_t *delta, const animTransform_t *reference, const float *mask, float weight, animTransform_t *out ) {
	for ( uint32_t i = 0; i < count; ++i ) {
		const float w = Clamp( weight * ( mask ? mask[i] : 1 ), 0, 1 );
		animTransform_t relative = identity, weighted;
		for ( uint32_t c = 0; c < 3; ++c ) {
			relative.translate[c] = delta[i].translate[c] - reference[i].translate[c];
			relative.scale[c] = reference[i].scale[c] != 0 ? delta[i].scale[c] / reference[i].scale[c] : 1;
		}
		const float inverse[4] = { -reference[i].rotate[0], -reference[i].rotate[1], -reference[i].rotate[2], reference[i].rotate[3] };
		MultiplyQuaternion( delta[i].rotate, inverse, relative.rotate );
		Blend( &identity, &relative, w, &weighted );
		for ( uint32_t c = 0; c < 3; ++c ) {
			out[i].translate[c] = base[i].translate[c] + weighted.translate[c];
			out[i].scale[c] = base[i].scale[c] * weighted.scale[c];
		}
		MultiplyQuaternion( weighted.rotate, base[i].rotate, out[i].rotate );
		Normalize( out[i].rotate, 4 );
	}
}
static void Subtract( const float *a, const float *b, float *out ) {
	for ( uint32_t i = 0; i < 3; ++i )
		out[i] = a[i] - b[i];
}
static void Cross( const float *a, const float *b, float *out ) {
	const float v[3] = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
	memcpy( out, v, sizeof( v ) );
}
bool Anim_TwoBoneIK( const float *root, const float *middle, const float *end, const float *target, const float *pole, float *outMiddle, float *outEnd ) {
	float a[3], b[3], direction[3], bend[3];
	Subtract( middle, root, a );
	Subtract( end, middle, b );
	Subtract( target, root, direction );
	Subtract( pole, root, bend );
	const float first = sqrtf( Dot( a, a, 3 ) ), second = sqrtf( Dot( b, b, 3 ) );
	const float distance = sqrtf( Dot( direction, direction, 3 ) );
	if ( !isfinite( first ) || !isfinite( second ) || first < 1e-6f || second < 1e-6f || !Normalize( direction, 3 ) || !Finite( bend, 3 ) )
		return false;
	const float reach = Clamp( distance, fmaxf( 1e-6f, fabsf( first - second ) ), first + second );
	const float along = ( first * first + reach * reach - second * second ) / ( 2 * reach );
	const float height = sqrtf( fmaxf( 0, first * first - along * along ) );
	const float projection = Dot( bend, direction, 3 );
	for ( uint32_t i = 0; i < 3; ++i )
		bend[i] -= projection * direction[i];
	if ( !Normalize( bend, 3 ) ) {
		const float axis[3] = { fabsf( direction[0] ) < 0.8f ? 1.0f : 0.0f, fabsf( direction[0] ) < 0.8f ? 0.0f : 1.0f, 0 };
		Cross( direction, axis, bend );
		if ( !Normalize( bend, 3 ) )
			return false;
	}
	for ( uint32_t i = 0; i < 3; ++i ) {
		outMiddle[i] = root[i] + direction[i] * along + bend[i] * height;
		outEnd[i] = root[i] + direction[i] * reach;
	}
	return true;
}
bool Anim_LookAt( const float *from, const float *to, float *rotation ) {
	float a[3], b[3];
	memcpy( a, from, sizeof( a ) );
	memcpy( b, to, sizeof( b ) );
	if ( !Normalize( a, 3 ) || !Normalize( b, 3 ) )
		return false;
	Cross( a, b, rotation );
	rotation[3] = 1 + Clamp( Dot( a, b, 3 ), -1, 1 );
	if ( !Normalize( rotation, 4 ) ) {
		const float axis[3] = { fabsf( a[0] ) < 0.8f ? 1.0f : 0.0f, fabsf( a[0] ) < 0.8f ? 0.0f : 1.0f, 0 };
		Cross( a, axis, rotation );
		rotation[3] = 0;
		return Normalize( rotation, 4 );
	}
	return true;
}
