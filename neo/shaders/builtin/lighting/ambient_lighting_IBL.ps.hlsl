/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2025 Robert Beckebans
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
#include "renderParmSet9.inc.hlsl"

#include "BRDF.inc.hlsl"


// *INDENT-OFF*
Texture2D t_Normal				: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_Specular			: register( t1 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_BaseColor			: register( t2 VK_DESCRIPTOR_SET( 1 ) );

Texture2D t_BrdfLut				: register( t3 VK_DESCRIPTOR_SET( 2 ) );
Texture2D t_Ssao				: register( t4 VK_DESCRIPTOR_SET( 2 ) );
//Texture2D t_IrradianceCubeMap	: register( t7 VK_DESCRIPTOR_SET( 2 ) );    //no longer used
TextureCube t_RadianceCubeMap1	: register( t8 VK_DESCRIPTOR_SET( 2 ) );
TextureCube t_RadianceCubeMap2	: register( t9 VK_DESCRIPTOR_SET( 2 ) );
TextureCube t_RadianceCubeMap3	: register( t10 VK_DESCRIPTOR_SET( 2 ) );
TextureCube t_DepthCubeMap		: register (t12 VK_DESCRIPTOR_SET( 2 ) );
TextureCube t_DepthCubeMap2		: register( t13 VK_DESCRIPTOR_SET( 2 ) );	// inter-probe distance 1
TextureCube t_DepthCubeMap3		: register( t14 VK_DESCRIPTOR_SET( 2 ) );	// inter-probe distance 2
#define NUM_RADIANCE_MIPS 10

SamplerState s_Material			: register( s0 VK_DESCRIPTOR_SET( 3 ) ); // (Wrap) Anisotropic sampler: normal sampler & specular sampler
SamplerState s_LinearClamp		: register( s1 VK_DESCRIPTOR_SET( 3 ) ); // (Clamp) Linear sampler: brdf lut sampler & ssao sampler
//SamplerState s_Light			: register( s2 VK_DESCRIPTOR_SET( 3 )); // (Clamp) Anisotropic sampler: irradiance, radiance 1, 2 and 3.

float3 ParallaxCorrectAndOcclude(float3 worldPos, float3 reflDir, float3 probeCentre,
								  float probeFarDist, float roughness, out float occlusion, out float hitDist)
{
    occlusion = 1.0;
    hitDist = probeFarDist;

	// the lookup is blurry anyway
    const int steps = (int) lerp(16.0, 4.0, roughness);
    const float stepLen = probeFarDist / max((float) steps, 1.0);

    float3 rayPos = worldPos + reflDir * (stepLen * 0.5);

    float3 dSeed = rayPos - probeCentre;
    float prevDiff = length(dSeed) - t_DepthCubeMap.SampleLevel(s_LinearClamp, normalize(dSeed), 0).r;

	[loop]
    for (int i = 0; i < steps; i++)
    {
        float3 nextPos = rayPos + reflDir * stepLen;
        float3 d = nextPos - probeCentre;
        float rayDist = length(d);
        float scnDist = t_DepthCubeMap.SampleLevel(s_LinearClamp, normalize(d), 0).r;
        float diff = rayDist - scnDist; 

        if (diff > 0.0)
        {
            float t = saturate(prevDiff / min(prevDiff - diff, -1e-4));
            float3 hitPos = lerp(rayPos, nextPos, t);

            hitDist = length(hitPos - worldPos);
            occlusion = saturate(hitDist / (probeFarDist * 0.5));

            return normalize(hitPos - probeCentre);
        }

        rayPos = nextPos;
        prevDiff = diff;
    }
    return reflDir;
}



// coarse (distances are from the probe centre, not the shading point)
// returns a bent normal 
#define NUM_AO_TAPS 6
float LargeScaleDepthAO(float3 worldPos, float3 N, float3 probeCentre, float probeFarDist, out float3 bentNormal)
{
	// tangent basis around N
    float3 up = (abs(N.z) < 0.999) ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 Bt = cross(N, T);

	// tangent-plane offsets; index 0 points straight up the normal
    const float2 kernel[NUM_AO_TAPS] =
    {
        float2(0.00, 0.00),
		float2(0.66, 0.00),
		float2(-0.33, 0.57),
		float2(-0.33, -0.57),
		float2(0.30, 0.52),
		float2(-0.60, -0.20),
    };

    const float radius = probeFarDist * 0.25;			// occlusion reach    (tunable)
    const float falloff = probeFarDist * 0.25;			// transition softness (tunable)

    float visSum = 0.0;
    float wSum = 0.0;
    float3 bent = float3(0, 0, 0);

	[unroll]
    for (int i = 0; i < NUM_AO_TAPS; i++)
    {
        float3 dir = normalize(N + T * kernel[i].x + Bt * kernel[i].y);
        float w = saturate(dot3(dir, N));				// cosine weight

        float3 p = worldPos + dir * radius;
        float3 fromC = p - probeCentre;
        float scnD = t_DepthCubeMap.SampleLevel(s_LinearClamp, normalize(fromC), 0).r;
        float behind = length(fromC) - scnD;			// > 0 => p is past the surface => occluded
        float vis = saturate(-behind / falloff);		// open space => ~1; behind geometry => 0

        visSum += vis * w;
        wSum += w;
        bent += dir * (vis * w);
    }

    bentNormal = (wSum > 0.0) ? normalize(bent + N * 1e-4) : N;
    return (wSum > 0.0) ? saturate(visSum / wSum) : 1.0;
}

// can this probe see worldPos? 1 = fully visible, -> 0 = behind geometry.
// Bias keeps points lying ON a surface visible; fade softens the wall transition.
float ProbeVisibility(TextureCube depthCube, float3 worldPos, float3 probePos, float probeFarDist)
{
    float3 toP = worldPos - probePos;
    float dP = length(toP);
    float scn = depthCube.SampleLevel(s_LinearClamp, normalize(toP), 0).r;

    const float bias = probeFarDist * 0.02;				// tunable
    const float fade = probeFarDist * 0.10;				// tunable
    return saturate(1.0 - (dP - scn - bias) / fade);
}

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



void shEvaluateL2(float3 d, out float y[9])
{
    float x = d.x, yv = d.y, z = d.z;
    float x2 = x * x, y2 = yv * yv, z2 = z * z;

    y[0] = 1.0 / ( 2.0 * sqrt( 3.1415927 ) );
    y[1] = -sqrt( 3.0 / ( 4.0 * 3.1415927 ) ) * yv;
    y[2] = sqrt( 3.0 / ( 4.0 * 3.1415927 ) ) * z;
    y[3] = -sqrt( 3.0 / ( 4.0 * 3.1415927 ) ) * x;
    y[4] = sqrt( 15.0 / ( 4.0 * 3.1415927 ) ) * yv * x;
    y[5] = -sqrt( 15.0 / ( 4.0 * 3.1415927 ) ) * yv * z;
    y[6] = sqrt( 5.0 / ( 16.0 * 3.1415927 ) ) * (3.0 * z2 - 1.0);
    y[7] = -sqrt( 15.0 / ( 4.0 * 3.1415927 ) ) * x * z;
    y[8] = sqrt( 15.0 / ( 16.0 * 3.1415927 ) ) * (x2 - y2);
}

// c[9] = the stored RGB coefficients (first 9 / L2). Returns irradiance E(n).
float3 shEvaluateDiffuseL2RGB(float3 c[9], float3 n)
{
    float b[9];
    shEvaluateL2(n, b);

    const float A0 = 3.1415927;              // SphericalHarmonics
    const float A1 = 3.1415927 * 2.0 / 3.0;
    const float A2 = 3.1415927 * 1.0 / 4.0;

    float3 r = c[0] * b[0] * A0;
    r += (c[1] * b[1] + c[2] * b[2] + c[3] * b[3]) * A1;
    r += (c[4] * b[4] + c[5] * b[5] + c[6] * b[6] + c[7] * b[7] + c[8] * b[8]) * A2;
    return r;
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 baseUV = fragment.texcoord1.xy;
	float2 bumpUV = fragment.texcoord0.xy;
	float2 specUV = fragment.texcoord2.xy;

	// PSX affine texture mapping
	if( pc.rpPSXDistortions.z > 0.0 )
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

    float3 globalPosition = fragment.texcoord7.xyz;
    float3 globalView = normalize(pc.rpGlobalEyePos.xyz - globalPosition);

	// View-dependent normal contrast 
    const float vdContrastMin = 1.0;
    const float vdContrastMax = 1.4;
	{
		
        float3 globalNormalPre;
        globalNormalPre.x = dot3(localNormal, fragment.texcoord4);
        globalNormalPre.y = dot3(localNormal, fragment.texcoord5);
        globalNormalPre.z = dot3(localNormal, fragment.texcoord6);
        globalNormalPre = normalize(globalNormalPre);

        float NdotV = saturate(dot3(globalNormalPre, globalView));
        float grazing = 1.0 - NdotV;
        float contrast = lerp(vdContrastMin, vdContrastMax, grazing);
        localNormal.xy *= contrast;
        localNormal = normalize(localNormal);
    }

    float3 globalNormal;
    globalNormal.x = dot3(localNormal, fragment.texcoord4);
    globalNormal.y = dot3(localNormal, fragment.texcoord5);
    globalNormal.z = dot3(localNormal, fragment.texcoord6);
    globalNormal = normalize(globalNormal);

	float3 reflectionVector = globalNormal * dot3( globalView, globalNormal );
	reflectionVector = normalize( ( reflectionVector * 2.0f ) - globalView );

#if 0
	// parallax box correction using portal area bounds
	float hitScale = 0.0;
	float3 bounds[2];
	bounds[0].x = pc.rpWobbleSkyX.x;
	bounds[0].y = pc.rpWobbleSkyX.y;
	bounds[0].z = pc.rpWobbleSkyX.z;

	bounds[1].x = pc.rpWobbleSkyY.x;
	bounds[1].y = pc.rpWobbleSkyY.y;
	bounds[1].z = pc.rpWobbleSkyY.z;

	// global fragment position
	float3 rayStart = fragment.texcoord7.xyz;

	// we can't start inside the box so move this outside and use the reverse path
	rayStart += reflectionVector * 10000.0;

	// only do a box <-> ray intersection test if we use a local cubemap
	if( ( pc.rpWobbleSkyX.w > 0.0 ) && AABBRayIntersection( bounds, rayStart, -reflectionVector, hitScale ) )
	{
		float3 hitPoint = rayStart - reflectionVector * hitScale;

		// pc.rpWobbleSkyZ is cubemap center
		reflectionVector = hitPoint - pc.rpWobbleSkyZ.xyz;
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
	float3 kD = ( float3( 1.0, 1.0, 1.0 ) - kS );

#else

	float ao = 1.0;

// legacy fudgefactor spec+bump wrangling into "PBR", leave it alone unless touching backwards compat
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
	float2 screenTexCoord = fragment.position.xy * pc.rpWindowCoord.xy;

	ao = min( ao,  t_Ssao.Sample( s_LinearClamp, screenTexCoord ).r );

	// evaluate diffuse IBL
    // Matches the bake's normalization: it stored shEvaluateDiffuse(...) / PI
	// (RenderWorld_envprobes), so divide by PI here too or be PI x too bright.
    float3 sh[9];
	[unroll]
    for (int i = 0; i < 9; i++)
    {
        sh[i] = pc.rpSHIrradiance[i].rgb;
    }
    
    float3 irradiance = max(shEvaluateDiffuseL2RGB(sh, globalNormal) / 3.14159265, 0.0);
    
    float2 envBRDF = t_BrdfLut.Sample(s_LinearClamp, float2(max(vDotN, 0.0), roughness)).rg;

	// multiple-scattering energy compensation - Fdez-Aguera
    float3 singleScatterSpec = specularColor * envBRDF.x + envBRDF.y;
    float singleScatterEnergy = envBRDF.x + envBRDF.y;
    float multiScatterEnergy = 1.0 - singleScatterEnergy;
    float3 averageFresnel = specularColor + (1.0 - specularColor) * (1.0 / 21.0);
    float3 multiScatterSpec = singleScatterSpec * averageFresnel / (1.0 - multiScatterEnergy * averageFresnel);
    float3 specularScatter = singleScatterSpec + multiScatterSpec * multiScatterEnergy;

	// energy left for diffuse after specular takes its full (multiscatter) share - replaces kD
    float3 diffuseEnergy = float3(1.0, 1.0, 1.0) - specularScatter;

    float3 diffuseLight = (diffuseEnergy * irradiance * diffuseColor) * ao * (pc.rpDiffuseModifier.xyz * 1.0);
	
	// large-scale AO folded into diffuse
    if (pc.rpWobbleSkyX.w > 0.0)
    {
        float3 bentN;
        float aoFarDist = length(pc.rpWobbleSkyY.xyz - pc.rpWobbleSkyZ.xyz);
        float largeAO = LargeScaleDepthAO(fragment.texcoord7.xyz, globalNormal, pc.rpWobbleSkyZ.xyz, aoFarDist, bentN);
        diffuseLight *= lerp(1.0, largeAO, 0.50); //third is tunable
    }

	// cube: full chain, hardware seam filtering. No octahedral mip cap
    const float MAX_REFLECTION_LOD = float(NUM_RADIANCE_MIPS - 1);
    float mip = roughness * MAX_REFLECTION_LOD;

	// parallax-correct march, per-probe reprojection, visibility-weighted blend
	// Gated on probe validity (rpWobbleSkyX.w == 1)
    float3 reflDir = reflectionVector;
    float reflOcclusion = 1.0;
    float3 blend = pc.rpLocalLightOrigin.xyz; // barycentric weights from the frontend
    float3 dir1 = reflectionVector;
    float3 dir2 = reflectionVector;
    float3 dir3 = reflectionVector;

    if ( pc.rpWobbleSkyX.w > 0.0 )
    {
        float probeFarDist = length( pc.rpWobbleSkyY.xyz - pc.rpWobbleSkyZ.xyz );
        float reflHitDist;
        reflDir = ParallaxCorrectAndOcclude( fragment.texcoord7.xyz, reflectionVector,
											 pc.rpWobbleSkyZ.xyz, probeFarDist, roughness, reflOcclusion, reflHitDist );

		// distant reflected geometry blurs with roughness (optional)
        mip = min( MAX_REFLECTION_LOD, mip + roughness * saturate( reflHitDist / probeFarDist ) * 2.0 );

		// The march walked along reflectionVector from the fragment, so hitPos sits on that ray. re-project it into each probe. On a miss,
		// reflHitDist == probeFarDist and this becomes correct finite-distance parallax.
        float3 hitPos = fragment.texcoord7.xyz + reflectionVector * reflHitDist;
        dir1 = ( pc.rpTexGen0S.w > 0.0 ) ? normalize(hitPos - pc.rpTexGen0S.xyz ) : reflDir;
        dir2 = ( pc.rpTexGen0T.w > 0.0 ) ? normalize(hitPos - pc.rpTexGen0T.xyz ) : reflDir;
        dir3 = ( pc.rpTexGen0Q.w > 0.0 ) ? normalize(hitPos - pc.rpTexGen0Q.xyz ) : reflDir;

        float v1 = ( pc.rpTexGen0S.w > 0.0 ) ? ProbeVisibility( t_DepthCubeMap, fragment.texcoord7.xyz, pc.rpTexGen0S.xyz, probeFarDist ) : 1.0;
        float v2 = ( pc.rpTexGen0T.w > 0.0 ) ? ProbeVisibility( t_DepthCubeMap2, fragment.texcoord7.xyz, pc.rpTexGen0T.xyz, probeFarDist ) : 1.0;
        float v3 = ( pc.rpTexGen0Q.w > 0.0 ) ? ProbeVisibility( t_DepthCubeMap3, fragment.texcoord7.xyz, pc.rpTexGen0Q.xyz, probeFarDist ) : 1.0;

        float3 w = blend * float3(v1, v2, v3);
        float s = w.x + w.y + w.z;
        blend = (s > 0.02) ? (w / s) : blend;
    }

	// per-probe directions and visibility-weighted blend replace the shared lookup
    float3 radiance = t_RadianceCubeMap1.SampleLevel( s_LinearClamp, dir1, mip ).rgb * blend.x;
    radiance += t_RadianceCubeMap2.SampleLevel( s_LinearClamp, dir2, mip ).rgb * blend.y;
    radiance += t_RadianceCubeMap3.SampleLevel( s_LinearClamp, dir3, mip ).rgb * blend.z;


#if 0
	result.color.rgb = float3( envBRDF.x, envBRDF.y, 0.0 );
	result.color.w = fragment.color.a;
	return;
#endif

	// Gate specular IBL by local lightgrid irradiance independent of ambient slider,
    float irradianceLum = dot(irradiance, float3(0.2126, 0.7152, 0.0722));

	// Tunable: irradiance level below which specular IBL begins to fade.
    const float specGateThreshold = 0.005;
    float specGate = smoothstep(0.0, specGateThreshold, irradianceLum);

	// Gate dielectric IBL specular to kill bleed in dim areas, 
#if USE_PBR
	float specVisibility = lerp( specGate, 1.0, metallic );
#else
    float metalnessSignal = saturate((max(max(specularColor.r, specularColor.g), specularColor.b) - 0.04) / 0.96);
    float specVisibility = lerp(specGate, 1.0, metalnessSignal);
#endif

    float specAO = ComputeSpecularAO(vDotN, ao, roughness);
    float3 specularLight = radiance * specularScatter * specAO * specVisibility * reflOcclusion * pc.rpSpecularModifier.xyz;

	// Marmoset Horizon Fade trick
    const half horizonFade = 1.3;
    half horiz = saturate( 1.0 + horizonFade * dot3(reflectionVector, globalNormal)); // signed dot
    horiz *= horiz;
	//horiz = clamp( horiz, 0.0, 1.0 );

	//float3 lightColor = sRGBToLinearRGB( pc.rpAmbientColor.rgb );
	float3 lightColor = ( pc.rpAmbientColor.rgb );

	//result.color.rgb = diffuseLight;
	//result.color.rgb = diffuseLight * lightColor;
	//result.color.rgb = specularLight;
    result.color.rgb = (diffuseLight * lightColor + specularLight * horiz) * fragment.color.rgb;
	//result.color.rgb = localNormal.xyz * 0.5 + 0.5;
	//result.color.rgb = float3( ao );
	result.color.w = fragment.color.a;
}
