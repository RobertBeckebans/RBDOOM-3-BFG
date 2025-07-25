/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2023 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#ifndef __RENDERER_H__
#define __RENDERER_H__

/*
===============================================================================

	idRenderSystem is responsible for managing the screen, which can have
	multiple idRenderWorld and 2D drawing done on it.

===============================================================================
*/


// In a head mounted display with separate displays for each eye,
// screen separation will be zero and world separation will be the eye distance.
struct stereoDistances_t
{
	// Offset to projection matrix, positive one eye, negative the other.
	// Total distance is twice this, so 0.05 would give a 10% of screen width
	// separation for objects at infinity.
	float	screenSeparation;

	// Game world units from one eye to the centerline.
	// Total distance is twice this.
	float	worldSeparation;

	// RB: offset behind both eyes considering the FOV
	// see https://github.com/RobertBeckebans/RBDOOM-3-BFG/issues/878
	float	combinedSeperation;
};

typedef enum
{
	AUTORENDER_DEFAULTICON = 0,
	AUTORENDER_HELLICON,
	AUTORENDER_DIALOGICON,
	AUTORENDER_MAX
} autoRenderIconType_t ;

enum stereoDepthType_t
{
	STEREO_DEPTH_TYPE_NONE,
	STEREO_DEPTH_TYPE_NEAR,
	STEREO_DEPTH_TYPE_MID,
	STEREO_DEPTH_TYPE_FAR
};

enum graphicsVendor_t
{
	VENDOR_NVIDIA,
	VENDOR_AMD,
	VENDOR_INTEL,
	VENDOR_APPLE,					// SRS - Added support for Apple GPUs
	VENDOR_OTHER
};

enum graphicsGpuType_t
{
	GPU_TYPE_DISCRETE,
	GPU_TYPE_OTHER
};

#define ID_MSAA 0

enum antiAliasingMode_t
{
	ANTI_ALIASING_NONE,
	ANTI_ALIASING_SMAA_1X,
	ANTI_ALIASING_TAA,

#if ID_MSAA
	ANTI_ALIASING_TAA_SMAA_1X,
	ANTI_ALIASING_MSAA_2X,
	ANTI_ALIASING_MSAA_4X,
#endif
};

// CPU counters and timers
struct performanceCounters_t
{
	int		c_box_cull_in;
	int		c_box_cull_out;
	int		c_createInteractions;	// number of calls to idInteraction::CreateInteraction
	int		c_createShadowVolumes;
	int		c_generateMd5;
	int		c_entityDefCallbacks;
	int		c_alloc;			// counts for R_StaticAllc/R_StaticFree
	int		c_free;
	int		c_visibleViewEntities;
	int		c_shadowViewEntities;
	int		c_viewLights;
	int		c_numViews;			// number of total views rendered
	int		c_deformedSurfaces;	// idMD5Mesh::GenerateSurface
	int		c_deformedVerts;	// idMD5Mesh::GenerateSurface
	int		c_deformedIndexes;	// idMD5Mesh::GenerateSurface
	int		c_tangentIndexes;	// R_DeriveTangents()
	int		c_entityUpdates;
	int		c_lightUpdates;
	int		c_envprobeUpdates;
	int		c_entityReferences;
	int		c_lightReferences;
	int		c_guiSurfs;

	int		c_mocVerts;
	int		c_mocIndexes;
	int		c_mocTests;
	int		c_mocCulledSurfaces;
	int		c_mocCulledLights;

	uint64	mocMicroSec;
	uint64	frontEndMicroSec;	// sum of time in all RE_RenderScene's in a frame
};

// CPU & GPU counters and timers
struct backEndCounters_t
{
	int		c_surfaces;
	int		c_shaders;

	int		c_drawElements;
	int		c_drawIndexes;

	int		c_shadowAtlasUsage; // allocated pixels in the atlas
	int		c_shadowViews;
	int		c_shadowElements;
	int		c_shadowIndexes;

	int		c_copyFrameBuffer;

	float	c_overDraw;

	uint64	cpuTotalMicroSec;		// total microseconds for backend run
	uint64	cpuShadowMicroSec;
	uint64	gpuBeginDrawingMicroSec;
	uint64	gpuDepthMicroSec;
	uint64	gpuGeometryMicroSec;
	uint64	gpuScreenSpaceAmbientOcclusionMicroSec;
	uint64	gpuScreenSpaceReflectionsMicroSec;
	uint64	gpuAmbientPassMicroSec;
	uint64	gpuShadowAtlasPassMicroSec;
	uint64	gpuInteractionsMicroSec;
	uint64	gpuShaderPassMicroSec;
	uint64	gpuFogAllLightsMicroSec;
	uint64	gpuBloomMicroSec;
	uint64	gpuShaderPassPostMicroSec;
	uint64	gpuMotionVectorsMicroSec;
	uint64	gpuTemporalAntiAliasingMicroSec;
	uint64	gpuToneMapPassMicroSec;
	uint64	gpuPostProcessingMicroSec;
	uint64	gpuDrawGuiMicroSec;
	uint64	gpuCrtPostProcessingMicroSec;
	uint64	gpuMicroSec;
};
// RB end

