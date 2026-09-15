/* Compile each imported translation unit in its module's namespace. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include "../engine/public/g_native_public.h"
#include "../engine/public/cg_native_public.h"
#include "../engine/public/ui_native_public.h"
#include "bg/native_abi_public.h"

#define NATIVE_JOIN_INNER(a,b) a##b
#define NATIVE_JOIN(a,b) NATIVE_JOIN_INNER(a,b)
#define vmMain NATIVE_JOIN(NATIVE_NAMESPACE,_vmMain)
#define dllEntry NATIVE_JOIN(NATIVE_NAMESPACE,_dllEntry)

namespace NATIVE_NAMESPACE {
void qsort( void *, size_t, size_t, int (*)( const void *, const void * ) );
void srand( unsigned );
int rand( void );
double atof( const char * );
void *memmove( void *, const void *, size_t );
#include NATIVE_SOURCE
}

#ifdef NATIVE_EXPORTS
#include NATIVE_EXPORTS
#endif
