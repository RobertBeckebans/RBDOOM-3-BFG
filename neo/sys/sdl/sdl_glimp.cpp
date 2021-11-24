/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "../../idlib/precompiled.h"
#include <GL/glew.h>

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL.h>

#include "renderer/RenderCommon.h"
#include "sdl_local.h"

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );

// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
idCVar r_waylandcompat( "r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

// RB: only relevant if using SDL 2.0
#if defined(__APPLE__)
	// only core profile is supported on OS X
	idCVar r_useOpenGL32( "r_useOpenGL32", "2", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#elif defined(__linux__)
	// Linux open source drivers suck
	idCVar r_useOpenGL32( "r_useOpenGL32", "0", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#else
	idCVar r_useOpenGL32( "r_useOpenGL32", "1", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#endif
// RB end

static bool grabbed = false;

#if SDL_VERSION_ATLEAST(2, 0, 0)
	static SDL_Window* window = NULL;
	static SDL_GLContext context = NULL;
#else
	static SDL_Surface* window = NULL;
	#define SDL_WINDOW_OPENGL SDL_OPENGL
	#define SDL_WINDOW_FULLSCREEN SDL_FULLSCREEN
	#define SDL_WINDOW_RESIZABLE SDL_RESIZABLE
#endif

/*
===================
GLimp_PreInit

 R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
 So do that in GLimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void GLimp_PreInit() // DG: added this function for SDL compatibility
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		if( SDL_Init( SDL_INIT_VIDEO ) )
		{
			common->Error( "Error while initializing SDL: %s", SDL_GetError() );
		}
	}
}

/*
===================
 Helper functions for GLimp_Init() and GLimp_SetScreenParms()
===================
*/

#if SDL_VERSION_ATLEAST(2, 0, 0)
static int GetDisplayIndex( glimpParms_t parms )
{
    int displayIdx;
    if( parms.fullScreen > 0 )
    {
        displayIdx = parms.fullScreen - 1; // first display for SDL is 0, in parms it's 1
    }
    else // 0, -1, -2 == use current display
    {
        // SRS - Find display containing the center of the windowed or borderless mode window
        int windowPosX = r_windowX.GetInteger() + r_windowWidth.GetInteger() / 2;
        int windowPosY = r_windowY.GetInteger() + r_windowHeight.GetInteger() / 2;
        
        displayIdx = 0;
        for( int i = 0; i < SDL_GetNumVideoDisplays(); i++ )
        {
            SDL_Rect rect;
            SDL_GetDisplayBounds( i, &rect );
            if( windowPosX >= rect.x && windowPosX < ( rect.x + rect.w ) && windowPosY >= rect.y && windowPosY < ( rect.y + rect.h ) )
            {
                displayIdx = i;
                break;
            }
        }
    }
    return displayIdx;
}

static int GetDisplayFrequency( glimpParms_t parms )
{
    int displayIdx = GetDisplayIndex( parms );
    if( displayIdx > SDL_GetNumVideoDisplays() )
    {
        return parms.displayHz;
    }

    SDL_DisplayMode m = {0};
    if( SDL_GetCurrentDisplayMode( displayIdx, &m ) < 0 )
    {
        common->Warning( "Couldn't get display refresh rate, reason: %s", SDL_GetError() );
        common->Printf( "GetDisplayFrequency() displayIdx %i returning parms.displayHz = %i\n", displayIdx, parms.displayHz );
        return parms.displayHz;
    }

    common->Printf( "GetDisplayFrequency() displayIdx %i returning mode.refresh_rate = %i\n", displayIdx, m.refresh_rate );
    return m.refresh_rate;
}
#endif

/*
===================
GLimp_Init
===================
*/
bool GLimp_Init( glimpParms_t parms )
{
#ifdef USE_VULKAN
	return true;
#endif

	common->Printf( "Initializing OpenGL subsystem\n" );

	GLimp_PreInit(); // DG: make sure SDL is initialized

	// DG: make window resizable
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	// DG end

	if( parms.fullScreen > 0 )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}
#if SDL_VERSION_ATLEAST(2, 0, 0)
    // SRS - Current monitor fullscreen mode
    else if( parms.fullScreen == -2 )
    {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    // SRS - Support basic borderless mode that can span multiple displays using explicit position / size for window
    else if( parms.fullScreen == -1 )
    {
        flags |= SDL_WINDOW_BORDERLESS;
    }
#endif

	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;

	for( int i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch( i / 4 )
			{
				case 2 :
					if( colorbits == 24 )
					{
						colorbits = 16;
					}
					break;
				case 1 :
					if( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if( depthbits == 16 )
					{
						depthbits = 8;
					}
				case 3 :
					if( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}

		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}

		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		int channelcolorbits = 4;
		if( tcolorbits == 24 )
		{
			channelcolorbits = 8;
		}

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

		if( r_waylandcompat.GetBool() )
		{
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
		}
		else
		{
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, channelcolorbits );
		}

		SDL_GL_SetAttribute( SDL_GL_STEREO, parms.stereo ? 1 : 0 );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

#if SDL_VERSION_ATLEAST(2, 0, 0)

		// RB begin
		if( r_useOpenGL32.GetInteger() > 0 )
		{
			glConfig.driverType = GLDRV_OPENGL32_COMPATIBILITY_PROFILE;

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );

			if( r_debugContext.GetBool() )
			{
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
			}
		}

		if( r_useOpenGL32.GetInteger() > 1 )
		{
			glConfig.driverType = GLDRV_OPENGL32_CORE_PROFILE;

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
		}
		// RB end

        // SRS - This sets the default centered launch position and size for windowed mode parms.fullScreen == 0, other modes can override below
        int windowPosX = SDL_WINDOWPOS_UNDEFINED;
        int windowPosY = SDL_WINDOWPOS_UNDEFINED;
        int windowWidth = parms.width;
        int windowHeight = parms.height;

        // DG: set display num and window position for fullscreen
		if( parms.fullScreen > 0 || parms.fullScreen == -2 )
		{
			if( parms.fullScreen > SDL_GetNumVideoDisplays() )
			{
				common->Warning( "Couldn't set display to num %i because we only have %i displays",
								 parms.fullScreen, SDL_GetNumVideoDisplays() );
			}
			else
			{
                int displayIdx = GetDisplayIndex( parms );
                windowPosX = windowPosY = SDL_WINDOWPOS_UNDEFINED_DISPLAY( displayIdx );
                
                // SRS - Handle current monitor mode parms.fullScreen == -2 (from r_fullscreen -2) meaning "do fullscreen using the current monitor"
                // The "current monitor" is defined as the one hosting the center of the windowed or borderless window, as returned by GetDisplayIndex()
                if( parms.fullScreen == -2 )
                {
                    SDL_DisplayMode m = {0};
                    SDL_GetDesktopDisplayMode( displayIdx, &m );
                    windowWidth = m.w;
                    windowHeight = m.h;
                }
			}
		}
        // SRS - Support borderless mode parms.fullScreen == -1 that can span multiple displays using explicit position / size settings for the window
        else if( parms.fullScreen == -1 )
        {
            windowPosX = parms.x;
            windowPosY = parms.y;
        }
        common->Printf( "GLimp_Init() windowPosX = %i, windowPosY = %i, width = %i, height = %i\n", windowPosX, windowPosY, parms.width, parms.height );


		window = SDL_CreateWindow( GAME_NAME,
								   windowPosX,
								   windowPosY,
                                   windowWidth, windowHeight, flags );
		// DG end

		context = SDL_GL_CreateContext( window );

		if( !window )
		{
			common->DPrintf( "Couldn't set GL mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

        // SRS - If fullscreen is active make sure display refresh rate is set properly
        if( parms.fullScreen > 0 && parms.displayHz != 0 )
        {
            SDL_DisplayMode m = {0};
            SDL_GetWindowDisplayMode( window, &m );

            m.refresh_rate = parms.displayHz;
            if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
            {
                common->Warning( "Couldn't set display refresh rate to %i Hz", parms.displayHz );
            }
        }

		if( SDL_GL_SetSwapInterval( r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "SDL_GL_SWAP_CONTROL not supported" );
		}

		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
        common->Printf( "GLimp_Init() setting glConfig.nativeScreenWidth = %i, glConfig.nativeScreenHeight = %i\n", glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
		// RB end

        // SRS - Detect and save actual fullscreen state supporting all modes (-2, -1, 0, 1, ...)
        glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN || parms.fullScreen == -1 ? parms.fullScreen : 0 );
        common->Printf( "GLimp_Init() parms.fullScreen = %i, glConfig.isFullscreen = %i\n", parms.fullScreen, glConfig.isFullscreen );
        // SRS - Initialize state variable capturing transition from fullscreen to windowed mode
        glConfig.exitingFullscreen = false;
        // SRS - Get the actual refresh rate for windowed and fullscreen modes
        glConfig.displayFrequency = GetDisplayFrequency( parms );
#else
		glConfig.driverType = GLDRV_OPENGL3X;

		SDL_WM_SetCaption( GAME_NAME, GAME_NAME );

		if( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval.GetInteger() ) < 0 )
		{
			common->Warning( "SDL_GL_SWAP_CONTROL not supported" );
		}

		window = SDL_SetVideoMode( parms.width, parms.height, colorbits, flags );
		if( !window )
		{
			common->DPrintf( "Couldn't set GL mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

		glConfig.nativeScreenWidth = window->w;
		glConfig.nativeScreenHeight = window->h;

		glConfig.isFullscreen = ( window->flags & SDL_FULLSCREEN ) == SDL_FULLSCREEN;
        glConfig.displayFrequency = parms.displayHz;
#endif

		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		// RB begin
		glConfig.isStereoPixelFormat = parms.stereo;
		glConfig.multisamples = parms.multiSamples;

		glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
		// should side-by-side stereo modes be consider aspect 0.5?

		// RB end

		break;
	}

	if( !window )
	{
		common->Printf( "No usable GL mode found: %s", SDL_GetError() );
		return false;
	}

#ifdef __APPLE__
	glewExperimental = GL_TRUE;
#endif

	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

#if defined(__APPLE__) && SDL_VERSION_ATLEAST(2, 0, 2)
	// SRS - On OSX enable SDL2 relative mouse mode warping to capture mouse properly if outside of window
	SDL_SetHintWithPriority( SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE );
#endif

	// DG: disable cursor, we have two cursors in menu (because mouse isn't grabbed in menu)
	SDL_ShowCursor( SDL_DISABLE );
	// DG end

	return true;
}

/*
===================
 Helper functions for GLimp_SetScreenParms()
===================
*/

#if SDL_VERSION_ATLEAST(2, 0, 0)
// SDL1 doesn't support multiple displays, so the source is much shorter and doesn't need separate functions
// makes sure the window will be full-screened on the right display and returns the SDL display index
static int ScreenParmsHandleDisplayIndex( glimpParms_t parms )
{
    int displayIdx = GetDisplayIndex( parms );
    if( displayIdx > SDL_GetNumVideoDisplays() )
    {
        common->Warning( "Can't set fullscreen mode to display number %i, because SDL2 only knows about %i displays!",
                         parms.fullScreen, SDL_GetNumVideoDisplays() );
        return -1;
    }

    common->Printf( "...HandleDisplayIndex() parms.fullScreen = %i, glConfig.isFullscreen = %i, displayIdx = %i\n", parms.fullScreen, glConfig.isFullscreen, displayIdx );
#if defined(__APPLE__)
    // SRS - For reliable operation on OSX, disable fullscreen before monitor or mode switching
    if( parms.fullScreen != glConfig.isFullscreen || glConfig.isFullscreen )
    {
#else
	if( parms.fullScreen != glConfig.isFullscreen )
	{
#endif
		// we have to switch to another display
		if( glConfig.isFullscreen )
		{
			// if we're already in fullscreen mode but want to switch to another monitor
			// we have to go to windowed mode first to move the window.. SDL-oddity.
			SDL_SetWindowFullscreen( window, SDL_FALSE );
            common->Printf("SetScreenParmsFullscreen() setting to windowed mode\n");
		}
        // select display ; SDL_WINDOWPOS_UNDEFINED_DISPLAY() doesn't work.
        int x = SDL_WINDOWPOS_CENTERED_DISPLAY( displayIdx );
        // move window to the center of selected display
        SDL_SetWindowPosition( window, x, x );
    }
    return displayIdx;
}

static bool SetScreenParmsFullscreen( glimpParms_t parms )
{
	SDL_DisplayMode m = {0};
    int displayIdx = ScreenParmsHandleDisplayIndex( parms );
    if( displayIdx < 0 )
	{
		return false;
	}

	// change settings in that display mode according to parms
    if( parms.fullScreen > 0 )
    {
        // get current mode of display the window should be full-screened on
        SDL_GetCurrentDisplayMode( displayIdx, &m );
        common->Printf( "SDL_GetCurrentDisplayMode() w = %i, h = %i, hz = %i\n", m.w, m.h, m.refresh_rate );

        // FIXME: check if refreshrate, width and height are supported?
        // SRS - Reenabled parms.displayHz - should defined by R_GetModeListForDisplay() unless set via r_displayRefresh (only for r_vidMode < 0)
        m.refresh_rate = parms.displayHz;
        m.w = parms.width;
        m.h = parms.height;
    }
    else // -2 == use current display
    {
        // get desktop mode of display the window should be full-screened on
        SDL_GetDesktopDisplayMode( displayIdx, &m );
        common->Printf( "SDL_GetDesktopDisplayMode() w = %i, h = %i, hz = %i\n", m.w, m.h, m.refresh_rate );
    }

	// set that displaymode
    common->Printf( "SDL_SetWindowDisplayMode() w = %i, h = %i, hz = %i\n", m.w, m.h, m.refresh_rate );
	if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
	{
		common->Warning( "Couldn't set window mode for fullscreen, reason: %s", SDL_GetError() );
		return false;
	}

	// if we're currently not in fullscreen mode, we need to switch to fullscreen
	if( !( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) )
	{
		if( SDL_SetWindowFullscreen( window, SDL_TRUE ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s!", SDL_GetError() );
			return false;
		}
        common->Printf( "SetScreenParmsFullscreen() setting to fullscreen mode\n" );
	}
	return true;
}

static bool SetScreenParmsWindowed( glimpParms_t parms )
{
	// SRS - if we're currently in fullscreen mode, we first need to disable that before setting window border, size, and position
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
        // SRS - Inform the SDL2 event handler to ignore the next window moved event caused by exiting fullscreen mode
        glConfig.exitingFullscreen = true;
        common->Printf("SetScreenParmsWindowed() setting to windowed mode\n");
		if( SDL_SetWindowFullscreen( window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s!", SDL_GetError() );
			return false;
		}
    }

    // SRS - Set window border based on fullscreen mode (off when in borderless mode -1, on otherwise)
    SDL_SetWindowBordered( window, ( parms.fullScreen == -1 ? SDL_FALSE : SDL_TRUE ) );
    SDL_SetWindowSize( window, parms.width, parms.height );
    SDL_SetWindowPosition( window, parms.x, parms.y );
    common->Printf( "...ParmsWindowed() x = %i, y = %i, width = %i, height = %i, fullScreen = %i\n", parms.x, parms.y, parms.width, parms.height, parms.fullScreen );

	return true;
}
#endif // SDL_VERSION_ATLEAST(2, 0, 0)

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
#ifdef USE_VULKAN
	return true;
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( parms.fullScreen > 0 || parms.fullScreen == -2 )
	{
		if( !SetScreenParmsFullscreen( parms ) )
		{
			return false;
		}
	}
	else // windowed mode 0 or borderless mode -1
	{
		if( !SetScreenParmsWindowed( parms ) )
		{
			return false;
		}
	}
    
    // SRS - Must query actual window size vs using parms.width or parms.height which may be incorrect for fullscreen mode -2
    SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
    // SRS - move mouse to upper left quadrant of changed window, helpful for monitor and mode changes
    SDL_WarpMouseInWindow( window, glConfig.nativeScreenWidth / 4, glConfig.nativeScreenHeight / 4 );

    // SRS - Detect and save actual fullscreen state supporting all modes (-2, -1, 0, 1, ...)
    glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN || parms.fullScreen == -1 ? parms.fullScreen : 0 );
    // SRS - Get the actual refresh rate for windowed and fullscreen modes
    glConfig.displayFrequency = GetDisplayFrequency( parms );
#else // SDL 1.2 - so much shorter, but doesn't handle multiple displays
	SDL_Surface* s = SDL_GetVideoSurface();
	if( s == NULL )
	{
		common->Warning( "GLimp_SetScreenParms: Couldn't get video information, reason: %s", SDL_GetError() );
		return false;
	}


	int bitsperpixel = 24;
	if( s->format )
	{
		bitsperpixel = s->format->BitsPerPixel;
	}

	Uint32 flags = s->flags;

	if( parms.fullScreen )
	{
		flags |= SDL_FULLSCREEN;
	}
	else
	{
		flags &= ~SDL_FULLSCREEN;
	}

	s = SDL_SetVideoMode( parms.width, parms.height, bitsperpixel, flags );
	if( s == NULL )
	{
		common->Warning( "GLimp_SetScreenParms: Couldn't set video information, reason: %s", SDL_GetError() );
		return false;
	}

    glConfig.nativeScreenWidth = parms.width;
    glConfig.nativeScreenHeight = parms.height;

    glConfig.isFullscreen = ( window->flags & SDL_FULLSCREEN ) == SDL_FULLSCREEN;
    glConfig.displayFrequency = parms.displayHz;
#endif // SDL_VERSION_ATLEAST(2, 0, 0)
    common->Printf( "...SetScreenParms() nativeScreenWidth = %i, nativeScreenHeight = %i\n\n", glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );

	// Note: the following stuff would also work with SDL1.2
	SDL_GL_SetAttribute( SDL_GL_STEREO, parms.stereo ? 1 : 0 );

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.multisamples = parms.multiSamples;

	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown()
{
#ifdef USE_VULKAN
	common->Printf( "Shutting down Vulkan subsystem\n" );
	return;
#else
	common->Printf( "Shutting down OpenGL subsystem\n" );
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( context )
	{
		SDL_GL_DeleteContext( context );
		context = NULL;
	}

	if( window )
	{
		SDL_DestroyWindow( window );
		window = NULL;
	}
#endif
}

/*
===================
GLimp_SwapBuffers
===================
*/
#ifndef USE_VULKAN
void GLimp_SwapBuffers()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GL_SwapWindow( window );
#else
	SDL_GL_SwapBuffers();
#endif
}
#endif

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
#ifndef USE_VULKAN
	if( !window )
	{
		common->Warning( "GLimp_SetGamma called without window" );
		return;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( SDL_SetWindowGammaRamp( window, red, green, blue ) )
#else
	if( SDL_SetGammaRamp( red, green, blue ) )
#endif
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
#endif
}

/*
===================
GLimp_ExtensionPointer
===================
*/
/*
GLExtension_t GLimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
*/

void GLimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		grabbed = grab;
	}

	if( in_nograb.GetBool() )
	{
		grab = false;
	}

	if( !window )
	{
		common->Warning( "GLimp_GrabInput called without window" );
		return;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled

	// DG: check for GRAB_ENABLE instead of GRAB_HIDECURSOR because we always wanna hide it
	SDL_SetRelativeMouseMode( flags & GRAB_ENABLE ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( window, grab ? SDL_TRUE : SDL_FALSE );
#else
	// DG end
	SDL_WM_GrabInput( grab ? SDL_GRAB_ON : SDL_GRAB_OFF );
#endif
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->DPrintf( "TODO: DumpAllDisplayDevices\n" );
}

class idSort_VidMode : public idSort_Quick< vidMode_t, idSort_VidMode >
{
public:
	int Compare( const vidMode_t& a, const vidMode_t& b ) const
	{
		int wd = a.width - b.width;
		int hd = a.height - b.height;
		int fd = a.displayHz - b.displayHz;
		return ( hd != 0 ) ? hd : ( wd != 0 ) ? wd : fd;
	}
};

// RB: resolutions supported by XreaL
static void FillStaticVidModes( idList<vidMode_t>& modeList )
{
	modeList.AddUnique( vidMode_t( 320,   240, 60 ) );
	modeList.AddUnique( vidMode_t( 400,   300, 60 ) );
	modeList.AddUnique( vidMode_t( 512,   384, 60 ) );
	modeList.AddUnique( vidMode_t( 640,   480, 60 ) );
	modeList.AddUnique( vidMode_t( 800,   600, 60 ) );
	modeList.AddUnique( vidMode_t( 960,   720, 60 ) );
	modeList.AddUnique( vidMode_t( 1024,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1152,  864, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  720, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  800, 60 ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, 60 ) );
	modeList.AddUnique( vidMode_t( 1360,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1440,  900, 60 ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, 60 ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 2048, 1536, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, 60 ) );

	modeList.SortWithTemplate( idSort_VidMode() );
}

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	assert( requestedDisplayNum >= 0 );

	modeList.Clear();
#if SDL_VERSION_ATLEAST(2, 0, 0)
	// DG: SDL2 implementation
	if( requestedDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		// requested invalid displaynum
		return false;
	}

	int numModes = SDL_GetNumDisplayModes( requestedDisplayNum );
	if( numModes > 0 )
	{
		for( int i = 0; i < numModes; i++ )
		{
			SDL_DisplayMode m;
			int ret = SDL_GetDisplayMode( requestedDisplayNum, i, &m );
			if( ret != 0 )
			{
				common->Warning( "Can't get video mode no %i, because of %s\n", i, SDL_GetError() );
				continue;
			}
            
            // SRS - Use same mode restrictions as on Windows: 32 or 24 bpp and height >= 720 pixels
            if( m.format != SDL_PIXELFORMAT_ARGB8888 && m.format != SDL_PIXELFORMAT_RGBA8888 && m.format != SDL_PIXELFORMAT_ABGR8888 && m.format != SDL_PIXELFORMAT_BGRA8888 && m.format != SDL_PIXELFORMAT_RGB888 && m.format != SDL_PIXELFORMAT_BGR888 )
            {
                continue;
            }

            if( m.h < 720 )
            {
                continue;
            }
            // SRS end

			vidMode_t mode;
			mode.width = m.w;
			mode.height = m.h;
			mode.displayHz = m.refresh_rate ? m.refresh_rate : 60; // default to 60 if unknown (0)
			modeList.AddUnique( mode );
		}

		if( modeList.Num() < 1 )
		{
			common->Warning( "Couldn't get a single video mode for display %i, using default ones..!\n", requestedDisplayNum );
			FillStaticVidModes( modeList );
		}

		// sort with lowest resolution first
		modeList.SortWithTemplate( idSort_VidMode() );
	}
	else
	{
		common->Warning( "Can't get Video Info, using default modes...\n" );
		if( numModes < 0 )
		{
			common->Warning( "Reason was: %s\n", SDL_GetError() );
		}
		FillStaticVidModes( modeList );
	}

	return true;
	// DG end

#else // SDL 1

	// DG: SDL1 only knows of one display - some functions rely on
	// R_GetModeListForDisplay() returning false for invalid displaynum to iterate all displays
	if( requestedDisplayNum >= 1 )
	{
		return false;
	}
	// DG end

	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();
	if( videoInfo == NULL )
	{
		// DG: yes, this can actually fail, e.g. if SDL_Init( SDL_INIT_VIDEO ) wasn't called
		common->Warning( "Can't get Video Info, using default modes...\n" );
		FillStaticVidModes( modeList );
		return true;
	}

	SDL_Rect** modes = SDL_ListModes( videoInfo->vfmt, SDL_OPENGL | SDL_FULLSCREEN );

	if( !modes )
	{
		common->Warning( "Can't get list of available modes, using default ones...\n" );
		FillStaticVidModes( modeList );
		return true;
	}

	if( modes == ( SDL_Rect** ) - 1 )
	{
		common->Printf( "Display supports any resolution\n" );
		FillStaticVidModes( modeList );
		return true;
	}

	int numModes;
	for( numModes = 0; modes[numModes]; numModes++ );

	if( numModes > 1 )
	{
		for( int i = 0; i < numModes; i++ )
		{
			vidMode_t mode;
			mode.width =  modes[i]->w;
			mode.height =  modes[i]->h;
			mode.displayHz = 60; // FIXME;
			modeList.AddUnique( mode );
		}

		// sort with lowest resolution first
		modeList.SortWithTemplate( idSort_VidMode() );

		return true;
	}

	return false;
#endif
}
