/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2024 Robert Beckebans

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


// *INDENT-OFF*
Texture2D t_NormalMap			: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_ScreenColor			: register( t1 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_ScreenNormals		: register( t2 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_Depth				: register( t3 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_RadianceCubeMap1	: register( t4 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_RadianceCubeMap2	: register( t5 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_RadianceCubeMap3	: register( t6 VK_DESCRIPTOR_SET( 1 ) );

SamplerState s_Material			: register( s0 VK_DESCRIPTOR_SET( 2 ) );
SamplerState s_LinearClamp		: register( s1 VK_DESCRIPTOR_SET( 2 ) );

struct PS_IN 
{
	float4 position		: SV_Position;
	float2 texcoord0	: TEXCOORD0_centroid;
	float3 texcoord1	: TEXCOORD1_centroid;
	float3 texcoord2	: TEXCOORD2_centroid;
	float3 texcoord3	: TEXCOORD3_centroid;
	float3 texcoord4	: TEXCOORD4_centroid;
	float4 texcoord5	: TEXCOORD5_centroid;
	float4 color		: COLOR0;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*


float3 ReconstructPositionCS( int2 hitPixel )
{
	// Load returns 0 for any value accessed out of bounds
	float depth = texelFetch( t_Depth, hitPixel, 0 ).r;

	float2 uv = hitPixel * rpWindowCoord.xy;

	// derive clip space from the depth buffer and screen position
	float3 ndc = float3( uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth );
	float clipW = -rpProjectionMatrixZ.w / ( -rpProjectionMatrixZ.z - ndc.z );

	float4 clip = float4( ndc * clipW, clipW );

	// camera space position
	float4 csP;
	csP.x = dot4( rpShadowMatrices[0], clip );
	csP.y = dot4( rpShadowMatrices[1], clip );
	csP.z = dot4( rpShadowMatrices[2], clip );
	csP.w = dot4( rpShadowMatrices[3], clip );

	csP.xyz /= csP.w;
	//csP.z = abs( csP.z );	// this is still negative Z like for OpenGL

	return csP.xyz;
}


float DistanceSquared( float2 a, float2 b )
{
	a -= b;
	return dot( a, a );
}

void Swap( inout float a, inout float b )
{
	float t = a;
	a = b;
	b = t;
}

bool IntersectsDepthBuffer( float z, float minZ, float maxZ, float zThickness )
{
	/*
	 * Based on how far away from the camera the depth is,
	 * adding a bit of extra thickness can help improve some
	 * artifacts. Driving this value up too high can cause
	 * artifacts of its own.
	 */
	const float strideZCutoff = 100.0 * METERS_TO_DOOM;

	//float depthScale = min( 1.0, z * strideZCutoff );
	//z += zThickness + lerp( 0.0, 2.0, depthScale );

	//return ( maxZ >= z ) && ( minZ - zThickness <= z );

	// like original version with negative linear Z
	return ( maxZ >= z - zThickness ) && ( minZ <= z );
}

// From the Efficient GPU Screen-Space Ray Tracing paper
// By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause

// Returns true if the ray hit something
bool TraceScreenSpaceRay(
	// Camera-space ray origin, which must be within the view volume
	float3 rayStart,

	// Unit length camera-space ray direction
	float3 rayDir,

	// Camera space thickness to ascribe to each pixel in the depth buffer
	float zThickness,

	// Stride samples trades quality for performance
	float _stride,

	// Number between 0 and 1 for how far to bump the ray in stride units
	// to conceal banding artifacts. Not needed if stride == 1.
	float jitter,

	// Maximum number of iterations. Higher gives better images but may be slow
	const float maxSteps,

	// Maximum camera-space distance to trace before returning a miss
	const float maxDistance,

	// Pixel coordinates of the first intersection with the scene
	out float2 hitPixel,

	// Camera space location of the ray hit
	out float3 hitPoint,

	out float3 rayDebug )
{
	const float nearPlaneZ = 3.0;

	// Clip to the near plane
	float rayLength = ( ( rayStart.z + rayDir.z * maxDistance ) < nearPlaneZ ) ?
					  ( nearPlaneZ - rayStart.z ) / rayDir.z : maxDistance;

	//float rayLength = 10000;
	float4 rayEnd = float4( rayStart + rayDir * rayLength, 1.0 );

	// Project into homogeneous clip space
	float4 ray4D = float4( rayStart, 1.0 );
	float4 H0;
	H0.x = dot4( ray4D, rpShadowMatrices[4] );
	H0.y = dot4( ray4D, rpShadowMatrices[5] );
	H0.z = dot4( ray4D, rpShadowMatrices[6] );
	H0.w = dot4( ray4D, rpShadowMatrices[7] );

	float4 H1;
	H1.x = dot4( rayEnd, rpShadowMatrices[4] );
	H1.y = dot4( rayEnd, rpShadowMatrices[5] );
	H1.z = dot4( rayEnd, rpShadowMatrices[6] );
	H1.w = dot4( rayEnd, rpShadowMatrices[7] );

	float k0 = 1.0f / H0.w;
	float k1 = 1.0f / H1.w;

	// Switch the original points to values that interpolate linearly in 2D
	float3 Q0 = rayStart * k0;
	float3 Q1 = rayEnd.xyz * k1;

	// Screen-space endpoints
	float2 P0 = H0.xy * k0;
	float2 P1 = H1.xy * k1;

	// Initialize to off screen
	hitPixel = float2( -1.0, -1.0 );

	// If the line is degenerate, make it cover at least one pixel
	// to avoid handling zero-pixel extent as a special case later
	P1 += ( DistanceSquared( P0, P1 ) < 0.0001 ) ? float2( 0.01, 0.01 ) : 0.0;
	float2 delta = P1 - P0;

	// Permute so that the primary iteration is in x to collapse
	// all quadrant-specific DDA cases later
	bool permute = false;
	if( abs( delta.x ) < abs( delta.y ) )
	{
		// This is a more-vertical line
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	// From now on, "x" is the primary iteration direction and "y" is the secondary one
	float stepDir = sign( delta.x );
	float invdx = stepDir / delta.x;
	float2 dP = float2( stepDir, delta.y * invdx );

	// Track the derivatives of Q and k
	float3 dQ = ( Q1 - Q0 ) * invdx;
	float dk = ( k1 - k0 ) * invdx;

	const float strideZCutoff = 100.0 * METERS_TO_DOOM;

	// Scale derivatives by the desired pixel stride and then
	// offset the starting values by the jitter fraction
	//float strideScale = 1.0f - min( 1.0f, rayStart.z * strideZCutoff );
	//float stride = 1.0f + strideScale * _stride;
	float stride = _stride;
	dP *= stride;
	dQ *= stride;
	dk *= stride;

	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	// Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1
	float3 Q = Q0;
	float  k = k0;

	// We track the ray depth at +/- 1/2 pixel to treat pixels as clip-space solid
	// voxels. Because the depth at -1/2 for a given pixel will be the same as at
	// +1/2 for the previous iteration, we actually only have to compute one value
	// per iteration.
	float stepCount = 0.0;
	float prevZMaxEstimate = rayStart.z;
	float rayZMin = prevZMaxEstimate;
	float rayZMax = prevZMaxEstimate;
	float sceneZMax = rayZMax + 1.0 * METERS_TO_DOOM;

	// P1.x is never modified after this point, so pre-scale it by
	// the step direction for a signed comparison
	float end = P1.x * stepDir;

	// We only advance the z field of Q in the inner loop, since
	// Q.xy is never used until after the loop terminates.

	for( float2 P = P0;
			( ( P.x * stepDir ) <= end ) &&
			( stepCount < maxSteps ) &&
			//!IntersectsDepthBuffer( sceneZMax, rayZMin, rayZMax, zThickness ) &&
			( ( rayZMax < sceneZMax - zThickness ) || ( rayZMin > sceneZMax ) ) &&
			( sceneZMax != 0.0f );
			P += dP, Q.z += dQ.z, k += dk, stepCount += 1.0 )
	{
		hitPixel = permute ? P.yx : P;

		// The depth range that the ray covers within this loop
		// iteration.  Assume that the ray is moving in increasing z
		// and swap if backwards.  Because one end of the interval is
		// shared between adjacent iterations, we track the previous
		// value and then swap as needed to ensure correct ordering
		rayZMin = prevZMaxEstimate;

		// Compute the value at 1/2 pixel into the future
		rayZMax = ( dQ.z * 0.5 + Q.z ) / ( dk * 0.5 + k );
		prevZMaxEstimate = rayZMax;

		if( rayZMin > rayZMax )
		{
			Swap( rayZMin, rayZMax );
		}

		// You may need hitPixel.y = depthBufferSize.y - hitPixel.y; here if your vertical axis
		// is different than ours in screen space
		//hitPixel.x = rpWindowCoord.z - hitPixel.x;
		//hitPixel.y = rpWindowCoord.w - hitPixel.y;

		sceneZMax = ReconstructPositionCS( hitPixel ).z;
	}

	// Advance Q based on the number of steps
	Q.xy += dQ.xy * stepCount;
	hitPoint = Q * ( 1.0f / k );

	//rayDebug.xyz = _float3( stepCount );

	return IntersectsDepthBuffer( sceneZMax, rayZMin, rayZMax, zThickness );
}


float2 GetSampleVector( float3 reflectionVector )
{
	float2 normalizedOctCoord = octEncode( reflectionVector );
	float2 normalizedOctCoordZeroOne = ( normalizedOctCoord + _float2( 1.0 ) ) * 0.5;

	return normalizedOctCoordZeroOne;
}

void main( PS_IN fragment, out PS_OUT result )
{
	float4 bump = t_NormalMap.Sample( s_Material, fragment.texcoord0 ) * 2.0f - 1.0f;

	// RB begin
	float3 localNormal;
#if defined(USE_NORMAL_FMT_RGB8)
	localNormal = float3( bump.rg, 0.0f );
#else
	localNormal = float3( bump.wy, 0.0f );
#endif
	// RB end
	localNormal.z = sqrt( 1.0f - dot3( localNormal, localNormal ) );

	float3 globalNormal;

	globalNormal.x = dot3( localNormal, fragment.texcoord2 );
	globalNormal.y = dot3( localNormal, fragment.texcoord3 );
	globalNormal.z = dot3( localNormal, fragment.texcoord4 );

	float3 screenNormalWS = ( ( 2.0 * t_ScreenNormals.Sample( s_LinearClamp, fragment.position.xy * rpWindowCoord.xy ).rgb ) - 1.0 );

	// https://blog.selfshadow.com/publications/blending-in-detail/

	// UDN blending
	//globalNormal = normalize( float3( screenNormalWS.xy + globalNormal.xy, screenNormalWS.z ) );

	// Whiteout blending
	globalNormal = normalize( float3( screenNormalWS.xy + globalNormal.xy, screenNormalWS.z * globalNormal.z ) );


	float3 globalPosition = fragment.texcoord5.xyz;

	float3 globalView = normalize( globalPosition - rpGlobalEyePos.xyz );

	float3 reflectionVector = reflect( globalView, globalNormal );
	reflectionVector = normalize( reflectionVector );

	float2 octCoord0 = GetSampleVector( reflectionVector );
	float2 octCoord1 = octCoord0;
	float2 octCoord2 = octCoord0;

	float3 rayStart = globalPosition;

#if 1
	// parallax box correction using portal area bounds
	float hitScale = 0.0;
	float3 bounds[2];
	bounds[0].x = rpWobbleSkyX.x;
	bounds[0].y = rpWobbleSkyX.y;
	bounds[0].z = rpWobbleSkyX.z;

	bounds[1].x = rpWobbleSkyY.x;
	bounds[1].y = rpWobbleSkyY.y;
	bounds[1].z = rpWobbleSkyY.z;

	// we can't start inside the box so move this outside and use the reverse path
	rayStart += reflectionVector * 10000.0;

	// only do a box <-> ray intersection test if we use a local cubemap
	if( ( rpWobbleSkyX.w > 0.0 ) && AABBRayIntersection( bounds, rayStart, -reflectionVector, hitScale ) )
	{
		float3 hitPoint = rayStart - reflectionVector * hitScale;

		// rpWobbleSkyZ is cubemap center
#if 1
		reflectionVector = hitPoint - rpWobbleSkyZ.xyz;
		octCoord0 = octCoord1 = octCoord2 = GetSampleVector( reflectionVector );
#else
		// this should look better but only works in the case all 3 probes are in this area bbox
		octCoord0 = GetSampleVector( hitPoint - rpTexGen0S.xyz );
		octCoord1 = GetSampleVector( hitPoint - rpTexGen0T.xyz );
		octCoord2 = GetSampleVector( hitPoint - rpTexGen0Q.xyz );
#endif
	}
#endif

	const float mip = 0;
	float3 radiance = t_RadianceCubeMap1.SampleLevel( s_LinearClamp, octCoord0, mip ).rgb * rpLocalLightOrigin.x;
	radiance += t_RadianceCubeMap2.SampleLevel( s_LinearClamp, octCoord1, mip ).rgb * rpLocalLightOrigin.y;
	radiance += t_RadianceCubeMap3.SampleLevel( s_LinearClamp, octCoord2, mip ).rgb * rpLocalLightOrigin.z;

#if USE_SSR
	// Screen Space Reflections

	float3 rayDir;

	float3 viewNormal;

	// TODO this should be rpViewMatrixX
	viewNormal.x = dot3( rpModelViewMatrixX, globalNormal );
	viewNormal.y = dot3( rpModelViewMatrixY, globalNormal );
	viewNormal.z = dot3( rpModelViewMatrixZ, globalNormal );

	rayStart = ReconstructPositionCS( fragment.position.xy );

	float3 V;
	V = normalize( rayStart );
	reflectionVector = reflect( V, viewNormal );
	rayDir = normalize( reflectionVector );

	// use forward vector instead of V to avoid bending
	float vDotR = ( dot3( float3( 0, 0, 1 ), reflectionVector ) );

	const float maxSteps = rpJitterTexScale.x;

	float2 hitPixel;
	float3 hitPoint;
	float3 rayDebug = float3( 0, 0, 1 );
	bool intersection = false;

	float jitter = 1.0;
	//jitter = ( int( fragment.position.x + fragment.position.y) & 1 ) * 0.5; // like in the paper but sucks
	jitter = InterleavedGradientNoise( fragment.position.xy );
	//jitter = InterleavedGradientNoiseAnim( fragment.position.xy, rpJitterTexOffset.w );

	jitter = lerp( 1.0, jitter, rpGlobalLightOrigin.w );

	// using the same jitter on probe fallback to make it seamless
	// looks kinda bad because on close ups you don't want to see the noise
	//radiance *= jitter;

	if( vDotR <= 0 )
	{
		intersection = TraceScreenSpaceRay(
						   rayStart,
						   rayDir,
						   rpGlobalLightOrigin.z,	// zThickness 0.5
						   rpGlobalLightOrigin.x,	// stride
						   jitter,					// jitter
						   maxSteps,				// max steps
						   rpGlobalLightOrigin.y * METERS_TO_DOOM,	// max Distance
						   hitPixel,
						   hitPoint,
						   rayDebug );
	}

	float2 delta = ( hitPixel * rpWindowCoord.xy ) - ( fragment.position.xy * rpWindowCoord.xy );
	float deltaLen = length( delta );

	if( ( hitPixel.x > rpWindowCoord.z || hitPixel.x < 0.0 || hitPixel.y > rpWindowCoord.w || hitPixel.y < 0.0 ) )
	{
		intersection = false;
	}

	if( intersection )
	{
		radiance = float3( 0, 1, 0 );
		radiance = t_ScreenColor.Sample( s_LinearClamp, hitPixel * rpWindowCoord.xy ).rgb;

		//radiance = float3( delta, 0 );
		//radiance = float3( 0, deltaLen, 0 );
		//radiance = rayDebug / maxSteps;

		//radiance = float3( hitPixel * rpWindowCoord.xy, 0 );
	}
	else
	{
		/*
		if( vDotR > 0.0 )
		{
			radiance = float3( 1, 0, 0 );
		}
		else
		{
			radiance = float3( 0, 0, 1 );
		}
		*/
		//radiance = rayDebug;
		//discard;
	}
#endif

	// give it a red blood tint
	//radiance *= float3( 0.5, 0.25, 0.25 );

	// make this really dark although it is already in linear RGB
	radiance = sRGBToLinearRGB( radiance.xyz );

	result.color = float4( radiance, 1.0 ) * fragment.color;
}
