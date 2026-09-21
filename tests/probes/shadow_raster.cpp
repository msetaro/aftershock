// The shadow atlas can exceed the window in both dimensions.
#include "../../engine/render/tr_backend.cpp"
#include <assert.h>

glconfig_t glConfig;
static rhiRenderArea_t area;
rhiRenderArea_t RHI_GetRenderArea( void ) {
	return area;
}

int main() {
	glConfig.vidWidth = 640;
	glConfig.vidHeight = 480;
	for ( int grid = 2; grid <= 4; grid += 2 ) {
		area = { 2048, 2048, 1, 1 };
		const int size = 2048 / grid;
		backEnd.viewParms.shadowView = grid == 4 ? 1 : 2;
		for ( int tile = 0; tile < grid * grid; ++tile ) {
			auto &view = backEnd.viewParms;
			view.viewportX = ( tile % grid ) * size;
			view.viewportY = ( grid - 1 - tile / grid ) * size;
			view.viewportWidth = view.viewportHeight = size;
			rhiRasterState_t raster = {};
			RB_GetRaster( DEPTH_RANGE_NORMAL, &raster );
			assert( raster.viewport.x == (tile % grid) * size );
			assert( raster.viewport.y == (tile / grid) * size );
			assert( raster.scissor.offset.x == raster.viewport.x );
			assert( raster.scissor.offset.y == raster.viewport.y );
			assert( raster.scissor.extent.width == (uint32_t)size && raster.scissor.extent.height == (uint32_t)size );
			assert( raster.viewport.minDepth == 0 && raster.viewport.maxDepth == 1 );
		}
	}
	puts( "PASS: all local/sun atlas tiles retain full scissors beyond the window" );
}
