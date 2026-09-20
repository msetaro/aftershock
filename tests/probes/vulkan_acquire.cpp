// Stop the real frame path at command recording; no GPU or window is created.
#include "../../engine/renderervk/vk.cpp"
#include "../../engine/renderervk/tr_init.cpp"

backEndState_t backEnd;
shaderCommands_t tess;
trGlobals_t tr;
backEndData_t *backEndData;
refimport_t ri;

static VkResult acquisition;

void QDECL Com_Error( errorParm_t, const char *, ... ) {
	abort();
}

void QDECL Com_Printf( const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
}

static qboolean not_minimized( void ) {
	return qfalse;
}

static VkResult VKAPI_CALL acquire( VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t *index ) {
	if ( acquisition == VK_SUCCESS || acquisition == VK_SUBOPTIMAL_KHR )
		*index = 3;
	return acquisition;
}

static VkResult VKAPI_CALL begin_commands( VkCommandBuffer, const VkCommandBufferBeginInfo * ) {
	if ( acquisition != VK_SUCCESS && acquisition != VK_SUBOPTIMAL_KHR ) {
		puts( "FAIL: command recording reached without acquiring an image" );
		exit( 1 );
	}
	exit( vk.cmd->swapchain_image_acquired && vk.cmd->swapchain_image_index == 3 ? 0 : 1 );
}

static void NORETURN QDECL frame_error( errorParm_t, const char *format, ... ) {
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	puts( "" );
	exit( !vk.cmd->swapchain_image_acquired && ( acquisition == VK_TIMEOUT || acquisition == VK_NOT_READY ) ? 42 : 1 );
}

int main( int argc, char **argv ) {
	if ( argc != 2 )
		return 2;
	acquisition = (VkResult)atoi( argv[1] );
	ri.CL_IsMinimized = not_minimized;
	ri.Error = frame_error;
	qvkAcquireNextImageKHR = acquire;
	qvkBeginCommandBuffer = begin_commands;
	vk_begin_frame();
	return 1;
}
