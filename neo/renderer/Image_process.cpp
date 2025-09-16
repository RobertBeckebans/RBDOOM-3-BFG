/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2021-2025 Robert Beckebans
Copyright (C) 2021 Stephen Pridham

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


#include "RenderCommon.h"

#include "../libs/mesa/format_r11g11b10f.h"

/*
================
R_ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only have filter coverage if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
after resampling to the next lower power of two.
================
*/
#define	MAX_DIMENSION	4096
byte* R_ResampleTexture( const byte* in, int inwidth, int inheight,
						 int outwidth, int outheight )
{
	int		i, j;
	const byte*	inrow, *inrow2;
	unsigned int	frac, fracstep;
	unsigned int	p1[MAX_DIMENSION], p2[MAX_DIMENSION];
	const byte*		pix1, *pix2, *pix3, *pix4;
	byte*		out, *out_p;

	if( outwidth > MAX_DIMENSION )
	{
		outwidth = MAX_DIMENSION;
	}
	if( outheight > MAX_DIMENSION )
	{
		outheight = MAX_DIMENSION;
	}

	out = ( byte* )R_StaticAlloc( outwidth * outheight * 4, TAG_IMAGE );
	out_p = out;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for( i = 0 ; i < outwidth ; i++ )
	{
		p1[i] = 4 * ( frac >> 16 );
		frac += fracstep;
	}
	frac = 3 * ( fracstep >> 2 );
	for( i = 0 ; i < outwidth ; i++ )
	{
		p2[i] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	for( i = 0 ; i < outheight ; i++, out_p += outwidth * 4 )
	{
		inrow = in + 4 * inwidth * ( int )( ( i + 0.25f ) * inheight / outheight );
		inrow2 = in + 4 * inwidth * ( int )( ( i + 0.75f ) * inheight / outheight );
		frac = fracstep >> 1;
		for( j = 0 ; j < outwidth ; j++ )
		{
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			out_p[j * 4 + 0] = ( pix1[0] + pix2[0] + pix3[0] + pix4[0] ) >> 2;
			out_p[j * 4 + 1] = ( pix1[1] + pix2[1] + pix3[1] + pix4[1] ) >> 2;
			out_p[j * 4 + 2] = ( pix1[2] + pix2[2] + pix3[2] + pix4[2] ) >> 2;
			out_p[j * 4 + 3] = ( pix1[3] + pix2[3] + pix3[3] + pix4[3] ) >> 2;
		}
	}

	return out;
}

/*
================
R_Dropsample

Used to resample images in a more general than quartering fashion.
Normal maps and such should not be bilerped.
================
*/
byte* R_Dropsample( const byte* in, int inwidth, int inheight,
					int outwidth, int outheight )
{
	int		i, j, k;
	const byte*	inrow;
	const byte*	pix1;
	byte*		out, *out_p;

	out = ( byte* )R_StaticAlloc( outwidth * outheight * 4, TAG_IMAGE );
	out_p = out;

	for( i = 0 ; i < outheight ; i++, out_p += outwidth * 4 )
	{
		inrow = in + 4 * inwidth * ( int )( ( i + 0.25 ) * inheight / outheight );
		for( j = 0 ; j < outwidth ; j++ )
		{
			k = j * inwidth / outwidth;
			pix1 = inrow + k * 4;
			out_p[j * 4 + 0] = pix1[0];
			out_p[j * 4 + 1] = pix1[1];
			out_p[j * 4 + 2] = pix1[2];
			out_p[j * 4 + 3] = pix1[3];
		}
	}

	return out;
}

/*
================
R_SetAlphaNormalDivergence

If any of the angles inside the cone would directly reflect to the light, there will be
a specular highlight.  The intensity of the highlight is inversely proportional to the
area of the spread.

Light source area is important for the base size.

area subtended in light is the divergence times the distance

Shininess value is subtracted from the divergence

Sets the alpha channel to the greatest divergence dot product of the surrounding texels.
1.0 = flat, 0.0 = turns a 90 degree angle
Lower values give less shiny specular
With mip maps, the lowest samnpled value will be retained

Should we rewrite the normal as the centered average?
================
*/
void	R_SetAlphaNormalDivergence( byte* in, int width, int height )
{
	for( int y = 0 ; y < height ; y++ )
	{
		for( int x = 0 ; x < width ; x++ )
		{
			// the divergence is the smallest dot product of any of the eight surrounding texels
			byte*	pic_p = in + ( y * width + x ) * 4;
			idVec3	center;
			center[0] = ( pic_p[0] - 128 ) / 127;
			center[1] = ( pic_p[1] - 128 ) / 127;
			center[2] = ( pic_p[2] - 128 ) / 127;
			center.Normalize();

			float	maxDiverge = 1.0;

			// FIXME: this assumes wrap mode, but should handle clamp modes and border colors
			for( int yy = -1 ; yy <= 1 ; yy++ )
			{
				for( int xx = -1 ; xx <= 1 ; xx++ )
				{
					if( yy == 0 && xx == 0 )
					{
						continue;
					}
					byte*	corner_p = in + ( ( ( y + yy ) & ( height - 1 ) ) * width + ( ( x + xx )&width - 1 ) ) * 4;
					idVec3	corner;
					corner[0] = ( corner_p[0] - 128 ) / 127;
					corner[1] = ( corner_p[1] - 128 ) / 127;
					corner[2] = ( corner_p[2] - 128 ) / 127;
					corner.Normalize();

					float	diverge = corner * center;
					if( diverge < maxDiverge )
					{
						maxDiverge = diverge;
					}
				}
			}

			// we can get a diverge < 0 in some extreme cases
			if( maxDiverge < 0 )
			{
				maxDiverge = 0;
			}
			pic_p[3] = maxDiverge * 255;
		}
	}
}

/*
================
R_MipMapWithAlphaSpecularity

Returns a new copy of the texture, quartered in size and filtered.
The alpha channel is taken to be the minimum of the dots of all surrounding normals.
================
*/
#define MIP_MIN(a,b) (a<b?a:b)

byte* R_MipMapWithAlphaSpecularity( const byte* in, int width, int height )
{
	int		i, j, c, x, y, sx, sy;
	const byte*	in_p;
	byte*	out, *out_p;
	int		row;
	int		newWidth, newHeight;
	float*	fbuf, *fbuf_p;

	if( width < 1 || height < 1 || ( width + height == 2 ) )
	{
		common->FatalError( "R_MipMapWithAlphaMin called with size %i,%i", width, height );
	}

	// convert the incoming texture to centered floating point
	c = width * height;
	fbuf = ( float* )_alloca( c * 4 * sizeof( *fbuf ) );
	in_p = in;
	fbuf_p = fbuf;
	for( i = 0 ; i < c ; i++, in_p += 4, fbuf_p += 4 )
	{
		fbuf_p[0] = ( in_p[0] / 255.0 ) * 2.0 - 1.0;	// convert to a normal
		fbuf_p[1] = ( in_p[1] / 255.0 ) * 2.0 - 1.0;
		fbuf_p[2] = ( in_p[2] / 255.0 ) * 2.0 - 1.0;
		fbuf_p[3] = ( in_p[3] / 255.0 );				// filtered divegence / specularity
	}

	row = width * 4;

	newWidth = width >> 1;
	newHeight = height >> 1;
	if( !newWidth )
	{
		newWidth = 1;
	}
	if( !newHeight )
	{
		newHeight = 1;
	}
	out = ( byte* )R_StaticAlloc( newWidth * newHeight * 4, TAG_IMAGE );
	out_p = out;

	in_p = in;

	for( i = 0 ; i < newHeight ; i++ )
	{
		for( j = 0 ; j < newWidth ; j++, out_p += 4 )
		{
			idVec3	total;
			float	totalSpec;

			total.Zero();
			totalSpec = 0;
			// find the average normal
			for( x = -1 ; x <= 1 ; x++ )
			{
				sx = ( j * 2 + x ) & ( width - 1 );
				for( y = -1 ; y <= 1 ; y++ )
				{
					sy = ( i * 2 + y ) & ( height - 1 );
					fbuf_p = fbuf + ( sy * width + sx ) * 4;

					total[0] += fbuf_p[0];
					total[1] += fbuf_p[1];
					total[2] += fbuf_p[2];

					totalSpec += fbuf_p[3];
				}
			}
			total.Normalize();
			totalSpec /= 9.0;

			// find the maximum divergence
			for( x = -1 ; x <= 1 ; x++ )
			{
				for( y = -1 ; y <= 1 ; y++ )
				{
				}
			}

			// store the average normal and divergence
		}
	}

	return out;
}

float mip_gammaTable[256] =
{
	0.000000f, 0.000005f, 0.000023f, 0.000057f, 0.000107f, 0.000175f, 0.000262f, 0.000367f, 0.000493f, 0.000638f, 0.000805f, 0.000992f, 0.001202f, 0.001433f, 0.001687f, 0.001963f,
	0.002263f, 0.002586f, 0.002932f, 0.003303f, 0.003697f, 0.004116f, 0.004560f, 0.005028f, 0.005522f, 0.006041f, 0.006585f, 0.007155f, 0.007751f, 0.008373f, 0.009021f, 0.009696f,
	0.010398f, 0.011126f, 0.011881f, 0.012664f, 0.013473f, 0.014311f, 0.015175f, 0.016068f, 0.016988f, 0.017936f, 0.018913f, 0.019918f, 0.020951f, 0.022013f, 0.023104f, 0.024223f,
	0.025371f, 0.026549f, 0.027755f, 0.028991f, 0.030257f, 0.031551f, 0.032876f, 0.034230f, 0.035614f, 0.037029f, 0.038473f, 0.039947f, 0.041452f, 0.042987f, 0.044553f, 0.046149f,
	0.047776f, 0.049433f, 0.051122f, 0.052842f, 0.054592f, 0.056374f, 0.058187f, 0.060032f, 0.061907f, 0.063815f, 0.065754f, 0.067725f, 0.069727f, 0.071761f, 0.073828f, 0.075926f,
	0.078057f, 0.080219f, 0.082414f, 0.084642f, 0.086901f, 0.089194f, 0.091518f, 0.093876f, 0.096266f, 0.098689f, 0.101145f, 0.103634f, 0.106156f, 0.108711f, 0.111299f, 0.113921f,
	0.116576f, 0.119264f, 0.121986f, 0.124741f, 0.127530f, 0.130352f, 0.133209f, 0.136099f, 0.139022f, 0.141980f, 0.144972f, 0.147998f, 0.151058f, 0.154152f, 0.157281f, 0.160444f,
	0.163641f, 0.166872f, 0.170138f, 0.173439f, 0.176774f, 0.180144f, 0.183549f, 0.186989f, 0.190463f, 0.193972f, 0.197516f, 0.201096f, 0.204710f, 0.208360f, 0.212044f, 0.215764f,
	0.219520f, 0.223310f, 0.227137f, 0.230998f, 0.234895f, 0.238828f, 0.242796f, 0.246800f, 0.250840f, 0.254916f, 0.259027f, 0.263175f, 0.267358f, 0.271577f, 0.275833f, 0.280124f,
	0.284452f, 0.288816f, 0.293216f, 0.297653f, 0.302126f, 0.306635f, 0.311181f, 0.315763f, 0.320382f, 0.325037f, 0.329729f, 0.334458f, 0.339223f, 0.344026f, 0.348865f, 0.353741f,
	0.358654f, 0.363604f, 0.368591f, 0.373615f, 0.378676f, 0.383775f, 0.388910f, 0.394083f, 0.399293f, 0.404541f, 0.409826f, 0.415148f, 0.420508f, 0.425905f, 0.431340f, 0.436813f,
	0.442323f, 0.447871f, 0.453456f, 0.459080f, 0.464741f, 0.470440f, 0.476177f, 0.481952f, 0.487765f, 0.493616f, 0.499505f, 0.505432f, 0.511398f, 0.517401f, 0.523443f, 0.529523f,
	0.535642f, 0.541798f, 0.547994f, 0.554227f, 0.560499f, 0.566810f, 0.573159f, 0.579547f, 0.585973f, 0.592438f, 0.598942f, 0.605484f, 0.612066f, 0.618686f, 0.625345f, 0.632043f,
	0.638779f, 0.645555f, 0.652370f, 0.659224f, 0.666117f, 0.673049f, 0.680020f, 0.687031f, 0.694081f, 0.701169f, 0.708298f, 0.715465f, 0.722672f, 0.729919f, 0.737205f, 0.744530f,
	0.751895f, 0.759300f, 0.766744f, 0.774227f, 0.781751f, 0.789314f, 0.796917f, 0.804559f, 0.812241f, 0.819964f, 0.827726f, 0.835528f, 0.843370f, 0.851252f, 0.859174f, 0.867136f,
	0.875138f, 0.883180f, 0.891262f, 0.899384f, 0.907547f, 0.915750f, 0.923993f, 0.932277f, 0.940601f, 0.948965f, 0.957370f, 0.965815f, 0.974300f, 0.982826f, 0.991393f, 1.000000f
};

/*
================
R_MipMapGamma

Returns a new copy of the texture, quartered in size with gamma correction.
================
*/
byte* R_MipMapWithGamma( const byte* in, int width, int height )
{
	int		i, j;
	const byte*	in_p;
	byte*	out, *out_p;
	int		row;
	int		newWidth, newHeight;

	if( width < 1 || height < 1 || ( width + height == 2 ) )
	{
		return NULL;
	}

	row = width * 4;

	newWidth = width >> 1;
	newHeight = height >> 1;
	if( !newWidth )
	{
		newWidth = 1;
	}
	if( !newHeight )
	{
		newHeight = 1;
	}
	out = ( byte* )R_StaticAlloc( newWidth * newHeight * 4, TAG_IMAGE );
	out_p = out;

	in_p = in;

	width >>= 1;
	height >>= 1;

	if( width == 0 || height == 0 )
	{
		width += height;	// get largest
		for( i = 0 ; i < width ; i++, out_p += 4, in_p += 8 )
		{
			out_p[0] = idMath::Ftob( 255.0f * idMath::Pow( 0.5f * ( mip_gammaTable[in_p[0]] + mip_gammaTable[in_p[4]] ), 1.0f / 2.2f ) );
			out_p[1] = idMath::Ftob( 255.0f * idMath::Pow( 0.5f * ( mip_gammaTable[in_p[1]] + mip_gammaTable[in_p[5]] ), 1.0f / 2.2f ) );
			out_p[2] = idMath::Ftob( 255.0f * idMath::Pow( 0.5f * ( mip_gammaTable[in_p[2]] + mip_gammaTable[in_p[6]] ), 1.0f / 2.2f ) );
			out_p[3] = idMath::Ftob( 255.0f * idMath::Pow( 0.5f * ( mip_gammaTable[in_p[3]] + mip_gammaTable[in_p[7]] ), 1.0f / 2.2f ) );
		}
		return out;
	}
	for( i = 0 ; i < height ; i++, in_p += row )
	{
		for( j = 0 ; j < width ; j++, out_p += 4, in_p += 8 )
		{
			out_p[0] = idMath::Ftob( 255.0f * idMath::Pow( 0.25f * ( mip_gammaTable[in_p[0]] + mip_gammaTable[in_p[4]] + mip_gammaTable[in_p[row + 0]] + mip_gammaTable[in_p[row + 4]] ), 1.0f / 2.2f ) );
			out_p[1] = idMath::Ftob( 255.0f * idMath::Pow( 0.25f * ( mip_gammaTable[in_p[1]] + mip_gammaTable[in_p[5]] + mip_gammaTable[in_p[row + 1]] + mip_gammaTable[in_p[row + 5]] ), 1.0f / 2.2f ) );
			out_p[2] = idMath::Ftob( 255.0f * idMath::Pow( 0.25f * ( mip_gammaTable[in_p[2]] + mip_gammaTable[in_p[6]] + mip_gammaTable[in_p[row + 2]] + mip_gammaTable[in_p[row + 6]] ), 1.0f / 2.2f ) );
			out_p[3] = idMath::Ftob( 255.0f * idMath::Pow( 0.25f * ( mip_gammaTable[in_p[3]] + mip_gammaTable[in_p[7]] + mip_gammaTable[in_p[row + 3]] + mip_gammaTable[in_p[row + 7]] ), 1.0f / 2.2f ) );
		}
	}

	return out;
}

/*
================
R_MipMap

Returns a new copy of the texture, quartered in size and filtered.
================
*/
byte* R_MipMap( const byte* in, int width, int height )
{
	int		i, j;
	const byte*	in_p;
	byte*	out, *out_p;
	int		row;
	int		newWidth, newHeight;

	if( width < 1 || height < 1 || ( width + height == 2 ) )
	{
		return NULL;
	}

	row = width * 4;

	newWidth = width >> 1;
	newHeight = height >> 1;
	if( !newWidth )
	{
		newWidth = 1;
	}
	if( !newHeight )
	{
		newHeight = 1;
	}
	out = ( byte* )R_StaticAlloc( newWidth * newHeight * 4, TAG_IMAGE );
	out_p = out;

	in_p = in;

	width >>= 1;
	height >>= 1;

	if( width == 0 || height == 0 )
	{
		width += height;	// get largest
		for( i = 0 ; i < width ; i++, out_p += 4, in_p += 8 )
		{
			out_p[0] = ( in_p[0] + in_p[4] ) >> 1;
			out_p[1] = ( in_p[1] + in_p[5] ) >> 1;
			out_p[2] = ( in_p[2] + in_p[6] ) >> 1;
			out_p[3] = ( in_p[3] + in_p[7] ) >> 1;
		}
		return out;
	}

	for( i = 0 ; i < height ; i++, in_p += row )
	{
		for( j = 0 ; j < width ; j++, out_p += 4, in_p += 8 )
		{
			out_p[0] = ( in_p[0] + in_p[4] + in_p[row + 0] + in_p[row + 4] ) >> 2;
			out_p[1] = ( in_p[1] + in_p[5] + in_p[row + 1] + in_p[row + 5] ) >> 2;
			out_p[2] = ( in_p[2] + in_p[6] + in_p[row + 2] + in_p[row + 6] ) >> 2;
			out_p[3] = ( in_p[3] + in_p[7] + in_p[row + 3] + in_p[row + 7] ) >> 2;
		}
	}

	return out;
}

// RB begin
byte* R_MipMapR11G11B10F( const byte* in, int width, int height )
{
	int		i, j;
	const byte*	in_p;
	byte*	out, *out_p;
	int		row;
	int		newWidth, newHeight;

	if( width < 1 || height < 1 || ( width + height == 2 ) )
	{
		return NULL;
	}

	row = width * 4;

	newWidth = width >> 1;
	newHeight = height >> 1;
	if( !newWidth )
	{
		newWidth = 1;
	}
	if( !newHeight )
	{
		newHeight = 1;
	}
	out = ( byte* )R_StaticAlloc( newWidth * newHeight * 4, TAG_IMAGE );
	out_p = out;

	in_p = in;

	width >>= 1;
	height >>= 1;

	typedef union
	{
		uint32	i;
		byte	b[4];
	} convert_t;

	if( width == 0 || height == 0 )
	{
		width += height;	// get largest
		for( i = 0 ; i < width ; i++, out_p += 4, in_p += 8 )
		{
			convert_t p1;
			p1.b[0] = in_p[0];
			p1.b[1] = in_p[1];
			p1.b[2] = in_p[2];
			p1.b[3] = in_p[3];

			convert_t p2;
			p2.b[0] = in_p[4];
			p2.b[1] = in_p[5];
			p2.b[2] = in_p[6];
			p2.b[3] = in_p[7];

			float c1[3];
			r11g11b10f_to_float3( p1.i, c1 );

			float c2[3];
			r11g11b10f_to_float3( p2.i, c2 );

			float rgb[3];
			rgb[0] = ( c1[0] + c2[0] ) * 0.5f;
			rgb[1] = ( c1[1] + c2[1] ) * 0.5f;
			rgb[2] = ( c1[2] + c2[2] ) * 0.5f;

			p1.i = float3_to_r11g11b10f( rgb );

			out_p[0] = p1.b[0];
			out_p[1] = p1.b[1];
			out_p[2] = p1.b[2];
			out_p[3] = p1.b[3];

			//out_p[0] = ( in_p[0] + in_p[4] ) >> 1;
			//out_p[1] = ( in_p[1] + in_p[5] ) >> 1;
			//out_p[2] = ( in_p[2] + in_p[6] ) >> 1;
			//out_p[3] = ( in_p[3] + in_p[7] ) >> 1;
		}
		return out;
	}

	for( i = 0 ; i < height ; i++, in_p += row )
	{
		for( j = 0 ; j < width ; j++, out_p += 4, in_p += 8 )
		{
			convert_t p1;
			p1.b[0] = in_p[0];
			p1.b[1] = in_p[1];
			p1.b[2] = in_p[2];
			p1.b[3] = in_p[3];

			convert_t p2;
			p2.b[0] = in_p[4];
			p2.b[1] = in_p[5];
			p2.b[2] = in_p[6];
			p2.b[3] = in_p[7];

			convert_t p3;
			p3.b[0] = in_p[row + 0];
			p3.b[1] = in_p[row + 1];
			p3.b[2] = in_p[row + 2];
			p3.b[3] = in_p[row + 3];

			convert_t p4;
			p4.b[0] = in_p[row + 4];
			p4.b[1] = in_p[row + 5];
			p4.b[2] = in_p[row + 6];
			p4.b[3] = in_p[row + 7];

			float c1[3];
			r11g11b10f_to_float3( p1.i, c1 );

			float c2[3];
			r11g11b10f_to_float3( p2.i, c2 );

			float c3[3];
			r11g11b10f_to_float3( p3.i, c3 );

			float c4[3];
			r11g11b10f_to_float3( p4.i, c4 );

			float rgb[3];
			rgb[0] = ( c1[0] + c2[0] + c3[0] + c4[0] ) * 0.25f;
			rgb[1] = ( c1[1] + c2[1] + c3[1] + c4[1] ) * 0.25f;
			rgb[2] = ( c1[2] + c2[2] + c3[2] + c4[2] ) * 0.25f;

			p1.i = float3_to_r11g11b10f( rgb );

			out_p[0] = p1.b[0];
			out_p[1] = p1.b[1];
			out_p[2] = p1.b[2];
			out_p[3] = p1.b[3];

			//out_p[0] = ( in_p[0] + in_p[4] + in_p[row + 0] + in_p[row + 4] ) >> 2;
			//out_p[1] = ( in_p[1] + in_p[5] + in_p[row + 1] + in_p[row + 5] ) >> 2;
			//out_p[2] = ( in_p[2] + in_p[6] + in_p[row + 2] + in_p[row + 6] ) >> 2;
			//out_p[3] = ( in_p[3] + in_p[7] + in_p[row + 3] + in_p[row + 7] ) >> 2;
		}
	}

	return out;
}
// RB end

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
void R_BlendOverTexture( byte* data, int pixelCount, const byte blend[4] )
{
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for( i = 0 ; i < pixelCount ; i++, data += 4 )
	{
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}


/*
==================
R_HorizontalFlip

Flip the image in place
==================
*/
void R_HorizontalFlip( byte* data, int width, int height )
{
	int		i, j;
	int		temp;

	for( i = 0 ; i < height ; i++ )
	{
		for( j = 0 ; j < width / 2 ; j++ )
		{
			temp = *( ( int* )data + i * width + j );
			*( ( int* )data + i * width + j ) = *( ( int* )data + i * width + width - 1 - j );
			*( ( int* )data + i * width + width - 1 - j ) = temp;
		}
	}
}

void R_VerticalFlip( byte* data, int width, int height )
{
	int		i, j;
	int		temp;

	for( i = 0 ; i < width ; i++ )
	{
		for( j = 0 ; j < height / 2 ; j++ )
		{
			temp = *( ( int* )data + j * width + i );
			*( ( int* )data + j * width + i ) = *( ( int* )data + ( height - 1 - j ) * width + i );
			*( ( int* )data + ( height - 1 - j ) * width + i ) = temp;
		}
	}
}

// RB: halfFloat_t helper
struct ColorRGB16F
{
	uint16	red;
	uint16	green;
	uint16	blue;
};

void R_VerticalFlipRGB16F( byte* data, int width, int height )
{
	int			i, j;
	ColorRGB16F	temp;

	for( i = 0; i < width; i++ )
	{
		for( j = 0; j < height / 2; j++ )
		{
			temp = *( ( ColorRGB16F* )data + j * width + i );

			*( ( ColorRGB16F* )data + j * width + i ) = *( ( ColorRGB16F* )data + ( height - 1 - j ) * width + i );
			*( ( ColorRGB16F* )data + ( height - 1 - j ) * width + i ) = temp;
		}
	}
}

void R_RotatePic( byte* data, int width )
{
	int		i, j;
	int*		temp;

	temp = ( int* )R_StaticAlloc( width * width * 4, TAG_IMAGE );

	for( i = 0 ; i < width ; i++ )
	{
		for( j = 0 ; j < width ; j++ )
		{
			// apparently rotates the picture and then it flips the picture horitzontally
			*( temp + i * width + j ) = *( ( int* )data + j * width + i );
		}
	}

	memcpy( data, temp, width * width * 4 );

	R_StaticFree( temp );
}

// transforms in both ways, the images from a cube map,
// in both the Env map and the Skybox map systems.
void R_ApplyCubeMapTransforms( int iter, byte* data, int size )
{
	if( ( iter == 1 ) || ( iter == 2 ) )
	{
		R_VerticalFlip( data, size, size );
	}
	if( ( iter == 0 ) || ( iter == 1 ) || ( iter == 4 ) || ( iter == 5 ) )
	{
		R_RotatePic( data, size ); // apparently not only rotates but also flips horitzontally
	}
	if( iter == 1 )
	{
		R_VerticalFlip( data, size, size );
	}
	else if( iter == 3 )      // that's so just for having less lines
	{
		R_HorizontalFlip( data, size, size );
	}
}


// This is the most efficient way to atlas a mip chain to a 2d texture
// https://twitter.com/SebAaltonen/status/1327188239451611139

idVec4 R_CalculateMipRect( uint dimensions, uint mip )
{
	uint pixels_mip = dimensions >> mip;
	idVec4 uv_rect = idVec4( 0, 0, pixels_mip, pixels_mip );

	if( mip > 0 )
	{
		uv_rect.x = dimensions;
		uv_rect.y = dimensions - pixels_mip * 2;
	}

	return uv_rect;
}

int R_CalculateUsedAtlasPixels( int dimensions )
{
	int numPixels = 0;
	const int numMips = idMath::BitsForInteger( dimensions );

	for( int mip = 0; mip < numMips; mip++ )
	{
		idVec4 dstRect = R_CalculateMipRect( dimensions, mip );

		numPixels += ( dstRect.z * dstRect.w );
	}

	return numPixels;
}

// SP begin
byte* R_GenerateCubeMapSideFromSingleImage( const byte* in, int srcWidth, int srcHeight, int size, int side )
{
	size_t x = 0, y = 0;
	switch( side )
	{
		case 0:
		{
			// negative Z, front
			x = size;
			y = size;
			break;
		}
		case 1:
		{
			// positive Z, back
			x = 3 * size;
			y = size;
			break;
		}
		case 2:
		{
			// negative X, left
			x = 0;
			y = size;
			break;
		}
		case 3:
		{
			// positive X, right
			x = size * 2;
			y = size;
			break;
		}
		case 4:
		{
			// positive Y, top
			x = size;
			y = 0;
			break;
		}
		case 5:
		{
			// negative Y, bottom
			x = size;
			y = 2 * ( size_t )size;
			break;
		}
		default:
		{
			common->Warning( "Invalid side when generating cube map images" );
			return nullptr;
		}
	}

	const size_t copySize = ( size_t )size * ( size_t )size * 4;
	byte* out = ( byte* )R_StaticAlloc( copySize, TAG_IMAGE );
	uint32_t* out_p = ( uint32_t* )out;
	const uint32_t* in_p = ( uint32_t* )in + x + y * srcWidth;

	for( int j = 0; j < size; j++ )
	{
		for( int i = 0; i < size; i++ )
		{
			out_p[i + j * size] = in_p[i + j * srcWidth];
		}
	}

	return out;
}
// SP end


// RB: ripped from cmft utils by Dario Manesku
/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */
///
///
///              +----------+
///              | +---->+x |
///              | |        |
///              | |  +y    |
///              |+z      2 |
///   +----------+----------+----------+----------+
///   | +---->+z | +---->+x | +---->-z | +---->-x |
///   | |        | |        | |        | |        |
///   | |  -x    | |  +z    | |  +x    | |  -z    |
///   |-y      1 |-y      4 |-y      0 |-y      5 |
///   +----------+----------+----------+----------+
///              | +---->+x |
///              | |        |
///              | |  -y    |
///              |-z      3 |
///              +----------+
///
static const float s_faceUvVectors[6][3][3] =
{
	{
		// +x face
		{  0.0f,  0.0f, -1.0f }, // u -> -z
		{  0.0f, -1.0f,  0.0f }, // v -> -y
		{  1.0f,  0.0f,  0.0f }, // +x face
	},
	{
		// -x face
		{  0.0f,  0.0f,  1.0f }, // u -> +z
		{  0.0f, -1.0f,  0.0f }, // v -> -y
		{ -1.0f,  0.0f,  0.0f }, // -x face
	},
	{
		// +y face
		{  1.0f,  0.0f,  0.0f }, // u -> +x
		{  0.0f,  0.0f,  1.0f }, // v -> +z
		{  0.0f,  1.0f,  0.0f }, // +y face
	},
	{
		// -y face
		{  1.0f,  0.0f,  0.0f }, // u -> +x
		{  0.0f,  0.0f, -1.0f }, // v -> -z
		{  0.0f, -1.0f,  0.0f }, // -y face
	},
	{
		// +z face
		{  1.0f,  0.0f,  0.0f }, // u -> +x
		{  0.0f, -1.0f,  0.0f }, // v -> -y
		{  0.0f,  0.0f,  1.0f }, // +z face
	},
	{
		// -z face
		{ -1.0f,  0.0f,  0.0f }, // u -> -x
		{  0.0f, -1.0f,  0.0f }, // v -> -y
		{  0.0f,  0.0f, -1.0f }, // -z face
	}
};

enum
{
	CMFT_FACE_POS_X = 0,
	CMFT_FACE_NEG_X = 1,
	CMFT_FACE_POS_Y = 2,
	CMFT_FACE_NEG_Y = 3,
	CMFT_FACE_POS_Z = 4,
	CMFT_FACE_NEG_Z = 5,
};

enum
{
	CMFT_EDGE_LEFT   = 0,
	CMFT_EDGE_RIGHT  = 1,
	CMFT_EDGE_TOP    = 2,
	CMFT_EDGE_BOTTOM = 3,
};

///
///    --> U    _____
///   |        |     |
///   v        | +Y  |
///   V   _____|_____|_____ _____
///      |     |     |     |     |
///      | -X  | +Z  | +X  | -Z  |
///      |_____|_____|_____|_____|
///            |     |
///            | -Y  |
///            |_____|
///
/// Neighbour faces in order: left, right, top, bottom.
/// FaceEdge is the edge that belongs to the neighbour face.
static const struct CubeFaceNeighbour
{
	uint8_t m_faceIdx;
	uint8_t m_faceEdge;
} s_cubeFaceNeighbours[6][4] =
{
	{
		//POS_X
		{ CMFT_FACE_POS_Z, CMFT_EDGE_RIGHT },
		{ CMFT_FACE_NEG_Z, CMFT_EDGE_LEFT  },
		{ CMFT_FACE_POS_Y, CMFT_EDGE_RIGHT },
		{ CMFT_FACE_NEG_Y, CMFT_EDGE_RIGHT },
	},
	{
		//NEG_X
		{ CMFT_FACE_NEG_Z, CMFT_EDGE_RIGHT },
		{ CMFT_FACE_POS_Z, CMFT_EDGE_LEFT  },
		{ CMFT_FACE_POS_Y, CMFT_EDGE_LEFT  },
		{ CMFT_FACE_NEG_Y, CMFT_EDGE_LEFT  },
	},
	{
		//POS_Y
		{ CMFT_FACE_NEG_X, CMFT_EDGE_TOP },
		{ CMFT_FACE_POS_X, CMFT_EDGE_TOP },
		{ CMFT_FACE_NEG_Z, CMFT_EDGE_TOP },
		{ CMFT_FACE_POS_Z, CMFT_EDGE_TOP },
	},
	{
		//NEG_Y
		{ CMFT_FACE_NEG_X, CMFT_EDGE_BOTTOM },
		{ CMFT_FACE_POS_X, CMFT_EDGE_BOTTOM },
		{ CMFT_FACE_POS_Z, CMFT_EDGE_BOTTOM },
		{ CMFT_FACE_NEG_Z, CMFT_EDGE_BOTTOM },
	},
	{
		//POS_Z
		{ CMFT_FACE_NEG_X, CMFT_EDGE_RIGHT  },
		{ CMFT_FACE_POS_X, CMFT_EDGE_LEFT   },
		{ CMFT_FACE_POS_Y, CMFT_EDGE_BOTTOM },
		{ CMFT_FACE_NEG_Y, CMFT_EDGE_TOP    },
	},
	{
		//NEG_Z
		{ CMFT_FACE_POS_X, CMFT_EDGE_RIGHT  },
		{ CMFT_FACE_NEG_X, CMFT_EDGE_LEFT   },
		{ CMFT_FACE_POS_Y, CMFT_EDGE_TOP    },
		{ CMFT_FACE_NEG_Y, CMFT_EDGE_BOTTOM },
	}
};

/// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
static inline void TexelCoordToVec( float* _out3f, float _u, float _v, uint8_t _faceId )
{
	// out = u * s_faceUv[0] + v * s_faceUv[1] + s_faceUv[2].
	_out3f[0] = s_faceUvVectors[_faceId][0][0] * _u + s_faceUvVectors[_faceId][1][0] * _v + s_faceUvVectors[_faceId][2][0];
	_out3f[1] = s_faceUvVectors[_faceId][0][1] * _u + s_faceUvVectors[_faceId][1][1] * _v + s_faceUvVectors[_faceId][2][1];
	_out3f[2] = s_faceUvVectors[_faceId][0][2] * _u + s_faceUvVectors[_faceId][1][2] * _v + s_faceUvVectors[_faceId][2][2];

	// Normalize.
	const float invLen = 1.0f / sqrtf( _out3f[0] * _out3f[0] + _out3f[1] * _out3f[1] + _out3f[2] * _out3f[2] );
	_out3f[0] *= invLen;
	_out3f[1] *= invLen;
	_out3f[2] *= invLen;
}

/// Notice: _faceSize should not be equal to one!
static inline float WarpFixupFactor( float _faceSize )
{
	// Edge fixup.
	// Based on Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp
	if( _faceSize == 1.0f )
	{
		return 1.0f;
	}

	const float fs = _faceSize;
	const float fsmo = fs - 1.0f;
	return ( fs * fs ) / ( fsmo * fsmo * fsmo );
}

/// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
static inline void TexelCoordToVecWarp( float* _out3f, float _u, float _v, uint8_t _faceId, float _warpFixup )
{
	_u = ( _warpFixup * _u * _u * _u ) + _u;
	_v = ( _warpFixup * _v * _v * _v ) + _v;

	TexelCoordToVec( _out3f, _u, _v, _faceId );
}

inline void vec3Mul( float* __restrict _result, const float* __restrict _a, float _b )
{
	_result[0] = _a[0] * _b;
	_result[1] = _a[1] * _b;
	_result[2] = _a[2] * _b;
}

inline float vec3Dot( const float* __restrict _a, const float* __restrict _b )
{
	return _a[0] * _b[0] + _a[1] * _b[1] + _a[2] * _b[2];
}

/// _u and _v are in [0.0 .. 1.0] range.
static inline void VecToTexelCoord( float& _u, float& _v, uint8_t& _faceIdx, const float* _vec )
{
	const float absVec[3] =
	{
		fabsf( _vec[0] ),
		fabsf( _vec[1] ),
		fabsf( _vec[2] ),
	};
	const float max = fmaxf( fmaxf( absVec[0], absVec[1] ), absVec[2] );

	// Get face id (max component == face vector).
	if( max == absVec[0] )
	{
		_faceIdx = ( _vec[0] >= 0.0f ) ? uint8_t( CMFT_FACE_POS_X ) : uint8_t( CMFT_FACE_NEG_X );
	}
	else if( max == absVec[1] )
	{
		_faceIdx = ( _vec[1] >= 0.0f ) ? uint8_t( CMFT_FACE_POS_Y ) : uint8_t( CMFT_FACE_NEG_Y );
	}
	else //if (max == absVec[2])
	{
		_faceIdx = ( _vec[2] >= 0.0f ) ? uint8_t( CMFT_FACE_POS_Z ) : uint8_t( CMFT_FACE_NEG_Z );
	}

	// Divide by max component.
	float faceVec[3];
	vec3Mul( faceVec, _vec, 1.0f / max );

	// Project other two components to face uv basis.
	_u = ( vec3Dot( s_faceUvVectors[_faceIdx][0], faceVec ) + 1.0f ) * 0.5f;
	_v = ( vec3Dot( s_faceUvVectors[_faceIdx][1], faceVec ) + 1.0f ) * 0.5f;
}

#define CMFT_RPI      0.31830988618379067153f

static inline void LatLongFromVec( float& _u, float& _v, const float _vec[3] )
{
	const float phi = atan2f( _vec[0], _vec[2] );
	const float theta = acosf( _vec[1] );

	_u = ( idMath::PI + phi ) * ( 0.5f / idMath::PI );
	_v = theta * CMFT_RPI;
}

static inline void VecFromLatLong( float _vec[3], float _u, float _v )
{
	const float phi   = _u * idMath::TWO_PI;
	const float theta = _v * idMath::PI;

	_vec[0] = -sinf( theta ) * sinf( phi );
	_vec[1] = cosf( theta );
	_vec[2] = -sinf( theta ) * cosf( phi );
}

byte* R_GenerateCubeMapSideFromPanoramaImage( const byte* in, int srcWidth, int srcHeight, int cubeWidth, int side )
{
	size_t x = 0, y = 0;

	const uint32 bytesPerPixel = 4;

	const int copySize = cubeWidth * cubeWidth * bytesPerPixel;

	byte* out = ( byte* )R_StaticAlloc( copySize, TAG_IMAGE );
	uint32_t* out_p = ( uint32_t* )out;

	const uint32 dstFaceSize = cubeWidth;
	const uint32 dstPitch = dstFaceSize * bytesPerPixel;
	//const uint32 dstFaceDataSize = dstPitch * dstFaceSize;
	//const uint32 dstDataSize = dstFaceDataSize * 6;

	const float srcWidthMinusOne  = float( int32( srcWidth - 1 ) );
	const float srcHeightMinusOne = float( int32( srcHeight - 1 ) );
	const uint32 srcPitch = srcWidth * bytesPerPixel;
	const float invDstFaceSizef = 1.0f / float( dstFaceSize );

	// maybe wrong, done by observation
	const idMat4 toDoomTransform( idAngles( -90, 0, -90 ).ToMat3(), vec3_origin );

	byte* dstFaceData = ( byte* )out;// + face * dstFaceDataSize;
	for( int y = 0; y < dstFaceSize; y++ )
	{
		byte* dstRowData = ( byte* )dstFaceData + y * dstPitch;
		for( int  x = 0; x < dstFaceSize; x++ )
		{
			// Cubemap (u,v) on current face.
			const float uu = 2.0f * x * invDstFaceSizef - 1.0f;
			const float vv = 2.0f * y * invDstFaceSizef - 1.0f;

			// Get cubemap vector (x,y,z) from (u,v,faceIdx).
			idVec3 vec;
			TexelCoordToVec( &vec[0], uu, vv, side );
			vec = toDoomTransform * vec;

			// Convert cubemap vector (x,y,z) to latlong (u,v).
			float xSrcf;
			float ySrcf;
			LatLongFromVec( xSrcf, ySrcf, &vec[0] );

			// Convert from [0..1] to [0..(size-1)] range.
			xSrcf *= srcWidthMinusOne;
			ySrcf *= srcHeightMinusOne;

			const int32 xSrc = int32( idMath::Fabs( idMath::Rint( xSrcf ) ) );
			const int32 ySrc = int32( idMath::Fabs( idMath::Rint( ySrcf ) ) );
			const byte* src = ( const byte* )in + ySrc * srcPitch + xSrc * bytesPerPixel ;

			byte* dstColumnData = ( uint8_t* )dstRowData + x * bytesPerPixel;
			dstColumnData[0] = src[0];
			dstColumnData[1] = src[1];
			dstColumnData[2] = src[2];
			dstColumnData[3] = src[3];
		}
	}

	return out;
}
// RB end