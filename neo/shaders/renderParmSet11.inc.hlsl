/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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

#include "vulkan.hlsli"

struct renderParmSet11_t
{
	float4 rpScreenCorrectionFactor;
	float4 rpDiffuseModifier;
	float4 rpSpecularModifier;

	float4 rpLocalLightOrigin;
	float4 rpLocalViewOrigin;

	float4 rpLightProjectionS;
	float4 rpLightProjectionT;
	float4 rpLightProjectionQ;
	float4 rpLightFalloffS;

	float4 rpBumpMatrixS;
	float4 rpBumpMatrixT;

	float4 rpDiffuseMatrixS;
	float4 rpDiffuseMatrixT;

	float4 rpSpecularMatrixS;
	float4 rpSpecularMatrixT;

	float4 rpVertexColorModulate;
	float4 rpVertexColorAdd;

	float4 rpMVPmatrixX;
	float4 rpMVPmatrixY;
	float4 rpMVPmatrixZ;
	float4 rpMVPmatrixW;

	float4 rpModelMatrixX;
	float4 rpModelMatrixY;
	float4 rpModelMatrixZ;
	float4 rpModelMatrixW;

	float4 rpProjectionMatrixW;

	float4 rpModelViewMatrixX;
	float4 rpModelViewMatrixY;
	float4 rpModelViewMatrixZ;
	float4 rpModelViewMatrixW;

	float4 rpGlobalLightOrigin;
	float4 rpJitterTexScale;
	float4 rpJitterTexOffset;
	float4 rpPSXDistortions;
	float4 rpCascadeDistances;

	float4 rpShadowMatrices[6 * 4];
	float4 rpShadowAtlasOffsets[6];
};

#if USE_PUSH_CONSTANTS

VK_PUSH_CONSTANT ConstantBuffer<renderParmSet11_t> pc : register( b0 );

#else

cbuffer pc : register( b0 VK_DESCRIPTOR_SET( 0 ) )
{
	renderParmSet11_t pc;
}

#endif
