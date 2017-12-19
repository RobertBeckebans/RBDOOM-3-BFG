/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

#if 0

/*
==========================================================================================

OLD MATRIX MATH

==========================================================================================
*/

/*
======================
R_AxisToModelMatrix
======================
*/
void R_AxisToModelMatrix( const idMat3& axis, const idVec3& origin, float modelMatrix[16] )
{
	modelMatrix[0 * 4 + 0] = axis[0][0];
	modelMatrix[1 * 4 + 0] = axis[1][0];
	modelMatrix[2 * 4 + 0] = axis[2][0];
	modelMatrix[3 * 4 + 0] = origin[0];
	
	modelMatrix[0 * 4 + 1] = axis[0][1];
	modelMatrix[1 * 4 + 1] = axis[1][1];
	modelMatrix[2 * 4 + 1] = axis[2][1];
	modelMatrix[3 * 4 + 1] = origin[1];
	
	modelMatrix[0 * 4 + 2] = axis[0][2];
	modelMatrix[1 * 4 + 2] = axis[1][2];
	modelMatrix[2 * 4 + 2] = axis[2][2];
	modelMatrix[3 * 4 + 2] = origin[2];
	
	modelMatrix[0 * 4 + 3] = 0.0f;
	modelMatrix[1 * 4 + 3] = 0.0f;
	modelMatrix[2 * 4 + 3] = 0.0f;
	modelMatrix[3 * 4 + 3] = 1.0f;
}

/*
======================
R_MatrixTranspose
======================
*/
void R_MatrixTranspose( const float in[16], float out[16] )
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			out[i * 4 + j] = in[j * 4 + i];
		}
	}
}

/*
==========================
R_TransformModelToClip
==========================
*/
void R_TransformModelToClip( const idVec3& src, const float* modelMatrix, const float* projectionMatrix, idPlane& eye, idPlane& dst )
{
	for( int i = 0; i < 4; i++ )
	{
		eye[i] = 	modelMatrix[i + 0 * 4] * src[0] +
					modelMatrix[i + 1 * 4] * src[1] +
					modelMatrix[i + 2 * 4] * src[2] +
					modelMatrix[i + 3 * 4];
	}
	
	for( int i = 0; i < 4; i++ )
	{
		dst[i] = 	projectionMatrix[i + 0 * 4] * eye[0] +
					projectionMatrix[i + 1 * 4] * eye[1] +
					projectionMatrix[i + 2 * 4] * eye[2] +
					projectionMatrix[i + 3 * 4] * eye[3];
	}
}

/*
==========================
R_TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
void R_TransformClipToDevice( const idPlane& clip, idVec3& ndc )
{
	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
==========================
R_GlobalToNormalizedDeviceCoordinates

-1 to 1 range in x, y, and z
==========================
*/
void R_GlobalToNormalizedDeviceCoordinates( const idVec3& global, idVec3& ndc )
{
	idPlane	view;
	idPlane	clip;
	
	// _D3XP use tr.primaryView when there is no tr.viewDef
	const idRenderView* viewDef = ( tr.viewDef != NULL ) ? tr.viewDef : tr.primaryView;
	
	for( int i = 0; i < 4; i ++ )
	{
		view[i] = 	viewDef->worldSpace.modelViewMatrix[i + 0 * 4] * global[0] +
					viewDef->worldSpace.modelViewMatrix[i + 1 * 4] * global[1] +
					viewDef->worldSpace.modelViewMatrix[i + 2 * 4] * global[2] +
					viewDef->worldSpace.modelViewMatrix[i + 3 * 4];
	}
	
	for( int i = 0; i < 4; i ++ )
	{
		clip[i] = 	viewDef->projectionMatrix[i + 0 * 4] * view[0] +
					viewDef->projectionMatrix[i + 1 * 4] * view[1] +
					viewDef->projectionMatrix[i + 2 * 4] * view[2] +
					viewDef->projectionMatrix[i + 3 * 4] * view[3];
	}
	
	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
======================
R_LocalPointToGlobal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0] + modelMatrix[3 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1] + modelMatrix[3 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2] + modelMatrix[3 * 4 + 2];
}

/*
======================
R_GlobalPointToLocal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	idVec3 temp;
	
	temp[0] = in[0] - modelMatrix[3 * 4 + 0];
	temp[1] = in[1] - modelMatrix[3 * 4 + 1];
	temp[2] = in[2] - modelMatrix[3 * 4 + 2];
	
	out[0] = temp[0] * modelMatrix[0 * 4 + 0] + temp[1] * modelMatrix[0 * 4 + 1] + temp[2] * modelMatrix[0 * 4 + 2];
	out[1] = temp[0] * modelMatrix[1 * 4 + 0] + temp[1] * modelMatrix[1 * 4 + 1] + temp[2] * modelMatrix[1 * 4 + 2];
	out[2] = temp[0] * modelMatrix[2 * 4 + 0] + temp[1] * modelMatrix[2 * 4 + 1] + temp[2] * modelMatrix[2 * 4 + 2];
}

/*
======================
R_LocalVectorToGlobal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
}

/*
======================
R_GlobalVectorToLocal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
}

/*
======================
R_GlobalPlaneToLocal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane& in, idPlane& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[0] * modelMatrix[3 * 4 + 0] + in[1] * modelMatrix[3 * 4 + 1] + in[2] * modelMatrix[3 * 4 + 2] + in[3];
}

/*
======================
R_LocalPlaneToGlobal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane& in, idPlane& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[3] - modelMatrix[3 * 4 + 0] * out[0] - modelMatrix[3 * 4 + 1] * out[1] - modelMatrix[3 * 4 + 2] * out[2];
}

#endif