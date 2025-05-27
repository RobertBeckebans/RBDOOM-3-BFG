/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2025 Robert Beckebans

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

#include "global_inc.hlsl"

#include "BRDF.inc.hlsl"


// *INDENT-OFF*
Texture2D t_Normal				: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_Specular			: register( t1 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_BaseColor			: register( t2 VK_DESCRIPTOR_SET( 1 ) );

Texture2D t_BrdfLut				: register( t3 VK_DESCRIPTOR_SET( 2 ) );
Texture2D t_Ssao				: register( t4 VK_DESCRIPTOR_SET( 2 ) );
Texture2D t_IrradianceCubeMap	: register( t7 VK_DESCRIPTOR_SET( 2 ) );
Texture2D t_RadianceCubeMap1	: register( t8 VK_DESCRIPTOR_SET( 2 ) );
Texture2D t_RadianceCubeMap2	: register( t9 VK_DESCRIPTOR_SET( 2 ) );
Texture2D t_RadianceCubeMap3	: register( t10 VK_DESCRIPTOR_SET( 2 ) );

SamplerState s_Material			: register( s0 VK_DESCRIPTOR_SET( 3 ) ); // (Wrap) Anisotropic sampler: normal sampler & specular sampler
SamplerState s_LinearClamp		: register( s1 VK_DESCRIPTOR_SET( 3 ) ); // (Clamp) Linear sampler: brdf lut sampler & ssao sampler
//SamplerState s_Light			: register( s2 VK_DESCRIPTOR_SET( 3 )); // (Clamp) Anisotropic sampler: irradiance, radiance 1, 2 and 3.

struct PS_IN
{
	float4 position		: SV_Position;
	float4 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
	float4 texcoord2	: TEXCOORD2_centroid;
	float4 texcoord3	: TEXCOORD3_centroid;
	float4 texcoord4	: TEXCOORD4_centroid;
	float4 texcoord5	: TEXCOORD5_centroid;
	float4 texcoord6	: TEXCOORD6_centroid;
	float4 texcoord7	: TEXCOORD7_centroid;
	float4 color		: COLOR0;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*




void main( PS_IN fragment, out PS_OUT result )
{
	float2 baseUV = fragment.texcoord1.xy;
	float2 bumpUV = fragment.texcoord0.xy;
	float2 specUV = fragment.texcoord2.xy;

	// PSX affine texture mapping
	if( rpPSXDistortions.z > 0.0 )
	{
		baseUV /= fragment.texcoord0.z;
		bumpUV /= fragment.texcoord0.z;
		specUV /= fragment.texcoord0.z;
	}

	float4 bumpMap =		t_Normal.Sample( s_Material, bumpUV );
	float4 YCoCG =		t_BaseColor.Sample( s_Material, baseUV );
	float4 specMapSRGB =	t_Specular.Sample( s_Material, specUV );
	float4 specMap =		sRGBAToLinearRGBA( specMapSRGB );

	float3 diffuseMap = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	float3 localNormal;
#if defined(USE_NORMAL_FMT_RGB8)
	localNormal.xy = bumpMap.rg - 0.5;
#else
	localNormal.xy = bumpMap.wy - 0.5;
#endif
	localNormal.z = sqrt( abs( dot( localNormal.xy, localNormal.xy ) - 0.25 ) );
	localNormal = normalize( localNormal );

	float3 globalNormal;
	globalNormal.x = dot3( localNormal, fragment.texcoord4 );
	globalNormal.y = dot3( localNormal, fragment.texcoord5 );
	globalNormal.z = dot3( localNormal, fragment.texcoord6 );
	globalNormal = normalize( globalNormal );

	float3 globalPosition = fragment.texcoord7.xyz;

	float3 globalView = normalize( rpGlobalEyePos.xyz - globalPosition );

	float3 reflectionVector = globalNormal * dot3( globalView, globalNormal );
	reflectionVector = normalize( ( reflectionVector * 2.0f ) - globalView );

#if 0
	// parallax box correction using portal area bounds
	float hitScale = 0.0;
	float3 bounds[2];
	bounds[0].x = rpWobbleSkyX.x;
	bounds[0].y = rpWobbleSkyX.y;
	bounds[0].z = rpWobbleSkyX.z;

	bounds[1].x = rpWobbleSkyY.x;
	bounds[1].y = rpWobbleSkyY.y;
	bounds[1].z = rpWobbleSkyY.z;

	// global fragment position
	float3 rayStart = fragment.texcoord7.xyz;

	// we can't start inside the box so move this outside and use the reverse path
	rayStart += reflectionVector * 10000.0;

	// only do a box <-> ray intersection test if we use a local cubemap
	if( ( rpWobbleSkyX.w > 0.0 ) && AABBRayIntersection( bounds, rayStart, -reflectionVector, hitScale ) )
	{
		float3 hitPoint = rayStart - reflectionVector * hitScale;

		// rpWobbleSkyZ is cubemap center
		reflectionVector = hitPoint - rpWobbleSkyZ.xyz;
	}
#endif

	float vDotN = saturate( dot3( globalView, globalNormal ) );

#if USE_PBR
	const float metallic = specMapSRGB.g;
	const float roughness = specMapSRGB.r;
	const float glossiness = 1.0 - roughness;
	float ao = specMapSRGB.b;

	// the vast majority of real-world materials (anything not metal or gems) have F(0)
	// values in a very narrow range (~0.02 - 0.08)

	// approximate non-metals with linear RGB 0.04 which is 0.08 * 0.5 (default in UE4)
	const float3 dielectricColor = _float3( 0.04 );

	// derive diffuse and specular from albedo(m) base color
	const float3 baseColor = diffuseMap;

	float3 diffuseColor = baseColor * ( 1.0 - metallic );
	float3 specularColor = lerp( dielectricColor, baseColor, metallic );

#if defined( DEBUG_PBR )
	diffuseColor = float3( 0.0, 0.0, 0.0 );
	specularColor = float3( 0.0, 1.0, 0.0 );
#endif

	float3 kS = Fresnel_SchlickRoughness( specularColor, vDotN, roughness );
	float3 kD = ( float3( 1.0, 1.0, 1.0 ) - kS ) * ( 1.0 - metallic );

#else

	float ao = 1.0;

#if KENNY_PBR
	float3 diffuseColor = diffuseMap;
	float3 specularColor;
	float roughness;

	PBRFromSpecmap( specMapSRGB.rgb, specularColor, roughness );
#else
	const float roughness = EstimateLegacyRoughness( specMapSRGB.rgb );

	float3 diffuseColor = diffuseMap;
	float3 specularColor = specMap.rgb;
#endif

#if defined( DEBUG_PBR )
	diffuseColor = float3( 0.0, 0.0, 0.0 );
	specularColor = float3( 1.0, 0.0, 0.0 );
#endif

	float3 kS = Fresnel_SchlickRoughness( specularColor, vDotN, roughness );

	// NOTE: metalness is missing
	float3 kD = ( float3( 1.0, 1.0, 1.0 ) - kS );

#endif

	//diffuseColor = float3( 1.0, 1.0, 1.0 );
	//diffuseColor = float3( 0.0, 0.0, 0.0 );

	// calculate the screen texcoord in the 0.0 to 1.0 range
	//float2 screenTexCoord = vposToScreenPosTexCoord( fragment.position.xy );
	float2 screenTexCoord = fragment.position.xy * rpWindowCoord.xy;

	ao = min( ao,  t_Ssao.Sample( s_LinearClamp, screenTexCoord ).r );

	// evaluate diffuse IBL

	float2 normalizedOctCoord = octEncode( globalNormal );
	float2 normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0, 1.0 ) ) * 0.5;

// lightgrid atlas

	//float3 lightGridOrigin = float3( -192.0, -128.0, 0 );
	//float3 lightGridSize = float3( 64.0, 64.0, 128.0 );
	//int3 lightGridBounds = int3( 7, 7, 3 );

	float3 lightGridOrigin = rpGlobalLightOrigin.xyz;
	float3 lightGridSize = rpJitterTexScale.xyz;
	int3 lightGridBounds = int3( rpJitterTexOffset.x, rpJitterTexOffset.y, rpJitterTexOffset.z );

	float invXZ = ( 1.0 / ( lightGridBounds[0] * lightGridBounds[2] ) );
	float invY = ( 1.0 / lightGridBounds[1] );

	normalizedOctCoordZeroOne.x *= invXZ;
	normalizedOctCoordZeroOne.y *= invY;

	int3 gridCoord = int3( 0, 0, 0 );
	float3 frac;
	float3 lightOrigin = globalPosition - lightGridOrigin;

	for( int j = 0; j < 3; j++ )
	{
		float           v;

		// walls can be sampled behind the grid sometimes so avoid negative weights
		v = max( 0, lightOrigin[j] * ( 1.0 / lightGridSize[j] ) );
		gridCoord[j] = int( floor( v ) );
		frac[ j ] = v - gridCoord[ j ];

		if( gridCoord[j] < 0 )
		{
			gridCoord[j] = 0;
		}
		else if( gridCoord[j] >= lightGridBounds[j] - 1 )
		{
			gridCoord[j] = lightGridBounds[j] - 1;
		}
	}

	// trilerp the light value
	int3 gridStep;

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	float totalFactor = 0.0;
	float3 irradiance = float3( 0.0, 0.0, 0.0 );

	/*
	for( int i = 0; i < 8; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			if( i & ( 1 << j ) )

		results in these offsets
	*/
	const float3 cornerOffsets[8] =
	{
		float3( 0.0, 0.0, 0.0 ),
		float3( 1.0, 0.0, 0.0 ),
		float3( 0.0, 2.0, 0.0 ),
		float3( 1.0, 2.0, 0.0 ),
		float3( 0.0, 0.0, 4.0 ),
		float3( 1.0, 0.0, 4.0 ),
		float3( 0.0, 2.0, 4.0 ),
		float3( 1.0, 2.0, 4.0 )
	};

	for( int i = 0; i < 8; i++ )
	{
		float factor = 1.0;

		int3 gridCoord2 = gridCoord;

		for( int j = 0; j < 3; j++ )
		{
			if( cornerOffsets[ i ][ j ] > 0.0 )
			{
				factor *= frac[ j ];

				gridCoord2[ j ] += 1;
			}
			else
			{
				factor *= ( 1.0 - frac[ j ] );
			}
		}

		// build P
		//float3 P = lightGridOrigin + ( gridCoord2[0] * gridStep[0] + gridCoord2[1] * gridStep[1] + gridCoord2[2] * gridStep[2] );

		float2 atlasOffset;

		atlasOffset.x = ( gridCoord2[0] * gridStep[0] + gridCoord2[2] * gridStep[1] ) * invXZ;
		atlasOffset.y = ( gridCoord2[1] * invY );

		// offset by one pixel border bleed size for linear filtering
#if 1
		// rpScreenCorrectionFactor.w = probeSize factor accounting account offset border, e.g = ( 16 / 18 ) = 0.8888
		float2 octCoordNormalizedToTextureDimensions = ( normalizedOctCoordZeroOne + atlasOffset ) * rpScreenCorrectionFactor.w;

		// skip by default 2 pixels for each grid cell and offset the start position by (1,1)
		// rpScreenCorrectionFactor.z = borderSize e.g = 2
		float2 probeTopLeftPosition;
		probeTopLeftPosition.x = ( gridCoord2[0] * gridStep[0] + gridCoord2[2] * gridStep[1] ) * rpScreenCorrectionFactor.z + rpScreenCorrectionFactor.z * 0.5;
		probeTopLeftPosition.y = ( gridCoord2[1] ) * rpScreenCorrectionFactor.z + rpScreenCorrectionFactor.z * 0.5;

		float2 normalizedProbeTopLeftPosition = probeTopLeftPosition * rpCascadeDistances.zw;

		float2 atlasCoord = normalizedProbeTopLeftPosition + octCoordNormalizedToTextureDimensions;
#else
		float2 atlasCoord = normalizedOctCoordZeroOne + atlasOffset;
#endif

		float3 color = t_IrradianceCubeMap.Sample( s_LinearClamp, atlasCoord, 0 ).rgb;

		if( ( color.r + color.g + color.b ) < 0.0001 )
		{
			// ignore samples in walls
			continue;
		}

		irradiance += color * factor;
		totalFactor += factor;
	}

	if( totalFactor > 0.0 && totalFactor < 0.9999 )
	{
		totalFactor = 1.0 / totalFactor;

		irradiance *= totalFactor;
	}

// lightgrid atlas


	float3 diffuseLight = ( kD * irradiance * diffuseColor ) * ao * ( rpDiffuseModifier.xyz * 1.0 );

	// evaluate specular IBL

	// 512^2 = 10 mips
	// however we can't use the last 3 mips with octahedrons because the quality suffers too much
	// so it is 7 - 1
	const float MAX_REFLECTION_LOD = 6.0;
	float mip = clamp( ( roughness * MAX_REFLECTION_LOD ), 0.0, MAX_REFLECTION_LOD );
	//float mip = 0.0;

	normalizedOctCoord = octEncode( reflectionVector );
	normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0, 1.0 ) ) * 0.5;

	float3 radiance = t_RadianceCubeMap1.SampleLevel( s_LinearClamp, normalizedOctCoordZeroOne, mip ).rgb * rpLocalLightOrigin.x;
	radiance += t_RadianceCubeMap2.SampleLevel( s_LinearClamp, normalizedOctCoordZeroOne, mip ).rgb * rpLocalLightOrigin.y;
	radiance += t_RadianceCubeMap3.SampleLevel( s_LinearClamp, normalizedOctCoordZeroOne, mip ).rgb * rpLocalLightOrigin.z;
	//radiance = float3( 0.0 );

	float2 envBRDF  = t_BrdfLut.Sample( s_LinearClamp, float2( max( vDotN, 0.0 ), roughness ) ).rg;

#if 0
	result.color.rgb = float3( envBRDF.x, envBRDF.y, 0.0 );
	result.color.w = fragment.color.a;
	return;
#endif

	float specAO = ComputeSpecularAO( vDotN, ao, roughness );
	float3 specularLight = radiance * ( kS * envBRDF.x + envBRDF.y ) * specAO * ( rpSpecularModifier.xyz * 1.0 );

	// Marmoset Horizon Fade trick
	const half horizonFade = 1.3;
	half horiz = saturate( 1.0 + horizonFade * saturate( dot3( reflectionVector, globalNormal ) ) );
	horiz *= horiz;
	//horiz = clamp( horiz, 0.0, 1.0 );

	//float3 lightColor = sRGBToLinearRGB( rpAmbientColor.rgb );
	float3 lightColor = ( rpAmbientColor.rgb );

	//result.color.rgb = diffuseLight;
	//result.color.rgb = diffuseLight * lightColor;
	//result.color.rgb = specularLight;
	result.color.rgb = ( diffuseLight + specularLight * horiz ) * lightColor * fragment.color.rgb;
	//result.color.rgb = localNormal.xyz * 0.5 + 0.5;
	//result.color.rgb = float3( ao );
	result.color.w = fragment.color.a;
}
