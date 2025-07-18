/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2023 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel
Copyright (C) 2022 Stephen Pridham

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

#ifndef __TR_LOCAL_H__
#define __TR_LOCAL_H__

#include "precompiled.h"

#include "GLState.h"
#include "ScreenRect.h"
#include "Image.h"
#include "Font.h"
#include "Framebuffer.h"

// maximum texture units
const int MAX_PROG_TEXTURE_PARMS	= 16;

const int FALLOFF_TEXTURE_SIZE		= 64;

const float	DEFAULT_FOG_DISTANCE	= 500.0f;

// picky to get the bilerp correct at terminator
const int FOG_ENTER_SIZE			= 64;
const float FOG_ENTER				= ( FOG_ENTER_SIZE + 1.0f ) / ( FOG_ENTER_SIZE * 2 );


/*
==============================================================================

SURFACES

==============================================================================
*/

#include "ModelDecal.h"
#include "ModelOverlay.h"
#include "Interaction.h"

// RB begin
#define MOC_MULTITHREADED 0

#if MOC_MULTITHREADED
	class CullingThreadpool;
#endif
class MaskedOcclusionCulling;
// RB end

class idRenderWorldLocal;
struct viewEntity_t;
struct viewLight_t;
struct viewEnvprobe_t;

// drawSurf_t structures command the back end to render surfaces
// a given srfTriangles_t may be used with multiple viewEntity_t,
// as when viewed in a subview or multiple viewport render, or
// with multiple shaders when skinned, or, possibly with multiple
// lights, although currently each lighting interaction creates
// unique srfTriangles_t
// drawSurf_t are always allocated and freed every frame, they are never cached

struct drawSurf_t
{
	const srfTriangles_t* 	frontEndGeo;		// don't use on the back end, it may be updated by the front end!
	int						numIndexes;
	vertCacheHandle_t		indexCache;			// triIndex_t
	vertCacheHandle_t		ambientCache;		// idDrawVert
	vertCacheHandle_t		jointCache;			// idJointMat
	const viewEntity_t* 	space;
	const idMaterial* 		material;			// may be NULL for shadow volumes
	uint64					extraGLState;		// Extra GL state |'d with material->stage[].drawStateBits
	float					sort;				// material->sort, modified by gui / entity sort offsets
	const float* 			shaderRegisters;	// evaluated and adjusted for referenceShaders
	drawSurf_t* 			nextOnLight;		// viewLight chains
	drawSurf_t** 			linkChain;			// defer linking to lights to a serial section to avoid a mutex
	idScreenRect			scissorRect;		// for scissor clipping, local inside renderView viewport
	const struct portalArea_s*	area;			// RB: if != NULL then the area provides valid lightgrid
};

// areas have references to hold all the lights and entities in them
struct areaReference_t
{
	areaReference_t* 		areaNext;				// chain in the area
	areaReference_t* 		areaPrev;
	areaReference_t* 		ownerNext;				// chain on either the entityDef or lightDef

	idRenderEntityLocal* 	entity;					// only one of entity / light / envprobe will be non-NULL
	idRenderLightLocal* 	light;					// only one of entity / light / envprobe will be non-NULL
	RenderEnvprobeLocal*	envprobe;				// only one of entity / light / envprobe will be non-NULL

	struct portalArea_s*		area;				// so owners can find all the areas they are in
};


// idRenderLight should become the new public interface replacing the qhandle_t to light defs in the idRenderWorld interface
class idRenderLight
{
public:
	virtual					~idRenderLight() {}

	virtual void			FreeRenderLight() = 0;
	virtual void			UpdateRenderLight( const renderLight_t* re, bool forceUpdate = false ) = 0;
	virtual void			GetRenderLight( renderLight_t* re ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;
};

// RB : RennderEnvprobe should become the new public interface replacing the qhandle_t to envprobe defs in the idRenderWorld interface
class RenderEnvprobe
{
public:
	virtual					~RenderEnvprobe() {}

	virtual void			FreeRenderEnvprobe() = 0;
	virtual void			UpdateRenderEnvprobe( const renderEnvironmentProbe_t* ep, bool forceUpdate = false ) = 0;
	virtual void			GetRenderEnvprobe( renderEnvironmentProbe_t* ep ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;
};
// RB end

// idRenderEntity should become the new public interface replacing the qhandle_t to entity defs in the idRenderWorld interface
class idRenderEntity
{
public:
	virtual					~idRenderEntity() {}

	virtual void			FreeRenderEntity() = 0;
	virtual void			UpdateRenderEntity( const renderEntity_t* re, bool forceUpdate = false ) = 0;
	virtual void			GetRenderEntity( renderEntity_t* re ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial* material ) = 0;
	virtual void			RemoveDecals() = 0;
};


class idRenderLightLocal : public idRenderLight
{
public:
	idRenderLightLocal();

	virtual void			FreeRenderLight();
	virtual void			UpdateRenderLight( const renderLight_t* re, bool forceUpdate = false );
	virtual void			GetRenderLight( renderLight_t* re );
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	bool					LightCastsShadows() const
	{
		return parms.forceShadows || ( !parms.noShadows && lightShader->LightCastsShadows() );
	}

	renderLight_t			parms;					// specification

	bool					lightHasMoved;			// the light has changed its position since it was
	// first added, so the prelight model is not valid
	idRenderWorldLocal* 	world;
	int						index;					// in world lightdefs

	int						areaNum;				// if not -1, we may be able to cull all the light's
	// interactions if !viewDef->connectedAreas[areaNum]

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory

	// derived information
	idPlane					lightProject[4];		// old style light projection where Z and W are flipped and projected lights lightProject[3] is divided by ( zNear + zFar )
	idRenderMatrix			baseLightProject;		// global xyz1 to projected light strq
	idRenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space

	const idMaterial* 		lightShader;			// guaranteed to be valid, even if parms.shader isn't
	idImage* 				falloffImage;

	idVec3					globalLightOrigin;		// accounting for lightCenter and parallel
	idBounds				globalLightBounds;

	int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
	viewLight_t* 			viewLight;

	areaReference_t* 		references;				// each area the light is present in will have a lightRef
	idInteraction* 			firstInteraction;		// doubly linked list
	idInteraction* 			lastInteraction;

	struct doublePortal_s* 	foggedPortals;
};


// RB begin
class RenderEnvprobeLocal : public RenderEnvprobe
{
public:
	RenderEnvprobeLocal();

	virtual void			FreeRenderEnvprobe() override;
	virtual void			UpdateRenderEnvprobe( const renderEnvironmentProbe_t* ep, bool forceUpdate = false ) override;
	virtual void			GetRenderEnvprobe( renderEnvironmentProbe_t* ep ) override;
	virtual void			ForceUpdate() override;
	virtual int				GetIndex() override;

	renderEnvironmentProbe_t	parms;					// specification

	bool						envprobeHasMoved;		// the light has changed its position since it was
	// first added, so the prelight model is not valid
	idRenderWorldLocal* 		world;
	int							index;					// in world envprobeDefs

	int							areaNum;				// if not -1, we may be able to cull all the envprobe's
	// interactions if !viewDef->connectedAreas[areaNum]

	int							lastModifiedFrameNum;	// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory

	// derived information
	idRenderMatrix				inverseBaseProbeProject;// transforms the zero-to-one cube to exactly cover the light in world space

	idBounds					globalProbeBounds;

	areaReference_t* 			references;				// each area the light is present in will have a lightRef

	idImage* 					irradianceImage;		// cubemap image used for diffuse IBL by backend
	idImage* 					radianceImage;			// cubemap image used for specular IBL by backend

	// temporary helpers
	int							viewCount;				// if == tr.viewCount, the envprobe is on the viewDef->viewEnvprobes list
	viewEnvprobe_t* 			viewEnvprobe;
};
// RB end

class idRenderEntityLocal : public idRenderEntity
{
public:
	idRenderEntityLocal();

