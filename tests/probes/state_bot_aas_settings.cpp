#include "../../engine/botlib/be_aas_move.cpp"
#include <assert.h>
int main() {
	aassettings.phys_gravity = 800;
	aassettings.phys_maxvelocity = 320;
	aassettings.phys_gravitydirection[2] = -1;
	aassettings.phys_friction = 6;
	aassettings.rs_waterjump = 20;
	vec3_t start{ 0, 0, 24 }, end{ 64, 24, 32 };
	float expected, actual;
	const int result = AAS_HorizontalVelocityForJump( 270, start, end, &expected );
	static unsigned char bytes[8192];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(AAS_WriteSettingsState(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	aassettings = {};
	assert(AAS_ReadSettingsState(reader,false) && aassettings.phys_gravity==0);
	assert(AAS_ReadSettingsState(reader,true) && aassettings.rs_waterjump==20 && aassettings.phys_friction==6);
	assert(AAS_HorizontalVelocityForJump(270,start,end,&actual)==result && !memcmp(&expected,&actual,sizeof(actual)));
	aas_settings_t saved = aassettings;
	saved.phys_gravity = std::numeric_limits<float>::infinity();
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,aasSettingsSchema,0,&saved));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(!AAS_ReadSettingsState(reader,true) && aassettings.phys_gravity==800);
	puts( "PASS: named AAS physics settings preserve bit-identical jump reachability after restore" );
}
