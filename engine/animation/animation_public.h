#ifndef ANIMATION_PUBLIC_H
#define ANIMATION_PUBLIC_H

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

constexpr uint32_t ANIM_MAX_JOINTS = 128;
constexpr uint32_t ANIM_MAX_PARAMETERS = 16;
constexpr uint32_t ANIM_MAX_STATES = 64;
constexpr uint32_t ANIM_MAX_EVENTS = 64;
constexpr uint32_t ANIM_LOOP = 1;
constexpr uint32_t ANIM_ON_END = 1;
constexpr uint32_t ANIM_ANY_STATE = UINT32_MAX;

struct animTransform_t {
	float translate[3], rotate[4], scale[3];
};
enum animSectionIndex_t : uint32_t {
	ANIM_JOINTS,
	ANIM_POSES,
	ANIM_FRAMES,
	ANIM_CLIPS,
	ANIM_PARAMETERS,
	ANIM_STATES,
	ANIM_TRANSITIONS,
	ANIM_CONDITIONS,
	ANIM_EVENTS,
	ANIM_SECTION_COUNT
};
struct animSection_t {
	uint32_t count, offset;
};
// Offsets are relative to the payload following the common 48-byte envelope.
struct animFileHeader_t {
	char model[64];
	uint8_t modelHash[32];
	uint32_t frameCount, frameChannels, initialState;
	animSection_t sections[ANIM_SECTION_COUNT];
};
struct animFileJoint_t {
	char name[64];
	int32_t parent;
	uint32_t channelOffset;
	animTransform_t bind;
};
struct animFilePose_t {
	uint32_t mask;
	float offset[10], scale[10];
};
struct animFileClip_t {
	char name[64];
	uint32_t firstFrame, frameCount, durationMs, flags;
};
struct animFileParameter_t {
	char name[64];
	float value, minimum, maximum;
};
struct animFileState_t {
	char name[64];
	uint32_t clip, flags, speedQ16, firstEvent, eventCount;
};
struct animFileTransition_t {
	uint32_t from, to, blendMs, firstCondition, conditionCount, flags;
};
struct animFileCondition_t {
	uint32_t parameter, operation;
	float value;
};
struct animFileEvent_t {
	char name[64];
	uint32_t timeMs;
	int32_t bone;
};
static_assert( sizeof( animTransform_t ) == 40 && offsetof( animTransform_t, scale ) == 28 );
static_assert( sizeof( animSection_t ) == 8 && sizeof( animFileHeader_t ) == 180 );
static_assert( offsetof( animFileHeader_t, sections ) == 108 );
static_assert( sizeof( animFileJoint_t ) == 112 && sizeof( animFilePose_t ) == 84 );
static_assert( sizeof( animFileClip_t ) == 80 && sizeof( animFileParameter_t ) == 76 );
static_assert( sizeof( animFileState_t ) == 84 && sizeof( animFileTransition_t ) == 24 );
static_assert( sizeof( animFileCondition_t ) == 12 && sizeof( animFileEvent_t ) == 72 );
static_assert( std::is_trivially_copyable_v<animFileHeader_t> && std::is_trivially_copyable_v<animFileJoint_t> );
static_assert( std::is_trivially_copyable_v<animFilePose_t> && std::is_trivially_copyable_v<animFileClip_t> );
static_assert( std::is_trivially_copyable_v<animFileParameter_t> && std::is_trivially_copyable_v<animFileState_t> );
static_assert( std::is_trivially_copyable_v<animFileTransition_t> && std::is_trivially_copyable_v<animFileCondition_t> );
static_assert( std::is_trivially_copyable_v<animFileEvent_t> );

// Non-owning view: the caller retains the immutable cooked bytes for its lifetime.
struct animAsset_t {
	const uint8_t *data;
	size_t size;
	animFileHeader_t header;
};
struct animState_t {
	uint32_t current, previous, entered, previousEntered;
	uint32_t blendStarted, blendDuration, lastTime, eventSequence;
	uint32_t initialized;
};
struct animPose_t {
	uint32_t jointCount;
	animTransform_t local[ANIM_MAX_JOINTS];
	float world[ANIM_MAX_JOINTS][12];
};
struct animEvent_t {
	uint32_t id;
	int32_t bone;
	uint32_t time, sequence;
};
struct animEvents_t {
	uint32_t count;
	animEvent_t items[ANIM_MAX_EVENTS];
};
static_assert( sizeof( animState_t ) == 36 && std::is_trivially_copyable_v<animState_t> );
static_assert( sizeof( animEvent_t ) == 16 && std::is_trivially_copyable_v<animEvent_t> );

bool Anim_Open( const void *bytes, size_t size, animAsset_t *asset );
int32_t Anim_ParameterIndex( const animAsset_t *asset, const char *name );
int32_t Anim_BoneIndex( const animAsset_t *asset, const char *name );
int32_t Anim_ClipIndex( const animAsset_t *asset, const char *name );
const char *Anim_StateName( const animAsset_t *asset, uint32_t state );
const char *Anim_EventName( const animAsset_t *asset, uint32_t event );
void Anim_DefaultParameters( const animAsset_t *asset, float *parameters );
void Anim_Reset( const animAsset_t *asset, uint32_t time, animState_t *state );
bool Anim_Tick( const animAsset_t *asset, const float *parameters, uint32_t time, animState_t *state, animEvents_t *events );
bool Anim_Evaluate( const animAsset_t *asset, const animState_t *state, const float *parameters, uint32_t time, animPose_t *pose );
// Rigid delta in the root frame at `from`; loop turns compose in order.
bool Anim_RootMotion( const animAsset_t *asset, int32_t clip, uint32_t from, uint32_t to, bool loop, animTransform_t *motion );
void Anim_BlendTransforms( uint32_t count, const animTransform_t *a, const animTransform_t *b, const float *mask, float weight, animTransform_t *out );
void Anim_AdditiveTransforms( uint32_t count, const animTransform_t *base, const animTransform_t *delta, const animTransform_t *reference, const float *mask, float weight, animTransform_t *out );
bool Anim_TwoBoneIK( const float *root, const float *middle, const float *end, const float *target, const float *pole, float *outMiddle, float *outEnd );
bool Anim_LookAt( const float *from, const float *to, float *rotation );

#endif