	virtual void			FreeRenderEntity();
	virtual void			UpdateRenderEntity( const renderEntity_t* re, bool forceUpdate = false );
	virtual void			GetRenderEntity( renderEntity_t* re );
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial* material );
	virtual void			RemoveDecals();

	bool					IsDirectlyVisible() const;
	renderEntity_t			parms;

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin
	idRenderMatrix			modelRenderMatrix;
	idRenderMatrix			inverseBaseModelProject;// transforms the unit cube to exactly cover the model in world space

	idRenderWorldLocal* 	world;
	int						index;					// in world entityDefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory

	idRenderModel* 			dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
	// dynamicModel if this doesn't == tr.viewCount
	idRenderModel* 			cachedDynamicModel;

	// the local bounds used to place entityRefs, either from parms for dynamic entities, or a model bounds
	idBounds				localReferenceBounds;

	// axis aligned bounding box in world space, derived from refernceBounds and
	// modelMatrix in R_CreateEntityRefs()
	idBounds				globalReferenceBounds;

	// a viewEntity_t is created whenever a idRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;				// if tr.viewCount == viewCount, viewEntity is valid,
	// but the entity may still be off screen
	viewEntity_t* 			viewEntity;				// in frame temporary memory

	idRenderModelDecal* 	decals;					// decals that have been projected on this model
	idRenderModelOverlay* 	overlays;				// blood overlays on animated models

	areaReference_t* 		entityRefs;				// chain of all references
	idInteraction* 			firstInteraction;		// doubly linked list
	idInteraction* 			lastInteraction;

	bool					needsPortalSky;
};

struct shadowOnlyEntity_t
{
	shadowOnlyEntity_t* 	next;
	idRenderEntityLocal*		edef;
};

// viewLights are allocated on the frame temporary stack memory
// a viewLight contains everything that the back end needs out of an idRenderLightLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum
struct viewLight_t
{
	viewLight_t* 			next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	idRenderLightLocal* 	lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals
	idScreenRect			scissorRect;

	// R_AddSingleLight() determined that the light isn't actually needed
	bool					removeFromList;

	// R_AddSingleLight builds this list of entities that need to be added
	// to the viewEntities list because they potentially cast shadows into
	// the view, even though the aren't directly visible
	shadowOnlyEntity_t* 	shadowOnlyViewEntities;

	enum interactionState_t
	{
		INTERACTION_UNCHECKED,
		INTERACTION_NO,
		INTERACTION_YES
	};
	byte* 					entityInteractionState;		// [numEntities]

	idVec3					globalLightOrigin;			// global light origin used by backend
	idPlane					lightProject[4];			// light project used by backend
	idPlane					fogPlane;					// fog plane for backend fog volume rendering
	// RB: added for shadow mapping
	idRenderMatrix			baseLightProject;			// global xyz1 to projected light strq
	bool					pointLight;					// otherwise a projection light (should probably invert the sense of this, because points are way more common)
	bool					parallel;					// lightCenter gives the direction to the light at infinity
	idVec3					lightCenter;				// offset the lighting direction for shading and
	int						shadowLOD;					// level of detail for shadowmap selection
	float					shadowFadeOut;				// blending from last shadow LOD to invisible
	idRenderMatrix			shadowV[6];					// shadow depth view matrix for lighting pass
	idRenderMatrix			shadowP[6];					// shadow depth projection matrix for lighting pass
	idVec2i					imageSize;
	idVec2i					imageAtlasOffset[6];
	// RB end
	idRenderMatrix			inverseBaseLightProject;	// the matrix for deforming the 'zeroOneCubeModel' to exactly cover the light volume in world space
	const idMaterial* 		lightShader;				// light shader used by backend
	const float*				shaderRegisters;			// shader registers used by backend
	idImage* 				falloffImage;				// falloff image used by backend

	drawSurf_t* 			globalShadows;				// shadow everything
	drawSurf_t* 			localInteractions;			// don't get local shadows
	drawSurf_t* 			localShadows;				// don't shadow local surfaces
	drawSurf_t* 			globalInteractions;			// get shadows from everything
	drawSurf_t* 			translucentInteractions;	// translucent interactions don't get shadows

	bool					ImageAtlasPlaced() const
	{
		return ( imageSize.x != -1 ) && ( imageSize.y != -1 );
	}
};

// a viewEntity is created whenever a idRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// viewEntity are allocated on the frame temporary stack memory
// a viewEntity contains everything that the back end needs out of a idRenderEntityLocal,
// which the front end may be modifying simultaneously if running in SMP mode.
// A single entityDef can generate multiple viewEntity_t in a single frame, as when seen in a mirror
struct viewEntity_t
{
	viewEntity_t* 			next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	idRenderEntityLocal*		entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewEntity can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	idScreenRect			scissorRect;

	bool					isGuiSurface;			// force two sided and vertex colors regardless of material setting

	bool					skipMotionBlur;

	bool					weaponDepthHack;
	float					modelDepthHack;

	float					modelMatrix[16];		// local coords to global coords
	float					modelViewMatrix[16];	// local coords to eye coords

	idRenderMatrix			mvp;
	idRenderMatrix			unjitteredMVP;			// no TAA subpixel jittering

	// parallelAddModels will build a chain of surfaces here that will need to
	// be linked to the lights or added to the drawsurf list in a serial code section
	drawSurf_t* 			drawSurfs;
};

// RB: viewEnvprobes are allocated on the frame temporary stack memory
// a viewEnvprobe contains everything that the back end needs out of an RenderEnvprobeLocal,
// which the front end may be modifying simultaniously if running in SMP mode.

// this structure will be especially helpful when we switch RBDOOM-3-BFG to forward cluster shading
// because then we can evaluate all viewEnvprobes properly in each pixel shader along with all other lighting information
struct viewEnvprobe_t
{
	viewEnvprobe_t* 		next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	RenderEnvprobeLocal* 	envprobeDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals
	idScreenRect			scissorRect;

	// R_AddSingleEnvprobe() determined that the light isn't actually needed
	bool					removeFromList;

	idVec3					globalOrigin;				// global envprobe origin used by backend
	idBounds				globalProbeBounds;

	idRenderMatrix			inverseBaseProbeProject;	// the matrix for deforming the 'zeroOneCubeModel' to exactly cover the light volume in world space
	idImage* 				irradianceImage;			// cubemap image used for diffuse IBL by backend
	idImage* 				radianceImage;				// cubemap image used for specular IBL by backend
};

struct calcEnvprobeParms_t
{
	// input
	byte*							radiance[6];			// HDR RGB16F standard OpenGL cubemap sides
	int								freeRadiance;
	int								samples;

	int								outWidth;
	int								outHeight;

	bool							printProgress;
	int								printWidth;
	int								printHeight;

	idStr							filename;

	// output
	halfFloat_t*					outBuffer;				// HDR RGB16F packed octahedron atlas
	int								time;					// execution time in milliseconds
};


#define STORE_LIGHTGRID_SHDATA 0

static const int LIGHTGRID_IRRADIANCE_BORDER_SIZE = 2;	// one pixel border all around the octahedron so 2 on each side
static const int LIGHTGRID_IRRADIANCE_SIZE = 14 + LIGHTGRID_IRRADIANCE_BORDER_SIZE;

struct calcLightGridPointParms_t
{
	// input
	byte*							radiance[6];			// HDR RGB16F standard OpenGL cubemap sides
	int								gridCoord[3];

	int								outWidth;				// LIGHTGRID_IRRADIANCE_SIZE
	int								outHeight;

	// output
#if STORE_LIGHTGRID_SHDATA
	SphericalHarmonicsT<idVec3, 4>	shRadiance;				// L4 Spherical Harmonics
#endif