// Contains variables specific to the OpenGL configuration being run right now.
// These are constant once the OpenGL subsystem is initialized.
struct glconfig_t
{
	graphicsVendor_t	vendor;
	graphicsGpuType_t	gpuType;

//	int					maxTextureSize;			// TODO
//	int					maxTextureCoords;		// TODO
//	int					maxTextureImageUnits;	// TODO
	int					uniformBufferOffsetAlignment;

	bool				timerQueryAvailable;

	int					nativeScreenWidth; // this is the native screen width resolution of the renderer
	int					nativeScreenHeight; // this is the native screen height resolution of the renderer

	int					displayFrequency;

	int					isFullscreen;					// monitor number
	int					multisamples;

	// Screen separation for stereoscopic rendering is set based on this.
	// PC vid code sets this, converting from diagonals / inches / whatever as needed.
	// If the value can't be determined, set something reasonable, like 50cm.
	float				physicalScreenWidthInCentimeters;

	float				pixelAspect;
};



struct emptyCommand_t;

const int SMALLCHAR_WIDTH		= 8;
const int SMALLCHAR_HEIGHT		= 16;
const int BIGCHAR_WIDTH			= 16;
const int BIGCHAR_HEIGHT		= 16;

// all drawing is done to a 640 x 480 virtual screen size
// and will be automatically scaled to the real resolution
const int SCREEN_WIDTH			= 640;
const int SCREEN_HEIGHT			= 480;

extern idCVar r_useVirtualScreenResolution;

class idRenderWorld;


class idRenderSystem
{
public:

	virtual					~idRenderSystem() {}

	// set up cvars and basic data structures, but don't
	// init OpenGL, so it can also be used for dedicated servers
	virtual void			Init() = 0;

	// only called before quitting
	virtual void			Shutdown() = 0;

	virtual bool			IsInitialized() const = 0;

	virtual void			ResetGuiModels() = 0;

	virtual void			InitBackend() = 0;

	virtual void			ShutdownOpenGL() = 0;

	virtual bool			IsOpenGLRunning() const = 0;

	virtual bool			IsFullScreen() const = 0;
	virtual int				GetWidth() const = 0;
	virtual int				GetHeight() const = 0;
	virtual int				GetNativeWidth() const = 0;
	virtual int				GetNativeHeight() const = 0;
	virtual int				GetVirtualWidth() const = 0;
	virtual int				GetVirtualHeight() const = 0;

	// return w/h of a single pixel. This will be 1.0 for normal cases.
	// A side-by-side stereo 3D frame will have a pixel aspect of 0.5.
	// A top-and-bottom stereo 3D frame will have a pixel aspect of 2.0
	virtual float			GetPixelAspect() const = 0;

	// This is used to calculate stereoscopic screen offset for a given interocular distance.
	virtual float			GetPhysicalScreenWidthInCentimeters() const = 0;

	// allocate a renderWorld to be used for drawing
	virtual idRenderWorld* 	AllocRenderWorld() = 0;
	virtual	void			FreeRenderWorld( idRenderWorld* rw ) = 0;

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	virtual void			BeginLevelLoad() = 0;
	virtual void			EndLevelLoad() = 0;
	virtual void			Preload( const idPreloadManifest& manifest, const char* mapName ) = 0;
	virtual void			LoadLevelImages() = 0;

	virtual void			BeginAutomaticBackgroundSwaps( autoRenderIconType_t icon = AUTORENDER_DEFAULTICON ) = 0;
	virtual void			EndAutomaticBackgroundSwaps() = 0;
	virtual bool			AreAutomaticBackgroundSwapsRunning( autoRenderIconType_t* icon = NULL ) const = 0;

	// font support
	virtual class idFont* 	RegisterFont( const char* fontName ) = 0;
	virtual void			ResetFonts() = 0;

	virtual void			SetColor( const idVec4& rgba ) = 0;
	virtual void			SetColor4( float r, float g, float b, float a )
	{
		SetColor( idVec4( r, g, b, a ) );
	}

	virtual uint32			GetColor() = 0;

	virtual void			SetGLState( const uint64 glState ) = 0;

