/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012 Robert Beckebans
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
along with Doom 3 Source Code.	If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL.h>

#include "renderer/RenderCommon.h"
#include "sdl_local.h"
#include "../posix/posix_public.h"

// DG: those are needed for moving/resizing windows
extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;
// DG end

const char* kbdNames[] =
{
	"english", "french", "german", "italian", "spanish", "turkish", "norwegian", NULL
};

idCVar in_keyboard( "in_keyboard", "english", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_NOCHEAT, "keyboard layout", kbdNames, idCmdSystem::ArgCompletion_String<kbdNames> );

struct kbd_poll_t
{
	int key;
	bool state;

	kbd_poll_t()
	{
	}

	kbd_poll_t( int k, bool s )
	{
		key = k;
		state = s;
	}
};

struct mouse_poll_t
{
	int action;
	int value;

	mouse_poll_t()
	{
	}

	mouse_poll_t( int a, int v )
	{
		action = a;
		value = v;
	}
};

static idList<kbd_poll_t> kbd_polls;
static idList<mouse_poll_t> mouse_polls;

struct joystick_poll_t
{
	int action;
	int value;

	joystick_poll_t()
	{
	}

	joystick_poll_t( int a, int v )
	{
		action = a;
		value = v;
	}
};
static idList<joystick_poll_t> joystick_polls;
SDL_Joystick* joy = NULL;
int SDL_joystick_has_hat = 0;
bool buttonStates[K_LAST_KEY];	// For keeping track of button up/down events


#include "sdl2_scancode_mappings.h"

static int SDLScanCodeToKeyNum( SDL_Scancode sc )
{
	int idx = int( sc );
	assert( idx >= 0 && idx < SDL_NUM_SCANCODES );

	return scanCodeToKeyNum[idx];
}

static SDL_Scancode KeyNumToSDLScanCode( int keyNum )
{
	if( keyNum < K_JOY1 )
	{
		for( int i = 0; i < SDL_NUM_SCANCODES; ++i )
		{
			if( scanCodeToKeyNum[i] == keyNum )
			{
				return SDL_Scancode( i );
			}
		}
	}
	return SDL_SCANCODE_UNKNOWN;
}

// both strings are expected to have at most SDL_TEXTINPUTEVENT_TEXT_SIZE chars/ints (including terminating null)
static void ConvertUTF8toUTF32( const char* utf8str, int32* utf32buf )
{
	static SDL_iconv_t cd = SDL_iconv_t( -1 );

	if( cd == SDL_iconv_t( -1 ) )
	{
		const char* toFormat = "UTF-32LE"; // TODO: what does d3bfg expect on big endian machines?
		cd = SDL_iconv_open( toFormat, "UTF-8" );
		if( cd == SDL_iconv_t( -1 ) )
		{
			common->Warning( "Couldn't initialize SDL_iconv for UTF-8 to UTF-32!" ); // TODO: or error?
			return;
		}
	}

	size_t len = strlen( utf8str );

	size_t inbytesleft = len;
	size_t outbytesleft = 4 * SDL_TEXTINPUTEVENT_TEXT_SIZE; // *4 because utf-32 needs 4x as much space as utf-8
	char* outbuf = ( char* )utf32buf;
	size_t n = SDL_iconv( cd, &utf8str, &inbytesleft, &outbuf, &outbytesleft );

	if( n == size_t( -1 ) ) // some error occured during iconv
	{
		common->Warning( "Converting UTF-8 string \"%s\" from SDL_TEXTINPUT to UTF-32 failed!", utf8str );

		// clear utf32-buffer, just to be sure there's no garbage..
		memset( utf32buf, 0, SDL_TEXTINPUTEVENT_TEXT_SIZE * sizeof( int32 ) );
	}

	// reset cd so it can be used again
	SDL_iconv( cd, NULL, &inbytesleft, NULL, &outbytesleft );

}

