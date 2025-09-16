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
#define NUM_COLORS 64 // original 61


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

	// + very colorful
	// - no blacks
	// - looks more 8 bit than 16 bit
	// https://lospec.com/palette-list/meld-plus

	const float3 palette[NUM_COLORS] = // 45
	{
		RGB( 0, 0, 55 ),
		RGB( 30, 13, 78 ),
		RGB( 0, 60, 69 ),
		RGB( 81, 6, 44 ),
		RGB( 82, 48, 55 ),
		RGB( 76, 2, 91 ),
		RGB( 64, 47, 93 ),
		RGB( 0, 89, 86 ),
		RGB( 19, 33, 120 ),
		RGB( 64, 60, 105 ),
		RGB( 79, 10, 138 ),
		RGB( 149, 18, 58 ),
		RGB( 135, 14, 87 ),
		RGB( 33, 165, 63 ),
		RGB( 95, 92, 128 ),
		RGB( 29, 84, 177 ),
		RGB( 131, 97, 144 ),
		RGB( 189, 101, 55 ),
		RGB( 205, 80, 24 ),
		RGB( 4, 182, 146 ),
		RGB( 156, 130, 119 ),
		RGB( 209, 57, 103 ),
		RGB( 53, 135, 210 ),
		RGB( 133, 223, 83 ),
		RGB( 13, 233, 142 ),
		RGB( 255, 94, 67 ),
		RGB( 149, 154, 187 ),
		RGB( 194, 42, 218 ),
		RGB( 230, 167, 88 ),
		RGB( 255, 130, 87 ),
		RGB( 255, 153, 51 ),
		RGB( 197, 180, 161 ),
		RGB( 210, 154, 173 ),
		RGB( 97, 212, 255 ),
		RGB( 152, 255, 192 ),
		RGB( 231, 255, 125 ),
		RGB( 249, 132, 237 ),
		RGB( 251, 227, 163 ),
		RGB( 242, 230, 179 ),
		RGB( 255, 231, 160 ),
		RGB( 255, 211, 200 ),
		RGB( 247, 236, 206 ),
		RGB( 209, 234, 251 ),
		RGB( 255, 205, 243 ),
		RGB( 255, 255, 255 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 29, 87, 155 );
	const float3 deviation = RGB( 80, 72, 55 );

#elif 1

	// + very good dithering variety at dark grey values
	// + does not leak too much color into grey values
	// + good saturation when colors are really needed
	// - a bit too strong visible dithering pattern
	// https://lospec.com/palette-list/famicube
	const float3 palette[NUM_COLORS] = // 64
	{
		RGB( 0, 0, 0 ),
		RGB( 21, 21, 21 ),
		RGB( 35, 23, 18 ),
		RGB( 23, 40, 8 ),
		RGB( 13, 32, 48 ),
		RGB( 33, 22, 64 ),
		RGB( 0, 78, 0 ),
		RGB( 79, 21, 7 ),
		RGB( 52, 52, 52 ),
		RGB( 92, 60, 13 ),
		RGB( 0, 96, 75 ),
		RGB( 55, 109, 3 ),
		RGB( 0, 23, 125 ),
		RGB( 0, 82, 128 ),
		RGB( 65, 93, 102 ),
		RGB( 135, 22, 70 ),
		RGB( 130, 60, 61 ),
		RGB( 19, 157, 8 ),
		RGB( 90, 25, 145 ),
		RGB( 61, 52, 165 ),
		RGB( 173, 78, 26 ),
		RGB( 32, 181, 98 ),
		RGB( 106, 180, 23 ),
		RGB( 147, 151, 23 ),
		RGB( 174, 108, 55 ),
		RGB( 123, 123, 123 ),
		RGB( 2, 74, 202 ),
		RGB( 10, 152, 172 ),
		RGB( 106, 49, 202 ),
		RGB( 88, 211, 50 ),
		RGB( 224, 60, 40 ),
		RGB( 207, 60, 113 ),
		RGB( 163, 40, 179 ),
		RGB( 204, 143, 21 ),
		RGB( 140, 214, 18 ),
		RGB( 113, 166, 161 ),
		RGB( 218, 101, 94 ),
		RGB( 98, 100, 220 ),
		RGB( 182, 193, 33 ),
		RGB( 197, 151, 130 ),
		RGB( 10, 137, 255 ),
		RGB( 246, 143, 55 ),
		RGB( 168, 168, 168 ),
		RGB( 225, 130, 137 ),
		RGB( 37, 226, 205 ),
		RGB( 91, 168, 255 ),
		RGB( 255, 187, 49 ),
		RGB( 190, 235, 113 ),
		RGB( 204, 105, 228 ),
		RGB( 166, 117, 254 ),
		RGB( 155, 160, 239 ),
		RGB( 245, 183, 132 ),
		RGB( 255, 231, 55 ),
		RGB( 255, 130, 206 ),
		RGB( 226, 215, 181 ),
		RGB( 213, 156, 252 ),
		RGB( 152, 220, 255 ),
		RGB( 215, 215, 215 ),
		RGB( 189, 255, 202 ),
		RGB( 238, 255, 169 ),
		RGB( 226, 201, 255 ),
		RGB( 255, 233, 197 ),
		RGB( 254, 201, 237 ),
		RGB( 255, 255, 255 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 63, 175, 2 );
	const float3 deviation = RGB( 76, 62, 75 );

#elif 0

	// Resurrect 64 - Most popular 64 colors palette
	// - leaks too much color into grey scale bar
	// - dark greys are just dark purple
	// https://lospec.com/palette-list/resurrect-64

	const float3 palette[NUM_COLORS] = // 64
	{
		RGB( 46, 34, 47 ),
		RGB( 49, 54, 56 ),
		RGB( 69, 41, 63 ),
		RGB( 76, 62, 36 ),
		RGB( 62, 53, 70 ),
		RGB( 50, 51, 83 ),
		RGB( 22, 90, 76 ),
		RGB( 55, 78, 74 ),
		RGB( 110, 39, 39 ),
		RGB( 11, 94, 101 ),
		RGB( 122, 48, 69 ),
		RGB( 103, 102, 51 ),
		RGB( 117, 60, 84 ),
		RGB( 72, 74, 119 ),
		RGB( 131, 28, 93 ),
		RGB( 105, 79, 98 ),
		RGB( 98, 85, 101 ),
		RGB( 107, 62, 117 ),
		RGB( 35, 144, 99 ),
		RGB( 84, 126, 100 ),
		RGB( 158, 69, 57 ),
		RGB( 174, 35, 52 ),
		RGB( 179, 56, 49 ),
		RGB( 11, 138, 143 ),
		RGB( 162, 75, 111 ),
		RGB( 150, 108, 108 ),
		RGB( 195, 36, 84 ),
		RGB( 127, 112, 138 ),
		RGB( 77, 101, 180 ),
		RGB( 30, 188, 115 ),
		RGB( 14, 175, 155 ),
		RGB( 205, 104, 61 ),
		RGB( 144, 94, 169 ),
		RGB( 162, 169, 71 ),
		RGB( 232, 59, 59 ),
		RGB( 234, 79, 54 ),
		RGB( 171, 148, 122 ),
		RGB( 146, 169, 132 ),
		RGB( 207, 101, 127 ),
		RGB( 251, 107, 29 ),
		RGB( 240, 79, 120 ),
		RGB( 230, 144, 78 ),
		RGB( 145, 219, 105 ),
		RGB( 245, 125, 74 ),
		RGB( 77, 155, 230 ),
		RGB( 247, 150, 23 ),
		RGB( 155, 171, 178 ),
		RGB( 178, 186, 144 ),
		RGB( 48, 225, 185 ),
		RGB( 246, 129, 129 ),
		RGB( 237, 128, 153 ),
		RGB( 213, 224, 75 ),
		RGB( 249, 194, 43 ),
		RGB( 205, 223, 108 ),
		RGB( 251, 185, 84 ),
		RGB( 168, 132, 243 ),
		RGB( 252, 167, 144 ),
		RGB( 143, 211, 255 ),
		RGB( 199, 220, 208 ),
		RGB( 143, 248, 226 ),
		RGB( 253, 203, 176 ),
		RGB( 234, 173, 237 ),
		RGB( 251, 255, 134 ),
		RGB( 255, 255, 255 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 94, 43, 86 );
	const float3 deviation = RGB( 64, 54, 46 );

#elif 0

	// Endesga 64
	// + great dithering in the grey scale bar
	// - makes the game look too grey
	// https://lospec.com/palette-list/endesga-64

	const float3 palette[NUM_COLORS] = // 64
	{
		RGB( 14, 7, 27 ),
		RGB( 19, 19, 19 ),
		RGB( 28, 18, 28 ),
		RGB( 27, 27, 27 ),
		RGB( 26, 25, 50 ),
		RGB( 39, 39, 39 ),
		RGB( 3, 25, 63 ),
		RGB( 57, 31, 33 ),
		RGB( 12, 46, 68 ),
		RGB( 59, 20, 67 ),
		RGB( 87, 28, 39 ),
		RGB( 42, 47, 78 ),
		RGB( 61, 61, 61 ),
		RGB( 19, 76, 76 ),
		RGB( 93, 44, 40 ),
		RGB( 0, 57, 109 ),
		RGB( 30, 111, 80 ),
		RGB( 98, 36, 97 ),
		RGB( 137, 30, 43 ),
		RGB( 12, 2, 147 ),
		RGB( 66, 76, 110 ),
		RGB( 142, 37, 29 ),
		RGB( 93, 93, 93 ),
		RGB( 138, 72, 54 ),
		RGB( 51, 152, 75 ),
		RGB( 0, 105, 170 ),
		RGB( 196, 36, 48 ),
		RGB( 101, 115, 146 ),
		RGB( 147, 56, 143 ),
		RGB( 198, 69, 36 ),
		RGB( 48, 3, 217 ),
		RGB( 133, 133, 133 ),
		RGB( 90, 197, 79 ),
		RGB( 191, 111, 74 ),
		RGB( 234, 50, 60 ),
		RGB( 200, 80, 134 ),
		RGB( 224, 116, 56 ),
		RGB( 255, 0, 64 ),
		RGB( 237, 118, 20 ),
		RGB( 255, 80, 0 ),
		RGB( 0, 152, 220 ),
		RGB( 245, 85, 93 ),
		RGB( 122, 9, 250 ),
		RGB( 146, 161, 185 ),
		RGB( 153, 230, 95 ),
		RGB( 202, 82, 201 ),
		RGB( 230, 156, 105 ),
		RGB( 255, 162, 20 ),
		RGB( 237, 171, 80 ),
		RGB( 246, 129, 135 ),
		RGB( 180, 180, 180 ),
		RGB( 0, 205, 249 ),
		RGB( 255, 200, 37 ),
		RGB( 219, 63, 253 ),
		RGB( 12, 241, 255 ),
		RGB( 211, 252, 126 ),
		RGB( 246, 202, 159 ),
		RGB( 255, 235, 87 ),
		RGB( 199, 207, 221 ),
		RGB( 243, 137, 245 ),
		RGB( 148, 253, 255 ),
		RGB( 249, 230, 207 ),
		RGB( 253, 210, 237 ),
		RGB( 255, 255, 255 ),
	};

	const float3 medianAbsoluteDeviation = RGB( 33, 166, 46 );
	const float3 deviation = RGB( 82, 65, 64 );

#endif

	float2 uv = ( fragment.texcoord0 );
	float2 uvPixelated = floor( fragment.position.xy / RESOLUTION_DIVISOR ) * RESOLUTION_DIVISOR;

	float3 quantizationPeriod = _float3( 1.0 / NUM_COLORS );
	float3 quantDeviation = deviation;
	//quantDeviation = medianAbsoluteDeviation;

	// get pixellated base color
	float3 color = t_BaseColor.Sample( s_LinearClamp, uvPixelated * rpWindowCoord.xy ).rgb;

	float2 uvDither = uvPixelated;
	//if( rpJitterTexScale.x > 1.0 )
	{
		uvDither = fragment.position.xy / ( RESOLUTION_DIVISOR / rpJitterTexScale.x );
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

		color.rgb += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;
		color = LinearSearch( color, palette );

		result.color = float4( color, 1.0 );
		return;
	}
	else if( uv.y < 0.25 )
	{
		color = _float3( uv.x );
		color = floor( color * NUM_COLORS ) * ( 1.0 / ( NUM_COLORS - 1.0 ) );
		color += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;
		color = LinearSearch( color.rgb, palette );

		result.color = float4( color, 1.0 );
		return;
	}
#endif

	color.rgb += float3( dither, dither, dither ) * quantDeviation * rpJitterTexScale.y;

	// find closest color match from C64 color palette
	color = LinearSearch( color.rgb, palette );

	result.color = float4( color, 1.0 );
}
