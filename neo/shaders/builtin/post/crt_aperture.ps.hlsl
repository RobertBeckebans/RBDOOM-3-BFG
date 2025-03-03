/*
    CRT Shader by EasyMode
    License: GPL

    A flat CRT shader ideally for 1080p or higher displays.

    Recommended Settings:

    Video
    - Aspect Ratio:  4:3
    - Integer Scale: Off

    Shader
    - Filter: Nearest
    - Scale:  Don't Care

    Example RGB Mask Parameter Settings:

    Aperture Grille (Default)
    - Dot Width:  1
    - Dot Height: 1
    - Stagger:    0

    Lottes' Shadow Mask
    - Dot Width:  2
    - Dot Height: 1
    - Stagger:    3
*/


#include <global_inc.hlsl>
#include "renderParmSet2.inc.hlsl"


// *INDENT-OFF*
Texture2D t_CurrentRender	: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_BlueNoise		: register( t1 VK_DESCRIPTOR_SET( 0 ) );

SamplerState s_NearestClamp	: register(s0 VK_DESCRIPTOR_SET( 1 ) );
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

#define TEX2D(c) dilate(t_CurrentRender.Sample( s_NearestClamp, c ).rgb)
#define FIX(c) max(abs(c), 1e-5)

// Set to 0 to use linear filter and gain speed
#define ENABLE_LANCZOS 1

#define RESOLUTION_DIVISOR 4.0

struct Params
{
	float4 sourceSize;
	float4 outputSize;
	uint FrameCount;

	float SHARPNESS_IMAGE;
	float SHARPNESS_EDGES;
	float GLOW_WIDTH;
	float GLOW_HEIGHT;
	float GLOW_HALATION;
	float GLOW_DIFFUSION;
	float MASK_COLORS;
	float MASK_STRENGTH;
	float MASK_SIZE;
	float SCANLINE_SIZE_MIN;
	float SCANLINE_SIZE_MAX;
	float SCANLINE_SHAPE;
	float SCANLINE_OFFSET;
	float SCANLINE_STRENGTH;
	float GAMMA_INPUT;
	float GAMMA_OUTPUT;
	float BRIGHTNESS;
};

float3 dilate( float3 col )
{
	// FIXME
	//return pow(col, float3(params.GAMMA_INPUT))
	return pow( col, _float3( 2.4 ) );
}

float mod( float x, float y )
{
	return x - y * floor( x / y );
}

float curve_distance( float x, float sharp )
{
	/*
	    apply half-circle s-curve to distance for sharper (more pixelated) interpolation
	    single line formula for Graph Toy:
	    0.5 - sqrt(0.25 - (x - step(0.5, x)) * (x - step(0.5, x))) * sign(0.5 - x)
	*/

	float x_step = step( 0.5, x );
	float curve = 0.5 - sqrt( 0.25 - ( x - x_step ) * ( x - x_step ) ) * sign( 0.5 - x );

	return lerp( x, curve, sharp );
}

float3x3 get_color_matrix( float2 co, float2 dx )
{
	return float3x3( TEX2D( co - dx ), TEX2D( co ), TEX2D( co + dx ) );
}
float3 blur( float3x3 m, float dist, float rad )
{
	float3 x = float3( dist - 1.0, dist, dist + 1.0 ) / rad;
	float3 w = exp2( x * x * -1.0 );

	return ( m[0] * w.x + m[1] * w.y + m[2] * w.z ) / ( w.x + w.y + w.z );
}

float3 filter_gaussian( float2 co, float2 tex_size, Params params )
{
	float2 dx = float2( 1.0 / tex_size.x, 0.0 );
	float2 dy = float2( 0.0, 1.0 / tex_size.y );
	float2 pix_co = co * tex_size;
	float2 tex_co = ( floor( pix_co ) + 0.5 ) / tex_size;
	float2 dist = ( frac( pix_co ) - 0.5 ) * -1.0;

	float3x3 line0 = get_color_matrix( tex_co - dy, dx );
	float3x3 line1 = get_color_matrix( tex_co, dx );
	float3x3 line2 = get_color_matrix( tex_co + dy, dx );
	float3x3 column = float3x3( blur( line0, dist.x, params.GLOW_WIDTH ),
								blur( line1, dist.x, params.GLOW_WIDTH ),
								blur( line2, dist.x, params.GLOW_WIDTH ) );

	return blur( column, dist.y, params.GLOW_HEIGHT );
}

float3 filter_lanczos2( float4 coeffs, float3x3 color_matrix )
{
	float3 col        = mul( color_matrix,  coeffs.rgb );
	float3 sample_min = min( color_matrix[1], color_matrix[2] );
	float3 sample_max = max( color_matrix[1], color_matrix[2] );

	col = clamp( col, sample_min, sample_max );

	return col.rgb;
}