	halfFloat_t*					outBuffer;				// HDR RGB16F octahedron LIGHTGRID_IRRADIANCE_SIZE^2
	int								time;					// execution time in milliseconds
};
// RB end

const int	MAX_CLIP_PLANES	= 1;				// we may expand this to six for some subview issues

// RB: added multiple subfrustums for cascaded shadow mapping
enum frustumPlanes_t
{
	FRUSTUM_PLANE_LEFT,
	FRUSTUM_PLANE_RIGHT,
	FRUSTUM_PLANE_BOTTOM,
	FRUSTUM_PLANE_TOP,
	FRUSTUM_PLANE_NEAR,
	FRUSTUM_PLANE_FAR,
	FRUSTUM_PLANES = 6,
	FRUSTUM_CLIPALL = 1 | 2 | 4 | 8 | 16 | 32
};

enum
{
	FRUSTUM_PRIMARY,
	FRUSTUM_CASCADE1,
	FRUSTUM_CASCADE2,
	FRUSTUM_CASCADE3,
	FRUSTUM_CASCADE4,
	FRUSTUM_CASCADE5,
	MAX_FRUSTUMS,
};

typedef idPlane frustum_t[FRUSTUM_PLANES];
// RB end

// viewDefs are allocated on the frame temporary stack memory
struct viewDef_t
{
	// specified in the call to DrawScene()
	renderView_t		renderView;

	float				projectionMatrix[16];
	idRenderMatrix		projectionRenderMatrix;	// tech5 version of projectionMatrix

	// RB begin
	float				unjitteredProjectionMatrix[16];		// second version without TAA subpixel jittering
	idRenderMatrix		unjitteredProjectionRenderMatrix;

	float				unprojectionToCameraMatrix[16];
	idRenderMatrix		unprojectionToCameraRenderMatrix;

	float				unprojectionToWorldMatrix[16];
	idRenderMatrix		unprojectionToWorldRenderMatrix;
	// RB end

	viewEntity_t		worldSpace;

	idRenderWorldLocal* renderWorld;

	idVec3				initialViewAreaOrigin;
	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.

	bool				isSubview;				// true if this view is not the main view
	bool				isMirror;				// the portal is a mirror, invert the face culling
	bool				isXraySubview;

	bool				isEditor;
	bool				is2Dgui;

	bool                isObliqueProjection;    // true if this view has an oblique projection
	int					numClipPlanes;			// mirrors will often use a single clip plane
	idPlane				clipPlanes[MAX_CLIP_PLANES];		// in world space, the positive side
	// of the plane is the visible side
	idScreenRect		viewport;				// in real pixels and proper Y flip

	idScreenRect		scissor;
	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height

	viewDef_t* 			superView;				// never go into an infinite subview loop
	const drawSurf_t* 	subviewSurface;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t** 		drawSurfs;				// we don't use an idList for this, because
	int					numDrawSurfs;			// it is allocated in frame temporary memory
	int					maxDrawSurfs;			// may be resized

	viewLight_t*		viewLights;				// chain of all viewLights effecting view
	viewEntity_t* 		viewEntitys;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	// RB begin
	frustum_t			frustums[MAX_FRUSTUMS];					// positive sides face outward, [4] is the front clip plane
	float				frustumSplitDistances[MAX_FRUSTUMS];
	idRenderMatrix		frustumMVPs[MAX_FRUSTUMS];
	// RB end

	int					areaNum;				// -1 = not in a valid area

	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.
	bool* 				connectedAreas;

	// RB: collect environment probes like lights
	viewEnvprobe_t*		viewEnvprobes;

	// RB: nearest 3 probes for now
	idBounds			globalProbeBounds;
	idRenderMatrix		inverseBaseEnvProbeProject;	// the matrix for deforming the 'zeroOneCubeModel' to exactly cover the environent probe volume in world space
	idImage* 			irradianceImage;			// cubemap image used for diffuse IBL by backend
	idImage* 			radianceImages[3];			// cubemap image used for specular IBL by backend
	idVec4				radianceImageBlends;		// blending weights
	idVec4				probePositions[3];			// only used by parallax correction

	Framebuffer*		targetRender;				// SP: The framebuffer to render to

	int					taaFrameCount;				// RB: so we have the same frame index in frontend and backend
};


// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
struct drawInteraction_t
{
	const viewLight_t*	vLight;

	const drawSurf_t* 	surf;

	idImage* 			bumpImage;
	idImage* 			diffuseImage;
	idImage* 			specularImage;

	idVec4				diffuseColor;	// may have a light color baked into it
	idVec4				specularColor;	// may have a light color baked into it
	stageVertexColor_t	vertexColor;	// applies to both diffuse and specular

	// these are loaded into the vertex program
	idVec4				bumpMatrix[2];
	idVec4				diffuseMatrix[2];
	idVec4				specularMatrix[2];
};

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

TR_CMDS

=============================================================
*/

enum renderCommand_t
{
	RC_NOP,
	RC_DRAW_VIEW_3D,		// may be at a reduced resolution, will be upsampled before 2D GUIs
	RC_DRAW_VIEW_GUI,		// not resolution scaled
	RC_SET_BUFFER,
	RC_COPY_RENDER,
	RC_POST_PROCESS,		// postfx after scene rendering is done but before GUI rendering
	RC_CRT_POST_PROCESS,	// CRT simulation after everything has been rendered on the final swapchain image
};

struct emptyCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;
};

struct setBufferCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;
	int					buffer;
};

struct drawSurfsCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;
	viewDef_t* 			viewDef;
};

struct copyRenderCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;
	int					x;
	int					y;
	int					imageWidth;
	int					imageHeight;
	idImage*				image;
	int					cubeFace;					// when copying to a cubeMap
	bool				clearColorAfterCopy;
};

struct postProcessCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;
	viewDef_t* 			viewDef;
};

struct crtPostProcessCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;
	int					padding;
};

//=======================================================================

// this is the inital allocation for max number of drawsurfs
// in a given view, but it will automatically grow if needed
const int INITIAL_DRAWSURFS =		2048;

enum frameAllocType_t
{
	FRAME_ALLOC_VIEW_DEF,
	FRAME_ALLOC_VIEW_ENTITY,
	FRAME_ALLOC_VIEW_LIGHT,
	FRAME_ALLOC_SURFACE_TRIANGLES,
	FRAME_ALLOC_DRAW_SURFACE,
	FRAME_ALLOC_INTERACTION_STATE,
	FRAME_ALLOC_SHADOW_ONLY_ENTITY,
	FRAME_ALLOC_SHADOW_VOLUME_PARMS,
	FRAME_ALLOC_SHADER_REGISTER,
	FRAME_ALLOC_DRAW_SURFACE_POINTER,
	FRAME_ALLOC_DRAW_COMMAND,
	FRAME_ALLOC_UNKNOWN,
	FRAME_ALLOC_MAX
};

// all of the information needed by the back end must be
// contained in a idFrameData.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine.
class idFrameData
{
public:
	idSysInterlockedInteger	frameMemoryAllocated;
	idSysInterlockedInteger	frameMemoryUsed;
	byte* 					frameMemory;

	int						highWaterAllocated;	// max used on any frame
	int						highWaterUsed;

	// the currently building command list commands can be inserted
	// at the front if needed, as required for dynamically generated textures
	emptyCommand_t* 		cmdHead;	// may be of other command type based on commandId
	emptyCommand_t* 		cmdTail;
};

extern	idFrameData*	frameData;

//=======================================================================

void R_AddDrawViewCmd( viewDef_t* parms, bool guiOnly );
void R_AddDrawPostProcess( viewDef_t* parms );

void R_ReloadGuis_f( const idCmdArgs& args );
void R_ListGuis_f( const idCmdArgs& args );

