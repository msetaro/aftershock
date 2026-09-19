//
//
// bg_lib,c -- standard C library replacement routines used by code
// compiled for the virtual machine

#include "q_shared.h"
#include <stdint.h>
/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)qsort.c	8.1 (Berkeley) 6/4/93";
#endif
static const char rcsid[] =
#endif /* LIBC_SCCS and not lint */

// bk001127 - needed for DLL's
#if !defined( Q3_VM )
typedef int		 cmp_t(const void *, const void *);
#endif

static char* med3(char *, char *, char *, cmp_t *);
static void	 swapfunc(char *, char *, int, int);

#ifndef min
#define min(a, b)	(a) < (b) ? a : b
#endif

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) { 		\
	long i = (n) / sizeof (TYPE); 			\
	TYPE *pi = (TYPE *) (parmi); 		\
	TYPE *pj = (TYPE *) (parmj); 		\
	do { 						\
		TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
        } while (--i > 0);				\
}

#define SWAPINIT(a, es) swaptype = ((uintptr_t)a) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static void
swapfunc(char *a, char *b, int n, int swaptype)
{
	if(swaptype <= 1)
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b)					\
	if (swaptype == 0) {				\
		long t = *(long *)(a);			\
		*(long *)(a) = *(long *)(b);		\
		*(long *)(b) = t;			\
	} else						\
		swapfunc(a, b, (int)es, swaptype)

#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n, swaptype)

static char *
med3(char *a, char *b, char *c, cmp_t *cmp)
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
              :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}

void
qsort(void *a, size_t n, size_t es, cmp_t *cmp)
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swaptype, swap_cnt;

loop:	SWAPINIT(a, es);
	swap_cnt = 0;
	if (n < 7) {
		for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
			for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}
	pm = (char *)a + (n / 2) * es;
	if (n > 7) {
		pl = (char *)a;
		pn = (char *)a + (n - 1) * es;
		if (n > 40) {
			d = (int)( (n / 8) * es );
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap((char *)a, pm);
	pa = pb = (char *)a + es;

	pc = pd = (char *)a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
			for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}

	pn = (char *)a + n * es;
	r = min(pa - (char *)a, pb - pa);
	vecswap((char *)a, pb - r, r);
	r = (int)( min((size_t)(pd - pc), pn - pd - es) );
	vecswap(pb, pn - r, r);
	if ((size_t)( (r = pb - pa) ) > es)
		qsort(a, r / es, es, cmp);
	if ((size_t)( (r = pd - pc) ) > es) {
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r / es;
		goto loop;
	}
/*		qsort(pn - r, r / es, es, cmp);*/
}

//==================================================================================


// this file is excluded from release builds because of intrinsics

// bk001211 - gcc errors on compiling strcpy:  parse error before `__extension__'
#if defined ( Q3_VM )

size_t strlen( const char *string ) {
	const char	*s;

	s = string;
	while ( *s ) {
		s++;
	}
	return s - string;
}


char *strcat( char *strDestination, const char *strSource ) {
	char	*s;

	s = strDestination;
	while ( *s ) {
		s++;
	}
	while ( *strSource ) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}

char *strcpy( char *strDestination, const char *strSource ) {
	char *s;

	s = strDestination;
	while ( *strSource ) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}


int strcmp( const char *string1, const char *string2 ) {
	while ( *string1 == *string2 && *string1 && *string2 ) {
		string1++;
		string2++;
	}
	return *string1 - *string2;
}


char *strchr( const char *string, int c ) {
	while ( *string ) {
		if ( *string == c ) {
			return ( char * )string;
		}
		string++;
	}
	return (char *)0;
}

char *strstr( const char *string, const char *strCharSet ) {
	while ( *string ) {
		int		i;

		for ( i = 0 ; strCharSet[i] ; i++ ) {
			if ( string[i] != strCharSet[i] ) {
				break;
			}
		}
		if ( !strCharSet[i] ) {
			return (char *)string;
		}
		string++;
	}
	return (char *)0;
}
#endif // bk001211

// bk001120 - presumably needed for Mac
//#if !defined(_MSC_VER) && !defined(__linux__)
// bk001127 - undid undo
#if defined ( Q3_VM )
int tolower( int c ) {
	if ( c >= 'A' && c <= 'Z' ) {
		c += 'a' - 'A';
	}
	return c;
}


int toupper( int c ) {
	if ( c >= 'a' && c <= 'z' ) {
		c += 'A' - 'a';
	}
	return c;
}

#endif
//#ifndef _MSC_VER

void *memmove( void *dest, const void *src, size_t count ) {
	int		i;

	if ( dest > src ) {
		for ( i = (int)( count-1 ) ; i >= 0 ; i-- ) {
			((char *)dest)[i] = ((char *)src)[i];
		}
	} else {
		for ( i = 0 ; (size_t)i < count ; i++ ) {
			((char *)dest)[i] = ((char *)src)[i];
		}
	}
	return dest;
}


#if 0

double floor( double x ) {
	return (int)(x + 0x40000000) - 0x40000000;
}

void *memset( void *dest, int c, size_t count ) {
	while ( count-- ) {
		((char *)dest)[count] = c;
	}
	return dest;
}

void *memcpy( void *dest, const void *src, size_t count ) {
	while ( count-- ) {
		((char *)dest)[count] = ((char *)src)[count];
	}
	return dest;
}

char *strncpy( char *strDest, const char *strSource, size_t count ) {
	char	*s;

	s = strDest;
	while ( *strSource && count ) {
		*s++ = *strSource++;
		count--;
	}
	while ( count-- ) {
		*s++ = 0;
	}
	return strDest;
}

double sqrt( double x ) {
	float	y;
	float	delta;
	float	maxError;

	if ( x <= 0 ) {
		return 0;
	}

	// initial guess
	y = x / 2;

	// refine
	maxError = x * 0.001f;

	do {
		delta = ( y * y ) - x;
		y -= delta / ( 2 * y );
	} while ( delta > maxError || delta < -maxError );

	return y;
}


