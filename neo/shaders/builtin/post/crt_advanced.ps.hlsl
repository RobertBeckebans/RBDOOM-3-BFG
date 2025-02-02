/*
===========================================================================

Doom 3 BFG Edition GPL Source Code

Copyright (C) 2022 whkrmrgks0
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

#include <global_inc.hlsl>
#include "renderParmSet2.inc.hlsl"


// *INDENT-OFF*
Texture2D t_CurrentRender	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise		: register( t1 VK_DESCRIPTOR_SET( 0 ) );

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

/*
#pragma parameter cus "CRT curvature" 0.15 0.0 1.0 0.01
#pragma parameter vstr "Vignette strength" 0.05 0.0 1.0 0.01
#pragma parameter marginv "Display margin" 0.02 0.0 0.1 0.005
#pragma parameter dts "Phosper size" 1.0 1.0 3.0 1.0
#pragma parameter AAz "De-moire conv iteration" 64.0 2.0 256.0 1.0
#pragma parameter vex "De-moire conv size" 2.0 0.0 4.0 0.1
#pragma parameter capa "Horizontal convolution size" 1.0 0.0 4.0 0.1
#pragma parameter capaiter "Horizontal convolution iterations" 5.0 1.0 20.0 1.0
#pragma parameter capashape "Horizontal convololution kernel shape" 3.0 1.0 40.0 0.1
#pragma parameter scl "Scnaline count, set 0 to match with input" 240.0 0.0 1080.0 1.0
#pragma parameter gma "Gamma correction" 1.0 0.1 4.0 0.1
#pragma parameter sling "line bleed" 2.0 1.0 2.0 0.1

This shader features:

1x1 to whatever size you want
Scanline width modulation
Customisable scanlines
Customisable curvature
Customisable vignette
Customisable capacitance artifact
Customisable horizontal pre-sharpen filter
Phosper pattern without any dimming
Anti moire convolution filter
*/

// CRT curvature
//static const float cus = 0.1;

// Vignette strength
static const float vstr = 0.05;

// Display margin
static const float marginv = 0.01; // 0.02

// Phosper size (should be an integer)
static const float dts = 1.0;

// De-moire convolution iteration (if you see moire, up this value)
static const float AAz = 64.0;

// De-moire convolution width (don't touch if you have no idea)
static const float vex = 2.0;

// Capacitance (scanline horizontal blur)
static const float capa = 0.5;

// Capacitance iteration
static const float capaiter = 5.0;

// Capacitance IR shape parameter
static const float capashape = 3.0;

// Scanline count
//static const float scl = 240.0;

// Gamma correction
static const float gma = 1.0;

// Line bleed
static const float sling = 2.0;

/*
struct Params
{
	float cus;			// CRT curvature
	float vstr;			// Vignette strength
	float marginv;		// Display margin
	float dts;			// Phosper size
	float AAz;			// De-moire conv iteration
	float vex;			// De-moire conv size
	float capa;			// Horizontal convolution size
	float capaiter;		// Horizontal convolution iterations
	float capashape;	// Horizontal convolution kernel shape
	float scl;			// Scanline count, set 0 to match with input
	float gma;			// Gamma correction
	float sling;		// line bleed
};
*/

#define tau 6.28318530718
#define cr float2(4.0, 0.0)
#define cb float2(2.0, 0.0)
#define cg float2(0.0, 0.0)
#define cw float2(3.0, 1.0)

#if 1
float crt_sawtooth( float inp )
{
	return inp - floor( inp );
	//return frac(inp) * 2.0 - 1.0;
}
#endif

float crt_square( float zed, float marchpoint, float floaz )
{
	return step( crt_sawtooth( zed / floaz ), marchpoint / floaz );
}

float crt_triangle( float zed )
{
	return abs( crt_sawtooth( zed + 0.5 ) - 0.5 ) * 2.0;
}


// pixelgrid mask
float grd( float2 uv, float2 disp )
{
	uv += disp * dts;
	uv /= dts;
	return crt_square( uv.x, 2.0, 6.0 ) * crt_square( uv.y, 1.0, 2.0 );
}

float3 tpscany( float3 bef, float3 ucj, float3 dcj, float temp )
{
	float3 scan = _float3( 0.0 );
	scan += max( ( crt_triangle( temp ) - 1.0 + ( bef * sling ) ), 0.0 );
	scan += max( ( clamp( 0.0, 1.0, temp * 2.0 - 1.0 ) - 2.0 ) + ( ucj * sling ), 0.0 );
	scan += max( ( clamp( 0.0, 1.0, -( temp * 2.0 - 1.0 ) ) - 2.0 ) + ( dcj * sling ), 0.0 );
	return scan / ( sling * 0.5 );
}

// nonlinear distortion
void pinc( float2 uv, inout float2 uv2, inout float mxbf, inout float vign, float ar )
{
	float cus = 0.0;
	if( pc.rpWindowCoord.x > 0.0 )
	{
		cus = 0.1;
	}

	uv2 = ( uv * _float2( 2.0 ) - _float2( 1.0 ) ) * float2( ( 1.0 + marginv ), ( 1.0 + marginv * ar ) );
	uv2 = float2( uv2.x / ( ( cos( abs( uv2.y * cus ) * tau / 4.0 ) ) ), uv2.y / ( ( cos( abs( uv2.x * cus * ar ) * tau / 4.0 ) ) ) );
	float2 uvbef = abs( uv2 ) - _float2( 1.0 ); //boarder
	mxbf = max( uvbef.x, uvbef.y );
	vign = max( uvbef.x * uvbef.y, 0.0 );
	uv2 = ( uv2 + _float2( 1.0 ) ) * _float2( 0.5 ); //recoordination
}

