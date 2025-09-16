/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2024 Robert Beckebans

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "dmap.h"

#ifdef DMAP

idRenderLightLocal::idRenderLightLocal()
{
	memset( &parms, 0, sizeof( parms ) );
	memset( lightProject, 0, sizeof( lightProject ) );

	lightHasMoved			= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	lightShader				= NULL;
	falloffImage			= NULL;
	globalLightOrigin		= vec3_zero;
	viewCount				= 0;
	viewLight				= NULL;
	references				= NULL;
	foggedPortals			= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;

	baseLightProject.Zero();
	inverseBaseLightProject.Zero();
}

void idRenderLightLocal::FreeRenderLight()
{
}
void idRenderLightLocal::UpdateRenderLight( const renderLight_t* re, bool forceUpdate )
{
}
void idRenderLightLocal::GetRenderLight( renderLight_t* re )
{
}
void idRenderLightLocal::ForceUpdate()
{
}
int idRenderLightLocal::GetIndex()
{
	return index;
}

/*
=================================================================================

LIGHT DEFS

=================================================================================
*/

/*
========================
R_ComputePointLightProjectionMatrix

Computes the light projection matrix for a point light.
========================
*/
static float R_ComputePointLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject )
{
	assert( light->parms.pointLight );

	// A point light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / light->parms.lightRadius[0];
	localProject[1][1] = 0.5f / light->parms.lightRadius[1];
	localProject[2][2] = 0.5f / light->parms.lightRadius[2];
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	return 1.0f;
}

static const float SPOT_LIGHT_MIN_Z_NEAR	= 8.0f;
static const float SPOT_LIGHT_MIN_Z_FAR		= 16.0f;

/*
========================
R_ComputeSpotLightProjectionMatrix

Computes the light projection matrix for a spot light.
========================
*/
static float R_ComputeSpotLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject )
{
	const float targetDistSqr = light->parms.target.LengthSqr();
	const float invTargetDist = idMath::InvSqrt( targetDistSqr );
	const float targetDist = invTargetDist * targetDistSqr;

	const idVec3 normalizedTarget = light->parms.target * invTargetDist;
	const idVec3 normalizedRight = light->parms.right * ( 0.5f * targetDist / light->parms.right.LengthSqr() );
	const idVec3 normalizedUp = light->parms.up * ( -0.5f * targetDist / light->parms.up.LengthSqr() );

	localProject[0][0] = normalizedRight[0];
	localProject[0][1] = normalizedRight[1];
	localProject[0][2] = normalizedRight[2];
	localProject[0][3] = 0.0f;

	localProject[1][0] = normalizedUp[0];
	localProject[1][1] = normalizedUp[1];
	localProject[1][2] = normalizedUp[2];
	localProject[1][3] = 0.0f;

	localProject[3][0] = normalizedTarget[0];
	localProject[3][1] = normalizedTarget[1];
	localProject[3][2] = normalizedTarget[2];
	localProject[3][3] = 0.0f;

	// Set the falloff vector.
	// This is similar to the Z calculation for depth buffering, which means that the
	// mapped texture is going to be perspective distorted heavily towards the zero end.
	const float zNear = Max( light->parms.start * normalizedTarget, SPOT_LIGHT_MIN_Z_NEAR );
	const float zFar = Max( light->parms.end * normalizedTarget, SPOT_LIGHT_MIN_Z_FAR );
	const float zScale = ( zNear + zFar ) / zFar;

	localProject[2][0] = normalizedTarget[0] * zScale;
	localProject[2][1] = normalizedTarget[1] * zScale;
	localProject[2][2] = normalizedTarget[2] * zScale;
	localProject[2][3] = - zNear * zScale;

	// now offset to the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range
	idVec4 projectedTarget;
	localProject.TransformPoint( light->parms.target, projectedTarget );

	const float ofs0 = 0.5f - projectedTarget[0] / projectedTarget[3];
	localProject[0][0] += ofs0 * localProject[3][0];
	localProject[0][1] += ofs0 * localProject[3][1];
	localProject[0][2] += ofs0 * localProject[3][2];
	localProject[0][3] += ofs0 * localProject[3][3];

	const float ofs1 = 0.5f - projectedTarget[1] / projectedTarget[3];
	localProject[1][0] += ofs1 * localProject[3][0];
	localProject[1][1] += ofs1 * localProject[3][1];
	localProject[1][2] += ofs1 * localProject[3][2];
	localProject[1][3] += ofs1 * localProject[3][3];

	return 1.0f / ( zNear + zFar );
}

/*
========================
R_ComputeParallelLightProjectionMatrix

Computes the light projection matrix for a parallel light.
========================
*/
static float R_ComputeParallelLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject )
{
	assert( light->parms.parallel );

	// A parallel light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / light->parms.lightRadius[0];
	localProject[1][1] = 0.5f / light->parms.lightRadius[1];
	localProject[2][2] = 0.5f / light->parms.lightRadius[2];
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	return 1.0f;
}