float sintable[1024] = {
0.000000f,0.001534f,0.003068f,0.004602f,0.006136f,0.007670f,0.009204f,0.010738f,
0.012272f,0.013805f,0.015339f,0.016873f,0.018407f,0.019940f,0.021474f,0.023008f,
0.024541f,0.026075f,0.027608f,0.029142f,0.030675f,0.032208f,0.033741f,0.035274f,
0.036807f,0.038340f,0.039873f,0.041406f,0.042938f,0.044471f,0.046003f,0.047535f,
0.049068f,0.050600f,0.052132f,0.053664f,0.055195f,0.056727f,0.058258f,0.059790f,
0.061321f,0.062852f,0.064383f,0.065913f,0.067444f,0.068974f,0.070505f,0.072035f,
0.073565f,0.075094f,0.076624f,0.078153f,0.079682f,0.081211f,0.082740f,0.084269f,
0.085797f,0.087326f,0.088854f,0.090381f,0.091909f,0.093436f,0.094963f,0.096490f,
0.098017f,0.099544f,0.101070f,0.102596f,0.104122f,0.105647f,0.107172f,0.108697f,
0.110222f,0.111747f,0.113271f,0.114795f,0.116319f,0.117842f,0.119365f,0.120888f,
0.122411f,0.123933f,0.125455f,0.126977f,0.128498f,0.130019f,0.131540f,0.133061f,
0.134581f,0.136101f,0.137620f,0.139139f,0.140658f,0.142177f,0.143695f,0.145213f,
0.146730f,0.148248f,0.149765f,0.151281f,0.152797f,0.154313f,0.155828f,0.157343f,
0.158858f,0.160372f,0.161886f,0.163400f,0.164913f,0.166426f,0.167938f,0.169450f,
0.170962f,0.172473f,0.173984f,0.175494f,0.177004f,0.178514f,0.180023f,0.181532f,
0.183040f,0.184548f,0.186055f,0.187562f,0.189069f,0.190575f,0.192080f,0.193586f,
0.195090f,0.196595f,0.198098f,0.199602f,0.201105f,0.202607f,0.204109f,0.205610f,
0.207111f,0.208612f,0.210112f,0.211611f,0.213110f,0.214609f,0.216107f,0.217604f,
0.219101f,0.220598f,0.222094f,0.223589f,0.225084f,0.226578f,0.228072f,0.229565f,
0.231058f,0.232550f,0.234042f,0.235533f,0.237024f,0.238514f,0.240003f,0.241492f,
0.242980f,0.244468f,0.245955f,0.247442f,0.248928f,0.250413f,0.251898f,0.253382f,
0.254866f,0.256349f,0.257831f,0.259313f,0.260794f,0.262275f,0.263755f,0.265234f,
0.266713f,0.268191f,0.269668f,0.271145f,0.272621f,0.274097f,0.275572f,0.277046f,
0.278520f,0.279993f,0.281465f,0.282937f,0.284408f,0.285878f,0.287347f,0.288816f,
0.290285f,0.291752f,0.293219f,0.294685f,0.296151f,0.297616f,0.299080f,0.300543f,
0.302006f,0.303468f,0.304929f,0.306390f,0.307850f,0.309309f,0.310767f,0.312225f,
0.313682f,0.315138f,0.316593f,0.318048f,0.319502f,0.320955f,0.322408f,0.323859f,
0.325310f,0.326760f,0.328210f,0.329658f,0.331106f,0.332553f,0.334000f,0.335445f,
0.336890f,0.338334f,0.339777f,0.341219f,0.342661f,0.344101f,0.345541f,0.346980f,
0.348419f,0.349856f,0.351293f,0.352729f,0.354164f,0.355598f,0.357031f,0.358463f,
0.359895f,0.361326f,0.362756f,0.364185f,0.365613f,0.367040f,0.368467f,0.369892f,
0.371317f,0.372741f,0.374164f,0.375586f,0.377007f,0.378428f,0.379847f,0.381266f,
0.382683f,0.384100f,0.385516f,0.386931f,0.388345f,0.389758f,0.391170f,0.392582f,
0.393992f,0.395401f,0.396810f,0.398218f,0.399624f,0.401030f,0.402435f,0.403838f,
0.405241f,0.406643f,0.408044f,0.409444f,0.410843f,0.412241f,0.413638f,0.415034f,
0.416430f,0.417824f,0.419217f,0.420609f,0.422000f,0.423390f,0.424780f,0.426168f,
0.427555f,0.428941f,0.430326f,0.431711f,0.433094f,0.434476f,0.435857f,0.437237f,
0.438616f,0.439994f,0.441371f,0.442747f,0.444122f,0.445496f,0.446869f,0.448241f,
0.449611f,0.450981f,0.452350f,0.453717f,0.455084f,0.456449f,0.457813f,0.459177f,
0.460539f,0.461900f,0.463260f,0.464619f,0.465976f,0.467333f,0.468689f,0.470043f,
0.471397f,0.472749f,0.474100f,0.475450f,0.476799f,0.478147f,0.479494f,0.480839f,
0.482184f,0.483527f,0.484869f,0.486210f,0.487550f,0.488889f,0.490226f,0.491563f,
0.492898f,0.494232f,0.495565f,0.496897f,0.498228f,0.499557f,0.500885f,0.502212f,
0.503538f,0.504863f,0.506187f,0.507509f,0.508830f,0.510150f,0.511469f,0.512786f,
0.514103f,0.515418f,0.516732f,0.518045f,0.519356f,0.520666f,0.521975f,0.523283f,
0.524590f,0.525895f,0.527199f,0.528502f,0.529804f,0.531104f,0.532403f,0.533701f,
0.534998f,0.536293f,0.537587f,0.538880f,0.540171f,0.541462f,0.542751f,0.544039f,
0.545325f,0.546610f,0.547894f,0.549177f,0.550458f,0.551738f,0.553017f,0.554294f,
0.555570f,0.556845f,0.558119f,0.559391f,0.560662f,0.561931f,0.563199f,0.564466f,
0.565732f,0.566996f,0.568259f,0.569521f,0.570781f,0.572040f,0.573297f,0.574553f,
0.575808f,0.577062f,0.578314f,0.579565f,0.580814f,0.582062f,0.583309f,0.584554f,
0.585798f,0.587040f,0.588282f,0.589521f,0.590760f,0.591997f,0.593232f,0.594466f,
0.595699f,0.596931f,0.598161f,0.599389f,0.600616f,0.601842f,0.603067f,0.604290f,
0.605511f,0.606731f,0.607950f,0.609167f,0.610383f,0.611597f,0.612810f,0.614022f,
0.615232f,0.616440f,0.617647f,0.618853f,0.620057f,0.621260f,0.622461f,0.623661f,
0.624859f,0.626056f,0.627252f,0.628446f,0.629638f,0.630829f,0.632019f,0.633207f,
0.634393f,0.635578f,0.636762f,0.637944f,0.639124f,0.640303f,0.641481f,0.642657f,
0.643832f,0.645005f,0.646176f,0.647346f,0.648514f,0.649681f,0.650847f,0.652011f,
0.653173f,0.654334f,0.655493f,0.656651f,0.657807f,0.658961f,0.660114f,0.661266f,
0.662416f,0.663564f,0.664711f,0.665856f,0.667000f,0.668142f,0.669283f,0.670422f,
0.671559f,0.672695f,0.673829f,0.674962f,0.676093f,0.677222f,0.678350f,0.679476f,
0.680601f,0.681724f,0.682846f,0.683965f,0.685084f,0.686200f,0.687315f,0.688429f,
0.689541f,0.690651f,0.691759f,0.692866f,0.693971f,0.695075f,0.696177f,0.697278f,
0.698376f,0.699473f,0.700569f,0.701663f,0.702755f,0.703845f,0.704934f,0.706021f,
0.707107f,0.708191f,0.709273f,0.710353f,0.711432f,0.712509f,0.713585f,0.714659f,
0.715731f,0.716801f,0.717870f,0.718937f,0.720003f,0.721066f,0.722128f,0.723188f,
0.724247f,0.725304f,0.726359f,0.727413f,0.728464f,0.729514f,0.730563f,0.731609f,
0.732654f,0.733697f,0.734739f,0.735779f,0.736817f,0.737853f,0.738887f,0.739920f,
0.740951f,0.741980f,0.743008f,0.744034f,0.745058f,0.746080f,0.747101f,0.748119f,
0.749136f,0.750152f,0.751165f,0.752177f,0.753187f,0.754195f,0.755201f,0.756206f,
0.757209f,0.758210f,0.759209f,0.760207f,0.761202f,0.762196f,0.763188f,0.764179f,
0.765167f,0.766154f,0.767139f,0.768122f,0.769103f,0.770083f,0.771061f,0.772036f,
0.773010f,0.773983f,0.774953f,0.775922f,0.776888f,0.777853f,0.778817f,0.779778f,
0.780737f,0.781695f,0.782651f,0.783605f,0.784557f,0.785507f,0.786455f,0.787402f,
0.788346f,0.789289f,0.790230f,0.791169f,0.792107f,0.793042f,0.793975f,0.794907f,
0.795837f,0.796765f,0.797691f,0.798615f,0.799537f,0.800458f,0.801376f,0.802293f,
0.803208f,0.804120f,0.805031f,0.805940f,0.806848f,0.807753f,0.808656f,0.809558f,
0.810457f,0.811355f,0.812251f,0.813144f,0.814036f,0.814926f,0.815814f,0.816701f,
0.817585f,0.818467f,0.819348f,0.820226f,0.821103f,0.821977f,0.822850f,0.823721f,
0.824589f,0.825456f,0.826321f,0.827184f,0.828045f,0.828904f,0.829761f,0.830616f,
0.831470f,0.832321f,0.833170f,0.834018f,0.834863f,0.835706f,0.836548f,0.837387f,
0.838225f,0.839060f,0.839894f,0.840725f,0.841555f,0.842383f,0.843208f,0.844032f,
0.844854f,0.845673f,0.846491f,0.847307f,0.848120f,0.848932f,0.849742f,0.850549f,
0.851355f,0.852159f,0.852961f,0.853760f,0.854558f,0.855354f,0.856147f,0.856939f,
0.857729f,0.858516f,0.859302f,0.860085f,0.860867f,0.861646f,0.862424f,0.863199f,
0.863973f,0.864744f,0.865514f,0.866281f,0.867046f,0.867809f,0.868571f,0.869330f,
0.870087f,0.870842f,0.871595f,0.872346f,0.873095f,0.873842f,0.874587f,0.875329f,
0.876070f,0.876809f,0.877545f,0.878280f,0.879012f,0.879743f,0.880471f,0.881197f,
0.881921f,0.882643f,0.883363f,0.884081f,0.884797f,0.885511f,0.886223f,0.886932f,
0.887640f,0.888345f,0.889048f,0.889750f,0.890449f,0.891146f,0.891841f,0.892534f,
0.893224f,0.893913f,0.894599f,0.895284f,0.895966f,0.896646f,0.897325f,0.898001f,
0.898674f,0.899346f,0.900016f,0.900683f,0.901349f,0.902012f,0.902673f,0.903332f,
0.903989f,0.904644f,0.905297f,0.905947f,0.906596f,0.907242f,0.907886f,0.908528f,
0.909168f,0.909806f,0.910441f,0.911075f,0.911706f,0.912335f,0.912962f,0.913587f,
0.914210f,0.914830f,0.915449f,0.916065f,0.916679f,0.917291f,0.917901f,0.918508f,
0.919114f,0.919717f,0.920318f,0.920917f,0.921514f,0.922109f,0.922701f,0.923291f,
0.923880f,0.924465f,0.925049f,0.925631f,0.926210f,0.926787f,0.927363f,0.927935f,
0.928506f,0.929075f,0.929641f,0.930205f,0.930767f,0.931327f,0.931884f,0.932440f,
0.932993f,0.933544f,0.934093f,0.934639f,0.935184f,0.935726f,0.936266f,0.936803f,
0.937339f,0.937872f,0.938404f,0.938932f,0.939459f,0.939984f,0.940506f,0.941026f,
0.941544f,0.942060f,0.942573f,0.943084f,0.943593f,0.944100f,0.944605f,0.945107f,
0.945607f,0.946105f,0.946601f,0.947094f,0.947586f,0.948075f,0.948561f,0.949046f,
0.949528f,0.950008f,0.950486f,0.950962f,0.951435f,0.951906f,0.952375f,0.952842f,
0.953306f,0.953768f,0.954228f,0.954686f,0.955141f,0.955594f,0.956045f,0.956494f,
0.956940f,0.957385f,0.957826f,0.958266f,0.958703f,0.959139f,0.959572f,0.960002f,
0.960431f,0.960857f,0.961280f,0.961702f,0.962121f,0.962538f,0.962953f,0.963366f,
0.963776f,0.964184f,0.964590f,0.964993f,0.965394f,0.965793f,0.966190f,0.966584f,
0.966976f,0.967366f,0.967754f,0.968139f,0.968522f,0.968903f,0.969281f,0.969657f,
0.970031f,0.970403f,0.970772f,0.971139f,0.971504f,0.971866f,0.972226f,0.972584f,
0.972940f,0.973293f,0.973644f,0.973993f,0.974339f,0.974684f,0.975025f,0.975365f,
0.975702f,0.976037f,0.976370f,0.976700f,0.977028f,0.977354f,0.977677f,0.977999f,
0.978317f,0.978634f,0.978948f,0.979260f,0.979570f,0.979877f,0.980182f,0.980485f,
0.980785f,0.981083f,0.981379f,0.981673f,0.981964f,0.982253f,0.982539f,0.982824f,
0.983105f,0.983385f,0.983662f,0.983937f,0.984210f,0.984480f,0.984749f,0.985014f,
0.985278f,0.985539f,0.985798f,0.986054f,0.986308f,0.986560f,0.986809f,0.987057f,
0.987301f,0.987544f,0.987784f,0.988022f,0.988258f,0.988491f,0.988722f,0.988950f,
0.989177f,0.989400f,0.989622f,0.989841f,0.990058f,0.990273f,0.990485f,0.990695f,
0.990903f,0.991108f,0.991311f,0.991511f,0.991710f,0.991906f,0.992099f,0.992291f,
0.992480f,0.992666f,0.992850f,0.993032f,0.993212f,0.993389f,0.993564f,0.993737f,
0.993907f,0.994075f,0.994240f,0.994404f,0.994565f,0.994723f,0.994879f,0.995033f,
0.995185f,0.995334f,0.995481f,0.995625f,0.995767f,0.995907f,0.996045f,0.996180f,
0.996313f,0.996443f,0.996571f,0.996697f,0.996820f,0.996941f,0.997060f,0.997176f,
0.997290f,0.997402f,0.997511f,0.997618f,0.997723f,0.997825f,0.997925f,0.998023f,
0.998118f,0.998211f,0.998302f,0.998390f,0.998476f,0.998559f,0.998640f,0.998719f,
0.998795f,0.998870f,0.998941f,0.999011f,0.999078f,0.999142f,0.999205f,0.999265f,
0.999322f,0.999378f,0.999431f,0.999481f,0.999529f,0.999575f,0.999619f,0.999660f,
0.999699f,0.999735f,0.999769f,0.999801f,0.999831f,0.999858f,0.999882f,0.999905f,
0.999925f,0.999942f,0.999958f,0.999971f,0.999981f,0.999989f,0.999995f,0.999999f
};