float scimpresp( float range ) //scanline IR
{
	return sin( pow( range, capashape ) * tau ) + 1.0;
}

void main( PS_IN fragment, out PS_OUT result )
{
	// revised version from RetroArch
	/*
	Params params;
	params.cus = 0.15;
	params.vstr = 0.05;
	params.marginv = 0.02;
	params.dts = 1.0;
	params.AAz = 64.0;
	params.vex = 2.0;
	params.capa = 1.0;
	params.capaiter = 5.0;
	params.capashape = 3.0;
	params.scl = 240.0;
	params.gma = 1.0;
	params.sling = 2.0;
	*/

	float4 outputSize;
	outputSize.xy = pc.rpWindowCoord.zw;
	outputSize.zw = float2( 1.0, 1.0 ) / pc.rpWindowCoord.zw;

	float4 sourceSize = pc.rpScreenCorrectionFactor;

	float2 vTexCoord = ( fragment.texcoord0.xy );
	//vTexCoord.y = 1.0 - vTexCoord.y;

	float2 uv2;
	float mxbf, vign;
	const float scl = outputSize.y / 2.0;
	float scanline = ( 1.0 - min( scl, 1.0 ) ) * sourceSize.y + scl;

	float2 ratd = vTexCoord * outputSize.xy;
	float2 uv = uv2 = vTexCoord;
	pinc( uv, uv2, mxbf, vign, outputSize.x / outputSize.y );

	float2 nuv = uv2;
	float2 nuvyud = float2( floor( nuv.y * scanline - 1.0 ) / scanline, floor( nuv.y * scanline + 1.0 ) / scanline );
	nuv.y = floor( nuv.y * scanline ) / scanline;

	float3 bef = _float3( 0.0 );
	float3 ucj = _float3( 0.0 );
	float3 dcj = _float3( 0.0 );

	float capatemp, capainteg = 0.0;

	// RB: workaround for stupid compiler bug

//#if 0
//	for( float i = -capaiter / 2.0; i <= ( capaiter / 2.0 ); i++ )
//#else
	const float capaVal[5] = { -2.5, -1.5, 0.5, 1.5, 2.5 };
	for( int s = 0; s < 5; s++ )
	{
		float i = capaVal[s];
//#endif
		// RB: avoid entering 0
		capatemp = scimpresp( ( i + capaiter / 2.0 ) / capaiter * 1.0001 );
		capainteg += capatemp;
		bef += t_CurrentRender.Sample( s_LinearWrap, float2( crt_sawtooth( nuv.x - ( capa / ( scanline * i ) ) / ( capaiter / 2.0 ) ), nuv.y ) ).xyz * capatemp;
		ucj += t_CurrentRender.Sample( s_LinearWrap, float2( crt_sawtooth( nuv.x - ( capa / ( scanline * i ) ) / ( capaiter / 2.0 ) ), nuvyud.y ) ).xyz * capatemp;
		dcj += t_CurrentRender.Sample( s_LinearWrap, float2( crt_sawtooth( nuv.x - ( capa / ( scanline * i ) ) / ( capaiter / 2.0 ) ), nuvyud.x ) ).xyz * capatemp;
	}

	dcj /= capainteg;
	bef /= capainteg;
	ucj /= capainteg;

	float3 scan = _float3( 0.0 );
	float temp;
	float snippet;
	float integral = 0.0;
	for( float a = -AAz / 2.0; a <= AAz / 2.0 ; a++ )
	{
		snippet = ( AAz / 2.0 - abs( a ) ) / AAz / 2.0;
		integral += snippet;
		temp = crt_sawtooth( uv2.y * scanline );
		scan += tpscany( bef, ucj, dcj, temp + ( a / AAz * 2.0 ) * vex / outputSize.y * scanline ) * snippet; //antimoire convolution
	}
	scan /= integral;

	float brd = step( mxbf, 0.0 );
	vign = pow( vign, vstr );

	float3 grid = float3( grd( ratd, cr ), grd( ratd, cg ), grd( ratd, cb ) );
	grid += float3( grd( ratd, cr + cw ), grd( ratd, cg + cw ), grd( ratd, cb + cw ) );

	float mask = brd * vign;

	scan /= sling;
	scan = pow( scan, _float3( 0.5 ) );
	scan = pow( scan, _float3( 1.0 + 1.0 / 3.0 ) );
	scan = pow( scan, _float3( gma ) );

	float3 grided = scan * grid * 3.0;
	float3 final = min( float3( lerp( grided, scan, scan ) ), _float3( 1.0 ) ) * mask;

//final = t_CurrentRender.Sample( s_LinearClamp, nuv ).xyz;
//final = _float3( 1.0 ) * mask;
//final = float3( nuv.x, nuv.y, 0.0 );
//final = float3( nuvyud.x, nuvyud.y, 0.0 );
//final = grid;
//final = bef;

	result.color = float4( final, 1.0 );
}
