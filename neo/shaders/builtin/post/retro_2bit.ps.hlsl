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
#define NUM_COLORS 4


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

	// Gameboy
	const float3 palette[NUM_COLORS] = // 4
	{
		RGB( 27, 42, 9 ),
		RGB( 14, 69, 11 ),
		RGB( 73, 107, 34 ),
		RGB( 154, 158, 63 ),
	};

#elif 0

	// Moonlight GB
	// https://lospec.com/palette-list/moonlight-gb
	const float3 palette[NUM_COLORS] = // 4
	{
		RGB( 15, 5, 45 ),
		RGB( 32, 54, 113 ),
		RGB( 54, 134, 143 ),
		RGB( 95, 199, 93 ),
	};

#elif 1

	// CGA
	// https://lospec.com/palette-list/cga-mibend4
	const float3 palette[NUM_COLORS] = // 4
	{
		RGB( 41, 31, 35 ),
		RGB( 189, 80, 47 ),
		RGB( 52, 209, 175 ),
		RGB( 247, 236, 185 ),
	};

#elif 0

	// Hollow
	// https://lospec.com/palette-list/hollow
	const float3 palette[NUM_COLORS] = // 4
	{
		RGB( 15, 15, 27 ),
		RGB( 86, 90, 117 ),
		RGB( 198, 183, 190 ),
		RGB( 250, 251, 246 ),
	};

#else

	// https://lospec.com/palette-list/2bit-demichrome
	const float3 palette[NUM_COLORS] = // 4
	{
		RGB( 33, 30, 32 ),
		RGB( 85, 85, 104 ),
		RGB( 160, 160, 139 ),
		RGB( 233, 239, 236 ),
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