void* R_GetCommandBuffer( int bytes );

// this allows a global override of all materials
bool R_GlobalShaderOverride( const idMaterial** shader );

// this does various checks before calling the idDeclSkin
const idMaterial* R_RemapShaderBySkin( const idMaterial* shader, const idDeclSkin* customSkin, const idMaterial* customShader );


//====================================================

enum vertexLayoutType_t
{
	LAYOUT_UNKNOWN = 0,	// RB: TODO -1
	LAYOUT_DRAW_VERT,
	LAYOUT_DRAW_IMGUI_VERT, // unused
	NUM_VERTEX_LAYOUTS
};

enum bindingLayoutType_t
{
	// REGULAR AND SKINNED VERSIONS
	BINDING_LAYOUT_DEFAULT,
	BINDING_LAYOUT_DEFAULT_SKINNED,

	BINDING_LAYOUT_CONSTANT_BUFFER_ONLY,
	BINDING_LAYOUT_CONSTANT_BUFFER_ONLY_SKINNED,

	BINDING_LAYOUT_AMBIENT_LIGHTING_IBL,
	BINDING_LAYOUT_AMBIENT_LIGHTING_IBL_SKINNED,

	BINDING_LAYOUT_DRAW_INTERACTION,
	BINDING_LAYOUT_DRAW_INTERACTION_SKINNED,
	BINDING_LAYOUT_DRAW_INTERACTION_SM,
	BINDING_LAYOUT_DRAW_INTERACTION_SM_SKINNED,

	BINDING_LAYOUT_FOG,
	BINDING_LAYOUT_FOG_SKINNED,
	BINDING_LAYOUT_BLENDLIGHT,
	BINDING_LAYOUT_BLENDLIGHT_SKINNED,

	BINDING_LAYOUT_NORMAL_CUBE,
	BINDING_LAYOUT_NORMAL_CUBE_SKINNED,

	BINDING_LAYOUT_OCTAHEDRON_CUBE,
	BINDING_LAYOUT_OCTAHEDRON_CUBE_SKINNED,

	// NO GPU SKINNING ANYMORE
	BINDING_LAYOUT_POST_PROCESS_INGAME,
	BINDING_LAYOUT_POST_PROCESS_FINAL,
	BINDING_LAYOUT_POST_PROCESS_FINAL2,
	BINDING_LAYOUT_POST_PROCESS_CRT,

	BINDING_LAYOUT_BLIT,
	BINDING_LAYOUT_DRAW_AO,
	BINDING_LAYOUT_DRAW_AO1,

	BINDING_LAYOUT_BINK_VIDEO,

	// SMAA
	BINDING_LAYOUT_SMAA_EDGE_DETECTION,
	BINDING_LAYOUT_SMAA_WEIGHT_CALC,

	// NVRHI render passes specific
	BINDING_LAYOUT_TAA_MOTION_VECTORS,
	BINDING_LAYOUT_TAA_RESOLVE,

	BINDING_LAYOUT_TONEMAP,
	BINDING_LAYOUT_HISTOGRAM,
	BINDING_LAYOUT_EXPOSURE,

	NUM_BINDING_LAYOUTS
};

class idParallelJobList;

const int MAX_GUI_SURFACES	= 1024;		// default size of the drawSurfs list for guis, will
// be automatically expanded as needed

static const int MAX_RENDER_CROPS = 8;

#include "RenderBackend.h"

