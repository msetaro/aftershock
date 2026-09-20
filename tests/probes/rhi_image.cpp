// Texture conversion is frontend scratch; release it before reporting GPU errors.
#include "../../engine/render/tr_image.cpp"
#include <assert.h>

refimport_t ri;
static uint16_t scratch[4];
static bool allocated;
static int checked;
static int uploads;
static rhiStatus_t upload_status;
static byte pixels[] = { 255, 0, 128, 64, 16, 32, 48, 255 };
static const byte expected[][8] = {
	{ 255, 0, 128, 64, 16, 32, 48, 255 },
	{ 128, 0, 255, 64, 48, 32, 16, 255 },
	{ 255, 0, 128, 16, 32, 48 },
	{ 0xf4, 0x80, 0x1f, 0x32 },
	{ 0x10, 0xfc, 0x86, 0x88 }
};
static const int widths[] = { 4, 4, 3, 2, 2 };

static void *allocate( size_t size ) {
	assert( !allocated && size <= sizeof( scratch ) );
	allocated = true;
	return scratch;
}
static void release( void *pointer ) {
	assert( allocated && pointer == scratch );
	allocated = false;
}
void R_CheckRHI( rhiStatus_t status, const char *operation ) {
	assert( !allocated && status == upload_status );
	assert( strcmp( operation, "UploadTexture" ) == 0 );
	checked++;
}
rhiStatus_t RHI_UploadTexture( const rhiTexture_t *, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mips, const uint8_t *data, int32_t bytesPerPixel, bool update ) {
	assert( x == 0 && y == 0 && width == 2 && height == 1 && mips == 1 && !update );
	const int format = uploads % 5;
	assert( bytesPerPixel == widths[format] );
	assert( allocated == ( format != 0 ) );
	assert( memcmp( data, expected[format], 2 * bytesPerPixel ) == 0 );
	uploads++;
	return upload_status;
}
int main( void ) {
	ri.Hunk_AllocateTempMemory = allocate;
	ri.Hunk_FreeTempMemory = release;
	const rhiStatus_t statuses[] = { rhiStatus_t::Success, rhiStatus_t::DeviceLost };
	for ( const rhiStatus_t status : statuses ) {
		upload_status = status;
		for ( int format = 0; format < 5; format++ ) {
			R_UploadTexture( nullptr, (rhiFormat_t)format, 0, 0, 2, 1, 1, pixels, sizeof( pixels ), false );
		}
	}
	assert( checked == 10 && uploads == 10 );
	return 0;
}
