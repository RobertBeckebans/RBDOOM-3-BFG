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
Texture2D t_BaseColor	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise	: register( t1 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_Normals		: register( t2 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_Depth		: register( t3 VK_DESCRIPTOR_SET( 0 ) );

SamplerState s_LinearClamp	: register(s0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState s_LinearWrap	: register(s1 VK_DESCRIPTOR_SET( 1 ) ); // blue noise 256

struct PS_IN
{
	float4 position : SV_Position;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*


#define RESOLUTION_DIVISOR 4.0
#define NUM_COLORS 31 // original 27


float3 Average( float3 pal[NUM_COLORS] )
{
	float3 sum = _float3( 0 );

	for( int i = 0; i < NUM_COLORS; i++ )
	{
		sum += pal[i];
	}

	return sum / float( NUM_COLORS );
}

float3 Deviation( float3 pal[NUM_COLORS] )
{
	float3 sum = _float3( 0 );
	float3 avg = Average( pal );

	for( int i = 0; i < NUM_COLORS; i++ )
	{
		sum += abs( pal[i] - avg );
	}

	return sum / float( NUM_COLORS );
}

// squared distance to avoid the sqrt of distance function
float ColorCompare( float3 a, float3 b )
{
	float3 diff = b - a;
	return dot( diff, diff );
}

// find nearest palette color using Euclidean distance
float3 LinearSearch( float3 c, float3 pal[NUM_COLORS] )
{
	int index = 0;
	float minDist = ColorCompare( c, pal[0] );

	for( int i = 1; i <	NUM_COLORS; i++ )
	{
		float dist = ColorCompare( c, pal[i] );

		if( dist < minDist )
		{
			minDist = dist;
			index = i;
		}
	}

	return pal[index];
}

#define RGB(r, g, b) float3(float(r)/255.0, float(g)/255.0, float(b)/255.0)

// http://www.cse.cuhk.edu.hk/~ttwong/papers/spheremap/spheremap.html
float3 healpix( float3 p )
{
	float a = atan2( p.z, p.x ) * 0.63662;
	float h = 3.0 * abs( p.y );
	float h2 = 0.75 * p.y;
	float2 uv = float2( a + h2, a - h2 );
	h2 = sqrt( 3.0 - h );
	float a2 = h2 * frac( a );
	uv = lerp( uv, float2( -h2 + a2, a2 ), step( 2.0, h ) );

#if 1
	return float3( uv, 1.0 );
#else
	float3 col = spheretex( uv );
	col.x = a * 0.5;

	return HSVToRGB( float3( col.x, 0.8, col.z ) );
#endif
}

// can be either view space or world space depending on rpModelMatrix
float3 ReconstructPosition( float2 S, float depth )
{
	// derive clip space from the depth buffer and screen position
	float2 uv = S * rpWindowCoord.xy;
	float3 ndc = float3( uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth );
	float clipW = -rpProjectionMatrixZ.w / ( -rpProjectionMatrixZ.z - ndc.z );

	float4 clip = float4( ndc * clipW, clipW );

	// camera space position
	float4 csP;
	csP.x = dot4( rpModelMatrixX, clip );
	csP.y = dot4( rpModelMatrixY, clip );
	csP.z = dot4( rpModelMatrixZ, clip );
	csP.w = dot4( rpModelMatrixW, clip );

	csP.xyz /= csP.w;

	return csP.xyz;
}

float3 GetPosition( int2 ssP )
{
	float depth = texelFetch( t_Depth, ssP, 0 ).r;

	// offset to pixel center
	float3 P = ReconstructPosition( float2( ssP ) + _float2( 0.5 ), depth );

	return P;
}

float BlueNoise( float2 n, float x )
{
	float noise = t_BlueNoise.Sample( s_LinearWrap, n.xy * rpJitterTexOffset.xy ).r;

	noise = frac( noise + c_goldenRatioConjugate * rpJitterTexOffset.z * x );

	noise = RemapNoiseTriErp( noise );
	noise = noise * 2.0 - 0.5;

	return noise;
}

float3 ditherRGB( float2 fragPos, float3 quantDeviation )
{
	float2 uvDither = fragPos / ( RESOLUTION_DIVISOR / rpJitterTexScale.x );
	float dither = DitherArray8x8( uvDither ) - 0.5;

	return float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;
}

void main( PS_IN fragment, out PS_OUT result )
{
#if 0
	// Amstrad CPC colors https://www.cpcwiki.eu/index.php/CPC_Palette
	// those are the original colors but they are too saturated and kinda suck
	const float3 palette[NUM_COLORS] =
	{
		RGB( 0, 0, 0 ),			// black
		RGB( 0, 0, 128 ),		// blue
		RGB( 0, 0, 255 ),		// bright blue
		RGB( 128, 0, 0 ),		// red
		RGB( 128, 0, 128 ),		// magenta
		RGB( 128, 0, 255 ),		// mauve
		RGB( 255, 0, 0 ),		// bright red
		RGB( 255, 0, 128 ),		// purple
		RGB( 255, 0, 255 ),		// bright magenta
		RGB( 0, 128, 0 ),		// green
		RGB( 0, 128, 128 ),		// cyan
		RGB( 0, 128, 255 ),		// sky blue
		RGB( 128, 128, 0 ),		// yellow
		RGB( 128, 128, 128 ),	// white
		RGB( 128, 128, 255 ),	// pastel blue
		RGB( 255, 128, 0 ),		// orange
		RGB( 255, 128, 128 ),	// pink
		RGB( 255, 128, 255 ),	// pastel magenta
		RGB( 0, 255, 0 ),		// bright green
		RGB( 0, 255, 128 ),		// sea green
		RGB( 0, 255, 255 ),		// bright cyan
		RGB( 128, 255, 0 ),		// lime
		RGB( 128, 255, 128 ),	// pastel green
		RGB( 128, 255, 255 ),	// pastel cyan
		RGB( 255, 255, 0 ),		// bright yellow
		RGB( 255, 255, 128 ),	// pastel yellow
		RGB( 255, 255, 255 ),	// bright white

#if 0
		RGB( 16, 16, 16 ),		// black
		RGB( 0, 28, 28 ),		// dark cyan
		RGB( 112, 164, 178 ) * 1.3,	// cyan

#else
		// https://lospec.com/palette-list/2bit-demichrome
		RGB( 33, 30, 32 ),
		RGB( 85, 85, 104 ),
		RGB( 160, 160, 139 ),
		RGB( 233, 239, 236 ),

#endif
	};

#elif 0
	// Tweaked LOSPEC CPC BOY PALETTE which is less saturated by Arne Niklas Jansson
	// https://lospec.com/palette-list/cpc-boy

	const float3 palette[NUM_COLORS] = // 32
	{
		RGB( 0, 0, 0 ),
		RGB( 36, 49, 55 ),
		RGB( 27, 27, 101 ),
		RGB( 27, 101, 27 ),
		RGB( 102, 30, 37 ),
		RGB( 63, 80, 63 ),
		RGB( 85, 51, 97 ),
		RGB( 121, 95, 27 ),
		RGB( 27, 110, 131 ),
		RGB( 53, 175, 53 ),
		RGB( 118, 132, 72 ),
		RGB( 188, 53, 53 ),
		RGB( 53, 53, 201 ),
		RGB( 128, 128, 128 ),
		RGB( 192, 70, 110 ),
		RGB( 53, 183, 143 ),
		RGB( 127, 201, 53 ),
		RGB( 127, 53, 201 ),
		RGB( 201, 127, 53 ),
		RGB( 30, 121, 229 ),
		RGB( 172, 181, 107 ),
		RGB( 223, 109, 155 ),
		RGB( 53, 193, 215 ),
		RGB( 145, 148, 223 ),
		RGB( 225, 198, 67 ),
		RGB( 227, 155, 141 ),
		RGB( 173, 200, 170 ),
		RGB( 141, 225, 199 ),
		RGB( 228, 221, 154 ),
		RGB( 248, 120, 248 ),
		RGB( 238, 234, 224 ),
		RGB( 255, 255, 255 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 65, 131, 57 );
	const float3 deviation = RGB( 66, 57, 63 );

#elif 1

	// https://lospec.com/palette-list/ruzzi-cpc
	const float3 palette[NUM_COLORS] = // 31
	{
		RGB( 0, 0, 28 ), // dark blue
		RGB( 28, 0, 28 ), // dark magenta
		RGB( 33, 30, 32 ),
		RGB( 85, 85, 104 ),

		RGB( 36, 14, 11 ),
		RGB( 26, 34, 114 ),
		RGB( 68, 118, 16 ),
		RGB( 129, 16, 51 ),
		RGB( 50, 131, 113 ),
		RGB( 124, 36, 136 ),
		RGB( 151, 122, 44 ),
		RGB( 16, 55, 218 ),
		RGB( 143, 136, 132 ),
		RGB( 222, 18, 92 ),
		RGB( 100, 222, 21 ),
		RGB( 32, 144, 210 ),
		RGB( 120, 58, 220 ),
		RGB( 74, 228, 112 ),
		RGB( 223, 39, 158 ),
		RGB( 236, 126, 74 ),
		RGB( 174, 228, 38 ),
		RGB( 136, 151, 219 ),
		RGB( 49, 234, 203 ),
		RGB( 163, 236, 128 ),
		RGB( 238, 142, 151 ),
		RGB( 224, 60, 223 ),
		RGB( 249, 234, 56 ),
		RGB( 152, 244, 218 ),
		RGB( 239, 157, 228 ),
		RGB( 252, 244, 144 ),
		RGB( 255, 255, 233 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 175, 6, 56 );
	const float3 deviation = RGB( 67, 70, 62 );

#elif 0

	// https://lospec.com/palette-list/ancientheritage30
	const float3 palette[NUM_COLORS] = // 30
	{
		RGB( 2, 4, 5 ),
		RGB( 21, 33, 43 ),
		RGB( 51, 38, 20 ),
		RGB( 30, 56, 56 ),
		RGB( 32, 50, 66 ),
		RGB( 77, 59, 34 ),
		RGB( 95, 26, 41 ),
		RGB( 41, 77, 70 ),
		RGB( 97, 31, 73 ),
		RGB( 45, 71, 94 ),
		RGB( 51, 99, 83 ),
		RGB( 125, 69, 52 ),
		RGB( 83, 88, 94 ),
		RGB( 113, 89, 72 ),
		RGB( 146, 40, 62 ),
		RGB( 67, 104, 121 ),
		RGB( 140, 76, 128 ),
		RGB( 175, 107, 66 ),
		RGB( 109, 153, 106 ),
		RGB( 199, 78, 51 ),
		RGB( 122, 129, 140 ),
		RGB( 154, 116, 181 ),
		RGB( 100, 166, 194 ),
		RGB( 240, 120, 62 ),
		RGB( 176, 178, 181 ),
		RGB( 255, 168, 97 ),
		RGB( 171, 157, 227 ),
		RGB( 119, 217, 213 ),
		RGB( 255, 240, 156 ),
		RGB( 233, 241, 242 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 87, 12, 60 );
	const float3 deviation = RGB( 57, 50, 51 );

#elif 0

	// Atari STE
	// https://lospec.com/palette-list/astron-ste32
	const float3 palette[NUM_COLORS] = // 32
	{
		RGB( 0, 0, 0 ),
		RGB( 192, 64, 80 ),
		RGB( 240, 240, 240 ),
		RGB( 192, 192, 176 ),
		RGB( 128, 144, 144 ),
		RGB( 96, 96, 112 ),
		RGB( 96, 64, 64 ),
		RGB( 64, 48, 32 ),
		RGB( 112, 80, 48 ),
		RGB( 176, 112, 64 ),
		RGB( 224, 160, 80 ),
		RGB( 224, 192, 128 ),
		RGB( 240, 224, 96 ),
		RGB( 224, 128, 48 ),
		RGB( 208, 80, 32 ),
		RGB( 144, 48, 32 ),
		RGB( 96, 48, 112 ),
		RGB( 176, 96, 160 ),
		RGB( 224, 128, 192 ),
		RGB( 192, 160, 208 ),
		RGB( 112, 112, 192 ),
		RGB( 48, 64, 144 ),
		RGB( 32, 32, 64 ),
		RGB( 32, 96, 208 ),
		RGB( 64, 160, 224 ),
		RGB( 128, 208, 224 ),
		RGB( 160, 240, 144 ),
		RGB( 48, 160, 96 ),
		RGB( 48, 64, 48 ),
		RGB( 48, 112, 32 ),
		RGB( 112, 160, 48 ),
		RGB( 160, 208, 80 ),
	};

#elif 0

	// Sega Genesis Evangelion
	// https://lospec.com/palette-list/sega-genesis-evangelion

	const float3 palette[NUM_COLORS] = // 17
	{
		RGB( 207, 201, 179 ),
		RGB( 163, 180, 158 ),
		RGB( 100, 166, 174 ),
		RGB( 101, 112, 141 ),
		RGB( 52, 54, 36 ),
		RGB( 37, 34, 70 ),
		RGB( 39, 28, 21 ),
		RGB( 20, 14, 11 ),
		RGB( 0, 0, 0 ),
		RGB( 202, 168, 87 ),
		RGB( 190, 136, 51 ),
		RGB( 171, 85, 92 ),
		RGB( 186, 47, 74 ),
		RGB( 131, 25, 97 ),
		RGB( 102, 52, 143 ),
		RGB( 203, 216, 246 ),
		RGB( 140, 197, 79 ),
	};

#elif 0

	// https://lospec.com/palette-list/existential-demo
	const float3 palette[NUM_COLORS] = // 8
	{
		RGB( 248, 243, 253 ),
		RGB( 250, 198, 180 ),
		RGB( 154, 218, 231 ),
		RGB( 151, 203, 29 ),
		RGB( 93, 162, 202 ),
		RGB( 218, 41, 142 ),
		RGB( 11, 134, 51 ),
		RGB( 46, 43, 18 ),
	};

#endif

	float2 uv = ( fragment.texcoord0 );
	float2 uvPixelated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	float3 quantizationPeriod = _float3( 1.0 / NUM_COLORS );
	float3 quantDeviation = Deviation( palette );
	//quantDeviation = medianAbsoluteDeviation;

	// get pixellated base color
	float4 color = t_BaseColor.Sample( s_LinearClamp, uvPixelated * rpWindowCoord.xy );

	float2 uvDither = uvPixelated;
	//if( rpJitterTexScale.x > 1.0 )
	{
		uvDither = fragment.position.xy / ( RESOLUTION_DIVISOR / rpJitterTexScale.x );
	}
	float dither = DitherArray8x8( uvDither ) - 0.5;

#if 0
	if( uv.y < 0.0625 )
	{
		color.rgb = HSVToRGB( float3( uv.x, 1.0, uv.y * 16.0 ) );

		result.color = float4( color.rgb, 1.0 );
		return;
	}
	else if( uv.y < 0.125 )
	{
		// quantized
		color.rgb = HSVToRGB( float3( uv.x, 1.0, ( uv.y - 0.0625 ) * 16.0 ) );
		color.rgb = LinearSearch( color.rgb, palette );

		result.color = float4( color.rgb, 1.0 );
		return;
	}
	else if( uv.y < 0.1875 )
	{
		// dithered quantized
		color.rgb = HSVToRGB( float3( uv.x, 1.0, ( uv.y - 0.125 ) * 16.0 ) );

		color.rgb += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;
		color.rgb = LinearSearch( color.rgb, palette );

		result.color = float4( color.rgb, 1.0 );
		return;
	}
	else if( uv.y < 0.25 )
	{
		color.rgb = _float3( uv.x );
		color.rgb = floor( color.rgb * NUM_COLORS ) * ( 1.0 / ( NUM_COLORS - 1.0 ) );

		color.rgb += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;
		color.rgb = LinearSearch( color.rgb, palette );

		result.color = float4( color.rgb, 1.0 );
		return;
	}
#endif

	//color.rgb += float3( dither, dither, dither ) * quantizationPeriod;
	color.rgb += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;

	// find closest color match from CPC color palette
	color.rgb = LinearSearch( color.rgb, palette );

#if 0

	//
	// similar to Obra Dinn
	//

	// don't post process the hands, which were drawn with alpha = 0
	if( color.a == 0.0 )
	{
		result.color = float4( color.rgb, 1.0 );
		return;
	}


	// triplanar mapping based on reconstructed depth buffer

	int2 ssP = int2( fragment.position.xy );
	float3 C = GetPosition( ssP );
	//float3 n_C = t_Normals.Sample( s_LinearClamp, uv ).rgb;
	float3 n_C = ( ( 2.0 * t_Normals.Sample( s_LinearClamp, uv ).rgb ) - 1.0 );

	//result.color = float4( n_C * 0.5 + 0.5, 1.0 );
	//return;

	// triplanar UVs
	float3 worldPos = C;

	float2 uvX = worldPos.zy; // x facing plane
	float2 uvY = worldPos.xz; // y facing plane
	float2 uvZ = worldPos.xy; // z facing plane

#if 1
	uvX = abs( uvX );// + 1.0;
	uvY = abs( uvY );// + 1.0;
	uvZ = abs( uvZ );// + 1.0;
#endif

#if 1
	uvX *= 4.0;
	uvY *= 4.0;
	uvZ *= 4.0;
#endif

#if 0
	uvX = round( float2( uvX.x, uvX.y ) );
	uvY = round( float2( uvY.x, uvY.y ) );
	uvZ = round( float2( uvZ.x, uvZ.y ) );
#endif

	// offset UVs to prevent obvious mirroring
	//uvY += 0.33;
	//uvZ += 0.67;

	float3 worldNormal = normalize( abs( n_C ) );
	//float3 worldNormal = normalize( n_C );

	float3 triblend = saturate( pow( worldNormal, 4.0 ) );
	triblend /= max( dot( triblend, float3( 1, 1, 1 ) ), 0.0001 );

#if 1
	// change from simple triplanar blending to cubic projection
	// which handles diagonal geometry way better
	if( ( abs( worldNormal.x ) > abs( worldNormal.y ) ) && ( abs( worldNormal.x ) > abs( worldNormal.z ) ) )
	{
		triblend = float3( 1, 0, 0 ); // X axis
	}
	else if( ( abs( worldNormal.z ) > abs( worldNormal.x ) ) && ( abs( worldNormal.z ) > abs( worldNormal.y ) ) )
	{
		triblend = float3( 0, 0, 1 ); // Z axis

	}
	else
	{
		triblend = float3( 0, 1, 0 ); // Y axis
	}
#endif

#if 0
	// preview blend
	result.color = float4( triblend.xyz, 1.0 );
	return;
#endif

	float3 axisSign = sign( n_C );

#if 0
	uvX.x *= axisSign.x;
	uvY.x *= axisSign.y;
	uvZ.x *= -axisSign.z;
#endif

	// FIXME get pixellated base color or not
	//float2 pixelatedUVX = floor( uvX / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;
	//float2 pixelatedUVY = floor( uvY / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;
	//float2 pixelatedUVZ = floor( uvZ / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	//float3 pixelatedColor = colX * triblend.x + colY * triblend.y + colZ * triblend.z;
	//
	//float2 uvPixelated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	color.rgb = t_BaseColor.Sample( s_LinearClamp, uvPixelated * rpWindowCoord.xy ).rgb;
	//color = t_BaseColor.Sample( s_LinearClamp, uv ).rgb;

	float3 colX = ditherRGB( uvX, quantDeviation ) * 1.0;
	float3 colY = ditherRGB( uvY, quantDeviation ) * 1.0;
	float3 colZ = ditherRGB( uvZ, quantDeviation ) * 1.0;

	float3 dither3D = colX * triblend.x + colY * triblend.y + colZ * triblend.z;
	color.rgb += dither3D;

	// find closest color match from CPC color palette
	color.rgb = LinearSearch( color.rgb, palette );

#if 0
	float2 uvC = cubeProject( abs( worldPos ) );
	color.rgb = _float3( DitherArray8x8( uvC ) - 0.5 );
#endif

#if 0
	colX = _float3( DitherArray8x8( uvX ) - 0.5 );
	colY = _float3( DitherArray8x8( uvY ) - 0.5 );
	colZ = _float3( DitherArray8x8( uvZ ) - 0.5 );

	dither3D = colX * triblend.x + colY * triblend.y + colZ * triblend.z;
	color.rgb = dither3D;
#endif

#if 0
	colX =  float3( uvZ * 0.5 + 0.5, 0.0 );
	colY = _float3( 0 );
	colZ = _float3( 0 );

	dither3D = colX * triblend.x + colY * triblend.y + colZ * triblend.z;
	color.rgb = dither3D;
#endif

#if 0
	color = _float3( uvZ.x * 0.5 + 0.5 );
	color = dither3D;
	color = floor( color * NUM_COLORS ) * ( 1.0 / ( NUM_COLORS - 1.0 ) );
	color.rgb += dither3D;
	//color.rgb += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;
	color = LinearSearch( color, palette );
#endif

#endif // cubic mapping

	//color.rgb = float3( suv.xy * 0.5 + 0.5, 1.0 );
	result.color = float4( color.rgb, 1.0 );
}