double sin( double x ) {
	int	index;
	int	quad;

	index = 1024 * x / (M_PI * 0.5f);
	quad = ( index >> 10 ) & 3;
	index &= 1023;
	switch ( quad ) {
	case 0:
		return sintable[index];
	case 1:
		return sintable[1023-index];
	case 2:
		return -sintable[index];
	case 3:
		return -sintable[1023-index];
	}
	return 0;
}


double cos( double x ) {
	int	index;
	int	quad;

	index = 1024 * x / (M_PI * 0.5f);
	quad = ( index >> 10 ) & 3;
	index &= 1023;
	switch ( quad ) {
	case 3:
		return sintable[index];
	case 0:
		return sintable[1023-index];
	case 1:
		return -sintable[index];
	case 2:
		return -sintable[1023-index];
	}
	return 0;
}


/*
void create_acostable( void ) {
	int i;
	FILE *fp;
	float a;

	fp = fopen("c:\\acostable.txt", "w");
	fprintf(fp, "float acostable[] = {");
	for (i = 0; i < 1024; i++) {
		if (!(i & 7))
			fprintf(fp, "\n");
		a = acos( (float) -1 + i / 512 );
		fprintf(fp, "%1.8f,", a);
	}
	fprintf(fp, "\n}\n");
	fclose(fp);
}
*/

