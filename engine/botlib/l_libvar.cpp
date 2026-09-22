/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		l_libvar.c
 *
 * desc:		bot library variables
 *
 * $Archive: /MissionPack/code/botlib/l_libvar.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "botlib_public.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "be_interface.h"
#include <cmath>

//list with library variables
libvar_t *libvarlist = NULL;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static float LibVarStringValue( const char *string ) {
	int dotfound = 0;
	float value = 0;

	while ( *string ) {
		if ( *string < '0' || *string > '9' ) {
			if ( dotfound || *string != '.' ) {
				return 0;
			} //end if
			else {
				dotfound = 10;
				string++;
			} //end if
		} //end if
		if ( dotfound ) {
			value = value + (float)( *string - '0' ) / (float)dotfound;
			dotfound *= 10;
		} //end if
		else {
			value = (float)( value * 10.0 + (float)( *string - '0' ) );
		} //end else
		string++;
	} //end while
	return value;
} //end of the function LibVarStringValue
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
libvar_t *LibVarAlloc( const char *var_name ) {
	libvar_t *v;

	v = (libvar_t *)GetMemory( sizeof( libvar_t ) );
	Com_Memset( v, 0, sizeof( libvar_t ) );
	v->name = (char *)GetMemory( strlen( var_name )+1 );
	strcpy( v->name, var_name );
	//add the variable in the list
	v->next = libvarlist;
	libvarlist = v;
	return v;
} //end of the function LibVarAlloc
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarDeAlloc( libvar_t *v ) {
	if ( v->string )
		FreeMemory( v->string );
	FreeMemory( v->name );
	FreeMemory( v );
} //end of the function LibVarDeAlloc
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarDeAllocAll( void ) {
	libvar_t *v;

	for ( v = libvarlist; v; v = libvarlist ) {
		libvarlist = libvarlist->next;
		LibVarDeAlloc( v );
	} //end for
	libvarlist = NULL;
} //end of the function LibVarDeAllocAll
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
libvar_t *LibVarGet( const char *var_name ) {
	libvar_t *v;

	for ( v = libvarlist; v; v = v->next ) {
		if ( !Q_stricmp( v->name, var_name ) ) {
			return v;
		} //end if
	} //end for
	return NULL;
} //end of the function LibVarGet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
const char *LibVarGetString( const char *var_name ) {
	libvar_t *v;

	v = LibVarGet( var_name );
	if ( v ) {
		return v->string;
	} //end if
	else {
		return "";
	} //end else
} //end of the function LibVarGetString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float LibVarGetValue( const char *var_name ) {
	libvar_t *v;

	v = LibVarGet( var_name );
	if ( v ) {
		return v->value;
	} //end if
	else {
		return 0;
	} //end else
} //end of the function LibVarGetValue
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
libvar_t *LibVar( const char *var_name, const char *value ) {
	libvar_t *v;
	v = LibVarGet( var_name );
	if ( v )
		return v;
	//create new variable
	v = LibVarAlloc( var_name );
	//variable string
	v->string = (char *)GetMemory( strlen( value ) + 1 );
	strcpy( v->string, value );
	//the value
	v->value = LibVarStringValue( v->string );
	//variable is modified
	v->modified = qtrue;
	//
	return v;
} //end of the function LibVar
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
const char *LibVarString( const char *var_name, const char *value ) {
	libvar_t *v;

	v = LibVar( var_name, value );
	return v->string;
} //end of the function LibVarString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float LibVarValue( const char *var_name, const char *value ) {
	libvar_t *v;

	v = LibVar( var_name, value );
	return v->value;
} //end of the function LibVarValue