/*
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
class idRenderSystemLocal : public idRenderSystem
{
public:
	// external functions
	virtual void			Init();
	virtual void			Shutdown();
	virtual bool			IsInitialized() const
	{
		return bInitialized;
	}
	virtual void			ResetGuiModels();
	virtual void			InitBackend();
	virtual void			ShutdownOpenGL();
	virtual bool			IsOpenGLRunning() const;
	virtual bool			IsFullScreen() const;
	virtual int				GetWidth() const;
	virtual int				GetHeight() const;
	virtual int				GetNativeWidth() const;
	virtual int				GetNativeHeight() const;
	virtual int				GetVirtualWidth() const;
	virtual int				GetVirtualHeight() const;
	virtual float			GetPixelAspect() const;
	virtual float			GetPhysicalScreenWidthInCentimeters() const;
	virtual idRenderWorld* 	AllocRenderWorld();
	virtual void			FreeRenderWorld( idRenderWorld* rw );
	virtual void			BeginLevelLoad();
	virtual void			EndLevelLoad();
	virtual void			LoadLevelImages();
	virtual void			Preload( const idPreloadManifest& manifest, const char* mapName );
	virtual void			BeginAutomaticBackgroundSwaps( autoRenderIconType_t icon = AUTORENDER_DEFAULTICON );
	virtual void			EndAutomaticBackgroundSwaps();
	virtual bool			AreAutomaticBackgroundSwapsRunning( autoRenderIconType_t* usingAlternateIcon = NULL ) const;

	virtual idFont* 		RegisterFont( const char* fontName );
	virtual void			ResetFonts();
	virtual void			PrintMemInfo( MemInfo_t* mi );

	virtual void			SetColor( const idVec4& color );
	virtual uint32			GetColor();
	virtual void			SetGLState( const uint64 glState ) ;
	virtual void			DrawFilled( const idVec4& color, float x, float y, float w, float h );
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float z = 0.0f );
	virtual void			DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material, float z = 0.0f );
	virtual void			DrawStretchTri( const idVec2& p1, const idVec2& p2, const idVec2& p3, const idVec2& t1, const idVec2& t2, const idVec2& t3, const idMaterial* material );
	virtual idDrawVert* 	AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const stereoDepthType_t stereoType = STEREO_DEPTH_TYPE_NONE );
	virtual void			DrawSmallChar( int x, int y, int ch );
	virtual void			DrawSmallStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor );
	virtual void			DrawBigChar( int x, int y, int ch );
	virtual void			DrawBigStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor );

	virtual void			DrawCRTPostFX(); // RB

	virtual const emptyCommand_t* 	SwapCommandBuffers( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* mocMicroSec, uint64* gpuMicroSec, backEndCounters_t* bc, performanceCounters_t* pc );

	virtual void					SwapCommandBuffers_FinishRendering( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* mocMicroSec, uint64* gpuMicroSec, backEndCounters_t* bc, performanceCounters_t* pc );
	virtual const emptyCommand_t* 	SwapCommandBuffers_FinishCommandBuffers();

	virtual void			RenderCommandBuffers( const emptyCommand_t* commandBuffers );
	virtual void			TakeScreenshot( int width, int height, const char* fileName, renderView_t* ref );
	virtual bool			IsTakingScreenshot()
	{
		return takingScreenshot;
	}
	virtual byte*			CaptureRenderToBuffer( int width, int height, renderView_t* ref );
	virtual void			CropRenderSize( int width, int height );
	virtual void            CropRenderSize( int x, int y, int width, int height, bool topLeftAncor );
	virtual void			CaptureRenderToImage( const char* imageName, bool clearColorAfterCopy = false );
	virtual void			UnCrop();
	virtual bool			UploadImage( const char* imageName, const byte* data, int width, int height );

	void					PrintPerformanceCounters();

	void					SetInitialized()
	{
		bInitialized = true;
	}

	void					InvalidateSwapBuffers()
	{
		omitSwapBuffers = true;
	}

	void					SetReadyToPresent()
	{
		omitSwapBuffers = false;
	}

public:
	// internal functions
	idRenderSystemLocal();
	~idRenderSystemLocal();

	void					Clear();
	void					GetCroppedViewport( idScreenRect* viewport );
	void					PerformResolutionScaling( int& newWidth, int& newHeight );
	int						GetFrameCount() const
	{
		return frameCount;
	};

	void					OnFrame();

public:
	// renderer globals

	nvrhi::CommandListHandle commandList;

	bool					registered;		// cleared at shutdown, set at InitOpenGL

	bool					takingScreenshot;
	bool					takingEnvprobe;

	int						frameCount;		// incremented every frame
	int						viewCount;		// incremented every view (twice a scene if subviewed)
	// and every R_MarkFragments call

	float					frameShaderTime;	// shader time for all non-world 2D rendering

	idVec4					ambientLightVector;	// used for "ambient bump mapping"

	idList<idRenderWorldLocal*>worlds;

	idRenderWorldLocal* 	primaryWorld;
	renderView_t			primaryRenderView;
	viewDef_t* 				primaryView;
	// many console commands need to know which world they should operate on

	const idMaterial* 		whiteMaterial;
	const idMaterial* 		charSetMaterial;
	const idMaterial*		imgGuiMaterial; // RB
	const idMaterial* 		defaultPointLight;
	const idMaterial* 		defaultProjectedLight;
	const idMaterial* 		defaultMaterial;
	idImage* 				testImage;
	idCinematic* 			testVideo;
	int						testVideoStartTime;

	idImage* 				ambientCubeImage;	// hack for testing dependent ambient lighting

	viewDef_t* 				viewDef;

	performanceCounters_t	pc;					// performance counters

	viewEntity_t			identitySpace;		// can use if we don't know viewDef->worldSpace is valid

	idScreenRect			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// GUI drawing variables for surface creation
	int						guiRecursionLevel;		// to prevent infinite overruns
	uint32					currentColorNativeBytesOrder;
	uint64					currentGLState;
	class idGuiModel* 		guiModel;

	idList<idFont*, TAG_FONT>		fonts;

	unsigned short			gammaTable[256];	// brightness / gamma modify this

	idMat3					cubeAxis[6]; // RB

	srfTriangles_t* 		unitSquareTriangles;
	srfTriangles_t* 		zeroOneCubeTriangles;
	srfTriangles_t* 		zeroOneSphereTriangles;
	srfTriangles_t* 		testImageTriangles;

	// these are allocated at buffer swap time, but
	// the back end should only use the ones in the backEnd stucture,
	// which are copied over from the frame that was just swapped.
	drawSurf_t				unitSquareSurface_;
	drawSurf_t				zeroOneCubeSurface_;
	drawSurf_t				zeroOneSphereSurface_;
	drawSurf_t				testImageSurface_;

	idParallelJobList* 		frontEndJobList;

	// RB irradiance and GGX background jobs
	idParallelJobList* 					envprobeJobList;
	idList<calcEnvprobeParms_t*>		envprobeJobs;
	idList<calcLightGridPointParms_t*>	lightGridJobs;

#if defined(USE_INTRINSICS_SSE)

#if MOC_MULTITHREADED
	CullingThreadpool*		maskedOcclusionThreaded;
#endif
	MaskedOcclusionCulling*	maskedOcclusionCulling;
	idVec4					maskedUnitCubeVerts[8];
	idVec4					maskedZeroOneCubeVerts[8];
	unsigned int			maskedZeroOneCubeIndexes[36];
#endif

private:
	bool					bInitialized;
	bool					omitSwapBuffers;
};

extern idRenderSystemLocal	tr;
extern idRenderBackend		backEnd;
extern glconfig_t			glConfig;		// outside of TR since it shouldn't be cleared during ref re-init

//
// cvars
//
extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;

extern idCVar r_debugContext;				// enable various levels of context debug
extern idCVar r_useValidationLayers;
extern idCVar r_vidMode;					// video mode number
extern idCVar r_displayRefresh;				// optional display refresh rate option for vid mode
extern idCVar r_fullscreen;					// 0 = windowed, 1 = full screen
extern idCVar r_antiAliasing;				// anti aliasing mode, SMAA, TXAA, MSAA etc.

extern idCVar r_znear;						// near Z clip plane

extern idCVar r_swapInterval;				// changes wglSwapIntarval
extern idCVar r_offsetFactor;				// polygon offset parameter
extern idCVar r_offsetUnits;				// polygon offset parameter
extern idCVar r_singleTriangle;				// only draw a single triangle per primitive
extern idCVar r_clear;						// force screen clear every frame
extern idCVar r_subviewOnly;				// 1 = don't render main view, allowing subviews to be debugged
extern idCVar r_lightScale;					// all light intensities are multiplied by this, which is normally 3
extern idCVar r_flareSize;					// scale the flare deforms from the material def

extern idCVar r_gamma;						// changes gamma tables
extern idCVar r_brightness;					// changes gamma tables

extern idCVar r_checkBounds;				// compare all surface bounds with precalculated ones
extern idCVar r_maxAnisotropicFiltering;	// texture filtering parameter
extern idCVar r_useTrilinearFiltering;		// Extra quality filtering
extern idCVar r_lodBias;					// lod bias

extern idCVar r_useLightPortalFlow;			// 1 = do a more precise area reference determination
extern idCVar r_useShadowSurfaceScissor;	// 1 = scissor shadows by the scissor rect of the interaction surfaces
extern idCVar r_useConstantMaterials;		// 1 = use pre-calculated material registers if possible
extern idCVar r_useNodeCommonChildren;		// stop pushing reference bounds early when possible
extern idCVar r_useSilRemap;				// 1 = consider verts with the same XYZ, but different ST the same for shadows
extern idCVar r_useLightPortalCulling;		// 0 = none, 1 = box, 2 = exact clip of polyhedron faces, 3 MVP to plane culling
extern idCVar r_useLightAreaCulling;		// 0 = off, 1 = on
extern idCVar r_useLightScissors;			// 1 = use custom scissor rectangle for each light
extern idCVar r_useEntityPortalCulling;		// 0 = none, 1 = box
extern idCVar r_useCachedDynamicModels;		// 1 = cache snapshots of dynamic models
extern idCVar r_useScissor;					// 1 = scissor clip as portals and lights are processed
extern idCVar r_usePortals;					// 1 = use portals to perform area culling, otherwise draw everything
extern idCVar r_useStateCaching;			// avoid redundant state changes in GL_*() calls
extern idCVar r_useEntityCallbacks;			// if 0, issue the callback immediately at update time, rather than defering
extern idCVar r_lightAllBackFaces;			// light all the back faces, even when they would be shadowed
extern idCVar r_useLightDepthBounds;		// use depth bounds test on lights to reduce both shadow and interaction fill
extern idCVar r_useShadowDepthBounds;		// use depth bounds test on individual shadows to reduce shadow fill
// RB begin
extern idCVar r_useShadowAtlas;				// temporary for perf testing: pack shadow maps into big atlas
// RB end

extern idCVar r_skipStaticInteractions;		// skip interactions created at level load
extern idCVar r_skipDynamicInteractions;	// skip interactions created after level load
extern idCVar r_skipPostProcess;			// skip all post-process renderings
extern idCVar r_skipBloom;					// Admer: skip bloom
extern idCVar r_skipSuppress;				// ignore the per-view suppressions
extern idCVar r_skipInteractions;			// skip all light/surface interaction drawing
extern idCVar r_skipFrontEnd;				// bypasses all front end work, but 2D gui rendering still draws
extern idCVar r_skipBackEnd;				// don't draw anything
extern idCVar r_skipCopyTexture;			// do all rendering, but don't actually copyTexSubImage2D
extern idCVar r_skipRender;					// skip 3D rendering, but pass 2D
extern idCVar r_skipTranslucent;			// skip the translucent interaction rendering
extern idCVar r_skipAmbient;				// bypasses all non-interaction drawing
extern idCVar r_skipNewAmbient;				// bypasses all vertex/fragment program ambients
extern idCVar r_skipBlendLights;			// skip all blend lights
extern idCVar r_skipFogLights;				// skip all fog lights
extern idCVar r_skipSubviews;				// 1 = don't render any mirrors / cameras / etc
extern idCVar r_skipGuiShaders;				// 1 = don't render any gui elements on surfaces
extern idCVar r_skipParticles;				// 1 = don't render any particles
extern idCVar r_skipUpdates;				// 1 = don't accept any entity or light updates, making everything static
extern idCVar r_skipDeforms;				// leave all deform materials in their original state
extern idCVar r_skipDynamicTextures;		// don't dynamically create textures
extern idCVar r_skipBump;					// uses a flat surface instead of the bump map
extern idCVar r_skipSpecular;				// use black for specular
extern idCVar r_skipDiffuse;				// use black for diffuse
extern idCVar r_skipDecals;					// skip decal surfaces
extern idCVar r_skipOverlays;				// skip overlay surfaces
extern idCVar r_skipShadows;				// disable shadows

extern idCVar r_ignoreGLErrors;

extern idCVar r_screenFraction;				// for testing fill rate, the resolution of the entire screen can be changed
extern idCVar r_showUnsmoothedTangents;		// highlight geometry rendered with unsmoothed tangents
extern idCVar r_showSilhouette;				// highlight edges that are casting shadow planes
extern idCVar r_showVertexColor;			// draws all triangles with the solid vertex color
extern idCVar r_showUpdates;				// report entity and light updates and ref counts
extern idCVar r_showDynamic;				// report stats on dynamic surface generation
extern idCVar r_showIntensity;				// draw the screen colors based on intensity, red = 0, green = 128, blue = 255
extern idCVar r_showTrace;					// show the intersection of an eye trace with the world
extern idCVar r_showDepth;					// display the contents of the depth buffer and the depth range
extern idCVar r_showTris;					// enables wireframe rendering of the world
extern idCVar r_showSurfaceInfo;			// show surface material name under crosshair
extern idCVar r_showNormals;				// draws wireframe normals
extern idCVar r_showEdges;					// draw the sil edges
extern idCVar r_showViewEntitys;			// displays the bounding boxes of all view models and optionally the index
extern idCVar r_showTexturePolarity;		// shade triangles by texture area polarity
extern idCVar r_showTangentSpace;			// shade triangles by tangent space
extern idCVar r_showDominantTri;			// draw lines from vertexes to center of dominant triangles
extern idCVar r_showTextureVectors;			// draw each triangles texture (tangent) vectors
extern idCVar r_showLights;					// 1 = print light info, 2 = also draw volumes
extern idCVar r_showLightCount;				// colors surfaces based on light count
extern idCVar r_showShadows;				// visualize the stencil shadow volumes
extern idCVar r_showLightScissors;			// show light scissor rectangles
extern idCVar r_showMemory;					// print frame memory utilization
extern idCVar r_showCull;					// report sphere and box culling stats
extern idCVar r_showAddModel;				// report stats from tr_addModel
extern idCVar r_showSurfaces;				// report surface/light/shadow counts
extern idCVar r_showPrimitives;				// report vertex/index/draw counts
extern idCVar r_showPortals;				// draw portal outlines in color based on passed / not passed
extern idCVar r_showSkel;					// draw the skeleton when model animates
extern idCVar r_showOverDraw;				// show overdraw
// RB begin
extern idCVar r_showShadowMaps;
extern idCVar r_showShadowMapLODs;
// RB end
extern idCVar r_jointNameScale;				// size of joint names when r_showskel is set to 1
extern idCVar r_jointNameOffset;			// offset of joint names when r_showskel is set to 1

extern idCVar r_testGamma;					// draw a grid pattern to test gamma levels
extern idCVar r_testGammaBias;				// draw a grid pattern to test gamma levels

extern idCVar r_singleLight;				// suppress all but one light
extern idCVar r_singleEntity;				// suppress all but one entity
extern idCVar r_singleEnvprobe;				// suppress all but one envprobe
extern idCVar r_singleArea;					// only draw the portal area the view is actually in
extern idCVar r_singleSurface;				// suppress all but one surface on each entity

extern idCVar r_orderIndexes;				// perform index reorganization to optimize vertex use

extern idCVar r_debugLineDepthTest;			// perform depth test on debug lines
extern idCVar r_debugLineWidth;				// width of debug lines
extern idCVar r_debugArrowStep;				// step size of arrow cone line rotation in degrees
extern idCVar r_debugPolygonFilled;

extern idCVar r_materialOverride;			// override all materials

extern idCVar r_debugRenderToTexture;

extern idCVar stereoRender_enable;
extern idCVar stereoRender_deGhost;			// subtract from opposite eye to reduce ghosting

// RB begin
extern idCVar r_useGPUSkinning;

extern idCVar r_shadowMapAtlasSize;
extern idCVar r_shadowMapFrustumFOV;
extern idCVar r_shadowMapSingleSide;
extern idCVar r_shadowMapImageSize;
extern idCVar r_shadowMapJitterScale;
//extern idCVar r_shadowMapBiasScale;
extern idCVar r_shadowMapRandomizeJitter;
extern idCVar r_shadowMapSamples;
extern idCVar r_shadowMapSplits;
extern idCVar r_shadowMapSplitWeight;
extern idCVar r_shadowMapLodScale;
extern idCVar r_shadowMapLodBias;
extern idCVar r_shadowMapPolygonFactor;
extern idCVar r_dxShadowMapPolygonOffset;
extern idCVar r_vkShadowMapPolygonOffset;
extern idCVar r_shadowMapOccluderFacing;
extern idCVar r_shadowMapRegularDepthBiasScale;
extern idCVar r_shadowMapSunDepthBiasScale;

extern idCVar r_hdrAutoExposure;
extern idCVar r_hdrAdaptionRate;
extern idCVar r_hdrMinLuminance;
extern idCVar r_hdrMaxLuminance;
extern idCVar r_hdrKey;
extern idCVar r_hdrContrastDynamicThreshold;
extern idCVar r_hdrContrastStaticThreshold;
extern idCVar r_hdrContrastOffset;
extern idCVar r_hdrGlarePasses;
extern idCVar r_hdrDebug;

extern idCVar r_ldrContrastThreshold;
extern idCVar r_ldrContrastOffset;

extern idCVar r_forceAmbient;

extern idCVar r_useSSAO;
extern idCVar r_ssaoDebug;
extern idCVar r_ssaoFiltering;
extern idCVar r_useHierarchicalDepthBuffer;

extern idCVar r_pbrDebug;
extern idCVar r_showViewEnvprobes;
extern idCVar r_showLightGrid;				// show Quake 3 style light grid points

extern idCVar r_useLightGrid;

extern idCVar r_exposure;

extern idCVar r_useSSR;
extern idCVar r_ssrJitter;
extern idCVar r_ssrMaxDistance;
extern idCVar r_ssrMaxSteps;
extern idCVar r_ssrStride;
extern idCVar r_ssrZThickness;

extern idCVar r_useTemporalAA;
extern idCVar r_taaJitter;
extern idCVar r_taaEnableHistoryClamping;
extern idCVar r_taaClampingFactor;
extern idCVar r_taaNewFrameWeight;
extern idCVar r_taaMaxRadiance;
extern idCVar r_taaMotionVectors;

extern idCVar r_useFilmicPostFX;
extern idCVar r_useCRTPostFX;
extern idCVar r_crtCurvature;
extern idCVar r_crtVignette;

extern idCVar r_useMaskedOcclusionCulling;

enum RenderMode
{
	RENDERMODE_DOOM,
	RENDERMODE_2BIT,
	RENDERMODE_2BIT_HIGHRES,
	RENDERMODE_C64,
	RENDERMODE_C64_HIGHRES,
	RENDERMODE_CPC,
	RENDERMODE_CPC_HIGHRES,
	RENDERMODE_GENESIS,
	RENDERMODE_GENESIS_HIGHRES,
	RENDERMODE_PSX,
};

extern idCVar r_retroDitherScale;

extern idCVar r_renderMode;
extern idCVar image_pixelLook;

extern idCVar r_psxVertexJitter;
extern idCVar r_psxAffineTextures;
// RB end

/*
====================================================================

INITIALIZATION

====================================================================
*/

