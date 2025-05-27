/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022-2025 Robert Beckebans (id Tech 4x integration)
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "precompiled.h"
#pragma hdrstop

#include "renderer/RenderCommon.h"
#include "TonemapPass.h"
#include "TonemapPass_cb.h"

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;

TonemapPass::TonemapPass()
	: isLoaded( false )
	, colorLut( nullptr )
	, colorLutSize( 0 )
	, commonPasses( nullptr )
	, pcEnabled( false )
{
}

void TonemapPass::Init( nvrhi::DeviceHandle _device, CommonRenderPasses* _commonPasses, const CreateParameters& _params, nvrhi::IFramebuffer* _sampleFramebuffer )
{
	assert( _params.histogramBins <= 256 );

	device = _device;
	commonPasses = _commonPasses;

	// Determine if push constants can be used
	size_t pcSize = sizeof( ToneMappingConstants );
	pcEnabled = pcSize <= deviceManager->GetMaxPushConstantSize();

	auto histogramShaderInfo = ( _params.isTextureArray ) ? renderProgManager.GetProgramInfo( BUILTIN_HISTOGRAM_TEX_ARRAY_CS ) : renderProgManager.GetProgramInfo( BUILTIN_HISTOGRAM_CS );
	auto exposureShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_EXPOSURE_CS );
	auto tonemapShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_TONEMAPPING );

	histogramShader = histogramShaderInfo.cs;
	exposureShader = exposureShaderInfo.cs;
	tonemapShader = tonemapShaderInfo.ps;

	if( !pcEnabled )
	{
		nvrhi::BufferDesc constantBufferDesc;
		constantBufferDesc.byteSize = pcSize;
		constantBufferDesc.debugName = "ToneMappingConstants";
		constantBufferDesc.isConstantBuffer = true;
		constantBufferDesc.isVolatile = true;
		constantBufferDesc.maxVersions = _params.numConstantBufferVersions;
		toneMappingCb = device->createBuffer( constantBufferDesc );
	}

	nvrhi::BufferDesc storageBufferDesc;
	storageBufferDesc.byteSize = sizeof( uint ) * _params.histogramBins;
	storageBufferDesc.format = nvrhi::Format::R32_UINT;
	storageBufferDesc.canHaveUAVs = true;
	storageBufferDesc.debugName = "HistogramBuffer";
	storageBufferDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
	storageBufferDesc.keepInitialState = true;
	storageBufferDesc.canHaveTypedViews = true;
	histogramBuffer = device->createBuffer( storageBufferDesc );

	if( _params.exposureBufferOverride )
	{
		exposureBuffer = _params.exposureBufferOverride;
	}
	else
	{
		storageBufferDesc.byteSize = sizeof( uint );
		storageBufferDesc.format = nvrhi::Format::R32_UINT;
		storageBufferDesc.debugName = "ExposureBuffer";
		exposureBuffer = device->createBuffer( storageBufferDesc );
	}

	colorLut = globalImages->blackImage;

	if( _params.colorLUT )
	{
		int w = _params.colorLUT->GetOpts().width;
		int h = _params.colorLUT->GetOpts().height;

		colorLutSize = h;

		if( w != h * h )
		{
			common->Error( "Color LUT texture size must be: width = (n*n), height = (n)" );
			colorLutSize = 0.f;
		}
		else
		{
			colorLut = _params.colorLUT;
		}
	}

	// histogram pipeline
	{
		nvrhi::BindingLayoutDesc histogramLayout;
		histogramLayout.visibility = nvrhi::ShaderType::Compute;
		if( pcEnabled )
		{
			histogramLayout.bindings =
			{
				nvrhi::BindingLayoutItem::PushConstants( 0, pcSize ),
				nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_UAV( 0 )
			};
		}
		else
		{
			histogramLayout.bindings =
			{
				nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
				nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_UAV( 0 )
			};
		}

		histogramBindingLayout = device->createBindingLayout( histogramLayout );

		nvrhi::ComputePipelineDesc computePipelineDesc;
		computePipelineDesc.CS = histogramShader;
		computePipelineDesc.bindingLayouts = { histogramBindingLayout };
		histogramPipeline = device->createComputePipeline( computePipelineDesc );
	}

	// exposure pipeline
	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.visibility = nvrhi::ShaderType::Compute;
		if( pcEnabled )
		{
			layoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::PushConstants( 0, pcSize ),
				nvrhi::BindingLayoutItem::TypedBuffer_SRV( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_UAV( 0 )
			};
		}
		else
		{
			// Use volatile constant buffer for exposure
			layoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_SRV( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_UAV( 0 )
			};
		}

		nvrhi::BindingSetDesc bindingSetDesc;
		if( pcEnabled )
		{
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::PushConstants( 0, pcSize ),
				nvrhi::BindingSetItem::TypedBuffer_SRV( 0, histogramBuffer ),
				nvrhi::BindingSetItem::TypedBuffer_UAV( 0, exposureBuffer )
			};
		}
		else
		{
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::ConstantBuffer( 0, toneMappingCb ),
				nvrhi::BindingSetItem::TypedBuffer_SRV( 0, histogramBuffer ),
				nvrhi::BindingSetItem::TypedBuffer_UAV( 0, exposureBuffer )
			};
		}
		exposureBindingLayout = device->createBindingLayout( layoutDesc );
		exposureBindingSet = device->createBindingSet( bindingSetDesc, exposureBindingLayout );

		nvrhi::ComputePipelineDesc computePipelineDesc;
		computePipelineDesc.CS = exposureShader;
		computePipelineDesc.bindingLayouts = { exposureBindingLayout };
		exposurePipeline = device->createComputePipeline( computePipelineDesc );
	}

	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.visibility = nvrhi::ShaderType::Pixel;
		if( pcEnabled )
		{
			layoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::PushConstants( 0, pcSize ),
				nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_SRV( 1 ),
				nvrhi::BindingLayoutItem::Texture_SRV( 2 ),
				nvrhi::BindingLayoutItem::Sampler( 0 )
			};
		}
		else
		{
			layoutDesc.bindings =
			{
				nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
				nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
				nvrhi::BindingLayoutItem::TypedBuffer_SRV( 1 ),
				nvrhi::BindingLayoutItem::Texture_SRV( 2 ),
				nvrhi::BindingLayoutItem::Sampler( 0 )
			};
		}
		renderBindingLayout = device->createBindingLayout( layoutDesc );

		nvrhi::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
		pipelineDesc.VS = tonemapShaderInfo.vs;
		pipelineDesc.PS = tonemapShaderInfo.ps;
		pipelineDesc.bindingLayouts = { renderBindingLayout };

		pipelineDesc.renderState.rasterState.setCullNone();
		pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
		pipelineDesc.renderState.depthStencilState.stencilEnable = false;

		renderPipeline = device->createGraphicsPipeline( pipelineDesc, _sampleFramebuffer );
	}

	isLoaded = true;
}

