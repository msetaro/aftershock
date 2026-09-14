#include "files.h"
#include "../../code/renderercommon/tr_public.h"

refimport_t ri;
void R_LoadTGA( const char *name, byte **pic, int *width, int *height );
void R_LoadPNG( const char *name, byte **pic, int *width, int *height );
void CL_LoadJPG( const char *name, byte **pic, int *width, int *height );
static void QDECL Print( printParm_t level, const char *fmt, ... ) { (void)level; (void)fmt; }

extern "C" int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
	byte *pic = NULL;
	int width = 0, height = 0;
	if ( size > 1024 * 1024 ) return 0;
	file_data = data; file_size = size;
	ri.Malloc = Z_Malloc; ri.Free = Z_Free;
	ri.FS_ReadFile = FS_ReadFile; ri.FS_FreeFile = FS_FreeFile;
	ri.Printf = Print; ri.Error = Com_Error;
	if ( !setjmp( fuzz_error ) ) {
#if defined(FUZZ_TGA)
		R_LoadTGA( "fuzz.tga", &pic, &width, &height );
#elif defined(FUZZ_PNG)
		R_LoadPNG( "fuzz.png", &pic, &width, &height );
#elif defined(FUZZ_JPEG)
		CL_LoadJPG( "fuzz.jpg", &pic, &width, &height );
#endif
	}
	ResetAllocations();
	return 0;
}
