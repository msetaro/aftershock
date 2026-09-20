#include "rhi_public.h"

static uint32_t TargetBit( rhiGraphTarget_t target ) {
	return UINT32_C( 1 ) << (uint32_t)target;
}

static rhiGraphTargetDesc_t &Target( rhiGraph_t *graph, rhiGraphTarget_t id,
	uint32_t width, uint32_t height, uint32_t samples, rhiGraphFormat_t format,
	uint32_t usage, rhiGraphLayout_t layout, bool transient = false ) {
	rhiGraphTargetDesc_t &target = graph->targets[(uint32_t)id];
	target.width = width;
	target.height = height;
	target.samples = samples;
	target.format = format;
	target.usage = usage;
	target.initialLayout = layout;
	target.transient = transient;
	target.enabled = true;
	target.firstUse = RHI_INVALID_OFFSET;
	target.imported = id == rhiGraphTarget_t::Present;
	if ( !target.imported )
		graph->targetOrder[graph->targetCount++] = id;
	return target;
}

static rhiGraphPassDesc_t &Pass( rhiGraph_t *graph, rhiGraphPass_t id,
	uint32_t width, uint32_t height, uint32_t reads, bool presentation = false ) {
	rhiGraphPassDesc_t &pass = graph->passes[(uint32_t)id];
	pass.enabled = true;
	pass.width = width;
	pass.height = height;
	pass.readMask = reads;
	pass.depth = pass.resolve = RHI_INVALID_OFFSET;
	graph->passOrder[graph->passCount++] = id;
	if ( presentation ) {
		pass.dependencyCount = 1;
		pass.dependencies[0] = { rhiGraphStage_t::ColorOutput, rhiGraphStage_t::ColorOutput,
			RHI_GRAPH_COLOR_READ, RHI_GRAPH_COLOR_WRITE, true, false };
	} else {
		pass.dependencyCount = 2;
		pass.dependencies[0] = { rhiGraphStage_t::Fragment, rhiGraphStage_t::ColorOutput,
			RHI_GRAPH_SHADER_READ, RHI_GRAPH_COLOR_READ | RHI_GRAPH_COLOR_WRITE, true, true };
		pass.dependencies[1] = { rhiGraphStage_t::ColorOutput, rhiGraphStage_t::Fragment,
			RHI_GRAPH_COLOR_WRITE, RHI_GRAPH_SHADER_READ, false, true };
	}
	return pass;
}

static void Attachment( rhiGraphPassDesc_t &pass, rhiGraphTarget_t target,
	rhiGraphLoad_t load, rhiGraphStore_t store, rhiGraphLayout_t initial, rhiGraphLayout_t final,
	rhiGraphLoad_t stencilLoad = rhiGraphLoad_t::Discard, rhiGraphStore_t stencilStore = rhiGraphStore_t::Discard ) {
	pass.attachments[pass.attachmentCount++] = { target, load, stencilLoad, store, stencilStore, initial, final };
	pass.writeMask |= TargetBit( target );
	if ( load == rhiGraphLoad_t::Load || stencilLoad == rhiGraphLoad_t::Load )
		pass.readMask |= TargetBit( target );
}