void TonemapPass::Render(
	nvrhi::ICommandList* commandList,
	const ToneMappingParameters& params,
	const viewDef_t* viewDef,
	nvrhi::ITexture* sourceTexture,
	nvrhi::FramebufferHandle _targetFb )
{
	size_t renderHash = std::hash<nvrhi::ITexture*>()( sourceTexture );
	nvrhi::BindingSetHandle renderBindingSet;
	for( int i = renderBindingHash.First( renderHash ); i != -1; i = renderBindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle bindingSet = renderBindingSets[i];
		if( sourceTexture == bindingSet->getDesc()->bindings[1].resourceHandle )
		{
			renderBindingSet = bindingSet;
			break;
		}
	}

	if( !renderBindingSet )
	{
		nvrhi::BindingSetDesc bindingSetDesc;
		if( pcEnabled )
		{
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::PushConstants( 0, sizeof( ToneMappingConstants ) ),
				nvrhi::BindingSetItem::Texture_SRV( 0, sourceTexture ),
				nvrhi::BindingSetItem::TypedBuffer_SRV( 1, exposureBuffer ),
				nvrhi::BindingSetItem::Texture_SRV( 2, colorLut->GetTextureHandle() ),
				nvrhi::BindingSetItem::Sampler( 0, commonPasses->m_LinearClampSampler )
			};
		}
		else
		{
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::ConstantBuffer( 0, toneMappingCb ),
				nvrhi::BindingSetItem::Texture_SRV( 0, sourceTexture ),
				nvrhi::BindingSetItem::TypedBuffer_SRV( 1, exposureBuffer ),
				nvrhi::BindingSetItem::Texture_SRV( 2, colorLut->GetTextureHandle() ),
				nvrhi::BindingSetItem::Sampler( 0, commonPasses->m_LinearClampSampler )
			};
		}
		renderBindingSet = device->createBindingSet( bindingSetDesc, renderBindingLayout );
		renderBindingSets.Append( renderBindingSet );
		renderBindingHash.Add( renderHash, renderBindingSets.Num() - 1 );
	}

	{
		nvrhi::GraphicsState state;
		state.pipeline = renderPipeline;
		state.framebuffer = _targetFb;
		state.bindings = { renderBindingSet };
		nvrhi::Viewport viewport{ ( float )viewDef->viewport.x1,
								  ( float )viewDef->viewport.x2 + 1,
								  ( float )viewDef->viewport.y1,
								  ( float )viewDef->viewport.y2 + 1,
								  viewDef->viewport.zmin,
								  viewDef->viewport.zmax };
		state.viewport.addViewportAndScissorRect( viewport );

		bool enableColorLUT = params.enableColorLUT && colorLutSize > 0;

		ToneMappingConstants toneMappingConstants = {};
		toneMappingConstants.exposureScale = ::exp2f( r_exposure.GetFloat() );
		toneMappingConstants.whitePointInvSquared = 1.f / powf( params.whitePoint, 2.f );
		toneMappingConstants.minAdaptedLuminance = r_hdrMinLuminance.GetFloat();
		toneMappingConstants.maxAdaptedLuminance = r_hdrMaxLuminance.GetFloat();
		toneMappingConstants.sourceSlice = 0;
		toneMappingConstants.colorLUTTextureSize = enableColorLUT ? idVec2( colorLutSize * colorLutSize, colorLutSize ) : idVec2( 0.f, 0.f );
		toneMappingConstants.colorLUTTextureSizeInv = enableColorLUT ? 1.f / toneMappingConstants.colorLUTTextureSize : idVec2( 0.f, 0.f );

		if( !pcEnabled )
		{
			commandList->writeBuffer( toneMappingCb, &toneMappingConstants, sizeof( toneMappingConstants ) );
		}

		commandList->setGraphicsState( state );

		if( pcEnabled )
		{
			commandList->setPushConstants( &toneMappingConstants, sizeof( toneMappingConstants ) );
		}

		nvrhi::DrawArguments args;
		args.instanceCount = 1;
		args.vertexCount = 4;
		commandList->draw( args );
	}
}