float acostable[] = {
3.14159265f,3.07908248f,3.05317551f,3.03328655f,3.01651113f,3.00172442f,2.98834964f,2.97604422f,
2.96458497f,2.95381690f,2.94362719f,2.93393068f,2.92466119f,2.91576615f,2.90720289f,2.89893629f,
2.89093699f,2.88318015f,2.87564455f,2.86831188f,2.86116621f,2.85419358f,2.84738169f,2.84071962f,
2.83419760f,2.82780691f,2.82153967f,2.81538876f,2.80934770f,2.80341062f,2.79757211f,2.79182724f,
2.78617145f,2.78060056f,2.77511069f,2.76969824f,2.76435988f,2.75909250f,2.75389319f,2.74875926f,
2.74368816f,2.73867752f,2.73372510f,2.72882880f,2.72398665f,2.71919677f,2.71445741f,2.70976688f,
2.70512362f,2.70052613f,2.69597298f,2.69146283f,2.68699438f,2.68256642f,2.67817778f,2.67382735f,
2.66951407f,2.66523692f,2.66099493f,2.65678719f,2.65261279f,2.64847088f,2.64436066f,2.64028133f,
2.63623214f,2.63221238f,2.62822133f,2.62425835f,2.62032277f,2.61641398f,2.61253138f,2.60867440f,
2.60484248f,2.60103507f,2.59725167f,2.59349176f,2.58975488f,2.58604053f,2.58234828f,2.57867769f,
2.57502832f,2.57139977f,2.56779164f,2.56420354f,2.56063509f,2.55708594f,2.55355572f,2.55004409f,
2.54655073f,2.54307530f,2.53961750f,2.53617701f,2.53275354f,2.52934680f,2.52595650f,2.52258238f,
2.51922417f,2.51588159f,2.51255441f,2.50924238f,2.50594525f,2.50266278f,2.49939476f,2.49614096f,
2.49290115f,2.48967513f,2.48646269f,2.48326362f,2.48007773f,2.47690482f,2.47374472f,2.47059722f,
2.46746215f,2.46433933f,2.46122860f,2.45812977f,2.45504269f,2.45196720f,2.44890314f,2.44585034f,
2.44280867f,2.43977797f,2.43675809f,2.43374890f,2.43075025f,2.42776201f,2.42478404f,2.42181622f,
2.41885841f,2.41591048f,2.41297232f,2.41004380f,2.40712480f,2.40421521f,2.40131491f,2.39842379f,
2.39554173f,2.39266863f,2.38980439f,2.38694889f,2.38410204f,2.38126374f,2.37843388f,2.37561237f,
2.37279910f,2.36999400f,2.36719697f,2.36440790f,2.36162673f,2.35885335f,2.35608768f,2.35332964f,
2.35057914f,2.34783610f,2.34510044f,2.34237208f,2.33965094f,2.33693695f,2.33423003f,2.33153010f,
2.32883709f,2.32615093f,2.32347155f,2.32079888f,2.31813284f,2.31547337f,2.31282041f,2.31017388f,
2.30753373f,2.30489988f,2.30227228f,2.29965086f,2.29703556f,2.29442632f,2.29182309f,2.28922580f,
2.28663439f,2.28404881f,2.28146900f,2.27889490f,2.27632647f,2.27376364f,2.27120637f,2.26865460f,
2.26610827f,2.26356735f,2.26103177f,2.25850149f,2.25597646f,2.25345663f,2.25094195f,2.24843238f,
2.24592786f,2.24342836f,2.24093382f,2.23844420f,2.23595946f,2.23347956f,2.23100444f,2.22853408f,
2.22606842f,2.22360742f,2.22115104f,2.21869925f,2.21625199f,2.21380924f,2.21137096f,2.20893709f,
2.20650761f,2.20408248f,2.20166166f,2.19924511f,2.19683280f,2.19442469f,2.19202074f,2.18962092f,
2.18722520f,2.18483354f,2.18244590f,2.18006225f,2.17768257f,2.17530680f,2.17293493f,2.17056692f,
2.16820274f,2.16584236f,2.16348574f,2.16113285f,2.15878367f,2.15643816f,2.15409630f,2.15175805f,
2.14942338f,2.14709226f,2.14476468f,2.14244059f,2.14011997f,2.13780279f,2.13548903f,2.13317865f,
2.13087163f,2.12856795f,2.12626757f,2.12397047f,2.12167662f,2.11938600f,2.11709859f,2.11481435f,
2.11253326f,2.11025530f,2.10798044f,2.10570867f,2.10343994f,2.10117424f,2.09891156f,2.09665185f,
2.09439510f,2.09214129f,2.08989040f,2.08764239f,2.08539725f,2.08315496f,2.08091550f,2.07867884f,
2.07644495f,2.07421383f,2.07198545f,2.06975978f,2.06753681f,2.06531651f,2.06309887f,2.06088387f,
2.05867147f,2.05646168f,2.05425445f,2.05204979f,2.04984765f,2.04764804f,2.04545092f,2.04325628f,
2.04106409f,2.03887435f,2.03668703f,2.03450211f,2.03231957f,2.03013941f,2.02796159f,2.02578610f,
2.02361292f,2.02144204f,2.01927344f,2.01710710f,2.01494300f,2.01278113f,2.01062146f,2.00846399f,
2.00630870f,2.00415556f,2.00200457f,1.99985570f,1.99770895f,1.99556429f,1.99342171f,1.99128119f,
1.98914271f,1.98700627f,1.98487185f,1.98273942f,1.98060898f,1.97848051f,1.97635399f,1.97422942f,
1.97210676f,1.96998602f,1.96786718f,1.96575021f,1.96363511f,1.96152187f,1.95941046f,1.95730088f,
1.95519310f,1.95308712f,1.95098292f,1.94888050f,1.94677982f,1.94468089f,1.94258368f,1.94048818f,
1.93839439f,1.93630228f,1.93421185f,1.93212308f,1.93003595f,1.92795046f,1.92586659f,1.92378433f,
1.92170367f,1.91962459f,1.91754708f,1.91547113f,1.91339673f,1.91132385f,1.90925250f,1.90718266f,
1.90511432f,1.90304746f,1.90098208f,1.89891815f,1.89685568f,1.89479464f,1.89273503f,1.89067683f,
1.88862003f,1.88656463f,1.88451060f,1.88245794f,1.88040664f,1.87835668f,1.87630806f,1.87426076f,
1.87221477f,1.87017008f,1.86812668f,1.86608457f,1.86404371f,1.86200412f,1.85996577f,1.85792866f,
1.85589277f,1.85385809f,1.85182462f,1.84979234f,1.84776125f,1.84573132f,1.84370256f,1.84167495f,
1.83964848f,1.83762314f,1.83559892f,1.83357582f,1.83155381f,1.82953289f,1.82751305f,1.82549429f,
1.82347658f,1.82145993f,1.81944431f,1.81742973f,1.81541617f,1.81340362f,1.81139207f,1.80938151f,
1.80737194f,1.80536334f,1.80335570f,1.80134902f,1.79934328f,1.79733848f,1.79533460f,1.79333164f,
1.79132959f,1.78932843f,1.78732817f,1.78532878f,1.78333027f,1.78133261f,1.77933581f,1.77733985f,
1.77534473f,1.77335043f,1.77135695f,1.76936428f,1.76737240f,1.76538132f,1.76339101f,1.76140148f,
1.75941271f,1.75742470f,1.75543743f,1.75345090f,1.75146510f,1.74948002f,1.74749565f,1.74551198f,
1.74352900f,1.74154672f,1.73956511f,1.73758417f,1.73560389f,1.73362426f,1.73164527f,1.72966692f,
1.72768920f,1.72571209f,1.72373560f,1.72175971f,1.71978441f,1.71780969f,1.71583556f,1.71386199f,
1.71188899f,1.70991653f,1.70794462f,1.70597325f,1.70400241f,1.70203209f,1.70006228f,1.69809297f,
1.69612416f,1.69415584f,1.69218799f,1.69022062f,1.68825372f,1.68628727f,1.68432127f,1.68235571f,
1.68039058f,1.67842588f,1.67646160f,1.67449772f,1.67253424f,1.67057116f,1.66860847f,1.66664615f,
1.66468420f,1.66272262f,1.66076139f,1.65880050f,1.65683996f,1.65487975f,1.65291986f,1.65096028f,
1.64900102f,1.64704205f,1.64508338f,1.64312500f,1.64116689f,1.63920905f,1.63725148f,1.63529416f,
1.63333709f,1.63138026f,1.62942366f,1.62746728f,1.62551112f,1.62355517f,1.62159943f,1.61964388f,
1.61768851f,1.61573332f,1.61377831f,1.61182346f,1.60986877f,1.60791422f,1.60595982f,1.60400556f,
1.60205142f,1.60009739f,1.59814349f,1.59618968f,1.59423597f,1.59228235f,1.59032882f,1.58837536f,
1.58642196f,1.58446863f,1.58251535f,1.58056211f,1.57860891f,1.57665574f,1.57470259f,1.57274945f,
1.57079633f,1.56884320f,1.56689007f,1.56493692f,1.56298375f,1.56103055f,1.55907731f,1.55712403f,
1.55517069f,1.55321730f,1.55126383f,1.54931030f,1.54735668f,1.54540297f,1.54344917f,1.54149526f,
1.53954124f,1.53758710f,1.53563283f,1.53367843f,1.53172389f,1.52976919f,1.52781434f,1.52585933f,
1.52390414f,1.52194878f,1.51999323f,1.51803748f,1.51608153f,1.51412537f,1.51216900f,1.51021240f,
1.50825556f,1.50629849f,1.50434117f,1.50238360f,1.50042576f,1.49846765f,1.49650927f,1.49455060f,
1.49259163f,1.49063237f,1.48867280f,1.48671291f,1.48475270f,1.48279215f,1.48083127f,1.47887004f,
1.47690845f,1.47494650f,1.47298419f,1.47102149f,1.46905841f,1.46709493f,1.46513106f,1.46316677f,
1.46120207f,1.45923694f,1.45727138f,1.45530538f,1.45333893f,1.45137203f,1.44940466f,1.44743682f,
1.44546850f,1.44349969f,1.44153038f,1.43956057f,1.43759024f,1.43561940f,1.43364803f,1.43167612f,
1.42970367f,1.42773066f,1.42575709f,1.42378296f,1.42180825f,1.41983295f,1.41785705f,1.41588056f,
1.41390346f,1.41192573f,1.40994738f,1.40796840f,1.40598877f,1.40400849f,1.40202755f,1.40004594f,
1.39806365f,1.39608068f,1.39409701f,1.39211264f,1.39012756f,1.38814175f,1.38615522f,1.38416795f,
1.38217994f,1.38019117f,1.37820164f,1.37621134f,1.37422025f,1.37222837f,1.37023570f,1.36824222f,
1.36624792f,1.36425280f,1.36225684f,1.36026004f,1.35826239f,1.35626387f,1.35426449f,1.35226422f,
1.35026307f,1.34826101f,1.34625805f,1.34425418f,1.34224937f,1.34024364f,1.33823695f,1.33622932f,
1.33422072f,1.33221114f,1.33020059f,1.32818904f,1.32617649f,1.32416292f,1.32214834f,1.32013273f,
1.31811607f,1.31609837f,1.31407960f,1.31205976f,1.31003885f,1.30801684f,1.30599373f,1.30396951f,
1.30194417f,1.29991770f,1.29789009f,1.29586133f,1.29383141f,1.29180031f,1.28976803f,1.28773456f,
1.28569989f,1.28366400f,1.28162688f,1.27958854f,1.27754894f,1.27550809f,1.27346597f,1.27142257f,
1.26937788f,1.26733189f,1.26528459f,1.26323597f,1.26118602f,1.25913471f,1.25708205f,1.25502803f,
1.25297262f,1.25091583f,1.24885763f,1.24679802f,1.24473698f,1.24267450f,1.24061058f,1.23854519f,
1.23647833f,1.23440999f,1.23234015f,1.23026880f,1.22819593f,1.22612152f,1.22404557f,1.22196806f,
1.21988898f,1.21780832f,1.21572606f,1.21364219f,1.21155670f,1.20946958f,1.20738080f,1.20529037f,
1.20319826f,1.20110447f,1.19900898f,1.19691177f,1.19481283f,1.19271216f,1.19060973f,1.18850553f,
1.18639955f,1.18429178f,1.18218219f,1.18007079f,1.17795754f,1.17584244f,1.17372548f,1.17160663f,
1.16948589f,1.16736324f,1.16523866f,1.16311215f,1.16098368f,1.15885323f,1.15672081f,1.15458638f,
1.15244994f,1.15031147f,1.14817095f,1.14602836f,1.14388370f,1.14173695f,1.13958808f,1.13743709f,
1.13528396f,1.13312866f,1.13097119f,1.12881153f,1.12664966f,1.12448556f,1.12231921f,1.12015061f,
1.11797973f,1.11580656f,1.11363107f,1.11145325f,1.10927308f,1.10709055f,1.10490563f,1.10271831f,
1.10052856f,1.09833638f,1.09614174f,1.09394462f,1.09174500f,1.08954287f,1.08733820f,1.08513098f,
1.08292118f,1.08070879f,1.07849378f,1.07627614f,1.07405585f,1.07183287f,1.06960721f,1.06737882f,
1.06514770f,1.06291382f,1.06067715f,1.05843769f,1.05619540f,1.05395026f,1.05170226f,1.04945136f,
1.04719755f,1.04494080f,1.04268110f,1.04041841f,1.03815271f,1.03588399f,1.03361221f,1.03133735f,
1.02905939f,1.02677830f,1.02449407f,1.02220665f,1.01991603f,1.01762219f,1.01532509f,1.01302471f,
1.01072102f,1.00841400f,1.00610363f,1.00378986f,1.00147268f,0.99915206f,0.99682798f,0.99450039f,
0.99216928f,0.98983461f,0.98749636f,0.98515449f,0.98280898f,0.98045980f,0.97810691f,0.97575030f,
0.97338991f,0.97102573f,0.96865772f,0.96628585f,0.96391009f,0.96153040f,0.95914675f,0.95675912f,
0.95436745f,0.95197173f,0.94957191f,0.94716796f,0.94475985f,0.94234754f,0.93993099f,0.93751017f,
0.93508504f,0.93265556f,0.93022170f,0.92778341f,0.92534066f,0.92289341f,0.92044161f,0.91798524f,
0.91552424f,0.91305858f,0.91058821f,0.90811309f,0.90563319f,0.90314845f,0.90065884f,0.89816430f,
0.89566479f,0.89316028f,0.89065070f,0.88813602f,0.88561619f,0.88309116f,0.88056088f,0.87802531f,
0.87548438f,0.87293806f,0.87038629f,0.86782901f,0.86526619f,0.86269775f,0.86012366f,0.85754385f,
0.85495827f,0.85236686f,0.84976956f,0.84716633f,0.84455709f,0.84194179f,0.83932037f,0.83669277f,
0.83405893f,0.83141877f,0.82877225f,0.82611928f,0.82345981f,0.82079378f,0.81812110f,0.81544172f,
0.81275556f,0.81006255f,0.80736262f,0.80465570f,0.80194171f,0.79922057f,0.79649221f,0.79375655f,
0.79101352f,0.78826302f,0.78550497f,0.78273931f,0.77996593f,0.77718475f,0.77439569f,0.77159865f,
0.76879355f,0.76598029f,0.76315878f,0.76032891f,0.75749061f,0.75464376f,0.75178826f,0.74892402f,
0.74605092f,0.74316887f,0.74027775f,0.73737744f,0.73446785f,0.73154885f,0.72862033f,0.72568217f,
0.72273425f,0.71977644f,0.71680861f,0.71383064f,0.71084240f,0.70784376f,0.70483456f,0.70181469f,
0.69878398f,0.69574231f,0.69268952f,0.68962545f,0.68654996f,0.68346288f,0.68036406f,0.67725332f,
0.67413051f,0.67099544f,0.66784794f,0.66468783f,0.66151492f,0.65832903f,0.65512997f,0.65191753f,
0.64869151f,0.64545170f,0.64219789f,0.63892987f,0.63564741f,0.63235028f,0.62903824f,0.62571106f,
0.62236849f,0.61901027f,0.61563615f,0.61224585f,0.60883911f,0.60541564f,0.60197515f,0.59851735f,
0.59504192f,0.59154856f,0.58803694f,0.58450672f,0.58095756f,0.57738911f,0.57380101f,0.57019288f,
0.56656433f,0.56291496f,0.55924437f,0.55555212f,0.55183778f,0.54810089f,0.54434099f,0.54055758f,
0.53675018f,0.53291825f,0.52906127f,0.52517867f,0.52126988f,0.51733431f,0.51337132f,0.50938028f,
0.50536051f,0.50131132f,0.49723200f,0.49312177f,0.48897987f,0.48480547f,0.48059772f,0.47635573f,
0.47207859f,0.46776530f,0.46341487f,0.45902623f,0.45459827f,0.45012983f,0.44561967f,0.44106652f,
0.43646903f,0.43182577f,0.42713525f,0.42239588f,0.41760600f,0.41276385f,0.40786755f,0.40291513f,
0.39790449f,0.39283339f,0.38769946f,0.38250016f,0.37723277f,0.37189441f,0.36648196f,0.36099209f,
0.35542120f,0.34976542f,0.34402054f,0.33818204f,0.33224495f,0.32620390f,0.32005298f,0.31378574f,
0.30739505f,0.30087304f,0.29421096f,0.28739907f,0.28042645f,0.27328078f,0.26594810f,0.25841250f,
0.25065566f,0.24265636f,0.23438976f,0.22582651f,0.21693146f,0.20766198f,0.19796546f,0.18777575f,
0.17700769f,0.16554844f,0.15324301f,0.13986823f,0.12508152f,0.10830610f,0.08841715f,0.06251018f,
}

