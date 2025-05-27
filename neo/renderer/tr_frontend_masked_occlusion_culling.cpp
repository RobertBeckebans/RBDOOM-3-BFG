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
#include "precompiled.h"
#pragma hdrstop

#if defined(USE_INTRINSICS_SSE)
	#if MOC_MULTITHREADED
		#include "CullingThreadPool.h"
	#else
		#include "../libs/moc/MaskedOcclusionCulling.h"
	#endif
#endif

#include "RenderCommon.h"
#include "Model_local.h"

static const float CHECK_BOUNDS_EPSILON = 1.0f;

/*
==================
R_SortViewEntities
==================
*/
viewEntity_t* R_SortViewEntities( viewEntity_t* vEntities )
{
	SCOPED_PROFILE_EVENT( "R_SortViewEntities" );

	// We want to avoid having a single AddModel for something complex be
	// the last thing processed and hurt the parallel occupancy, so
	// sort dynamic models first, _area models second, then everything else.
	viewEntity_t* dynamics = NULL;
	viewEntity_t* areas = NULL;
	viewEntity_t* others = NULL;
	for( viewEntity_t* vEntity = vEntities; vEntity != NULL; )
	{
		viewEntity_t* next = vEntity->next;
		const idRenderModel* model = vEntity->entityDef->parms.hModel;
		if( model->IsDynamicModel() != DM_STATIC )
		{
			vEntity->next = dynamics;
			dynamics = vEntity;
		}
		else if( model->IsStaticWorldModel() )
		{
			vEntity->next = areas;
			areas = vEntity;
		}
		else
		{
			vEntity->next = others;
			others = vEntity;
		}
		vEntity = next;
	}

	// concatenate the lists
	viewEntity_t* all = others;

	for( viewEntity_t* vEntity = areas; vEntity != NULL; )
	{
		viewEntity_t* next = vEntity->next;
		vEntity->next = all;
		all = vEntity;
		vEntity = next;
	}

	for( viewEntity_t* vEntity = dynamics; vEntity != NULL; )
	{
		viewEntity_t* next = vEntity->next;
		vEntity->next = all;
		all = vEntity;
		vEntity = next;
	}

	return all;
}


