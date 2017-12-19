
#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

/*
=====================================================================================================
	
	idRenderView	

	A view may be either the actual camera view,
	a mirror / remote location, or a 3D view on a gui surface.

=====================================================================================================
*/

idCVar r_centerX( "r_centerX", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_centerY( "r_centerY", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );
idCVar r_zfar( "r_zfar", "131072", CVAR_RENDERER | CVAR_FLOAT, "far Z clip plane distance", 0.001f, 999999.0f );
idCVar r_useInfiniteFarZ( "r_useInfiniteFarZ", "1", CVAR_RENDERER | CVAR_BOOL, "use the no-far-clip-plane trick, ignoring r_zfar value" );
idCVar r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix", 0, 1 );

/*
======================
======================
*/
float idRenderView::GetZNear() const
{
	return ( parms.cramZNear )? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();
}

/*
=================
idRenderView::DeriveData
=================
*/
void idRenderView::DeriveData()
{
	if( this->is2Dgui )
	{
		this->worldSpace.modelMatrix.Identity();

		this->worldSpace.modelViewMatrix.Identity();

		idRenderMatrix::CreateOrthogonalProjection(
			renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 1.0f, this->projectionMatrix, true );

		// no other data is needed in 2d rendering ?
		return;
	}
	// setup the matrix for world space to eye space
	{		
		memset( &this->worldSpace, 0, sizeof( this->worldSpace ) );

		// the model matrix is an identity
		this->worldSpace.modelMatrix.Identity();

		// transform by the camera placement,
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		idRenderMatrix::CreateViewMatrix( this->GetOrigin(), this->GetAxis(), this->worldSpace.modelViewMatrix );
	}

	// we need to set the projection matrix before doing portal-to-screen scissor calculations
	{
		// random jittering is usefull when multiple frames are going to be blended together
		// for motion blurred anti-aliasing
		float jitterx = 0.0f; float jittery = 0.0f;
		if( r_jitter.GetBool() && !this->isSubview )
		{
			static idRandom random;
			jitterx = random.RandomFloat();
			jittery = random.RandomFloat();
		}

		//
		// set up projection matrix
		//
		const float zNear = GetZNear();
		const float zFar = r_useInfiniteFarZ.GetBool() ? 0.0 : r_zfar.GetFloat();

		float xmax = zNear * idMath::Tan( DEG2RAD( this->parms.fov_x ) * 0.5f );
		float ymax = zNear * idMath::Tan( DEG2RAD( this->parms.fov_y ) * 0.5f );

		float ymin = -ymax;
		float xmin = -xmax;

		const float width = xmax - xmin;
		const float height = ymax - ymin;

		const int viewWidth = this->viewport.x2 - this->viewport.x1 + 1;
		const int viewHeight = this->viewport.y2 - this->viewport.y1 + 1;

		jitterx = jitterx * width / viewWidth;
		jitterx += r_centerX.GetFloat();
		jitterx += this->parms.stereoScreenSeparation;

		jittery = jittery * height / viewHeight;
		jittery += r_centerY.GetFloat();

		xmin += jitterx * width;  ymin += jittery * height;
		xmax += jitterx * width;  ymax += jittery * height;

		idRenderMatrix::CreateProjectionMatrix( xmin, xmax, ymin, ymax, zNear, zFar, this->projectionMatrix );
	}

	// Lots of usefull stuff here for postproc, deferred passes, faster culling etc.

	idRenderMatrix::Multiply( this->GetProjectionMatrix(), this->GetViewMatrix(), this->worldSpace.mvp );

	if( !idRenderMatrix::Inverse( this->GetViewMatrix(), this->inverseViewMatrix ) )
	{
		idLib::Warning( "SetupUtilityMatrices() ViewMatrix invert failed" );
	}
	if( !idRenderMatrix::Inverse( this->GetProjectionMatrix(), this->inverseProjectionMatrix ) )
	{
		idLib::Warning( "SetupUtilityMatrices() ProjectionMatrix invert failed" );
	}
	if( !idRenderMatrix::Inverse( this->GetMVPMatrix(), this->inverseViewProjectionMatrix ) )
	{
		idLib::Warning( "SetupUtilityMatrices() MVPMatrix invert failed" );
	}

	// the planes of the view frustum are needed for portal visibility culling
	{
		idRenderMatrix::GetFrustumPlanes( &this->frustums[ FRUSTUM_PRIMARY ][ 0 ], this->GetMVPMatrix(), false, true );

		// the DOOM 3 frustum planes point outside the frustum
		for( int i = 0; i < 6; i++ )
		{
			this->frustums[ FRUSTUM_PRIMARY ][ i ] = -this->frustums[ FRUSTUM_PRIMARY ][ i ];
		}
		// remove the Z-near to avoid portals from being near clipped
		this->frustums[ FRUSTUM_PRIMARY ][ 4 ][ 3 ] -= r_znear.GetFloat();
	}

	// RB Shadow maps split frustums 
	if( r_useShadowMapping.GetBool() )
	{
		const float zNearStart = this->GetZNear();
		const float zFarEnd = 10000;

		const float lambda = r_shadowMapSplitWeight.GetFloat();
		const float ratio = zFarEnd / zNearStart;

		float zNear = zNearStart;
		float zFar = zFarEnd;

		for( int i = 0; i < 6; i++ )
		{
			this->frustumSplitDistances[ i ] = idMath::INFINITY;
		}

		for( int i = 1; i <= ( r_shadowMapSplits.GetInteger() + 1 ) && i < MAX_FRUSTUMS; i++ )
		{
			float si = i / ( float )( r_shadowMapSplits.GetInteger() + 1 );

			if( i > FRUSTUM_CASCADE1 )
			{
				zNear = zFar - ( zFar * 0.005f );
			}

			zFar = 1.005f * lambda * ( zNearStart * idMath::Pow( ratio, si ) ) + ( 1 - lambda ) * ( zNearStart + ( zFarEnd - zNearStart ) * si );

			if( i <= r_shadowMapSplits.GetInteger() )
			{
				this->frustumSplitDistances[ i - 1 ] = zFar;
			}

			idRenderMatrix projectionRenderMatrix;
			idRenderMatrix::CreateProjectionMatrixFov( this->GetFOVx(), this->GetFOVy(), zNear, zFar, 0.0f, 0.0f, projectionRenderMatrix );

			idRenderMatrix frustumMVP;
			idRenderMatrix::Multiply( projectionRenderMatrix, this->GetViewMatrix(), frustumMVP );

			idRenderMatrix::GetFrustumPlanes( &this->frustums[ i ][ 0 ], frustumMVP, false, true );

			// the DOOM 3 frustum planes point outside the frustum
			for( int j = 0; j < 6; j++ )
			{
				this->frustums[ i ][ j ] = -this->frustums[ i ][ j ];
			}
			// remove the Z-near to avoid portals from being near clipped
			if( i == FRUSTUM_CASCADE1 )
			{
				this->frustums[ i ][ 4 ][ 3 ] -= r_znear.GetFloat();
			}

			if( !idRenderMatrix::Inverse( frustumMVP, this->frustumInvMVPs[ i ] ) )
			{
				idLib::Warning( "SetupFrustums() frustumMVPs invert failed" );
			}
		}
	}
}

/*
==========================
 R_GlobalToNormalizedDeviceCoordinates

	-1 to 1 range in x, y, and z
==========================
*/
void idRenderView::GlobalToNormalizedDeviceCoordinates( const idVec3& global, idVec3& ndc ) const
{
	idVec4 view, clip;
	idRenderMatrix::TransformModelToClip( global, GetViewMatrix(), GetProjectionMatrix(), view, clip );
	idRenderMatrix::TransformClipToDevice( clip, ndc );
}

/*
===================
 ScreenRectForWinding
===================
*/
void idRenderView::ScreenRectFromWinding( const idWinding & w, const idRenderMatrix & modelMatrix, idScreenRect & screenRect ) const
{
	const float viewWidth  = ( float )this->viewport.x2 - ( float )this->viewport.x1;
	const float viewHeight = ( float )this->viewport.y2 - ( float )this->viewport.y1;

	screenRect.Clear();
	for( int i = 0; i < w.GetNumPoints(); i++ )
	{
		idVec3 v, ndc;

		modelMatrix.TransformPoint( w[ i ].ToVec3(), v );

		GlobalToNormalizedDeviceCoordinates( v, ndc );

		screenRect.AddPoint( 
			( ndc[ 0 ] * 0.5f + 0.5f ) * viewWidth,		// windowX
			( ndc[ 1 ] * 0.5f + 0.5f ) * viewHeight		// windowY
		);
	}

	screenRect.Expand();
}
