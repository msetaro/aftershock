#include "../../game/ui/ui_spskill.cpp"
#include <assert.h>

vec4_t color_red = { 1, 0, 0, 1 };
vec4_t color_white = { 1, 1, 1, 1 };
static volatile float currentSkill;
static int writes;
static char scoreName[32];
static char scoreValue[MAX_INFO_VALUE];

float trap_Cvar_VariableValue( const char *name ) {
	assert( !strcmp( name, "g_spSkill" ) );
	return currentSkill;
}

void trap_Cvar_SetValue( const char *name, float value ) {
	assert( !strcmp( name, "g_spSkill" ) );
	currentSkill = value;
}

void trap_Cvar_VariableStringBuffer( const char *name, char *buffer, int size ) {
	assert( size > 0 );
	assert( !strncmp( name, "g_spScores", 10 ) );
	buffer[0] = '\0';
}

void trap_Cvar_Set( const char *name, const char *value ) {
	++writes;
	Q_strncpyz( scoreName, name, sizeof( scoreName ) );
	Q_strncpyz( scoreValue, value, sizeof( scoreValue ) );
}

void trap_S_StartLocalSound( sfxHandle_t, int ) {}
void QDECL Com_Error( int, const char *, ... ) { abort(); }
void QDECL Com_Printf( const char *, ... ) {}

int main( int argc, char **argv ) {
	const float values[] = { 1e38f, -1e38f, -2147483648.0f, 2147483648.0f,
		-1, 0, 0.9f, 1, 1.9f, 2, 3, 4, 5, 5.9f, 6 };
	assert( argc == 2 );
	for ( unsigned i = 0; i < sizeof( values ) / sizeof( values[0] ); ++i ) {
		currentSkill = values[i];
		if ( !strcmp( argv[1], "event" ) ) {
			for ( int skill = 1; skill <= 5; ++skill ) {
				menucommon_s item = {};
				item.id = ID_BABY + skill - 1;
				currentSkill = values[i];
				skillMenuInfo.skillpics[skill - 1] = 100 + skill;
				UI_SPSkillMenu_SkillEvent( &item, QM_ACTIVATED );
				assert( currentSkill == skill );
				assert( skillMenuInfo.art_skillPic.shader == 100 + skill );
			}
		} else {
			writes = 0;
			UI_SetBestScore( 7, 1 );
			if ( values[i] >= 1 && values[i] < 6 ) {
				char expected[32];
				snprintf( expected, sizeof( expected ), "g_spScores%d", (int)values[i] );
				assert( writes == 1 && !strcmp( scoreName, expected ) );
				assert( !strcmp( Info_ValueForKey( scoreValue, "l7" ), "1" ) );
			} else {
				assert( writes == 0 );
			}
		}
	}
	return 0;
}