	virtual void			DrawFilled( const idVec4& color, float x, float y, float w, float h ) = 0;
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float z = 0.0f ) = 0;
	void			DrawStretchPic( const idVec4& rect, const idVec4& st, const idMaterial* material, float z = 0.0f )
	{
		DrawStretchPic( rect.x, rect.y, rect.z, rect.w, st.x, st.y, st.z, st.w, material, z );
	}
	virtual void			DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material, float z = 0.0f ) = 0;
	virtual void			DrawStretchTri( const idVec2& p1, const idVec2& p2, const idVec2& p3, const idVec2& t1, const idVec2& t2, const idVec2& t3, const idMaterial* material ) = 0;
	virtual idDrawVert* 	AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const stereoDepthType_t stereoType = STEREO_DEPTH_TYPE_NONE ) = 0;

	virtual void			PrintMemInfo( MemInfo_t* mi ) = 0;

	virtual void			DrawSmallChar( int x, int y, int ch ) = 0;
	virtual void			DrawSmallStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor ) = 0;
	virtual void			DrawBigChar( int x, int y, int ch ) = 0;
	virtual void			DrawBigStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor ) = 0;

	virtual void			DrawCRTPostFX() = 0; // RB

	// Performs final closeout of any gui models being defined.
	//
	// Waits for the previous GPU rendering to complete and vsync.
	//
	// Returns the head of the linked command list that was just closed off.
	//
	// Returns timing information from the previous frame.
	//
	// After this is called, new command buffers can be built up in parallel
	// with the rendering of the closed off command buffers by RenderCommandBuffers()
	virtual const emptyCommand_t* 	SwapCommandBuffers( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec, backEndCounters_t* bc, performanceCounters_t* pc ) = 0;

	// SwapCommandBuffers operation can be split in two parts for non-smp rendering
	// where the GPU is idled intentionally for minimal latency.
	virtual void			SwapCommandBuffers_FinishRendering( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec, backEndCounters_t* bc, performanceCounters_t* pc ) = 0;
	virtual const emptyCommand_t* 	SwapCommandBuffers_FinishCommandBuffers() = 0;

	// issues GPU commands to render a built up list of command buffers returned
	// by SwapCommandBuffers().  No references should be made to the current frameData,
	// so new scenes and GUIs can be built up in parallel with the rendering.
	virtual void			RenderCommandBuffers( const emptyCommand_t* commandBuffers ) = 0;

	// aviDemo uses this.
	// Will automatically tile render large screen shots if necessary
	// If ref == NULL, common->UpdateScreen will be used
	// This will perform swapbuffers, so it is NOT an approppriate way to
	// generate image files that happen during gameplay, as for savegame
	// markers.  Use WriteRender() instead.
	virtual void			TakeScreenshot( int width, int height, const char* fileName, struct renderView_s* ref ) = 0;

	// RB
	virtual bool			IsTakingScreenshot() = 0;
	virtual byte*			CaptureRenderToBuffer( int width, int height, renderView_t* ref ) = 0;

	// the render output can be cropped down to a subset of the real screen, as
	// for save-game reviews and split-screen multiplayer.  Users of the renderer
	// will not know the actual pixel size of the area they are rendering to

	// the x,y,width,height values are in virtual SCREEN_WIDTH / SCREEN_HEIGHT coordinates

	// to render to a texture, first set the crop size with makePowerOfTwo = true,
	// then perform all desired rendering, then capture to an image
	// if the specified physical dimensions are larger than the current cropped region, they will be cut down to fit
	virtual void			CropRenderSize( int width, int height ) = 0;
	virtual void            CropRenderSize( int x, int y, int width, int height, bool topLeftAncor ) = 0;
	virtual void			CaptureRenderToImage( const char* imageName, bool clearColorAfterCopy = false ) = 0;
	virtual void			UnCrop() = 0;

	// the image has to be already loaded ( most straightforward way would be through a FindMaterial )
	// texture filter / mipmapping / repeat won't be modified by the upload
	// returns false if the image wasn't found
	virtual bool			UploadImage( const char* imageName, const byte* data, int width, int height ) = 0;

	// consoles switch stereo 3D eye views each 60 hz frame
	virtual int				GetFrameCount() const = 0;

	virtual void			OnFrame() = 0;
};

extern idRenderSystem* 			renderSystem;

//
// functions mainly intended for editor and dmap integration
//

// for use by dmap to do the carving-on-light-boundaries and for the editor for display
void R_LightProjectionMatrix( const idVec3& origin, const idPlane& rearPlane, idVec4 mat[4] );

// used by the view shot taker
void R_ScreenshotFilename( int& lastNumber, const char* base, idStr& fileName );

#endif /* !__RENDERER_H__ */