/*
===================
R_RenderSingleModel

May be run in parallel.

Here is where dynamic models actually get instantiated, and necessary
interaction surfaces get created. This is all done on a sort-by-model
basis to keep source data in cache (most likely L2) as any interactions
and shadows are generated, since dynamic models will typically be lit by
two or more lights.
===================
*/
#if defined(USE_INTRINSICS_SSE)
void R_RenderSingleModel( viewEntity_t* vEntity )
{
	// we will add all interaction surfs here, to be chained to the lights in later serial code
	vEntity->drawSurfs = NULL;

	// globals we really should pass in...
	const viewDef_t* viewDef = tr.viewDef;

	idRenderEntityLocal* entityDef = vEntity->entityDef;
	const renderEntity_t* renderEntity = &entityDef->parms;
	const idRenderWorldLocal* world = entityDef->world;

	if( viewDef->isXraySubview && entityDef->parms.xrayIndex == 1 )
	{
		return;
	}
	else if( !viewDef->isXraySubview && entityDef->parms.xrayIndex == 2 )
	{
		return;
	}

	SCOPED_PROFILE_EVENT( renderEntity->hModel == NULL ? "Unknown Model" : renderEntity->hModel->Name() );

	// calculate the znear for testing whether or not the view is inside a shadow projection
	const float znear = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();

	// if the entity wasn't seen through a portal chain, it was added just for light shadows
	const bool modelIsVisible = !vEntity->scissorRect.IsEmpty();
	const bool addInteractions = modelIsVisible && ( !viewDef->isXraySubview || entityDef->parms.xrayIndex == 2 );
	const int entityIndex = entityDef->index;

	// if we aren't visible we don't need to do anything else
	if( !modelIsVisible )
	{
		return;
	}

	//---------------------------
	// create a dynamic model if the geometry isn't static
	//---------------------------
	idRenderModel* model = R_EntityDefDynamicModel( entityDef );
	if( model == NULL || model->NumSurfaces() <= 0 )
	{
		return;
	}

	//---------------------------
	// copy matrix related stuff for back-end use
	// and setup a render matrix for faster culling
	//---------------------------
	vEntity->modelDepthHack = renderEntity->modelDepthHack;
	vEntity->weaponDepthHack = renderEntity->weaponDepthHack;
	vEntity->skipMotionBlur = renderEntity->skipMotionBlur;

	memcpy( vEntity->modelMatrix, entityDef->modelMatrix, sizeof( vEntity->modelMatrix ) );
	R_MatrixMultiply( entityDef->modelMatrix, viewDef->worldSpace.modelViewMatrix, vEntity->modelViewMatrix );

	idRenderMatrix viewMat;
	idRenderMatrix::Transpose( *( idRenderMatrix* )vEntity->modelViewMatrix, viewMat );
	idRenderMatrix::Multiply( viewDef->projectionRenderMatrix, viewMat, vEntity->mvp );
	idRenderMatrix::Multiply( viewDef->unjitteredProjectionRenderMatrix, viewMat, vEntity->unjitteredMVP );
	if( renderEntity->weaponDepthHack )
	{
		idRenderMatrix::ApplyDepthHack( vEntity->mvp );
	}
	if( renderEntity->modelDepthHack != 0.0f )
	{
		idRenderMatrix::ApplyModelDepthHack( vEntity->mvp, renderEntity->modelDepthHack );
	}

	// local light and view origins are used to determine if the view is definitely outside
	// an extruded shadow volume, which means we can skip drawing the end caps
	idVec3 localViewOrigin;
	R_GlobalPointToLocal( vEntity->modelMatrix, viewDef->renderView.vieworg, localViewOrigin );

	extern idCVar r_lodMaterialDistance;

	//---------------------------
	// add all the model surfaces
	//---------------------------
	bool occlusionSurface = false;
	for( int surfaceNum = 0; surfaceNum < model->NumSurfaces(); surfaceNum++ )
	{
		const modelSurface_t* surf = model->Surface( surfaceNum );

		const idMaterial* shader = surf->shader;
		if( shader == NULL )
		{
			continue;
		}

		if( shader->IsOccluder() )
		{
			occlusionSurface = true;
		}
	}

	for( int surfaceNum = 0; surfaceNum < model->NumSurfaces(); surfaceNum++ )
	{
		const modelSurface_t* surf = model->Surface( surfaceNum );

		// for debugging, only show a single surface at a time
		if( r_singleSurface.GetInteger() >= 0 && surfaceNum != r_singleSurface.GetInteger() )
		{
			continue;
		}

		srfTriangles_t* tri = surf->geometry;
		if( tri == NULL )
		{
			continue;
		}
		if( tri->numIndexes == 0 )
		{
			continue;		// happens for particles
		}
		const idMaterial* shader = surf->shader;
		if( shader == NULL )
		{
			continue;
		}

		// if the model has a occlusion surface and this surface is not a occluder
		if( occlusionSurface && !shader->IsOccluder() )
		{
			continue;
		}

		// motorsep 11-24-2014; checking for LOD surface for LOD1 iteration
		if( shader->IsLOD() )
		{
			// foresthale 2014-11-24: calculate the bounds and get the distance from camera to bounds
			idBounds& localBounds = tri->bounds;
			if( tri->staticModelWithJoints )
			{
				// skeletal models have difficult to compute bounds for surfaces, so use the whole entity
				localBounds = vEntity->entityDef->localReferenceBounds;
			}
			const float* bounds = localBounds.ToFloatPtr();
			idVec3 nearestPointOnBounds = localViewOrigin;
			nearestPointOnBounds.x = Max( nearestPointOnBounds.x, bounds[0] );
			nearestPointOnBounds.x = Min( nearestPointOnBounds.x, bounds[3] );
			nearestPointOnBounds.y = Max( nearestPointOnBounds.y, bounds[1] );
			nearestPointOnBounds.y = Min( nearestPointOnBounds.y, bounds[4] );
			nearestPointOnBounds.z = Max( nearestPointOnBounds.z, bounds[2] );
			nearestPointOnBounds.z = Min( nearestPointOnBounds.z, bounds[5] );
			idVec3 delta = nearestPointOnBounds - localViewOrigin;
			float distance = delta.LengthFast();

			if( !shader->IsLODVisibleForDistance( distance, r_lodMaterialDistance.GetFloat() ) )
			{
				continue;
			}
		}

		// foresthale 2014-09-01: don't skip surfaces that use the "forceShadows" flag
		if( !shader->IsDrawn() && !shader->SurfaceCastsShadow() && !shader->IsOccluder() )
		{
			continue;		// collision hulls, etc
		}

		// RemapShaderBySkin
		if( entityDef->parms.customShader != NULL )
		{
			// this is sort of a hack, but causes deformed surfaces to map to empty surfaces,
			// so the item highlight overlay doesn't highlight the autosprite surface
			if( shader->Deform() )
			{
				continue;
			}
			shader = entityDef->parms.customShader;
		}
		else if( entityDef->parms.customSkin )
		{
			shader = entityDef->parms.customSkin->RemapShaderBySkin( shader );
			if( shader == NULL )
			{
				continue;
			}
			// foresthale 2014-09-01: don't skip surfaces that use the "forceShadows" flag
			if( !shader->IsDrawn() && !shader->SurfaceCastsShadow() )
			{
				continue;
			}
		}

		// optionally override with the renderView->globalMaterial
		if( tr.primaryRenderView.globalMaterial != NULL )
		{
			shader = tr.primaryRenderView.globalMaterial;
		}

		SCOPED_PROFILE_EVENT( shader->GetName() );

		// debugging tool to make sure we have the correct pre-calculated bounds
		if( r_checkBounds.GetBool() )
		{
			for( int j = 0; j < tri->numVerts; j++ )
			{
				int k;
				for( k = 0; k < 3; k++ )
				{
					if( tri->verts[j].xyz[k] > tri->bounds[1][k] + CHECK_BOUNDS_EPSILON
							|| tri->verts[j].xyz[k] < tri->bounds[0][k] - CHECK_BOUNDS_EPSILON )
					{
						common->Printf( "bad tri->bounds on %s:%s\n", entityDef->parms.hModel->Name(), shader->GetName() );
						break;
					}
					if( tri->verts[j].xyz[k] > entityDef->localReferenceBounds[1][k] + CHECK_BOUNDS_EPSILON
							|| tri->verts[j].xyz[k] < entityDef->localReferenceBounds[0][k] - CHECK_BOUNDS_EPSILON )
					{
						common->Printf( "bad referenceBounds on %s:%s\n", entityDef->parms.hModel->Name(), shader->GetName() );
						break;
					}
				}
				if( k != 3 )
				{
					break;
				}
			}
		}

		// view frustum culling for the precise surface bounds, which is tighter
		// than the entire entity reference bounds
		// If the entire model wasn't visible, there is no need to check the
		// individual surfaces.
		const bool surfaceDirectlyVisible = modelIsVisible && !idRenderMatrix::CullBoundsToMVP( vEntity->mvp, tri->bounds );

		// RB: added check wether GPU skinning is available at all
		const bool gpuSkinned = ( tri->staticModelWithJoints != NULL && r_useGPUSkinning.GetBool() );

		//--------------------------
		// base drawing surface
		//--------------------------
		const float* shaderRegisters = NULL;
		drawSurf_t* baseDrawSurf = NULL;

		if( surfaceDirectlyVisible &&
				( ( shader->IsDrawn() && shader->Coverage() == MC_OPAQUE && !renderEntity->weaponDepthHack && renderEntity->modelDepthHack == 0.0f ) || shader->IsOccluder() )
		  )
		{
			// render to masked occlusion buffer

			//if( !gpuSkinned )

			// render the BSP area surfaces and from static model entities only the occlusion surfaces to keep the tris count at minimum
			if( model->IsStaticWorldModel() || ( shader->IsOccluder() && !gpuSkinned ) )
			{
				tr.pc.c_mocIndexes += tri->numIndexes;
				tr.pc.c_mocVerts += tri->numIndexes;

				R_CreateMaskedOcclusionCullingTris( tri );

				idRenderMatrix mvp;
				idRenderMatrix::Transpose( vEntity->unjitteredMVP, mvp );

#if MOC_MULTITHREADED
				tr.maskedOcclusionThreaded->SetMatrix( ( float* )&mvp[0][0] );
				tr.maskedOcclusionThreaded->RenderTriangles( tri->mocVerts->ToFloatPtr(), tri->mocIndexes, tri->numIndexes / 3, MaskedOcclusionCulling::BACKFACE_CCW, MaskedOcclusionCulling::CLIP_PLANE_ALL );
#else
				tr.maskedOcclusionCulling->RenderTriangles( tri->mocVerts->ToFloatPtr(), tri->mocIndexes, tri->numIndexes / 3, ( float* )&mvp[0][0], MaskedOcclusionCulling::BACKFACE_CCW, MaskedOcclusionCulling::CLIP_PLANE_ALL, MaskedOcclusionCulling::VertexLayout( 16, 4, 8 ) );
#endif
			}
#if 0
			else
			{
				tr.pc.c_mocIndexes += 36;
				tr.pc.c_mocVerts += 8;

				idBounds surfaceBounds;
				if( gpuSkinned )
				{
					surfaceBounds = vEntity->entityDef->localReferenceBounds;
				}
				else
				{
					surfaceBounds = tri->bounds;
				}

				idRenderMatrix modelRenderMatrix;
				idRenderMatrix::CreateFromOriginAxis( renderEntity->origin, renderEntity->axis, modelRenderMatrix );

				idRenderMatrix inverseBaseModelProject;
				idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, surfaceBounds, inverseBaseModelProject );

				idRenderMatrix invProjectMVPMatrix;
				idRenderMatrix::Multiply( viewDef->worldSpace.unjitteredMVP, inverseBaseModelProject, invProjectMVPMatrix );

				// NOTE: unit cube instead of zeroToOne cube
				idVec4* verts = tr.maskedUnitCubeVerts;
				idVec4 triVerts[8];

				for( int i = 0; i < 8; i++ )
				{
					// transform to clip space
					invProjectMVPMatrix.TransformPoint( verts[i], triVerts[i] );
				}

				tr.maskedOcclusionCulling->RenderTriangles( ( float* )triVerts, tr.maskedZeroOneCubeIndexes, 12, NULL, MaskedOcclusionCulling::BACKFACE_CCW );
			}

#elif 0
			else
			{
				idBounds surfaceBounds;
				if( gpuSkinned )
				{
					surfaceBounds = vEntity->entityDef->localReferenceBounds;
				}
				else
				{
					surfaceBounds = tri->bounds;
				}

				idRenderMatrix modelRenderMatrix;
				idRenderMatrix::CreateFromOriginAxis( renderEntity->origin, renderEntity->axis, modelRenderMatrix );

				idRenderMatrix inverseBaseModelProject;
				idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, surfaceBounds, inverseBaseModelProject );

				// NOTE: unit cube instead of zeroToOne cube
				idVec4* verts = tr.maskedUnitCubeVerts;
				idVec4 triVerts[8];

				for( int i = 0; i < 8; i++ )
				{
					// transform to clip space
					inverseBaseModelProject.TransformPoint( verts[i], triVerts[i] );
				}

				static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
				idVec4 color = colors[surfaceNum & 7];

				// same as idRenderWorldLocal::DebugBox
				const int lifetime = 0;
				for( int i = 0; i < 4; i++ )
				{
					tr.viewDef->renderWorld->DebugLine( color, triVerts[i].ToVec3(), triVerts[( i + 1 ) & 3].ToVec3(), lifetime );
					tr.viewDef->renderWorld->DebugLine( color, triVerts[4 + i].ToVec3(), triVerts[4 + ( ( i + 1 ) & 3 )].ToVec3(), lifetime );
					tr.viewDef->renderWorld->DebugLine( color, triVerts[i].ToVec3(), triVerts[4 + i].ToVec3(), lifetime );
				}
			}
#endif

			/*
			// add the surface for drawing
			// we can re-use some of the values for light interaction surfaces
			baseDrawSurf = ( drawSurf_t* )R_FrameAlloc( sizeof( *baseDrawSurf ), FRAME_ALLOC_DRAW_SURFACE );
			baseDrawSurf->frontEndGeo = tri;
			baseDrawSurf->space = vEntity;
			baseDrawSurf->scissorRect = vEntity->scissorRect;
			baseDrawSurf->extraGLState = 0;

			R_SetupDrawSurfShader( baseDrawSurf, shader, renderEntity );

			shaderRegisters = baseDrawSurf->shaderRegisters;

			// Check for deformations (eyeballs, flares, etc)
			const deform_t shaderDeform = shader->Deform();
			if( shaderDeform != DFRM_NONE )
			{
				drawSurf_t* deformDrawSurf = R_DeformDrawSurf( baseDrawSurf );
				if( deformDrawSurf != NULL )
				{
					// any deforms may have created multiple draw surfaces
					for( drawSurf_t* surf = deformDrawSurf, * next = NULL; surf != NULL; surf = next )
					{
						next = surf->nextOnLight;

						surf->linkChain = NULL;
						surf->nextOnLight = vEntity->drawSurfs;
						vEntity->drawSurfs = surf;
					}
				}
			}

			// Most deform source surfaces do not need to be rendered.
			// However, particles are rendered in conjunction with the source surface.
			if( shaderDeform == DFRM_NONE || shaderDeform == DFRM_PARTICLE || shaderDeform == DFRM_PARTICLE2 )
			{
				// copy verts and indexes to this frame's hardware memory if they aren't already there
				if( !vertexCache.CacheIsCurrent( tri->ambientCache ) )
				{
					tri->ambientCache = vertexCache.AllocVertex( tri->verts, tri->numVerts );
				}
				if( !vertexCache.CacheIsCurrent( tri->indexCache ) )
				{
					tri->indexCache = vertexCache.AllocIndex( tri->indexes, tri->numIndexes );
				}

				R_SetupDrawSurfJoints( baseDrawSurf, tri, shader );

				baseDrawSurf->numIndexes = tri->numIndexes;
				baseDrawSurf->ambientCache = tri->ambientCache;
				baseDrawSurf->indexCache = tri->indexCache;

				baseDrawSurf->linkChain = NULL;		// link to the view
				baseDrawSurf->nextOnLight = vEntity->drawSurfs;
				vEntity->drawSurfs = baseDrawSurf;
			}
			*/
		}
	}
}
#endif

