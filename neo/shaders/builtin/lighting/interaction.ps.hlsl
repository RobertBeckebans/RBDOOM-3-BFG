/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2024 Robert Beckebans
Copyright (C) 2026 Phillip Melnichenko

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
#include "renderParmSet10.inc.hlsl"
#include "BRDF.inc.hlsl"
#include "mathlight.inc.hlsl"


// *INDENT-OFF*
Texture2D				t_Normal			: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D				t_Specular			: register( t1 VK_DESCRIPTOR_SET( 1 ) );
Texture2D				t_BaseColor			: register( t2 VK_DESCRIPTOR_SET( 1 ) );

Texture2D				t_LightFalloff		: register( t3 VK_DESCRIPTOR_SET( 2 ) );
Texture2D				t_LightProjection	: register( t4 VK_DESCRIPTOR_SET( 2 ) );

SamplerState			s_Material : register( s0 VK_DESCRIPTOR_SET( 3 ) ); // for the normal/specular/basecolor
SamplerState 			s_Lighting : register( s1 VK_DESCRIPTOR_SET( 3 ) ); // for sampling the jitter

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
	float4 color		: COLOR0;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
	float2 baseUV = fragment.texcoord4.xy;
	float2 bumpUV = fragment.texcoord1.xy;
	float2 specUV = fragment.texcoord5.xy;

	// PSX affine texture mapping
	if( pc.rpPSXDistortions.z > 0.0 )
	{
		baseUV /= fragment.texcoord1.z;
		bumpUV /= fragment.texcoord1.z;
		specUV /= fragment.texcoord1.z;
	}

	float4 bumpMap =		t_Normal.Sample( s_Material, bumpUV );
    float3 lightFalloffRGB;
    float3 lightProjRGB;
    if (!MLEvaluate(
        fragment.texcoord2, fragment.texcoord3,
        pc.rpUser0, pc.rpUser1, pc.rpUser2,
        lightFalloffRGB, lightProjRGB))
    {
    // legacy path
        lightFalloffRGB = idtex2Dproj(s_Lighting, t_LightFalloff, fragment.texcoord2).xyz;
        lightProjRGB = idtex2Dproj(s_Lighting, t_LightProjection, fragment.texcoord3).xyz;
    }
	float4 YCoCG =			t_BaseColor.Sample( s_Material, baseUV );
	float4 specMapSRGB =	t_Specular.Sample( s_Material, specUV );
	float4 specMap =		sRGBAToLinearRGBA( specMapSRGB );

	float3 lightVector = normalize( fragment.texcoord0.xyz );
	float3 viewVector = normalize( fragment.texcoord6.xyz );
	float3 diffuseMap = sRGBToLinearRGB( ConvertYCoCgToRGB( YCoCG ) );

	float3 localNormal;
	// RB begin
#if USE_NORMAL_FMT_RGB8
	localNormal.xy = bumpMap.rg - 0.5;
#else
	localNormal.xy = bumpMap.wy - 0.5;
#endif
	// RB end
	localNormal.z = sqrt( abs( dot( localNormal.xy, localNormal.xy ) - 0.25 ) );
	localNormal = normalize( localNormal );
	
	// View-dependent normal contrast
    const float vdContrastMin = 0.8; // contrast head-on  (1.0 = no change, <1 flattens, >1 amplifies)
    const float vdContrastMax = 1.2; // contrast at grazing
{
        float NdotV = saturate(viewVector.z);
        float grazing = 1.0 - NdotV; // 0 head-on, 1 grazing
        float contrast = lerp(vdContrastMin, vdContrastMax, grazing);
        localNormal.xy *= contrast;
        localNormal = normalize(localNormal);
    }

	// traditional very dark Lambert light model used in Doom 3
	float ldotN = saturate( dot3( localNormal, lightVector ) );

#if defined(USE_HALF_LAMBERT)
	// RB: http://developer.valvesoftware.com/wiki/Half_Lambert
	float halfLdotN = dot3( localNormal, lightVector ) * 0.5 + 0.5;
	halfLdotN *= halfLdotN;

	// tweak to not loose so many details
	float lambert = lerp( ldotN, halfLdotN, 0.5 );
