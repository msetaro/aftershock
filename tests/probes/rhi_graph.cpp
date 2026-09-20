// Graph declarations must retain dependencies and target lifetimes without aliasing.
#include "../../engine/rhi/rhi_public.h"
#include <assert.h>

static uint32_t target( rhiGraphTarget_t value ) {
	return (uint32_t)value;
}
static uint32_t pass( rhiGraphPass_t value ) {
	return (uint32_t)value;
}
static uint32_t bit( rhiGraphTarget_t value ) {
	return UINT32_C( 1 ) << target( value );
}

int main( void ) {
	static_assert( std::is_trivially_copyable_v<rhiGraph_t> );
	rhiGraphConfig_t config = {};
	config.renderWidth = 640;
	config.renderHeight = 360;
	config.windowWidth = config.captureWidth = 640;
	config.windowHeight = config.captureHeight = 360;
	config.screenWidth = 160;
	config.screenHeight = 90;
	config.samples = config.screenSamples = 1;
	rhiGraph_t graph;
	assert( RHI_CompileGraph( &config, &graph ) );
	assert( graph.targetCount == 1 && graph.passCount == 1 );
	assert( graph.targets[target( rhiGraphTarget_t::Present )].imported );
	assert( graph.targetOrder[0] == rhiGraphTarget_t::MainDepth );
	assert( graph.passOrder[0] == rhiGraphPass_t::Main );
	assert( graph.passes[pass( rhiGraphPass_t::Main )].writeMask & bit( rhiGraphTarget_t::Present ) );

	config.bloom = config.capture = true; // Inactive post-process flags remain legal in direct mode.
	assert( RHI_CompileGraph( &config, &graph ) );
	assert( graph.targetCount == 1 && graph.passCount == 1 );
	config.bloom = config.capture = false;
	config.offscreen = true;
	config.renderWidth = 1280;
	config.renderHeight = 720;
	assert( RHI_CompileGraph( &config, &graph ) );
	assert( graph.targetCount == 4 && graph.passCount == 3 );
	assert( graph.targets[target( rhiGraphTarget_t::ScreenColor )].persistent );
	assert( graph.targets[target( rhiGraphTarget_t::MainColor )].exported );
	assert( graph.passes[pass( rhiGraphPass_t::Main )].readMask & bit( rhiGraphTarget_t::ScreenColor ) );
	assert( graph.targets[target( rhiGraphTarget_t::MainDepth )].lastUse == pass( rhiGraphPass_t::Main ) );

	config.bloom = config.capture = config.stencil = true;
	config.samples = 4;
	config.screenSamples = 2;
	assert( RHI_CompileGraph( &config, &graph ) );
	assert( graph.targetCount == 16 && graph.passCount == 14 );
	assert( graph.targetOrder[0] == rhiGraphTarget_t::Bloom0 );
	assert( graph.targetOrder[9] == rhiGraphTarget_t::MainColor );
	assert( graph.targetOrder[15] == rhiGraphTarget_t::MainDepth );
	assert( graph.passOrder[0] == rhiGraphPass_t::Main );
	assert( graph.passOrder[1] == rhiGraphPass_t::PostBloom );
	assert( graph.passOrder[13] == rhiGraphPass_t::ScreenMap );
	assert( graph.targets[target( rhiGraphTarget_t::Bloom0 )].width == 640 );
	assert( graph.targets[target( rhiGraphTarget_t::Bloom8 )].width == 40 );
	assert( graph.targets[target( rhiGraphTarget_t::Bloom8 )].height == 22 );
	assert( graph.targets[target( rhiGraphTarget_t::MainDepth )].lastUse == pass( rhiGraphPass_t::PostBloom ) );
	assert( graph.targets[target( rhiGraphTarget_t::MainMsaa )].lastUse == pass( rhiGraphPass_t::PostBloom ) );
	assert( graph.targets[target( rhiGraphTarget_t::MainColor )].lastUse == pass( rhiGraphPass_t::Gamma ) );
	assert( graph.targets[target( rhiGraphTarget_t::Capture )].exported );
	assert( graph.targets[target( rhiGraphTarget_t::ScreenColor )].persistent );
	assert( graph.passes[pass( rhiGraphPass_t::PostBloom )].dependencyMask & ( UINT32_C( 1 ) << pass( rhiGraphPass_t::Main ) ) );
	for ( uint32_t i = 0; i < graph.passCount; ++i ) {
		const auto id = graph.passOrder[i];
		const auto &node = graph.passes[pass( id )];
		assert( node.enabled && node.attachmentCount > 0 && node.attachmentCount <= 3 );
		// Every dependency is a prior pass in execution order, not creation order.
		assert( !( node.dependencyMask >> pass( id ) ) );
		for ( uint32_t r = 0; r < (uint32_t)rhiGraphTarget_t::Count; ++r ) {
			if ( ( node.readMask | node.writeMask ) & ( UINT32_C( 1 ) << r ) ) {
				const auto &resource = graph.targets[r];
				assert( resource.enabled && resource.firstUse <= pass( id ) && resource.lastUse >= pass( id ) );
			}
		}
	}
	config.renderWidth = 0;
	assert( !RHI_CompileGraph( &config, &graph ) );
}