//REGISTER_PARALLEL_JOB( R_AddSingleModel, "R_AddSingleModel" );



/*
===================
R_FillMaskedOcclusionBufferWithModels
===================
*/
void R_FillMaskedOcclusionBufferWithModels( viewDef_t* viewDef )
{
	SCOPED_PROFILE_EVENT( "R_FillMaskedOcclusionBufferWithModels" );

	tr.viewDef->viewEntitys = R_SortViewEntities( tr.viewDef->viewEntitys );

#if defined(USE_INTRINSICS_SSE)
	if( !r_useMaskedOcclusionCulling.GetBool() )
	{
		return;
	}

	int startTime = Sys_Microseconds();

	int viewWidth = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	int viewHeight = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

	// reduce to half res
	viewWidth = idMath::Floor( viewWidth * 0.5f );
	viewHeight = idMath::Floor( viewHeight * 0.5f );

	if( viewWidth & 7 )
	{
		// must be multiple of 8
		viewWidth = ( viewWidth + 7 ) & ~7;
	}

	if( viewHeight & 3 )
	{
		// must be multiple of 4
		viewHeight = ( viewHeight + 3 ) & ~3;
	}

	const float zNear = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();

#if MOC_MULTITHREADED
	tr.maskedOcclusionThreaded->SetResolution( viewWidth, viewHeight );
	tr.maskedOcclusionThreaded->SetNearClipPlane( zNear );
	tr.maskedOcclusionThreaded->ClearBuffer();

#else
	tr.maskedOcclusionCulling->SetResolution( viewWidth, viewHeight );
	tr.maskedOcclusionCulling->SetNearClipPlane( zNear );
	tr.maskedOcclusionCulling->ClearBuffer();
#endif

	//-------------------------------------------------
	// Go through each view entity that is either visible to the view, or to
	// any light that intersects the view (for shadows).
	//-------------------------------------------------

	/*
	if( r_useParallelAddModels.GetBool() )
	{
		for( viewEntity_t* vEntity = tr.viewDef->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
		{
			tr.frontEndJobList->AddJob( ( jobRun_t )R_AddSingleModel, vEntity );
		}
		tr.frontEndJobList->Submit();
		tr.frontEndJobList->Wait();
	}
	else
	*/
	{
		for( viewEntity_t* vEntity = tr.viewDef->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
		{
			const idRenderModel* model = vEntity->entityDef->parms.hModel;

			// skip after rendering BSP area models
			if( !model->IsStaticWorldModel() )
			{
				//continue;
			}

			R_RenderSingleModel( vEntity );
		}
	}

#if MOC_MULTITHREADED
	// wait for jobs to be finished
	tr.maskedOcclusionThreaded->Flush();
#endif

	int endTime = Sys_Microseconds();

	tr.pc.mocMicroSec += endTime - startTime;
#endif
}

#if defined(USE_INTRINSICS_SSE)
static void TonemapDepth( float* depth, unsigned char* image, int w, int h )
{
	// Find min/max w coordinate (discard cleared pixels)
	float minW = idMath::INFINITUM, maxW = 0.0f;
	for( int i = 0; i < w * h; ++i )
	{
		if( depth[i] > 0.0f )
		{
			minW = std::min( minW, depth[i] );
			maxW = std::max( maxW, depth[i] );
		}
	}

	// Tonemap depth values
	for( int i = 0; i < w * h; ++i )
	{
		int intensity = 0;
		if( depth[i] > 0 )
		{
			intensity = ( unsigned char )( 223.0 * ( depth[i] - minW ) / ( maxW - minW ) + 32.0 );
		}

		image[i * 3 + 0] = intensity;
		image[i * 3 + 1] = intensity;
		image[i * 3 + 2] = intensity;
	}
}

CONSOLE_COMMAND( maskShot, "Dumping masked occlusion culling buffer", NULL )
{
	unsigned int width, height;

	tr.maskedOcclusionCulling->GetResolution( width, height );

	// compute a per pixel depth buffer from the hierarchical depth buffer, used for visualization
	float* perPixelZBuffer = new float[width * height];

#if MOC_MULTITHREADED
	tr.maskedOcclusionThreaded->ComputePixelDepthBuffer( perPixelZBuffer, false );
#else
	tr.maskedOcclusionCulling->ComputePixelDepthBuffer( perPixelZBuffer, false );
#endif

	halfFloat_t* halfImage = new halfFloat_t[width * height * 3];

	for( unsigned int i = 0; i < ( width * height ); i++ )
	{
		float depth = perPixelZBuffer[i];
		halfFloat_t f16Depth = F32toF16( depth );

		halfImage[ i * 3 + 0 ] = f16Depth;
		halfImage[ i * 3 + 1 ] = f16Depth;
		halfImage[ i * 3 + 2 ] = f16Depth;
	}

	// write raw values
	R_WriteEXR( "screenshots/soft_occlusion_buffer.exr", halfImage, 3, width, height, "fs_basepath" );

	// tonemap the image
	unsigned char* image = new unsigned char[width * height * 3];
	TonemapDepth( perPixelZBuffer, image, width, height );

	R_WritePNG( "screenshots/soft_occlusion_buffer.png", image, 3, width, height, "fs_basepath" );
	delete[] image;
}
#endif
