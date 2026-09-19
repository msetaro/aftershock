/* Native compatibility with the QVM's binary32 arithmetic and module interface. */
#ifndef NATIVE_ABI_H
#define NATIVE_ABI_H

#include <stdint.h>
#include <math.h>

/* Use q_shared.h's binary32 constant on every compiler. */
#undef M_PI

#define BASEGAME "baseq3"
#define COM_TRAP_GETVALUE 700

#ifdef __cplusplus
#define Q_EXTERN_C extern "C"
#else
#define Q_EXTERN_C
#endif

/* The engine's VM math calls evaluate in host double and return one float word. */
#define sin( x ) ((float)sin((double)(x)))
#define cos( x ) ((float)cos((double)(x)))
#define tan( x ) ((float)tan((double)(x)))
#define asin( x ) ((float)asin((double)(x)))
#define acos( x ) ((float)acos((double)(x)))
#define atan( x ) ((float)atan((double)(x)))
#define atan2( y, x ) ((float)atan2((double)(y),(double)(x)))
#define sqrt( x ) ((float)sqrt((double)(x)))
#define floor( x ) ((float)floor((double)(x)))
#define ceil( x ) ((float)ceil((double)(x)))
#define fabs( x ) ((float)fabs((double)(x)))
#define pow( x, y ) ((float)pow((double)(x),(double)(y)))

/* Transitional DLL hosts read their maximum word count for every call. */
#define NATIVE_ZEROS (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0
#ifdef QAGAME
#define NATIVE_SYSCALL( ... ) (syscall)(__VA_ARGS__, NATIVE_ZEROS, (intptr_t)0, (intptr_t)0, (intptr_t)0, (intptr_t)0)
#else
#define NATIVE_SYSCALL( ... ) (syscall)(__VA_ARGS__, NATIVE_ZEROS)
#endif

#endif