static void PushConsoleEvent( const char* s )
{
	char* b;
	size_t len;

	len = strlen( s ) + 1;
	b = ( char* )Mem_Alloc( len, TAG_EVENTS );
	strcpy( b, s );

	SDL_Event event;

	event.type = SDL_USEREVENT;
	event.user.code = SE_CONSOLE;
	event.user.data1 = ( void* )len;
	event.user.data2 = b;

	SDL_PushEvent( &event );
}



/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput()
{
	int numJoysticks, i;

	kbd_polls.SetGranularity( 64 );
	mouse_polls.SetGranularity( 64 );

	memset( buttonStates, 0, sizeof( buttonStates ) );

	in_keyboard.SetModified();

	// GameController
	if( SDL_Init( SDL_INIT_GAMECONTROLLER ) )
	{
		common->Printf( "Sys_InitInput: SDL_INIT_GAMECONTROLLER error: %s\n", SDL_GetError() );
	}

	SDL_GameController* controller = NULL;
	for( int i = 0; i < SDL_NumJoysticks(); ++i )
	{
		if( SDL_IsGameController( i ) )
		{
			controller = SDL_GameControllerOpen( i );
			if( controller )
			{
				common->Printf( "GameController %i name: %s\n", i, SDL_GameControllerName( controller ) );
				common->Printf( "GameController %i is mapped as \"%s\".\n", i, SDL_GameControllerMapping( controller ) );
			}
			else
			{
				common->Printf( "Could not open gamecontroller %i: %s\n", i, SDL_GetError() );
			}

		}
	}
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput()
{
	kbd_polls.Clear();
	mouse_polls.Clear();
	joystick_polls.Clear();

	memset( buttonStates, 0, sizeof( buttonStates ) );

	// Close any opened SDL Joystic
	if( joy )
	{
		common->Printf( "Sys_ShutdownInput: closing SDL joystick.\n" );
		SDL_JoystickClose( joy );
	}
	else
	{
		common->Printf( "Sys_ShutdownInput: SDL joystick not initialized. Nothing to close.\n" );
	}
}

/*
===========
Sys_InitScanTable
===========
*/
// Windows has its own version due to the tools
#ifndef _WIN32
void Sys_InitScanTable()
{
}
#endif

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey( bool shifted )
{
	static unsigned char keys[2] = { '`', '~' };

	if( in_keyboard.IsModified() )
	{
		idStr lang = in_keyboard.GetString();

		if( lang.Length() )
		{
			if( !lang.Icmp( "french" ) )
			{
				keys[0] = '<';
				keys[1] = '>';
			}
			else if( !lang.Icmp( "german" ) )
			{
				keys[0] = '^';
				keys[1] = 176; // °
			}
			else if( !lang.Icmp( "italian" ) )
			{
				keys[0] = '\\';
				keys[1] = '|';
			}
			else if( !lang.Icmp( "spanish" ) )
			{
				keys[0] = 186; // º
				keys[1] = 170; // ª
			}
			else if( !lang.Icmp( "turkish" ) )
			{
				keys[0] = '"';
				keys[1] = 233; // é
			}
			else if( !lang.Icmp( "norwegian" ) )
			{
				keys[0] = 124; // |
				keys[1] = 167; // §
			}
		}

		in_keyboard.ClearModified();
	}

	return shifted ? keys[1] : keys[0];
}

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey( int key )
{
	return key & 0xff;
}

/*
===============
Sys_GrabMouseCursor
===============
*/
void Sys_GrabMouseCursor( bool grabIt )
{
	int flags;

	if( grabIt )
	{
		// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled
		flags = GRAB_ENABLE | GRAB_SETSTATE;
		// DG end
	}
	else
	{
		flags = GRAB_SETSTATE;
	}
// SRS - Generalized Vulkan SDL platform
#if defined( VULKAN_USE_PLATFORM_SDL )
	VKimp_GrabInput( flags );
#else
	GLimp_GrabInput( flags );
#endif
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent()
{
	sysEvent_t res = { };

	SDL_Event ev;
	int key;

	// when this is returned, it's assumed that there are no more events!
	static const sysEvent_t no_more_events = { SE_NONE, 0, 0, 0, NULL };

	// WM0110: previous state of joystick hat
	static int previous_hat_state = SDL_HAT_CENTERED;

	// utf-32 version of the textinput event
	static int32 uniStr[SDL_TEXTINPUTEVENT_TEXT_SIZE] = {0};
	static size_t uniStrPos = 0;

	if( uniStr[0] != 0 )
	{
		res.evType = SE_CHAR;
		res.evValue = uniStr[uniStrPos];

		++uniStrPos;

		if( !uniStr[uniStrPos] || uniStrPos == SDL_TEXTINPUTEVENT_TEXT_SIZE )
		{
			memset( uniStr, 0, sizeof( uniStr ) );
			uniStrPos = 0;
		}

		return res;
	}

	// DG: fake a "mousewheel not pressed anymore" event for SDL2
	// so scrolling in menus stops after one step
	static int mwheelRel = 0;
	if( mwheelRel )
	{
		res.evType = SE_KEY;
		res.evValue = mwheelRel;
		res.evValue2 = 0; // "not pressed anymore"
		mwheelRel = 0;
		return res;
	}
	// DG end

	static int32 uniChar = 0;

	if( uniChar )
	{
		res.evType = SE_CHAR;
		res.evValue = uniChar;

		uniChar = 0;

		return res;
	}

	// loop until there is an event we care about (will return then) or no more events
	while( SDL_PollEvent( &ev ) )
	{
		switch( ev.type )
		{
			case SDL_WINDOWEVENT:
				switch( ev.window.event )
				{
					case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
						// unset modifier, in case alt-tab was used to leave window and ALT is still set
						// as that can cause fullscreen-toggling when pressing enter...
						SDL_Keymod currentmod = SDL_GetModState();
						int newmod = KMOD_NONE;
						if( currentmod & KMOD_CAPS ) // preserve capslock
						{
							newmod |= KMOD_CAPS;
						}

						SDL_SetModState( ( SDL_Keymod )newmod );

						// DG: un-pause the game when focus is gained, that also re-grabs the input
						//     disabling the cursor is now done once in GLimp_Init() because it should always be disabled
						cvarSystem->SetCVarBool( "com_pause", false );
						cvarSystem->SetCVarBool( "com_activeApp", true );
						// DG end
						break;
					}

					case SDL_WINDOWEVENT_FOCUS_LOST:
						// DG: pause the game when focus is lost, that also un-grabs the input
						cvarSystem->SetCVarBool( "com_pause", true );
						cvarSystem->SetCVarBool( "com_activeApp", false );
						// DG end
						break;

					case SDL_WINDOWEVENT_LEAVE:
						// mouse has left the window
						res.evType = SE_MOUSE_LEAVE;
						return res;

					// DG: handle resizing and moving of window
					case SDL_WINDOWEVENT_RESIZED:
					{
						int w = ev.window.data1;
						int h = ev.window.data2;

						// SRS - Only save window resized events when in windowed mode
						if( !renderSystem->IsFullScreen() )
						{
							// SRS - If window was maximized by window manager set to borderless fullscreen mode = -2
							SDL_Window* window = SDL_GetWindowFromID( ev.window.windowID );
							if( SDL_GetWindowFlags( window ) & SDL_WINDOW_MAXIMIZED )
							{
								SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP );

								glConfig.isFullscreen = -2;
								cvarSystem->SetCVarInteger( "r_fullscreen", glConfig.isFullscreen );
							}
							else
							{
								r_windowWidth.SetInteger( w );
								r_windowHeight.SetInteger( h );
							}
						}

						glConfig.nativeScreenWidth = w;
						glConfig.nativeScreenHeight = h;

						// SRS - Make sure ImGui gets new window boundaries
						ImGuiHook::NotifyDisplaySizeChanged( glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
						break;
					}

					case SDL_WINDOWEVENT_MOVED:
					{
						int x = ev.window.data1;
						int y = ev.window.data2;

						// SRS - Only save window moved events when in windowed mode and not maximized by window manager
						SDL_Window* window = SDL_GetWindowFromID( ev.window.windowID );
						if( !renderSystem->IsFullScreen() && !( SDL_GetWindowFlags( window ) & SDL_WINDOW_MAXIMIZED ) )
						{
							// SRS - take window border into account when when saving window position cvars
							int topBorder, leftBorder, bottomBorder, rightBorder;
							SDL_Window* window = SDL_GetWindowFromID( ev.window.windowID );
							SDL_GetWindowBordersSize( window, &topBorder, &leftBorder, &bottomBorder, &rightBorder );
							r_windowX.SetInteger( x - leftBorder );
							r_windowY.SetInteger( y - topBorder );
						}
						break;
					}
				}

				continue; // handle next event

			case SDL_KEYDOWN:
				if( ev.key.keysym.sym == SDLK_RETURN && ( ev.key.keysym.mod & KMOD_ALT ) > 0 )
				{
					// DG: go to fullscreen on current display, instead of always first display
					int fullscreen = 0;
					if( !renderSystem->IsFullScreen() )
					{
						// this will be handled as "fullscreen on current window"
						// r_fullscreen 1 means "fullscreen on first window" in d3 bfg
						fullscreen = -2;
					}
					cvarSystem->SetCVarInteger( "r_fullscreen", fullscreen );
					// DG end
					PushConsoleEvent( "vid_restart" );
					continue; // handle next event
				}

				// DG: ctrl-g to un-grab mouse - yeah, left ctrl shoots, then just use right ctrl :)
				if( ev.key.keysym.sym == SDLK_g && ( ev.key.keysym.mod & KMOD_CTRL ) > 0 )
				{
					bool grab = cvarSystem->GetCVarBool( "in_nograb" );
					grab = !grab;
					cvarSystem->SetCVarBool( "in_nograb", grab );
					continue; // handle next event
				}
			// DG end

			// fall through
			case SDL_KEYUP:
			{
				bool isChar;

				// DG: special case for SDL_SCANCODE_GRAVE - the console key under Esc
				if( ev.key.keysym.scancode == SDL_SCANCODE_GRAVE )
				{
					key = K_GRAVE;
					uniChar = K_BACKSPACE; // bad hack to get empty console inputline..
				} // DG end, the original code is in the else case
				else
				{
					key = SDLScanCodeToKeyNum( ev.key.keysym.scancode );

					if( key == 0 )
					{
						// SDL2 has no ev.key.keysym.unicode anymore.. but the scancode should work well enough for console
						if( ev.type == SDL_KEYDOWN ) // FIXME: don't complain if this was an ASCII char and the console is open?
						{
							common->Warning( "unmapped SDL key %d scancode %d", ev.key.keysym.sym, ev.key.keysym.scancode );
						}

						continue; // just handle next event
					}
				}

				res.evType = SE_KEY;
				res.evValue = key;
				res.evValue2 = ev.key.state == SDL_PRESSED ? 1 : 0;

				kbd_polls.Append( kbd_poll_t( key, ev.key.state == SDL_PRESSED ) );

				if( key == K_BACKSPACE && ev.key.state == SDL_PRESSED )
				{
					uniChar = key;
				}

				return res;
			}

			case SDL_TEXTINPUT:
				if( ev.text.text[0] != '\0' )
				{
					// fill uniStr array for SE_CHAR events
					ConvertUTF8toUTF32( ev.text.text, uniStr );

					// return an event with the first/only char
					res.evType = SE_CHAR;
					res.evValue = uniStr[0];

					uniStrPos = 1;

					if( uniStr[1] == 0 )
					{
						// it's just this one character, clear uniStr
						uniStr[0] = 0;
						uniStrPos = 0;
					}
					return res;
				}

				continue; // just handle next event

			case SDL_MOUSEMOTION:
				// DG: return event with absolute mouse-coordinates when in menu
				// to fix cursor problems in windowed mode
				if( game && ( game->Shell_IsActive() || ImGuiTools::ReleaseMouseForTools() ) )
				{
					res.evType = SE_MOUSE_ABSOLUTE;
					res.evValue = ev.motion.x;
					res.evValue2 = ev.motion.y;
				}
				else     // this is the old, default behavior
				{
					res.evType = SE_MOUSE;
					res.evValue = ev.motion.xrel;
					res.evValue2 = ev.motion.yrel;
				}
				// DG end

				mouse_polls.Append( mouse_poll_t( M_DELTAX, ev.motion.xrel ) );
				mouse_polls.Append( mouse_poll_t( M_DELTAY, ev.motion.yrel ) );

				return res;

			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
				continue; // Avoid 'unknown event' spam when testing with touchpad by skipping this

			case SDL_MOUSEWHEEL:
				res.evType = SE_KEY;

				res.evValue = ( ev.wheel.y > 0 ) ? K_MWHEELUP : K_MWHEELDOWN;
				mouse_polls.Append( mouse_poll_t( M_DELTAZ, ev.wheel.y ) );

				res.evValue2 = 1; // for "pressed"

				// remember mousewheel direction to issue a "not pressed anymore" event
				mwheelRel = res.evValue;

				return res;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				res.evType = SE_KEY;

				switch( ev.button.button )
				{
					case SDL_BUTTON_LEFT:
						res.evValue = K_MOUSE1;
						mouse_polls.Append( mouse_poll_t( M_ACTION1, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						break;

					case SDL_BUTTON_MIDDLE:
						res.evValue = K_MOUSE3;
						mouse_polls.Append( mouse_poll_t( M_ACTION3, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						break;

					case SDL_BUTTON_RIGHT:
						res.evValue = K_MOUSE2;
						mouse_polls.Append( mouse_poll_t( M_ACTION2, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						break;

					default:
						// handle X1 button and above
						if( ev.button.button <= 16 ) // d3bfg doesn't support more than 16 mouse buttons
						{
							int buttonIndex = ev.button.button - SDL_BUTTON_LEFT;
							res.evValue = K_MOUSE1 + buttonIndex;
							mouse_polls.Append( mouse_poll_t( M_ACTION1 + buttonIndex, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						}
						else // unsupported mouse button
						{
							continue; // just ignore
						}
				}

				res.evValue2 = ev.button.state == SDL_PRESSED ? 1 : 0;

				return res;

			// GameController
			case SDL_JOYAXISMOTION:
			case SDL_JOYBALLMOTION:
			case SDL_JOYHATMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
			case SDL_JOYBATTERYUPDATED:
				// Avoid 'unknown event' spam
				continue;

			case SDL_CONTROLLERAXISMOTION:
			{
				extern idCVar joy_deadZone;
				static int controllerAxisRemap[][2] =
				{
					{K_JOY_STICK1_LEFT, K_JOY_STICK1_RIGHT},	// Left Stick X Axis: SDL_CONTROLLER_AXIS_LEFTX
					{K_JOY_STICK1_UP, K_JOY_STICK1_DOWN},		// Left Stick Y Axis: SDL_CONTROLLER_AXIS_LEFTY
					{K_JOY_STICK2_LEFT, K_JOY_STICK2_RIGHT},	// Right Stick X Axis: SDL_CONTROLLER_AXIS_RIGHTX
					{K_JOY_STICK2_UP, K_JOY_STICK2_DOWN},		// Right Stick Y Axis: SDL_CONTROLLER_AXIS_RIGHTY
					{K_NONE, K_NONE},							// Null padding: SDL_CONTROLLER_AXIS_TRIGGERLEFT
					{K_NONE, K_NONE},							// Null padding: SDL_CONTROLLER_AXIS_TRIGGERRIGHT
				};

				res.evType = SE_JOYSTICK;
				res.evValue = J_AXIS_LEFT_X + ( ev.caxis.axis - SDL_CONTROLLER_AXIS_LEFTX );
				res.evValue2 = ev.caxis.value;

				joystick_polls.Append( joystick_poll_t( res.evValue, res.evValue2 ) );

				// SRS - Synthesize joystick axis key presses to enable navigation in game menus and the PDA
				if( ev.caxis.axis <= SDL_CONTROLLER_AXIS_RIGHTY )
				{
					int axisIndex = ( ev.caxis.axis - SDL_CONTROLLER_AXIS_LEFTX );
					bool stickNeg = static_cast<float>( ev.caxis.value ) / 32767.0f < - joy_deadZone.GetFloat();
					bool stickPos = static_cast<float>( ev.caxis.value ) / 32767.0f >   joy_deadZone.GetFloat();

					if( buttonStates[controllerAxisRemap[axisIndex][0]] != stickNeg )
					{
						buttonStates[controllerAxisRemap[axisIndex][0]] = stickNeg;
						res.evType = SE_KEY;
						res.evValue = controllerAxisRemap[axisIndex][0];
						res.evValue2 = stickNeg ? 1 : 0;
					}
					else if( buttonStates[controllerAxisRemap[axisIndex][1]] != stickPos )
					{
						buttonStates[controllerAxisRemap[axisIndex][1]] = stickPos;
						res.evType = SE_KEY;
						res.evValue = controllerAxisRemap[axisIndex][1];
						res.evValue2 = stickPos ? 1 : 0;
					}
				}
				return res;
			}
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				static int controllerButtonRemap[][2] =
				{
					{K_JOY1, J_ACTION1},	// A
					{K_JOY2, J_ACTION2},	// B
					{K_JOY3, J_ACTION3},	// X
					{K_JOY4, J_ACTION4},	// Y
					{K_JOY10, J_ACTION10},	// Back / View
					{K_JOY11, J_ACTION11},	// Guide / Xbox button (shouldn't be used)
					{K_JOY9, J_ACTION9},	// Start / Menu
					{K_JOY7, J_ACTION7},	// Left Stick Down
					{K_JOY8, J_ACTION8},	// Right Stick Down
					{K_JOY5, J_ACTION5},	// LB / L1
					{K_JOY6, J_ACTION6},	// RB / R1
					{K_JOY_DPAD_UP, J_DPAD_UP},
					{K_JOY_DPAD_DOWN, J_DPAD_DOWN},
					{K_JOY_DPAD_LEFT, J_DPAD_LEFT},
					{K_JOY_DPAD_RIGHT, J_DPAD_RIGHT},
				};

				res.evType = SE_KEY;
				res.evValue = controllerButtonRemap[ev.cbutton.button][0];
				res.evValue2 = ev.cbutton.state == SDL_PRESSED ? 1 : 0;

				joystick_polls.Append( joystick_poll_t( controllerButtonRemap[ev.cbutton.button][1], res.evValue2 ) );
				return res;

			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
			case SDL_CONTROLLERDEVICEREMAPPED:
			case SDL_CONTROLLERTOUCHPADDOWN:
			case SDL_CONTROLLERTOUCHPADMOTION:
			case SDL_CONTROLLERTOUCHPADUP:
			case SDL_CONTROLLERSENSORUPDATE:
				//case SDL_CONTROLLERUPDATECOMPLETE_RESERVED_FOR_SDL3:
				//case SDL_CONTROLLERSTEAMHANDLEUPDATED:
				// Avoid more 'unknown event' spam
				continue;

			case SDL_QUIT:
				PushConsoleEvent( "quit" );
				res = no_more_events; // don't handle next event, just quit.
				return res;

			case SDL_USEREVENT:
				switch( ev.user.code )
				{
					case SE_CONSOLE:
						res.evType = SE_CONSOLE;
						res.evPtrLength = ( intptr_t )ev.user.data1;
						res.evPtr = ev.user.data2;
						return res;
					default:
						common->Warning( "unknown user event %u", ev.user.code );
				}
				continue; // just handle next event

			// Avoid 'unknown event' spam
			case SDL_TEXTEDITING:
			case SDL_KEYMAPCHANGED:
			case SDL_CLIPBOARDUPDATE:
				continue; // just handle next event

			default:
				common->Warning( "unknown event %u = %#x", ev.type, ev.type );
				continue; // just handle next event
		}
	}

	res = no_more_events;
	return res;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents()
{
	SDL_Event ev;

	while( SDL_PollEvent( &ev ) )
		;

	kbd_polls.SetNum( 0 );
	mouse_polls.SetNum( 0 );
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents()
{
	char* s = Posix_ConsoleInput();

	if( s )
	{
		PushConsoleEvent( s );
	}

	SDL_PumpEvents();
}

/*
================
Sys_PollKeyboardInputEvents
================
*/
int Sys_PollKeyboardInputEvents()
{
	return kbd_polls.Num();
}

/*
================
Sys_ReturnKeyboardInputEvent
================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int& key, bool& state )
{
	if( n >= kbd_polls.Num() )
	{
		return 0;
	}

	key = kbd_polls[n].key;
	state = kbd_polls[n].state;
	return 1;
}

/*
================
Sys_EndKeyboardInputEvents
================
*/
void Sys_EndKeyboardInputEvents()
{
	kbd_polls.SetNum( 0 );
}

/*
================
Sys_PollMouseInputEvents
================
*/
int Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] )
{
	int numEvents = mouse_polls.Num();

	if( numEvents > MAX_MOUSE_EVENTS )
	{
		numEvents = MAX_MOUSE_EVENTS;
	}

	for( int i = 0; i < numEvents; i++ )
	{
		const mouse_poll_t& mp = mouse_polls[i];

		mouseEvents[i][0] = mp.action;
		mouseEvents[i][1] = mp.value;
	}

	mouse_polls.SetNum( 0 );

	return numEvents;
}

const char* Sys_GetKeyName( keyNum_t keynum )
{
	// unfortunately, in SDL1.2 there is no way to get the keycode for a scancode, so this doesn't work there.
	// so this is SDL2-only.
	SDL_Scancode scancode = KeyNumToSDLScanCode( ( int )keynum );
	SDL_Keycode keycode = SDL_GetKeyFromScancode( scancode );

	const char* ret = SDL_GetKeyName( keycode );
	if( ret != NULL && ret[0] != '\0' )
	{
		return ret;
	}

	return NULL;
}

char* Sys_GetClipboardData()
{
	char* txt = SDL_GetClipboardText();

	if( txt == NULL )
	{
		return NULL;
	}
	else if( txt[0] == '\0' )
	{
		SDL_free( txt );
		return NULL;
	}

	char* ret = Mem_CopyString( txt );
	SDL_free( txt );

	return ret;
}

void Sys_SetClipboardData( const char* string )
{
	SDL_SetClipboardText( string );
}


//=====================================================================================
//	Joystick Input Handling
//=====================================================================================

void Sys_SetRumble( int device, int low, int hi )
{
	// TODO;
	// SDL 2.0 required (SDL Haptic subsystem)
}

int Sys_PollJoystickInputEvents( int deviceNum )
{
	int numEvents = joystick_polls.Num();

	return numEvents;
}

// This funcion called by void idUsercmdGenLocal::Joystick( int deviceNum ) in
// file UsercmdGen.cpp
// action - must have values belonging to enum sys_jEvents (sys_public.h)
// value - must be 1/0 for button or DPAD pressed/released
//         for joystick axes must be in the range (-32769, 32768)
//         for joystick trigger must be in the range (0, 32768)
int Sys_ReturnJoystickInputEvent( const int n, int& action, int& value )
{
	// Get last element of the list and copy into argument references
	const joystick_poll_t& mp = joystick_polls[n];
	action = mp.action;
	value = mp.value;

	return 1;
}

// This funcion called by void idUsercmdGenLocal::Joystick( int deviceNum ) in
// file UsercmdGen.cpp
void Sys_EndJoystickInputEvents()
{
	// Empty the joystick event container. This is called after
	// all joystick events have been read using Sys_ReturnJoystickInputEvent()
	joystick_polls.SetNum( 0 );
}