bool RHI_CompileGraph( const rhiGraphConfig_t *config, rhiGraph_t *graph ) {
	*graph = {};
	const rhiGraphConfig_t &c = *config;
	if ( !c.renderWidth || !c.renderHeight || !c.windowWidth || !c.windowHeight ||
		 !c.captureWidth || !c.captureHeight || !c.screenWidth || !c.screenHeight || !c.samples || !c.screenSamples )
		return false;
	using T = rhiGraphTarget_t;
	using P = rhiGraphPass_t;
	using F = rhiGraphFormat_t;
	using L = rhiGraphLayout_t;
	using Load = rhiGraphLoad_t;
	using Store = rhiGraphStore_t;
	Target( graph, T::Present, c.windowWidth, c.windowHeight, 1, F::Present,
		RHI_GRAPH_COLOR | RHI_GRAPH_TRANSFER_SOURCE, L::Present )
		.exported = true;
	const bool msaa = c.offscreen && c.samples > 1;
	const uint32_t sampledColor = RHI_GRAPH_COLOR | RHI_GRAPH_SAMPLED;
	if ( c.offscreen ) {
		if ( c.bloom ) {
			for ( uint32_t i = 0; i <= RHI_GRAPH_BLOOM_PASSES * 2; ++i ) {
				const uint32_t divisor = UINT32_C( 1 ) << ( ( i + 1 ) / 2 );
				Target( graph, (T)i, c.captureWidth / divisor, c.captureHeight / divisor, 1, F::Bloom, sampledColor, L::Sampled );
			}
		}
		Target( graph, T::MainColor, c.renderWidth, c.renderHeight, 1, F::Color,
			sampledColor | RHI_GRAPH_TRANSFER_SOURCE, L::Sampled )
			.exported = true;
		if ( c.screenSamples > 1 )
			Target( graph, T::ScreenMsaa, c.screenWidth, c.screenHeight, c.screenSamples, F::Color, RHI_GRAPH_COLOR, L::Color, true );
		Target( graph, T::ScreenColor, c.screenWidth, c.screenHeight, 1, F::Color, sampledColor, L::Sampled ).persistent = true;
		Target( graph, T::ScreenDepth, c.screenWidth, c.screenHeight, c.screenSamples, F::Depth, RHI_GRAPH_DEPTH, L::Depth, true );
		if ( msaa )
			Target( graph, T::MainMsaa, c.renderWidth, c.renderHeight, c.samples, F::Color, RHI_GRAPH_COLOR, L::Color, true );
		if ( c.capture )
			Target( graph, T::Capture, c.captureWidth, c.captureHeight, 1, F::Capture,
				RHI_GRAPH_COLOR | RHI_GRAPH_TRANSFER_SOURCE, L::TransferSource )
				.exported = true;
	}
	Target( graph, T::MainDepth, c.renderWidth, c.renderHeight, c.samples, F::Depth, RHI_GRAPH_DEPTH, L::Depth, !( c.offscreen && c.bloom ) );
	for ( const rhiGraphTargetDesc_t &target : graph->targets ) {
		if ( target.enabled && ( !target.width || !target.height ) )
			return false;
	}

	// Creation order matches the reference backend, independently of execution order.
	rhiGraphPassDesc_t &main = Pass( graph, P::Main, c.offscreen ? c.renderWidth : c.windowWidth,
		c.offscreen ? c.renderHeight : c.windowHeight, c.offscreen ? TargetBit( T::ScreenColor ) : 0, !c.offscreen );
	const T color = c.offscreen ? T::MainColor : T::Present;
	const L colorLayout = c.offscreen ? L::Sampled : L::Present;
	Attachment( main, color, msaa ? Load::Discard : Load::Clear, Store::Store, colorLayout, colorLayout );
	Attachment( main, T::MainDepth, Load::Clear, c.bloom ? Store::Store : Store::Discard, L::Depth, L::Depth,
		c.stencil ? Load::Clear : Load::Discard, c.bloom && c.stencil ? Store::Store : Store::Discard );
	main.depth = 1;
	if ( msaa ) {
		Attachment( main, T::MainMsaa, Load::Clear, c.bloom ? Store::Store : Store::Discard, L::Color, L::Color );
		main.color = 2;
		main.resolve = 0;
	}
	if ( c.offscreen ) {
		if ( c.bloom ) {
			// Geometry submitted after bloom may still sample the retained screen map.
			uint32_t reads = TargetBit( T::ScreenColor );
			for ( uint32_t i = 1; i <= RHI_GRAPH_BLOOM_PASSES; ++i )
				reads |= TargetBit( (T)( i * 2 ) );
			rhiGraphPassDesc_t &post = Pass( graph, P::PostBloom, c.renderWidth, c.renderHeight, reads );
			Attachment( post, T::MainColor, Load::Load, Store::Store, L::Sampled, L::Sampled );
			Attachment( post, T::MainDepth, Load::Load, Store::Discard, L::Depth, L::Depth, Load::Load );
			post.depth = 1;
			if ( msaa ) {
				Attachment( post, T::MainMsaa, Load::Load, Store::Discard, L::Color, L::Color );
				post.color = 2;
				post.resolve = 0;
			}
			rhiGraphPassDesc_t &extract = Pass( graph, P::BloomExtract, c.captureWidth, c.captureHeight, TargetBit( T::MainColor ) );
			Attachment( extract, T::Bloom0, Load::Discard, Store::Store, L::Sampled, L::Sampled );
			for ( uint32_t i = 0; i < RHI_GRAPH_BLOOM_PASSES * 2; ++i ) {
				const T output = (T)( i + 1 );
				const rhiGraphTargetDesc_t &target = graph->targets[(uint32_t)output];
				rhiGraphPassDesc_t &blur = Pass( graph, (P)( (uint32_t)P::Blur0 + i ), target.width, target.height, TargetBit( (T)i ) );
				Attachment( blur, output, Load::Discard, Store::Store, L::Sampled, L::Sampled );
			}
		}
		if ( c.capture ) {
			rhiGraphPassDesc_t &capture = Pass( graph, P::Capture, c.captureWidth, c.captureHeight, TargetBit( T::MainColor ) );
			Attachment( capture, T::Capture, Load::Discard, Store::Store, L::Undefined, L::TransferSource );
		}
		rhiGraphPassDesc_t &gamma = Pass( graph, P::Gamma, c.windowWidth, c.windowHeight, TargetBit( T::MainColor ), true );
		Attachment( gamma, T::Present, Load::Discard, Store::Store, L::Present, L::Present );
		rhiGraphPassDesc_t &screen = Pass( graph, P::ScreenMap, c.screenWidth, c.screenHeight, 0 );
		Attachment( screen, T::ScreenColor, c.screenSamples > 1 ? Load::Discard : Load::Clear, Store::Store, L::Sampled, L::Sampled );
		Attachment( screen, T::ScreenDepth, Load::Clear, Store::Discard, L::Depth, L::Depth, Load::Clear );
		screen.depth = 1;
		if ( c.screenSamples > 1 ) {
			Attachment( screen, T::ScreenMsaa, Load::Clear, Store::Discard, L::Color, L::Color );
			screen.color = 2;
			screen.resolve = 0;
		}
	}

	uint32_t writers[(uint32_t)T::Count] = {};
	uint32_t readers[(uint32_t)T::Count] = {};
	for ( uint32_t p = 0; p < (uint32_t)P::Count; ++p ) {
		rhiGraphPassDesc_t &pass = graph->passes[p];
		if ( !pass.enabled )
			continue;
		for ( uint32_t t = 0; t < (uint32_t)T::Count; ++t ) {
			const uint32_t bit = UINT32_C( 1 ) << t;
			if ( !( ( pass.readMask | pass.writeMask ) & bit ) )
				continue;
			rhiGraphTargetDesc_t &target = graph->targets[t];
			if ( !target.enabled )
				return false;
			if ( target.firstUse == RHI_INVALID_OFFSET )
				target.firstUse = p;
			target.lastUse = p;
			pass.dependencyMask |= writers[t];
			if ( pass.writeMask & bit ) {
				pass.dependencyMask |= readers[t];
				writers[t] = UINT32_C( 1 ) << p;
				readers[t] = 0;
			} else {
				readers[t] |= UINT32_C( 1 ) << p;
			}
		}
	}
	return true;
}
