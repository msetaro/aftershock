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
// Platform clock, processor and affinity services.

#include "runtime_public.h"
#include "../qcommon/qcommon_public.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/*
================
Com_RealTime
================
*/
int Com_RealTime(qtime_t *qtime) {
	time_t t;
	struct tm *tms;

	t = time(NULL);
	if (!qtime)
		return t;
	tms = localtime(&t);
	if (tms) {
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}


/*
================
Sys_Microseconds
================
*/
int64_t Sys_Microseconds( void )
{
#ifdef _WIN32
	static qboolean inited = qfalse;
	static LARGE_INTEGER base;
	static LARGE_INTEGER freq;
	LARGE_INTEGER curr;

	if ( !inited )
	{
		QueryPerformanceFrequency( &freq );
		QueryPerformanceCounter( &base );
		if ( !freq.QuadPart )
		{
			return (int64_t)Sys_Milliseconds() * 1000LL; // fallback
		}
		inited = qtrue;
		return 0;
	}

	QueryPerformanceCounter( &curr );

	return ((curr.QuadPart - base.QuadPart) * 1000000LL) / freq.QuadPart;
#else
	struct timeval curr;
	gettimeofday( &curr, NULL );

	return (int64_t)curr.tv_sec * 1000000LL + (int64_t)curr.tv_usec;
#endif
}


/*
** --------------------------------------------------------------------------------
**
** PROCESSOR STUFF
**
** --------------------------------------------------------------------------------
*/

#ifdef USE_AFFINITY_MASK
static uint64_t eCoreMask;
static uint64_t pCoreMask;
static uint64_t affinityMask; // saved at startup
#endif

#if (idx64 || id386)

#if defined _MSC_VER
#include <intrin.h>
static void CPUID( int func, unsigned int *regs )
{
	__cpuid( (int*)regs, func );
}

#ifdef USE_AFFINITY_MASK
#if idx64
Q_EXTERN_C void CPUID_EX( int func, int param, unsigned int *regs );
#else
Q_EXTERN_C void CPUID_EX( int func, int param, unsigned int *regs )
{
	__asm {
		push edi
		mov eax, func
		mov ecx, param
		cpuid
		mov edi, regs
		mov [edi +0], eax
		mov [edi +4], ebx
		mov [edi +8], ecx
		mov [edi+12], edx
		pop edi
	}
}
#endif // !idx64
#endif // USE_AFFINITY_MASK

#else // clang/gcc/mingw

static void CPUID( int func, unsigned int *regs )
{
	__asm__ __volatile__( "cpuid" :
		"=a"(regs[0]),
		"=b"(regs[1]),
		"=c"(regs[2]),
		"=d"(regs[3]) :
		"a"(func) );
}

#ifdef USE_AFFINITY_MASK
static void CPUID_EX( int func, int param, unsigned int *regs )
{
	__asm__ __volatile__( "cpuid" :
		"=a"(regs[0]),
		"=b"(regs[1]),
		"=c"(regs[2]),
		"=d"(regs[3]) :
		"a"(func),
		"c"(param) );
}
#endif // USE_AFFINITY_MASK

#endif  // clang/gcc/mingw

void Sys_GetProcessorId( char *vendor )
{
	uint32_t regs[4]; // EAX, EBX, ECX, EDX
	uint32_t cpuid_level_ex;
	char vendor_str[12 + 1]; // short CPU vendor string

	// setup initial features
#if idx64
	CPU_Flags |= CPU_SSE | CPU_SSE2 | CPU_FCOM;
#else
	CPU_Flags = 0;
#endif
	vendor[0] = '\0';

	CPUID( 0x80000000, regs );
	cpuid_level_ex = regs[0];

	// get CPUID level & short CPU vendor string
	CPUID( 0x0, regs );
	memcpy(vendor_str + 0, (char*)&regs[1], 4);
	memcpy(vendor_str + 4, (char*)&regs[3], 4);
	memcpy(vendor_str + 8, (char*)&regs[2], 4);
	vendor_str[12] = '\0';

	// get CPU feature bits
	CPUID( 0x1, regs );

	// bit 15 of EDX denotes CMOV/FCMOV/FCOMI existence
	if ( regs[3] & ( 1 << 15 ) )
		CPU_Flags |= CPU_FCOM;

	// bit 23 of EDX denotes MMX existence
	if ( regs[3] & ( 1 << 23 ) )
		CPU_Flags |= CPU_MMX;

	// bit 25 of EDX denotes SSE existence
	if ( regs[3] & ( 1 << 25 ) )
		CPU_Flags |= CPU_SSE;

	// bit 26 of EDX denotes SSE2 existence
	if ( regs[3] & ( 1 << 26 ) )
		CPU_Flags |= CPU_SSE2;

	// bit 0 of ECX denotes SSE3 existence
	//if ( regs[2] & ( 1 << 0 ) )
	//	CPU_Flags |= CPU_SSE3;

	// bit 19 of ECX denotes SSE41 existence
	if ( regs[ 2 ] & ( 1 << 19 ) )
		CPU_Flags |= CPU_SSE41;

	if ( vendor ) {
		if ( cpuid_level_ex >= 0x80000004 ) {
			// read CPU Brand string
			uint32_t i;
			for ( i = 0x80000002; i <= 0x80000004; i++) {
				CPUID( i, regs );
				memcpy( vendor+0, (char*)&regs[0], 4 );
				memcpy( vendor+4, (char*)&regs[1], 4 );
				memcpy( vendor+8, (char*)&regs[2], 4 );
				memcpy( vendor+12, (char*)&regs[3], 4 );
				vendor[16] = '\0';
				vendor += strlen( vendor );
			}
		} else {
			const int print_flags = CPU_Flags;
			vendor = Q_stradd( vendor, vendor_str );
			if (print_flags) {
				// print features
				strcat(vendor, " w/");
				if (print_flags & CPU_FCOM)
					strcat(vendor, " CMOV");
				if (print_flags & CPU_MMX)
					strcat(vendor, " MMX");
				if (print_flags & CPU_SSE)
					strcat(vendor, " SSE");
				if (print_flags & CPU_SSE2)
					strcat(vendor, " SSE2");
				//if ( CPU_Flags & CPU_SSE3 )
				//	strcat( vendor, " SSE3" );
				if (print_flags & CPU_SSE41)
					strcat(vendor, " SSE4.1");
			}
		}
	}
}


#ifdef USE_AFFINITY_MASK
static void DetectCPUCoresConfig( void )
{
	uint32_t regs[4];
	uint32_t i;

	// get highest function parameter and vendor id
	CPUID( 0x0, regs );
	if ( regs[1] != 0x756E6547 || regs[2] != 0x6C65746E || regs[3] != 0x49656E69 || regs[0] < 0x1A ) {
		// non-intel signature or too low cpuid level - unsupported
		eCoreMask = pCoreMask = affinityMask;
		return;
	}

	eCoreMask = 0;
	pCoreMask = 0;

	for ( i = 0; i < sizeof( affinityMask ) * 8; i++ ) {
		const uint64_t mask = 1ULL << i;
		if ( (mask & affinityMask) && Sys_SetAffinityMask( mask ) ) {
			CPUID_EX( 0x1A, 0x0, regs );
			switch ( (regs[0] >> 24) & 0xFF ) {
				case 0x20: eCoreMask |= mask; break;
				case 0x40: pCoreMask |= mask; break;
				default: // non-existing leaf
					eCoreMask = pCoreMask = 0;
					break;
			}
		}
	}

	// restore original affinity
	Sys_SetAffinityMask( affinityMask );

	if ( pCoreMask == 0 || eCoreMask == 0 ) {
		// if either mask is empty - assume non-hybrid configuration
		eCoreMask = pCoreMask = affinityMask;
	}
}
#endif // USE_AFFINITY_MASK

#else // non-x86

#ifndef __linux__

void Sys_GetProcessorId( char *vendor )
{
	Com_sprintf( vendor, 100, "%s", ARCH_STRING );
#ifdef _WIN32
	CPU_Flags |= CPU_ARMv7 | CPU_IDIVA | CPU_VFPv3;
#endif
}

#else // __linux__

#include <sys/auxv.h>

#if arm32
#include <asm/hwcap.h>
#endif

void Sys_GetProcessorId( char *vendor )
{
#if arm32
	const char *platform;
	long hwcaps;
	CPU_Flags = 0;

	platform = (const char*)getauxval( AT_PLATFORM );

	if ( !platform || *platform == '\0' ) {
		platform = "(unknown)";
	}

	if ( platform[0] == 'v' || platform[0] == 'V' ) {
		if ( atoi( platform + 1 ) >= 7 ) {
			CPU_Flags |= CPU_ARMv7;
		}
	}

	Com_sprintf( vendor, 100, "ARM %s", platform );
	hwcaps = getauxval( AT_HWCAP );
	if ( hwcaps & ( HWCAP_IDIVA | HWCAP_VFPv3 ) ) {
		strcat( vendor, " /w" );

		if ( hwcaps & HWCAP_IDIVA ) {
			CPU_Flags |= CPU_IDIVA;
			strcat( vendor, " IDIVA" );
		}

		if ( hwcaps & HWCAP_VFPv3 ) {
			CPU_Flags |= CPU_VFPv3;
			strcat( vendor, " VFPv3" );
		}
	}
#else // !arm32
	CPU_Flags = 0;
#if arm64
	Com_sprintf( vendor, 100, "%s", ARCH_STRING );
#else
	Com_sprintf( vendor, 128, "%s %s", ARCH_STRING, (const char*)getauxval( AT_PLATFORM ) );
#endif
#endif // !arm32
}

#endif // __linux__

#endif // non-x86

/*
================
Sys_SnapVector
================
*/
#ifdef _MSC_VER
#if idx64
void Sys_SnapVector( float *vector )
{
	__m128 vf0, vf1, vf2;
	__m128i vi;
	DWORD mxcsr;

	mxcsr = _mm_getcsr();
	vf0 = _mm_setr_ps( vector[0], vector[1], vector[2], 0.0f );

	_mm_setcsr( mxcsr & ~0x6000 ); // enforce rounding mode to "round to nearest"

	vi = _mm_cvtps_epi32( vf0 );
	vf0 = _mm_cvtepi32_ps( vi );

	vf1 = _mm_shuffle_ps(vf0, vf0, _MM_SHUFFLE(1,1,1,1));
	vf2 = _mm_shuffle_ps(vf0, vf0, _MM_SHUFFLE(2,2,2,2));

	_mm_setcsr( mxcsr ); // restore rounding mode

	_mm_store_ss( &vector[0], vf0 );
	_mm_store_ss( &vector[1], vf1 );
	_mm_store_ss( &vector[2], vf2 );
}
#endif // idx64

#if id386
void Sys_SnapVector( float *vector )
{
	static const DWORD cw037F = 0x037F;
	DWORD cwCurr;
__asm {
	fnstcw word ptr [cwCurr]
	mov ecx, vector
	fldcw word ptr [cw037F]

	fld   dword ptr[ecx+8]
	fistp dword ptr[ecx+8]
	fild  dword ptr[ecx+8]
	fstp  dword ptr[ecx+8]

	fld   dword ptr[ecx+4]
	fistp dword ptr[ecx+4]
	fild  dword ptr[ecx+4]
	fstp  dword ptr[ecx+4]

	fld   dword ptr[ecx+0]
	fistp dword ptr[ecx+0]
	fild  dword ptr[ecx+0]
	fstp  dword ptr[ecx+0]

	fldcw word ptr cwCurr
	}; // __asm
}
#endif // id386

#if arm64 || arm32
void Sys_SnapVector( float *vector )
{
	vector[0] = rint( (double)vector[0] );
	vector[1] = rint( (double)vector[1] );
	vector[2] = rint( (double)vector[2] );
}
#endif

#else // clang/gcc/mingw

#if id386

#define QROUNDX87(src) \
	"flds " src "\n" \
	"fistpl " src "\n" \
	"fildl " src "\n" \
	"fstps " src "\n"

void Sys_SnapVector( float *vector )
{
	static const unsigned short cw037F = 0x037F;
	unsigned short cwCurr;

	__asm__ volatile
	(
		"fnstcw %1\n" \
		"fldcw %2\n" \
		QROUNDX87("0(%0)")
		QROUNDX87("4(%0)")
		QROUNDX87("8(%0)")
		"fldcw %1\n" \
		:
		: "r" (vector), "m"(cwCurr), "m"(cw037F)
		: "memory", "st"
	);
}

#else // idx64, non-x86

void Sys_SnapVector( float *vector )
{
	vector[0] = rint( (double)vector[0] );
	vector[1] = rint( (double)vector[1] );
	vector[2] = rint( (double)vector[2] );
}

#endif

#endif // clang/gcc/mingw

#ifdef USE_AFFINITY_MASK

static int hex_code( const int code ) {
	if ( code >= '0' && code <= '9' ) {
		return code - '0';
	}
	if ( code >= 'A' && code <= 'F' ) {
		return code - 'A' + 10;
	}
	if ( code >= 'a' && code <= 'f' ) {
		return code - 'a' + 10;
	}
	return -1;
}


static const char *parseAffinityMask( const char *str, uint64_t *outv, int level ) {
	uint64_t v, mask = 0;

	while ( *str != '\0' ) {
		if ( *str == 'A' || *str == 'a' ) {
			mask = affinityMask;
			++str;
			continue;
		}
		else if ( *str == 'P' || *str == 'p' ) {
			mask = pCoreMask;
			++str;
			continue;
		}
		else if ( *str == 'E' || *str == 'e' ) {
			mask = eCoreMask;
			++str;
			continue;
		}
		else if ( *str == '0' && (str[1] == 'x' || str[1] == 'X') && (v = hex_code( str[2] )) >= 0 ) {
			int hex;
			str += 3; // 0xH
			while ( (hex = hex_code( *str )) >= 0 ) {
				v = v * 16 + hex;
				str++;
			}
			mask = v;
			continue;
		}
		else if ( *str >= '0' && *str <= '9' ) {
			mask = *str++ - '0';
			while ( *str >= '0' && *str <= '9' ) {
				mask = mask * 10 + *str - '0';
				++str;
			}
			continue;
		}

		if ( level == 0 ) {
			while ( *str == '+' || *str == '-' ) {
				str = parseAffinityMask( str + 1, &v, level + 1 );
				switch ( *str ) {
					case '+': mask |= v; break;
					case '-': mask &= ~v; break;
					default: str = ""; break;
				}
			}
			if ( *str != '\0' ) {
				++str; // skip unknown characters
			}
		} else {
			break;
		}
	}

	*outv = mask;
	return str;
}


// parse and set affinity mask
void Sys_ApplyAffinityMask( const char *str )
{
	uint64_t mask = 0;

	parseAffinityMask( str, &mask, 0 );

	if ( ( mask & affinityMask ) == 0 ) {
		mask = affinityMask; // reset to default
	}

	if ( mask != 0 ) {
		Sys_SetAffinityMask( mask );
	}
}
#endif // USE_AFFINITY_MASK

#ifdef USE_AFFINITY_MASK
void Sys_InitAffinity( void )
{
	// get initial process affinity - we will respect it when setting custom affinity masks
	eCoreMask = pCoreMask = affinityMask = Sys_GetAffinityMask();
#if (idx64 || id386)
	DetectCPUCoresConfig();
#endif
}
#endif

time_t Sys_Time( time_t *result )
{
	return time( result );
}

struct tm *Sys_LocalTime( const time_t *value )
{
	return localtime( value );
}

char *Sys_CTime( const time_t *value )
{
	return ctime( value );
}
