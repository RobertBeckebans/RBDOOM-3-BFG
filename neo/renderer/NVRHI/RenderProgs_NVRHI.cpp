/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2022 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "../RenderCommon.h"
#include <ShaderMake/ShaderBlob.h>
#include <sys/DeviceManager.h>


/*
========================
idRenderProgManager::StartFrame
========================
*/
void idRenderProgManager::StartFrame()
{

}

/*
================================================================================================
idRenderProgManager::BindProgram
================================================================================================
*/
void idRenderProgManager::BindProgram( int index )
{
	if( currentIndex == index )
	{
		return;
	}

	currentIndex = index;
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	currentIndex = -1;
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
void idRenderProgManager::LoadShader( int index, rpStage_t stage )
{
	if( shaders[index].handle )
	{
		return; // Already loaded
	}

	LoadShader( shaders[index] );
}

extern DeviceManager* deviceManager;

/*
================================================================================================
 createShaderPermutation

 * Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
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
 *
================================================================================================
*/
nvrhi::ShaderHandle createShaderPermutation( nvrhi::IDevice* device, const nvrhi::ShaderDesc& d, const void* blob, size_t blobSize,
		const ShaderMake::ShaderConstant* constants, uint32_t numConstants, bool errorIfNotFound = true )
{
	const void* binary = nullptr;
	size_t binarySize = 0;

	if( ShaderMake::FindPermutationInBlob( blob, blobSize, constants, numConstants, &binary, &binarySize ) )
	{
		return device->createShader( d, binary, binarySize );
	}

	if( errorIfNotFound )
	{
		std::string message = ShaderMake::FormatShaderNotFoundMessage( blob, blobSize, constants, numConstants );
		device->getMessageCallback()->message( nvrhi::MessageSeverity::Error, message.c_str() );
	}

	return nullptr;
}

/*
================================================================================================
idRenderProgManager::LoadGLSLShader
================================================================================================
*/
void idRenderProgManager::LoadShader( shader_t& shader )
{
	idStr stage;
	nvrhi::ShaderType shaderType{};

	if( shader.stage == SHADER_STAGE_VERTEX )
	{
		stage = "vs";
		shaderType = nvrhi::ShaderType::Vertex;
	}
	else if( shader.stage == SHADER_STAGE_FRAGMENT )
	{
		stage = "ps";
		shaderType = nvrhi::ShaderType::Pixel;
	}
	else if( shader.stage == SHADER_STAGE_COMPUTE )
	{
		stage = "cs";
		shaderType = nvrhi::ShaderType::Compute;
	}

	idStr adjustedName = shader.name;
	adjustedName.StripFileExtension();
	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::D3D12 )
	{
		adjustedName = idStr( "renderprogs2/dxil/" ) + adjustedName + "." + stage + ".bin";
	}
	else if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		adjustedName = idStr( "renderprogs2/spirv/" ) + adjustedName + "." + stage + ".bin";
	}
	else
	{
		common->FatalError( "Unsupported graphics api" );
	}

	ShaderBlob shaderBlob = GetBytecode( adjustedName );

	if( !shaderBlob.data )
	{
		return;
	}

	idList<ShaderMake::ShaderConstant> constants;

	for( int i = 0; i < shader.macros.Num(); i++ )
	{
		constants.Append( ShaderMake::ShaderConstant
		{
			shader.macros[i].name.c_str(),
			shader.macros[i].definition.c_str()
		} );
	}

	nvrhi::ShaderDesc desc = nvrhi::ShaderDesc( shaderType );
	desc.debugName = ( idStr( shader.name ) + idStr( shader.nameOutSuffix ) ).c_str();

	nvrhi::ShaderDesc descCopy = desc;
	// TODO(Stephen): Might not want to hard-code this.
	descCopy.entryName = "main";

	ShaderMake::ShaderConstant* shaderConstant( nullptr );

	nvrhi::ShaderHandle shaderHandle = createShaderPermutation( device, descCopy, shaderBlob.data, shaderBlob.size,
									   ( constants.Num() > 0 ) ? &constants[0] : shaderConstant, uint32_t( constants.Num() ) );

	shader.handle = shaderHandle;

	// SRS - Free the shader blob data, otherwise a leak will occur
	Mem_Free( shaderBlob.data );
}