void TonemapPass::SimpleRender( nvrhi::ICommandList* commandList, const ToneMappingParameters& params, const viewDef_t* viewDef, nvrhi::ITexture* sourceTexture, nvrhi::FramebufferHandle _fbHandle )
{
	commandList->beginMarker( "ToneMapping" );
	ResetHistogram( commandList );
	AddFrameToHistogram( commandList, viewDef, sourceTexture );
	ComputeExposure( commandList, params );
	Render( commandList, params, viewDef, sourceTexture, _fbHandle );
	commandList->endMarker();
}

void TonemapPass::ResetExposure( nvrhi::ICommandList* commandList, float initialExposure )
{
	uint32_t uintValue = *( uint32_t* )&initialExposure;
	commandList->clearBufferUInt( exposureBuffer, uintValue );
}

void TonemapPass::ResetHistogram( nvrhi::ICommandList* commandList )
{
	commandList->clearBufferUInt( histogramBuffer, 0 );
}

void TonemapPass::AddFrameToHistogram( nvrhi::ICommandList* commandList, const viewDef_t* viewDef, nvrhi::ITexture* sourceTexture )
{
	size_t renderHash = std::hash<nvrhi::ITexture*>()( sourceTexture );
	nvrhi::BindingSetHandle bindingSet;
	for( int i = histogramBindingHash.First( renderHash ); i != -1; i = histogramBindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle foundSet = histogramBindingSets[i];
		if( sourceTexture == foundSet->getDesc()->bindings[1].resourceHandle )
		{
			bindingSet = foundSet;
			break;
		}
	}

	if( !bindingSet )
	{
		nvrhi::BindingSetDesc bindingSetDesc;
		if( pcEnabled )
		{
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::PushConstants( 0, sizeof( ToneMappingConstants ) ),
				nvrhi::BindingSetItem::Texture_SRV( 0, sourceTexture ),
				nvrhi::BindingSetItem::TypedBuffer_UAV( 0, histogramBuffer )
			};
		}
		else
		{
			bindingSetDesc.bindings =
			{
				nvrhi::BindingSetItem::ConstantBuffer( 0, toneMappingCb ),
				nvrhi::BindingSetItem::Texture_SRV( 0, sourceTexture ),
				nvrhi::BindingSetItem::TypedBuffer_UAV( 0, histogramBuffer )
			};
		}

		bindingSet = device->createBindingSet( bindingSetDesc, histogramBindingLayout );
		histogramBindingSets.Append( bindingSet );
		histogramBindingHash.Add( renderHash, histogramBindingSets.Num() - 1 );
	}

	nvrhi::ViewportState viewportState;
	nvrhi::Viewport viewport{ ( float )viewDef->viewport.x1,
							  ( float )viewDef->viewport.x2,
							  ( float )viewDef->viewport.y1,
							  ( float )viewDef->viewport.y2,
							  viewDef->viewport.zmin,
							  viewDef->viewport.zmax };
	viewportState.addViewportAndScissorRect( viewport );

	for( uint viewportIndex = 0; viewportIndex < viewportState.scissorRects.size(); viewportIndex++ )
	{
		ToneMappingConstants toneMappingConstants = {};
		toneMappingConstants.logLuminanceScale = 1.0f / ( r_hdrMinLuminance.GetFloat() - r_hdrMaxLuminance.GetFloat() );
		toneMappingConstants.logLuminanceBias = -r_hdrMinLuminance.GetFloat() * toneMappingConstants.logLuminanceScale;

		nvrhi::Rect& scissor = viewportState.scissorRects[viewportIndex];
		toneMappingConstants.viewOrigin = idVec2i( scissor.minX, scissor.minY );
		toneMappingConstants.viewSize = idVec2i( scissor.maxX - scissor.minX, scissor.maxY - scissor.minY );
		toneMappingConstants.sourceSlice = 0;

		if( !pcEnabled )
		{
			commandList->writeBuffer( toneMappingCb, &toneMappingConstants, sizeof( toneMappingConstants ) );
		}

		nvrhi::ComputeState state;
		state.pipeline = histogramPipeline;
		state.bindings = { bindingSet };
		commandList->setComputeState( state );

		if( pcEnabled )
		{
			commandList->setPushConstants( &toneMappingConstants, sizeof( toneMappingConstants ) );
		}

		idVec2i numGroups = ( toneMappingConstants.viewSize + idVec2i( 15, 15 ) ) / idVec2i( 16, 16 );
		commandList->dispatch( numGroups.x, numGroups.y, 1 );
	}
}

