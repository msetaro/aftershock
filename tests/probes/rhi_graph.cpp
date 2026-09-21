// Graph declarations must retain dependencies and target lifetimes without aliasing.
#include "../../engine/rhi/rhi_public.h"
#include <assert.h>
#include <initializer_list>

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
	assert( graph.targets[target( rhiGraphTarget_t::ScreenColor )].lastUse == pass( rhiGraphPass_t::PostBloom ) );
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
	// New depth-only shadow atlases are explicit graph resources. Their writers
	// precede every scene consumer even though public legacy pass IDs stay fixed.
	config.shadowSize = 2048;
	assert( RHI_CompileGraph( &config, &graph ) );
	assert( graph.targetCount == 18 && graph.passCount == 18 );
	for ( const auto id : { rhiGraphPass_t::MainResume, rhiGraphPass_t::ScreenResume } ) {
		const auto &node = graph.passes[pass( id )];
		assert( node.enabled && node.depth == 1 );
		assert( node.dependencies[0].sourceStage == rhiGraphStage_t::SceneAttachments );
		assert( node.dependencies[0].sourceAccess & RHI_GRAPH_DEPTH_WRITE );
		assert( node.dependencies[0].destinationAccess & RHI_GRAPH_DEPTH_READ );
		for ( uint32_t i = 0; i < node.attachmentCount; ++i ) {
			assert( node.attachments[i].load == rhiGraphLoad_t::Load );
			assert( node.attachments[i].store == rhiGraphStore_t::Store );
			assert( !graph.targets[target( node.attachments[i].target )].transient );
		}
	}
	const rhiGraphTarget_t shadowTargets[] = { rhiGraphTarget_t::LocalShadow, rhiGraphTarget_t::SunShadow };
	const rhiGraphPass_t shadowPasses[] = { rhiGraphPass_t::LocalShadow, rhiGraphPass_t::SunShadow };
	for ( int i = 0; i < 2; ++i ) {
		const auto &resource = graph.targets[target( shadowTargets[i] )];
		const auto &node = graph.passes[pass( shadowPasses[i] )];
		assert( resource.width == 2048 && resource.height == 2048 && resource.samples == 1 );
		assert( resource.usage == ( RHI_GRAPH_DEPTH | RHI_GRAPH_SAMPLED ) && !resource.transient );
		assert( resource.format == rhiGraphFormat_t::ShadowDepth );
		assert( node.color == RHI_INVALID_OFFSET && node.depth == 0 && node.attachmentCount == 1 );
		assert( node.attachments[0].load == rhiGraphLoad_t::Clear && node.attachments[0].store == rhiGraphStore_t::Store );
		assert( node.attachments[0].finalLayout == rhiGraphLayout_t::DepthSampled );
		assert( node.dependencies[1].sourceAccess & RHI_GRAPH_DEPTH_WRITE );
		assert( node.dependencies[1].destinationAccess & RHI_GRAPH_SHADER_READ );
		assert( graph.executionOrder[i] == shadowPasses[i] );
		assert( graph.passes[pass( rhiGraphPass_t::Main )].dependencyMask & ( UINT32_C( 1 ) << pass( shadowPasses[i] ) ) );
		assert( resource.firstUse < graph.targets[target( rhiGraphTarget_t::MainColor )].firstUse );
	}
	config.shadowSize = 17;
	assert( !RHI_CompileGraph( &config, &graph ) );
	config.shadowSize = 0;
	config.renderWidth = 0;
	assert( !RHI_CompileGraph( &config, &graph ) );
}