bool R_UsePixelatedLook();

bool R_UseTemporalAA();

bool R_UseHiZ();

uint R_GetMSAASamples();

void R_SetNewMode( const bool fullInit );

void R_SetColorMappings();

void R_ScreenShot_f( const idCmdArgs& args );

/*
====================================================================

NVRHI helpers

====================================================================
*/
bool R_ReadPixelsRGB8( nvrhi::IDevice* device, CommonRenderPasses* pPasses, nvrhi::ITexture* texture, nvrhi::ResourceStates textureState, const char* fullname );
bool R_ReadPixelsRGB16F( nvrhi::IDevice* device, CommonRenderPasses* pPasses, nvrhi::ITexture* texture, nvrhi::ResourceStates textureState, byte** pic, int picWidth, int picHeight );

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

struct vidMode_t
{
	int width;
	int height;
	int displayHz;

	// RB begin
	vidMode_t()
	{
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;
		displayHz = 60;
	}

	vidMode_t( int width, int height, int displayHz ) :
		width( width ), height( height ), displayHz( displayHz ) {}
	// RB end

	bool operator==( const vidMode_t& a )
	{
		return a.width == width && a.height == height && a.displayHz == displayHz;
	}
};

// the number of displays can be found by itterating this until it returns false
// displayNum is the 0 based value passed to EnumDisplayDevices(), you must add
// 1 to this to get an r_fullScreen value.
bool R_GetModeListForDisplay( const int displayNum, idList<vidMode_t>& modeList );