/*
================================================================================================
idRenderProgManager::GetBytecode
================================================================================================
*/
ShaderBlob idRenderProgManager::GetBytecode( const char* fileName )
{
	ShaderBlob blob;

	blob.size = fileSystem->ReadFile( fileName, &blob.data );

	if( !blob.data )
	{
		common->FatalError( "Couldn't read the binary file for shader %s", fileName );
	}

	return blob;
}

/*
================================================================================================
idRenderProgManager::LoadGLSLProgram
================================================================================================
*/
void idRenderProgManager::LoadProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex )
{
	renderProg_t& prog = renderProgs[programIndex];
	prog.fragmentShaderIndex = fragmentShaderIndex;
	prog.vertexShaderIndex = vertexShaderIndex;
	if( prog.vertexLayout != LAYOUT_UNKNOWN )
	{
		prog.inputLayout = device->createInputLayout(
							   &vertexLayoutDescs[prog.vertexLayout][0],
							   vertexLayoutDescs[prog.vertexLayout].Num(),
							   shaders[prog.vertexShaderIndex].handle );
	}
	prog.bindingLayouts = bindingLayouts[prog.bindingLayoutType];
}

/*
================================================================================================
idRenderProgManager::LoadComputeProgram
================================================================================================
*/
void idRenderProgManager::LoadComputeProgram( const int programIndex, const int computeShaderIndex )
{
	renderProg_t& prog = renderProgs[programIndex];
	prog.computeShaderIndex = computeShaderIndex;
	if( prog.vertexLayout != LAYOUT_UNKNOWN )
	{
		prog.inputLayout = device->createInputLayout(
							   &vertexLayoutDescs[prog.vertexLayout][0],
							   vertexLayoutDescs[prog.vertexLayout].Num(),
							   shaders[prog.vertexShaderIndex].handle );
	}
	prog.bindingLayouts = bindingLayouts[prog.bindingLayoutType];
}


/*
================================================================================================
idRenderProgManager::FindProgram
================================================================================================
*/
int	 idRenderProgManager::FindProgram( const char* name, int vIndex, int fIndex, bindingLayoutType_t bindingType )
{
	for( int i = 0; i < renderProgs.Num(); ++i )
	{
		if( ( renderProgs[i].vertexShaderIndex == vIndex ) && ( renderProgs[i].fragmentShaderIndex == fIndex ) )
		{
			return i;
		}
	}

	renderProg_t program;
	program.name = name;
	program.vertexLayout = LAYOUT_DRAW_VERT;
	program.bindingLayoutType = bindingType;
	int index = renderProgs.Append( program );
	LoadProgram( index, vIndex, fIndex );
	return index;
}

int idRenderProgManager::UniformSize()
{
	return uniforms.Allocated();
}

/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
void idRenderProgManager::CommitUniforms( uint64 stateBits )
{
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();

	tr.backend.ResetPipelineCache();

	for( int i = 0; i < shaders.Num(); i++ )
	{
		if( shaders[i].handle )
		{
			shaders[i].handle.Reset();
		}
	}
}

/*
================================================================================================
idRenderProgManager::SetUniformValue
================================================================================================
*/
void idRenderProgManager::SetUniformValue( const renderParm_t rp, const float value[4] )
{
	bool rpChanged = false;

	for( int i = 0; i < 4; i++ )
	{
		if( uniforms[rp][i] != value[i] )
		{
			uniforms[rp][i] = value[i];
			rpChanged = true;
		}
	}

	if( rpChanged )
	{
		for( int i = 0; i < renderParmLayoutTypes[rp].Num(); i++ )
		{
			// SRS - set flag if uniforms changed for associated binding layout types
			uniformsChanged[renderParmLayoutTypes[rp][i]] = true;
		}
	}
}

/*
================================================================================================
idRenderProgManager::ZeroUniforms
================================================================================================
*/
void idRenderProgManager::ZeroUniforms()
{
	memset( uniforms.Ptr(), 0, uniforms.Allocated() );

	for( int i = 0; i < NUM_BINDING_LAYOUTS; i++ )
	{
		uniformsChanged[i] = true;
	}
}