float3 filter_lanczos( float2 co, float2 tex_size, float sharp )
{
	tex_size.x *= sharp;

	float2 dx = float2( 1.0 / tex_size.x, 0.0 );
	float2 pix_co = co * tex_size - float2( 0.5, 0.0 );
	float2 tex_co = ( floor( pix_co ) + float2( 0.5, 0.001 ) ) / tex_size;
	float2 dist = frac( pix_co );
	float4 coef = PI * float4( dist.x + 1.0, dist.x, dist.x - 1.0, dist.x - 2.0 );

	coef = FIX( coef );
	coef = 2.0 * sin( coef ) * sin( coef / 2.0 ) / ( coef * coef );
	coef /= dot( coef, _float4( 1.0 ) );

#if 0
	float4 col1 = float4( TEX2D( tex_co ), 1.0 );
	float4 col2 = float4( TEX2D( tex_co + dx ), 1.0 );

	return ( mul( float4x4( col1, col1, col2, col2 ), coef ) ).rgb;
#else
	float3 col  = filter_lanczos2( coef, get_color_matrix( tex_co, _float2( 0 ) ) );
	float3 col2 = filter_lanczos2( coef, get_color_matrix( tex_co, dx ) );

	//col = lerp( col, col2, curve_distance( dist.y, params.SHARPNESS_V ) );
	col = lerp( col, col2, sharp );

	return col;
#endif
}

float3 get_scanline_weight( float x, float3 col, Params params )
{
	float3 beam = lerp( _float3( params.SCANLINE_SIZE_MIN ), _float3( params.SCANLINE_SIZE_MAX ), pow( col, _float3( 1.0 / params.SCANLINE_SHAPE ) ) );
	float3 x_mul = 2.0 / beam;
	float3 x_offset = x_mul * 0.5;

	return smoothstep( 0.0, 1.0, 1.0 - abs( x * x_mul - x_offset ) ) * x_offset * params.SCANLINE_STRENGTH;
}

float3 get_mask_weight( float x, Params params )
{
	float i = mod( floor( x * params.outputSize.x * params.sourceSize.x / ( params.sourceSize.x * params.MASK_SIZE ) ), params.MASK_COLORS );

	if( i == 0.0 )
	{
		return lerp( float3( 1.0, 0.0, 1.0 ), float3( 1.0, 0.0, 0.0 ), params.MASK_COLORS - 2.0 );
	}
	else if( i == 1.0 )
	{
		return float3( 0.0, 1.0, 0.0 );
	}
	else
	{
		return float3( 0.0, 0.0, 1.0 );
	}
}




void main( PS_IN fragment, out PS_OUT result )
{
	// revised version from RetroArch

	Params params;
	params.FrameCount = int( pc.rpJitterTexOffset.w );
	params.SHARPNESS_IMAGE = 1.0;
	params.SHARPNESS_EDGES = 3.0;
	params.GLOW_WIDTH = 0.5;
	params.GLOW_HEIGHT = 0.5;
	params.GLOW_HALATION = 0.1;
	params.GLOW_DIFFUSION = 0.5;
	params.MASK_COLORS = 2.0;
	params.MASK_STRENGTH = 0.5;
	params.MASK_SIZE = 1.0;
	params.SCANLINE_SIZE_MIN = 0.5;
	params.SCANLINE_SIZE_MAX = 1.5;
	params.SCANLINE_SHAPE = 2.5;
	params.SCANLINE_OFFSET = 1.0;
	params.SCANLINE_STRENGTH = 1.0;
	params.GAMMA_INPUT = 2.4;
	params.GAMMA_OUTPUT = 2.4;
	params.BRIGHTNESS = 1.5;

	float4 outputSize;
	outputSize.xy = pc.rpWindowCoord.zw;
	outputSize.zw = float2( 1.0, 1.0 ) / pc.rpWindowCoord.zw;

	float4 sourceSize = outputSize;

	params.sourceSize = sourceSize;
	params.outputSize = outputSize;

	float scale = floor( outputSize.y * sourceSize.w );
	float offset = 1.0 / scale * 0.5;

	if( bool( mod( scale, 2.0 ) ) )
	{
		offset = 0.0;
	}

	float2 vTexCoord = abs( fragment.texcoord0.xy );

	float2 co = ( vTexCoord * sourceSize.xy - float2( 0.0, offset * params.SCANLINE_OFFSET ) ) * sourceSize.zw;
	float3 col_glow = filter_gaussian( co, sourceSize.xy, params );
	float3 col_soft = filter_lanczos( co, sourceSize.xy, params.SHARPNESS_IMAGE );
	float3 col_sharp = filter_lanczos( co, sourceSize.xy, params.SHARPNESS_EDGES );
	float3 col = sqrt( col_sharp * col_soft );

	col *= get_scanline_weight( frac( co.y * sourceSize.y / RESOLUTION_DIVISOR ), col_soft, params );
	col_glow = saturate( col_glow - col );
	col += col_glow * col_glow * params.GLOW_HALATION;
	col = lerp( col, col * get_mask_weight( vTexCoord.x, params ) * params.MASK_COLORS, params.MASK_STRENGTH );
	col += col_glow * params.GLOW_DIFFUSION;
	col = pow( col * params.BRIGHTNESS, _float3( 1.0 / params.GAMMA_OUTPUT ) );

	//col = col_soft;

	result.color = float4( col, 1.0 );
}