struct glimpParms_t
{
	int			x;				// ignored in fullscreen
	int			y;				// ignored in fullscreen
	int			width;
	int			height;
	int			fullScreen;		// 0 = windowed, otherwise 1 based monitor number to go full screen on
	// -1 = borderless window for spanning multiple displays
	bool		startMaximized = false;
	int			displayHz;
	int			multiSamples;
};

// Eric: If on Linux using Vulkan use the sdl_vkimp.cpp methods
// SRS - Generalized Vulkan SDL platform
#if defined( VULKAN_USE_PLATFORM_SDL )
#include <vector>

#define CLAMP(x, lo, hi)    ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
// Helper function for using SDL2 and Vulkan on Linux.
std::vector<const char*> get_required_extensions();

// DG: R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
// So add PreInit for platforms that need it, others can just stub it.
void		VKimp_PreInit();

// If the desired mode can't be set satisfactorily, false will be returned.
// If succesful, sets glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, and glConfig.pixelAspect
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.
bool		VKimp_Init( glimpParms_t parms );

// will set up gl up with the new parms
bool		VKimp_SetScreenParms( glimpParms_t parms );

// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.  SRS - Optionally shuts down SDL for quit.
void		VKimp_Shutdown( bool shutdownSDL );

// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision
void		VKimp_SetGamma( unsigned short red[256],
							unsigned short green[256],
							unsigned short blue[256] );
#else
// DG: R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
// So add PreInit for platforms that need it, others can just stub it.
void		GLimp_PreInit();

// If the desired mode can't be set satisfactorily, false will be returned.
// If succesful, sets glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, and glConfig.pixelAspect
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.
bool		GLimp_Init( glimpParms_t parms );

// will set up gl up with the new parms
bool		GLimp_SetScreenParms( glimpParms_t parms );

// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.
void		GLimp_Shutdown();

// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision
void		GLimp_SetGamma( unsigned short red[256],
							unsigned short green[256],
							unsigned short blue[256] );

#endif

/*
============================================================

RENDERWORLD_DEFS

============================================================
*/

void R_DeriveEntityData( idRenderEntityLocal* def );
void R_CreateEntityRefs( idRenderEntityLocal* def );
void R_FreeEntityDefDerivedData( idRenderEntityLocal* def, bool keepDecals, bool keepCachedDynamicModel );
void R_FreeEntityDefCachedDynamicModel( idRenderEntityLocal* def );
void R_FreeEntityDefDecals( idRenderEntityLocal* def );
void R_FreeEntityDefOverlay( idRenderEntityLocal* def );
void R_FreeEntityDefFadedDecals( idRenderEntityLocal* def, int time );

// RB: for dmap
void R_DeriveLightData( idRenderLightLocal* light );

// Called by the editor and dmap to operate on light volumes
void R_RenderLightFrustum( const renderLight_t& renderLight, idPlane lightFrustum[6] );

srfTriangles_t* R_PolytopeSurface( int numPlanes, const idPlane* planes, idWinding** windings );

void R_CreateEnvprobeRefs( RenderEnvprobeLocal* probe );
void R_FreeEnvprobeDefDerivedData( RenderEnvprobeLocal* probe );

// RB end
void R_CreateLightRefs( idRenderLightLocal* light );
void R_FreeLightDefDerivedData( idRenderLightLocal* light );

void R_FreeDerivedData();
void R_ReCreateWorldReferences();
void R_CheckForEntityDefsUsingModel( idRenderModel* model );
void R_ModulateLights_f( const idCmdArgs& args );

/*
============================================================

RENDERWORLD_PORTALS

============================================================
*/

viewEntity_t* R_SetEntityDefViewEntity( idRenderEntityLocal* def );
viewLight_t* R_SetLightDefViewLight( idRenderLightLocal* def );

/*
============================================================

RENDERWORLD_ENVPROBES

============================================================
*/

void R_SampleCubeMapHDR( const idVec3& dir, int size, byte* buffers[6], float result[3], float& u, float& v );
void R_SampleCubeMapHDR16F( const idVec3& dir, int size, halfFloat_t* buffers[6], float result[3], float& u, float& v );

idVec2 NormalizedOctCoord( int x, int y, const int probeSideLength );

/*
====================================================================

TR_FRONTEND_MAIN

====================================================================
*/

void R_InitFrameData();
void R_ShutdownFrameData();
void R_ToggleSmpFrame();
void* R_FrameAlloc( int bytes, frameAllocType_t type = FRAME_ALLOC_UNKNOWN );
void* R_ClearedFrameAlloc( int bytes, frameAllocType_t type = FRAME_ALLOC_UNKNOWN );

void* R_StaticAlloc( int bytes, const memTag_t tag = TAG_RENDER_STATIC );		// just malloc with error checking
void* R_ClearedStaticAlloc( int bytes );	// with memset
void R_StaticFree( void* data );

void R_RenderView( viewDef_t* parms );
void R_RenderPostProcess( viewDef_t* parms );

/*
============================================================

TR_FRONTEND_ADDLIGHTS

============================================================
*/

void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds );

ID_INLINE bool R_CullModelBoundsToLight( const idRenderLightLocal* light, const idBounds& localBounds, const idRenderMatrix& modelRenderMatrix )
{
	idRenderMatrix modelLightProject;
	idRenderMatrix::Multiply( light->baseLightProject, modelRenderMatrix, modelLightProject );
	return idRenderMatrix::CullBoundsToMVP( modelLightProject, localBounds, true );
}

void R_AddLights();
void R_OptimizeViewLightsList();

/*
============================================================

TR_FRONTEND_ADDMODELS

============================================================
*/

bool R_IssueEntityDefCallback( idRenderEntityLocal* def );
idRenderModel* R_EntityDefDynamicModel( idRenderEntityLocal* def );
void R_ClearEntityDefDynamicModel( idRenderEntityLocal* def );

