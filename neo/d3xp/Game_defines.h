/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#ifndef __GAME_DEFINES_H__
#define	__GAME_DEFINES_H__

#include "../sys/sys_session.h"  // for MAX_PLAYERS, etc
#include "../renderer/RenderWorld.h"  // for PS_BLOCK_ALL

class idRenderWorld;
class idSoundWorld;

#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
// This is real evil but allows the code to inspect arbitrary class variables.
#define private		public
#define protected	public
#endif

extern idRenderWorld* 				gameRenderWorld;
extern idSoundWorld* 				gameSoundWorld;

// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"baseDOOM-1"


const int MAX_CLIENTS			= MAX_PLAYERS;
const int MAX_CLIENTS_IN_PVS	= MAX_CLIENTS >> 3;
const int GENTITYNUM_BITS		= 12;
const int MAX_GENTITIES			= 1 << GENTITYNUM_BITS;
const int ENTITYNUM_NONE		= MAX_GENTITIES - 1;
const int ENTITYNUM_WORLD		= MAX_GENTITIES - 2;
const int ENTITYNUM_MAX_NORMAL	= MAX_GENTITIES - 2;
const int ENTITYNUM_FIRST_NON_REPLICATED	= ENTITYNUM_MAX_NORMAL - 256;

//============================================================================

void gameError( const char* fmt, ... );

//============================================================================

const int MAX_GAME_MESSAGE_SIZE		= 8192;
const int MAX_ENTITY_STATE_SIZE		= 512;
const int ENTITY_PVS_SIZE			= ( ( MAX_GENTITIES + 31 ) >> 5 );
const int NUM_RENDER_PORTAL_BITS	= idMath::BitsForInteger( PS_BLOCK_ALL );

const int MAX_EVENT_PARAM_SIZE		= 128;

#endif