int LibVarInteger( const char *var_name, const char *value, int min_v, int max_v ) {
	int v = (int)LibVarValue( var_name, value );

	// if less than minimum, reset to default value
	// if more than maximum, set to maximum
	if ( v < min_v || v > max_v ) {
		botimport.Print( PRT_ERROR, "%s = %d\n", var_name, v );
		if ( v < min_v ) {
			v = atoi( value );
			LibVarSet( var_name, value );
		} else {
			v = max_v;
			LibVarSet( var_name, va( "%d", max_v ) );
		}
	}

	return v;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarSet( const char *var_name, const char *value ) {
	libvar_t *v;

	v = LibVarGet( var_name );
	if ( v ) {
		FreeMemory( v->string );
	} //end if
	else {
		v = LibVarAlloc( var_name );
	} //end else
	//variable string
	v->string = (char *)GetMemory( strlen( value ) + 1 );
	strcpy( v->string, value );
	//the value
	v->value = LibVarStringValue( v->string );
	//variable is modified
	v->modified = qtrue;
} //end of the function LibVarSet
#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean LibVarChanged( const char *var_name )
{
	libvar_t *v;

	v = LibVarGet( var_name );
	if ( v )
	{
		return v->modified;
	} //end if
	else
	{
		return qfalse;
	} //end else
} //end of the function LibVarChanged
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void LibVarSetNotModified( const char *var_name )
{
	libvar_t *v;

	v = LibVarGet( var_name );
	if ( v )
	{
		v->modified = qfalse;
	} //end if
} //end of the function LibVarSetNotModified
#endif

// ponytail: checkpoints support 256 named bot variables with existing engine
// string limits; reject excess rather than silently omitting a setting.
static constexpr uint32_t MAX_SAVED_LIBVARS = 256;
struct libvarSave_t {
	char name[MAX_QPATH], string[MAX_STRING_CHARS];
	int32_t flags;
	uint32_t modified;
	float value;
};
static constexpr stateField_t libvarFields[] = {
	{ "name", offsetof( libvarSave_t, name ), MAX_QPATH, stateType_t::String },
	{ "string", offsetof( libvarSave_t, string ), MAX_STRING_CHARS, stateType_t::String },
	{ "flags", offsetof( libvarSave_t, flags ), 1, stateType_t::Int32 },
	{ "modified", offsetof( libvarSave_t, modified ), 1, stateType_t::UInt32 },
	{ "value", offsetof( libvarSave_t, value ), 1, stateType_t::Float32 }
};
static constexpr stateSchema_t libvarSchema = { "botlib.variable", 1, 1, sizeof( libvarSave_t ), libvarFields, 5 };
static constexpr stateField_t libvarCountField = { "count", 0, 1, stateType_t::UInt32 };
static constexpr stateSchema_t libvarCountSchema = { "botlib.variables", 1, 1, sizeof( uint32_t ), &libvarCountField, 1 };
static libvarSave_t savedLibVars[MAX_SAVED_LIBVARS];
static bool ValidSavedLibVars( uint32_t count ) {
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto &var = savedLibVars[i];
		if ( !var.name[0] || var.modified > 1 || !std::isfinite( var.value ) )
			return false;
		for ( uint32_t j = 0; j < i; ++j )
			if ( !Q_stricmp( var.name, savedLibVars[j].name ) )
				return false;
	}
	return true;
}
bool LibVar_WriteState( stateWriter_t *writer ) {
	if ( !writer )
		return false;
	uint32_t count = 0;
	for ( auto *var = libvarlist; var; var = var->next ) {
		if ( count == MAX_SAVED_LIBVARS || !var->name || !var->string || strlen( var->name ) >= MAX_QPATH || strlen( var->string ) >= MAX_STRING_CHARS ) {
			writer->failed = true;
			return false;
		}
		auto &saved = savedLibVars[count++];
		saved = {};
		strcpy( saved.name, var->name );
		strcpy( saved.string, var->string );
		saved.flags = var->flags;
		saved.modified = uint32_t( var->modified );
		saved.value = var->value;
	}
	if ( !ValidSavedLibVars( count ) ) {
		writer->failed = true;
		return false;
	}
	if ( !State_Append( writer, libvarCountSchema, 0, &count ) )
		return false;
	for ( uint32_t i = 0; i < count; ++i )
		if ( !State_Append( writer, libvarSchema, i, &savedLibVars[i] ) )
			return false;
	return true;
}
bool LibVar_ReadState( const stateReader_t &reader, bool apply ) {
	uint32_t count, version;
	if ( !State_Find( reader, libvarCountSchema, 0, &count, &version ) || count > MAX_SAVED_LIBVARS )
		return false;
	for ( uint32_t i = 0; i < count; ++i )
		if ( !State_Find( reader, libvarSchema, i, &savedLibVars[i], &version ) )
			return false;
	if ( !ValidSavedLibVars( count ) )
		return false;
	libvar_t *loaded[MAX_SAVED_LIBVARS]{};
	uint32_t existing = 0;
	for ( auto *var = libvarlist; var; var = var->next ) {
		if ( ++existing > count || !var->name )
			return false;
		uint32_t i = 0;
		for ( ; i < count; ++i )
			if ( !Q_stricmp( var->name, savedLibVars[i].name ) )
				break;
		// Preserve cached libvar_t pointers. An unexpected initialized variable needs
		// a migration decision, not deletion behind an owner's retained pointer.
		if ( i == count || loaded[i] )
			return false;
		loaded[i] = var;
	}
	if ( apply ) {
		for ( uint32_t i = 0; i < count; ++i ) {
			auto *var = loaded[i];
			if ( !var )
				var = loaded[i] = LibVarAlloc( savedLibVars[i].name );
			char *string = static_cast<char *>( GetMemory(strlen(savedLibVars[i].string)+1));
			strcpy( string, savedLibVars[i].string );
			if ( var->string )
				FreeMemory( var->string );
			var->string = string;
			var->flags = savedLibVars[i].flags;
			var->modified = static_cast<qboolean>( savedLibVars[i].modified );
			var->value = savedLibVars[i].value;
		}
		for ( uint32_t i = 0; i < count; ++i )
			loaded[i]->next = i + 1 < count ? loaded[i + 1] : nullptr;
		libvarlist = count ? loaded[0] : nullptr;
	}
	return true;
}