#else
	float lambert = ldotN;
#endif


	float3 halfAngleVector = normalize( lightVector + viewVector );
	float hdotN = clamp( dot3( halfAngleVector, localNormal ), 0.0, 1.0 );

#if USE_PBR
	// RB: roughness 0 somehow is not shiny so we clamp it
	float roughness = max( 0.045, specMapSRGB.r );
	const float metallic = specMapSRGB.g;

	// the vast majority of real-world materials (anything not metal or gems) have F(0)
	// values in a very narrow range (~0.02 - 0.08)

	// approximate non-metals with linear RGB 0.04 which is 0.08 * 0.5 (default in UE4)
	const float3 dielectricColor = _float3( 0.04 );

	// derive diffuse and specular from albedo(m) base color
	const float3 baseColor = diffuseMap;

	float3 diffuseColor = baseColor * ( 1.0 - metallic );
	float3 specularColor = lerp( dielectricColor, baseColor, metallic );

// legacy fudgefactor spec+bump wrangling into "PBR", leave it alone unless touching backwards compat
#elif KENNY_PBR
	float3 diffuseColor = diffuseMap;
	float3 specularColor;
	float roughness;

	PBRFromSpecmap( specMapSRGB.rgb, specularColor, roughness );
#else
	const float roughness = EstimateLegacyRoughness( specMapSRGB.rgb );

	float3 diffuseColor = diffuseMap;
	float3 specularColor = specMapSRGB.rgb; // RB: should be linear but it looks too flat
#endif


	// RB FIXME or not: compensate r_lightScale 3 and the division of Pi
	//lambert *= 1.3;
	// see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//lambert /= PI;

	// pc.rpDiffuseModifier contains light color multiplier
    float3 lightColor = sRGBToLinearRGB(lightProjRGB * lightFalloffRGB);

	float vdotN = clamp( dot3( viewVector, localNormal ), 0.0, 1.0 );
	float vdotH = clamp( dot3( viewVector, halfAngleVector ), 0.0, 1.0 );
	float ldotH = clamp( dot3( lightVector, halfAngleVector ), 0.0, 1.0 );


    float rr = roughness * roughness;
    float rrrr = rr * rr;

	// Disney/GGX NDF
    float D_denom = (hdotN * hdotN) * (rrrr - 1.0) + 1.0;
    float D = rrrr / (3.14159f * D_denom * D_denom);

	// Hammon height-correlated Smith G2 approximation
    float Vis = Visibility_SmithGGXHammon(vdotN, ldotN, rr);

	// Schlick Fresnel at the micro-facet vdotH is deliberate
    float3 F = Fresnel_Schlick(specularColor, vdotH);

// legacy fudgefactor spec+bump wrangling into "PBR", leave it alone unless touching backwards compat
#if KENNY_PBR
	float3 specularLight = D * Vis * F * ldotN * PI * pc.rpSpecularModifier.rgb;
#else
    float3 specularLight = D * Vis * F * ldotN * pc.rpSpecularModifier.rgb;
#endif

	// multiple-scattering energy compensation  no DFG LUT in the direct path
    float singleScatterEnergy = EnvBRDFEnergyApprox(roughness, vdotN);
    float3 energyCompensation = 1.0 + specularColor * (1.0 / max(singleScatterEnergy, 1e-3) - 1.0);
    specularLight *= energyCompensation;

#if 0
	result.color = float4( _float3( VFapprox ), 1.0 );
	return;
#endif

	//float3 diffuseColor = mix( diffuseMap, F0, metal ) * pc.rpDiffuseModifier.xyz;
	float3 diffuseLight = diffuseColor * lambert * ( pc.rpDiffuseModifier.xyz );

	float3 color = ( diffuseLight + specularLight ) * lightColor * fragment.color.rgb;

	result.color.rgb = color;
	result.color.a = 1.0;
}
