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
#include "renderParmSet2.inc.hlsl"


// *INDENT-OFF*
Texture2D t_BaseColor	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise	: register( t1 VK_DESCRIPTOR_SET( 0 ) );

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
#define NUM_COLORS 55


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

void main( PS_IN fragment, out PS_OUT result )
{
#if 0

	// NES 1
	// https://lospec.com/palette-list/nintendo-entertainment-system

	const float3 palette[NUM_COLORS] = // 55
	{
		RGB( 0, 0, 0 ),
		RGB( 252, 252, 252 ),
		RGB( 248, 248, 248 ),
		RGB( 188, 188, 188 ),
		RGB( 124, 124, 124 ),
		RGB( 164, 228, 252 ),
		RGB( 60, 188, 252 ),
		RGB( 0, 120, 248 ),
		RGB( 0, 0, 252 ),
		RGB( 184, 184, 248 ),
		RGB( 104, 136, 252 ),
		RGB( 0, 88, 248 ),
		RGB( 0, 0, 188 ),
		RGB( 216, 184, 248 ),
		RGB( 152, 120, 248 ),
		RGB( 104, 68, 252 ),
		RGB( 68, 40, 188 ),
		RGB( 248, 184, 248 ),
		RGB( 248, 120, 248 ),
		RGB( 216, 0, 204 ),
		RGB( 148, 0, 132 ),
		RGB( 248, 164, 192 ),
		RGB( 248, 88, 152 ),
		RGB( 228, 0, 88 ),
		RGB( 168, 0, 32 ),
		RGB( 240, 208, 176 ),
		RGB( 248, 120, 88 ),
		RGB( 248, 56, 0 ),
		RGB( 168, 16, 0 ),
		RGB( 252, 224, 168 ),
		RGB( 252, 160, 68 ),
		RGB( 228, 92, 16 ),
		RGB( 136, 20, 0 ),
		RGB( 248, 216, 120 ),
		RGB( 248, 184, 0 ),
		RGB( 172, 124, 0 ),
		RGB( 80, 48, 0 ),
		RGB( 216, 248, 120 ),
		RGB( 184, 248, 24 ),
		RGB( 0, 184, 0 ),
		RGB( 0, 120, 0 ),
		RGB( 184, 248, 184 ),
		RGB( 88, 216, 84 ),
		RGB( 0, 168, 0 ),
		RGB( 0, 104, 0 ),
		RGB( 184, 248, 216 ),
		RGB( 88, 248, 152 ),
		RGB( 0, 168, 68 ),
		RGB( 0, 88, 0 ),
		RGB( 0, 252, 252 ),
		RGB( 0, 232, 216 ),
		RGB( 0, 136, 136 ),
		RGB( 0, 64, 88 ),
		RGB( 248, 216, 248 ),
		RGB( 120, 120, 120 ),
	};

#else

	// NES Advanced
	// https://lospec.com/palette-list/nes-advanced

	const float3 palette[NUM_COLORS] = // 55
	{
		RGB( 0, 0, 0 ),
		RGB( 38, 35, 47 ),
		RGB( 49, 64, 71 ),
		RGB( 89, 109, 98 ),
		RGB( 146, 156, 116 ),
		RGB( 200, 197, 163 ),
		RGB( 252, 252, 252 ),
		RGB( 27, 55, 127 ),
		RGB( 20, 122, 191 ),
		RGB( 64, 175, 221 ),
		RGB( 178, 219, 244 ),
		RGB( 24, 22, 103 ),
		RGB( 59, 44, 150 ),
		RGB( 112, 106, 225 ),
		RGB( 143, 149, 238 ),
		RGB( 68, 10, 65 ),
		RGB( 129, 37, 147 ),
		RGB( 204, 75, 185 ),
		RGB( 236, 153, 219 ),
		RGB( 63, 0, 17 ),
		RGB( 179, 28, 53 ),
		RGB( 239, 32, 100 ),
		RGB( 242, 98, 130 ),
		RGB( 150, 8, 17 ),
		RGB( 232, 24, 19 ),
		RGB( 167, 93, 105 ),
		RGB( 236, 158, 164 ),
		RGB( 86, 13, 4 ),
		RGB( 196, 54, 17 ),
		RGB( 226, 106, 18 ),
		RGB( 240, 175, 102 ),
		RGB( 42, 26, 20 ),
		RGB( 93, 52, 42 ),
		RGB( 166, 110, 70 ),
		RGB( 223, 156, 110 ),
		RGB( 142, 78, 17 ),
		RGB( 216, 149, 17 ),
		RGB( 234, 209, 30 ),
		RGB( 245, 235, 107 ),
		RGB( 47, 84, 28 ),
		RGB( 90, 131, 27 ),
		RGB( 162, 187, 30 ),
		RGB( 198, 223, 107 ),
		RGB( 15, 69, 15 ),
		RGB( 0, 139, 18 ),
		RGB( 11, 203, 18 ),
		RGB( 62, 243, 63 ),
		RGB( 17, 81, 83 ),
		RGB( 12, 133, 99 ),
		RGB( 4, 191, 121 ),
		RGB( 106, 230, 170 ),
		RGB( 38, 39, 38 ),
		RGB( 81, 79, 76 ),
		RGB( 136, 126, 131 ),
		RGB( 179, 170, 192 ),
	};

#endif

	float2 uv = ( fragment.texcoord0 );
	float2 uvPixelated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	float3 quantizationPeriod = _float3( 1.0 / NUM_COLORS );
	float3 quantDeviation = Deviation( palette );

	// get pixellated base color
	float3 color = t_BaseColor.Sample( s_LinearClamp, uvPixelated * pc.rpWindowCoord.xy ).rgb;

	float2 uvDither = uvPixelated;
	//if( pc.rpJitterTexScale.x > 1.0 )
	{
		uvDither = fragment.position.xy / ( RESOLUTION_DIVISOR / pc.rpJitterTexScale.x );
	}
	float dither = DitherArray8x8( uvDither ) - 0.5;

#if 0
	if( uv.y < 0.0625 )
	{
		color = HSVToRGB( float3( uv.x, 1.0, uv.y * 16.0 ) );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.125 )
	{
		// quantized
		color = HSVToRGB( float3( uv.x, 1.0, ( uv.y - 0.0625 ) * 16.0 ) );
		color = LinearSearch( color, palette );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.1875 )
	{
		// dithered quantized
		color = HSVToRGB( float3( uv.x, 1.0, ( uv.y - 0.125 ) * 16.0 ) );

		color.rgb += float3( dither, dither, dither ) * quantDeviation * pc.rpJitterTexScale.y;
		color = LinearSearch( color, palette );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.25 )
	{
		color = _float3( uv.x );
		color = floor( color * NUM_COLORS ) * ( 1.0 / ( NUM_COLORS - 1.0 ) );
		color += float3( dither, dither, dither ) * quantDeviation * pc.rpJitterTexScale.y;
		color = LinearSearch( color.rgb, palette );

		result.color = float4( color, 1.0 );
		return;
	}
#endif

	color.rgb += float3( dither, dither, dither ) * quantDeviation * pc.rpJitterTexScale.y;

	// find closest color match from C64 color palette
	color = LinearSearch( color.rgb, palette );

	result.color = float4( color, 1.0 );
}
