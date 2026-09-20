// Exercise the production upload allocator without creating a GPU device.
#include "../../engine/renderervk/vk.cpp"
#include <assert.h>

Vk_Instance vk;
Vk_World vk_world;

static int destroyed;
static void VKAPI_CALL destroy_image( VkDevice, VkImage image, const VkAllocationCallbacks * ) {
	assert( (uintptr_t)image == 11 && destroyed == 0 );
	destroyed = 1;
}
static void VKAPI_CALL destroy_view( VkDevice, VkImageView view, const VkAllocationCallbacks * ) {
	assert( (uintptr_t)view == 12 && destroyed == 1 );
	destroyed = 2;
}

int main( void ) {
	byte storage[512];
	byte uniform[128];
	memset( storage, 0xa5, sizeof( storage ) );
	memset( uniform, 0x3c, sizeof( uniform ) );
	assert( !RHI_Available() );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == RHI_INVALID_OFFSET );
	vk.cmd = &vk.tess[0];
	vk.cmd->vertex_buffer_ptr = storage;
	vk.cmd->vertex_buffer_offset = 1;
	vk.cmd->uniform_descriptor = (VkDescriptorSet)(uintptr_t)1;
	vk.uniform_alignment = 256;
	vk.uniform_item_size = 128;
	vk.geometry_buffer_size = sizeof( storage );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == 256 );
	assert( memcmp( storage + 256, uniform, sizeof( uniform ) ) == 0 );
	assert( storage[255] == 0xa5 && storage[384] == 0xa5 );
	assert( vk.cmd->vertex_buffer_offset == 384 );
	assert( vk.cmd->descriptor_set.offset[0] == 256 );
	assert( vk.cmd->descriptor_set.current[0] == vk.cmd->uniform_descriptor );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == RHI_INVALID_OFFSET );
	assert( vk.cmd->vertex_buffer_offset == 384 );
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) + 1 ) == RHI_INVALID_OFFSET );
	assert( RHI_UploadUniform( nullptr, sizeof( uniform ) ) == RHI_INVALID_OFFSET );
	// A different frame slot has independent upload space and binding offsets.
	vk.cmd = &vk.tess[1];
	vk.cmd->vertex_buffer_ptr = storage;
	assert( RHI_UploadUniform( uniform, sizeof( uniform ) ) == 0 );
	assert( vk.tess[0].vertex_buffer_offset == 384 );
	assert( vk.tess[1].vertex_buffer_offset == 128 );
	const rhiStats_t stats = RHI_GetStats();
	assert( stats.geometryBytes == sizeof( storage ) && stats.frameSlots == 2 );
	assert( vk_texture_format( rhiFormat_t::RGBA8 ) == VK_FORMAT_R8G8B8A8_UNORM );
	assert( vk_texture_format( rhiFormat_t::BGRA8 ) == VK_FORMAT_B8G8R8A8_UNORM );
	assert( vk_texture_format( rhiFormat_t::RGB8 ) == VK_FORMAT_R8G8B8_UNORM );
	assert( vk_texture_format( rhiFormat_t::BGRA4 ) == VK_FORMAT_B4G4R4A4_UNORM_PACK16 );
	assert( vk_texture_format( rhiFormat_t::A1RGB5 ) == VK_FORMAT_A1R5G5B5_UNORM_PACK16 );
	assert( vk_texture_address( rhiAddress_t::Repeat ) == VK_SAMPLER_ADDRESS_MODE_REPEAT );
	assert( vk_texture_address( rhiAddress_t::ClampToEdge ) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
	assert( vk_texture_address( rhiAddress_t::ClampToBorder ) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );
	rhiTexture_t texture = { 11, 12, 13 };
	RHI_BindTexture( 3, &texture );
	assert( (uintptr_t)vk.cmd->descriptor_set.current[3] == 13 );
	qvkDestroyImage = destroy_image;
	qvkDestroyImageView = destroy_view;
	RHI_DestroyTexture( &texture );
	assert( destroyed == 2 && texture.image == 0 && texture.view == 0 && texture.binding == 13 );
	RHI_DestroyTexture( &texture );
	assert( destroyed == 2 );
	puts( "PASS: RHI uniform alignment/bytes/bindings/exhaustion/frame slots; texture formats/addressing/binding/destruction" );
	return 0;
}
