/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014-2020 Robert Beckebans

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

// Normal Distribution Function ( NDF ) or D( h )
// GGX ( Trowbridge-Reitz )
float Distribution_GGX( float hdotN, float alpha )
{
	// alpha is assumed to be roughness^2
	float a2 = alpha * alpha;
	//float tmp = ( hdotN * hdotN ) * ( a2 - 1.0 ) + 1.0;
	float tmp = ( hdotN * a2 - hdotN ) * hdotN + 1.0;

	return ( a2 / ( PI * tmp * tmp ) );
}

float Distribution_GGX_Disney( float hdotN, float alphaG )
{
	float a2 = alphaG * alphaG;
	float tmp = ( hdotN * hdotN ) * ( a2 - 1.0 ) + 1.0;
	tmp *= tmp;

	//return ( a2 / ( PI * tmp ) );
	return ( a2 / tmp );
}

float Distribution_GGX_1886( float hdotN, float alpha )
{
	// alpha is assumed to be roughness^2
	return ( alpha / ( PI * pow( hdotN * hdotN * ( alpha - 1.0 ) + 1.0, 2.0 ) ) );
}

// Fresnel term F( v, h )
// Fnone( v, h ) = F(0) = specularColor
float3 Fresnel_Schlick( float3 specularColor, float vDotN )
{
	return specularColor + ( 1.0 - specularColor ) * pow( 1.0 - vDotN, 5.0 );
}

// Fresnel term that takes roughness into account so rough non-metal surfaces aren't too shiny [Lagarde11]
float3 Fresnel_SchlickRoughness( float3 specularColor, float vDotN, float roughness )
{
	float oneMinusRoughness = 1.0 - roughness;
	return specularColor + ( max( float3( oneMinusRoughness, oneMinusRoughness, oneMinusRoughness ), specularColor ) - specularColor ) * pow( 1.0 - vDotN, 5.0 );
}

// Sebastien Lagarde proposes an empirical approach to derive the specular occlusion term from the diffuse occlusion term in [Lagarde14].
// The result does not have any physical basis but produces visually pleasant results.
// See Sebastien Lagarde and Charles de Rousiers. 2014. Moving Frostbite to PBR.
float ComputeSpecularAO( float vDotN, float ao, float roughness )
{
	return clamp( pow( vDotN + ao, exp2( -16.0 * roughness - 1.0 ) ) - 1.0 + ao, 0.0, 1.0 );
}

// Visibility term G( l, v, h )
// Very similar to Marmoset Toolbag 2 and gives almost the same results as Smith GGX
float Visibility_Schlick( float vdotN, float ldotN, float alpha )
{
	float k = alpha * 0.5;

	float schlickL = ( ldotN * ( 1.0 - k ) + k );
	float schlickV = ( vdotN * ( 1.0 - k ) + k );

	return ( 0.25 / max( 0.001, schlickL * schlickV ) );
	//return ( ( schlickL * schlickV ) / ( 4.0 * vdotN * ldotN ) );
}

// see s2013_pbs_rad_notes.pdf
// Crafting a Next-Gen Material Pipeline for The Order: 1886
// this visibility function also provides some sort of back lighting
float Visibility_SmithGGX( float vdotN, float ldotN, float alpha )
{
	// alpha is already roughness^2

	float V1 = ldotN + sqrt( alpha + ( 1.0 - alpha ) * ldotN * ldotN );
	float V2 = vdotN + sqrt( alpha + ( 1.0 - alpha ) * vdotN * vdotN );

	// RB: avoid too bright spots
	return ( 1.0 / max( V1 * V2, 0.15 ) );
}

// RB: HACK calculate roughness from D3 gloss maps
float EstimateLegacyRoughness( float3 specMapSRGB )
{
	float Y = dot( LUMINANCE_SRGB.rgb, specMapSRGB );

	//float glossiness = clamp( 1.0 - specMapSRGB.r, 0.0, 0.98 );
	float glossiness = clamp( pow( Y, 1.0 / 2.0 ), 0.0, 0.98 );

	float roughness = 1.0 - glossiness;

	return roughness;
}

#define KENNY_PBR 1

// Kennedith98 begin
// takes a gamma-space specular texture
// outputs F0 color and roughness for PBR
void PBRFromSpecmap( float3 specMap, out float3 F0, out float roughness )
{
	// desaturate specular
	//float specLum = max( specMap.r, max( specMap.g, specMap.b ) );
	float specLum = dot( LUMINANCE_SRGB.rgb, specMap );

	// fresnel base
	F0 = _float3( 0.04 );

	// fresnel contrast (will tighten low spec and broaden high spec, stops specular looking too flat or shiny)
	float contrastMid = 0.214;
	float contrastAmount = 2.0;
	float contrast = saturate( ( specLum - contrastMid ) / ( 1 - contrastMid ) ); //high spec
	contrast += saturate( specLum / contrastMid ) - 1.0; //low spec
	contrast = exp2( contrastAmount * contrast );
	F0 *= contrast;

	// reverse blinn BRDF to perfectly match vanilla specular brightness
	// fresnel is affected when specPow is 0, experimentation is desmos showed that happens at F0/4
	float linearBrightness = Linear1( 2.0 * specLum );
	float specPow = max( 0.0, ( ( 8 * linearBrightness ) / F0.y ) - 2.0 );
	F0 *= min( 1.0, linearBrightness / ( F0.y * 0.25 ) );

	// specular power to roughness
	roughness = sqrt( 2.0 / ( specPow + 2.0 ) );

#if 1
	// RB: try to distinct between dielectrics and metal materials
	float glossiness = saturate( 1.0 - roughness );
	float metallic = step( 0.7, glossiness );

	float3 glossColor = Linear3( specMap.rgb );
	F0 = lerp( F0, glossColor, metallic );
#endif

	// RB: do another sqrt because PBR shader squares it
	roughness = sqrt( roughness );
}
// Kennedith98 end