/*
=================
R_DeriveLightData

Fills everything in based on light->parms
=================
*/
void R_DeriveLightData( idRenderLightLocal* light )
{
	// RB: skip the light shader stuff for dmap

	// ------------------------------------
	// compute the light projection matrix
	// ------------------------------------

	idRenderMatrix localProject;
	float zScale = 1.0f;
	if( light->parms.parallel )
	{
		zScale = R_ComputeParallelLightProjectionMatrix( light, localProject );
	}
	else if( light->parms.pointLight )
	{
		zScale = R_ComputePointLightProjectionMatrix( light, localProject );
	}
	else
	{
		zScale = R_ComputeSpotLightProjectionMatrix( light, localProject );
	}

	// set the old style light projection where Z and W are flipped and
	// for projected lights lightProject[3] is divided by ( zNear + zFar )
	light->lightProject[0][0] = localProject[0][0];
	light->lightProject[0][1] = localProject[0][1];
	light->lightProject[0][2] = localProject[0][2];
	light->lightProject[0][3] = localProject[0][3];

	light->lightProject[1][0] = localProject[1][0];
	light->lightProject[1][1] = localProject[1][1];
	light->lightProject[1][2] = localProject[1][2];
	light->lightProject[1][3] = localProject[1][3];

	light->lightProject[2][0] = localProject[3][0];
	light->lightProject[2][1] = localProject[3][1];
	light->lightProject[2][2] = localProject[3][2];
	light->lightProject[2][3] = localProject[3][3];

	light->lightProject[3][0] = localProject[2][0] * zScale;
	light->lightProject[3][1] = localProject[2][1] * zScale;
	light->lightProject[3][2] = localProject[2][2] * zScale;
	light->lightProject[3][3] = localProject[2][3] * zScale;

	// transform the lightProject
	float lightTransform[16];
	R_AxisToModelMatrix( light->parms.axis, light->parms.origin, lightTransform );
	for( int i = 0; i < 4; i++ )
	{
		idPlane temp = light->lightProject[i];
		R_LocalPlaneToGlobal( lightTransform, temp, light->lightProject[i] );
	}

	// adjust global light origin for off center projections and parallel projections
	// we are just faking parallel by making it a very far off center for now
	if( light->parms.parallel )
	{
		idVec3 dir = light->parms.lightCenter;
		if( dir.Normalize() == 0.0f )
		{
			// make point straight up if not specified
			dir[2] = 1.0f;
		}
		light->globalLightOrigin = light->parms.origin + dir * 100000.0f;
	}
	else
	{
		light->globalLightOrigin = light->parms.origin + light->parms.axis * light->parms.lightCenter;
	}

	// Rotate and translate the light projection by the light matrix.
	// 99% of lights remain axis aligned in world space.
	idRenderMatrix lightMatrix;
	idRenderMatrix::CreateFromOriginAxis( light->parms.origin, light->parms.axis, lightMatrix );

	idRenderMatrix inverseLightMatrix;
	if( !idRenderMatrix::Inverse( lightMatrix, inverseLightMatrix ) )
	{
		idLib::Warning( "lightMatrix invert failed" );
	}

	// 'baseLightProject' goes from global space -> light local space -> light projective space
	idRenderMatrix::Multiply( localProject, inverseLightMatrix, light->baseLightProject );

	// Invert the light projection so we can deform zero-to-one cubes into
	// the light model and calculate global bounds.
	if( !idRenderMatrix::Inverse( light->baseLightProject, light->inverseBaseLightProject ) )
	{
		idLib::Warning( "baseLightProject invert failed" );
	}

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idRenderMatrix::ProjectedBounds( light->globalLightBounds, light->inverseBaseLightProject, bounds_zeroOneCube, false );
}

/*
====================
R_FreeLightDefDerivedData

Frees all references and lit surfaces from the light
====================
*/
void R_FreeLightDefDerivedData( idRenderLightLocal* ldef )
{
	// remove any portal fog references
	for( doublePortal_t* dp = ldef->foggedPortals; dp != NULL; dp = dp->nextFoggedPortal )
	{
		dp->fogLight = NULL;
	}

	// free all the interactions
	/*
	while( ldef->firstInteraction != NULL )
	{
		ldef->firstInteraction->UnlinkAndFree();
	}
	*/

	// free all the references to the light
	areaReference_t* nextRef = NULL;
	for( areaReference_t* lref = ldef->references; lref != NULL; lref = nextRef )
	{
		nextRef = lref->ownerNext;

		// unlink from the area
		lref->areaNext->areaPrev = lref->areaPrev;
		lref->areaPrev->areaNext = lref->areaNext;

		// put it back on the free list for reuse
		ldef->world->areaReferenceAllocator.Free( lref );
	}
	ldef->references = NULL;
}

/*
===================
R_CheckForEntityDefsUsingModel
===================
*/
void R_CheckForEntityDefsUsingModel( idRenderModel* model )
{
	// STUB
}

#endif