double acos( double x ) {
	int index;

	if (x < -1)
		x = -1;
	if (x > 1)
		x = 1;
	index = (float) (1.0f + x) * 511.9f;
	return acostable[index];
}

double atan2( double y, double x ) {
	float	base;
	float	temp;
	float	dir;
	float	test;
	int		i;

	if ( x < 0 ) {
		if ( y >= 0 ) {
			// quad 1
			base = M_PI / 2;
			temp = x;
			x = y;
			y = -temp;
		} else {
			// quad 2
			base = M_PI;
			x = -x;
			y = -y;
		}
	} else {
		if ( y < 0 ) {
			// quad 3
			base = 3 * M_PI / 2;
			temp = x;
			x = -y;
			y = temp;
		}
	}

	if ( y > x ) {
		base += M_PI/2;
		temp = x;
		x = y;
		y = temp;
		dir = -1;
	} else {
		dir = 1;
	}

	// calcualte angle in octant 0
	if ( x == 0 ) {
		return base;
	}
	y /= x;

	for ( i = 0 ; i < 512 ; i++ ) {
		test = sintable[i] / sintable[1023-i];
		if ( test > y ) {
			break;
		}
	}

	return base + dir * i * ( M_PI/2048); 
}


#endif

#ifdef Q3_VM
// bk001127 - guarded this tan replacement 
// ld: undefined versioned symbol name tan@@GLIBC_2.0
double tan( double x ) {
	return sin(x) / cos(x);
}
#endif


