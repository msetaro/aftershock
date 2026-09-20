// Exercise the production upload allocator without creating a GPU device.
#include "../../engine/renderervk/vk.cpp"
#include <assert.h>

Vk_Instance vk;
Vk_World vk_world;

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
	puts( "PASS: RHI uniform alignment, bytes, bindings, exhaustion and independent frame slots" );
	return 0;
}
