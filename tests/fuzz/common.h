/* Test-only Com_Error boundary: every object inside the jump is trivial. */
#include "../../code/qcommon/q_shared.h"
#include "../../code/qcommon/qcommon.h"
#include <setjmp.h>

static jmp_buf fuzz_error;
qboolean com_errorEntered;
void QDECL Com_Error( errorParm_t level, const char *fmt, ... )
{
	(void)level;
	(void)fmt;
	longjmp( fuzz_error, 1 );
}
void QDECL Com_Printf( const char *fmt, ... ) { (void)fmt; }
void QDECL Com_DPrintf( const char *fmt, ... ) { (void)fmt; }
