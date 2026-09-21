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
	if ( c.shadowSize && ( c.shadowSize < 128 || c.shadowSize > 8192 || ( c.shadowSize & ( c.shadowSize - 1 ) ) ) )
		return false;
	if ( c.occlusionScale && ( !c.offscreen || c.occlusionScale > 2 ) )
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
	if ( c.shadowSize ) {
		for ( uint32_t index = 0; index < 2; ++index )
			Target( graph, (T)( (uint32_t)T::LocalShadow + index ), c.shadowSize, c.shadowSize, 1, F::ShadowDepth, RHI_GRAPH_DEPTH | RHI_GRAPH_SAMPLED, L::DepthSampled );
	}
	if ( c.occlusionScale ) {
		for ( uint32_t i = 0; i < 2; ++i )
			Target( graph, (T)( (uint32_t)T::Occlusion + i ), c.renderWidth / c.occlusionScale,
				c.renderHeight / c.occlusionScale, 1, F::Occlusion, sampledColor, L::Sampled );
		auto &depth = graph->targets[(uint32_t)T::MainDepth];
		depth.usage |= RHI_GRAPH_SAMPLED;
		depth.initialLayout = L::DepthSampled;
		depth.transient = false;
	}
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

	if ( c.shadowSize ) {
		const uint32_t shadowReads = TargetBit( T::LocalShadow ) | TargetBit( T::SunShadow );
		graph->passes[(uint32_t)P::Main].readMask |= shadowReads;
		if ( c.offscreen ) {
			graph->passes[(uint32_t)P::ScreenMap].readMask |= shadowReads;
			if ( c.bloom )
				graph->passes[(uint32_t)P::PostBloom].readMask |= shadowReads;
		}
		for ( uint32_t index = 0; index < 2; ++index ) {
			const P id = (P)( (uint32_t)P::LocalShadow + index );
			rhiGraphPassDesc_t &shadow = Pass( graph, id, c.shadowSize, c.shadowSize, 0 );
			shadow.color = RHI_INVALID_OFFSET;
			shadow.depth = 0;
			Attachment( shadow, (T)( (uint32_t)T::LocalShadow + index ), Load::Clear, Store::Store, L::DepthSampled, L::DepthSampled );
			shadow.dependencies[0] = { rhiGraphStage_t::Fragment, rhiGraphStage_t::DepthTests,
				RHI_GRAPH_SHADER_READ, RHI_GRAPH_DEPTH_WRITE, true, false };
			shadow.dependencies[1] = { rhiGraphStage_t::DepthTests, rhiGraphStage_t::Fragment,
				RHI_GRAPH_DEPTH_WRITE, RHI_GRAPH_SHADER_READ, false, false };
			graph->executionOrder[graph->executionCount++] = id;
		}
		// A shadow interlude can occur after UI or another scene. Retain every
		// attachment and resume with compatible load passes, never another clear.
		for ( uint32_t index = 0; index < ( c.offscreen ? 2u : 1u ); ++index ) {
			const P source = index ? P::ScreenMap : P::Main;
			const P id = index ? P::ScreenResume : P::MainResume;
			rhiGraphPassDesc_t &scene = graph->passes[(uint32_t)source];
			rhiGraphPassDesc_t &resume = Pass( graph, id, scene.width, scene.height, scene.readMask, !c.offscreen );
			resume.dependencies[0] = { rhiGraphStage_t::SceneAttachments, rhiGraphStage_t::SceneAttachments,
				RHI_GRAPH_COLOR_WRITE | RHI_GRAPH_DEPTH_WRITE,
				RHI_GRAPH_COLOR_READ | RHI_GRAPH_COLOR_WRITE | RHI_GRAPH_DEPTH_READ | RHI_GRAPH_DEPTH_WRITE, true, true };
			resume.color = scene.color;
			resume.depth = scene.depth;
			resume.resolve = scene.resolve;
			for ( uint32_t i = 0; i < scene.attachmentCount; ++i ) {
				auto &attachment = scene.attachments[i];
				attachment.store = Store::Store;
				if ( c.stencil && i == scene.depth )
					attachment.stencilStore = Store::Store;
				graph->targets[(uint32_t)attachment.target].transient = false;
				Attachment( resume, attachment.target, Load::Load, Store::Store,
					attachment.initialLayout, attachment.finalLayout,
					c.stencil && i == scene.depth ? Load::Load : Load::Discard, attachment.stencilStore );
			}
		}
		if ( c.offscreen && c.bloom ) {
			auto &post = graph->passes[(uint32_t)P::PostBloom];
			for ( uint32_t i = 0; i < post.attachmentCount; ++i ) {
				post.attachments[i].store = Store::Store;
				if ( c.stencil && i == post.depth )
					post.attachments[i].stencilStore = Store::Store;
			}
		}
	}
	if ( c.occlusionScale ) {
		// Depth must survive every scene interlude, including shadow resumption.
		for ( uint32_t p = 0; p < (uint32_t)P::Count; ++p ) {
			auto &scene = graph->passes[p];
			if ( !scene.enabled || scene.depth == RHI_INVALID_OFFSET || scene.attachments[scene.depth].target != T::MainDepth )
				continue;
			for ( uint32_t i = 0; i < scene.attachmentCount; ++i ) {
				auto &attachment = scene.attachments[i];
				attachment.store = Store::Store;
				graph->targets[(uint32_t)attachment.target].transient = false;
				if ( i == scene.depth ) {
					attachment.initialLayout = attachment.finalLayout = L::DepthSampled;
					if ( c.stencil )
						attachment.stencilStore = Store::Store;
				}
			}
			scene.dependencies[1] = { rhiGraphStage_t::SceneAttachments, rhiGraphStage_t::Fragment,
				RHI_GRAPH_COLOR_WRITE | RHI_GRAPH_DEPTH_WRITE, RHI_GRAPH_SHADER_READ, false, false };
		}
		for ( uint32_t i = 0; i < 2; ++i ) {
			const T target = (T)( (uint32_t)T::Occlusion + i );
			const auto &image = graph->targets[(uint32_t)target];
			auto &node = Pass( graph, (P)( (uint32_t)P::Occlusion + i ), image.width, image.height,
				TargetBit( T::MainDepth ) | ( i ? TargetBit( T::Occlusion ) : 0 ) );
			Attachment( node, target, Load::Discard, Store::Store, L::Sampled, L::Sampled );
		}
		const auto &scene = graph->passes[(uint32_t)P::Main];
		auto &apply = Pass( graph, P::OcclusionApply, scene.width, scene.height, scene.readMask | TargetBit( T::OcclusionBlur ) );
		apply.color = scene.color;
		apply.depth = scene.depth;
		apply.resolve = scene.resolve;
		apply.dependencies[0] = { rhiGraphStage_t::Fragment, rhiGraphStage_t::SceneAttachments,
			RHI_GRAPH_SHADER_READ, RHI_GRAPH_COLOR_READ | RHI_GRAPH_COLOR_WRITE | RHI_GRAPH_DEPTH_READ | RHI_GRAPH_DEPTH_WRITE, true, false };
		apply.dependencies[1] = scene.dependencies[1];
		for ( uint32_t i = 0; i < scene.attachmentCount; ++i ) {
			const auto &a = scene.attachments[i];
			Attachment( apply, a.target, Load::Load, Store::Store, a.initialLayout, a.finalLayout,
				c.stencil && i == scene.depth ? Load::Load : Load::Discard, a.stencilStore );
		}
	}
	// Preserve the legacy IDs/possible-pass intervals when lighting is disabled.
	// Creation order is independent of this dependency/lifetime order.
	for ( uint32_t p = 0; p < (uint32_t)P::LocalShadow; ++p ) {
		graph->executionOrder[graph->executionCount++] = (P)p;
		if ( c.shadowSize && p == (uint32_t)P::Main )
			graph->executionOrder[graph->executionCount++] = P::MainResume;
		if ( c.occlusionScale && p == (uint32_t)P::Main ) {
			graph->executionOrder[graph->executionCount++] = P::Occlusion;
			graph->executionOrder[graph->executionCount++] = P::OcclusionBlur;
			graph->executionOrder[graph->executionCount++] = P::OcclusionApply;
		}
		if ( c.shadowSize && c.offscreen && p == (uint32_t)P::ScreenMap )
			graph->executionOrder[graph->executionCount++] = P::ScreenResume;
	}

	uint32_t writers[(uint32_t)T::Count] = {};
	uint32_t readers[(uint32_t)T::Count] = {};
	for ( uint32_t order = 0; order < graph->executionCount; ++order ) {
		const uint32_t p = (uint32_t)graph->executionOrder[order];
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
				target.firstUse = order;
			target.lastUse = order;
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