static int randSeed = 0;

void	srand( unsigned seed ) {
	randSeed = seed;
}

int		rand( void ) {
	randSeed = (69069 * randSeed + 1);
	return randSeed & 0x7fff;
}

double atof( const char *string ) {
	float sign;
	float value;
	int		c;


	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	c = string[0];
	if ( c != '.' ) {
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value = value * 10 + c;
		} while ( 1 );
	} else {
		string++;
	}

	// check for decimal point
	if ( c == '.' ) {
		double fraction;

		fraction = 0.1f;
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1f;
		} while ( 1 );

	}

	// not handling 10e10 notation...

	return value * sign;
}

double _atof( const char **stringPtr ) {
	const char	*string;
	float sign;
	float value;
	int		c = '0'; // bk001211 - uninitialized use possible

	string = *stringPtr;

	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			*stringPtr = string;
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	if ( string[0] != '.' ) {
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value = value * 10 + c;
		} while ( 1 );
	}

	// check for decimal point
	if ( c == '.' ) {
		double fraction;

		fraction = 0.1f;
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1f;
		} while ( 1 );

	}

	// not handling 10e10 notation...
	*stringPtr = string;

	return value * sign;
}


// bk001120 - presumably needed for Mac
//#if !defined ( _MSC_VER ) && ! defined ( __linux__ )