// https://yusuketokuyoshi.com/papers/2021/Tokuyoshi2021SAA.pdf

float2x2 NonAxisAlignedNDFFiltering( float3 halfvectorTS, float2 roughness2 )
{
	// Compute the derivatives of the halfvector in the projected space.
	float2 halfvector2D = halfvectorTS.xy / abs( halfvectorTS.z );
	float2 deltaU = ddx( halfvector2D );
	float2 deltaV = ddy( halfvector2D );

	// Compute 2 * covariance matrix for the filter kernel (Eq. (3)).
	float SIGMA2 = 0.15915494;
	float2x2 delta = {deltaU, deltaV};
	float2x2 kernelRoughnessMat = 2.0 * SIGMA2 * mul( transpose( delta ), delta );

	// Approximate NDF filtering (Eq. (9)).
	float2x2 roughnessMat = {roughness2.x, 0.0, 0.0, roughness2.y};
	float2x2 filteredRoughnessMat = roughnessMat + kernelRoughnessMat;

	return filteredRoughnessMat;
}

float2 AxisAlignedNDFFiltering( float3 halfvectorTS, float2 roughness2 )
{
	// Compute the bounding rectangle of halfvector derivatives.
	float2 halfvector2D = halfvectorTS.xy / abs( halfvectorTS.z );
	float2 bounds = fwidth( halfvector2D );

	// Compute an axis-aligned filter kernel from the bounding rectangle.
	float SIGMA2 = 0.15915494;
	float2 kernelRoughness2 = 2.0 * SIGMA2 * ( bounds * bounds );

	// Approximate NDF filtering (Eq. (9)).
	// We clamp the kernel size to avoid overfiltering.
	float KAPPA = 0.18;
	float2 clampedKernelRoughness2 = min( kernelRoughness2, KAPPA );
	float2 filteredRoughness2 = saturate( roughness2 + clampedKernelRoughness2 );
	return filteredRoughness2;
}


float IsotropicNDFFiltering( float3 normal, float roughness2 )
{
	const float SIGMA2 = 0.15915494;
	const float KAPPA = 0.18;
	float3 dndu = ddx( normal );
	float3 dndv = ddy( normal );
	float kernelRoughness2 = 2.0 * SIGMA2 * ( dot( dndu, dndu ) + dot( dndv, dndv ) );
	float clampedKernelRoughness2 = min( kernelRoughness2, KAPPA );
	float filteredRoughness2 = saturate( roughness2 + clampedKernelRoughness2 );

	return filteredRoughness2;
}

// Environment BRDF approximations
// see s2013_pbs_black_ops_2_notes.pdf
/*
float a1vf( float g )
{
	return ( 0.25 * g + 0.75 );
}

float a004( float g, float vdotN )
{
	float t = min( 0.475 * g, exp2( -9.28 * vdotN ) );
	return ( t + 0.0275 ) * g + 0.015;
}

float a0r( float g, float vdotN )
{
	return ( ( a004( g, vdotN ) - a1vf( g ) * 0.04 ) / 0.96 );
}

float3 EnvironmentBRDF( float g, float vdotN, float3 rf0 )
{
	float4 t = float4( 1.0 / 0.96, 0.475, ( 0.0275 - 0.25 * 0.04 ) / 0.96, 0.25 );
	t *= float4( g, g, g, g );
	t += float4( 0.0, 0.0, ( 0.015 - 0.75 * 0.04 ) / 0.96, 0.75 );
	float a0 = t.x * min( t.y, exp2( -9.28 * vdotN ) ) + t.z;
	float a1 = t.w;

	return saturate( a0 + rf0 * ( a1 - a0 ) );
}


float3 EnvironmentBRDFApprox( float roughness, float vdotN, float3 specularColor )
{
	const float4 c0 = float4( -1, -0.0275, -0.572, 0.022 );
	const float4 c1 = float4( 1, 0.0425, 1.04, -0.04 );

	float4 r = roughness * c0 + c1;
	float a004 = min( r.x * r.x, exp2( -9.28 * vdotN ) ) * r.x + r.y;
	float2 AB = float2( -1.04, 1.04 ) * a004 + r.zw;

	return specularColor * AB.x + AB.y;
}
*/