static const float maxLuminance = 4.0f;
static const float minLuminance = -10.0f;

void TonemapPass::ComputeExposure( nvrhi::ICommandList* commandList, const ToneMappingParameters& params )
{
	ToneMappingConstants toneMappingConstants = {};
	toneMappingConstants.logLuminanceScale = maxLuminance - minLuminance;
	toneMappingConstants.logLuminanceBias = minLuminance;
	toneMappingConstants.histogramLowPercentile = Min( 0.99f, Max( 0.f, params.histogramLowPercentile ) );
	toneMappingConstants.histogramHighPercentile = Min( 1.f, Max( toneMappingConstants.histogramLowPercentile, params.histogramHighPercentile ) );
	toneMappingConstants.eyeAdaptationSpeedUp = r_hdrAdaptionRate.GetFloat();
	toneMappingConstants.eyeAdaptationSpeedDown = r_hdrAdaptionRate.GetFloat() / 2.f;
	toneMappingConstants.minAdaptedLuminance = r_hdrMinLuminance.GetFloat();
	toneMappingConstants.maxAdaptedLuminance = r_hdrMaxLuminance.GetFloat();
	toneMappingConstants.frameTime = Sys_Milliseconds() / 1000.0f;

	if( !pcEnabled )
	{
		commandList->writeBuffer( toneMappingCb, &toneMappingConstants, sizeof( toneMappingConstants ) );
	}

	nvrhi::ComputeState state;
	state.pipeline = exposurePipeline;
	state.bindings = { exposureBindingSet };
	commandList->setComputeState( state );

	if( pcEnabled )
	{
		commandList->setPushConstants( &toneMappingConstants, sizeof( toneMappingConstants ) );
	}

	commandList->dispatch( 1 );
}