/*
================================================================================================
idRenderProgManager::SelectUniforms
================================================================================================
*/
void idRenderProgManager::SelectUniforms( renderParmSet_t* renderParmSet, int bindingLayoutType )
{
	switch ( layoutTypeAttributes[bindingLayoutType].rpSubSet )
	{
		case renderParmSet0:
		{
			for( int i = 0; i < rpMinimalSet0.Num(); i++ )
			{
				memcpy( &renderParmSet->minimalSet[i], &uniforms[rpMinimalSet0[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet1:
		{
			for( int i = 0; i < rpMinimalSet1.Num(); i++ )
			{
				memcpy( &renderParmSet->minimalSet[i], &uniforms[rpMinimalSet1[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet2:
		{
			for( int i = 0; i < rpMinimalSet2.Num(); i++ )
			{
				memcpy( &renderParmSet->minimalSet[i], &uniforms[rpMinimalSet2[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet3:
		{
			for( int i = 0; i < rpNominalSet3.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet3[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet4:
		{
			for( int i = 0; i < rpNominalSet4.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet4[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet5:
		{
			for( int i = 0; i < rpNominalSet5.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet5[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet6:
		{
			for( int i = 0; i < rpNominalSet6.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet6[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet7:
		{
			for( int i = 0; i < rpNominalSet7.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet7[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet8:
		{
			for( int i = 0; i < rpNominalSet8.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet8[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet9:
		{
			for( int i = 0; i < rpMaximalSet9.Num(); i++ )
			{
				memcpy( &renderParmSet->maximalSet[i], &uniforms[rpMaximalSet9[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet10:
		{
			for( int i = 0; i < rpMaximalSet10.Num(); i++ )
			{
				memcpy( &renderParmSet->maximalSet[i], &uniforms[rpMaximalSet10[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet11:
		{
			for( int i = 0; i < rpMaximalSet11.Num(); i++ )
			{
				memcpy( &renderParmSet->maximalSet[i], &uniforms[rpMaximalSet11[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet12:
		{
			for( int i = 0; i < rpNominalSet12.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet12[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmSet13:
		{
			for( int i = 0; i < rpNominalSet13.Num(); i++ )
			{
				memcpy( &renderParmSet->nominalSet[i], &uniforms[rpNominalSet13[i]], sizeof( idVec4 ) );
			}
			break;
		}

		case renderParmNullSet:
		default:
		{
			common->FatalError( "No renderparm subset found for binding layout type %d\n", bindingLayoutType );
		}
	}

	return;
}

// Only updates the constant buffer if it was updated at all
bool idRenderProgManager::CommitConstantBuffer( nvrhi::ICommandList* commandList, bool bindingLayoutTypeChanged )
{
	const int bindingLayoutType = BindingLayoutType();

	// SRS - If push constants enabled, skip writing the constant buffer but return state change indicator
	if( layoutTypeAttributes[bindingLayoutType].pcEnabled )
	{
			return uniformsChanged[bindingLayoutType] || bindingLayoutTypeChanged;
	}
	// RB: It would be better to NUM_BINDING_LAYOUTS uniformsChanged entrys but we don't know the current binding layout type when we set the uniforms.
	// The vkDoom3 backend even didn't bother with this and always fired the uniforms for each draw call.
	// SRS - Implemented uniformsChanged detection at per-binding layout type granularity
	else if( uniformsChanged[bindingLayoutType] || bindingLayoutTypeChanged )
	{
		renderParmSet_t renderParmSet;

		SelectUniforms( &renderParmSet, bindingLayoutType );

		commandList->writeBuffer( constantBuffer[bindingLayoutType], &renderParmSet, layoutTypeAttributes[bindingLayoutType].rpBufSize );

		uniformsChanged[bindingLayoutType] = false;

		return true;
	}

	return false;
}

void idRenderProgManager::CommitPushConstants( nvrhi::ICommandList* commandList, int bindingLayoutType )
{
	if( layoutTypeAttributes[bindingLayoutType].pcEnabled )
	{
		renderParmSet_t renderParmSet;

		SelectUniforms( &renderParmSet, bindingLayoutType );

		commandList->setPushConstants( &renderParmSet, layoutTypeAttributes[bindingLayoutType].rpBufSize );

		uniformsChanged[bindingLayoutType] = false;
	}
}