void R_SetupDrawSurfShader( drawSurf_t* drawSurf, const idMaterial* shader, const renderEntity_t* renderEntity );
void R_SetupDrawSurfJoints( drawSurf_t* drawSurf, const srfTriangles_t* tri, const idMaterial* shader, nvrhi::ICommandList* commandList = nullptr );
void R_LinkDrawSurfToView( drawSurf_t* drawSurf, viewDef_t* viewDef );

void R_AddModels();

/*
============================================================

TR_FRONTEND_MASKED_OCCLUSION_CULLING

============================================================
*/

void R_FillMaskedOcclusionBufferWithModels( viewDef_t* viewDef );

/*
=============================================================

TR_FRONTEND_DEFORM

=============================================================
*/

drawSurf_t* R_DeformDrawSurf( drawSurf_t* drawSurf );

drawSurf_t* R_DeformDrawSurf( drawSurf_t* drawSurf, deform_t deformType );

/*
=============================================================

TR_FRONTEND_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxis( const srfTriangles_t* tri, idVec3& origin, idVec3 axis[3] );
void R_AddInGameGuis( const drawSurf_t* const drawSurfs[], const int numDrawSurfs );

/*
============================================================

TR_FRONTEND_SUBVIEW

============================================================
*/

bool R_PreciseCullSurface( const drawSurf_t* drawSurf, idBounds& ndcBounds );
bool R_GenerateSubViews( const drawSurf_t* const drawSurfs[], const int numDrawSurfs );

/*
============================================================

TR_TRISURF

============================================================
*/

srfTriangles_t* 	R_AllocStaticTriSurf();
void				R_AllocStaticTriSurfVerts( srfTriangles_t* tri, int numVerts );
void				R_AllocStaticTriSurfIndexes( srfTriangles_t* tri, int numIndexes );
void				R_AllocStaticTriSurfSilIndexes( srfTriangles_t* tri, int numIndexes );
void				R_AllocStaticTriSurfDominantTris( srfTriangles_t* tri, int numVerts );
void				R_AllocStaticTriSurfMirroredVerts( srfTriangles_t* tri, int numMirroredVerts );
void				R_AllocStaticTriSurfDupVerts( srfTriangles_t* tri, int numDupVerts );

// RB begin
void				R_AllocStaticTriSurfMocIndexes( srfTriangles_t* tri, int numIndexes );
void				R_AllocStaticTriSurfMocVerts( srfTriangles_t* tri, int numVerts );
// RB end

srfTriangles_t* 	R_CopyStaticTriSurf( const srfTriangles_t* tri );

void				R_ResizeStaticTriSurfVerts( srfTriangles_t* tri, int numVerts );
void				R_ResizeStaticTriSurfIndexes( srfTriangles_t* tri, int numIndexes );
void				R_ReferenceStaticTriSurfVerts( srfTriangles_t* tri, const srfTriangles_t* reference );
void				R_ReferenceStaticTriSurfIndexes( srfTriangles_t* tri, const srfTriangles_t* reference );

void				R_FreeStaticTriSurfSilIndexes( srfTriangles_t* tri );
void				R_FreeStaticTriSurf( srfTriangles_t* tri );
void				R_FreeStaticTriSurfVerts( srfTriangles_t* tri );
void				R_FreeStaticTriSurfVertexCaches( srfTriangles_t* tri );
int					R_TriSurfMemory( const srfTriangles_t* tri );

void				R_BoundTriSurf( srfTriangles_t* tri );
void				R_RemoveDuplicatedTriangles( srfTriangles_t* tri );
void				R_CreateSilIndexes( srfTriangles_t* tri );
void				R_CreateMaskedOcclusionCullingTris( srfTriangles_t* tri ); // RB
void				R_RemoveDegenerateTriangles( srfTriangles_t* tri );
void				R_RemoveUnusedVerts( srfTriangles_t* tri );
void				R_RangeCheckIndexes( const srfTriangles_t* tri );
void				R_CreateVertexNormals( srfTriangles_t* tri );		// also called by dmap
void				R_CleanupTriangles( srfTriangles_t* tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents, bool useMikktspace );
void				R_ReverseTriangles( srfTriangles_t* tri );

// Only deals with vertexes and indexes, not silhouettes, planes, etc.
// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
srfTriangles_t* 	R_MergeSurfaceList( const srfTriangles_t** surfaces, int numSurfaces );
srfTriangles_t* 	R_MergeTriangles( const srfTriangles_t* tri1, const srfTriangles_t* tri2 );

// if the deformed verts have significant enough texture coordinate changes to reverse the texture
// polarity of a triangle, the tangents will be incorrect
void				R_DeriveTangents( srfTriangles_t* tri );

// copy data from a front-end srfTriangles_t to a back-end drawSurf_t
void				R_InitDrawSurfFromTri( drawSurf_t& ds, srfTriangles_t& tri, nvrhi::ICommandList* commandList );

// For static surfaces, the indexes, ambient, and shadow buffers can be pre-created at load
// time, rather than being re-created each frame in the frame temporary buffers.
void				R_CreateStaticBuffersForTri( srfTriangles_t& tri, nvrhi::ICommandList* commandList );

// RB
idVec3				R_ClosestPointPointTriangle( const idVec3& point, const idVec3& vertex1, const idVec3& vertex2, const idVec3& vertex3 );
idVec3				R_ClosestPointOnLineSegment( const idVec3& point, const idVec3& lineStart, const idVec3& lineEnd, float& t );

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
struct deformInfo_t
{
	int					numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int					numOutputVerts;
	idDrawVert* 		verts;

	int					numIndexes;
	triIndex_t* 		indexes;

	triIndex_t* 		silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int					numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int* 				mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int					numDupVerts;			// number of duplicate vertexes
	int* 				dupVerts;				// pairs of the number of the first vertex and the number of the duplicate vertex

	vertCacheHandle_t	staticIndexCache;		// GL_INDEX_TYPE
	vertCacheHandle_t	staticAmbientCache;		// idDrawVert
};


// if outputVertexes is not NULL, it will point to a newly allocated set of verts that includes the mirrored ones
deformInfo_t* 		R_BuildDeformInfo( int numVerts, const idDrawVert* verts, int numIndexes, const int* indexes,
									   bool useUnsmoothedTangents );
void				R_CreateDeformStaticVertices( deformInfo_t* deform, nvrhi::ICommandList* commandList );
void				R_FreeDeformInfo( deformInfo_t* deformInfo );
int					R_DeformInfoMemoryUsed( deformInfo_t* deformInfo );

/*
=============================================================

TR_TRACE

=============================================================
*/

struct localTrace_t
{
	float		fraction;
	// only valid if fraction < 1.0
	idVec3		point;
	idVec3		normal;
	int			indexes[3];
};

localTrace_t R_LocalTrace( const idVec3& start, const idVec3& end, const float radius, const srfTriangles_t* tri );


/*
============================================================

TR_BACKEND_DRAW

============================================================
*/

void RB_SetMVP( const idRenderMatrix& mvp );

/*
=============================================================

TR_BACKEND_RENDERTOOLS

=============================================================
*/

float RB_DrawTextLength( const char* text, float scale, int len );
void RB_AddDebugText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest );
void RB_ClearDebugText( int time );
void RB_AddDebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifeTime, const bool depthTest );
void RB_ClearDebugLines( int time );
void RB_AddDebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest );
void RB_ClearDebugPolygons( int time );
void RB_DrawBounds( const idBounds& bounds );

void RB_ShutdownDebugTools();
void RB_SetVertexColorParms( stageVertexColor_t svc );




//=============================================

#include "ResolutionScale.h"
#include "RenderLog.h"
#include "GLMatrix.h"

#include "BufferObject.h"
#include "RenderProgs.h"
#include "RenderWorld_local.h"
#include "GuiModel.h"
#include "VertexCache.h"

#endif /* !__TR_LOCAL_H__ */
