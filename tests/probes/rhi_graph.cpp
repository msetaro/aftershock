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
	// SSAO samples retained scene depth, filters separately, then resumes the
	// scene with load operations. Exercise MSAA, stencil, bloom and shadows.
	for ( const uint32_t scale : { 1u, 2u } ) {
		for ( const uint32_t samples : { 1u, 4u } ) {
			for ( const bool bloom : { false, true } ) {
				for ( const uint32_t shadows : { 0u, 2048u } ) {
					config.occlusionScale = scale;
					config.samples = samples;
					config.bloom = bloom;
					config.shadowSize = shadows;
					assert( RHI_CompileGraph( &config, &graph ) );
					const auto &depth = graph.targets[target( rhiGraphTarget_t::MainDepth )];
					assert( !depth.transient && depth.samples == samples );
					assert( depth.usage & RHI_GRAPH_SAMPLED );
					assert( depth.initialLayout == rhiGraphLayout_t::DepthSampled );
					for ( const auto id : { rhiGraphTarget_t::Occlusion, rhiGraphTarget_t::OcclusionBlur } ) {
						const auto &image = graph.targets[target( id )];
						assert( image.width == config.renderWidth / scale && image.height == config.renderHeight / scale );
						assert( image.samples == 1 && image.usage == ( RHI_GRAPH_COLOR | RHI_GRAPH_SAMPLED ) );
					}
					const auto &ao = graph.passes[pass( rhiGraphPass_t::Occlusion )];
					const auto &blur = graph.passes[pass( rhiGraphPass_t::OcclusionBlur )];
					const auto &apply = graph.passes[pass( rhiGraphPass_t::OcclusionApply )];
					assert( ao.readMask & bit( rhiGraphTarget_t::MainDepth ) );
					assert( blur.readMask & bit( rhiGraphTarget_t::MainDepth ) );
					assert( blur.dependencyMask & ( 1u << pass( rhiGraphPass_t::Occlusion ) ) );
					assert( apply.dependencyMask & ( 1u << pass( rhiGraphPass_t::OcclusionBlur ) ) );
					assert( apply.readMask & bit( rhiGraphTarget_t::OcclusionBlur ) );
					assert( apply.depth == 1 && apply.attachmentCount == ( samples > 1 ? 3u : 2u ) );
					for ( uint32_t i = 0; i < apply.attachmentCount; ++i ) {
						assert( apply.attachments[i].load == rhiGraphLoad_t::Load );
						assert( apply.attachments[i].store == rhiGraphStore_t::Store );
					}
					const auto &main = graph.passes[pass( rhiGraphPass_t::Main )];
					assert( main.dependencies[1].sourceAccess & RHI_GRAPH_DEPTH_WRITE );
					assert( main.dependencies[1].destinationAccess & RHI_GRAPH_SHADER_READ );
					uint32_t seen = 0;
					for ( uint32_t i = 0; i < graph.executionCount; ++i ) {
						const auto id = graph.executionOrder[i];
						const auto &node = graph.passes[pass( id )];
						if ( !node.enabled )
							continue;
						assert( !( node.dependencyMask & ~seen ) );
						seen |= 1u << pass( id );
					}
					if ( bloom )
						assert( graph.passes[pass( rhiGraphPass_t::BloomExtract )].dependencyMask & ( 1u << pass( rhiGraphPass_t::OcclusionApply ) ) );
				}
			}
		}
	}
	// Soft particles sample depth while it is detached, then resume every scene
	// attachment with load operations, including multisample color and stencil.
	for ( uint32_t samples : { 1u, 4u } ) {
		for ( uint32_t ao : { 0u, 2u } ) {
			config.samples = samples;
			config.occlusionScale = ao;
			config.softParticles = true;
			assert( RHI_CompileGraph( &config, &graph ) );
			const auto &depth = graph.targets[target( rhiGraphTarget_t::MainDepth )];
			assert( !depth.transient && ( depth.usage & RHI_GRAPH_SAMPLED ) );
			const auto &fx = graph.passes[pass( rhiGraphPass_t::Particles )];
			const auto &resume = graph.passes[pass( rhiGraphPass_t::ParticlesResume )];
			assert( fx.enabled && fx.depth == RHI_INVALID_OFFSET );
			assert( fx.readMask & bit( rhiGraphTarget_t::MainDepth ) );
			assert( !( fx.writeMask & bit( rhiGraphTarget_t::MainDepth ) ) );
			assert( fx.attachmentCount == ( samples > 1 ? 2u : 1u ) );
			assert( resume.enabled && resume.depth == 1 );
			assert( resume.dependencyMask & ( 1u << pass( rhiGraphPass_t::Particles ) ) );
			for ( uint32_t i = 0; i < resume.attachmentCount; ++i ) {
				assert( resume.attachments[i].load == rhiGraphLoad_t::Load );
				assert( resume.attachments[i].store == rhiGraphStore_t::Store );
			}
			uint32_t seen = 0;
			for ( uint32_t i = 0; i < graph.executionCount; ++i ) {
				const auto id = graph.executionOrder[i];
				const auto &node = graph.passes[pass( id )];
				if ( !node.enabled ) continue;
				assert( !( node.dependencyMask & ~seen ) );
				seen |= 1u << pass( id );
			}
		}
	}
	config.offscreen = false;
	assert( !RHI_CompileGraph( &config, &graph ) );
	config.offscreen = true;
	config.softParticles = false;
	config.occlusionScale = 3;
	assert( !RHI_CompileGraph( &config, &graph ) );
	config.occlusionScale = 0;
	config.shadowSize = 17;
	assert( !RHI_CompileGraph( &config, &graph ) );
	config.shadowSize = 0;
	config.renderWidth = 0;
	assert( !RHI_CompileGraph( &config, &graph ) );
}