// bk001127 - undid undo
#if defined ( Q3_VM )
int atoi( const char *string ) {
	int		sign;
	int		value;
	int		c;


	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if ( c < '0' || c > '9' ) {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while ( 1 );

	// not handling 10e10 notation...

	return value * sign;
}


int _atoi( const char **stringPtr ) {
	int		sign;
	int		value;
	int		c;
	const char	*string;

	string = *stringPtr;

	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if ( c < '0' || c > '9' ) {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while ( 1 );

	// not handling 10e10 notation...

	*stringPtr = string;

	return value * sign;
}

int abs( int n ) {
	return n < 0 ? -n : n;
}

double fabs( double x ) {
	return x < 0 ? -x : x;
}



//=========================================================


#define ALT			0x00000001		/* alternate form */
#define HEXPREFIX	0x00000002		/* add 0x or 0X prefix */
#define LADJUST		0x00000004		/* left adjustment */
#define LONGDBL		0x00000008		/* long double */
#define LONGINT		0x00000010		/* long integer */
#define QUADINT		0x00000020		/* quad integer */
#define SHORTINT	0x00000040		/* short integer */
#define ZEROPAD		0x00000080		/* zero (as opposed to blank) pad */
#define FPT			0x00000100		/* floating point number */

#define to_digit(c)		((c) - '0')
#define is_digit(c)		((unsigned)to_digit(c) <= 9)
#define to_char(n)		((n) + '0')

void AddInt( char **buf_p, int val, int width, int flags ) {
	char	text[32];
	int		digits;
	int		signedVal;
	char	*buf;

	digits = 0;
	signedVal = val;
	if ( val < 0 ) {
		val = -val;
	}
	do {
		text[digits++] = '0' + val % 10;
		val /= 10;
	} while ( val );

	if ( signedVal < 0 ) {
		text[digits++] = '-';
	}

	buf = *buf_p;

	if( !( flags & LADJUST ) ) {
		while ( digits < width ) {
			*buf++ = ( flags & ZEROPAD ) ? '0' : ' ';
			width--;
		}
	}

	while ( digits-- ) {
		*buf++ = text[digits];
		width--;
	}

	if( flags & LADJUST ) {
		while ( width-- ) {
			*buf++ = ( flags & ZEROPAD ) ? '0' : ' ';
		}
	}

	*buf_p = buf;
}

void AddFloat( char **buf_p, float fval, int width, int prec ) {
	char	text[32];
	int		digits;
	float	signedVal;
	char	*buf;
	int		val;

	// get the sign
	signedVal = fval;
	if ( fval < 0 ) {
		fval = -fval;
	}

	// write the float number
	digits = 0;
	val = (int)fval;
	do {
		text[digits++] = '0' + val % 10;
		val /= 10;
	} while ( val );

	if ( signedVal < 0 ) {
		text[digits++] = '-';
	}

	buf = *buf_p;

	while ( digits < width ) {
		*buf++ = ' ';
		width--;
	}

	while ( digits-- ) {
		*buf++ = text[digits];
	}

	*buf_p = buf;

	if (prec < 0)
		prec = 6;
	// write the fraction
	digits = 0;
	while (digits < prec) {
		fval -= (int) fval;
		fval *= 10.0f;
		val = (int) fval;
		text[digits++] = '0' + val % 10;
	}

	if (digits > 0) {
		buf = *buf_p;
		*buf++ = '.';
		for (prec = 0; prec < digits; prec++) {
			*buf++ = text[prec];
		}
		*buf_p = buf;
	}
}


void AddString( char **buf_p, char *string, int width, int prec ) {
	int		size;
	char	*buf;

	buf = *buf_p;

	if ( string == NULL ) {
		string = "(null)";
		prec = -1;
	}

	if ( prec >= 0 ) {
		for( size = 0; size < prec; size++ ) {
			if( string[size] == '\0' ) {
				break;
			}
		}
	}
	else {
		size = strlen( string );
	}

	width -= size;

	while( size-- ) {
		*buf++ = *string++;
	}

	while( width-- > 0 ) {
		*buf++ = ' ';
	}

	*buf_p = buf;
}

/*
vsprintf

I'm not going to support a bunch of the more arcane stuff in here
just to keep it simpler.  For example, the '*' and '$' are not
currently supported.  I've tried to make it so that it will just
parse and ignore formats we don't support.
*/
int vsprintf( char *buffer, const char *fmt, va_list argptr ) {
	int		*arg;
	char	*buf_p;
	char	ch;
	int		flags;
	int		width;
	int		prec;
	int		n;
	char	sign;

	buf_p = buffer;
	arg = (int *)argptr;

	while( qtrue ) {
		// run through the format string until we hit a '%' or '\0'
		for ( ch = *fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++ ) {
			*buf_p++ = ch;
		}
		if ( ch == '\0' ) {
			goto done;
		}

		// skip over the '%'
		fmt++;

		// reset formatting state
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch( ch ) {
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '.':
			n = 0;
			while( is_digit( ( ch = *fmt++ ) ) ) {
				n = 10 * n + ( ch - '0' );
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			flags |= ZEROPAD;
			goto rflag;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			n = 0;
			do {
				n = 10 * n + ( ch - '0' );
				ch = *fmt++;
			} while( is_digit( ch ) );
			width = n;
			goto reswitch;
		case 'c':
			*buf_p++ = (char)*arg;
			arg++;
			break;
		case 'd':
		case 'i':
			AddInt( &buf_p, *arg, width, flags );
			arg++;
			break;
		case 'f':
			AddFloat( &buf_p, *(double *)arg, width, prec );
#ifdef __LCC__
			arg += 1;	// everything is 32 bit in my compiler
#else
			arg += 2;
#endif
			break;
		case 's':
			AddString( &buf_p, (char *)*arg, width, prec );
			arg++;
			break;
		case '%':
			*buf_p++ = ch;
			break;
		default:
			*buf_p++ = (char)*arg;
			arg++;
			break;
		}
	}

done:
	*buf_p = 0;
	return buf_p - buffer;
}

/* this is really crappy */
int sscanf( const char *buffer, const char *fmt, ... ) {
	int		cmd;
	int		**arg;
	int		count;

	arg = (int **)&fmt + 1;
	count = 0;

	while ( *fmt ) {
		if ( fmt[0] != '%' ) {
			fmt++;
			continue;
		}

		cmd = fmt[1];
		fmt += 2;

		switch ( cmd ) {
		case 'i':
		case 'd':
		case 'u':
			**arg = _atoi( &buffer );
			break;
		case 'f':
			*(float *)*arg = _atof( &buffer );
			break;
		}
		arg++;
	}

	return count;
}

#endif
